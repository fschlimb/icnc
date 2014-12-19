#include <src/dist/distributed_scheduler.h>
#include <cnc/serializer.h>
#include <cnc/internal/context_base.h>
#include <cnc/internal/scheduler_i.h>
#include <cnc/internal/dist/distributor.h>
#include <cnc/internal/step_instance_base.h>
#include <cnc/internal/dist/distributable_context.h>
#include <cnc/internal/dist/distributable.h>

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
        distributed_scheduler* distributed_scheduler::instance;

        distributed_scheduler::distributed_scheduler( context_base & ctxt, scheduler_i & scheduler )
            : m_scheduler( scheduler ),
              m_context( ctxt )
              
        {
            Eo("distributed_scheduler::distributed_scheduler");
            instance = this;
            m_context.subscribe( this );
            std::cerr << "my rank = " << distributor::myPid() << std::endl;
            m_topo.init_ring(distributor::numProcs(),distributor::myPid());
        }

        distributed_scheduler::~distributed_scheduler()
        {
            instance = NULL;
            m_context.unsubscribe( this );
        }

        /* virtual */ void distributed_scheduler::recv_msg( CnC::serializer* ser )
        {
            char marker;
            int senderId;
            (*ser) & marker & senderId;
            Eo(int(marker));Eo(int(WORK_REQUEST));
            if (marker == WORK_REQUEST) {
                recv_work_request( ser, senderId );
            } else if (marker == WORK_CHUNK){
                recv_steps( *ser );
                on_received_workchunk( ser, senderId );
            } else if (marker == STATE_UPDATE){
                recv_state_update( ser, senderId );
            } else {
                CNC_ASSERT("Impossible code path" == 0);
            }
        }

        bool distributed_scheduler::bcast_work_request()
        {
            {Speaker spkr; spkr << "distributed_scheduler::bcast_work_request()";}
            CNC_ASSERT( m_topo.neighbors().size() );

            {Speaker spkr; spkr << "distributed_scheduler::bcast_work_request() getting serializer";}
            CnC::serializer* ser = m_context.new_serializer( this );
            {Speaker spkr; spkr << "distributed_scheduler::bcast_work_request() got serializer";}
            char marker = WORK_REQUEST;
            int myId = distributor::myPid();
            (*ser) & marker & myId;
            {Speaker spkr; spkr << "distributed_scheduler::bcast_work_request() before bcast";}
            m_context.bcast_msg( ser, m_topo.neighbors().data(), m_topo.neighbors().size() );
            //m_context.bcast_msg( ser );
            {Speaker spkr; spkr << "distributed_scheduler: done bcast_work_request()";}
            return true;
        }

        bool distributed_scheduler::send_work_request( int receiver )
        {
            CnC::serializer* ser = m_context.new_serializer( this );
            char marker = WORK_REQUEST;
            int myId = distributor::myPid();
            (*ser) & marker & myId;
            m_context.send_msg( ser, receiver );
            return true;
        }

        void distributed_scheduler::serialize_step( CnC::serializer & ser, schedulable & step )
        {
            CNC_ASSERT(dynamic_cast<step_instance_base*>(&step));
            step.serialize( ser );
        }

        // /* virtual */ void distributed_scheduler::activate()
        // {
        //     Eo("activation");
        //     loadBalanceCallback();
        // }

        void distributed_scheduler::send_steps_to_client( int clientId, schedulable ** steps, size_t n )
        {
            CnC::serializer* ser = m_context.new_serializer( this );
            char marker = WORK_CHUNK;
            int myId = distributor::myPid();
            (*ser) & marker & myId & n;
            while( n-- ) {
                serialize_step( *ser, *steps[n] );
            }
            m_context.send_msg( ser, clientId );
        }

        void distributed_scheduler::recv_steps( CnC::serializer & ser )
        {
            step_vec_type::size_type sz;
            ser & sz;
            while( sz-- ) {
                m_context.recv_msg( &ser );
            }
        }

        void distributed_scheduler::bcast_state_update( int me, int value )
        {
            // what's this gid business for?
            int g = m_context.gid();
            if (g < 0) {
                return;
            }

            CnC::serializer* ser = m_context.new_serializer( this );
            char marker = STATE_UPDATE;
            (*ser) & marker & me & value;
            m_context.bcast_msg( ser, m_topo.neighbors().data(), m_topo.neighbors().size() );
            //m_context.bcast_msg( ser );
        }

    } // namespace Internal
} // namespace CnC
