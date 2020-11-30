Quality Testing
===============

This section will explain how do we take care of the quality of the code
in Deltamodels. We use a series of automated tests to verify coverage, 
general quality and correctness of all the major components.

Testing
-------------

The blocks are tested in a docker container to minimize interface
with the user environment.
`cmake` is used to run tests and extract coverage. 

Direct Testing
--------------

All the tests (and flags) are defined in the `CMakeList <https://github.com/riverlane/deltamodels/blob/dev/CMakeLists.txt>`_ file. 
If you modify/expand the library, you can check that the tests are passing:

.. code-block:: console

    $ make clean
    $ make tests

Code Coverage
-------------

Code coverage is checked via `gcov` using the command

.. code-block:: console

    $ make clean
    $ make tests
    $ make coverage


Memory Checks
-------------

We check the code agains potential memory leaks, bad allocations etc via `valgrind`.
To run the tests:

.. code-block:: console

    $ make clean
    $ make memcheck

What we will add
----------------

We have started working on randomized testing and formal verification for our components. Stay tuned!
