/*
 * fino.c -- find inode functions for lsof library
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

/*
 * fino.c -- find block (optional) and character device file inode numbers
 *
 * The caller must define:
 *
 *	HASBLKDEV	to activate the block device inode lookup
 */

#include "../machine.h"

#if defined(HASBLKDEV) || defined(USE_LIB_FIND_CH_INO)

#include "../lsof.h"

#else  /* !defined(HASBLKDEV) && !defined(USE_LIB_FIND_CH_INO) */
char fino_d1[] = "d";
char *fino_d2 = fino_d1;
#endif /* defined(HASBLKDEV) || defined(USE_LIB_FIND_CH_INO) */

#if defined(HASBLKDEV)
/*
 * find_bl_ino() - find the inode number for a block device file
 */

void find_bl_ino() {
    dev_t ldev, tdev;
    int low, hi, mid;

    readdev(0);

    if (!CurrentLocalFile->dev_def || (CurrentLocalFile->dev != DeviceOfDev) ||
        !CurrentLocalFile->rdev_def)
        return;
    ldev = CurrentLocalFile->rdev;

#if defined(HASDCACHE)
    for (;;) {
        int retry = 0;
#endif /* defined(HASDCACHE) */

    low = mid = 0;
    hi = BlockNumDevices - 1;
    while (low <= hi) {
        mid = (low + hi) / 2;
        tdev = BlockSortedDevices[mid]->rdev;
        if (ldev < tdev)
            hi = mid - 1;
        else if (ldev > tdev)
            low = mid + 1;
        else {

#if defined(HASDCACHE)
            if (DevCacheUnsafe && !BlockSortedDevices[mid]->v && !vfy_dev(BlockSortedDevices[mid])) {
                retry = 1;
                break;
            }
#endif /* defined(HASDCACHE) */

            CurrentLocalFile->inode = BlockSortedDevices[mid]->inode;
            if (CurrentLocalFile->inp_ty == 0)
                CurrentLocalFile->inp_ty = 1;
            return;
        }
    }

#if defined(HASDCACHE)
    if (retry)
        continue;
    break;
    } /* end for(;;) */
#endif /* defined(HASDCACHE) */

}
#endif /* defined(HASBLKDEV) */

#if defined(USE_LIB_FIND_CH_INO)
/*
 * find_ch_ino() - find the inode number for a character device file
 */

void find_ch_ino() {
    dev_t ldev, tdev;
    int low, hi, mid;

    readdev(0);

    if (!CurrentLocalFile->dev_def || (CurrentLocalFile->dev != DeviceOfDev) ||
        !CurrentLocalFile->rdev_def)
        return;
    ldev = CurrentLocalFile->rdev;

#if defined(HASDCACHE)
    for (;;) {
        int retry = 0;
#endif /* defined(HASDCACHE) */

    low = mid = 0;
    hi = NumDevices - 1;
    while (low <= hi) {
        mid = (low + hi) / 2;
        tdev = SortedDevices[mid]->rdev;
        if (ldev < tdev)
            hi = mid - 1;
        else if (ldev > tdev)
            low = mid + 1;
        else {

#if defined(HASDCACHE)
            if (DevCacheUnsafe && !SortedDevices[mid]->v && !vfy_dev(SortedDevices[mid])) {
                retry = 1;
                break;
            }
#endif /* defined(HASDCACHE) */

            CurrentLocalFile->inode = SortedDevices[mid]->inode;
            if (CurrentLocalFile->inp_ty == 0)
                CurrentLocalFile->inp_ty = 1;
            return;
        }
    }

#if defined(HASDCACHE)
    if (retry)
        continue;
    break;
    } /* end for(;;) */
#endif /* defined(HASDCACHE) */

}
#endif /* defined(USE_LIB_FIND_CH_INO) */
