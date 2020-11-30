#include <iostream>
#include "systemc.h"
#include "models/memories/sdram/generic_sdram.hpp"
#include "commons/assertions.hpp"
#include "commons/testbench_utils.hpp"
#include <map>

using namespace std;

template <int A_SIZE, int BA_SIZE>
struct MockDDRController: sc_module
{
    sc_in<sc_bv<1>> clk;
    sc_out<sc_bv<1>> cs_n;
    sc_out<sc_bv<A_SIZE>> a;
    sc_out<sc_bv<BA_SIZE>>  ba;
    sc_inout<sc_bv<32>> dq;
    sc_out<sc_bv<32/8>>  dm;
    sc_out<sc_bv<1>>  cke;
    sc_out<sc_bv<1>>  ras_n;
    sc_out<sc_bv<1>>  cas_n;
    sc_out<sc_bv<1>>  we_n;

    int cnt {0};
    uint32_t value {0};

    void do_nop(){
        cs_n.write(1);
        a.write(0);
        ba.write(0);
        dq.write(0);
        dm.write(0);
        cke.write(1);
        ras_n.write(0);
        cas_n.write(0);
        we_n.write(0);
    }

    void activate_bank(uint32_t bank, uint32_t row){
        cs_n.write(0);
        a.write(row);
        ba.write(bank);
        dq.write(0);
        dm.write(0);
        cke.write(1);
        ras_n.write(0);
        cas_n.write(1);
        we_n.write(1);
        sc_start(1, SC_NS);
        do_nop();
        sc_start(1, SC_NS);
    }

   void do_write(uint32_t bank, uint32_t col, uint32_t data){
       cs_n.write(0);
       a.write(col);
       ba.write(bank);
       dq.write(data);
       dm.write(0x0);
       cke.write(1);
       ras_n.write(1);
       cas_n.write(0);
       we_n.write(0);
       sc_start(1, SC_NS);
       do_nop();
       sc_start(1, SC_NS);
   }

   uint32_t do_read(uint32_t bank, uint32_t col){
       cs_n.write(0);
       a.write(col);
       ba.write(bank);
       cke.write(1);
       ras_n.write(1);
       cas_n.write(0);
       we_n.write(1);
       sc_start(1, SC_NS);
       do_nop();
       sc_start(1, SC_NS);
       return dq.read().to_uint();
   }

   SC_CTOR(MockDDRController)
    {
        sensitive << clk;
    }
};


void test_allocation(GENERIC_SDRAM<12,2>& sdram){
    std::string img_file =
        std::string("/workdir/models/memories/tests/image_for_storage.img");
    sdram.configure_region(img_file, 0x400000);
    checkValuesMatch<uint32_t>(sdram.mem[0x400000/4], 0x1e, "check_init");
    checkValuesMatch<uint32_t>(sdram.mem[0x400004/4], 0x6d616300, "check_init_2nd_loc");
    sdram.mem.clear();
}

void test_addressing(GENERIC_SDRAM<12,2>& sdram){
  for (int i=0; i < 6; i++){
    uint32_t row, bank, col;
    uint32_t address = 0xFFF0 << i;
    sdram.to_row_bank_column(address, row, bank, col);
    uint32_t address_computed = sdram.to_address(row, bank, col);
    checkValuesMatch<uint32_t>(address_computed, address, "check_address");
  }
}

void _write(GENERIC_SDRAM<12,2>& sdram, MockDDRController<12,2>& mock, uint32_t addr, uint32_t data){
    uint32_t row, bank, col;
    sdram.to_row_bank_column(addr, row, bank, col);
    mock.activate_bank(bank, row);
    mock.do_write(bank, col, data);
}

uint32_t _read(GENERIC_SDRAM<12,2>& sdram, MockDDRController<12,2>& mock, uint32_t addr){
    uint32_t row, bank, col;
    sdram.to_row_bank_column(addr, row, bank, col);
    mock.activate_bank(bank, row);
    return mock.do_read(bank, col);
}


void test_simple_sequence(GENERIC_SDRAM<12,2>& sdram, MockDDRController<12,2>& mock){
    mock.do_nop();
    sc_start(5, SC_NS);
    _write(sdram, mock, 0x300, 0xBABECAFE);
    sc_start(5, SC_NS);
    uint32_t value = _read(sdram, mock, 0x300);
    sdram.hexdump("sdram.dump");
    checkValuesMatch<uint32_t>(sdram.mem[0x300/4], 0xbabecafe, "check_write");
    checkValuesMatch<uint32_t>(value, 0xBABECAFE, "check_read");
    sdram.mem.clear();
}

void test_interleaved_sequence(GENERIC_SDRAM<12,2>& sdram, MockDDRController<12,2>& mock){
    std::map<uint32_t, uint32_t> values;
    values[0x1000] = 0xbabecafe;
    values[0x10000] = 0xbabababa;
    values[0x10] = 0xfefefefe;
    values[0x3000] = 0xfafafafa;

    // Write phase
    for( auto const& [addr, val] : values ){
        mock.do_nop();
        sc_start(5, SC_NS);
        _write(sdram, mock, addr, val);
        checkValuesMatch<uint32_t>(sdram.mem[addr/4], val, "check_write");
    }

    // Interleaving read/writes (on the already written locations)
    uint32_t value;
    for( auto const& [waddr, wval] : values ){
        for( auto const& [raddr, rval] : values ){
            mock.do_nop();
            sc_start(5, SC_NS);
            _write(sdram, mock, waddr, wval);
            sc_start(5, SC_NS);
            value = _read(sdram, mock, raddr);
            checkValuesMatch<uint32_t>(sdram.mem[waddr/4], wval, "check_write");
            checkValuesMatch<uint32_t>(value, rval, "check_read");
        }
    }
    sdram.mem.clear();
}


int sc_main(int argc, char** argv) {

    sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_test_sdram");
    Tf->set_time_unit(100,SC_PS);
    sc_clock clk("clk", sc_time(1, SC_NS));
    sc_trace(Tf, clk, "clk");

    sc_signal<sc_bv<1>> ddr_cs_n;
    sc_trace(Tf, ddr_cs_n, "ddr_cs_n");
    sc_signal<sc_bv<12>> ddr_a;
    sc_trace(Tf, ddr_a, "ddr_a");
    sc_signal<sc_bv<2>>  ddr_ba;
    sc_trace(Tf, ddr_ba, "ddr_ba");
    sc_signal<sc_bv<32>, sc_core::SC_MANY_WRITERS> ddr_dq;
    sc_trace(Tf, ddr_dq, "ddr_dq");
    sc_signal<sc_bv<32/8>>  ddr_dm;
    sc_trace(Tf, ddr_dm, "ddr_dm");
    sc_signal<sc_bv<1>>  ddr_cke;
    sc_trace(Tf, ddr_cke, "ddr_cke");
    sc_signal<sc_bv<1>>  ddr_ras_n;
    sc_trace(Tf, ddr_ras_n, "ddr_ras_n");
    sc_signal<sc_bv<1>>  ddr_cas_n;
    sc_trace(Tf, ddr_cas_n, "ddr_cas_n");
    sc_signal<sc_bv<1>>  ddr_we_n;
    sc_trace(Tf, ddr_we_n, "ddr_we_n");
    // Backdoor signals
    sc_signal<sc_bv<32>> backdoor_copy_from;
    sc_signal<sc_bv<32>> backdoor_copy_to;
    sc_signal<sc_bv<32>> backdoor_copy_size;
    sc_signal<sc_bv<1>> backdoor_copy;
    sc_signal<sc_bv<32>> backdoor_clear_from;
    sc_signal<sc_bv<32>> backdoor_clear_to;
    sc_signal<sc_bv<1>> backdoor_clear;


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

    GENERIC_SDRAM<12,2> sdram = GENERIC_SDRAM<12,2>("sdram", SDRAM_GEOM{.row_bits=12, .bank_bits=2, .col_bits=8});
    sdram.hexdump("/workdir/build/sdram_image_for_storage.img.hexdump");
    sdram.ck(clk_bv);
    sdram.cs_n(ddr_cs_n);
    sdram.a(ddr_a);
    sdram.ba(ddr_ba);
    sdram.dq(ddr_dq);
    sdram.dm(ddr_dm);
    sdram.cke(ddr_cke);
    sdram.ras_n(ddr_ras_n);
    sdram.cas_n(ddr_cas_n);
    sdram.we_n(ddr_we_n);

    sdram.backdoor_copy_from(backdoor_copy_from);
    sdram.backdoor_copy_to(backdoor_copy_to);
    sdram.backdoor_copy_size(backdoor_copy_size);
    sdram.backdoor_copy(backdoor_copy);
    sdram.backdoor_clear_from(backdoor_clear_from);
    sdram.backdoor_clear_to(backdoor_clear_to);
    sdram.backdoor_clear(backdoor_clear);


    MockDDRController<12,2> mock = MockDDRController<12,2>("mock");
    mock.clk(clk_bv);
    mock.cs_n(ddr_cs_n);
    mock.a(ddr_a);
    mock.ba(ddr_ba);
    mock.dq(ddr_dq);
    mock.dm(ddr_dm);
    mock.cke(ddr_cke);
    mock.ras_n(ddr_ras_n);
    mock.cas_n(ddr_cas_n);
    mock.we_n(ddr_we_n);

    try {
      test_allocation(sdram);
      test_addressing(sdram);
      test_simple_sequence(sdram, mock);
      test_interleaved_sequence(sdram, mock);
    } catch (const std::exception& ex) {
      std::cerr << "One of the test failed - rethrowing exception after saving wave files" << std::endl;
      sc_close_vcd_trace_file(Tf);
      SC_REPORT_ERROR("TEST_FAILURE", ex.what());
    }
    return 0;
}
