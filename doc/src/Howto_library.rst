Library interface to LAMMPS
===========================

As described on the :doc:`Build basics <Build_basics>` doc page, LAMMPS
can be built as a static or shared library, so that it can be called by
another code, used in a :doc:`coupled manner <Howto_couple>` with other
codes, or driven through a :doc:`Python interface <Python_head>`.

At the core of LAMMPS is the ``LAMMPS`` class which encapsulates the
state of the simulation program through the state of the various class
instances that it is composed of.  So a calculation using LAMMPS
requires to create an instance of the ``LAMMPS`` class and then send it
(text) commands, either individually or from a file, or perform other
operations that modify the state stored inside that instance or drive
simulations.  This is essentially what the ``src/main.cpp`` file does
as well for the standalone LAMMPS executable with reading commands
either from an input file or stdin.

Creating a LAMMPS instance can be done by using C++ code directly or
through a C-style interface library to LAMMPS that is provided in the
files ``src/library.cpp`` and ``src/library.h``.  This C language API
can be used from both, C and C++, and is also the basis for the Python
and Fortran interfaces or wrappers included in the LAMMPS source code.
Please find some simple commented examples below.

The ``examples/COUPLE`` and ``python/examples`` directories contain
these and additional example programs written in C++, C, Fortran,
and Python, which show how a driver code can link to LAMMPS as a
library, run LAMMPS on a subset of processors (so the others are
available to run some other code concurrently), grab data from
LAMMPS, change it, and send it back into LAMMPS.

A detailed documentation of the available APIs and examples of how to
use them can be found in the :doc:`Programmer Documentation
<pg_library>` section of this manual.

.. _thread-safety:

.. note:: Thread-safety

   LAMMPS was initially not conceived as a thread-safe program, but over
   the years changes have been applied to replace operations that
   collide with creating multiple LAMMPS instances from multiple-threads
   of the same process with thread-safe alternatives.  This primarily
   applies to the core LAMMPS code and less so on add-on packages,
   especially when those packages require additional code in the *lib*
   folder, interface LAMMPS to Fortran libraries, or the code uses
   static variables (like the USER-COLVARS package).

   Another major issue to deal with is to correctly handle MPI.
   Creating a LAMMPS instance requires passing an MPI communicator, or
   it assumes the ``MPI_COMM_WORLD`` communicator, which spans all MPI
   processor ranks.  When creating multiple LAMMPS object instances from
   different threads, this communicator has to be different for each
   thread or else collisions can happen.  or it has to be guaranteed,
   that only one thread at a time is active.  MPI communicators,
   however, are not a problem, if LAMMPS is compiled with the MPI STUBS
   library, which implies that there is no MPI communication and only 1
   MPI rank.
