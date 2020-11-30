
#ifndef __OR1K_DEFS__
#define __OR1K_DEFS__

#include <list>
#include <string>
#include <optional>
#include <sstream>

/**
 * Implementation of an OR1K descriptor
 *
 * This object allows the mapping of a bit-sequence against an instruction.
 */
class Or1kDescriptor {
public:
    std::string rep;
    uint32_t mask;
    uint32_t matcher;
    std::list<std::string> regs;

    explicit Or1kDescriptor(const std::string& rep, uint32_t mask, uint32_t matcher, const std::list<std::string>& regs):
        rep(rep), mask(mask), matcher(matcher), regs(regs) {
        if ((mask & matcher) != matcher)
            std::cerr << "opcode " << rep << " has mismatching mask/matcher" << std::endl;
    }
    explicit Or1kDescriptor(const std::string& rep, uint32_t mask, uint32_t matcher):
        Or1kDescriptor(rep, mask, matcher, std::list<std::string>()) {
    }

    bool matches(const uint32_t word) const {
        return ((word & mask) == matcher);
    }

    std::string to_string(uint32_t word) {
        std::string tmp {};
        for (auto r : regs) {
            if (r == "reg_d")
                tmp = tmp + reg_d(word) + ", ";
            else if (r == "reg_a")
                tmp = tmp + reg_a(word) + ", ";
            else if (r == "reg_b")
                tmp = tmp + reg_b(word) + ", ";
            else if (r == "imm_i")
                tmp = tmp + imm_i(word) + ", ";
            else if (r == "imm_k")
                tmp = tmp + imm_k(word) + ", ";
            else if (r == "imm_k2")
                tmp = tmp + imm_k2(word) + ", ";
            else if (r == "imm_l")
                tmp = tmp + imm_l(word) + ", ";
            else if (r == "imm_n")
                tmp = tmp + imm_n(word) + ", ";
        }
        return std::string(rep + " " + tmp);
    }

private:
    static inline uint32_t extend_sign(uint32_t dat, int i) {
        int32_t t = (dat << (31 - i));
        return t >> (31 - i);
    }

    static std::string reg_d(uint32_t word) {
        std::stringstream ss;
        ss << "r" << ((word >> 21) & 0xf);
        return ss.str();
    }

    static std::string reg_a(uint32_t word) {
        std::stringstream ss;
        ss << "r" << ((word >> 16) & 0xf);
        return ss.str();
    }

    static std::string reg_b(uint32_t word) {
        std::stringstream ss;
        ss << "r" << ((word >> 11) & 0xf);
        return ss.str();
    }

    static std::string imm_i(uint32_t word) {
        int32_t imm = static_cast<int32_t>(extend_sign(word, 15));
        std::stringstream ss;
        if (imm >= 0)
            ss << "0x" << std::hex << imm;
        else
            ss << imm;
        return ss.str();
    }

    static std::string imm_k(uint32_t word) {
        uint32_t imm = word & 0xffff;
        if (imm == 0)
            return "0";
        std::stringstream ss;
        ss << "0x" << std::hex << imm;
        return ss.str();
    }

    static std::string imm_k2(uint32_t word) {
        uint32_t imm = (((word >> 21) & 0xf) << 11) |
                       (word & 0x3ff);
        if (imm == 0)
            return "0";
        std::stringstream ss;
        ss << "0x" << std::hex << imm;
        return ss.str();
    }

    static std::string imm_l(uint32_t word) {
        uint32_t imm = word & 0x3f;
        if (imm == 0)
            return "0";
        std::stringstream ss;
        ss << "0x" << std::hex << imm << std::dec;
        return ss.str();
    }

    static std::string imm_n(uint32_t word) {
        int32_t imm = static_cast<int32_t>(extend_sign(word, 25));
        std::stringstream ss;
        if (imm >= 0)
            ss << "0x" << std::hex << imm;
        else
            ss << imm;
        return ss.str();
    }
};

/**
 * Implementation of an OR1K Definitions
 *
 * This object define the OR1K instruction set mapping.
 */
class Or1kDefs {
private:
    // Constants specific to the OR1K
    const uint32_t SHIFT_6BITS = 26;
    const uint32_t MASK_6BITS  = 0x3F << SHIFT_6BITS;
    const uint32_t SHIFT_8BITS = 24;
    const uint32_t MASK_8BITS  = 0xFF << SHIFT_8BITS;
    const uint32_t SHIFT_11BITS = 21;
    const uint32_t MASK_11BITS  = 0x7FF << SHIFT_11BITS;
    const uint32_t SHIFT_16BITS  = 16;
    const uint32_t MASK_16BITS  = 0xFFFF << SHIFT_16BITS;
    const uint32_t ALU_MATCHER = (0x38 << SHIFT_6BITS);
    const uint32_t ALU_MASK_ARITH = MASK_6BITS + (0x3 << 8) + 0xF;
    const uint32_t ALU_MASK_LOGIC = MASK_6BITS + (0xF << 6) + 0xF;
    const uint32_t SHIFTER_MATCHER = (0x2e << SHIFT_6BITS);
    const uint32_t SHIFTER_MASK = MASK_6BITS + (0x3 << 6);
    const uint32_t MAC_MATCHER = (0x31 << SHIFT_6BITS);
    const uint32_t MAC_MASK = MASK_6BITS + 0xF;
    const uint32_t MASK_32BITS = 0xFFFFFFFF;

public:
    Or1kDefs() {}

    std::list<Or1kDescriptor> get_opcodes() {
        std::list<Or1kDescriptor> entries;
        /* ORBIS32 */
        entries.push_back(Or1kDescriptor("l.nop", MASK_8BITS, (0x15) << SHIFT_8BITS, {"imm_k"}));
        entries.push_back(Or1kDescriptor("l.mfspr", MASK_6BITS, (0x2d) << SHIFT_6BITS, {"reg_d", "reg_a", "imm_k"}));
        entries.push_back(Or1kDescriptor("l.mtspr", MASK_6BITS, (0x30) << SHIFT_6BITS, {"reg_a", "reg_b", "imm_k2"}));
        entries.push_back(Or1kDescriptor("l.movhi", MASK_6BITS, (0x6 << SHIFT_6BITS), {"reg_d", "imm_k"} ));
        /* Control */
        entries.push_back(Or1kDescriptor("l.j", MASK_6BITS, (0 << SHIFT_6BITS), {"imm_n"} ));
        entries.push_back(Or1kDescriptor("l.jr", MASK_6BITS, (0x11 << SHIFT_6BITS), {"reg_b"} ));
        entries.push_back(Or1kDescriptor("l.jal", MASK_6BITS, (0x1 << SHIFT_6BITS), {"imm_n"} ));
        entries.push_back(Or1kDescriptor("l.jalr", MASK_6BITS, (0x12 << SHIFT_6BITS), {"reg_b"} ));
        entries.push_back(Or1kDescriptor("l.bf", MASK_6BITS, (0x4 << SHIFT_6BITS), {"imm_n"} ));
        entries.push_back(Or1kDescriptor("l.bnf", MASK_6BITS, (0x3 << SHIFT_6BITS), {"imm_n"} ));

        /* Load & Store */
        entries.push_back(Or1kDescriptor("l.lwa", MASK_6BITS, (0x1b << SHIFT_6BITS) ));
        entries.push_back(Or1kDescriptor("l.lwz", MASK_6BITS, (0x21 << SHIFT_6BITS), {"reg_d", "imm_i", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.lws", MASK_6BITS, (0x22 << SHIFT_6BITS), {"reg_d", "imm_i", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.lhz", MASK_6BITS, (0x25 << SHIFT_6BITS), {"reg_d", "imm_i", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.lhs", MASK_6BITS, (0x26 << SHIFT_6BITS), {"reg_d", "imm_i", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.lbz", MASK_6BITS, (0x23 << SHIFT_6BITS), {"reg_d", "imm_i", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.lbs", MASK_6BITS, (0x24 << SHIFT_6BITS), {"reg_d", "imm_i", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.swa", MASK_6BITS, (0x33 << SHIFT_6BITS) ));
        entries.push_back(Or1kDescriptor("l.sw", MASK_6BITS, (0x35 << SHIFT_6BITS), {"imm_i2", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.sh", MASK_6BITS, (0x37 << SHIFT_6BITS), {"imm_i2", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.sb", MASK_6BITS, (0x36 << SHIFT_6BITS), {"imm_i2", "reg_a", "reg_b"} ));
        /* Sign/Zero Extend */
        entries.push_back(Or1kDescriptor("l.extwz", ALU_MASK_LOGIC, ALU_MATCHER + (0x1 << 6) + 0xd, {"reg_d", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.extws", ALU_MASK_LOGIC, ALU_MATCHER + 0xd, {"reg_d", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.exthz", ALU_MASK_LOGIC, ALU_MATCHER + (0x2 << 6) + 0xc, {"reg_d", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.exths", ALU_MASK_LOGIC, ALU_MATCHER + 0xc, {"reg_d", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.extbz", ALU_MASK_LOGIC, ALU_MATCHER + (0x3 << 6) + 0xc, {"reg_d", "reg_a"} ));
        entries.push_back(Or1kDescriptor("l.extbs", ALU_MASK_LOGIC, ALU_MATCHER + (0x1 << 6) + 0xc, {"reg_d", "reg_a"} ));

        /* ALU (reg, reg) */
        entries.push_back(Or1kDescriptor("l.add", ALU_MASK_ARITH, ALU_MATCHER + 0x0, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.addc", ALU_MASK_ARITH, ALU_MATCHER + 0x1, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.sub", ALU_MASK_ARITH, ALU_MATCHER + 0x2, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.and", ALU_MASK_ARITH, ALU_MATCHER + 0x3, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.or", ALU_MASK_ARITH, ALU_MATCHER + 0x4, {"reg_d", "reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.xor", ALU_MASK_ARITH, ALU_MATCHER + 0x5, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.cmov", ALU_MASK_ARITH, ALU_MATCHER + 0xe, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.ff1", ALU_MASK_ARITH, ALU_MATCHER + 0xf, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.cmov", ALU_MASK_ARITH, ALU_MATCHER + 0xe, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.ff1", ALU_MASK_ARITH, ALU_MATCHER + 0xf ));
        entries.push_back(Or1kDescriptor("l.fl1", ALU_MASK_ARITH, ALU_MATCHER + (0x1 << 8) + 0xf, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.sll", ALU_MASK_LOGIC, ALU_MATCHER + 0x8, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.srl", ALU_MASK_LOGIC, ALU_MATCHER + (0x1 << 8) + 0x8, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.sra", ALU_MASK_LOGIC, ALU_MATCHER + (0x2 << 8) + 0x8, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.ror", ALU_MASK_LOGIC, ALU_MATCHER + (0x3 << 8) + 0x8, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.mul", ALU_MASK_ARITH, ALU_MATCHER + (0x3 << 8) + 0x6, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.mulu", ALU_MASK_ARITH, ALU_MATCHER + (0x3 << 8) + 0xb, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.muld", ALU_MASK_ARITH, ALU_MATCHER + (0x3 << 8) + 0x7, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.muldu", ALU_MASK_ARITH, ALU_MATCHER + (0x3 << 8) + 0xc, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.div", ALU_MASK_ARITH, ALU_MATCHER + (0x3 << 8) + 0x9, {"reg_d", "reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.divu", ALU_MASK_ARITH, ALU_MATCHER + (0x3 << 8) + 0xa, {"reg_d", "reg_a", "reg_b"} ));

        /* ALU (reg, imm) */
        entries.push_back(Or1kDescriptor("l.addi", MASK_6BITS, (0x27 << SHIFT_6BITS), {"reg_d", "reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.addic", MASK_6BITS, (0x28 << SHIFT_6BITS), {"reg_d", "reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.andi", MASK_6BITS, (0x29 << SHIFT_6BITS), {"reg_d", "reg_a", "imm_k"}  ));
        entries.push_back(Or1kDescriptor("l.ori", MASK_6BITS, (0x2a << SHIFT_6BITS), {"reg_d", "reg_a", "imm_k"}  ));
        entries.push_back(Or1kDescriptor("l.xori", MASK_6BITS, (0x2b << SHIFT_6BITS), {"reg_d", "reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.muli", MASK_6BITS, (0x2c << SHIFT_6BITS), {"reg_d", "reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.slli", SHIFTER_MASK, SHIFTER_MATCHER, {"reg_d", "reg_a", "imm_l"}  ));
        entries.push_back(Or1kDescriptor("l.srli", SHIFTER_MASK, SHIFTER_MATCHER + (0x1<<6), {"reg_d", "reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.srai", SHIFTER_MASK, SHIFTER_MATCHER + (0x2<<6), {"reg_d", "reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.rori", SHIFTER_MASK, SHIFTER_MATCHER + (0x3<<6), {"reg_d", "reg_a", "imm_i"} ));

        /* Comparison (reg, reg) */
        entries.push_back(Or1kDescriptor("l.sfeq", MASK_11BITS, 0x720  << SHIFT_11BITS, {"reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.sfne", MASK_11BITS, 0x721  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.sfgtu", MASK_11BITS, 0x722  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.sfgeu", MASK_11BITS, 0x723  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.sfltu", MASK_11BITS, 0x724  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.sfleu", MASK_11BITS, 0x725  << SHIFT_11BITS, {"reg_a", "reg_b"} ));
        entries.push_back(Or1kDescriptor("l.sfgts", MASK_11BITS, 0x72a  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.sfges", MASK_11BITS, 0x72b  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.sflts", MASK_11BITS, 0x72c  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));
        entries.push_back(Or1kDescriptor("l.sfles", MASK_11BITS, 0x72d  << SHIFT_11BITS, {"reg_a", "reg_b"}  ));

        /* Comparison (reg, imm) */
        entries.push_back(Or1kDescriptor("l.sfeqi", MASK_11BITS, 0x5e0  << SHIFT_11BITS, {"reg_a", "imm_i"}  ));
        entries.push_back(Or1kDescriptor("l.sfnei", MASK_11BITS, 0x5e1  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sfgtui", MASK_11BITS, 0x5e2  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sfgeui", MASK_11BITS, 0x5e3  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sfltui", MASK_11BITS, 0x5e4  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sfleui", MASK_11BITS, 0x5e5  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sfgtsi", MASK_11BITS, 0x5ea  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sfgesi", MASK_11BITS, 0x5eb  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sfltsi", MASK_11BITS, 0x5ec  << SHIFT_11BITS, {"reg_a", "imm_i"} ));
        entries.push_back(Or1kDescriptor("l.sflesi", MASK_11BITS, 0x5ed  << SHIFT_11BITS, {"reg_a", "imm_i"} ));

        /* Multiply Accumulate */
        entries.push_back(Or1kDescriptor("l.mac", MAC_MASK, MAC_MATCHER + 0x1 ));
        entries.push_back(Or1kDescriptor("l.macu", MAC_MASK, MAC_MATCHER + 0x3 ));
        entries.push_back(Or1kDescriptor("l.msb", MAC_MASK, MAC_MATCHER  + 0x2 ));
        entries.push_back(Or1kDescriptor("l.msbu", MAC_MASK, MAC_MATCHER + 0x4 ));
        entries.push_back(Or1kDescriptor("l.maci", MASK_11BITS,  0x13 << SHIFT_11BITS ));
        entries.push_back(Or1kDescriptor("l.macrc", MASK_6BITS+0x1FFFF, (0x6 << SHIFT_6BITS) + 0x10000));

        /* System Interface */
        entries.push_back(Or1kDescriptor("l.sys", MASK_16BITS, 0x2000 << SHIFT_16BITS ));
        entries.push_back(Or1kDescriptor("l.trap", MASK_16BITS, 0x2001 << SHIFT_16BITS ));
        entries.push_back(Or1kDescriptor("l.msync", MASK_32BITS, 0x22000000 ));
        entries.push_back(Or1kDescriptor("l.psync", MASK_32BITS, 0x22800000 ));
        entries.push_back(Or1kDescriptor("l.csync", MASK_32BITS, 0x23000000 ));
        entries.push_back(Or1kDescriptor("l.rfe", MASK_6BITS, (0x9 << SHIFT_6BITS)));
        return entries;
    }
};
#endif //__OR1K_DEFS__
