AC_INIT([freeacs-ng], [0.1], [freeacs-ng@lukaperkov.net])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/freeacs-ng.c])

# checks for programs
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_CC_C_O

# checks for libraries
AC_ARG_WITH([libfreecwmp-include-path],
  [AS_HELP_STRING([--with-libfreecwmp-include-path],
    [location of the libfreecwmp library headers])],
  [LIBFREECWMP_CFLAGS="-I$withval"])
AC_SUBST([LIBFREECWMP_CFLAGS])

AC_ARG_WITH([libfreecwmp-lib-path],
  [AS_HELP_STRING([--with-libfreecwmp-lib-path], [location of the libfreecwmp library])], [LIBFREECWMP_LDFLAGS="-L$withval"])
AC_SUBST([LIBFREECWMP_LDFLAGS])

LIBFREECWMP_LIBS='-lfreecwmp'
AC_SUBST([LIBFREECWMP_LIBS])

AC_ARG_WITH([event-include-path],
  [AS_HELP_STRING([--with-event-include-path],
    [location of the event library headers])],
  [EVENT_CFLAGS="-I$withval"])
AC_SUBST([EVENT_CFLAGS])

AC_ARG_WITH([event-lib-path],
  [AS_HELP_STRING([--with-event-lib-path], [location of the event library])], [EVENT_LDFLAGS="-L$withval"])
AC_SUBST([EVENT_LDFLAGS])

EVENT_LIBS='-levent'
AC_SUBST([EVENT_LIBS])

AC_ARG_WITH([netstring-include-path],
  [AS_HELP_STRING([--with-netstring-include-path],
    [location of the netstring library headers])],
  [NETSTRING_CFLAGS="-I$withval"])
AC_SUBST([NETSTRING_CFLAGS])

AC_ARG_WITH([netstring-lib-path],
  [AS_HELP_STRING([--with-netstring-lib-path], [location of the netstring library])], [NETSTRING_LDFLAGS="-L$withval"])
AC_SUBST([NETSTRING_LDFLAGS])

NETSTRING_LIBS='-lnetstring'
AC_SUBST([NETSTRING_LIBS])

AC_ARG_WITH([cscgi-include-path],
  [AS_HELP_STRING([--with-cscgi-include-path],
    [location of the cscgi library headers])],
  [CSCGI_CFLAGS="-I$withval"])
AC_SUBST([CSCGI_CFLAGS])

AC_ARG_WITH([cscgi-lib-path],
  [AS_HELP_STRING([--with-cscgi-lib-path], [location of the cscgi library])], [CSCGI_LDFLAGS="-L$withval"])
AC_SUBST([CSCGI_LDFLAGS])

CSCGI_LIBS='-lcscgi'
AC_SUBST([CSCGI_LIBS])

AC_ARG_WITH([uci-include-path],
  [AS_HELP_STRING([--with-uci-include-path],
    [location of the uci library headers])],
  [LIBUCI_CFLAGS="-I$withval"])
AC_SUBST([LIBUCI_CFLAGS])

AC_ARG_WITH([uci-lib-path],
  [AS_HELP_STRING([--with-uci-lib-path], [location of the uci library])], [LIBUCI_LDFLAGS="-L$withval"])
AC_SUBST([LIBUCI_LDFLAGS])

LIBUCI_LIBS='-luci'
AC_SUBST([LIBUCI_LIBS])

PKG_CHECK_MODULES(LIBXML, libxml-2.0)
PKG_CHECK_MODULES(LIBJSON, json)
PKG_CHECK_MODULES(LIBRABBITMQ, librabbitmq)

AC_ARG_ENABLE(dummy_mode, [AS_HELP_STRING([--enable-dummy-mode], [enable dummy mode])], AC_DEFINE(DUMMY_MODE),)

# checks for header files
AC_CHECK_HEADERS([stdlib.h string.h])

# checks for typedefs, structures, and compiler characteristics
AC_TYPE_UINT8_T

# Makefiles
AC_CONFIG_FILES([
Makefile
bin/Makefile
])

AC_OUTPUT
