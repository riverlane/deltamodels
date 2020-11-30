/**
 * @file or1k_instruction_tracer.hpp
 * @author Riverlane, 2020
 *
 */


#ifndef __OR1K_INSTRUCTION_TRACER_H__
#define __OR1K_INSTRUCTION_TRACER_H__

#include "systemc.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "models/wishbone/or1k/or1k_decoder.hpp"
#include <optional>

using namespace std;

/**
 * Implementation of an instruction tracer
 *
 * Generic CPU i-bus transactions can be converted into a humanly readable
 * representation and stored
 */
template <int AWIDTH, int DWIDTH>
struct Or1kInstructionTracer: sc_module
{
    sc_in<sc_bv<1>> clk;
    sc_in<sc_bv<AWIDTH>> ibus_adr;
    sc_in<sc_bv<DWIDTH>> ibus_dat;
    sc_in<sc_bv<1>> ibus_ack;

    Or1kDecoder or1k;
    bool stop_on_invalid;

    void parse() {
        if (clk.read().to_uint() == 1) {
            if (ibus_ack.read().to_uint() == 1) {
                uint32_t data = ibus_dat.read().to_uint();
                if (auto st = or1k.parse(data)) {
                    spdlog::get("or1k_logger")->info("@{:12s} {:08x} {:02x} {:02x} {:02x} {:02x} {}",
                                                     sc_time_stamp().to_string(), ibus_adr.read().to_uint(),
                                                     (data >> 24) & 0xff, (data >> 16) & 0xff, (data >> 8) & 0xff,
                                                     data & 0xff, *st);
                } else {
                    if (stop_on_invalid) {
                        std::stringstream err;
                        err << "OR1K_DEBUGGER: command 0x" << hex << data << " does not match any defined instruction";
                        SC_REPORT_ERROR("TLM-ROUTER", err.str().c_str());
                    } else {
                        spdlog::get("or1k_logger")->info("@{:12s} {:08x} {:02x} {:02x} {:02x} {:02x} invalid-instruction",
                                                         sc_time_stamp().to_string(), ibus_adr.read().to_uint(),
                                                         (data >> 24) & 0xff, (data >> 16) & 0xff, (data >> 8) & 0xff,
                                                         data & 0xff);
                    }
                }
            }
        }
    }

    void setup_logger(const char* filename) {
        // We setup n rotating logs to avoid consuming an excessive amount of memory
        auto max_size = 128*1024*1024;
        auto max_files = 3;
        spdlog::rotating_logger_mt(
                               "or1k_logger", filename, max_size, max_files);
        spdlog::set_pattern("%v");
        spdlog::flush_every(std::chrono::seconds(1));
    }

    explicit Or1kInstructionTracer(sc_module_name name, const char* filename="/workdir/build/cpu.csv", bool stop_on_invalid=false):
        sc_module(name), or1k(), stop_on_invalid(stop_on_invalid)
    {
        or1k.check_opcodes();
        setup_logger(filename);
        SC_METHOD(parse);
        sensitive << clk;
        dont_initialize();
    }

    SC_HAS_PROCESS(Or1kInstructionTracer);
};

#endif // __OR1K_INSTRUCTION_TRACER_H__
