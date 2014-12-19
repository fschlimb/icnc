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

#ifndef _CnC_HYBRID_DISTRIBUTED_SCHEDULER_H
#define _CnC_HYBRID_DISTRIBUTED_SCHEDULER_H

#include <src/dist/distributed_scheduler.h>
#include <tbb/atomic.h>
#include <tbb/spin_mutex.h>
#include <tbb/concurrent_queue.h>

namespace CnC {

    class serializer;

    namespace Internal {

        class schedulable;

        class hybrid_distributed_scheduler : public distributed_scheduler
        {
            public:
                hybrid_distributed_scheduler( context_base & ctxt, scheduler_i & scheduler );
                virtual ~hybrid_distributed_scheduler();
            void progress( unsigned int nsif );
                virtual void on_received_workchunk( CnC::serializer* ser, int senderId );
                bool postRequest();
                virtual bool migrate_step(unsigned int, schedulable* s);

            private:
                virtual void recv_work_request( CnC::serializer* ser, int senderId );
            bool needs_bcast( int nsif );
            void do_bcast( unsigned int nsif );
                static const int LOG_BCAST_FREQUENCY = 9;
                static const int FEW_STEPS = (1<<8);
                static const int ENOUGH_STEPS = FEW_STEPS<<2;
                static const int MANY_STEPS = ENOUGH_STEPS<<2;

                tbb::atomic<int>* m_clientRequests;
                tbb::atomic<long>* m_clientState;
                tbb::atomic<int>* m_sentRequests;
                tbb::atomic<int> m_bcastCounter;
                tbb::atomic<int> m_last;
                tbb::spin_mutex send_request_mutex;
        };

    } // namespace Internal
} // namespace CnC

#endif // _CnC_HYBRID_DISTRIBUTED_SCHEDULER_H
