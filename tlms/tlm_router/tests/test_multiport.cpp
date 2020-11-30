#include <iostream>
#include <systemc>
#include <tlm.h>
#include "tlms/tlm_router/tlm_router.hpp"
#include "tlms/commons/memory.h"
#include "tlms/commons/initiator.h"
#include "commons/assertions.hpp"

using namespace std;

int sc_main(int argc, char** argv) {

  sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_tlm_advanced_routing");
  Tf->set_time_unit(100,SC_PS);

  Initiator init1 = Initiator("init1");
  Initiator init2 = Initiator("init2");

  Memory<0x100> mem1 = Memory<0x100>("memory1");
  Memory<0x100> mem2 = Memory<0x100>("memory2");
  Memory<0x100> mem3 = Memory<0x100>("memory3");

  TLMRouter<2, 3> router = TLMRouter<2, 3>("router");
 
  init1.socket.bind(*(router.target_socket[0]));
  init2.socket.bind(*(router.target_socket[1]));

  const InitiatorConfig mem1Config = {0x0, 0x100, 0xFFF};
  router.setInitiatorProperties(0, mem1Config);
  router.initiator_socket[0]->bind(mem1.socket);
  
  const InitiatorConfig mem2Config = {0x1000, 0x1100, 0xEFFF};
  router.setInitiatorProperties(1, mem2Config);
  router.initiator_socket[1]->bind(mem2.socket);

  const InitiatorConfig mem3Config = {0x10000, 0x10100, 0xEFFFF};
  router.setInitiatorProperties(2, mem3Config);
  router.initiator_socket[2]->bind(mem3.socket);
   
  // Binding trace files   
  sc_trace(Tf, mem1.wr_ops, "mem1.wr_ops");
  sc_trace(Tf, mem1.rd_ops, "mem1.rd_ops");
  sc_trace(Tf, mem2.wr_ops, "mem2.wr_ops");
  sc_trace(Tf, mem2.rd_ops, "mem2.rd_ops");
  sc_trace(Tf, mem3.wr_ops, "mem3.wr_ops");
  sc_trace(Tf, mem3.rd_ops, "mem3.rd_ops");

  const int NACCESSES = 10;

  uint32_t data1, data2, data3;

   for (int i=0; i<NACCESSES; i++) {
       data1 = i;
       data2 = i+0x1000;
       data3 = i+0x10000;
        _initiator_dowrite(init1, &data1, i*4);
        _initiator_dowrite(init2, &data2, i*4+0x1000);
        _initiator_dowrite(init2, &data3, i*4+0x10000);
    }

    for (int i=0; i<NACCESSES; i++) {
        _initiator_doread(init1, &data1, i*4+0x10000);
        _initiator_doread(init2, &data2, i*4+0x1000);
        _initiator_doread(init1, &data3, i*4);
        checkValuesMatch<int32_t>(i+0x10000, data1, "readback1");
        checkValuesMatch<int32_t>(i+0x1000, data2, "readback2");
        checkValuesMatch<int32_t>(i, data3, "readback3");
    }


  sc_close_vcd_trace_file(Tf);
  
  // Verification phase
  checkValuesMatch<long int>(NACCESSES, mem1.wr_ops, "mem1_wr");
  checkValuesMatch<long int>(NACCESSES, mem1.rd_ops, "mem1_rd");
  checkValuesMatch<long int>(NACCESSES, mem2.wr_ops, "mem2_wr");
  checkValuesMatch<long int>(NACCESSES, mem2.rd_ops, "mem2_rd");
  checkValuesMatch<long int>(NACCESSES, mem3.wr_ops, "mem3_wr");
  checkValuesMatch<long int>(NACCESSES, mem3.rd_ops, "mem3_rd");
      
  return 0;
}
