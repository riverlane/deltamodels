/**
 * @file generic_sdram.hpp
 * @author Riverlane, 2020
 * @brief A simplified SDRAM model with backdoors
 */

#ifndef __GENERIC_SDRAM__
#define __GENERIC_SDRAM__

#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <sstream>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <map>
#include <iomanip>

using namespace std;
using namespace sc_dt;

struct SDRAM_GEOM {
    int row_bits = 0;
    int bank_bits = 0;
    int col_bits = 0;
};


/**
 * A simplified SDRAM model with backdoors.
 * The backdoors are used to accelerate the movement and erasing of data
*/
template <int A_SIZE, int BA_SIZE>
SC_MODULE(GENERIC_SDRAM) {
    /* GENERIC_SDRAM https://www.nxp.com/files-static/training_pdf/VFTF09_AN108.pdf
    COMMANDS TABLE:
    Command         /CS     /RAS    /CAS    /WE     ADDR
    NOP             H       X       X       X       X
    NOP             L       H       H       H       X
    ACTIVE          L       L       H       H       BA, Row
    READ            L       H       L       H       BA, Col
    WRITE           L       H       L       L       BA, Col
    PRECHARGE       L       L       H       L       BA
    PRECHARGE_ALL   L       L       H       L       A[10]
    REFRESH         L       L       L       H       X
    LOAD MEM REG    L       L       L       L       Bank, Opcode
    */

    sc_in<sc_bv<1>>  ck;
    sc_in<sc_bv<1>>  cs_n;
    sc_in<sc_bv<A_SIZE>> a;
    sc_in<sc_bv<BA_SIZE>> ba;
    sc_inout<sc_bv<32>> dq;
    sc_in<sc_bv<32/8>> dm;
    sc_in<sc_bv<1>> cke;
    sc_in<sc_bv<1>> ras_n;
    sc_in<sc_bv<1>> cas_n;
    sc_in<sc_bv<1>> we_n;

    sc_in<sc_bv<32>> backdoor_copy_from;
    sc_in<sc_bv<32>> backdoor_copy_to;
    sc_in<sc_bv<32>> backdoor_copy_size;
    sc_in<sc_bv<1>> backdoor_copy;
    sc_in<sc_bv<32>> backdoor_clear_from;
    sc_in<sc_bv<32>> backdoor_clear_to;
    sc_in<sc_bv<1>> backdoor_clear;

    static constexpr uint8_t CS_H = 0x10;
    static constexpr uint8_t CS_L = 0x00;
    static constexpr uint8_t RAS_H = 0x08;
    static constexpr uint8_t RAS_L = 0x00;
    static constexpr uint8_t CAS_H = 0x04;
    static constexpr uint8_t CAS_L = 0x00;
    static constexpr uint8_t WE_H = 0x02;
    static constexpr uint8_t WE_L = 0x00;
    static constexpr uint8_t A10_H = 0x01;
    static constexpr uint8_t A10_L = 0x00;

    enum class CMD_T: uint8_t {NOP_0 = CS_H,
                               NOP_1 = RAS_H + CAS_H + WE_H,
                               ACTIVATE = CAS_H + WE_H,
                               READ = RAS_H + WE_H,
                               WRITE = RAS_H,
                               PRECHARGE = CAS_H,
                               PRECHARGE_ALL = CAS_H + A10_H,
                               REFRESH = WE_H,
                               LOAD_MEM_REG = 0
                              };

    std::map<uint32_t, uint32_t> mem;
    std::vector<uint32_t> read_buf;

    uint32_t bank_addr {0};
    uint32_t col_addr {0};
    static const int ROW_SIZE = 1<<BA_SIZE; 
    uint32_t row_addr [ROW_SIZE] = {0};

    bool verbose {false};
    bool extra_verbose {false};

    SDRAM_GEOM geometry;
    const uint32_t OFFSET;

    uint32_t update_address() {
        // NOTE: the addressing mode is half-word
        return to_word_address(row_addr[bank_addr], bank_addr, col_addr);
    }

    inline uint32_t to_address(uint32_t row, uint32_t bank, uint32_t col){
        return to_word_address(row, bank, col) << 2;
    }

    uint32_t to_word_address(uint32_t& row, uint32_t& bank, uint32_t& col){
        /* Minicon maps a wishbone address as:
        ROW    - BANK    - COL
        */
        return (row << (geometry.bank_bits+geometry.col_bits)) + (bank << (geometry.col_bits)) + (col);
    }

    void to_row_bank_column(uint32_t addr, uint32_t& row, uint32_t& bank, uint32_t& col){
        addr >>= 2;
        row = (addr >> (geometry.bank_bits+geometry.col_bits)) & ((1 << geometry.row_bits) - 1);
        bank = (addr >> geometry.col_bits) & ((1 << geometry.bank_bits) - 1);
        col = addr & ((1 << geometry.col_bits) - 1 );
    }

    void handle_cmd(CMD_T cmd) {
        uint32_t address;
        uint32_t data, prev_data;
        uint32_t byte_disable;
        uint32_t mask;
        switch(cmd) {
        case CMD_T::NOP_0:
        case CMD_T::NOP_1:
            if (!read_buf.empty()) {
                spdlog::get("SDRAM_logger")->info("{},READ[RETURNED],-,0x{:x}", sc_time_stamp().to_string(), read_buf.back());
                dq.write(read_buf.back());
                read_buf.pop_back();
            }
            break;
        case CMD_T::ACTIVATE:
            bank_addr = ba.read().to_uint();
            row_addr[bank_addr] = a.read().to_uint();
            spdlog::get("SDRAM_logger")->info("{},ACTIVATE,-,-", sc_time_stamp().to_string());
            break;
        case CMD_T::READ:
            bank_addr = ba.read().to_uint();
            col_addr = a.read().to_uint();
            address = update_address();
            if (mem.count(address) == 0) {
                read_buf.push_back(0);
            } else {
                read_buf.push_back(mem[address]);
            }
            spdlog::get("SDRAM_logger")->info("{},READ[REQUESTED],0x{:x},[0x{:x}]", sc_time_stamp().to_string(), toWishbone(address), mem[address]);
            break;
        case CMD_T::WRITE:
            bank_addr = ba.read().to_uint();
            col_addr = a.read().to_uint();
            address = update_address();
            data = dq.read().to_uint();
            byte_disable = dm.read().to_uint();
            prev_data = mem[address];
            mask = 0;
            for (int i=0; i<4; i++) {
                if ((byte_disable << (i)) & 0x1) {
                    mask = mask + (0xff << (i*8));
                }
            }
            mem[address] = (data & (~mask)) | (prev_data & mask);
            spdlog::get("SDRAM_logger")->info("{},WRITE,0x{:x},0x{:x}", sc_time_stamp().to_string(), toWishbone(address), mem[address]);
            break;
        case CMD_T::PRECHARGE:
            bank_addr = ba.read().to_uint();
            row_addr[bank_addr] = 0;
            spdlog::get("SDRAM_logger")->info("{},PRECHARGE,-,-", sc_time_stamp().to_string());
            break;
        case CMD_T::PRECHARGE_ALL:
            for (int i=0; i < ROW_SIZE; i++){
                row_addr[i] = 0;
            }
            spdlog::get("SDRAM_logger")->info("{},PRECHARGE_ALL,-,-", sc_time_stamp().to_string());
            break;
        case CMD_T::REFRESH:
            spdlog::get("SDRAM_logger")->info("{},REFRESH,-,-", sc_time_stamp().to_string());
            break;
        case CMD_T::LOAD_MEM_REG:
            spdlog::get("SDRAM_logger")->info("{},LOAD_MEM_REG,x,x", sc_time_stamp().to_string());
            break;
        }
    }

    void sdram_handler() {
        if ((ck.read() & cke.read()) == 1) {
            sc_bv<A_SIZE> add = a.read().to_int();
            uint8_t agg;
            if (cs_n.read() == 1) {
                agg = CS_H;
            } else {
                agg = (ras_n.read().to_uint() << 3) + (cas_n.read().to_uint() << 2) + 
                    (we_n.read().to_uint() << 1) + (add.get_bit(10));
            }
            handle_cmd(CMD_T{agg});
        }
    }

    uint32_t fromWishbone(uint32_t addr) {
        return (addr - OFFSET) >> 2;
    }

    uint32_t toWishbone(uint32_t addr) {
        return (addr<<2) + OFFSET;
    }

    void backdoor_handler() {
        uint32_t start_add, end_add;
        if (backdoor_copy.read() == 1) {
            start_add = fromWishbone(backdoor_copy_from.read().to_uint());
            end_add = fromWishbone(backdoor_copy_to.read().to_uint());
            uint32_t tsize = backdoor_copy_size.read().to_uint() >> 2;
            for (uint32_t i=0; i<tsize; i++) {
                spdlog::get("SDRAM_logger")->info("{},BACKDOOR_COPY,0x{:x}, 0x{:x}",
                                                  sc_time_stamp().to_string(),
                                                  toWishbone(end_add+i), mem[start_add+i]);
                mem[end_add+i] = mem[start_add+i];
            }
        }
        if (backdoor_clear.read() == 1) {
            start_add = fromWishbone(backdoor_clear_from.read().to_uint());
            end_add = fromWishbone(backdoor_clear_to.read().to_uint());
            for (uint32_t i=start_add; i<end_add; i++) {
                spdlog::get("SDRAM_logger")->info("{},BACKDOOR_CLEAR,0x{:x}, 0",
                                                  sc_time_stamp().to_string(), toWishbone(i), 0);
                mem[i] = 0;
            }
        }
    }


    void setup_logger(const char* filename) {
        // We setup n rotating logs to avoid consuming an excessive amount of memory
        auto max_size = 128*1024*1024;
        auto max_files = 3;
        auto file_logger = spdlog::rotating_logger_mt(
                               "SDRAM_logger", filename, max_size, max_files);
        spdlog::set_pattern("%v");
        spdlog::flush_every(std::chrono::seconds(1));
        spdlog::get("SDRAM_logger")->info("TimeStamp, CMD, Address, Value");
    }


    void configure_region(std::string conf_file, uint32_t conf_addr, uint32_t first_byte=0, bool msb=false) {
        std::ifstream is (conf_file, std::ifstream::binary);
        if (is) {
            // get length of file:
            is.seekg (first_byte, is.end);
            int length = is.tellg();
            is.seekg (first_byte, is.beg);
            char * buffer = new char [length];
            // read data as a block:
            is.read ((char *) buffer, length);
            is.close();
            // Start saving it as a map
            int rem = length % 4;
            uint32_t val = 0;
            // Address needs to be word aligned
            conf_addr = conf_addr >> 2;
            for (int i = 0; i<(length-rem); i=i+4)
            {
                memcpy(&val, &(buffer[i]), 4);
                if (msb) {
                    mem[conf_addr++] = val;
                } else {
                    mem[conf_addr++] = __bswap_32(val);
                }
            };
            // Copy last bytes.
            if (rem) {
                memcpy(&val, &(buffer[length-rem]), rem);
                mem[conf_addr] = val;
            }
        }
    }

    void hexdump(std::string filename) {
        ofstream myfile (filename);
        int cnt = 0;
        std::string line;
        uint32_t last_address = 0;
        for( auto const& [key, val] : mem )
        {
            uint32_t address = key*4;
            // There was a jump in the map
            if (address > (last_address+4)){
                myfile << std::endl;
                cnt = 0;
            }
            if (cnt % 4 == 0)
                myfile << std::setfill ('0') << std::setw(8) << std::hex << (unsigned int)(address);
            for (int i=0; i<4; i++) {
                myfile << " " << std::setfill ('0') << std::setw(2) <<
                       std::hex << (unsigned int) ((val >>((3-i) * 8)) & 0xff) ;
            }
            if (cnt % 4 == 3)
                myfile << std::endl;
            cnt++;
            // keeping track of the previous address.
            last_address = address;
        }
        myfile.close();
    }

    GENERIC_SDRAM(sc_module_name name, SDRAM_GEOM geom, const char* filename="/workdir/build/sdram.csv", const uint32_t OFFSET=0x40000000)
        : sc_module(name), geometry(geom), OFFSET(OFFSET)
    {
        setup_logger(filename);
        SC_METHOD(sdram_handler);
        sensitive << ck << cke;
        dont_initialize();
        SC_METHOD(backdoor_handler);
        sensitive << backdoor_copy << backdoor_clear;
        dont_initialize();
    }

    SC_HAS_PROCESS(GENERIC_SDRAM);
};

#endif // __GENERIC_SDRAM__


