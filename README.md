# Deltamodels

[![Build Status](http://jenkins-riverlane.northeurope.cloudapp.azure.com/buildStatus/icon?job=deltamodels_multibranch_build%2Fdev)](http://jenkins-riverlane.northeurope.cloudapp.azure.com/job/deltamodels_multibranch_build/job/dev/)

A library of building blocks for your simulation/emulation environemnt.
The `deltamodels` components let you build a simulator with the right
combination of speed and internal visibility. 

## Quick Links

* [Documentation](https://riverlane.github.io/deltamodels)

* [GitHub](https://github.com/riverlane/deltamodels)

* [Riverlane Website](https://www.riverlane.com/)

## Prerequisite

`deltamodels` leverages Docker to be cross-platform.
Follow [docker](https://www.docker.com) instructions to set it up for your system. 

## Installing

`deltamodels` will create a local Docker image the first time you will
run a command.
Recommended command:

```console
river@lane:~$ make shell
```
## Using deltamodels

You can a peek-around the various blocks
(Section `What's inside` for more details) and run the test-suite via:

```console
river@lane:~$ make tests
```

## What's inside

### logging

Logging components.
Mainly used for extracting traces and live reports.
We strongly rely on [spdlog](https://github.com/gabime/spdlog).
Please refer to the [logging/spdlog/LICENSE](logging/spdlog/LICENSE) for
more information on spdlog licensing. 

### models

Contains general SystemC models (no-TLM) with (Semi) Accurate Timing
representation.
Currently we have models for:

#### basic_blocks

General FPGA building blocks: 

- a D Flip-Flop model in [SystemC](models/basic_blocks/FDPE.hpp)
    and [Verilog](models/basic_blocks/FDPE.v)

- a [Tristate](models/basic_blocks/tristate.hpp) component,
    mainly needed for input/output pins

#### clocking

Blocks for clock generation, buffering etc:

- [clockgen](models/basic_blocks/clockgen.hpp), a SystemC clock
    generator (divider/multiplier)

#### memories

Memory blocks (hardware models):

- [flash-N25QX](models/memories/flash/N25QX.hpp).
    A quad-spi flash model.
    Memory is represented as a map to allow data sparsity.
    Content can be configured via `configure_region`.

- [generic_sdram](models/memories/sdram/generic_sdram.hpp).
    A simple sdram model with additional backdoor interface.
    The backdoor allows for copy and movement of data intra-memory.
    Content can be preloaded via `configure_region`.

#### network

Various blocks related to GMII interfacing:

- [mock_gmii](models/network/mock_gmii.hpp).
    Provides a fake GMII interface for unit-testing

- [ethbridge](models/network/ethbridge.hpp).
    A bridge for the MAC layer of the ethernet protocol

- [network_helpers](models/network/network_helpers.hpp). Helpers and utilities

#### wishbone

Models of wishbone components:

- [network_helpers](models/wishbone/wbram.hpp).
    The model of an internal FPGA memory block (BRAM).

- [ork1_instruction_tracer](models/wishbone/or1k/or1k_instruction_tracer.hpp).
    A SystemC component that converts raw instruction bus accesses (wishbone)
    back into or1k instructions.

- [ork1_data_tracer](models/wishbone/or1k/or1k_data_tracer.hpp).
    A SystemC component that traces raw (wishbone) data transactions.

### tlms

Transaction Level Models (TLMs) are in general time-agnostic.
Some of them support backpressure to bridge from Timing Accurate to
Timing Agnostic approach.

#### tlm_adapters

Logic that bridges tlm to other buses and vice-versa:

- [tlm2wishbone.hpp](tlms/tlm_adapters/tlm2wishbone.hpp).
Block that converts TLM requests into Wishbone transactions

#### tlm_common

Contains common reusable blocks, like initiators and glue logic

[TLM_COMMONS](tlms/commons/README.md)

#### tlm_memories

Contains memories that support the tlm protocol:

- [tlm_rom](tlms/tlm_memories/tlm_rom.hpp): implements a read-only memory

#### tlm_router

Contains routing logic for tlm messages:

- [tlm_router](tlms/tlm_router/tlm_router.hpp): a router with configurable
    number of initiators and targets.
    Address mapping can be configured during initialization.

### integration 

Contains examples of mixed environment (verilated+systemc) for testing
and analysis.

## Contributing to deltamodels

To contribute to artiq emulator, follow these steps (and please respect our
[Code of Conduct](CODE_OF_CONDUCT.md):

- Fork this repository.

- Create a branch: git checkout -b <branch_name>.

- Have a look at the `Code-quality policies` and `How to add tests` sections for information on how we develop

- Make your changes and commit them: git commit -m '<commit_message>'

- Push to the original branch: git push origin <project_name>/

- Create the pull request. If in doubt, please do write to team@riverlane.com

### How to add tests 

In the [CMake definitions](CMakeLists.txt) add an executable via
`add_executable(name files...)` and a optionally link
libraries `target_link_libraries(name lib1 lib2)`. 

Note: you might have to add `include_directories(...)` for
the `#include ...` to work.

To add a ctest simply `add_test(name executable)`.
The file just need to implement a main call with a return value of
0/not zero and or exceptions.

### Code-quality policies

The blocks are tested in a docker container to minimize interface
with the user environment.
`cmake` is used to run tests and extract coverage. 
All the tests (and flags) are defined in the [CMakeLists](CMakeLists.txt) file. 
If you modify/expand the library, you can check that the tests are passing:

```console
river@lane:~$ make clean
river@lane:~$ make tests
Create new tag: 20200927-1331 - Experimental
Test project /workdir/build
    Start 1: test_wishbone_adapter
1/8 Test #1: test_wishbone_adapter ............   Passed    0.02 sec
    Start 2: test_router_basic
2/8 Test #2: test_router_basic ................   Passed    0.06 sec
    Start 3: test_router_advanced
...

100% tests passed, 0 tests failed out of 8
```

That you have sufficient coverage on your block:

```console
river@lane:~$ make clean
river@lane:~$ make coverage
docker exec -w /workdir/build 0b46c45246f2d70ff2f7e4ffbe76d7cc24a199faffcede42c23d7156d6c8581d gcovr -r /workdir  
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: /workdir
------------------------------------------------------------------------------
File                                       Lines    Exec  Cover   Missing
------------------------------------------------------------------------------
integration/oxionics_demo/version_0.0.1/test_dut.cpp
                                              81      81   100%   
integration/oxionics_demo/version_0.0.2/test_dut.cpp
                                              82      82   100%   
models/clocking/clockgen.hpp                  26      26   100%   
models/clocking/tests/test_clockgen.cpp       32      32   100%   
models/memories/flash/N25QX.hpp               75      75   100%   
...

------------------------------------------------------------------------------
TOTAL                                       1636    1297    79%
------------------------------------------------------------------------------
```

That you have not introduced memory leaks:

```console
river@lane:~$ make clean
river@lane:~$ make memcheck
docker exec -w /workdir/build 0b46c45246f2d70ff2f7e4ffbe76d7cc24a199faffcede42c23d7156d6c8581d sudo ctest -D ExperimentalMemCheck
   Site: ad94d8080d0f
   Build name: Linux-c++
Create new tag: 20201001-1104 - Experimental
Memory check project /workdir/build
    Start 1: test_wishbone_adapter
1/9 MemCheck #1: test_wishbone_adapter ............   Passed    1.53 sec
    Start 2: test_router_basic
2/9 MemCheck #2: test_router_basic ................   Passed    1.83 sec
    Start 3: test_router_advanced
3/9 MemCheck #3: test_router_advanced .............   Passed    1.85 sec
....
100% tests passed, 9 tests failed out of 9

Total Test time (real) =  17.13 sec
```

## Licensing

Please refer to our [License](LICENSE.rst) for more information
