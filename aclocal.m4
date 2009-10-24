# check for missing posix types
AC_DEFUN([CHECK_POSIX_TYPES], [
	AC_CHECK_TYPE(int8_t,,AC_DEFINE(MISSING_SIGNED),)
	AC_CHECK_TYPE(int16_t,,AC_DEFINE(MISSING_SIGNED),)
	AC_CHECK_TYPE(int32_t,,AC_DEFINE(MISSING_SIGNED),)
	AC_CHECK_TYPE(u_int8_t,,AC_DEFINE(MISSING_UNSIGNED),)
	AC_CHECK_TYPE(u_int16_t,,AC_DEFINE(MISSING_UNSIGNED),)
	AC_CHECK_TYPE(u_int32_t,,AC_DEFINE(MISSING_UNSIGNED),)
	])


# check if the setenv function exists
AC_DEFUN([CHECK_SETENV], [
	has_setenv="no";
	AC_CHECK_LIB([c], [setenv], [has_setenv="yes"],,)

	if test "$has_setenv" != "no"; then
		AC_DEFINE(HAVE_SETENV)
	fi

	])

# check for many pthread ways
AC_DEFUN([CHECK_POSIX_THREADS], [
	pthread_libs="no";
	AC_CHECK_LIB([pthread], [pthread_create],
	[pthread_libs="-lpthread"],   
	AC_CHECK_LIB([c_r], [pthread_create], [pthread_libs="-lc_r"],   
	AC_CHECK_LIB([c], [pthread_create],,
	AC_MSG_WARN(pthreads lib not found - threading will be disabled.))))

  	if test "$pthread_libs" != "no"; then
    		AC_DEFINE(YSM_WITH_THREADS)
	        if test `uname -s` = "OpenBSD"; then
        	        LIBS="-pthread $LIBS"
		else    
                	LIBS="$pthread_libs $LIBS"
        	fi      
    	fi
	])

# check for iconv..
AC_DEFUN([CHECK_ICONV], [
	iconv_libs="no";
	AC_CHECK_LIB([iconv], [iconv_open], [iconv_libs="-liconv"],
		AC_CHECK_LIB([iconv], [libiconv_open], [iconv_libs="-liconv"],
		AC_CHECK_LIB([c], [iconv_open], [iconv_libs=""],
		AC_CHECK_LIB([c], [libiconv_open], [iconv_libs=""],
	AC_MSG_WARN(libiconv not found - charset convertion and utf8 support will be disabled.)))))

	if test "$iconv_libs" != "no"; then
		AC_DEFINE(YSM_USE_CHARCONV)
		LIBS="$iconv_libs $LIBS"
	fi
	])

# check for readline..the many ways we can.
AC_DEFUN([CHECK_READLINE], [
  AC_CACHE_CHECK([for a readline compatible library],
                 vl_cv_lib_readline, [
    ORIG_LIBS="$LIBS"
    for readline_lib in readline edit editline; do
      for termcap_lib in "" termcap curses ncurses; do
        if test -z "$termcap_lib"; then
          TRY_LIB="-l$readline_lib"
        else
          TRY_LIB="-l$readline_lib -l$termcap_lib"
        fi
        LIBS="$ORIG_LIBS $TRY_LIB"
        AC_TRY_LINK_FUNC(readline, vl_cv_lib_readline="$TRY_LIB")
        if test -n "$vl_cv_lib_readline"; then
          break
        fi
      done
      if test -n "$vl_cv_lib_readline"; then
        break
      fi
    done
    if test -z "$vl_cv_lib_readline"; then
      vl_cv_lib_readline="no"
      LIBS="$ORIG_LIBS"
    fi
  ])

  if test "$vl_cv_lib_readline" != "no"; then
	# use VI_MODE for default ;P
	AC_DEFINE(VI_MODE)
    AC_DEFINE(HAVE_LIBREADLINE, 1,
              [Define if you have a readline compatible library])
    AC_CHECK_HEADERS(readline.h readline/readline.h)
    AC_CACHE_CHECK([whether readline supports history],
                   vl_cv_lib_readline_history, [
      vl_cv_lib_readline_history="no"
      AC_TRY_LINK_FUNC(add_history, vl_cv_lib_readline_history="yes")
    ])
    if test "$vl_cv_lib_readline_history" = "yes"; then
      AC_DEFINE(HAVE_READLINE_HISTORY, 1,
                [Define if your readline library has \`add_history'])
      AC_CHECK_HEADERS(history.h readline/history.h)
    fi
  fi
])dnl
