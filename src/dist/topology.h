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

#ifndef _CnC_LOGICAL_TOPOLOGY_H_
#define _CnC_LOGICAL_TOPOLOGY_H_

#include <cnc/internal/scalable_vector.h>

namespace CnC {
    namespace Internal {
        
        class topology {
        public:
            typedef scalable_vector< int > neighbor_list_type;

            int neighboursCount() const { return a.size(); }
            int getNeighbour(int index) const { return a[index]; }
            const neighbor_list_type & neighbors() const {
                return a;
            }
            void init_ring(int n, int myrank);
            
        private:
            neighbor_list_type a;
            scalable_vector< neighbor_list_type > edges;
        };
        
    } // namespace Internal
} // namespace CnC

#endif // _CnC_LOGICAL_TOPOLOGY_H_
