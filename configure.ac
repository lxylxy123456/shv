# Template from http://www.idryman.org/blog/2016/03/10/autoconf-tutorial-1/

# Must init the autoconf setup
# The first parameter is project name
# second is version number
# third is bug report address
AC_INIT([shv], [1.0])

# Safety checks in case user overwritten --srcdir
AC_CONFIG_SRCDIR([kernel.c])

# Store the auxiliary build tools (e.g., install-sh, config.sub, config.guess)
# in this dir (build-aux)
AC_CONFIG_AUX_DIR([build-aux])

# https://stackoverflow.com/questions/71709907/
AC_CONFIG_HEADERS([include/config.h])

# Init automake, and specify this program use relaxed structures.
# i.e. this program doesn't follow the gnu coding standards, and doesn't have
# ChangeLog, COPYING, AUTHORS, INSTALL, README etc. files.
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Check for C compiler
AC_PROG_CC
AM_PROG_AS
AC_CHECK_PROGS([GRUB_MKRESCUE], [grub2-mkrescue grub-mkrescue])
# We can add more checks in this section

AM_CONDITIONAL([I386], [test x"$host_alias" = x"i686-linux-gnu"])

# Define macros that will be passed to C and assembly files, following:
# https://stackoverflow.com/questions/71709907/
# https://stackoverflow.com/questions/32092644/

# SHV_OPT
AC_ARG_WITH([shv_opt],
            [AS_HELP_STRING([--with-shv-opt=SHV_OPT],
                            [specify SHV options, see shv.h])],
            [shv_opt="$withval"], [shv_opt=0])
AC_DEFINE_UNQUOTED([SHV_OPT],[${shv_opt}],[SHV options])

# I386_PAE
AC_ARG_ENABLE([i386_pae],
              [AS_HELP_STRING([--enable-i386-pae@<:@=yes|no@:>@],
                              [use PAE paging, ignored if not in 32-bit mode])],
              [AS_CASE(${enableval}, [yes], [i386_pae=1], [no], [i386_pae=0],
                       [AC_MSG_ERROR([bad value ${enableval}])])],
              [i386_pae=0])
AC_DEFINE_UNQUOTED([I386_PAE],[${i386_pae}],[Whether use PAE paging in 32-bit])

# DEBUG_VGA
AC_ARG_ENABLE([debug_vga],
              [AS_HELP_STRING([--enable-debug-vga@<:@=yes|no@:>@],
                              [enable VGA debugging])],
              [AS_CASE(${enableval}, [yes], [debug_vga=1], [no], [debug_vga=0],
                       [AC_MSG_ERROR([bad value ${enableval}])])],
              [debug_vga=0])
AC_DEFINE_UNQUOTED([DEBUG_VGA],[${debug_vga}],[Whether use VGA to debug])

# DEBUG_SERIAL
AC_ARG_ENABLE([debug_serial],
              [AS_HELP_STRING([--enable-debug-serial@<:@=yes|no@:>@],
                              [enable serial debugging])],
              [AS_CASE(${enableval}, [yes], [debug_serial=1],
                       [no], [debug_serial=0],
                       [AC_MSG_ERROR([bad value ${enableval}])])],
              [debug_serial=1])
AC_DEFINE_UNQUOTED([DEBUG_SERIAL],[${debug_serial}],
                   [Whether use Serial to debug])

# Tells automake to create a Makefile
# See https://www.gnu.org/software/automake/manual/html_node/Requirements.html
AC_CONFIG_FILES([Makefile])

# Generate the output
AC_OUTPUT
