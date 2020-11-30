/*
 This file contains some logging functions
 2020 Tom Parks, Riverlane
 */

#ifndef LOGGING_H
#define LOGGING_H

#include "systemc"
#include <iostream>
#include <sstream>

using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
// *************************************************************************************i
// Logging functionalities
// **************************************************************************************
string tlm_info(tlm::tlm_generic_payload& trans) {
    ostringstream info (std::ostringstream::ate);
    info.str("TLM_INFO: ");
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    addr = trans.get_address();
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length();

    info << "\n cmd: " << (cmd == tlm::TLM_WRITE_COMMAND ? "write" : "read") <<
         "\n raw transaction addr: 0x" << std::hex << addr <<
         "\n data ptr: 0x" << (size_t)ptr <<
         "\n data len: " << std::dec << len;
    return info.str();
}

#endif


