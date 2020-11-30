/**
 * @file or1k_data_tracer.hpp
 * @author Riverlane, 2020
 * @brief SystemC Implementation of a wishbone CPU tracer
 */

#ifndef __OR1K_DATA_TRACER_H__
#define __OR1K_DATA_TRACER_H__

#include "systemc.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

using namespace std;

/**
 * SystemC Implementation of a wishbone CPU tracer
 *
 * This block can be used to trace transactions generated from a CPU.
 */
template <int AWIDTH, int DWIDTH>
struct Or1kDataTracer: sc_module
{
    sc_in<sc_bv<1>> clk;
    sc_in<sc_bv<AWIDTH>> dbus_adr;
    sc_in<sc_bv<DWIDTH>> dbus_dat_w;
    sc_in<sc_bv<DWIDTH>> dbus_dat_r;
    sc_in<sc_bv<1>> dbus_ack;
    sc_in<sc_bv<1>> dbus_we;

    void trace() {
        if (clk.read().to_uint() == 1) {
            if (dbus_ack.read().to_uint() == 1) {
                uint32_t add = dbus_adr.read().to_uint();
                if (dbus_we.read().to_uint() == 1) {
                    uint32_t data_w = dbus_dat_w.read().to_uint();
                    spdlog::get("or1k_data_logger")->info("@{:12s}, {:08x}, {:08x}, WRITE",
                                                          sc_time_stamp().to_string(), add, data_w);
                } else {
                    uint32_t data_r = dbus_dat_w.read().to_uint();
                    spdlog::get("or1k_data_logger")->info("@{:12s}, {:08x}, {:08x}, READ",
                                                          sc_time_stamp().to_string(), add, data_r);

                }

            }
        }
    }

    void setup_logger(const char* filename) {
        // We setup n rotating logs to avoid consuming an excessive amount of memory
        auto max_size = 128*1024*1024;
        auto max_files = 3;
        auto file_logger = spdlog::rotating_logger_mt(
                               "or1k_data_logger", filename, max_size, max_files);
        spdlog::set_pattern("%v");
        spdlog::get("or1k_data_logger")->info("TimeStamp, Address, Data(read or written), operation");
        spdlog::flush_every(std::chrono::seconds(1));
    }

    explicit Or1kDataTracer(sc_module_name name, const char* filename="/workdir/build/cpu_data.csv"):
        sc_module(name)
    {
        setup_logger(filename);
        SC_METHOD(trace);
        sensitive << clk;
        dont_initialize();
    }

    SC_HAS_PROCESS(Or1kDataTracer);
};

#endif // __OR1K_DATA_TRACER_H__
