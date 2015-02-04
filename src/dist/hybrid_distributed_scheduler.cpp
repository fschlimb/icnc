/* *******************************************************************************
 *  Copyright (c) 2010-2014, Intel Corporation
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************************/

#include <src/dist/hybrid_distributed_scheduler.h>
#include <cnc/internal/dist/distributor.h>
#include <cnc/internal/scheduler_i.h>

#include <iostream>
#include <sstream>
#if defined(_DEBUG)
#define Eo(x) { Speaker spkr; spkr << " " << #x << " = " << (x); }
#else
#define Eo(x)
#endif

namespace CnC {
    namespace Internal
    {

        hybrid_distributed_scheduler::hybrid_distributed_scheduler( context_base & ctxt, scheduler_i & scheduler )
            : distributed_scheduler(ctxt, scheduler)
        {
            m_last = 0;
            int numProcs = distributor::numProcs();
            m_clientRequests = new tbb::atomic<int>[numProcs];
            m_sentRequests = new tbb::atomic<int>[numProcs];
            m_clientState = new tbb::atomic<long>[numProcs];
            for (int i = 0; i < numProcs; i++) m_clientRequests[i] = 0;
            for (int i = 0; i < numProcs; i++) m_sentRequests[i] = 0;
            for (int i = 0; i < numProcs; i++) m_clientState[i] = 0;
            //            progress();
            std::cerr << "Hybrid" << std::endl;
        }

        hybrid_distributed_scheduler::~hybrid_distributed_scheduler(){
            delete[] m_sentRequests; m_sentRequests = 0;
            delete[] m_clientRequests; m_clientRequests = 0;
            delete[] m_clientState; m_clientState = 0;
        }

        void hybrid_distributed_scheduler::do_bcast( unsigned int nsif )
        {
            bcast_state_update( distributor::myPid(), nsif );
        }

        bool hybrid_distributed_scheduler::needs_bcast( int nsif )
        {
            int z = nsif&((1<<LOG_BCAST_FREQUENCY)-1);
            return z == 0;
        }

        void hybrid_distributed_scheduler::progress( unsigned int nsif )
        {
            if( needs_bcast( nsif ) ) {
                //                {Speaker spkr; spkr << "hybrid_distributed_scheduler: need bcast";}
                do_bcast( nsif );
            }
            Eo(nsif);
            if( nsif < FEW_STEPS ){
                {Speaker spkr; spkr << "hybrid_distributed_scheduler: need more steps";}
                int numProcs = distributor::numProcs();
                int me = distributor::myPid();
                // synchronization is wrong
                if (m_sentRequests[0].compare_and_swap(1,0)==0){
                    if (!bcast_work_request()){
                        m_sentRequests[0] = 0;
                    }
                }
            }
        }

        /* virtual */ void hybrid_distributed_scheduler::recv_work_request( CnC::serializer* ser, int senderId ){
            Eo("stealing recv_work_request");

            // m_clientRequests[senderId] ^= 1
            // xor is not implemented for atomics, unfortunately
            int cnt = 0;
            for (;cnt<10;cnt++){
                if (m_clientRequests[senderId].compare_and_swap(1,0)==0){
                    break;
                }
                if (m_clientRequests[senderId].compare_and_swap(0,1)==1){
                    break;
                }
            }
            if (cnt>=10){
                std::cerr << "failed to update m_clientRequests[senderId]" << std::endl;
            }
        }

        void hybrid_distributed_scheduler::on_received_workchunk( CnC::serializer* ser, int senderId )
        {
            // synchronization is wrong
            {Speaker spkr; spkr << "hybrid_distributed_scheduler::on_received_workchunk";}
            if (this->m_scheduler.num_steps_in_flight() >= ENOUGH_STEPS){
                if (m_sentRequests[0].compare_and_swap(0,1)==1){
                    if (!bcast_work_request()){
                        m_sentRequests[0] = 1;
                    }
                }
            }
        }

        bool hybrid_distributed_scheduler::migrate_step( unsigned int nsif, schedulable * s )
        {
            progress( nsif );
            if( nsif <= ENOUGH_STEPS ) return false;
            Eo("many steps");
            int client = -1;
#if 1
            int n = distributor::numProcs();
            int me = distributor::myPid();
            int last = (m_last++)%n; //
            for (int i = (last+1)%n; i != last; i = (i+1)%n) if (i!=me){
                if (m_clientRequests[i]){
                    client = i;
                    break;
                }
            }
            if (client >= 0){
                m_clientState[client]++;
                send_steps_to_client( client, &s, 1 );
                return true;
            }
            for (int i = (last+1)%n; i != last; i = (i+1)%n) {
                if ((m_clientState[i])<(nsif>>2)){
                    client = i;
                    break;
                }
            }
            if (client >= 0){
                send_steps_to_client( client, &s, 1 );
                m_clientState[client]++;
                return true;
            }
#else
            int n = m_topo.neighboursCount();
            int last = (m_last++)%n; //
            for (int i = (last+1)%n; i != last; i = (i+1)%n) {
                int pid = m_topo.getNeighbour(i);
                if (m_clientRequests[pid]){
                    client = pid;
                    Eo(client);
                    break;
                }
            }
            if (client >= 0){
                send_steps_to_client( client, &s, 1 );
                return true;
            }
            for (int i = (last+1)%n; i != last; i = (i+1)%n) {
                int pid = m_topo.getNeighbour(i);
                if ((m_clientState[pid])<(cur>>2)){
                    send_steps_to_client( pid, &s, 1 );
                    m_clientState[pid]++;
                    return true;
                }
            }
#endif
            return false;
        }

    } // namespace Internal
} // namespace CnC
