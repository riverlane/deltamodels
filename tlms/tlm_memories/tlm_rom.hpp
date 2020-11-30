/**
 * @file tlm_rom.hpp
 * @author Riverlane, 2020
 *
 */

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <sstream>

using namespace std;

/**
 * TLM_ROM implements a basic rom memory that exposes one 
 * TLM socket.
*/
SC_MODULE(TLM_ROM) {

    // TLM cpu port
    tlm_utils::simple_target_socket<TLM_ROM> dataBus;

    size_t baseaddr;
    std::vector<unsigned char> ROM;

    TLM_ROM(sc_module_name name, size_t baseaddr_, const char* rom_path) {
        std::ifstream file(rom_path, std::ios::in | std::ios::binary);
        ROM = std::vector<unsigned char>((std::istreambuf_iterator<char>(file)),
                                         std::istreambuf_iterator<char>());
        cout << "initalised the ROM for module " << name << endl;
        baseaddr = baseaddr_;
        dataBus.register_b_transport(this, &TLM_ROM::b_transport);
    };

    void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        tlm::tlm_command cmd = trans.get_command();
        sc_dt::uint64    addr = trans.get_address();
        unsigned char*   ptr = trans.get_data_ptr();
        unsigned int     len = trans.get_data_length();
        addr -= baseaddr;

        std::stringstream info;
        info << "TLM_ROM: got a generic transaction "<<
             "\n cmd: " << cmd <<
             "\n addr: " << addr <<
             "\n raw transaction addr: " << trans.get_address() <<
             "\n data ptr: " << (size_t)ptr <<
             "\n data len: " << len <<
             "\n delay: " << delay;
        SC_REPORT_INFO_VERB("TLM_ROM", info.str().c_str(), SC_DEBUG);

        if (addr >= ROM.size()) {
            std::stringstream err;
            err << "TLM_ROM: relative address " << hex << addr << " is outside memory [Memory size is " << ROM.size() << "]";
            SC_REPORT_ERROR("TLM_ROM", err.str().c_str());
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        if ( cmd == tlm::TLM_READ_COMMAND ) {
            for (size_t i=0; i<len; i++) {
                ptr[i] = ROM[addr+i];
            }
        }
        if ( cmd == tlm::TLM_WRITE_COMMAND ) {
            trans.set_response_status( tlm::TLM_ADDRESS_ERROR_RESPONSE ); // write disabled
            SC_REPORT_ERROR("TLM_ROM", "TLM_ROM: write not allowed on ROMs");
        }

        trans.set_response_status( tlm::TLM_OK_RESPONSE );
    }
};
