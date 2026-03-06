#[=======================================================================[.rst:
FindZFP
-------

Find the ZFP compression library (for installations without CMake config files,
e.g. when ZFP was built with its vanilla Makefile).

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported target, if found:

``zfp::zfp``
  The ZFP library

Result Variables
^^^^^^^^^^^^^^^^

``ZFP_FOUND``
  True if ZFP was found
``ZFP_INCLUDE_DIRS``
  Include directories for ZFP headers
``ZFP_LIBRARIES``
  Libraries to link against

Hints
^^^^^

Set ``ZFP_DIR`` or ``ZFP_ROOT`` to the ZFP installation prefix.

#]=======================================================================]

# Look for the header
find_path(ZFP_INCLUDE_DIR
  NAMES zfp.h
  PATHS
    ${ZFP_DIR} ${ZFP_ROOT}
    ENV ZFP_DIR ENV ZFP_ROOT
  PATH_SUFFIXES include
)

# Look for the library
find_library(ZFP_LIBRARY
  NAMES zfp
  PATHS
    ${ZFP_DIR} ${ZFP_ROOT}
    ENV ZFP_DIR ENV ZFP_ROOT
  PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZFP
  REQUIRED_VARS ZFP_LIBRARY ZFP_INCLUDE_DIR
)

if (ZFP_FOUND)
  set(ZFP_LIBRARIES ${ZFP_LIBRARY})
  set(ZFP_INCLUDE_DIRS ${ZFP_INCLUDE_DIR})

  if (NOT TARGET zfp::zfp)
    add_library(zfp::zfp UNKNOWN IMPORTED)
    set_target_properties(zfp::zfp PROPERTIES
      IMPORTED_LOCATION "${ZFP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ZFP_INCLUDE_DIR}"
    )
  endif ()
endif ()

mark_as_advanced(ZFP_INCLUDE_DIR ZFP_LIBRARY)
