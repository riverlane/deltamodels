#include <iostream>
#include "systemc.h"
#include "models/wishbone/or1k/or1k_instruction_tracer.hpp"
#include "commons/assertions.hpp"
#include "commons/testbench_utils.hpp"

using namespace std;

#include <fstream>


void test_recognize_all_used_codes(Or1kInstructionTracer<30,32>& debugger, 
    sc_signal<sc_bv<30>>& ibus_adr, sc_signal<sc_bv<32>>& ibus_dat,
    sc_signal<sc_bv<1>>& ibus_ack){
    bool passed = true;
    std::ifstream infile("/workdir/models/wishbone/or1k/tests/kernel_cpu.input");
    try {
        uint32_t ts, adr, data;
        while (infile >> ts >> std::hex >> adr >> std::hex >> data)
        {
            ibus_dat.write(data);
            ibus_adr.write(adr);
            ibus_ack.write(1);
            sc_start(1, SC_NS);
            ibus_ack.write(0);
            sc_start(1, SC_NS);
        }
    } catch (const sc_core::sc_report& ex){
        passed = false;
    }
    checkValuesMatch<bool>(passed, true, "All codes are valid");
}

void test_detects_not_valid_code(Or1kInstructionTracer<30,32>& debugger, 
    sc_signal<sc_bv<30>>& ibus_adr, sc_signal<sc_bv<32>>& ibus_dat,
    sc_signal<sc_bv<1>>& ibus_ack){
        bool detected = false;
    try {
        ibus_dat.write(0xFFFFFFFF);
        ibus_adr.write(0);
        ibus_ack.write(1);
        sc_start(1, SC_NS);
        ibus_ack.write(0);
        sc_start(1, SC_NS);
    } catch (const sc_core::sc_report& ex){
        detected = true;
    }
    checkValuesMatch<bool>(detected, true, "Invalid code was detected");
}

int sc_main(int argc, char** argv) {
    sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_test_or1k");
    Tf->set_time_unit(100,SC_PS);
    sc_clock clk("clk", sc_time(1, SC_NS));
    sc_trace(Tf, clk, "clk");
    sc_signal<sc_bv<30>> ibus_adr;
    sc_signal<sc_bv<32>> ibus_dat;
    sc_signal<sc_bv<1>> ibus_ack;
    sc_signal<sc_bv<1>> clk_bv;
    Clk2BV clk2bv = Clk2BV("clk2bv");
    clk2bv.clk(clk);
    clk2bv.clkout(clk_bv);

    Or1kInstructionTracer<30,32> debugger = Or1kInstructionTracer<30,32> {"ork1_dbg", 
        "/workdir/build/test_or1k_instruction_trace_out.log", true};
    debugger.clk(clk_bv);
    debugger.ibus_adr(ibus_adr);
    debugger.ibus_dat(ibus_dat);
    debugger.ibus_ack(ibus_ack);

    test_recognize_all_used_codes(debugger, ibus_adr, ibus_dat, ibus_ack);
    test_detects_not_valid_code(debugger, ibus_adr, ibus_dat, ibus_ack);

    return 0;
}
