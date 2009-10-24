AC_INIT(src/main.c)
# give welcome
echo ""
echo "ysmd ICQ client. [http://ysmd.sourceforge.net]"
echo "vs at kp4 dot ru"
echo ""

# do we have what we need?
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# check missing posix types 
CHECK_POSIX_TYPES

##### fribidi #####

AC_ARG_WITH(fribidi,
	[  --with-fribidi=[DIR]	  enable the fribidi library in [DIR] to enable Hebrew and Arabic support],
	[with_fribidi=$withval],
	[with_fribidi=no])

if test "$with_fribidi" = "yes"; then
    for ac_dir in /usr/local /usr; do
        if test -f "$ac_dir/include/fribidi/fribidi.h"; then
            with_fribidi=$ac_dir
            break;
        fi
    done
fi

if test -n "$with_fribidi" -a "$with_fribidi" != "no"; then
    if test "$with_fribidi" = "yes"; then with_fribidi="/usr"; fi

    if test -r $with_fribidi/include/fribidi/fribidi.h; then
        LIBS="$LIBS `$with_fribidi/bin/fribidi-config --libs`"
        AC_DEFINE(USE_FRIBIDI, 1, [use fribidi])
    fi
fi

##### fribidi #####

# check if we have threads support
CHECK_POSIX_THREADS

# check for iconv
CHECK_ICONV

# check for setenv
CHECK_SETENV

# check for sparc required libraries
AC_CHECK_LIB([socket],
                [connect],
		LIBS="-lsocket $LIBS")
AC_CHECK_LIB([nsl],
                [gethostbyname],
		LIBS="-lnsl $LIBS")

# final checks

AC_SUBST(pthread_libs)
AC_SUBST(CMDOBJS)
AC_STDC_HEADERS
AC_OUTPUT([Makefile src/Makefile src/man/Makefile])
