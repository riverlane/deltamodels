/*
This file contains:
- a module that converts and sc_in_clk into a simple sc_out.
- a module that converts an sc_bv<1> into bool (to enable,
use the .pos_edge() method)
2020 Tom Parks, Marco Ghibaudi Riverlane
*/
#ifndef __TESTBENCH_UTILS__
#define __TESTBENCH_UTILS__

#include <systemc>

using namespace std;

template<class T>
SC_MODULE(ClkCast) {
    sc_in_clk clk;
    sc_out<T> clkout;

    void run() {
        clkout.write(clk.read());
    }

    SC_CTOR(ClkCast) {
        SC_METHOD(run);
        sensitive << clk;
    }
};

using Clk2BV = ClkCast<sc_bv<1>>;
using Clk2Bool = ClkCast<bool>;


SC_MODULE(BvToBool) {
    sc_in<sc_bv<1>> in;
    sc_out<bool> out;

    void convert() {
        out.write((bool)in.read().to_int());
    }

    SC_CTOR(BvToBool) {
        SC_METHOD(convert);
        sensitive << in;
    }
};

/*
SC_MODULE(BoolToBv) {
  sc_in<bool> sig_in;
  sc_out<sc_bv<1>> sig_out;
  void run() {
    sig_out.write(sig_in.read());
  }
  SC_CTOR(BoolToBv) {
    SC_METHOD(run);
    sensitive << sig_in;
  }
};
*/


#endif //__TESTBENCH_UTILS__
