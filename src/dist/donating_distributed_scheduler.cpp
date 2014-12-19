#include <src/dist/donating_distributed_scheduler.h>
#include <cnc/internal/dist/distributor.h>
#include <cnc/internal/scheduler_i.h>
#include <cnc/serializer.h>

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

        donating_distributed_scheduler::donating_distributed_scheduler( context_base & ctxt, scheduler_i & scheduler )
            : distributed_scheduler(ctxt, scheduler)
        {
            m_last = 0;
            int n = distributor::numProcs();
            m_clientState = new tbb::atomic<long>[n];
            for (int i = 0; i < n; i++) m_clientState[i] = 0;
            loadBalanceCallback();
            std::cerr << "Donating" << std::endl;
        }

        donating_distributed_scheduler::~donating_distributed_scheduler(){
            delete[] m_clientState;
        }

        void donating_distributed_scheduler::loadBalanceCallback(){
            int old = m_bcastCounter.fetch_and_increment();
            if (needsBcast(old)){
                doBcast();
            }
        }

        bool donating_distributed_scheduler::needsBcast(int x){
            int z = x&((1<<LOG_BCAST_FREQUENCY)-1);
            return z == 0;
        }

        void donating_distributed_scheduler::on_received_workchunk( CnC::serializer* ser, int senderId )
        {
        }

        /* virtual */ bool donating_distributed_scheduler::migrate_step(unsigned int, schedulable* s){
            long cur = this->m_scheduler.num_steps_in_flight();
            if (cur < ENOUGH_STEPS) return false;
#if 0
            int n = m_topo.neighboursCount();
            for (int i = (last+1)%n; i != last; i = (i+1)%n) {
                int pid = m_topo.getNeighbour(i);
                if (m_clientState[pid]<(cur>>2)){
                    send_steps_to_client( pid, &s, 1 );
                    m_clientState[pid]++;
                    return true;
                }
            }
#endif
#if 1
            int n = distributor::numProcs();
            int last = (m_last++)%n;
            int me = distributor::myPid();
            for (int i = (last+1)%n; i != last; i = (i+1)%n) if (i != me){
            //for (int i = 0; i < n; i++) if (i != me){
                if ((m_clientState[i]<<2)<cur){
                    send_steps_to_client( i, &s, 1 );
                    m_clientState[i]++;
                    return true;
                }
            }
#endif
#if 0
            int n = distributor::numProcs();
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

        void donating_distributed_scheduler::doBcast(){
            bcast_state_update( distributor::myPid(), this->m_scheduler.num_steps_in_flight() );
        }

        /* virtual */ void donating_distributed_scheduler::recv_state_update( CnC::serializer* ser, int senderId ){
            int value;
            (*ser) & value;
            m_clientState[senderId] = value;
        }

    } // namespace Internal
} // namespace CnC
