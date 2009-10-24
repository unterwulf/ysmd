AC_INIT(src/main.c)

# do we have what we need?
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# check missing posix types 
CHECK_POSIX_TYPES

# check if we have threads support
CHECK_POSIX_THREADS

# check for iconv
CHECK_ICONV

# check for setenv
CHECK_SETENV

# check for sparc required libraries
AC_CHECK_LIB([socket], [connect], LIBS="-lsocket $LIBS")
AC_CHECK_LIB([nsl], [gethostbyname], LIBS="-lnsl $LIBS")

AC_SUBST(pthread_libs)
AC_SUBST(CMDOBJS)
AC_STDC_HEADERS
AC_OUTPUT([Makefile src/Makefile src/man/Makefile])
