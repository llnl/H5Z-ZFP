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
``ZFP_VERSION``
  Version string (e.g. "1.0.0")
``ZFP_VERSION_MAJOR``, ``ZFP_VERSION_MINOR``, ``ZFP_VERSION_PATCH``, ``ZFP_VERSION_TWEAK``
  Individual version components

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

# Extract version from headers
if (ZFP_INCLUDE_DIR)
  # ZFP 1.0.0+ has version in zfp/version.h; older versions in zfp.h
  if (EXISTS "${ZFP_INCLUDE_DIR}/zfp/version.h")
    set(_zfp_version_header "${ZFP_INCLUDE_DIR}/zfp/version.h")
  elseif (EXISTS "${ZFP_INCLUDE_DIR}/zfp.h")
    set(_zfp_version_header "${ZFP_INCLUDE_DIR}/zfp.h")
  endif ()

  if (_zfp_version_header)
    file(STRINGS "${_zfp_version_header}" _zfp_major
      REGEX "^#define[ \t]+ZFP_VERSION_MAJOR[ \t]+[0-9]+")
    file(STRINGS "${_zfp_version_header}" _zfp_minor
      REGEX "^#define[ \t]+ZFP_VERSION_MINOR[ \t]+[0-9]+")
    file(STRINGS "${_zfp_version_header}" _zfp_patch
      REGEX "^#define[ \t]+ZFP_VERSION_PATCH[ \t]+[0-9]+")
    file(STRINGS "${_zfp_version_header}" _zfp_tweak
      REGEX "^#define[ \t]+ZFP_VERSION_TWEAK[ \t]+[0-9]+")

    if (_zfp_major)
      string(REGEX REPLACE ".*ZFP_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" ZFP_VERSION_MAJOR "${_zfp_major}")
    endif ()
    if (_zfp_minor)
      string(REGEX REPLACE ".*ZFP_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" ZFP_VERSION_MINOR "${_zfp_minor}")
    endif ()
    if (_zfp_patch)
      string(REGEX REPLACE ".*ZFP_VERSION_PATCH[ \t]+([0-9]+).*" "\\1" ZFP_VERSION_PATCH "${_zfp_patch}")
    endif ()
    if (_zfp_tweak)
      string(REGEX REPLACE ".*ZFP_VERSION_TWEAK[ \t]+([0-9]+).*" "\\1" ZFP_VERSION_TWEAK "${_zfp_tweak}")
    else ()
      set(ZFP_VERSION_TWEAK 0)
    endif ()

    if (ZFP_VERSION_MAJOR AND ZFP_VERSION_MINOR AND ZFP_VERSION_PATCH)
      set(ZFP_VERSION "${ZFP_VERSION_MAJOR}.${ZFP_VERSION_MINOR}.${ZFP_VERSION_PATCH}")
    endif ()

    unset(_zfp_version_header)
    unset(_zfp_major)
    unset(_zfp_minor)
    unset(_zfp_patch)
    unset(_zfp_tweak)
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZFP
  REQUIRED_VARS ZFP_LIBRARY ZFP_INCLUDE_DIR
  VERSION_VAR ZFP_VERSION
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
