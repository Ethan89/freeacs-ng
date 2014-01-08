# CSCGI_FOUND - true if library and headers were found
# CSCGI_INCLUDE_DIRS - include directories
# CSCGI_LIBRARIES - library directories

find_package(PkgConfig)
pkg_check_modules(PC_CSCGI QUIET scgi)

find_path(CSCGI_INCLUDE_DIR scgi.h
	HINTS ${PC_CSCGI_INCLUDEDIR} ${PC_CSCGI_INCLUDE_DIRS})

find_library(CSCGI_LIBRARY NAMES cscgi libcscgi
	HINTS ${PC_CSCGI_LIBDIR} ${PC_CSCGI_LIBRARY_DIRS})

set(CSCGI_LIBRARIES ${CSCGI_LIBRARY})
set(CSCGI_INCLUDE_DIRS ${CSCGI_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(CSCGI DEFAULT_MSG CSCGI_LIBRARY CSCGI_INCLUDE_DIR)

mark_as_advanced(CSCGI_INCLUDE_DIR CSCGI_LIBRARY)
