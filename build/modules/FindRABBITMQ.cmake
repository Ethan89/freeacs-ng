# RABBITMQ_FOUND - true if library and headers were found
# RABBITMQ_INCLUDE_DIRS - include directories
# RABBITMQ_LIBRARIES - library directories

find_package(PkgConfig)
pkg_check_modules(PC_RABBITMQ QUIET rabbitmq)

find_path(RABBITMQ_INCLUDE_DIR amqp.h
	HINTS ${PC_RABBITMQ_INCLUDEDIR} ${PC_RABBITMQ_INCLUDE_DIRS})

find_library(RABBITMQ_LIBRARY NAMES rabbitmq librabbitmq
	HINTS ${PC_RABBITMQ_LIBDIR} ${PC_RABBITMQ_LIBRARY_DIRS})

set(RABBITMQ_LIBRARIES ${RABBITMQ_LIBRARY})
set(RABBITMQ_INCLUDE_DIRS ${RABBITMQ_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(RABBITMQ DEFAULT_MSG RABBITMQ_LIBRARY RABBITMQ_INCLUDE_DIR)

mark_as_advanced(RABBITMQ_INCLUDE_DIR RABBITMQ_LIBRARY)
