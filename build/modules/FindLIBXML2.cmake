# LIBXML2_FOUND - true if library and headers were found
# LIBXML2_INCLUDE_DIRS - include directories
# LIBXML2_LIBRARIES - library directories

find_package(PkgConfig)
pkg_check_modules(PC_LIBXML2 QUIET xml2)

find_path(LIBXML2_INCLUDE_DIR libxml/tree.h
	HINTS ${PC_LIBXML2_INCLUDEDIR} ${PC_LIBXML2_INCLUDE_DIRS} PATH_SUFFIXES libxml2)

find_library(LIBXML2_LIBRARY NAMES xml2 libxml2
	HINTS ${PC_LIBXML2_LIBDIR} ${PC_LIBXML2_LIBRARY_DIRS})

set(LIBXML2_LIBRARIES ${LIBXML2_LIBRARY})
set(LIBXML2_INCLUDE_DIRS ${LIBXML2_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBXML2 DEFAULT_MSG LIBXML2_LIBRARY LIBXML2_INCLUDE_DIR)

mark_as_advanced(LIBXML2_INCLUDE_DIR LIBXML2_LIBRARY)
