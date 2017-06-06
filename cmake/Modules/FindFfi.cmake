# - Try to find ffi
# Once done this will define
#  FFI_FOUND - System has ffi
#  FFI_INCLUDE_DIRS - The ffi include directories
#  FFI_LIBRARIES - The libraries needed to use ffi
#  FFI_LDFLAGS_OTHER - Other LDFLAGS needed te use ffi.
#  FFI_DEFINITIONS - Compiler switches required for using ffi

find_package(PkgConfig)
if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER "2.8.1")
   # "QUIET" was introduced in 2.8.2
   set(_QUIET QUIET)
endif ()
pkg_check_modules(PC_LIBFFI ${_QUIET} libffi)
#set(FFI_DEFINITIONS ${PC_LIBFFI_CFLAGS_OTHER})
#set(FFI_LDFLAGS_OTHER ${PC_LIBFFI_LDFLAGS_OTHER})

# before verison 2.8.11 variable CMAKE_LIBRARY_ARCHITECTURE wasn't automatically added to search path
if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_LESS "2.8.11")
   set(FIND_PATH_HINTS ${PC_LIBFFI_INCLUDEDIR}/${CMAKE_LIBRARY_ARCHITECTURE})
endif ()

find_path(FFI_INCLUDE_DIR ffi.h
          HINTS ${PC_LIBFFI_INCLUDEDIR} ${PC_LIBFFI_INCLUDE_DIRS}
          ${FIND_PATH_HINTS}
          )

find_library(FFI_LIBRARY NAMES ffi libffi
             HINTS ${PC_LIBFFI_LIBDIR} ${PC_LIBFFI_LIBRARY_DIRS})

set(FFI_INCLUDE_DIRS ${FFI_INCLUDE_DIR})
set(FFI_LIBRARIES ${FFI_LIBRARY})

#include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EINA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ffi  DEFAULT_MSG
                                  FFI_LIBRARY)

mark_as_advanced(FFI_INCLUDE_DIRS FFI_LIBRARY)
