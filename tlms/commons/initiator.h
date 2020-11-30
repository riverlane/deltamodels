/**
 * @file initiator.hpp
 * @author Riverlane, 2020
 *
 */
#ifndef __INITIATOR_H__
#define __INITIATOR_H__

#include "tlm_utils/simple_initiator_socket.h"


/**
 * Initiator implements simple read and write operations via
 * TLM mechanism
*/
struct Initiator: sc_module
{
    tlm_utils::simple_initiator_socket<Initiator> socket;

    sc_signal<bool> we;
    sc_signal<bool> re;

    uint64_t addr;
    uint64_t wid;
    unsigned char *ptr; 
    unsigned char *byt;

    SC_CTOR(Initiator) : socket("socket")
    {
        SC_THREAD(pwrite);
        sensitive_pos << we;
        dont_initialize();
        SC_THREAD(pread);
        sensitive_pos << re;
        dont_initialize();
    }

    void pwrite()
    {
        tlm::tlm_generic_payload trans;
        while(true) {
            sc_time delay(0,SC_NS);
            trans.set_command( tlm::TLM_WRITE_COMMAND );
            trans.set_address( addr );
            trans.set_data_ptr( ptr );
            trans.set_data_length( 4 );
            trans.set_streaming_width( wid ); // = data_length to indicate no streaming
            trans.set_byte_enable_ptr( byt );
            trans.set_dmi_allowed( false ); // Mandatory initial value
            trans.set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
            socket->b_transport( trans, delay );

            // Initiator obliged to check response status
            if (trans.is_response_error())
                SC_REPORT_ERROR("TLM-2", trans.get_response_string().c_str());
            wait();
        }
    }

    void pread()
    {
        tlm::tlm_generic_payload trans;
        while (true) {
            sc_time delay(0,SC_NS);
            trans.set_command( tlm::TLM_READ_COMMAND );
            trans.set_address( addr );
            trans.set_data_ptr( ptr );
            trans.set_data_length( 4 );
            trans.set_streaming_width( wid ); // = data_length to indicate no streaming
            trans.set_byte_enable_ptr( 0 ); // 0 indicates unused
            trans.set_dmi_allowed( false ); // Mandatory initial value
            trans.set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value

            socket->b_transport( trans, delay );
            // Initiator obliged to check response status
            if (trans.is_response_error())
                SC_REPORT_ERROR("TLM-2", trans.get_response_string().c_str());
            wait();
        }
    }
};

inline void _initiator_dowrite(Initiator& init, uint32_t* data, uint32_t addr, uint32_t len, uint32_t wid, uint8_t* mask=NULL){
    cout << "dowrite called" << " @ " << sc_time_stamp() << endl;
    init.addr = static_cast<uint64_t>(addr);
    init.ptr = reinterpret_cast<unsigned char*>(data);
    init.wid = wid;
    init.byt = mask; 
    init.we.write(1);
    sc_start(1, SC_NS);
    init.we.write(0);
    sc_start(20, SC_NS);
}

inline void _initiator_dowrite(Initiator& init, uint32_t* data, uint32_t addr, uint32_t len=4){
    _initiator_dowrite(init, data, addr, len, len);
}

inline void _initiator_doread(Initiator& init, uint32_t* data, uint32_t addr, uint32_t len, uint32_t wid){
    cout << "doread called" << " @ " << sc_time_stamp() << endl;
    init.addr = static_cast<uint64_t>(addr);
    init.wid = len;
    init.ptr = reinterpret_cast<unsigned char*> (data);
    init.re.write(1);
    sc_start(1, SC_NS);
    init.re.write(0);
    sc_start(20, SC_NS);
}

inline void _initiator_doread(Initiator& init, uint32_t* data, uint32_t addr, uint32_t len=4){
    _initiator_doread(init, data, addr, len, len);
}

#endif //__INITIATOR_H__


