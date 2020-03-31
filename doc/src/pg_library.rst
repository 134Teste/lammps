LAMMPS C library API
********************

Overview
========

As described on the :doc:`library interface to LAMMPS <Howto_library>`
doc page, LAMMPS can be built as a library, so that it can be called by
another code, used in a :doc:`coupled manner <Howto_couple>` with other
codes, or driven through a :doc:`Python interface <Python_head>`.
Several of these approaches are based on a C language wrapper functions
in the files ``src/library.h`` and ``src/library.cpp``, which are
documented below.  Behind the scenes this will create, delete, or
modify and instance of the :ref:`LAMMPS class <lammps_ns_lammps>`.
Thus all functions require
an argument containing a ``handle`` in the form of a ``void`` pointer,
which contains or will be assigned to a reference of this class instance.

.. note::

   Please see the :ref:`note about thread-safety <thread-safety>`
   in the library Howto doc page.

Creating or deleting a LAMMPS object
====================================

The :ref:`lammps_open() <lammps_open>` and :ref:`lammps_open_no_mpi()
<lammps_open_no_mpi>` functions are used to create and initialize a
:ref:`LAMMPS instance <lammps_ns_lammps>`.  The calling program has to
provide a handle where a reference to this instance can be stored and
which has to be used in all subsequent function calls until that
instance is destroyed by calling :ref:`lammps_close() <lammps_close>`.
Here is a simple example demonstrating its use:

.. code-block:: C

   #include <library.h>
   #include <stdio.h>

   int main(int argc, char **argv)
   {
     void *handle;
     int version;
     const char *lmpargv[] { "lammps", "-log", "none"};
     int lmpargc = 3;

     /* create LAMMPS instance */
     lammps_open_no_mpi(lmpargc, lmpargv, &handle);
     if (handle == NULL) {
       printf("LAMMPS initialization failed");
       lammps_finalize();
       return 1;
     }

     /* get and print numerical version */
     version = lammps_version(handle);
     printf("LAMMPS Version: %d\n",version);

     /* delete LAMMPS instance and shut down MPI */
     lammps_close(handle);
     lammps_finalize();
     return 0;
   }

The LAMMPS library will be using the MPI library it was compiled with
and will either run on all processors in the ``MPI_COMM_WORLD``
communicator or on the set of processors in the communicator given in
the ``comm`` argument of :ref:`lammps_open() <lammps_open>`.  This means
the calling code can run LAMMPS on all or a subset of processors.  For
example, a wrapper script might decide to alternate between LAMMPS and
another code, allowing them both to run on all the processors.  Or it
might allocate half the processors to LAMMPS and half to the other code
by creating a custom communicator with ``MPI_Comm_split()`` and run both
codes concurrently before syncing them up periodically.  Or it might
instantiate multiple instances of LAMMPS to perform different
calculations and either alternate between them or run them one after the
other.  The :ref:`lammps_open() <lammps_open>` function may be called
multiple times for this purpose.

The :ref:`lammps_close() <lammps_close>` function is used to shut down
the :ref:`LAMMPS instance <lammps_ns_lammps>` pointed to by the handle
passed as an argument and free all its memory. This has to be called for
every instance created with any of the :ref:`lammps_open()
<lammps_open>` functions.  It will **not** call ``MPI_Finalize()``,
since that may only be called once. See :ref:`lammps_finalize()
<lammps_finalize>` for an alternative to calling ``MPI_Finalize()``
explicitly in the calling program.

-----------------------

.. _lammps_open:
.. doxygenfunction:: lammps_open
   :project: progguide
   :no-link:

-----------------------

.. _lammps_open_no_mpi:
.. doxygenfunction:: lammps_open_no_mpi
   :project: progguide
   :no-link:

-----------------------

.. _lammps_close:
.. doxygenfunction:: lammps_close
   :project: progguide
   :no-link:

-----------------------

.. _lammps_finalize:
.. doxygenfunction:: lammps_finalize
   :project: progguide
   :no-link:

-----------------------

.. _lammps_version:
.. doxygenfunction:: lammps_version
   :project: progguide
   :no-link:

-----------------------

Executing LAMMPS commands
=========================

Once a LAMMPS instance is created, there are multiple ways
to "drive" a simulation.  In most cases it is easiest to
process single or multiple LAMMPS commands like in an input
file.  This can be done through reading a file or passing
single commands or lists of commands with the following functions.

.. _lammps_file:
.. doxygenfunction:: lammps_file
   :project: progguide
   :no-link:

-------------------

TODO: this part still needs to be edited/adapted

.. note::

   You can write code for additional functions as needed to define
   how your code talks to LAMMPS and add them to src/library.cpp and
   src/library.h, as well as to the :doc:`Python interface <Python_head>`.
   The added functions can access or change any internal LAMMPS data you
   wish.

.. code-block:: c

   void lammps_file(void *, char *)
   char *lammps_command(void *, char *)
   void lammps_commands_list(void *, int, char **)
   void lammps_commands_string(void *, char *)
   void lammps_free(void *)

The lammps_file(), lammps_command(), lammps_commands_list(), and
lammps_commands_string() functions are used to pass one or more
commands to LAMMPS to execute, the same as if they were coming from an
input script.

Via these functions, the calling code can read or generate a series of
LAMMPS commands one or multiple at a time and pass it through the library
interface to setup a problem and then run it in stages.  The caller
can interleave the command function calls with operations it performs,
calls to extract information from or set information within LAMMPS, or
calls to another code's library.

The lammps_file() function passes the filename of an input script.
The lammps_command() function passes a single command as a string.
The lammps_commands_list() function passes multiple commands in a
char\*\* list.  In both lammps_command() and lammps_commands_list(),
individual commands may or may not have a trailing newline.  The
lammps_commands_string() function passes multiple commands
concatenated into one long string, separated by newline characters.
In both lammps_commands_list() and lammps_commands_string(), a single
command can be spread across multiple lines, if the last printable
character of all but the last line is "&", the same as if the lines
appeared in an input script.

The lammps_free() function is a clean-up function to free memory that
the library allocated previously via other function calls.  See
comments in src/library.cpp file for which other functions need this
clean-up.

The file src/library.cpp also contains these functions for extracting
information from LAMMPS and setting value within LAMMPS.  Again, see
the documentation in the src/library.cpp file for details, including
which quantities can be queried by name:

.. code-block:: c

   int lammps_extract_setting(void *, char *)
   void *lammps_extract_global(void *, char *)
   void lammps_extract_box(void *, double *, double *,
                           double *, double *, double *, int *, int *)
   void *lammps_extract_atom(void *, char *)
   void *lammps_extract_compute(void *, char *, int, int)
   void *lammps_extract_fix(void *, char *, int, int, int, int)
   void *lammps_extract_variable(void *, char *, char *)

The extract_setting() function returns info on the size
of data types (e.g. 32-bit or 64-bit atom IDs) used
by the LAMMPS executable (a compile-time choice).

The other extract functions return a pointer to various global or
per-atom quantities stored in LAMMPS or to values calculated by a
compute, fix, or variable.  The pointer returned by the
extract_global() function can be used as a permanent reference to a
value which may change.  For the extract_atom() method, see the
extract() method in the src/atom.cpp file for a list of valid per-atom
properties.  New names could easily be added if the property you want
is not listed.  For the other extract functions, the underlying
storage may be reallocated as LAMMPS runs, so you need to re-call the
function to assure a current pointer or returned value(s).

.. code-block:: c

   double lammps_get_thermo(void *, char *)
   int lammps_get_natoms(void *)

   int lammps_set_variable(void *, char *, char *)
   void lammps_reset_box(void *, double *, double *, double, double, double)

The lammps_get_thermo() function returns the current value of a thermo
keyword as a double precision value.

The lammps_get_natoms() function returns the total number of atoms in
the system and can be used by the caller to allocate memory for the
lammps_gather_atoms() and lammps_scatter_atoms() functions.

The lammps_set_variable() function can set an existing string-style
variable to a new string value, so that subsequent LAMMPS commands can
access the variable.

The lammps_reset_box() function resets the size and shape of the
simulation box, e.g. as part of restoring a previously extracted and
saved state of a simulation.

.. code-block:: c

   void lammps_gather_atoms(void *, char *, int, int, void *)
   void lammps_gather_atoms_concat(void *, char *, int, int, void *)
   void lammps_gather_atoms_subset(void *, char *, int, int, int, int *, void *)
   void lammps_scatter_atoms(void *, char *, int, int, void *)
   void lammps_scatter_atoms_subset(void *, char *, int, int, int, int *, void *)

The gather functions collect peratom info of the requested type (atom
coords, atom types, forces, etc) from all processors, and returns the
same vector of values to each calling processor.  The scatter
functions do the inverse.  They distribute a vector of peratom values,
passed by all calling processors, to individual atoms, which may be
owned by different processors.

.. warning::

   These functions are not compatible with the
   -DLAMMPS_BIGBIG setting when compiling LAMMPS.  Dummy functions
   that result in an error message and abort will be substituted
   instead of resulting in random crashes and memory corruption.

The lammps_gather_atoms() function does this for all N atoms in the
system, ordered by atom ID, from 1 to N.  The
lammps_gather_atoms_concat() function does it for all N atoms, but
simply concatenates the subset of atoms owned by each processor.  The
resulting vector is not ordered by atom ID.  Atom IDs can be requested
by the same function if the caller needs to know the ordering.  The
lammps_gather_subset() function allows the caller to request values
for only a subset of atoms (identified by ID).
For all 3 gather function, per-atom image flags can be retrieved in 2 ways.
If the count is specified as 1, they are returned
in a packed format with all three image flags stored in a single integer.
If the count is specified as 3, the values are unpacked into xyz flags
by the library before returning them.

The lammps_scatter_atoms() function takes a list of values for all N
atoms in the system, ordered by atom ID, from 1 to N, and assigns
those values to each atom in the system.  The
lammps_scatter_atoms_subset() function takes a subset of IDs as an
argument and only scatters those values to the owning atoms.

.. code-block:: c

   void lammps_create_atoms(void *, int, tagint *, int *, double *, double *,
                            imageint *, int)

The lammps_create_atoms() function takes a list of N atoms as input
with atom types and coords (required), an optionally atom IDs and
velocities and image flags.  It uses the coords of each atom to assign
it as a new atom to the processor that owns it.  This function is
useful to add atoms to a simulation or (in tandem with
lammps_reset_box()) to restore a previously extracted and saved state
of a simulation.  Additional properties for the new atoms can then be
assigned via the lammps_scatter_atoms() or lammps_extract_atom()
functions.

.. removed from Build_link.rst

**Calling the LAMMPS library**\ :

Either flavor of library (static or shared) allows one or more LAMMPS
objects to be instantiated from the calling program. When used from a
C++ program, most of the symbols and functions in LAMMPS are wrapped
in a LAMMPS_NS namespace; you can safely use any of its classes and
methods from within the calling code, as needed, and you will not incur
conflicts with functions and variables in your code that share the name.
This, however, does not extend to all additional libraries bundled with
LAMMPS in the lib folder and some of the low-level code of some packages.

To be compatible with C, Fortran, Python programs, the library has a simple
C-style interface, provided in src/library.cpp and src/library.h.

See the :doc:`Python library <Python_library>` doc page for a
description of the Python interface to LAMMPS, which wraps the C-style
interface from a shared library through the `ctypes python module <ctypes_>`_.

See the sample codes in examples/COUPLE/simple for examples of C++ and
C and Fortran codes that invoke LAMMPS through its library interface.
Other examples in the COUPLE directory use coupling ideas discussed on
the :doc:`Howto couple <Howto_couple>` doc page.

.. _ctypes: https://docs.python.org/3/library/ctypes.html

.. removed from Howto_couple.rst

Examples of driver codes that call LAMMPS as a library are included in
the examples/COUPLE directory of the LAMMPS distribution; see
examples/COUPLE/README for more details:

* simple: simple driver programs in C++ and C which invoke LAMMPS as a
  library
* plugin: simple driver program in C which invokes LAMMPS as a plugin
  from a shared library.
* lammps_quest: coupling of LAMMPS and `Quest <quest_>`_, to run classical
  MD with quantum forces calculated by a density functional code
* lammps_spparks: coupling of LAMMPS and `SPPARKS <spparks_>`_, to couple
  a kinetic Monte Carlo model for grain growth using MD to calculate
  strain induced across grain boundaries

.. _quest: http://dft.sandia.gov/Quest

.. _spparks: http://www.sandia.gov/~sjplimp/spparks.html

The :doc:`Build basics <Build_basics>` doc page describes how to build
LAMMPS as a library.  Once this is done, you can interface with LAMMPS
either via C++, C, Fortran, or Python (or any other language that
supports a vanilla C-like interface).  For example, from C++ you could
create one (or more) "instances" of LAMMPS, pass it an input script to
process, or execute individual commands, all by invoking the correct
class methods in LAMMPS.  From C or Fortran you can make function
calls to do the same things.  See the :doc:`Python <Python_head>` doc
pages for a description of the Python wrapper provided with LAMMPS
that operates through the LAMMPS library interface.

The files src/library.cpp and library.h contain the C-style interface
to LAMMPS.  See the :doc:`Howto library <Howto_library>` doc page for a
description of the interface and how to extend it for your needs.

Note that the lammps_open() function that creates an instance of
LAMMPS takes an MPI communicator as an argument.  This means that
instance of LAMMPS will run on the set of processors in the
communicator.  Thus the calling code can run LAMMPS on all or a subset
of processors.  For example, a wrapper script might decide to
alternate between LAMMPS and another code, allowing them both to run
on all the processors.  Or it might allocate half the processors to
LAMMPS and half to the other code and run both codes simultaneously
before syncing them up periodically.  Or it might instantiate multiple
instances of LAMMPS to perform different calculations.
   


                 
