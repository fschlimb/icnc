# see parent $CNCROOT/samples/CMakeLists.txt

set(myname jacobi2d_heureka)

add_executable(${myname} jacobi2D.cpp jacobi2D_cnc.cpp)
target_link_libraries(${myname} ${NEEDED_LIBS})

if(USE_ITAC)
  set_itac_link_flags(${myname})
endif(USE_ITAC)

unset(myname CACHE)
