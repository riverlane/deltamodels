/**
 * @file ClockGen.hpp
 * @author Riverlane, 2020
 *
 */
#ifndef __CLOCKGEN_H__
#define __CLOCKGEN_H__

#include <systemc.h>

using namespace std;
using namespace sc_dt;

/**
 * Implementation of a clock generator (divider/multiplier).
 * The component extracts the input clock frequency and 
 * via wait statements implements the required output frequency.
 */
template <int DIV, int MULT>
struct ClockGen: sc_module
{
    sc_in_clk clk_i;
    sc_out<bool> clk_o;
    sc_out<bool> locked;

    int pos_duration = 0;
    bool unitialized = true;

    void clk_gen() {
        while(locked.read()) {
            clk_o.write(1);
            wait(pos_duration/MULT*DIV,SC_PS);
            clk_o.write(0);
            wait(pos_duration/MULT*DIV,SC_PS);
        }
    }

    void measure_edges() {
        locked.write(0);
        while(unitialized) {
            // Waiting for the first pos-edge
            if (clk_i.read()) {
                while(clk_i.read() == 1) {
                    pos_duration++;
                    wait(1, SC_PS);
                }
                unitialized = false;
            }
            // Need to progress simulation time in case of negedge
            wait(1, SC_PS);
        }
        locked.write(1);
        cout << "ClockGen: measured baseclock period: " << 2*pos_duration << "PS" << endl;
    }

    SC_CTOR(ClockGen)
    {
        SC_THREAD(measure_edges);
        sensitive << clk_i.pos();
        SC_THREAD(clk_gen);
        sensitive << locked;
        dont_initialize();
    }
};

#endif //__CLOCKGEN_H__


