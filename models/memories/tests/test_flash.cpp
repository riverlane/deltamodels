#include <iostream>
#include <systemc>
#include "models/memories/flash/N25QX.hpp"
#include "commons/assertions.hpp"
#include "commons/testbench_utils.hpp"

using namespace std;



int sc_main(int argc, char** argv) {

    sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_test_flash");
    Tf->set_time_unit(100,SC_PS);
    sc_clock clk("clk", sc_time(1, SC_NS));
    sc_trace(Tf, clk, "clk");
    sc_signal<sc_bv<1>> cs_n;
    sc_trace(Tf, cs_n, "cs_n");
    sc_signal<sc_bv<4>, sc_core::SC_MANY_WRITERS> dq;
    sc_trace(Tf, dq, "dq");

    // We are currently generating a sc_bv<1> instead of bool for the clock.
    // It should be changed at Deltaruntime level
    sc_signal<sc_bv<1>> clk_bv;
    Clk2BV clk2bv = Clk2BV("clk2bv");
    clk2bv.clk(clk);
    clk2bv.clkout(clk_bv);

    sc_signal<bool> clk_b;
    BvToBool bv2bool = BvToBool("bv2bool");
    bv2bool.in(clk_bv);
    bv2bool.out(clk_b);

    std::string img_file =
        std::string("/workdir/models/memories/tests/image_for_storage.img");
    const uint32_t DUMMY_CYCLES = 11;
    N25QX flash = N25QX("flash", DUMMY_CYCLES);

    flash.configure_region(img_file, 0x123456);
    flash.cs_n(cs_n);
    flash.dq(dq);
    flash.clk(clk_b);

    // generate a simple sequence
    uint8_t cmd = 0xeb;
    cs_n.write(1);
    sc_start(10, SC_NS);
    cs_n.write(0);

    for (int i=0; i<8; i++) {
        dq.write(cmd >>(7-i) & 0x1);
        sc_start(1, SC_NS);
    }

    uint32_t address = 0x123456;
    for (int i=0; i<6; i++) {
        dq.write((address >>(4*(5-i))) & 0xF);
        sc_start(1, SC_NS);
    }

    sc_start((int)DUMMY_CYCLES-1, SC_NS);

    std::ifstream in(img_file);
    std::string exp_str;
    if(in) {
        std::ostringstream ss;
        ss << in.rdbuf(); // reading data
        exp_str = ss.str();
    }
    std::cout << "expected string is " << exp_str << std::endl;
    std::string content;
    for (int i=0; i<52; i++) {
        uint8_t val;
        sc_start(1, SC_NS);
        val = dq.read().to_uint();
        sc_start(1, SC_NS);
        val = dq.read().to_uint() | (val << 4);
        std::cout << sc_time_stamp() << " Content is 0x" << std::hex << (int) val << std::endl;
        content.push_back((char)val);
    }
    std::cout << "Final string: " << content << std::endl;
    checkValuesMatch<std::string>(content, exp_str, "check");

    std::cout << "Done" << std::endl;
    cs_n.write(1);
    sc_start(10, SC_NS);

    sc_close_vcd_trace_file(Tf);


    return 0;
}
