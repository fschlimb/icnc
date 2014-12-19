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

#include <src/dist/sharing_distributed_scheduler.h>
#include <cnc/internal/dist/distributor.h>
#include <cnc/internal/scheduler_i.h>

#include <iostream>
#include <sstream>
#if defined(_DEBUG)
#define Eo(x) { std::stringstream ss; ss << "my pid = " << getpid() << " " << #x << " = " << (x) << std::endl; std::cerr << ss.str(); }
#else
#define Eo(x)
#endif

namespace CnC {
    namespace Internal
    {

        sharing_distributed_scheduler::sharing_distributed_scheduler( context_base & ctxt, scheduler_i & scheduler )
            : distributed_scheduler(ctxt, scheduler)
        {
            m_requestSent = 0;
            m_last = 0;
            if (distributor::myPid() == HOST){
                int numProcs = distributor::numProcs();
                Eo(numProcs);
                m_allClientRequests.set_capacity(numProcs);
                m_clientRequests = new tbb::atomic<int>[numProcs];
                // not needed?
                for (int i = 0; i < numProcs; i++) m_clientRequests[i] = 0;
            } else {
                m_clientRequests = NULL;
            }
            loadBalanceCallback();
            std::cerr << "Sharing" << std::endl;
        }

        sharing_distributed_scheduler::~sharing_distributed_scheduler(){
            if (distributor::myPid() == HOST){
                delete[] m_clientRequests;
            }
        }

        void sharing_distributed_scheduler::loadBalanceCallback(){
            int me = distributor::myPid();
            if (me != HOST){
                int localSteps = this->m_scheduler.num_steps_in_flight();
                if (localSteps < FEW_STEPS){
                    if (m_requestSent.compare_and_swap(1,0)==0){
                        if (!postRequest()){
                            m_requestSent = 0;
                        }
                    }
                } else {
                    if (m_requestSent.compare_and_swap(0,1)==1){
                        if (!postRequest()){
                            m_requestSent = 1;
                        }
                    }
                }
            }
        }

        bool sharing_distributed_scheduler::postRequest(){
            return send_work_request(HOST);
        }

        bool sharing_distributed_scheduler::hasClientRequests(int& clientId){
            if (!m_allClientRequests.try_pop(clientId)){
                return false;
            }
            Eo("use request from"); Eo(clientId);
            return true;
        }

        void sharing_distributed_scheduler::putClientRequest(int clientId){
            Eo("put request from");Eo(clientId);
            m_clientRequests[clientId] = 1;
//            m_allClientRequests.push(clientId);
        }

        /* virtual */ void sharing_distributed_scheduler::recv_work_request( CnC::serializer* ser, int senderId ){
            Eo("sharing recv_work_request");
            if (distributor::myPid() == HOST){
                putClientRequest( senderId );
            }
        }

        void sharing_distributed_scheduler::on_received_workchunk( CnC::serializer* ser, int senderId )
        {
        }

        /* virtual */ bool sharing_distributed_scheduler::migrate_step(unsigned int, schedulable* s){
            Eo("!");
//            return false;
            if (distributor::myPid() == HOST){
                if (this->m_scheduler.num_steps_in_flight() > HOST_FEW_STEPS){
                    Eo("many steps");
                    int client = -1;
                    int n= distributor::numProcs();
                    int last = (m_last++)%n;
                    for (int i = (last+1)%n; i != n; i++) if (i != HOST){
                        if (m_clientRequests[i]){
                            client = i;
                            break;
                        }
                    }
                    if (client < 0) return false;
                    send_steps_to_client( client, &s, 1 );
                    return true;
                }
            } else {
                if (this->m_scheduler.num_steps_in_flight() > LOTS_OF_STEPS){
                    send_steps_to_client( HOST, &s, 1 );
                    return true;
                }
            }
            return false;
        }

    } // namespace Internal
} // namespace CnC
