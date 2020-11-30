/**
 * @file N25QX.hpp
 * @author Riverlane, 2020
 *  @brief A simplified quad-spi flash based on the N25Q
 */
#ifndef __N25QX__
#define __N25QX__

#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <sstream>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"


using namespace std;
using namespace sc_dt;

/**
 * N25QX defines a simplified model of a quad-spi (N25Q).
 * Out of the standard commands available, only READ_QUAD is
 * currently implemented
*/
template <int IF_WIDTH=4>
SC_MODULE(N25QX) {
    // N25Q-128M
    // Command table:
    // RESET ENABLE 0x66
    // RESET MEMORY 0x99
    // READ ID 0x9E 0x9F
    // READ 0x3
    // FAST READ 0xB
    sc_in<sc_bv<1>>  cs_n;
    sc_inout<sc_bv<IF_WIDTH>> dq;
    sc_in<bool> clk;

    const uint8_t CMD_QUAD_READ = 0xeb;

    enum class PHASE_T {IDLE_PHASE,
                        CMD_PHASE,
                        QUAD_READ_ADDR_PHASE,
                        QUAD_READ_DUMMY_PHASE,
                        QUAD_READ_RESP_PHASE
                       };
    PHASE_T phase = PHASE_T::IDLE_PHASE;

    uint8_t cmd;
    uint32_t cnt ;
    uint32_t addr;

    std::map<uint32_t, uint8_t> conf_memory;

    uint32_t dummy_cycles = 0;

    sc_bv<IF_WIDTH> spi_io_out;

    uint32_t idx = 0;
    uint8_t word = 0;

    void flash_handler() {
        if (clk) {
            switch(phase) {
            case PHASE_T::IDLE_PHASE:
                if (cs_n.read() == 0) {
                    cnt = 1;
                    phase = PHASE_T::CMD_PHASE;
                    cmd = dq.read().get_bit(0);
                }
                break;
            case PHASE_T::CMD_PHASE:
                if (cnt++ < 8) {
                    cmd = (uint8_t) (dq.read().get_bit(0)) | (cmd << 1);
                } else {
                    if (cmd == CMD_QUAD_READ) {
                        phase = PHASE_T::QUAD_READ_ADDR_PHASE;
                        cnt = 1;
                        addr = dq.read().to_uint();
                    }
                }
                break;
            case PHASE_T::QUAD_READ_ADDR_PHASE:
                if (cnt++ < (24/ IF_WIDTH )) {
                    addr = (addr << IF_WIDTH) | dq.read().to_uint();
                } else {
                    cnt = 2;
                    dq.write(0);
                    phase = PHASE_T::QUAD_READ_DUMMY_PHASE;
                }
                break;
            case PHASE_T::QUAD_READ_DUMMY_PHASE:
                dq.write(0);
                if (++cnt >= dummy_cycles) {
                    phase = PHASE_T::QUAD_READ_RESP_PHASE;
                    cnt = 0;
                    idx = 0;
                    word = 0;
                }
                break;
            case PHASE_T::QUAD_READ_RESP_PHASE:
                if (cs_n.read()==0) {
                    // Our address is half-byte aligned
                    idx = (addr*2)+cnt;
                    word = conf_memory[idx];
                    cnt++;
                    dq.write(word);
                } else {
                    phase = PHASE_T::IDLE_PHASE;
                }
                break;
            }
        } else {
            if (phase == PHASE_T::QUAD_READ_RESP_PHASE) {
                spdlog::get("N25QX_logger")->info("{},0x{:x},{}, 0x{:x}",
                                                  sc_time_stamp().to_string(), addr+(cnt/2), idx, word);
                dq.write(word);
            }
        }
    }

    void setup_logger(const char* filename) {
        // We setup n rotating logs to avoid consuming an excessive amount of memory
        auto max_size = 128*1024*1024;
        auto max_files = 3;
        auto file_logger = spdlog::rotating_logger_mt("N25QX_logger", filename, max_size, max_files);
        spdlog::set_pattern("%v");
        spdlog::info("TimeStamp, Addr, Idx, Value");
        spdlog::flush_every(std::chrono::seconds(1));
    }


    void configure_region(std::string conf_file, uint32_t conf_addr) {
        // Conf Memory
        // Our memory is internally organized in blocks of half-words and we address it
        // via a half-word address.
        uint32_t address = conf_addr << 1;
        std::ifstream is (conf_file, std::ifstream::binary);
        if (is) {
            // get length of file:
            is.seekg (0, is.end);
            int length = is.tellg();
            is.seekg (0, is.beg);
            char * buffer = new char [length];
            // read data as a block:
            is.read (buffer,length);
            for (int i = 0; i<length; i++)
            {
                // Splitting in two half-words
                conf_memory[address++] = (((uint8_t)buffer[i])>>4) & 0xF;
                conf_memory[address++] = ((uint8_t)buffer[i]) & 0xF;
            }
            is.close();
            delete [] buffer;
        }
    }

    N25QX(sc_module_name name, uint32_t dummy_cycles=11, const char* filename="/workdir/build/N25QX.csv")
        : sc_module(name),  dummy_cycles(dummy_cycles)
    {
        setup_logger(filename);
        SC_METHOD(flash_handler);
        sensitive << clk;
        dont_initialize();
    }

    SC_HAS_PROCESS(N25QX);
};

#endif


