/**
 * @file mock_gmii.hpp
 * @author Riverlane, 2020
 *
 */

#ifndef __MOCK_GMII__
#define __MOCK_GMII__

#include <systemc.h>

using namespace std;
using namespace sc_dt;

/**
 * Implementation of a mock GMII interface.
 * The component can bind to a behave as a generic PHY-to-MAC interface
 */
struct MockGMII: sc_module
{
    sc_in_clk clk;
    sc_in<bool> start;
    sc_out<sc_bv<1>> tx_new_pkt;
    sc_out<sc_bv<8>> tx_pkt;

    const uint8_t * pkt;
    const int pkt_size;

    void send_pkt() {
        while (true) {
            if (start.read() == 1) {
                tx_pkt.write(0);
                for (int i=0; i < pkt_size; i++) {
                    wait(clk.posedge());
                    tx_new_pkt.write(1);
                    tx_pkt.write(pkt[i]);
                }
                wait(clk.posedge());
                tx_new_pkt.write(0);
                wait(clk.posedge());
                break;
            } else {
                wait();
            }
        }
    }

    MockGMII(sc_module_name name, const uint8_t * pkt, const uint16_t pkt_size) :
        sc_module(name), pkt(pkt), pkt_size(pkt_size)
    {
        SC_THREAD(send_pkt);
        sensitive << clk.pos() << start;
        dont_initialize();
    }
    SC_HAS_PROCESS(MockGMII);
};
#endif


