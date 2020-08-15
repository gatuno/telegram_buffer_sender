AC_INIT([Telegram Buffer Sender], [0.1.1], [], [telegram-buffer-sender])

AC_REVISION([Revision 1])

AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_SRCDIR([src/main.c])

# For debian systems, /usr as default
AC_PREFIX_DEFAULT([/usr])

# Fix paths...
if test "$prefix" = /usr; then
	test "$sysconfdir" = '${prefix}/etc' && sysconfdir=/etc
	test "$localstatedir" = '${prefix}/var' && localstatedir=/var
fi

if test "$prefix" = NONE; then
	test "$sysconfdir" = '${prefix}/etc' && sysconfdir=/etc
	test "$localstatedir" = '${prefix}/var' && localstatedir=/var
fi

# We need Gcc
AC_PROG_CC

AM_PROG_CC_C_O

# Revisar el host
AC_CANONICAL_HOST

# and automake
AM_INIT_AUTOMAKE([-Wall -Werror])

# We need pthreads
AX_PTHREAD([have_pthread=yes], [have_pthread=no])
if test "$have_pthread" = no; then
	AC_MSG_ERROR([this program needs pthreads])
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)

# Translate this program
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

ALL_LINGUAS=""
AC_SUBST(ALL_LINGUAS)

# Necesito openssl
AX_CHECK_OPENSSL([have_openssl=yes], [have_openssl=no])

if test "$have_openssl" = no; then
	AC_MSG_ERROR([this program need openssl library])
fi

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([ po/Makefile.in
                 Makefile
                 src/Makefile
                 data/Makefile
])

AC_OUTPUT
