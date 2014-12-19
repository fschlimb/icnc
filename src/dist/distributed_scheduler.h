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

#ifndef _CnC_DISTRIBUTED_SCHEDULER_H
#define _CnC_DISTRIBUTED_SCHEDULER_H

#include <cnc/internal/dist/distributable.h>
#include <src/dist/topology.h>

namespace CnC {

    class serializer;

    namespace Internal {

        class context_base;
        class scheduler_i;
        class schedulable;

        /// Scheduling capabilites for distributed systems.
        ///
        /// Currently this implements load-balancing features only. At some
        /// point the distribution features (as provided in tuners like
        /// compute_on) should find their home here as well.
        ///
        /// This base class implements low-level functionality which is used by the
        /// actual implementations (see *_distributed_scheduler); it's mostly
        /// about the communication with other processes, e.g. to exchange
        /// status information and to migrate steps.
        ///
        /// Actual implementations implement the load balancing strategies like
        /// stealing, sharing, donating or an hybrid approach. Right now they
        /// are are supposed to provide a progress engine which gets called
        /// whenever a step is to be executed (by scheduler_i). For this the
        /// implementaion must provide a method migrate_step. It gets called
        /// with the current number of steps in flight and the step that's
        /// about to be executed. The load balancer can decide to migrate the
        /// step or not.
        ///
        /// In the future we might want to detach the load-balancer and run it
        /// in an extra thread.  It would allow us to get the load-balancer
        /// from the critical path. Additionally, the current approach will not
        /// work well if computation grain is large (as the load-balancer gets
        /// in control only rarely).
        class distributed_scheduler : public distributable
        {
        public:
            typedef scalable_vector< schedulable * > step_vec_type;
            static const char WORK_REQUEST = 79;
            static const char WORK_CHUNK = 84;
            static const char STATE_UPDATE = 90;
            distributed_scheduler( context_base & ctxt, scheduler_i & scheduler );
            virtual ~distributed_scheduler();
            virtual void recv_msg( CnC::serializer* ser );

            //            virtual void loadBalanceCallback() = 0;
            /// \return true if the step was migrated to another process, false otherwise
            virtual bool migrate_step(unsigned int, schedulable* s) = 0;
            virtual void reset(){}
            //            virtual void activate();
            static distributed_scheduler * getInstance() { return instance; }

            virtual void unsafe_reset( bool dist ) {}

        protected:
            virtual void recv_work_request( CnC::serializer* ser, int senderId ) = 0;
            bool send_work_request( int receiver );
            bool getStep(schedulable*& step);
            void serialize_step( CnC::serializer & ser, schedulable & step );
            void send_steps_to_client( int clientId, schedulable ** steps, size_t n  );
            static const int MAX_STEP_GROUP_SIZE = (1<<4);
            void recv_steps( CnC::serializer & ser );
            void bcast_state_update( int me, int value );
            bool bcast_work_request();
            topology m_topo;

            scheduler_i  & m_scheduler;
        private:
            context_base & m_context;
            // yeap, this is a singleton, because scheduler_i::do_execute() is static
            static distributed_scheduler* instance;
            virtual void on_received_workchunk( CnC::serializer* ser, int senderId ) {}
            virtual void recv_state_update( CnC::serializer* ser, int senderId ){}
        };

    } // namespace Internal
} // namespace CnC

#endif // _CnC_DISTRIBUTED_SCHEDULER_H
