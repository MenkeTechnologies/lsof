/*
 * dfile.c - Linux file processing functions for /proc-based lsof
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

#include "lsof.h"


/*
 * printdevname() - print character device name
 *
 * Note: this function should not be needed in /proc-based lsof, but
 *	 since it is called by printname() in print.c, an ersatz one
 *	 is provided here.
 */

int
printdevname(dev_t * dev, dev_t * rdev, int f, int nty)
{
    char buf[128];

    (void) snpf(buf, sizeof(buf), "%s device: %d,%d",
                (nty == N_BLK) ? "BLK" : "CHR",
                (int) GET_MAJ_DEV(*rdev), (int) GET_MIN_DEV(*rdev));
    safestrprt(buf, stdout, f);
    return (1);
}
