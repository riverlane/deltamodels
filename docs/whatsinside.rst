What's inside
============

Contains general SystemC models (no-TLM) with (Semi) Accurate Timing
representation.
Currently we have models for:

basic_blocks
------------

General FPGA building blocks: 

- a D Flip-Flop model in `SystemC version <models/basic_blocks/FDPE.hpp>`
  and `Verilog <models/basic_blocks/FDPE.v>`

- a `Tristate <models/basic_blocks/tristate.hpp` component,
  mainly needed for input/output pins

clocking
--------

Blocks for clock generation, buffering etc:

- [clockgen](models/basic_blocks/clockgen.hpp), a SystemC clock
  generator (divider/multiplier)

memories
--------

Memory blocks (hardware models):

- `flash-N25QX <models/memories/flash/N25QX.hpp>`_.
  A quad-spi flash model.
  Memory is represented as a map to allow data sparsity.
  Content can be configured via `configure_region`.

- `generic_sdram <models/memories/sdram/generic_sdram.hpp>`.
  A simple sdram model with additional backdoor interface.
  The backdoor allows for copy and movement of data intra-memory.
  Content can be preloaded via `configure_region`.

network
-------

Various blocks related to GMII interfacing:

- `mock_gmii <models/network/mock_gmii.hpp>`.
  Provides a fake GMII interface for unit-testing

- `ethbridge <models/network/ethbridge.hpp>`.
  A bridge for the MAC layer of the ethernet protocol

- `network_helpers <models/network/network_helpers.hpp>`. Helpers and utilities

wishbone
--------

Models of wishbone components:

- `network_helpers <models/wishbone/wbram.hpp>`.
  The model of an internal FPGA memory block (BRAM).

- `ork1_instruction_tracer <models/wishbone/or1k/or1k_instruction_tracer.hpp>`_.
  A SystemC component that converts raw instruction bus accesses (wishbone)
  back into or1k instructions.

- `ork1_data_tracer <models/wishbone/or1k/or1k_data_tracer.hpp>`.
  A SystemC component that traces raw (wishbone) data transactions.

tlms
====

Transaction Level Models (TLMs) are in general time-agnostic.
Some of them support backpressure to bridge from Timing Accurate to
Timing Agnostic approach.

tlm_adapters
------------

Logic that bridges tlm to other buses and vice-versa:

- `tlm2wishbone.hpp <tlms/tlm_adapters/tlm2wishbone.hpp>`.
Block that converts TLM requests into Wishbone transactions

tlm_common
----------

Contains common reusable blocks, like initiators and glue logic

tlm_memories
------------

Contains memories that support the tlm protocol:

- `tlm_rom <tlms/tlm_memories/tlm_rom.hpp>`: implements a read-only memory

tlm_router
----------

Contains routing logic for tlm messages:

- `tlm_router <tlms/tlm_router/tlm_router.hpp>`: a router with configurable
  number of initiators and targets.
  Address mapping can be configured during initialization.

