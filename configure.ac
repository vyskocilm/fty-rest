# autoconf/automake basis
AC_PREREQ([2.5])
AC_INIT([bios], [0.1.0], [EatonIPCOpensource@Eaton.com])
PACKAGE_VENDOR="Eaton"

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
# Sub-makefiles: do not forget to update SUBDIRS list in root Makefile.am
AC_CONFIG_FILES([Makefile])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules tar-pax no-dist-gzip dist-xz subdir-objects])

# libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

AS_IF([test "x$enable_static" = "xyes"], [AC_MSG_ERROR([--enable-static is not supported by BIOS core])])

AC_ARG_WITH([package-vendor],
	[AS_HELP_STRING([--with-package-vendor=ARG],
		[name of the entity which distributes this build
		 of the package (default is '$PACKAGE_VENDOR')])],
	[PACKAGE_VENDOR="${withval}"
	 AC_MSG_NOTICE([Using requested PACKAGE_VENDOR='$PACKAGE_VENDOR'])]
	# This may be left empty by explicit request
)

PKG_PROG_PKG_CONFIG
# Set pkgconfigdir
AC_ARG_WITH([pkgconfigdir], AS_HELP_STRING([--with-pkgconfigdir=PATH],
	[Path to the pkgconfig directory [[LIBDIR/pkgconfig]]]),
	[pkgconfigdir="$withval"], [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST([pkgconfigdir])

# Use the provided pkgconfigdir not only to write our pkg-config data,
# but also as an additional location to look for other packages metadata.
AS_IF([test -n "${pkgconfigdir}" -a -d "${pkgconfigdir}"],
	[AS_IF([test -z "${PKG_CONFIG_PATH}"],
		[PKG_CONFIG_PATH="${pkgconfigdir}"],
		[PKG_CONFIG_PATH="${pkgconfigdir}:${PKG_CONFIG_PATH}"])
	 export PKG_CONFIG_PATH
	])

# the default value may be revised, so we have two layers of conditionals here
_ENABLE_DEBUG_BUILD=1
AC_ARG_ENABLE(debug-build,
	[AS_HELP_STRING([--enable-debug-build],
		[configure Makefile to enable various debugging flags])],
	[AS_CASE(["x$enableval"],
		["xyes"],[_ENABLE_DEBUG_BUILD=1],
		["xno"],[_ENABLE_DEBUG_BUILD=0])])
# value for both Makefile conditionals as well as the AM_COND_IF macro below
AM_CONDITIONAL([ENABLE_DEBUG_BUILD],
	[test "x$_ENABLE_DEBUG_BUILD" != x0])
AM_COND_IF([ENABLE_DEBUG_BUILD],
	[AC_MSG_NOTICE([Enabling various debugging flags])
         my_CFLAGS="$my_CFLAGS -DDEBUG -DDEVEL"
         my_CPPFLAGS="$my_CPPFLAGS -DDEBUG -DDEVEL"
         my_CXXFLAGS="$my_CXXFLAGS -DDEBUG -DDEVEL"],
	[AC_MSG_NOTICE([Not enabling various debugging flags])])
# a define-flag into config.h just in case some C code would need it later on?
AC_DEFINE_UNQUOTED([ENABLE_DEBUG_BUILD],   [$_ENABLE_DEBUG_BUILD],
        [Flag mostly for Makefile: enable various debugging flags (0=no)?])

ci_tests=false
AC_ARG_ENABLE(ci-tests,
	[AS_HELP_STRING([--enable-ci-tests],
		[turn on build of CI test programs along with "all"])],
	[AS_CASE(["x$enableval"],
		["xyes"],[ci_tests=true],
		["xno"],[ci_tests=false],
		[*],[AC_MSG_ERROR([bad value ${enableval} for --enable-ci-tests])])])
AM_CONDITIONAL([ENABLE_CI_TESTS], [test "x${ci_tests}" = xtrue])

# Google Thread Sanitizer; see
#   http://code.google.com/p/thread-sanitizer/wiki
# Note that TSAN incurs 5x-15x slowdown, so it is off by default
# As of GCC4.8, TSAN is only available for 64-bit Linux
# As of GCC4.9, either TSAN or ASAN may be used, not both
_CFLAGS_TSAN="-fsanitize=thread -fPIE -pie"
_LDFLAGS_TSAN="-pie -ltsan -shared"
_ENABLE_DEBUG_TSAN=0
#_ENABLE_DEBUG_TSAN=$ENABLE_DEBUG_BUILD
AC_ARG_ENABLE(thread-sanitizer,
	[AS_HELP_STRING([--enable-thread-sanitizer],
		[configure Makefile to enable Thread Sanitizer])],
	[AS_CASE(["x$enableval"],
		["xyes"],[_ENABLE_DEBUG_TSAN=1],
		["xno"],[_ENABLE_DEBUG_TSAN=0])])
AS_IF([test "x$_ENABLE_DEBUG_TSAN" != x0],
        [my_save_cflags="$CFLAGS"
         my_save_ldflags="$LDFLAGS"
         AC_MSG_CHECKING([whether CC supports Thread Sanitizer: CFLAGS='$_CFLAGS_TSAN' LDFLAGS='$_LDFLAGS_TSAN'])
         CFLAGS="$_CFLAGS_TSAN -Werror"
         LDFLAGS="$_LDFLAGS_TSAN -Werror"
         AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
            [AC_MSG_RESULT([yes])],
            [AC_MSG_RESULT([no])
             AC_MSG_NOTICE([NOTE: Thread Sanitizer was requested but support was not detected, DISABLING])
             _ENABLE_DEBUG_TSAN=0])
         LDFLAGS="$my_save_ldflags"
         CFLAGS="$my_save_cflags"]
)
# value for both Makefile conditionals as well as the AM_COND_IF macro below
AM_CONDITIONAL([ENABLE_DEBUG_TSAN],
	[test "x$_ENABLE_DEBUG_TSAN" != x0])
AM_COND_IF([ENABLE_DEBUG_TSAN],
	[AC_MSG_NOTICE([Enabling Thread Sanitizer])
         my_LDFLAGS="$my_LDFLAGS $_LDFLAGS_TSAN"
         my_CFLAGS="$my_CFLAGS $_CFLAGS_TSAN"
         my_CPPFLAGS="$my_CPPFLAGS $_CFLAGS_TSAN"
         my_CXXFLAGS="$my_CXXFLAGS $_CFLAGS_TSAN"],
	[AC_MSG_NOTICE([Not enabling Thread Sanitizer])])
# a define-flag into config.h just in case some C code would need it later on?
AC_DEFINE_UNQUOTED([ENABLE_DEBUG_TSAN],   [$_ENABLE_DEBUG_TSAN],
        [Flag mostly for Makefile: enable Thread Sanitizer (0=no)?])

# Google Address Sanitizer and Leak Sanitizer; see
#   http://code.google.com/p/address-sanitizer/wiki
# As of GCC4.9, either TSAN or ASAN+LSAN may be used, not both at once.
# If TSan is off, then by default ASan is (or not) enabled if Debug is (or not)
# It can be manually enabled for "production" builds as well, just 2x slowdown
_CFLAGS_ASAN="-fsanitize=address -fno-omit-frame-pointer"
_LDFLAGS_ASAN="-lasan -llsan -shared"
AS_IF([test "x$_ENABLE_DEBUG_TSAN" = x0],
        [_ENABLE_DEBUG_ASAN=$ENABLE_DEBUG_BUILD],
        [_ENABLE_DEBUG_ASAN=0])

# TODO: Currently CI tests fail with ASan in place because our code is buggy
# Until this is fixed, default builds are without ASAN, it can be enabled on
# the `configure` command-line explicitly.
_ENABLE_DEBUG_ASAN=0

AC_ARG_ENABLE(address-sanitizer,
	[AS_HELP_STRING([--enable-address-sanitizer],
		[configure Makefile to enable Address Sanitizer])],
	[AS_CASE(["x$enableval"],
		["xyes"],[_ENABLE_DEBUG_ASAN=1],
		["xno"],[_ENABLE_DEBUG_ASAN=0])])
AS_IF([test "x$_ENABLE_DEBUG_ASAN" != x0],
        [my_save_cflags="$CFLAGS"
         my_save_ldflags="$LDFLAGS"
         AC_MSG_CHECKING([whether CC supports Address Sanitizer: CFLAGS='$_CFLAGS_ASAN' LDFLAGS='$_LDFLAGS_ASAN'])
         CFLAGS="$_CFLAGS_ASAN -Werror"
         LDFLAGS="$_LDFLAGS_ASAN -Werror"
         AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
            [AC_MSG_RESULT([yes])],
            [AC_MSG_RESULT([no])
             AC_MSG_NOTICE([NOTE: Address Sanitizer was requested but support was not detected, DISABLING])
             _ENABLE_DEBUG_ASAN=0])
         LDFLAGS="$my_save_ldflags"
         CFLAGS="$my_save_cflags"]
)
# value for both Makefile conditionals as well as the AM_COND_IF macro below
# note that in the Makefile we (should?) conditionally link against libasan;
# at least this holds for our built SO libraries like bios_web.so
AM_CONDITIONAL([ENABLE_DEBUG_ASAN],
	[test "x$_ENABLE_DEBUG_ASAN" != x0])
AM_COND_IF([ENABLE_DEBUG_ASAN],
	[AC_MSG_NOTICE([Enabling Address Sanitizer])
         my_LDFLAGS="$my_LDFLAGS $_LDFLAGS_ASAN"
         my_CFLAGS="$my_CFLAGS $_CFLAGS_ASAN"
         my_CPPFLAGS="$my_CPPFLAGS $_CFLAGS_ASAN"
         my_CXXFLAGS="$my_CXXFLAGS $_CFLAGS_ASAN"],
	[AC_MSG_NOTICE([Not enabling Address Sanitizer])])
# a define-flag into config.h just in case some C code would need it later on?
AC_DEFINE_UNQUOTED([ENABLE_DEBUG_ASAN],   [$_ENABLE_DEBUG_ASAN],
        [Flag mostly for Makefile: enable Address Sanitizer (0=no)?])

### At least for some GCC versions, only one of these sanitizers may be used
AS_IF([test "x$_ENABLE_DEBUG_TSAN" != x0 -a "x$_ENABLE_DEBUG_ASAN" != x0],
        [AC_MSG_CHECKING([whether -fsanitize=address is incompatible with -fsanitize=thread for this compiler])
         my_save_cflags="$CFLAGS"
         my_save_ldflags="$LDFLAGS"
         CFLAGS="$_CFLAGS_ASAN $_CFLAGS_TSAN -Werror"
         LDFLAGS="$_LDFLAGS_ASAN $_LDFLAGS_TSAN -Werror"
         AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
            [AC_MSG_RESULT([no])],
            [AC_MSG_RESULT([yes])
             AC_MSG_ERROR([You can only enable either Address Sanitizer or Thread Sanitizer, sorry])])
         LDFLAGS="$my_save_ldflags"
         CFLAGS="$my_save_cflags"]
)

# display source/build paths of interest to us
AC_MSG_NOTICE([Determining source code paths:])
AC_MSG_NOTICE(AS_HELP_STRING([Value of srcdir:], ['$srcdir']))
_curdir_abs="`pwd`"
AC_MSG_NOTICE(AS_HELP_STRING([Value of _curdir_abs:], ['$_curdir_abs']))
AS_CASE(["x$srcdir"],
    [x/*], [_srcdir_abs="$srcdir"],
    [*],   [_srcdir_abs="$_curdir_abs/$srcdir"
	    AS_IF([test -d "$_srcdir_abs"],
		  [_srcdir_abs="`cd "$_srcdir_abs" && pwd || echo "$_srcdir_abs"`"])
])
AC_MSG_NOTICE(AS_HELP_STRING([Value of _srcdir_abs:], ['$_srcdir_abs']))

# This script is part of the sources, so we expect it to be available
# We call it to prepare a file with Git workspace metadata, if available;
# alternately a copy of the file may have been git-archive'd to tarball
# with sources that so are not a Git workspace (repository clone) anymore.
# This includes the case of "make dist(check)" where a copy of the file
# is passed at some stage via Makefile's EXTRA_DIST.
GDETAILSH="$_srcdir_abs/tools/git_details.sh"
AS_IF([test -s "$GDETAILSH" -a -x "$GDETAILSH" -a -r "$GDETAILSH"],
	[AC_MSG_NOTICE([Found git_details.sh: $GDETAILSH])],
	[AC_MSG_ERROR([cannot find required git_details.sh])])

# If we have an (older) .git_details file, and a clean out-of-tree build,
# then prepopulate the build directory with a copy
AS_IF([test x"$_srcdir_abs" != x"$_curdir_abs" -a \
	  -s "$_srcdir_abs/.git_details" -a \
	! -s "$_curdir_abs/.git_details"],
	[AC_MSG_NOTICE([Pre-populating .git_details in the out-of-tree build area from source workspace...])
	 cp -pf "$_srcdir_abs/.git_details" "$_curdir_abs/.git_details"
])

AS_IF([test -s "$_curdir_abs/.git_details.tmp"],
	[rm -f "$_curdir_abs/.git_details.tmp"])

AS_IF([test -d "$_srcdir_abs/.git"],
	[AC_MSG_NOTICE([Getting current Git workspace attributes...])
	 # The script would need a git... can detect one or use the set envvar
	 # So let's help it if we can
	 AC_PATH_PROGS([GIT], [git])
	 AS_IF([test -n "$GIT" -a -x "$GIT"], [export GIT])
	 AC_PATH_PROGS([DATE], [gdate date])
	 AS_IF([test -n "$DATE" -a -x "$DATE"], [export DATE])
	 AS_IF([(cd "$_srcdir_abs" && $GDETAILSH ) > "$_curdir_abs/.git_details.tmp"],
		[AS_IF([test -s "$_curdir_abs/.git_details.tmp"],
		 [AS_IF([test -s "$_curdir_abs/.git_details"],
			[AS_IF([diff -bu "$_curdir_abs/.git_details" \
				             "$_curdir_abs/.git_details.tmp" | \
				    egrep -v '^(\-\-\-|\+\+\+|[ @]|[\+\-]PACKAGE_BUILD_TSTAMP)' \
				    >/dev/null 2>&1],
				[AC_MSG_NOTICE([Got new workspace metadata to replace the old copy])
				 mv -f "$_curdir_abs/.git_details.tmp" "$_curdir_abs/.git_details"],
				[AC_MSG_NOTICE([Got new workspace metadata but no significant changes against the old copy])]
			 )],
			[AC_MSG_NOTICE([Got new workspace metadata and had no old stashed away, using freely])
			 mv -f "$_curdir_abs/.git_details.tmp" "$_curdir_abs/.git_details"])
		 ], [AC_MSG_NOTICE([No Git metadata was detected (empty output)])
			 rm -f "$_curdir_abs/.git_details.tmp"])
			], [AC_MSG_NOTICE([No Git metadata was detected (script errored out)])
				rm -f "$_curdir_abs/.git_details.tmp"])
	], [AC_MSG_NOTICE([No Git workspace was detected ($_srcdir_abs/.git directory missing)])
])

# From old copy or a new generation, we may have the file with metadata...
# We no longer cook (most of) this data into config.h because it does get
# obsolete quickly during development. Rather, the build products that
# need (test, display) this information should use or convert the file.
# The Makefile takes care to keep it current as long as possible.
# Also, for display in the end of execution, try to "source" the values.
AS_IF([test -s "$_curdir_abs/.git_details"],
	[AC_MSG_NOTICE([Getting Git details into PACKAGE_* macros...])
	 source "$_curdir_abs/.git_details" && for V in \
		PACKAGE_GIT_ORIGIN PACKAGE_GIT_BRANCH PACKAGE_GIT_TAGGED \
		PACKAGE_GIT_TSTAMP PACKAGE_GIT_HASH_S PACKAGE_GIT_HASH_L \
		PACKAGE_GIT_STATUS PACKAGE_BUILD_TSTAMP \
		PACKAGE_GIT_TSTAMP_ISO8601 PACKAGE_BUILD_TSTAMP_ISO8601 \
		PACKAGE_BUILD_HOST_UNAME PACKAGE_BUILD_HOST_NAME \
		PACKAGE_BUILD_HOST_OS ; do
			eval $V='$'${V}_ESCAPED || eval $V=""
		done
	 AS_IF([test -z "$PACKAGE_GIT_HASH_S"], [source "$_curdir_abs/.git_details"])
	 AS_IF([test -n "$PACKAGE_GIT_HASH_S" -a x"$PACKAGE_GIT_HASH_S" != x'""' \
	          -a -n "$PACKAGE_GIT_TSTAMP" -a x"$PACKAGE_GIT_TSTAMP" != x'""'],
		[AC_MSG_NOTICE([Setting PACKAGE_STRING and PACKAGE_VERSION to include Git hash '$PACKAGE_GIT_HASH_S' and commit timestamp '$PACKAGE_GIT_TSTAMP'...])
		 PACKAGE_STRING="$PACKAGE_STRING.$PACKAGE_GIT_TSTAMP~$PACKAGE_GIT_HASH_S"
		 PACKAGE_VERSION="$PACKAGE_VERSION.$PACKAGE_GIT_TSTAMP~$PACKAGE_GIT_HASH_S"
		 _FIX_PACKAGE_STRING=y
		 _FIX_PACKAGE_VERSION=y
		 ### Rather don't fix the VERSION as it influences the
		 ### tarball name in "make distcheck", install paths, etc.
		 #VERSION="$VERSION.$PACKAGE_GIT_TSTAMP~$PACKAGE_GIT_HASH_S"
		 #_FIX_VERSION=y
	 ])
	], [AC_MSG_NOTICE([No Git details detected ($_curdir_abs/.git_details file missing), they will be missing from REST API and other reports])
])

### Allow to revise the packaging strings in config.h
m4_define([__AC_UNDEFINE],[_AC_DEFINE([#ifdef $1
 #undef $1
#endif])])

# General substitutions for all these packaging variables
AS_IF([test x"$_FIX_PACKAGE_NAME" = xy],
	[AC_MSG_NOTICE([Overriding PACKAGE_NAME='$PACKAGE_NAME'])
	 __AC_UNDEFINE([PACKAGE_NAME])
	 AC_DEFINE_UNQUOTED([PACKAGE_NAME],
	    ["$PACKAGE_NAME"],
	    [Packaging metadata: distro source code name])
])
AS_IF([test x"$_FIX_PACKAGE_VERSION" = xy],
	[AC_MSG_NOTICE([Overriding PACKAGE_VERSION='$PACKAGE_VERSION'])
	 __AC_UNDEFINE([PACKAGE_VERSION])
	 AC_DEFINE_UNQUOTED([PACKAGE_VERSION],
	    ["$PACKAGE_VERSION"],
	    [Packaging metadata: distro source code/packaged release version])
])
AS_IF([test x"$_FIX_VERSION" = xy],
	[AC_MSG_NOTICE([Overriding VERSION='$VERSION'])
	 __AC_UNDEFINE([VERSION])
	 AC_DEFINE_UNQUOTED([VERSION],
	    ["$VERSION"],
	    [Version number of package])
])
AS_IF([test x"$_FIX_PACKAGE_STRING" = xy],
	[AC_MSG_NOTICE([Overriding PACKAGE_STRING='$PACKAGE_STRING'])
	 __AC_UNDEFINE([PACKAGE_STRING])
	 AC_DEFINE_UNQUOTED([PACKAGE_STRING],
	    ["$PACKAGE_STRING"],
	    [Packaging metadata: distro source code name+version])
])
AS_IF([test x"$_FIX_PACKAGE_URL" = xy],
	[AC_MSG_NOTICE([Overriding PACKAGE_URL='$PACKAGE_URL'])
	 __AC_UNDEFINE([PACKAGE_URL])
	 AC_DEFINE_UNQUOTED([PACKAGE_URL],
	    ["$PACKAGE_URL"],
	    [Packaging metadata: distro contact (website)])
])
AS_IF([test x"$_FIX_PACKAGE_BUGREPORT" = xy],
	[AC_MSG_NOTICE([Overriding PACKAGE_BUGREPORT='$PACKAGE_BUGREPORT'])
	 __AC_UNDEFINE([PACKAGE_BUGREPORT])
	 AC_DEFINE_UNQUOTED([PACKAGE_BUGREPORT],
	    ["$PACKAGE_BUGREPORT"],
	    [Packaging metadata: distro contact (email/tracker)])
])
AC_MSG_NOTICE([Overriding PACKAGE_VENDOR='$PACKAGE_VENDOR'])
AC_DEFINE_UNQUOTED([PACKAGE_VENDOR],
	    ["$PACKAGE_VENDOR"],
	    [Packaging metadata: name of the entity which distributes this build of the package])

# toolchain - after all those definitions and redefinitions that can be broken
# verify that needed tools exist and work: at least for AC_PROG_CXX we had a
# surprise that it did not fail a ./configure run if no C++ compiler was found
unset _TMP_GCC
unset _TMP_GXX

AC_PROG_CC
AS_IF([test -n "$CC"], [], AC_MSG_ERROR([cannot find C compiler ($CC)]))
AC_PATH_PROGS([_TMP_GCC], [`basename $CC` gcc], [])
AS_IF([test -n "$_TMP_GCC" -a -x "$_TMP_GCC"], [], AC_MSG_ERROR([cannot find C compiler]))

AM_PROG_CC_C_O
AC_PROG_CXX
AS_IF([test -n "$CXX"], [], AC_MSG_ERROR([cannot find C++ compiler ($CXX)]))
AC_PATH_PROGS([_TMP_GXX], [`basename $CXX` g++], [])
AS_IF([test -n "$_TMP_GXX" -a -x "$_TMP_GXX"], [], AC_MSG_ERROR([cannot find C++ compiler]))

AC_MSG_CHECKING([presence of ccache])
AS_IF([which ccache >/dev/null 2>&1], [
	AC_MSG_RESULT([yes])
	AC_MSG_NOTICE([Detected presence of ccache in PATH])
	AS_IF([diff "`which ccache`" "$_TMP_GCC"], [AC_MSG_NOTICE([The C compiler program ($_TMP_GCC) is a symlink to ccache])], [AC_MSG_WARN([The C compiler program ($_TMP_GCC) is not a symlink to ccache, you miss out on build speed])])
	AS_IF([diff "`which ccache`" "$_TMP_GXX"], [AC_MSG_NOTICE([The C++ compiler program ($_TMP_GXX) is a symlink to ccache])], [AC_MSG_WARN([The C++ compiler program ($_TMP_GXX) is not a symlink to ccache, you miss out on build speed])])
	],[AC_MSG_RESULT([no])])

unset _TMP_GCC
unset _TMP_GXX

AC_PROG_INSTALL
AC_PROG_LN_S
PKG_PROG_PKG_CONFIG

# set flags CFLAGS
my_CFLAGS="$my_CFLAGS \
-std=c11 \
-Wall \
-Wextra \
-Wunused-function \
-fstack-protector \
-fstack-protector-all \
"

# set flags CXXFLAGS
my_CXXFLAGS="$my_CXXFLAGS \
-std=c++11 \
-Wall \
-Wextra \
-Wunused-function \
-fstack-protector \
-fstack-protector-all \
"

# set flags CPPFLAGS
#my_CPPFLAGS="$my_CPPFLAGS \
#"

# set flags LDFLAGS
#my_LDFLAGS="$my_LDFLAGS \
#"

# Early checks for library functions that may be picky - optionally
# enable macros that may be further needed for compilation of the project
unset _CC_ORIG
AS_CASE(["x$CC"],
	[xgcc*], [_CC_ORIG="$CC"
		CC="$CC -Werror $my_CFLAGS"]
	)
AS_IF([test -n "$_CC_ORIG"], [AC_MSG_NOTICE([NOTE: Temporarily made the compiler more picky: $CC])])
AC_CHECK_HEADERS([string.h], [], AC_MSG_ERROR([cannot find required header string.h]))
# Note: dummy code below is wrong and leaky, but we just want it compiled
my_try_includes='#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>'
my_try_code='char *s; int i; struct tm t;
s = strdup("123"); if (s) {;}
s = strndup("123",4); if (s) {;}
i = setenv("TZ", "", 1); if (i) {;}
i = unsetenv("TZ"); if (i) {;}
i = asprintf(&s, "%d", 1); if (i) {;}
s = strptime("1970-01-01", "%s", &t); if (t.tm_sec) {;}
tzset();'
my_msg_funcnames='strdup() strndup() asprintf() setenv() unsetenv() tzset()'
AC_MSG_CHECKING([${my_msg_funcnames} with your default env])
AC_TRY_LINK([${my_try_includes}], [${my_try_code}],
	[AC_DEFINE([HAVE_STRDUP], [1], [Define to 1 if you have the `strdup' function.])
	 AC_DEFINE([HAVE_STRNDUP], [1], [Define to 1 if you have the `strndup' function.])
	 AC_DEFINE([HAVE_ASPRINTF], [1], [Define to 1 if you have the `asprintf' function.])
	 AC_DEFINE([HAVE_TZSET], [1], [Define to 1 if you have the `tzset' function.])
	 AC_DEFINE([HAVE_SETENV], [1], [Define to 1 if you have the `setenv' function.])
	 AC_DEFINE([HAVE_UNSETENV], [1], [Define to 1 if you have the `unsetenv' function.])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])
	 AC_MSG_CHECKING([${my_msg_funcnames} with -D_GNU_SOURCE])
	 my_save_CFLAGS="$my_CFLAGS"
	 my_save_CPPFLAGS="$my_CPPFLAGS"
	 my_save_CXXFLAGS="$my_CXXFLAGS"
	 my_CFLAGS="$my_save_CFLAGS -D_GNU_SOURCE"
	 my_CPPFLAGS="$my_save_CPPFLAGS -D_GNU_SOURCE"
	 my_CXXFLAGS="$my_save_CXXFLAGS -D_GNU_SOURCE"
	 AC_DEFINE([_GNU_SOURCE], [1], [Define this if needed to compile POSIX code])
	 AC_TRY_LINK([${my_try_includes}], [${my_try_code}],
		[AC_DEFINE([HAVE_STRDUP], [1], [Define to 1 if you have the `strdup' function.])
		 AC_DEFINE([HAVE_STRNDUP], [1], [Define to 1 if you have the `strndup' function.])
		 AC_DEFINE([HAVE_ASPRINTF], [1], [Define to 1 if you have the `asprintf' function.])
		 AC_DEFINE([HAVE_TZSET], [1], [Define to 1 if you have the `tzset' function.])
		 AC_DEFINE([HAVE_SETENV], [1], [Define to 1 if you have the `setenv' function.])
		 AC_DEFINE([HAVE_UNSETENV], [1], [Define to 1 if you have the `unsetenv' function.])
		 AC_MSG_RESULT([yes])],
		[AC_MSG_RESULT([no])
		 AC_MSG_CHECKING([${my_msg_funcnames} with -D__EXTENSIONS__ also enabled])
		 my_CFLAGS="$my_save_CFLAGS -D_GNU_SOURCE -D__EXTENSIONS__"
		 my_CPPFLAGS="$my_save_CPPFLAGS -D_GNU_SOURCE -D__EXTENSIONS__"
		 my_CXXFLAGS="$my_save_CXXFLAGS -D_GNU_SOURCE -D__EXTENSIONS__"
		 AC_DEFINE([__EXTENSIONS__], [1], [Define this if needed to compile POSIX code])
		 AC_TRY_LINK([${my_try_includes}], [${my_try_code}],
			[AC_DEFINE([HAVE_STRDUP], [1], [Define to 1 if you have the `strdup' function.])
			 AC_DEFINE([HAVE_STRNDUP], [1], [Define to 1 if you have the `strndup' function.])
			 AC_DEFINE([HAVE_ASPRINTF], [1], [Define to 1 if you have the `asprintf' function.])
			 AC_DEFINE([HAVE_TZSET], [1], [Define to 1 if you have the `tzset' function.])
			 AC_DEFINE([HAVE_SETENV], [1], [Define to 1 if you have the `setenv' function.])
			 AC_DEFINE([HAVE_UNSETENV], [1], [Define to 1 if you have the `unsetenv' function.])
			 AC_MSG_RESULT([yes])],
			[AC_MSG_RESULT([no])
			 AC_MSG_CHECKING([${my_msg_funcnames} with -D_XOPEN_SOURCE=700 also enabled])
			 my_CFLAGS="$my_save_CFLAGS -D_GNU_SOURCE -D__EXTENSIONS__ -D_XOPEN_SOURCE=700"
			 my_CPPFLAGS="$my_save_CPPFLAGS -D_GNU_SOURCE -D__EXTENSIONS__ -D_XOPEN_SOURCE=700"
			 my_CXXFLAGS="$my_save_CXXFLAGS -D_GNU_SOURCE -D__EXTENSIONS__ -D_XOPEN_SOURCE=700"
			 AC_DEFINE([_XOPEN_SOURCE], [700], [Define this if needed to compile POSIX code])
			 AC_TRY_LINK([${my_try_includes}], [${my_try_code}],
				[AC_DEFINE([HAVE_STRDUP], [1], [Define to 1 if you have the `strdup' function.])
				 AC_DEFINE([HAVE_STRNDUP], [1], [Define to 1 if you have the `strndup' function.])
				 AC_DEFINE([HAVE_ASPRINTF], [1], [Define to 1 if you have the `asprintf' function.])
				 AC_DEFINE([HAVE_TZSET], [1], [Define to 1 if you have the `tzset' function.])
				 AC_DEFINE([HAVE_SETENV], [1], [Define to 1 if you have the `setenv' function.])
				 AC_DEFINE([HAVE_UNSETENV], [1], [Define to 1 if you have the `unsetenv' function.])
				 AC_MSG_RESULT([yes])],
				[AC_MSG_RESULT([no])
				 AC_MSG_ERROR([cannot find at least one of: ${my_msg_funcnames}, not even with -D_GNU_SOURCE -D__EXTENSIONS__ -D_XOPEN_SOURCE=700])])]
			)]
		)]
	)
AS_IF([test -n "$_CC_ORIG"], [
	CC="$_CC_ORIG"
	unset _CC_ORIG
	AC_MSG_NOTICE([NOTE: Restored default compiler settings: $CC])])

# Checks for the header files
AC_CHECK_HEADERS([magic.h], [], AC_MSG_ERROR([libmagic: cannot find required header magic.h]))
AC_CHECK_LIB(
             [magic],
             [magic_open],
             [],
             [AC_MSG_ERROR([function magic_open() is not available, this is not the libmagic we are looking for])]
             )
SAVE_LIBS="$LIBS"
LIBS="$LIBS -lmagic"
AC_MSG_CHECKING([linkability with libmagic])
AC_TRY_LINK([#include <magic.h>], [magic_t magic_cookie = magic_open(MAGIC_NONE); if (!magic_cookie) {;}; magic_close(magic_cookie);],
             [AC_MSG_RESULT([ok])],
             [AC_MSG_ERROR([function magic_open() is not available, this is not the libmagic we are looking for])]
             )
LIBS="$SAVE_LIBS"

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([tntdb.h],
                 [],
                 AC_MSG_ERROR([cannot find required header tntdb.h]))
AC_CHECK_HEADER([tnt/tntnet.h], , AC_MSG_ERROR([tntnet headers not found]))
AC_LANG_POP([C++])

AC_MSG_NOTICE([Using PKG_CONFIG_PATH='$PKG_CONFIG_PATH'])
PKG_CHECK_MODULES([LIBCIDR], [libcidr], [], [AC_MSG_ERROR([cannot find libcidr])])
PKG_CHECK_MODULES([CXXTOOLS], [cxxtools >= 2.2.0], [], [AC_MSG_ERROR([cannot find cxxtools >= 2.2.0])])
PKG_CHECK_MODULES([LIBSASL2], [libsasl2 >= 2.1.0], [], [AC_MSG_ERROR([cannot find sasl 2 library >= 2.1.0])])
PKG_CHECK_MODULES([LIBSODIUM], [libsodium >= 0.7.0], [], [AC_MSG_ERROR([cannot find libsodium >= 0.7.0])])
PKG_CHECK_MODULES([LIBZMQ], [libzmq >= 3.0], [], [AC_MSG_ERROR([cannot find libzmq version >= 3])])
PKG_CHECK_MODULES([LIBCZMQ], [libczmq >= 3.0], [], [AC_MSG_ERROR([cannot find libczmq version >= 3])])
PKG_CHECK_MODULES([LIBMLM], [libmlm >= 0.1.0], [], [AC_MSG_ERROR([cannot find malamute library >= 0.1.0])])
PKG_CHECK_MODULES([LIBFTYPROTO], [libfty_proto >= 1.0.0], [], AC_MSG_ERROR([cannot find libfty_proto library >= 1.0.0]))

AC_MSG_CHECKING([presence of augtool])
AS_IF([which augtool >/dev/null 2>&1], [AC_MSG_RESULT([yes])], [
	AC_MSG_RESULT([no])
	AC_MSG_WARN([some CI tests (inspection and modification of OS configuration) can fail on this host due to lack of augeas-tools; but this is not fatal for compilation itself])
])

dnl ------ it was suggested by autoscan
dnl TODO  I am not sure whether we need this. TODO MVY Please look at this.
dnl Checks for libraries.
dnl FIXME: Replace `main' with a function in `-lzmq':
dnl AC_CHECK_LIB([zmq], [main])


AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdint.h stdlib.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset socket])

dnl--- end of the section suggested by autoscan

# Checks for optional programs. 

# astyle
dnl ASTYLE would have the absolute path, use AC_CHECK_PROGS for relative
AC_PATH_PROGS([ASTYLE], [astyle])
AM_CONDITIONAL([HAVE_ASTYLE], [test -n "${ASTYLE}"])

# sed is great!
AC_CHECK_PROGS([SED], [sed])
# TNTnet preprocesor
AC_CHECK_PROGS([ECPPC], [ecppc])
# TNTnet
AC_CHECK_PROGS([TNTNET], [tntnet])
AS_IF([test -z "$TNTNET" -o -z "$ECPPC"],[
  AC_MSG_ERROR([TNTnet not found!])
])

# sourced from an m4/*.m4 include file during configure script compilation
gl_VISIBILITY
BS_CHECK_SASLAUTHD_MUX

# see daemon(7)
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
     [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
     def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
         [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
                [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
          with_systemdsystemunitdir=no],
         [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
        [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

# same logic as above, for presets (list of svcs enabled/disabled by default)
AC_ARG_WITH([systemdsystempresetdir],
     [AS_HELP_STRING([--with-systemdsystempresetdir=DIR], [Directory for systemd preset files])],,
     [with_systemdsystempresetdir=auto])
AS_IF([test "x$with_systemdsystempresetdir" = "xyes" -o "x$with_systemdsystempresetdir" = "xauto"], [
     def_systemdsystempresetdir=$($PKG_CONFIG --variable=systemdsystempresetdir systemd)

     AS_IF([test "x$def_systemdsystempresetdir" = "x"],
         [AS_IF([test "x$with_systemdsystempresetdir" = "xyes"],
                [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
          with_systemdsystempresetdir=no],
         [with_systemdsystempresetdir="$def_systemdsystempresetdir"])])
AS_IF([test "x$with_systemdsystempresetdir" != "xno"],
        [AC_SUBST([systemdsystempresetdir], [$with_systemdsystempresetdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystempresetdir" != "xno"])

# Note: default path for "systemd-tmpfiles" packaged configs is currently
# hardcoded per http://www.freedesktop.org/software/systemd/man/tmpfiles.d.html
#systemdtmpfilesdir =    /usr/lib/tmpfiles.d/
AC_ARG_WITH([systemdtmpfilesdir],
     [AS_HELP_STRING([--with-systemdtmpfilesdir=DIR], [Directory for systemd preset files])],,
     [with_systemdtmpfilesdir=auto])
AS_IF([test "x$with_systemdtmpfilesdir" = "xyes" -o "x$with_systemdtmpfilesdir" = "xauto"], [
     def_systemdtmpfilesdir=$($PKG_CONFIG --variable=systemdtmpfilesdir systemd)
     # NOTE: The variable queried above might not exist at all
     AS_IF([test "x$def_systemdtmpfilesdir" = "x"],
         [with_systemdtmpfilesdir="/usr/lib/tmpfiles.d"],
         [with_systemdtmpfilesdir="$def_systemdtmpfilesdir"])])
AS_IF([test "x$with_systemdtmpfilesdir" != "xno"],
        [AC_SUBST([systemdtmpfilesdir], [$with_systemdtmpfilesdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdtmpfilesdir" != "xno"])

AC_SUBST([my_CFLAGS])
AC_SUBST([my_CPPFLAGS])
AC_SUBST([my_CXXFLAGS])
AC_SUBST([my_LDFLAGS])

# Snatched and adapted from libpng:
# http://www.opensource.apple.com/source/X11libs/X11libs-40/libpng/libpng-1.2.35/configure.ac
AC_MSG_CHECKING([if libraries can be versioned])
GLD="`$LD --help < /dev/null 2>/dev/null | grep version-script`"
AS_IF([test -n "$GLD"],
        [have_ld_version_script=yes
         AC_MSG_RESULT(yes)],
        [have_ld_version_script=no
         AC_MSG_RESULT(no)
         AC_MSG_WARN(*** You have not enabled versioned symbols.)
])
AM_CONDITIONAL(HAVE_LD_VERSION_SCRIPT, test "$have_ld_version_script" = "yes")
AS_IF([test "$have_ld_version_script" = "yes"],
        [AC_MSG_CHECKING([for symbol prefix])
         SYMBOL_PREFIX=`echo "PREFIX=__USER_LABEL_PREFIX__" \
                   | ${CPP-${CC-gcc} -E} - 2>&1 \
                   | ${EGREP-grep} "^PREFIX=" \
                   | ${SED-sed} "s:^PREFIX=::"`
         AC_SUBST(SYMBOL_PREFIX)
         AC_MSG_RESULT($SYMBOL_PREFIX)
         CXXFLAG_VISIBILITY=""
         gl_VISIBILITY
         AS_CASE(["$CFLAG_VISIBILITY"],
            [*-fvisibility-inlines-hidden*],[
                CXXFLAG_VISIBILITY="$CFLAG_VISIBILITY"],
            [*-fvisibility=hidden*],[
                CXXFLAG_VISIBILITY="$CFLAG_VISIBILITY -fvisibility-inlines-hidden"])
        AC_SUBST(CXXFLAG_VISIBILITY)
])


AC_MSG_NOTICE([
Summary:

        Version:                 ${PACKAGE_NAME} ${VERSION}

        Git origin URL:          ${PACKAGE_GIT_ORIGIN}
        Git branch:              ${PACKAGE_GIT_BRANCH}
        Git tag:                 ${PACKAGE_GIT_TAGGED}
        Git last-commit tstamp:  ${PACKAGE_GIT_TSTAMP} ${PACKAGE_GIT_TSTAMP_ISO8601}
        Git last-commit hash(S): ${PACKAGE_GIT_HASH_S}
        Git last-commit hash(L): ${PACKAGE_GIT_HASH_L}
        Git status (uncommitted files):	'${PACKAGE_GIT_STATUS}'

        Build-host 'uname -a':   ${PACKAGE_BUILD_HOST_UNAME}
        Build-host system name:  ${PACKAGE_BUILD_HOST_NAME}
        Build-host OS/kernel:    ${PACKAGE_BUILD_HOST_OS}
        Build-host timestamp:    ${PACKAGE_BUILD_TSTAMP} ${PACKAGE_BUILD_TSTAMP_ISO8601}

        prefix:                  ${prefix}
        sysconf dir:             ${sysconfdir}
        datarootdir:             ${datarootdir}
        includedir:              ${includedir}
        lib dir:                 ${libdir}

        astyle:                  ${ASTYLE:-none}
        ecppc:                   ${ECPPC:-none}

        ENABLE_CI_TESTS:         ${ci_tests}

        CFLAGS:                  ${my_CFLAGS} ${CFLAGS}
        CXXFLAGS:                ${my_CXXFLAGS} ${CXXFLAGS}
        CPPFLAGS:                ${OUR_CPPFLAGS} ${CPPFLAGS}
        LDFLAGS:                 ${OUR_LDFLAGS} ${LDFLAGS}
        LIBZMQ_CFLAGS:           ${LIBZMQL_CFLAGS}
        LIBZMQ_LIBS:             ${LIBZMQ_LIBS}
        LIBCZMQ_CFLAGS:          ${LIBCZMQ_CFLAGS}
        LIBCZMQ_LIBS:            ${LIBCZMQ_LIBS}
        LIBCIDR_CFLAGS:          ${LIBCIDR_CFLAGS}
        LIBCIDR_LIBS:            ${LIBCIDR_LIBS}
        LIBNUTCLIENT_CFLAGS:     ${LIBNUTCLIENT_CFLAGS}
        LIBNUTCLIENT_LIBS:       ${LIBNUTCLIENT_LIBS}
        CXXTOOLS_CFLAGS:         ${CXXTOOLS_CFLAGS}
        CXXTOOLS_LIBS:           ${CXXTOOLS_LIBS}
        LIBSODIUM_CFLAGS:        ${LIBSODIUM_CFLAGS}
        LIBSODIUM_LIBS:          ${LIBSODIUM_LIBS}
        LIBSASL2_CFLAGS:         ${LIBSASL2_CFLAGS}
        LIBSASL2_LIBS:           ${LIBSASL2_LIBS}
        LIBMLM_CFLAGS:           ${LIBMLM_CFLAGS}
        LIBMLM_LIBS:             ${LIBMLM_LIBS}
        LIBFTYPROTO_LIBS:        ${LIBFTYPROTO_LIBS}
        SASLAUTHD_MUX:           ${SASLAUTHD_MUX}
        PKG_CONFIG:              ${PKG_CONFIG}
        SYSTEMDSYSTEMUNITDIR:    ${systemdsystemunitdir}
        SYSTEMDSYSTEMPRESETDIR:  ${systemdsystempresetdir}
        SYSTEMDTMPFILESDIR:      ${systemdtmpfilesdir}

        absolute SRC DIR:        ${_srcdir_abs}
        absolute CUR DIR:        ${_curdir_abs}
])

unset _srcdir_abs

AC_OUTPUT
