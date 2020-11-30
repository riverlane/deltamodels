cmake_minimum_required(VERSION 2.8)
project(Deltamodels CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -ggdb3 -DSC_DISABLE_API_VERSION_CHECK -D_FORTIFY_SOURCE=2 -faligned-new -fstack-protector -Wall -Werror -g -grecord-gcc-switches -O0 -fprofile-arcs -ftest-coverage -pthread")
set(CFLAGS "${CFLAGS} -g -O0 -Wall -W -fprofile-arcs -ftest-coverage")
set(LDFLAGS "${LDFLAGS} -fprofile-arcs -ftest-coverage")

find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --xml=yes --xml-file=/workdir/valgrind.xml --leak-check=summary --error-exitcode=1" )
set( MEMORYCHECK_SUPPRESSIONS_FILE "${PROJECT_SOURCE_DIR}/valgrind_suppress.txt" )
include (CTest)
include_directories(./)
include_directories(tlms/commons)
include_directories(tlms/tlm_adapters)
include_directories(tlms/tlm_router)
include_directories(tlms/commons)
include_directories(models/wishbone)
include_directories(models/clocking)
include_directories(models/memories/flash)
include_directories(logging)

include_directories($ENV{SYSTEMC_INCLUDE})
include_directories($ENV{TBB_LIBRARY_RELEASE})
include_directories($ENV{VERILATOR_INCLUDE})
link_directories($ENV{SYSTEMC_HOME}/lib-linux64/)

set(comp OFF)
add_executable(test_wishbone_adapter
    tlms/tlm_adapters/tests/test_wishbone_adapter.cpp)
target_link_libraries (test_wishbone_adapter systemc)

add_executable(test_router_basic
    tlms/tlm_router/tests/test_basic_routing.cpp)
target_link_libraries (test_router_basic systemc)

add_executable(test_router_multiport
    tlms/tlm_router/tests/test_multiport.cpp)
target_link_libraries (test_router_multiport systemc)

add_executable(test_clockgen
    models/clocking/tests/test_clockgen.cpp)
target_link_libraries (test_clockgen systemc)

add_executable(test_flash
    models/memories/tests/test_flash.cpp)
target_link_libraries (test_flash systemc)

add_executable(test_sdram
    models/memories/tests/test_sdram.cpp)
target_link_libraries (test_sdram systemc)

add_executable(test_or1k_inst_tracer
    models/wishbone/or1k/tests/test_instruction_tracer.cpp)
target_link_libraries (test_or1k_inst_tracer systemc)


add_executable(test_eth_bridge_udp
    models/network/tests/test_eth_bridge_udp.cpp)
target_link_libraries (test_eth_bridge_udp systemc)

add_test(test_wishbone_adapter test_wishbone_adapter)
add_test(test_router_basic   test_router_basic)
add_test(test_router_advanced test_router_multiport)
add_test(test_flash test_flash)
add_test(test_sdram test_sdram)
add_test(test_or1k_inst_tracer test_or1k_inst_tracer)
add_test(test_clockgen test_clockgen)

