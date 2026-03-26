/*
 * lkud.c -- device lookup functions for lsof library
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
 * lkud.c -- lookup device
 *
 * The caller may define:
 *
 *	HASBLKDEV	to activate block device lookup
 */

#include "../machine.h"

#if defined(HASBLKDEV) || defined(USE_LIB_LKUPDEV)

#include "../lsof.h"

#else  /* !defined(HASBLKDEV) && !defined(USE_LIB_LKUPDEV) */
char lkud_d1[] = "d";
char *lkud_d2 = lkud_d1;
#endif /* defined(HASBLKDEV) || defined(USE_LIB_LKUPDEV) */

#if defined(HASBLKDEV)
/*
 * lkupbdev() - look up a block device
 */

struct l_dev *lkupbdev(dev_t *dev, dev_t *rdev, int inode_match, int rebuild) {
    INODETYPE inode = (INODETYPE)0;
    int low, hi, mid;
    struct l_dev *dp;
    int ty = 0;

    if (*dev != DeviceOfDev)
        return ((struct l_dev *)NULL);
    readdev(0);
    if (inode_match) {
        inode = CurrentLocalFile->inode;
        ty = CurrentLocalFile->inp_ty;
    }
    /*
 * Search block device table for match.
 */

#if defined(HASDCACHE)
    for (;;) {
        int retry = 0;
#endif /* defined(HASDCACHE) */

    low = mid = 0;
    hi = BlockNumDevices - 1;
    while (low <= hi) {
        mid = (low + hi) / 2;
        dp = BlockSortedDevices[mid];
        if (*rdev < dp->rdev)
            hi = mid - 1;
        else if (*rdev > dp->rdev)
            low = mid + 1;
        else {
            if ((inode_match == 0) || (ty != 1) || (inode == dp->inode)) {

#if defined(HASDCACHE)
                if (DevCacheUnsafe && !dp->v && !vfy_dev(dp)) {
                    retry = 1;
                    break;
                }
#endif /* defined(HASDCACHE) */

                return (dp);
            }
            if (inode < dp->inode)
                hi = mid - 1;
            else
                low = mid + 1;
        }
    }

#if defined(HASDCACHE)
    if (retry)
        continue;
    if (DevCacheUnsafe && rebuild) {
        (void)rereaddev();
        continue;
    }
    break;
    } /* end for(;;) */
#endif /* defined(HASDCACHE) */

    return ((struct l_dev *)NULL);
}
#endif /* defined(HASBLKDEV) */

#if defined(USE_LIB_LKUPDEV)
/*
 * lkupdev() - look up a character device
 */

struct l_dev *lkupdev(dev_t *dev, dev_t *rdev, int inode_match, int rebuild) {
    INODETYPE inode = (INODETYPE)0;
    int low, hi, mid;
    struct l_dev *dp;
    int ty = 0;

    if (*dev != DeviceOfDev)
        return ((struct l_dev *)NULL);
    readdev(0);
    if (inode_match) {
        inode = CurrentLocalFile->inode;
        ty = CurrentLocalFile->inp_ty;
    }
    /*
 * Search device table for match.
 */

#if defined(HASDCACHE)
    for (;;) {
        int retry = 0;
#endif /* defined(HASDCACHE) */

    low = mid = 0;
    hi = NumDevices - 1;
    while (low <= hi) {
        mid = (low + hi) / 2;
        dp = SortedDevices[mid];
        if (*rdev < dp->rdev)
            hi = mid - 1;
        else if (*rdev > dp->rdev)
            low = mid + 1;
        else {
            if ((inode_match == 0) || (ty != 1) || (inode == dp->inode)) {

#if defined(HASDCACHE)
                if (DevCacheUnsafe && !dp->v && !vfy_dev(dp)) {
                    retry = 1;
                    break;
                }
#endif /* defined(HASDCACHE) */

                return (dp);
            }
            if (inode < dp->inode)
                hi = mid - 1;
            else
                low = mid + 1;
        }
    }

#if defined(HASDCACHE)
    if (retry)
        continue;
    if (DevCacheUnsafe && rebuild) {
        (void)rereaddev();
        continue;
    }
    break;
    } /* end for(;;) */
#endif /* defined(HASDCACHE) */

    return ((struct l_dev *)NULL);
}
#endif /* defined(USE_LIB_LKUPDEV) */
