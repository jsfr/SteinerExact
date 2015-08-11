/* config.h.  Generated automatically by configure.  */
/**********************************************************************

	File:	config.h
	Rev:	a-2
	Date:	02/28/2001

	Copyright (c) 1998, 2001 by David M. Warme

***********************************************************************

	Compile time switches that are automatically set to the
	proper values by the configuration script.

***********************************************************************

	Modification Log:

	a-1:	12/20/98	warme
		: Created.
	a-2:	02/28/2001	warme
		: Changes for 3.1 release.  Support for GMP,
		:  Intel floating point precision fix, and
		:  stderr being an lvalue.

***********************************************************************/

#ifndef _CONFIG_H_
#define	_CONFIG_H_

/* Define to CPLEX version number, if using CPLEX. */
/* #undef CPLEX */

/* Define if using lp_solve instead of cplex. */
#define LPSOLVE 1

/* Define if have GMP library available. */
/* #undef HAVE_GMP */

/* Define if have Intel x86 floating point precision fix: <fpu_control.h>. */
/* #undef HAVE_X86_FLOATING_POINT_PRECISION_FIX */

/* Define if stderr is an lvalue that can be stored into. */
#define HAVE_STDERR_IS_LVALUE 1

/* Define if times(), struct tms, and CLK_TCK work. */
/* #undef UNIX_CPU_TIME */

/* Define if uname(), <utsname.h> and struct utsname all work. */
#define UNAME_FUNCTION_WORKS 1

/* Define as a string that describes the machine running this software. */
/* This overrides the use of uname(2) or uname(1) to get such a string. */
/* #undef MACHDESC */

/* Define this if popen is available */
#define HAVE_POPEN 1

/* Define this is pclose is available */
#define HAVE_PCLOSE 1

/* Define as the pathname of the uname command, if available */
#define UNAME_PATH "/usr/bin/uname"

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define as void or int, which ever is the return type of signal handlers. */
#define RETSIGTYPE void

/* Define the directories where package is installed */
#define INSTALLDIR_PREFIX "/usr/local"
#define INSTALLDIR_EXEC_PREFIX "/usr/local"
#define INSTALLDIR_BINDIR "/usr/local/bin"
#define INSTALLDIR_DATADIR "/usr/local/share"

/* The current version of GEOSTEINER, as a string. */
#define	GEOSTEINER_VERSION_STRING "3.1"


#endif
