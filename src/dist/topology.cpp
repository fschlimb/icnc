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

