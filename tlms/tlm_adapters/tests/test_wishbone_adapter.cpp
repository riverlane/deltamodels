#include <iostream>
#include <systemc>
#include <tlm.h>
#include "tlms/tlm_adapters/tlm2wishbone.hpp"
#include "tlms/commons/initiator.h"
#include "models/wishbone/wbram.hpp"
#include "commons/assertions.hpp"

using namespace std;

// To hide some complexity;

void test_standard_writes_reads(Initiator& init) {
    uint32_t data = 0xCAFEBABE;
    _initiator_dowrite(init, &data, 0x0);
    data = 0;
    _initiator_doread(init, &data,  0x0);
    cout << "DUT: " << hex << data << endl;
    checkValuesMatch<uint32_t>(data, 0xCAFEBABE, "check_@0");
}

void test_streaming_writes_reads(Initiator& init) {
    uint32_t data [4] = {0xCAFEBABE, 0xCAFEBABF, 0xCAFEBAC0, 0xCAFEBAC1};

    _initiator_dowrite(init, data, 0x4, 16);
    // Clearing data
    for (int i=0; i<4; i++) {
        data[i] = 0;
    }
    _initiator_doread(init, data, 0x4, 16);
    for (int i=0; i<4; i++) {
        cout << "DUT: " << hex << data[i] << endl;
    }

    // Assertion phase
    checkValuesMatch<uint32_t>(data[0], 0xCAFEBABE, "check_@4");
    checkValuesMatch<uint32_t>(data[1], 0xCAFEBABF, "check_@8");
    checkValuesMatch<uint32_t>(data[2], 0xCAFEBAC0, "check_@C");
    checkValuesMatch<uint32_t>(data[3], 0xCAFEBAC1, "check_@10");
}

void test_irregular_writes_reads(Initiator& init) {
    uint32_t data = 0xBABECAFE;
    // Streaming width of zero. Currently a warning only.
    _initiator_dowrite(init, &data, 0x20, 4, 0);
    data = 0;
    _initiator_doread(init, &data, 0x20, 4, 0);
    checkValuesMatch<uint32_t>(data, 0xBABECAFE, "check_@20");
}

void test_byte_enable(Initiator& init) {
    uint32_t data;
    data = 0x0;
    _initiator_dowrite(init, &data, 0x0);
    data = 0xbabecafe;
    uint8_t enable [4] = {0xff, 0, 0, 0xff};
    _initiator_dowrite(init, &data, 0x0, 4, 0, enable);
    data = 0;
    _initiator_doread(init, &data, 0x0);
    checkValuesMatch<uint32_t>(data, 0xBA0000FE, "check_@20");
}


void test_timeout(Initiator& init, TLM2WB_32& bridge) {
    uint32_t data = 0xBABECAFE;
    bool ex_triggered = false;
    bridge.setTimeout(1);
    try {
        _initiator_dowrite(init, &data, 0x20, 4, 0);
    } catch (const sc_core::sc_report& ex) {
        ex_triggered = true;
    }
    checkValuesMatch(true, ex_triggered, "check_timeout");
}


int sc_main(int argc, char** argv) {

    sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_tlm_wishbone");
    Tf->set_time_unit(100,SC_PS);
    sc_clock clk("clk", sc_time(1, SC_NS));
    sc_trace(Tf, clk, "clk");
    sc_signal<bool> rst;
    sc_trace(Tf, rst, "rst");

    sc_signal<sc_bv<32>> m2s_adr_o ;
    sc_signal<sc_bv<32>> m2s_dat_o;
    sc_signal<bool> m2s_we_o ;
    sc_signal<sc_bv<32/8>> m2s_sel_o;
    sc_signal<bool> m2s_stb_o;
    sc_signal<bool> m2s_cyc_o;
    sc_signal<sc_bv<32>> m2s_dat_i;
    sc_signal<bool> m2s_ack_i;

    sc_trace(Tf, m2s_adr_o, "m2s_adr_o");
    sc_trace(Tf, m2s_dat_o, "m2s_dat_o");
    sc_trace(Tf, m2s_we_o,  "m2s_we_o");
    sc_trace(Tf, m2s_sel_o, "m2s_sel_o");
    sc_trace(Tf, m2s_stb_o, "m2s_stb_o");
    sc_trace(Tf, m2s_cyc_o, "m2s_cyc_o");
    sc_trace(Tf, m2s_dat_i, "m2s_dat_i");
    sc_trace(Tf, m2s_ack_i, "m2s_ack_i");

    Initiator init1 = Initiator("tlm_init");

    TLM2WB_32 bridge = TLM2WB_32("bridge");

    WBRAM32 ram = WBRAM32("RAM", 0x100);

    init1.socket.bind(bridge.tlm_socket);

    bridge.clk(clk);
    bridge.rst(rst);
    bridge.adr_o(m2s_adr_o);
    bridge.dat_o(m2s_dat_o);
    bridge.sel_o(m2s_sel_o);
    bridge.cyc_o(m2s_cyc_o);
    bridge.we_o(m2s_we_o);
    bridge.stb_o(m2s_stb_o);
    bridge.ack_i(m2s_ack_i);
    bridge.dat_i(m2s_dat_i);

    ram.clk_i(clk);
    ram.rst_i(rst);
    ram.adr_i(m2s_adr_o);
    ram.dat_i(m2s_dat_o);
    ram.sel_i(m2s_sel_o);
    ram.cyc_i(m2s_cyc_o);
    ram.we_i(m2s_we_o);
    ram.stb_i(m2s_stb_o);
    ram.ack_o(m2s_ack_i);
    ram.dat_o(m2s_dat_i);

    try {
        test_standard_writes_reads(init1);
        test_streaming_writes_reads(init1);
        test_irregular_writes_reads(init1);
        test_byte_enable(init1);
        test_timeout(init1, bridge);
    } catch (const std::exception& ex) {
        sc_close_vcd_trace_file(Tf);
        SC_REPORT_ERROR("TEST_FAILURE", ex.what());
    }

    return 0;
}
