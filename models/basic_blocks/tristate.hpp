#ifndef __TRISTATE_H__
#define __TRISTATE_H__

#include <systemc.h>


using namespace std;
using namespace sc_dt;

template <int W>
struct TriState: sc_module {

    sc_out<sc_bv<W>> dout;
    sc_in<sc_bv<W>> din;
    sc_inout<sc_bv<W>> dinout;
    sc_in<sc_bv<1>> oe;

    void assign()
    {
        if (oe.read() == 1) {
            dinout.write(din);
        }
        else
        {
            dout.write(dinout);
        }
    }
    SC_CTOR(TriState)
    {
        SC_METHOD(assign);
        sensitive << oe << din << dinout;
    }
};

#endif //__TRISTATE_H__
