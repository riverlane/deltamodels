/**
 * @file or1k_decoder.hpp
 * @author Riverlane, 2020
 * @brief Implementation of a generic instruction decoder
 */

#ifndef __OR1K_DECODER_H__
#define __OR1K_DECODER_H__

using namespace std;
#include "or1k_defs.hpp"

/**
 * Implementation of a generic instruction decoder
 *
 * A list of descriptors is currently loaded at construction time,
 * we will have a dynamic loading of different instruction sets and
 * correspondent masks, matchers and tostring.
 */
class Or1kDecoder {
public:
    std::list<Or1kDescriptor> descriptors;
    Or1kDecoder(): descriptors(Or1kDefs().get_opcodes()) {}

    std::optional<std::string> parse(uint32_t word) {
        std::list<Or1kDescriptor>::iterator it = std::find_if (
                descriptors.begin(), descriptors.end(), 
                [&] (Or1kDescriptor const& m) { return m.matches(word);});
        if (it != descriptors.end())        
            return (*it).to_string(word);
        else
            return {};
    }

    bool check_opcodes() {
        for (auto d : descriptors) {
            for (auto k : descriptors) {
                if (d.rep != k.rep) {
                    if (d.matcher == k.matcher) {
                        std::cerr << "check_opcodes: " << d.rep << " and " << k.rep << " have the same matcher!" << std::endl;
                        return false;
                    }
                }
            }
        }
        return true;

    }
};
#endif //__OR1K_DECODER_H__
