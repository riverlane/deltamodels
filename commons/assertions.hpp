#ifndef ASSERTIONS_H
#define ASSERTIONS_H

#include "systemc"
#include "vector"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include <sstream>

template <class T>
static void checkValuesMatch(T val1, T val2, const char* msg) {
    std::ostringstream ostr (std::ostringstream::ate);
    if (val1 != val2) {
        ostr << msg << " failed: Value 0x" << hex << val1 << " does not match with 0x" << hex << val2 << endl;
        SC_REPORT_ERROR("ASSERTION_MISMATCH", ostr.str().c_str());
    }
}

template <class T>
static void checkValuesMatch(std::vector<T> val1, std::vector<T> val2, const char* msg) {
    std::ostringstream ostr (std::ostringstream::ate);
    if (val1 != val2) {
        ostr << msg << " failed: first value ";
        for(auto n : val1) {
            ostr << hex << static_cast<unsigned>(n);
        }
        ostr << " doesn't match with ";
        for(auto n : val2) {
            ostr << hex << static_cast<unsigned>(n);
        }
        ostr << endl;
        SC_REPORT_ERROR("ASSERTION_MISMATCH", ostr.str().c_str());
    }
}

template <class T>
static void checkValuesDifferentFrom(T val1, T val2, const char* msg) {
    std::ostringstream ostr (std::ostringstream::ate);
    if (val1 == val2) {
        ostr << msg << " failed: Value 0x" << hex << val1 << " does not match with 0x" << hex << val2 << endl;
        SC_REPORT_ERROR("ASSERTION_MISMATCH", ostr.str().c_str());
    }
}

#endif


