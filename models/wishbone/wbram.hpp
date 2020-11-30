/**
 * @file WBRAM.hpp
 * @author Riverlane, 2020
 *
 */

#ifndef __WBRAM_H__
#define __WBRAM_H__

#include "systemc"
#include <map>

using namespace std;

/**
 * Implementation of a simple memory block over wishbone.
 *
 * This blocks represents a simple memory exposed to a single `DWIDTH`-wide wishbone bus
 *  SIZE defines the size of the Memory.
 */
template <int DWIDTH>
struct WBRAM: sc_module
{
    sc_in_clk clk_i;
    sc_in<bool> rst_i;
    sc_in<sc_bv<DWIDTH>> adr_i;
    sc_in<sc_bv<DWIDTH>> dat_i;
    sc_in<bool> cyc_i;
    sc_in<bool> stb_i;
    sc_in<bool> we_i;
    sc_in<sc_bv<DWIDTH/8>> sel_i;
    sc_out<bool> ack_o;
    sc_out<sc_bv<DWIDTH>> dat_o;

    std::map<int32_t, sc_bv<DWIDTH>> memory;
    sc_bv<DWIDTH> word;
    sc_bv<DWIDTH> word_in;
    bool n_tran = false;

    uint32_t mem_size;

    // Debug signals
    sc_signal<long int> db_wr_ops;
    sc_signal<long int> db_rd_ops;


    sc_bv<DWIDTH> retrieve_word(uint32_t address) {
        sc_bv<DWIDTH> w;
        if (address > mem_size) {
            SC_REPORT_FATAL("TLM-ROUTER", "Out of bound access to WBRAM");
        }
        if (memory.find(address) != memory.end()) {
            w = memory.at(address);
        } else {
            // uninitialized memory location, returning XXX;
            for (int i=0; i < DWIDTH; i++)
                w.set_bit(i, 'X');
        }
        return w;
    }

    sc_bv<DWIDTH> set_word(uint32_t address, sc_bv<DWIDTH> value, sc_bv<DWIDTH/8> sel) {
        sc_bv<DWIDTH> w;
        if (address > mem_size) {
            SC_REPORT_FATAL("TLM-ROUTER", "Out of bound access to WBRAM");
        }
        w = retrieve_word(address);
        for (int i=0; i<DWIDTH/8; i++) {
            if (sel.get_bit(i) == 1) {
                w.range((i+1)*8-1,i*8) = value.range((i+1)*8-1,i*8);
            }
        }
        return w;
    }


    void handleop() {
        ack_o.write(false);
        if (cyc_i.read() && stb_i.read()) {
            if (!n_tran) {
                int32_t address = adr_i.read().to_uint();
                if (we_i.read()) {
                    word = set_word(address, dat_i.read(), sel_i.read());
                    cout << "WBRAM: Writing: 0x" << hex << word.to_int() << " @ "<< sc_time_stamp() << endl;
                    db_wr_ops.write(db_wr_ops.read()+1);
                    memory[address] = word;
                    cout << "WBRAM: Wrote: 0x" << hex << memory[address] << " @ "<< sc_time_stamp() << endl;
                } else {
                    word = retrieve_word(address);
                    cout << "WBRAM: Read: " << hex << word.to_int() << " @ " << sc_time_stamp() << endl;
                    db_rd_ops.write(db_rd_ops.read()+1);
                }
                n_tran = true;
                ack_o.write(true);
                dat_o.write(word);
            } else {
                n_tran = false;
            }
        }
    }


    WBRAM(sc_module_name name, uint32_t mem_size=0x100)
        : sc_module(name), mem_size(mem_size)
    {
        SC_METHOD(handleop);
        sensitive << clk_i.pos();
    }

    SC_HAS_PROCESS(WBRAM);


};

using WBRAM32 = WBRAM<32>;

#endif //__WBRAM_H__


