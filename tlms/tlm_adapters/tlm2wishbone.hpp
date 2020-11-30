/**
 * @file tlm2wishbone.hpp
 * @author Riverlane, 2020
 *
 */

#ifndef TLM2WB_H
#define TLM2WB_H

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"


/**
 * TLM2WB implements a conversion from TLMs to Wishbone.
 * The Transaction layer is clock agnostic so we use an sc_event to handshake between
 * the clocked logic (Wishbone) and the transaction layer.
 * NOTE: we currently support only 32 bits operations with byte granularity
 * Refer to rules RULE 3.95, RULE: 3.96 on Wishbone B4 specifications for more information
 */
template <int AWIDTH=32, typename T=bool>
struct TLM2WB: sc_module
{
    static constexpr int BUS_WIDTH    = 32;
    static constexpr int DWIDTH       = BUS_WIDTH;
    static constexpr int DWIDTH_BYTES = DWIDTH / 8;
    static constexpr int BYTE_ADDRESSING = 2;

    tlm_utils::simple_target_socket <TLM2WB>    tlm_socket = tlm_utils::simple_target_socket<TLM2WB>("TlmBus");

    sc_in_clk  clk;
    sc_in<bool> rst;
    sc_out<sc_bv<AWIDTH>> adr_o ;
    sc_out<sc_bv<BUS_WIDTH>> dat_o;
    sc_out<T> we_o ;
    sc_out<sc_bv<BUS_WIDTH/8>> sel_o;
    sc_out<T> stb_o;
    sc_out<T> cyc_o;
    sc_in<T> ack_i;
    sc_in<sc_bv<32>> dat_i;
    //sc_out<bool> tagn_o;

    sc_bv<DWIDTH> bv_data;
    sc_bv<DWIDTH_BYTES> bv_byteen;
    // bus signals
    bool             wnr;
    sc_dt::uint64    adr;
    unsigned char*   ptr;
    unsigned int     len;
    unsigned char*   byt;
    unsigned int     wid;
    unsigned char    byteen;
    bool             pending_trans = false;
    unsigned char data [DWIDTH_BYTES];

    // State machine to handle the logic
    enum WB_state_t {IDLE, EXECUTING_WRITE, EXECUTING_READ, WAITING_FOR_ACK};
    WB_state_t state = {IDLE};

    // handshake among callback and SC_METHOD
    sc_event ack_event;

    // Maximum allocated time for the ack response
    unsigned int timeout = 10;
    unsigned int elapsed {0};

    void setTimeout(unsigned int timeout) {
        this->timeout = timeout;
    }

    void wishbone_handler() {
        switch (state) {
        case IDLE:
            break;
        case EXECUTING_WRITE:
            stb_o.write(true);
            cyc_o.write(true);
            we_o.write(true);
            adr_o.write(adr);
            byteen = 0;
            for (int i=0; i < DWIDTH_BYTES; i++) {
                bv_data.range((i+1)*8-1, i*8) = (ptr[DWIDTH_BYTES-i-1]);
                if (byt)
                    bv_byteen.set_bit(i, byt[DWIDTH_BYTES-i-1] == 0xff);
                else
                    bv_byteen.set_bit(i, 1);
            }
            dat_o = bv_data;
            wnr = true;
            sel_o.write(bv_byteen);
            state = WAITING_FOR_ACK;
            elapsed = 0; 
            break;
        case EXECUTING_READ:
            stb_o.write(true);
            cyc_o.write(true);
            we_o.write(false);
            adr_o.write(adr);
            sel_o = 0;
            wnr = false;
            state = WAITING_FOR_ACK;
            elapsed = 0; 
            break;
        case WAITING_FOR_ACK:
            if (ack_i.read() != 0) {
                stb_o.write(false);
                cyc_o.write(false);
                we_o.write(false);
                sel_o = 0;
                if (!wnr) {
                    bv_data = dat_i;
                    for (int i=0; i < DWIDTH_BYTES; i++) {
                        ptr[DWIDTH_BYTES-i-1] = (unsigned char)((bv_data.to_uint() >> i*8) & 0xFF);
                    }
                }
                state = IDLE;
                ack_event.notify();
            }
            if ((++elapsed) >= timeout) {
                SC_REPORT_ERROR("TLM2WB", "Acknowledge not received");
            }
            break;
        }
    }

    // TLM-2 blocking transport method
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay )
    {
        tlm::tlm_command cmd = trans.get_command();
        if ((cmd != tlm::tlm_command::TLM_WRITE_COMMAND) && (cmd != tlm::tlm_command::TLM_READ_COMMAND))
            SC_REPORT_ERROR("TLM-2", "Received an unsupported cmd");
        WB_state_t _state;
        _state = (trans.get_command() == tlm::tlm_command::TLM_WRITE_COMMAND) ? EXECUTING_WRITE : EXECUTING_READ;
        state = _state;
        adr = trans.get_address() >> BYTE_ADDRESSING;
        ptr = trans.get_data_ptr();
        len = trans.get_data_length();
        byt = trans.get_byte_enable_ptr();
        wid = trans.get_streaming_width();

        if (wid == 0) {
            SC_REPORT_WARNING("TLM_WB2TLM", "TLM LRM: A streaming width of 0 shall be invalid. Making wid=len and proceeding.");
            wid = len;
        }

        while (true) {
            wait(ack_event);
            wid -= len;
            if (wid == 0) break;
            state = _state;
            adr += len;
            ptr += len;
            if (byt)
                byt += len;
        }
        trans.set_response_status( tlm::TLM_OK_RESPONSE );
    }

    SC_CTOR(TLM2WB)
    {
        tlm_socket.register_b_transport(this, &TLM2WB::b_transport);
        SC_METHOD(wishbone_handler);
        sensitive << clk.pos();
        dont_initialize();
    }
};

using TLM2WB_32 = TLM2WB<32, bool>;
#endif
