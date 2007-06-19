# Check for pthread: This code was taken from:
# http://autoconf-archive.cryp.to/acx_pthread.html

# License 
# Copyright © 2006 Steven G. Johnson <stevenj@alum.mit.edu> 
#  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
#  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. 
#  You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
#  As a special exception, the respective Autoconf Macro's copyright owner gives unlimited permission to copy, distribute and modify the configure scripts that are the output of Autoconf when processing the Macro. You need not follow the terms of the GNU General Public License when using or distributing such scripts, even though portions of the text of the Macro appear in them. The GNU General Public License (GPL) does govern all other use of the material that constitutes the Autoconf Macro. 
#  This special exception to the GPL applies to versions of the Autoconf Macro released by the Autoconf Macro Archive. When you make and distribute a modified version of the Autoconf Macro, you may extend this special exception to the GPL to apply to your modified version as well. 

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
#      ... -mt is also the pthreads flag for HP/aCC
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthreads/-mt/
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthreads pthread -mt -pthread $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                pthread-config)
                AC_CHECK_PROG(acx_pthread_config, pthread-config, yes, no)
                if test x"$acx_pthread_config" = xno; then continue; fi
                PTHREAD_CFLAGS="`pthread-config --cflags`"
                PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
        AC_MSG_CHECKING([for joinable pthread attribute])
        attr_name=unknown
        for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
            AC_TRY_LINK([#include <pthread.h>], [int attr=$attr; return attr;],
                        [attr_name=$attr; break])
        done
        AC_MSG_RESULT($attr_name)
        if test "$attr_name" != PTHREAD_CREATE_JOINABLE; then
            AC_DEFINE_UNQUOTED(PTHREAD_CREATE_JOINABLE, $attr_name,
                               [Define to necessary symbol if this constant
                                uses a non-standard name on your system.])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
            *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
            *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
            PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with xlc_r or cc_r
        if test x"$GCC" != xyes; then
          AC_CHECK_PROGS(PTHREAD_CC, xlc_r cc_r, ${CC})
        else
          PTHREAD_CC=$CC
        fi
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD


#************************************************************************
# This check was borrowed from:
# http://autoconf-archive.cryp.to/ax_lib_mysql.html
#
# Copyright notice:
# License 
# Copyright © 2006 Mateusz Loskot <mateusz@loskot.net> 
#  Copying and distribution of this file, with or without modification, are permitted in any medium without royalty provided the copyright notice and this notice are preserved. 
#

AC_DEFUN([AX_LIB_MYSQL],
[
    AC_ARG_WITH([mysql],
        AC_HELP_STRING([--with-mysql=@<:@ARG@:>@],
            [use MySQL client library @<:@default=yes@:>@, optionally specify path to mysql_config]
        ),
        [
        if test "$withval" = "no"; then
            want_mysql="no"
        elif test "$withval" = "yes"; then
            want_mysql="yes"
        else
            want_mysql="yes"
            MYSQL_CONFIG="$withval"
        fi
        ],
        [want_mysql="yes"]
    )

    MYSQL_CFLAGS=""
    MYSQL_LDFLAGS=""
    MYSQL_VERSION=""

    dnl
    dnl Check MySQL libraries (libpq)
    dnl

    if test "$want_mysql" = "yes"; then

        if test -z "$MYSQL_CONFIG" -o test; then
            AC_PATH_PROG([MYSQL_CONFIG], [mysql_config], [no])
        fi

        if test "$MYSQL_CONFIG" != "no"; then
            AC_MSG_CHECKING([for MySQL libraries])

            MYSQL_CFLAGS="`$MYSQL_CONFIG --cflags`"
            MYSQL_LDFLAGS="`$MYSQL_CONFIG --libs`"

            MYSQL_VERSION=`$MYSQL_CONFIG --version`

            AC_DEFINE([HAVE_MYSQL], [1],
                [Define to 1 if MySQL libraries are available])

            found_mysql="yes"
            AC_MSG_RESULT([yes])
        else
            found_mysql="no"
            AC_MSG_RESULT([no])
        fi
    fi

    dnl
    dnl Check if required version of MySQL is available
    dnl


    mysql_version_req=ifelse([$1], [], [], [$1])

    if test "$found_mysql" = "yes" -a -n "$mysql_version_req"; then

        AC_MSG_CHECKING([if MySQL version is >= $mysql_version_req])

        dnl Decompose required version string of MySQL
        dnl and calculate its number representation
        mysql_version_req_major=`expr $mysql_version_req : '\([[0-9]]*\)'`
        mysql_version_req_minor=`expr $mysql_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_req_micro=`expr $mysql_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$mysql_version_req_micro" = "x"; then
            mysql_version_req_micro="0"
        fi

        mysql_version_req_number=`expr $mysql_version_req_major \* 1000000 \
                                   \+ $mysql_version_req_minor \* 1000 \
                                   \+ $mysql_version_req_micro`

        dnl Decompose version string of installed MySQL
        dnl and calculate its number representation
        mysql_version_major=`expr $MYSQL_VERSION : '\([[0-9]]*\)'`
        mysql_version_minor=`expr $MYSQL_VERSION : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_micro=`expr $MYSQL_VERSION : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$mysql_version_micro" = "x"; then
            mysql_version_micro="0"
        fi

        mysql_version_number=`expr $mysql_version_major \* 1000000 \
                                   \+ $mysql_version_minor \* 1000 \
                                   \+ $mysql_version_micro`

        mysql_version_check=`expr $mysql_version_number \>\= $mysql_version_req_number`
        if test "$mysql_version_check" = "1"; then
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([no])
        fi
    fi

    AC_SUBST([MYSQL_VERSION])
    AC_SUBST([MYSQL_CFLAGS])
    AC_SUBST([MYSQL_LDFLAGS])
])



###############################################################################
# Check for boost base
#
# http://autoconf-archive.cryp.to/ax_boost_base.html
#
# License
# Copyright © 2007 Thomas Porschberg <thomas@randspringer.de>
#
# Copying and distribution of this file, with or without modification, are 
# permitted in any medium without royalty provided the copyright notice and 
# this notice are preserved.
#
AC_DEFUN([AX_BOOST_BASE],
[
AC_ARG_WITH([boost],
        AS_HELP_STRING([--with-boost@<:@=DIR@:>@], [use boost (default is yes) - it is possible to specify the root directory for boost (optional)]),
        [
    if test "$withval" = "no"; then
                want_boost="no"
    elif test "$withval" = "yes"; then
        want_boost="yes"
        ac_boost_path=""
    else
            want_boost="yes"
        ac_boost_path="$withval"
        fi
    ],
    [want_boost="yes"])

if test "x$want_boost" = "xyes"; then
        boost_lib_version_req=ifelse([$1], ,1.20.0,$1)
        boost_lib_version_req_shorten=`expr $boost_lib_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
        boost_lib_version_req_major=`expr $boost_lib_version_req : '\([[0-9]]*\)'`
        boost_lib_version_req_minor=`expr $boost_lib_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        boost_lib_version_req_sub_minor=`expr $boost_lib_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$boost_lib_version_req_sub_minor" = "x" ; then
                boost_lib_version_req_sub_minor="0"
        fi
        WANT_BOOST_VERSION=`expr $boost_lib_version_req_major \* 100000 \+  $boost_lib_version_req_minor \* 100 \+ $boost_lib_version_req_sub_minor`
        AC_MSG_CHECKING(for boostlib >= $boost_lib_version_req)
        succeeded=no

        dnl first we check the system location for boost libraries
        dnl this location ist chosen if boost libraries are installed with the --layout=system option
        dnl or if you install boost with RPM
        if test "$ac_boost_path" != ""; then
                BOOST_LDFLAGS="-L$ac_boost_path/lib"
                BOOST_CPPFLAGS="-I$ac_boost_path/include"
        else
                for ac_boost_path_tmp in /usr /usr/local /opt ; do
                        if test -d "$ac_boost_path_tmp/include/boost" && test -r "$ac_boost_path_tmp/include/boost"; then
                                BOOST_LDFLAGS="-L$ac_boost_path_tmp/lib"
                                BOOST_CPPFLAGS="-I$ac_boost_path_tmp/include"
                                break;
                        fi
                done
        fi

        CPPFLAGS_SAVED="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
        export CPPFLAGS

        LDFLAGS_SAVED="$LDFLAGS"
        LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
        export LDFLAGS

        AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <boost/version.hpp>
        ]], [[
        #if BOOST_VERSION >= $WANT_BOOST_VERSION
        // Everything is okay
        #else
        #  error Boost version is too old
        #endif
        ]])],[
        AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
        ],[
        ])
        AC_LANG_POP([C++])



        dnl if we found no boost with system layout we search for boost libraries
        dnl built and installed without the --layout=system option or for a staged(not installed) version
        if test "x$succeeded" != "xyes"; then
                _version=0
                if test "$ac_boost_path" != ""; then
                        BOOST_LDFLAGS="-L$ac_boost_path/lib"
                        if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                        _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                        V_CHECK=`expr $_version_tmp \> $_version`
                                        if test "$V_CHECK" = "1" ; then
                                                _version=$_version_tmp
                                        fi
                                        VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                                        BOOST_CPPFLAGS="-I$ac_boost_path/include/boost-$VERSION_UNDERSCORE"
                                done
                        fi
                else
                        for ac_boost_path in /usr /usr/local /opt ; do
                                if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                        for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                                _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                                V_CHECK=`expr $_version_tmp \> $_version`
                                                if test "$V_CHECK" = "1" ; then
                                                        _version=$_version_tmp
                                                        best_path=$ac_boost_path
                                                fi
                                        done
                                fi
                        done

                        VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                        BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
                        BOOST_LDFLAGS="-L$best_path/lib"

                        if test "x$BOOST_ROOT" != "x"; then
                                if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/lib" && test -r "$BOOST_ROOT/stage/lib"; then
                                        version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
                                        stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
                                        stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
                                        V_CHECK=`expr $stage_version_shorten \>\= $_version`
                                        if test "$V_CHECK" = "1" ; then
                                                AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
                                                BOOST_CPPFLAGS="-I$BOOST_ROOT"
                                                BOOST_LDFLAGS="-L$BOOST_ROOT/stage/lib"
                                        fi
                                fi
                        fi
                fi

                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

                AC_LANG_PUSH(C++)
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                @%:@include <boost/version.hpp>
                ]], [[
                #if BOOST_VERSION >= $WANT_BOOST_VERSION
                // Everything is okay
                #else
                #  error Boost version is too old
                #endif
                ]])],[
                AC_MSG_RESULT(yes)
                succeeded=yes
                found_system=yes
                ],[
                ])
                AC_LANG_POP([C++])
        fi

        if test "$succeeded" != "yes" ; then
                if test "$_version" = "0" ; then
                        AC_MSG_ERROR([[We could not detect the boost libraries (version $boost_lib_version_req_shorten or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.]])
                else
                        AC_MSG_NOTICE([Your boost libraries seems to old (version $_version).])
                fi
        else
                AC_SUBST(BOOST_CPPFLAGS)
                AC_SUBST(BOOST_LDFLAGS)
                AC_DEFINE(HAVE_BOOST,,[define if the Boost library is available])
        fi

        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
fi

])


###############################################################################
# Check for boost regex
#
# http://autoconf-archive.cryp.to/ax_boost_regex.html
#
# License
# Copyright © 2007 Thomas Porschberg <thomas@randspringer.de>
# Copyright © 2007 Michael Tindal <mtindal@paradoxpoint.com>
#
#
# Copying and distribution of this file, with or without modification, are 
# permitted in any medium without royalty provided the copyright notice and 
# this notice are preserved.
#
AC_DEFUN([AX_BOOST_REGEX],
[
        AC_ARG_WITH([boost-regex],
        AS_HELP_STRING([--with-boost-regex@<:@=special-lib@:>@],
                   [use the Regex library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-regex=boost_regex-gcc-mt-d-1_33_1 ]),
        [
        if test "$withval" = "no"; then
                        want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_regex_lib=""
        else
                    want_boost="yes"
                ax_boost_user_regex_lib="$withval"
                fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        AC_CACHE_CHECK(whether the Boost::Regex library is available,
                                           ax_cv_boost_regex,
        [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/regex.hpp>
                                                                                                ]],
                                   [[boost::regex r(); return 0;]]),
                   ax_cv_boost_regex=yes, ax_cv_boost_regex=no)
         AC_LANG_POP([C++])
                ])
                if test "x$ax_cv_boost_regex" = "xyes"; then
                        AC_DEFINE(HAVE_BOOST_REGEX,,[define if the Boost::Regex library is available])
                        BN=boost_regex
            if test "x$ax_boost_user_regex_lib" = "x"; then
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main, [BOOST_REGEX_LIB="-l$ax_lib"; AC_SUBST(BOOST_REGEX_LIB) link_regex="yes"; break],
                                 [link_regex="no"])
                                done
            else
               for ax_lib in $ax_boost_user_regex_lib $BN-$ax_boost_user_regex_lib; do
                                      AC_CHECK_LIB($ax_lib, main,
                                   [BOOST_REGEX_LIB="-l$ax_lib"; AC_SUBST(BOOST_REGEX_LIB) link_regex="yes"; break],
                                   [link_regex="no"])
               done
            fi
                        if test "x$link_regex" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
                        fi
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        fi
])


###############################################################################
# Check for boost (all of it)
#
# http://autoconf-archive.cryp.to/ax_boost.html
#
# License
# Copyright © 2007 Thomas Porschberg <thomas@randspringer.de>
#
# Copying and distribution of this file, with or without modification, are 
# permitted in any medium without royalty provided the copyright notice and 
# this notice are preserved.
#
AC_DEFUN([AX_BOOST],
[
    AC_ARG_WITH([boost],
                AS_HELP_STRING([--with-boost=DIR],
                [use boost (default is yes) specify the root directory for boost library (optional)]),
                [
                if test "$withval" = "no"; then
                            want_boost="no"
                elif test "$withval" = "yes"; then
                    want_boost="yes"
                    ac_boost_path=""
                else
                                want_boost="yes"
                        ac_boost_path="$withval"
                        fi
                ],
                [want_boost="yes"])

    AC_CANONICAL_BUILD
        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
                boost_lib_version_req=ifelse([$1], ,1.20.0,$1)
                boost_lib_version_req_shorten=`expr $boost_lib_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
                boost_lib_version_req_major=`expr $boost_lib_version_req : '\([[0-9]]*\)'`
                boost_lib_version_req_minor=`expr $boost_lib_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
                boost_lib_version_req_sub_minor=`expr $boost_lib_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
                if test "x$boost_lib_version_req_sub_minor" = "x" ; then
                        boost_lib_version_req_sub_minor="0"
        fi
                WANT_BOOST_VERSION=`expr $boost_lib_version_req_major \* 100000 \+  $boost_lib_version_req_minor \* 100 \+ $boost_lib_version_req_sub_minor`
                AC_MSG_CHECKING(for boostlib >= $boost_lib_version_req)
                succeeded=no

                dnl first we check the system location for boost libraries
                dnl this location ist chosen if boost libraries are installed with the --layout=system option
                dnl or if you install boost with RPM
                if test "$ac_boost_path" != ""; then
                        BOOST_LDFLAGS="-L$ac_boost_path/lib"
                        BOOST_CPPFLAGS="-I$ac_boost_path/include"
                else
                        for ac_boost_path_tmp in /usr /usr/local /opt ; do
                                if test -d "$ac_boost_path_tmp/include/boost" && test -r "$ac_boost_path_tmp/include/boost"; then
                                        BOOST_LDFLAGS="-L$ac_boost_path_tmp/lib"
                                        BOOST_CPPFLAGS="-I$ac_boost_path_tmp/include"
                                        break;
                                fi
                        done
                fi

                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
@%:@include <boost/version.hpp>
]],
       [[
#if BOOST_VERSION >= $WANT_BOOST_VERSION
// Everything is okay
#else
#  error Boost version is too old
#endif

                ]])],
        [
         AC_MSG_RESULT(yes)
                 succeeded=yes
                 found_system=yes
         ifelse([$2], , :, [$2])
       ],
       [
       ])
       AC_LANG_POP([C++])
                dnl if we found no boost with system layout we search for boost libraries
                dnl built and installed without the --layout=system option or for a staged(not installed) version
                if test "x$succeeded" != "xyes"; then
                        _version=0
                        if test "$ac_boost_path" != ""; then
                BOOST_LDFLAGS="-L$ac_boost_path/lib"
                                if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                        for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                                _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                                V_CHECK=`expr $_version_tmp \> $_version`
                                                if test "$V_CHECK" = "1" ; then
                                                        _version=$_version_tmp
                                                fi
                                                VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                                                BOOST_CPPFLAGS="-I$ac_boost_path/include/boost-$VERSION_UNDERSCORE"
                                        done
                                fi
                        else
                                for ac_boost_path in /usr /usr/local /opt ; do
                                        if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                                for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                                        _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                                        V_CHECK=`expr $_version_tmp \> $_version`
                                                        if test "$V_CHECK" = "1" ; then
                                                                _version=$_version_tmp
                                                                best_path=$ac_boost_path
                                                        fi
                                                done
                                        fi
                                done

                                VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                                BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
                                BOOST_LDFLAGS="-L$best_path/lib"

                        if test "x$BOOST_ROOT" != "x"; then
                    if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/lib" && test -r "$BOOST_ROOT/stage/lib"; then
                                                version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
                                                stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
                                                stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
                                                V_CHECK=`expr $stage_version_shorten \>\= $_version`
                                                if test "$V_CHECK" = "1" ; then
                                                        AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
                                                        BOOST_CPPFLAGS="-I$BOOST_ROOT"
                                                        BOOST_LDFLAGS="-L$BOOST_ROOT/stage/lib"
                                                fi
                                        fi
                        fi
                        fi

                        CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                        export CPPFLAGS
                        LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                        export LDFLAGS

            AC_LANG_PUSH(C++)
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
@%:@include <boost/version.hpp>
]],
       [[
#if BOOST_VERSION >= $WANT_BOOST_VERSION
// Everything is okay
#else
#  error Boost version is too old
#endif

                ]])],
        [
         AC_MSG_RESULT(yes ($_version))
                 succeeded=yes
         ifelse([$2], , :, [$2])
       ],
       [
         AC_MSG_RESULT(no ($_version))
         ifelse([$3], , :, [$3])
       ])
        AC_LANG_POP([C++])
                fi

                if test "$succeeded" != "yes" ; then
                        if test "$_version" = "0" ; then
                                AC_MSG_ERROR([[We could not detect the boost libraries (version $boost_lib_version_req_shorten or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.]])
                        else
                                AC_MSG_ERROR('Your boost libraries seems to old (version $_version).  We need at least $boost_lib_version_shorten')
                        fi
                else
                        AC_SUBST(BOOST_CPPFLAGS)
                        AC_SUBST(BOOST_LDFLAGS)
                        AC_DEFINE(HAVE_BOOST,,[define if the Boost library is available])

                        AC_CACHE_CHECK([whether the Boost::Filesystem library is available],
                                                   ax_cv_boost_filesystem,
                                                [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/filesystem/path.hpp>]],
                                   [[using namespace boost::filesystem;
                                   path my_path( "foo/bar/data.txt" );
                                   return 0;]]),
                                               ax_cv_boost_filesystem=yes, ax_cv_boost_filesystem=no)
                                   AC_LANG_POP([C++])
                        ])
                        if test "$ax_cv_boost_filesystem" = "yes"; then
                                AC_DEFINE(HAVE_BOOST_FILESYSTEM,,[define if the Boost::FILESYSTEM library is available])
                                BN=boost_filesystem
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_FILESYSTEM_LIB="-l$ax_lib"; AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes"; break],
                                 [link_filesystem="no"])
                                done
                                if test "x$link_filesystem" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi

                        AC_CACHE_CHECK([whether the Boost::Program_Options library is available],
                                                   ax_cv_boost_program_options,
                                                   [AC_LANG_PUSH([C++])
                                       AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/program_options.hpp>]],
                                   [[boost::program_options::options_description generic("Generic options");
                                   return 0;]]),
                           ax_cv_boost_program_options=yes, ax_cv_boost_program_options=no)
                           AC_LANG_POP([C++])
                        ])
                        if test "$ax_cv_boost_program_options" = yes; then
                                AC_DEFINE(HAVE_BOOST_PROGRAM_OPTIONS,,[define if the Boost::PROGRAM_OPTIONS library is available])
                                BN=boost_program_options
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_PROGRAM_OPTIONS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB) link_program_options="yes"; break],
                                 [link_program_options="no"])
                                done
                                if test "x$link_program_options="no"" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi

                        AC_CACHE_CHECK(whether the Boost::Thread library is available,
                                                   ax_cv_boost_thread,
                                                [AC_LANG_PUSH([C++])
                         CXXFLAGS_SAVE=$CXXFLAGS

                         if test "x$build_os" = "xsolaris" ; then
                                 CXXFLAGS="-pthreads $CXXFLAGS"
                         elif test "x$build_os" = "xming32" ; then
                                 CXXFLAGS="-mthreads $CXXFLAGS"
                         else
                                CXXFLAGS="-pthread $CXXFLAGS"
                         fi
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/thread/thread.hpp>]],
                                   [[boost::thread_group thrds;
                                   return 0;]]),
                   ax_cv_boost_thread=yes, ax_cv_boost_thread=no)
                         CXXFLAGS=$CXXFLAGS_SAVE
             AC_LANG_POP([C++])
                        ])
                        if test "x$ax_cv_boost_thread" = "xyes"; then
               if test "x$build_os" = "xsolaris" ; then
                                  BOOST_CPPFLAGS="-pthreads $BOOST_CPPFLAGS"
                           elif test "x$build_os" = "xming32" ; then
                                  BOOST_CPPFLAGS="-mthreads $BOOST_CPPFLAGS"
                           else
                                  BOOST_CPPFLAGS="-pthread $BOOST_CPPFLAGS"
                           fi

                                AC_SUBST(BOOST_CPPFLAGS)
                                AC_DEFINE(HAVE_BOOST_THREAD,,[define if the Boost::THREAD library is available])
                                BN=boost_thread
                        LDFLAGS_SAVE=$LDFLAGS
                        case "x$build_os" in
                          *bsd* )
                               LDFLAGS="-pthread $LDFLAGS"
                          break;
                          ;;
                        esac

                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main, [BOOST_THREAD_LIB="-l$ax_lib"; AC_SUBST(BOOST_THREAD_LIB) link_thread="yes"; break],
                                 [link_thread="no"])
                                done
                                if test "x$link_thread" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                else
                    case "x$build_os" in
                       *bsd* )
                       BOOST_LDFLAGS="-pthread $BOOST_LDFLAGS"
                       break;
                       ;;
                    esac
                                fi
                        fi

                        AC_CACHE_CHECK(whether the Boost::IOStreams library is available,
                                                   ax_cv_boost_iostreams,
                                                [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/iostreams/filtering_stream.hpp>
                                                                                                 @%:@include <boost/range/iterator_range.hpp>
                                                                                                ]],
                                   [[std::string  input = "Hello World!";
                                                                         namespace io = boost::iostreams;
                                                                         io::filtering_istream  in(boost::make_iterator_range(input));
                                                                         return 0;
                                   ]]),
                   ax_cv_boost_iostreams=yes, ax_cv_boost_iostreams=no)
                         AC_LANG_POP([C++])
                        ])
                        if test "x$ax_cv_boost_iostreams" = "xyes"; then
                                AC_DEFINE(HAVE_BOOST_IOSTREAMS,,[define if the Boost::IOStreams library is available])
                                BN=boost_iostreams
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main, [BOOST_IOSTREAMS_LIB="-l$ax_lib"; AC_SUBST(BOOST_IOSTREAMS_LIB) link_thread="yes"; break],
                                 [link_thread="no"])
                                done
                                if test "x$link_thread" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi

                        AC_CACHE_CHECK(whether the Boost::Serialization library is available,
                                                   ax_cv_boost_serialization,
                                                [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <fstream>
                                                                                                 @%:@include <boost/archive/text_oarchive.hpp>
                                                 @%:@include <boost/archive/text_iarchive.hpp>
                                                                                                ]],
                                   [[std::ofstream ofs("filename");
                                                                        boost::archive::text_oarchive oa(ofs);
                                                                         return 0;
                                   ]]),
                   ax_cv_boost_serialization=yes, ax_cv_boost_serialization=no)
                         AC_LANG_POP([C++])
                        ])
                        if test "x$ax_cv_boost_serialization" = "xyes"; then
                                AC_DEFINE(HAVE_BOOST_SERIALIZATION,,[define if the Boost::Serialization library is available])
                                BN=boost_serialization
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_SERIALIZATION_LIB="-l$ax_lib"; AC_SUBST(BOOST_SERIALIZATION_LIB) link_serialization="yes"; break],
                                 [link_serialization="no"])
                                done
                                if test "x$link_serialization" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi

                                BN=boost_wserialization
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_WSERIALIZATION_LIB="-l$ax_lib"; AC_SUBST(BOOST_WSERIALIZATION_LIB) link_wserialization="yes"; break],
                                 [link_wserialization="no"])
                                done
                                if test "x$link_wserialization" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi

                        AC_CACHE_CHECK(whether the Boost::Signals library is available,
                                                   ax_cv_boost_signals,
                                                [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/signal.hpp>
                                                                                                ]],
                                   [[boost::signal<void ()> sig;
                                     return 0;
                                   ]]),
                   ax_cv_boost_signals=yes, ax_cv_boost_signals=no)
                         AC_LANG_POP([C++])
                        ])
                        if test "x$ax_cv_boost_signals" = "xyes"; then
                                AC_DEFINE(HAVE_BOOST_SIGNALS,,[define if the Boost::Signals library is available])
                                BN=boost_signals
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main, [BOOST_SIGNALS_LIB="-l$ax_lib"; AC_SUBST(BOOST_SIGNALS_LIB) link_signals="yes"; break],
                                 [link_signals="no"])
                                done
                                if test "x$link_signals" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi

                        AC_CACHE_CHECK(whether the Boost::Date_Time library is available,
                                                   ax_cv_boost_date_time,
                                                [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/date_time/gregorian/gregorian_types.hpp>
                                                                                                ]],
                                   [[using namespace boost::gregorian; date d(2002,Jan,10);
                                     return 0;
                                   ]]),
                   ax_cv_boost_date_time=yes, ax_cv_boost_date_time=no)
                         AC_LANG_POP([C++])
                        ])
                        if test "x$ax_cv_boost_date_time" = "xyes"; then
                                AC_DEFINE(HAVE_BOOST_DATE_TIME,,[define if the Boost::Date_Time library is available])
                                BN=boost_date_time
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main, [BOOST_DATE_TIME_LIB="-l$ax_lib"; AC_SUBST(BOOST_DATE_TIME_LIB) link_thread="yes"; break],
                                 [link_thread="no"])
                                done
                                if test "x$link_thread"="no" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi

                        AC_CACHE_CHECK(whether the Boost::Regex library is available,
                                                   ax_cv_boost_regex,
                                                [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/regex.hpp>
                                                                                                ]],
                                   [[boost::regex r(); return 0;]]),
                   ax_cv_boost_regex=yes, ax_cv_boost_regex=no)
                         AC_LANG_POP([C++])
                        ])
                        if test "x$ax_cv_boost_regex" = "xyes"; then
                                AC_DEFINE(HAVE_BOOST_REGEX,,[define if the Boost::Regex library is available])
                                BN=boost_regex
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main, [BOOST_REGEX_LIB="-l$ax_lib"; AC_SUBST(BOOST_REGEX_LIB) link_regex="yes"; break],
                                 [link_regex="no"])
                                done
                                if test "x$link_regex" = "xno"; then
                                        AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi

                        AC_CACHE_CHECK(whether the Boost::UnitTestFramework library is available,
                                                   ax_cv_boost_unit_test_framework,
                                                [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/test/unit_test.hpp>]],
                                    [[using boost::unit_test::test_suite;
                                                         test_suite* test= BOOST_TEST_SUITE( "Unit test example 1" ); return 0;]]),
                   ax_cv_boost_unit_test_framework=yes, ax_cv_boost_unit_test_framework=no)
                         AC_LANG_POP([C++])
                        ])
                        if test "x$ax_cv_boost_unit_test_framework" = "xyes"; then
                AC_DEFINE(HAVE_BOOST_UNIT_TEST_FRAMEWORK,,[define if the Boost::Unit_test_framework library is available])
                        BN=boost_unit_test_framework
                saved_ldflags="${LDFLAGS}"
                        for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                          lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                          $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                LDFLAGS="${LDFLAGS} -l$ax_lib"
                        AC_CACHE_CHECK(the name of the Boost::UnitTestFramework library,
                                                   ax_cv_boost_unit_test_framework_link,
                                                [AC_LANG_PUSH([C++])
                   AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <boost/test/unit_test.hpp>
                                                     using boost::unit_test::test_suite;
                                                     test_suite* init_unit_test_suite( int argc, char * argv[] ) {
                                                     test_suite* test= BOOST_TEST_SUITE( "Unit test example 1" );
                                                     return test;
                                                     }
                                                   ]],
                                 [[ return 0;]])],
                                 link_unit_test_framework="yes",link_unit_test_framework="no")
                         AC_LANG_POP([C++])
               ])
                LDFLAGS="${saved_ldflags}"
                            if test "x$link_unit_test_framework" = "xyes"; then
                    BOOST_UNIT_TEST_FRAMEWORK_LIB="-l$ax_lib"
                    AC_SUBST(BOOST_UNIT_TEST_FRAMEWORK_LIB)
                                        break
                                fi
              done
                            if test "x$link_unit_test_framework" = "xno"; then
                                   AC_MSG_NOTICE(Could not link against $ax_lib !)
                                fi
                        fi
                fi
        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        fi
])

