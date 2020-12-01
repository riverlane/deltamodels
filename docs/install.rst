Installation
============

This section will explain how to install Deltamodels.
Deltamodels runs on Docker container to provide you with all the right
dependencies at tools.

Prerequisites
-------------

Currently, we only support and test Deltamodels on Ubuntu 20.04.
Please follow `Docker <https://www.docker.com>`_ instructions to set it up
for your system. 

We are in the process of extending both the supported operating systems.
If you have specific requirements, you are welcome request support.
Get in contact by `emailing us <mailto:deltaflow@riverlane.com>`_

Install with Docker
-------------------

Deltamodels can be used and explored via, *make*.
The docker image will be generated only the first time and then stored
for quick-access.

.. code-block:: console

    make shell

This will create the image and open a shell on the remote docker. 

To test your installation and run the tests

.. code-block:: console

    make tests

Tests are based on CMake to increase readibility and maintainability.
