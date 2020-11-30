Introduction
============

System-On-Chip (SOC) comes in different flavours but in general they rely
on similar building blocks.
To accelerate the design of classical and quantum emulators we have put
together this library of components with the idea of expanding it even more
in the future.
We have selected as SystemC as our language of choice because we think that:

* It provides configurable levels of adherence to the hardware.

* It is generally used in the modeling community for both digital and analogue
  designs.

* It is based on C++ so the learning rate is faster than for other hardware
  oriented modeling languages.
  
Types of models
---------------

We have implemented our models into two fashions:   

* **Transaction Level Modeling (TLM)**: high level models that use transactions 
  instead of signals.
  This accelerates execution by simplifying the timing model.

* **Generic SystemC models**: they provide the most accurate timing
  representation of internal signals.  

Please refer to the `SystemC documentation
<https://www.accellera.org/downloads/standards/systemc>`_.
