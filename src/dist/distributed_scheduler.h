//********************************************************************************
// Copyright (c) Intel Corporation. All Rights Reserved.                        **
//                                                                              **
// The source code contained or described herein and all documents related to   **
// the source code ("Material") are owned by Intel Corporation or its suppliers **
// or licensors. Title to the Material remains with Intel Corporation or its    **
// suppliers and licensors. The Material contains trade secrets and proprietary **
// and confidential information of Intel or its suppliers and licensors. The    **
// Material is protected by worldwide copyright and trade secret laws and       **
// treaty provisions. No part of the Material may be used, copied, reproduced,  **
// modified, published, uploaded, posted, transmitted, distributed, or          **
// disclosed in any way without Intel's prior express written permission.       **
//                                                                              **
// No license under any patent, copyright, trade secret or other intellectual   **
// property right is granted to or conferred upon you by disclosure or delivery **
// of the Materials, either expressly, by implication, inducement, estoppel or  **
// otherwise. Any license under such intellectual property rights must be       **
// express and approved by Intel in writing.                                    **
//********************************************************************************

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
