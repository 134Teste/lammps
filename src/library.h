/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifndef LAMMPS_LIBRARY_H
#define LAMMPS_LIBRARY_H

/*! \file library.h
 *
 * C style library interface to LAMMPS which allows to create
 * and control instances of the LAMMPS C++ class and exchange
 * data with it.  The C bindings are then used as basis for
 * the ctypes based Python wrapper in lammps.py and also there
 * are Fortran interfaces that expose the functionality by
 * using the ISO_C_BINDINGS module in modern Fortran.
 * If needed, new LAMMPS-specific functions can be added to
 * expose additional LAMMPS functionality to this library interface.
*/

/*
 * Follow the behavior of regular LAMMPS compilation and assume
 * -DLAMMPS_SMALLBIG when no define is set.
 */
#if !defined(LAMMPS_BIGBIG) && !defined(LAMMPS_SMALLBIG) && !defined(LAMMPS_SMALLSMALL)
#define LAMMPS_SMALLBIG
#endif

#include <mpi.h>
#if defined(LAMMPS_BIGBIG) || defined(LAMMPS_SMALLBIG)
#include <inttypes.h>  /* for int64_t */
#endif

/* ifdefs allow this file to be included in a C program */

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Create an instance of the LAMMPS class and return a pointer
 *  to it. Pass a list of command line arguments and the MPI communicator.
 *
 * \param argc number of command line arguments
 * \param argv list of command line argument strings
 * \param comm MPI communicator for this LAMMPS instance.
 * \param ptr pointer to a location where reference to the created LAMMPS instance is stored. Will be pointing to a NULL
 * pointer if the function failed.
 */
  void lammps_open(int argc, char **argv, MPI_Comm comm, void **ptr);

/** \brief Variant of lammps_open() that will implicitly use MPI_COMM_WORLD.
 *  Will run MPI_Init() if it has not been called before.
 *
 * \param argc number of command line arguments
 * \param argv list of command line argument strings
 * \param ptr pointer to a location where reference to the created LAMMPS instance is stored. Will be pointing to a NULL
 * pointer if the function failed.
 */
  void lammps_open_no_mpi(int argc, char **argv, void **ptr);

/** \brief Delete a LAMMPS instance created by lammps_open() or lammps_open_no_mpi()
 *
 * \param ptr pointer to a previously created LAMMPS instance cast to void *.
 */
  void lammps_close(void *ptr);

  /** \brief Get the numerical representation of the current LAMMPS version.
   *
   * \param ptr pointer to a previously created LAMMPS instance cast to void *.
   * \return an integer representing the version data in the format YYYYMMDD
   */
  int  lammps_version(void *ptr);

  /** \brief Process LAMMPS input from a file
   *
   * \param ptr pointer to a previously created LAMMPS instance cast to void *.
   * \param filename name of a file with LAMMPS input

\verbatim embed:rst

This function will process the commands in the file pointed to
by ``filename`` line by line like a file processed with the
:doc:`include <include>` command.  The function returns when
the end of the file is reached or a :doc:`quit <quit>`
command is encountered.

\endverbatim
  */
  void lammps_file(void * ptr, char * filename);

  char *lammps_command(void *, char *);
  void lammps_commands_list(void *, int, char **);
  void lammps_commands_string(void *, char *);
  void lammps_free(void *);

  int lammps_extract_setting(void *, char *);
  void *lammps_extract_global(void *, char *);
  void lammps_extract_box(void *, double *, double *,
                          double *, double *, double *, int *, int *);
  void *lammps_extract_atom(void *, char *);
  void *lammps_extract_compute(void *, char *, int, int);
  void *lammps_extract_fix(void *, char *, int, int, int, int);
  void *lammps_extract_variable(void *, char *, char *);

  double lammps_get_thermo(void *, char *);
  int lammps_get_natoms(void *);

  int lammps_set_variable(void *, char *, char *);
  void lammps_reset_box(void *, double *, double *, double, double, double);

  void lammps_gather_atoms(void *, char *, int, int, void *);
  void lammps_gather_atoms_concat(void *, char *, int, int, void *);
  void lammps_gather_atoms_subset(void *, char *, int, int, int, int *, void *);
  void lammps_scatter_atoms(void *, char *, int, int, void *);
  void lammps_scatter_atoms_subset(void *, char *, int, int, int, int *, void *);

#if defined(LAMMPS_BIGBIG)
  typedef void (*FixExternalFnPtr)(void *, int64_t, int, int64_t *, double **, double **);
  void lammps_set_fix_external_callback(void *, char *, FixExternalFnPtr, void*);
#elif defined(LAMMPS_SMALLBIG)
  typedef void (*FixExternalFnPtr)(void *, int64_t, int, int *, double **, double **);
  void lammps_set_fix_external_callback(void *, char *, FixExternalFnPtr, void*);
#else
  typedef void (*FixExternalFnPtr)(void *, int, int, int *, double **, double **);
  void lammps_set_fix_external_callback(void *, char *, FixExternalFnPtr, void*);
#endif

  int lammps_config_has_package(char * package_name);
  int lammps_config_package_count();
  int lammps_config_package_name(int index, char * buffer, int max_size);
  int lammps_config_has_gzip_support();
  int lammps_config_has_png_support();
  int lammps_config_has_jpeg_support();
  int lammps_config_has_ffmpeg_support();
  int lammps_config_has_exceptions();

  int lammps_find_pair_neighlist(void* ptr, char * style, int exact, int nsub, int request);
  int lammps_find_fix_neighlist(void* ptr, char * id, int request);
  int lammps_find_compute_neighlist(void* ptr, char * id, int request);
  int lammps_neighlist_num_elements(void* ptr, int idx);
  void lammps_neighlist_element_neighbors(void * ptr, int idx, int element, int * iatom, int * numneigh, int ** neighbors);

// lammps_create_atoms() takes tagint and imageint as args
// ifdef insures they are compatible with rest of LAMMPS
// caller must match to how LAMMPS library is built

#ifdef LAMMPS_BIGBIG
  void lammps_create_atoms(void *, int, int64_t *, int *,
                           double *, double *, int64_t *, int);
#else
  void lammps_create_atoms(void *, int, int *, int *,
                           double *, double *, int *, int);
#endif

#ifdef LAMMPS_EXCEPTIONS
  int lammps_has_error(void *);
  int lammps_get_last_error_message(void *, char *, int);
#endif

#undef LAMMPS
#ifdef __cplusplus
}
#endif

#endif /* LAMMPS_LIBRARY_H */
/* ERROR/WARNING messages:

E: Library error: issuing LAMMPS command during run

UNDOCUMENTED

W: Library error in lammps_gather_atoms

This library function cannot be used if atom IDs are not defined
or are not consecutively numbered.

W: lammps_gather_atoms: unknown property name

UNDOCUMENTED

W: Library error in lammps_gather_atoms_subset

UNDOCUMENTED

W: lammps_gather_atoms_subset: unknown property name

UNDOCUMENTED

W: Library error in lammps_scatter_atoms

This library function cannot be used if atom IDs are not defined or
are not consecutively numbered, or if no atom map is defined.  See the
atom_modify command for details about atom maps.

W: lammps_scatter_atoms: unknown property name

UNDOCUMENTED

W: Library error in lammps_scatter_atoms_subset

UNDOCUMENTED

W: lammps_scatter_atoms_subset: unknown property name

UNDOCUMENTED

W: Library error in lammps_create_atoms

UNDOCUMENTED

W: Library warning in lammps_create_atoms, invalid total atoms %ld %ld

UNDOCUMENTED

*/
