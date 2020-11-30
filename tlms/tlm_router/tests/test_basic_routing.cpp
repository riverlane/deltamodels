
#include <iostream>
#include <systemc>
#include <tlm.h>
#include "tlms/tlm_router/tlm_router.hpp"
#include "tlms/commons/memory.h"
#include "tlms/commons/initiator.h"
#include "commons/assertions.hpp"
#include <stdexcept>

using namespace std;


void test_simple_access(Initiator& init, const Memory<0x100>& mem, const long int naccesses){
    uint32_t value;
    for (int i=0; i<naccesses; i++) {
        value = i;
        _initiator_dowrite(init, &value, i*4);
    }
    for (int i=0; i<naccesses; i++) {
        _initiator_doread(init, &value, i*4);
        sc_start(10, SC_NS);
        cout << "Checking: " << i << " expected - " << value << " read " << endl;
        checkValuesMatch<int>(i, value, "readback");
    }
            checkValuesMatch<long int>(naccesses, mem.wr_ops, "mem_wr");
        checkValuesMatch<long int>(naccesses, mem.rd_ops, "mem_rd");
}

void test_out_of_bound_error(Initiator& init){
    uint32_t value = 32;
    bool ex_triggered = false;
    try { 
        // Doing an out-of-bound write, expecting error and an exception of type sc_core::sc_repor
        _initiator_dowrite(init, &value, 0x10000);
    } catch (const sc_core::sc_report& ex){
        ex_triggered = true;
    }
    checkValuesMatch<bool>(ex_triggered, true, "out-of-bound-detection");
}

int sc_main(int argc, char** argv) {

    sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_tlm_basic_routing");
    Tf->set_time_unit(100,SC_PS);

    Initiator init1 = Initiator("init1");
    Memory<0x100> mem = Memory<0x100>("memory");

    TLMRouter<1, 1> router = TLMRouter<1, 1>("router");

    init1.socket.bind(*(router.target_socket[0]));
    router.initiator_socket[0]->bind(mem.socket);

    const InitiatorConfig memConfig = {0x0, 0x1000, 0xFFFFFFFF};
    router.setInitiatorProperties(0, memConfig);

    // Binding trace files
    sc_trace(Tf, mem.wr_ops, "wr_ops");
    sc_trace(Tf, mem.rd_ops, "rd_ops");
   

    // running the tests
    const long int NACCESSES = 3; 

    try {
        test_simple_access(init1, mem, NACCESSES);
        test_out_of_bound_error(init1);
    } catch (const std::exception& ex) {
        sc_close_vcd_trace_file(Tf);
        SC_REPORT_ERROR("TEST_FAILURE", ex.what());
    }
    return 0;
}
