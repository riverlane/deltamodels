#ifndef MEMORY_H
#define MEMORY_H

// *****************************************************************************************
// Target memory implements b_transport, DMI and debug
// *****************************************************************************************

template <unsigned int SIZE>
struct Memory: sc_module
{
  tlm_utils::simple_target_socket<Memory> socket;

  const sc_time LATENCY;
  sc_signal<long int> wr_ops;
  sc_signal<long int> rd_ops;
 
  SC_CTOR(Memory)
  : socket("socket"), LATENCY(10, SC_NS)
  {
    wr_ops.write(0);
    rd_ops.write(0);
    socket.register_b_transport(       this, &Memory::b_transport);
    socket.register_get_direct_mem_ptr(this, &Memory::get_direct_mem_ptr);
    socket.register_transport_dbg(     this, &Memory::transport_dbg);
  }

  virtual void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address() / 4;
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    if (adr > sc_dt::uint64(SIZE)) {
      trans.set_response_status( tlm::TLM_ADDRESS_ERROR_RESPONSE );
      return;
    }
    if (byt != 0) {
      trans.set_response_status( tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE );
      return;
    }
    if (len > 4 || wid < len) {
      trans.set_response_status( tlm::TLM_BURST_ERROR_RESPONSE );
      return;
    }

    if (trans.get_command() == tlm::TLM_READ_COMMAND){
      memcpy(ptr, &mem[adr], len);
      rd_ops.write(rd_ops+1);
    }
    else if (cmd == tlm::TLM_WRITE_COMMAND){
      memcpy(&mem[adr], ptr, len);
      wr_ops.write(wr_ops+1);
    }

    // Use temporal decoupling: add memory latency to delay argument
    delay += LATENCY;

    trans.set_dmi_allowed(true);
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
  }


  // TLM-2 DMI method
  virtual bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans,
                                  tlm::tlm_dmi& dmi_data)
  {
    // Permit read and write access
    dmi_data.allow_read_write();

    // Set other details of DMI region
    dmi_data.set_dmi_ptr( reinterpret_cast<unsigned char*>( &mem[0] ) );
    dmi_data.set_start_address( 0 );
    dmi_data.set_end_address( SIZE*4-1 );
    dmi_data.set_read_latency( LATENCY );
    dmi_data.set_write_latency( LATENCY );

    return true;
  }


  // TLM-2 debug transaction method
  virtual unsigned int transport_dbg(tlm::tlm_generic_payload& trans)
  {
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address() / 4;
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length();

    // Calculate the number of bytes to be actually copied
    unsigned int num_bytes = (len < (SIZE - adr) * 4) ? len : (SIZE - adr) * 4;

    if ( cmd == tlm::TLM_READ_COMMAND )
      memcpy(ptr, &mem[adr], num_bytes);
    else if ( cmd == tlm::TLM_WRITE_COMMAND )
      memcpy(&mem[adr], ptr, num_bytes);

    return num_bytes;
  }

  int mem[SIZE];
};

#endif


