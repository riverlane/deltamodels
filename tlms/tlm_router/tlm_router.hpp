/**
 * @file tlm_router.hpp
 * @author Riverlane, 2020
 *
 */

#ifndef __TLMROUTER_H__
#define __TLMROUTER_H__

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include <sstream> // std::stringstream
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

struct InitiatorConfig {
    unsigned int baseAddress;
    unsigned int topAddress;
    unsigned int mask;
};

/**
 * TLMRouter implements the router logic requires to connect multiple
 * TLM blocks
 */
template<unsigned int N_TARGETS, unsigned int N_INITIATORS>
struct TLMRouter: sc_module
{
    // IMPORTANT NOTE:
    // The standard approach swaps target and initiator in a router (server view vs client view)
    //
    tlm_utils::simple_target_socket_tagged<TLMRouter>*    target_socket[N_TARGETS];
    tlm_utils::simple_initiator_socket_tagged<TLMRouter>* initiator_socket[N_INITIATORS];

    InitiatorConfig initiatorsConfig[N_INITIATORS];

    void setup_logger(const char* filename) {
        auto max_size = 128*1024*1024;
        auto max_files = 3;
        auto file_logger = spdlog::rotating_logger_mt("tlms_logger", filename, max_size, max_files);
        spdlog::set_default_logger(file_logger);
        spdlog::set_pattern("%v");
        spdlog::info("TimeStamp, Target, Initiator, Address, Cmd, Value, Len, Wid, Byt, Data");
        spdlog::flush_every(std::chrono::seconds(1));
    }

    void setInitiatorProperties(int init_nr, const InitiatorConfig& config) {
        initiatorsConfig[init_nr] = config;
    }

    virtual void b_transport(int id, tlm::tlm_generic_payload& trans, sc_time& delay )
    {
        sc_dt::uint64 address = trans.get_address();
        sc_dt::uint64 masked_address = 0;
        tlm::tlm_command cmd = trans.get_command();
        unsigned char* ptr = trans.get_data_ptr();
        unsigned int initiator_nr = decode_address( address, masked_address);
        unsigned int len = trans.get_data_length();
        unsigned int wid = trans.get_streaming_width();
        unsigned char* byt = trans.get_byte_enable_ptr();

        // Modify address within transaction
        trans.set_address( masked_address );
        // Forward transaction to appropriate initiator (output bus)
        ( *initiator_socket[initiator_nr] )->b_transport( trans, delay );
        // Generating traces
        uint32_t data = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
        uint32_t enables = 0;
        if (byt)
            enables = byt[0] | (byt[1] << 8) | (byt[2] << 16) | (byt[3] << 24);
        spdlog::get("tlms_logger")->info("{},{},{},0x{:x},{},{},{},0x{:x},0x{:x}", sc_time_stamp().to_string(), id, initiator_nr, address,
                                         (cmd== tlm::tlm_command::TLM_WRITE_COMMAND ? "write" : "read"),
                                         len, wid, enables, data);
    }

    inline unsigned int decode_address( sc_dt::uint64 address, sc_dt::uint64& masked_address )
    {
        int index = -1;
        for (unsigned int i=0; i<N_INITIATORS; i++) {
            if ((address >= initiatorsConfig[i].baseAddress) && (address < initiatorsConfig[i].topAddress)) {
                masked_address = address & initiatorsConfig[i].mask;
                index = i;
                break;
            }
        }
        if (index < 0) {
            std::stringstream err;
            err << "TLM-ROUTER: Address " << hex << address << " does not match any defined subspaces";
            SC_REPORT_ERROR("TLM-ROUTER", err.str().c_str());
        }
        return index;
    }

    explicit TLMRouter(sc_module_name name, const char* filename="/workdir/build/tlm_router.csv")
        : sc_module(name)
    {
        setup_logger(filename);
        for (unsigned int i = 0; i < N_TARGETS; i++)
        {
            char txt[20];
            sprintf(txt, "targ_socket_%u", i);
            target_socket[i] = new tlm_utils::simple_target_socket_tagged<TLMRouter>(txt);
            target_socket[i]->register_b_transport(this, &TLMRouter::b_transport,i);
        }
        for (unsigned int i = 0; i < N_INITIATORS; i++)
        {
            char txt[20];
            sprintf(txt, "init_socket_%u", i);
            initiator_socket[i] = new tlm_utils::simple_initiator_socket_tagged<TLMRouter>(txt);
            initiatorsConfig[i] = {0, 0, 0};
        }
    }

    SC_HAS_PROCESS(TLMRouter);
};

#endif //__TLMROUTER_H__


