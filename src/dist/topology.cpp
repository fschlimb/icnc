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

#include "src/dist/topology.h"

#include <iostream>

namespace CnC {
    namespace Internal {

        void topology::init_ring(int n, int myrank){
//            std::cerr << "n = " << n << std::endl;
//            std::cerr << "myrank = " << myrank << std::endl;
            a.clear();
            edges.clear();
            edges.resize(n);
            if (n < 16){
                for (int i = 0; i < n; i++){
                    for (int j = 0; j < n; j++) if (i!=j){
                        edges[i].push_back(j);
                    }
                }
            } else {
                for (int i = 0; i < n; i++){
                    edges[i].push_back((i+1)%n);
                    edges[(i+1)%n].push_back(i);
                }
                int l = 0;
                int r = n;
                for (;r-l>1;){
                    int m = (l+r)/2;
                    if (m*m>n) r = m;
                    else l = m;
                }
                for (int i = 0; i < n; i++){
                    for (int j = 1; j*l < n; j++){
                        edges[i].push_back((i+j*l)%n);
                    }
                }
            }
            for (int i = 0; i < static_cast<int>(edges[myrank].size()); i++){
                a.push_back(edges[myrank][i]);
            }
        }

    } // namespace Internal
} // namespace CnC

