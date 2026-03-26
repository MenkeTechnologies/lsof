/*
 * cvfs.c -- ckkv() function for lsof library
 */


/*
 *
 * Written by Jacob Menke
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */


#include "../machine.h"

#if    defined(USE_LIB_CKKV)

#include "../lsof.h"
#include <sys/utsname.h>


/*
 * ckkv() - check kernel version
 */

void
ckkv(dialect, expected_rev, expected_ver, expected_arch)
    char *dialect;			/* dialect */
    char *expected_rev;			/* expected revision; NULL, no test */
    char *expected_ver;			/* expected version; NULL, no test */
    char *expected_arch;		/* expected architecture; NULL, no
					 * test */
{

# if	defined(HASKERNIDCK)
    struct utsname u;

    if (OptWarnings)
        return;
/*
 * Read the system information via uname(2).
 */
    if (uname(&u) < 0) {
        (void) fprintf(stderr, "%s: uname error: %s\n",
        ProgramName, strerror(errno));
        Exit(1);
    }
    if (expected_rev && strcmp(expected_rev, u.release)) {
        (void) fprintf(stderr,
        "%s: WARNING: compiled for %s release %s; this is %s.\n",
        ProgramName, dialect, expected_rev, u.release);
    }
    if (expected_ver && strcmp(expected_ver, u.version)) {
        (void) fprintf(stderr,
        "%s: WARNING: compiled for %s version %s; this is %s.\n",
        ProgramName, dialect, expected_ver, u.version);
    }
    if (expected_arch && strcmp(expected_arch, u.machine)) {
        (void) fprintf(stderr,
        "%s: WARNING: compiled for %s architecture %s; this is %s.\n",
        ProgramName, dialect, expected_arch, u.machine);
    }
# endif	/* defined(HASKERNIDCK) */

}
#else	/* !defined(USE_LIB_CKKV) */
char ckkv_d1[] = "d";
char *ckkv_d2 = ckkv_d1;
#endif    /* defined(USE_LIB_CKKV) */
