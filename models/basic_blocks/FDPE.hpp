#ifndef FDPE_H
#define FDPE_H

#include <systemc.h>


using namespace std;
using namespace sc_dt;

struct FDPE: sc_module
{

    sc_in_clk clk;
    sc_in<sc_bv<1>> PRE;
    sc_in<sc_bv<1>> CE;
    sc_in<sc_bv<1>> D;
    sc_in<sc_bv<1>> C;
    sc_out<sc_bv<1>> Q;

    void handle() {
        Q.write(D.read());
    }

    SC_CTOR(FDPE)
    {
        SC_METHOD(handle);
        sensitive << clk.pos();
    }

};

#endif


