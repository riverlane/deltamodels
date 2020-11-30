#include <iostream>
#include <systemc.h>
#include "tlm.h"
#include "clockgen.hpp"
#include "commons/assertions.hpp"

using namespace sc_dt;
using namespace std;

struct FreqMonitor: sc_module
{
   sc_in<bool> clk;
   uint32_t ticks {0};

   void cnt_ticks(){
      if (clk.read())
         ticks++;
   }

   SC_CTOR(FreqMonitor)
    {
        SC_METHOD(cnt_ticks);
        sensitive << clk;
        dont_initialize();
    }
};


int sc_main(int argc, char** argv) {

   sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_clockgen");
   Tf->set_time_unit(1,SC_PS);
   sc_clock clk("clk", sc_time(1, SC_NS)); sc_trace(Tf, clk, "clk");
   sc_signal<bool> clkdiv;
   sc_signal<bool> locked;
   sc_trace(Tf, clkdiv, "clkdiv");
   sc_trace(Tf, locked, "locked");

   ClockGen<2,5> clockgen = ClockGen<2,5>("clockgen");
   clockgen.clk_i(clk);
   clockgen.clk_o(clkdiv);
   clockgen.locked(locked);
   
   FreqMonitor mon = FreqMonitor("freq_monitor");
   mon.clk(clkdiv);

   sc_start(100, SC_NS);
   // Checking that the clockgen locks
   checkValuesMatch<bool>(clockgen.locked, true, "locked");

   // Checking that the output frequency matches the requested one.
   uint32_t ticks = mon.ticks;
   sc_start(100, SC_NS);
   uint32_t delta = mon.ticks - ticks;    
   checkValuesMatch<uint32_t>(delta, 100 * 5 / 2, "num_of_ticks");

   sc_close_vcd_trace_file(Tf);
   return 0;
}
