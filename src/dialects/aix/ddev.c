/*
 * ddev.c - AIX device support functions for lsof
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

#ifndef lint
static char copyright[] =
        "@(#) Copyright 1994 lsof contributors.\nAll rights reserved.\n";
#endif


#include "lsof.h"


/*
 * Local definitions
 */

#define    LIKE_BLK_SPEC    "like block special"
#define    LIKE_CHR_SPEC    "like character special"


/*
 * Local function prototypes
 */

_PROTOTYPE(static int rmdupdev, (struct l_dev ***dp, int n, char *nm));


#if    defined(HASDCACHE) && AIXV >= 4140


/*
 * clr_sect() - clear cached clone and pseudo sections
 */

void
clr_sect()
{
    struct clone *c, *c1;

    if (Clone) {
        for (c = Clone; c; c = c1) {
        c1 = c->next;
        if (c->cd.name)
            (void) free((FREE_P *)c->cd.name);
        (void) free((FREE_P *)c);
        }
        Clone = (struct clone *)NULL;
    }
}
#endif    /* defined(HASDCACHE) && AIXV>=4140 */


/*
 * getchan() - get channel from file path name
 */

int
getchan(p)
        char *p;            /* file path name */
{
    int ch;
    char *s;

    if (!(s = strrchr(p, '/')))
        return (-1);
    if (*(++s) == '\0')
        return (-1);
    for (ch = 0; *s; s++) {

#if    defined(__STDC__)
        if (!isdigit(*s))
#else
            if ( ! isascii(*s) || ! isdigit(*s))
#endif    /* __STDC__ */

            return (-1);
        ch = (ch * 10) + *s - '0';
    }
    return (ch);
}


/*
 * printdevname() - print device name
 */

int
printdevname(dev, rdev, f, nty)
        dev_t *dev;            /* device */
        dev_t *rdev;            /* raw device */
        int f;                /* 1 = follow with '\n' */
        int nty;            /* node type: N_BLK or N_CHR */
{
    struct l_dev *dp;
/*
 * Search device table for a full match.
 */
    if ((dp = lkupdev(dev, rdev, 1, 1))) {
        if (CurrentLocalFile->channel < 0)
            safestrprt(dp->name, stdout, f);
        else {
            safestrprt(dp->name, stdout, 0);
            (void) printf("/%d%s", CurrentLocalFile->channel, f ? "\n" : "");
        }
        return (1);
    }
/*
 * Search device table for a match without inode number and dev.
 */
    if ((dp = lkupdev(&DeviceOfDev, rdev, 0, 1))) {

        /*
         * A raw device match was found.  Record it as a name column addition.
         */
        char *cp, *ttl;
        int len;

        ttl = (nty == N_BLK) ? LIKE_BLK_SPEC : LIKE_CHR_SPEC;
        len = (int) (1 + strlen(ttl) + 1 + strlen(dp->name) + 1);
        if (!(cp = (char *) malloc((MALLOC_S)(len + 1)))) {
            (void) fprintf(stderr, "%s: no nma space for: (%s %s)\n",
                           ProgramName, ttl, dp->name);
            Exit(1);
        }
        (void) snpf(cp, len + 1, "(%s %s)", ttl, dp->name);
        (void) add_nma(cp, len);
        (void) free((MALLOC_P *) cp);
        return (0);
    }
    return (0);
}


/*
 * readdev() - read device names, modes and types
 */

void
readdev(skip)
        int skip;            /* skip device cache read if 1 */
{

#if    defined(HASDCACHE)
    int dcrd;
#endif    /* defined(HASDCACHE) */

    DIR *dfp;
    struct dirent *dp;
    char *fp = (char *) NULL;
    int i = 0;

#if    defined(HASBLKDEV)
    int j = 0;
#endif    /* defined(HASBLKDEV) */

    char *path = (char *) NULL;
    MALLOC_S pl;
    struct stat sb;

#if    AIXV >= 4140
    struct clone *c;
    dev_t cd;
#endif    /* AIXV >=4140 */

    if (SortedDevices)
        return;

#if    defined(HASDCACHE)
    /*
     * Read device cache, as directed.
     */
        if (!skip) {
            if (DevCacheState == 2 || DevCacheState == 3) {
            if ((dcrd = read_dcache()) == 0)
                return;
            }
        } else
            dcrd = 1;
#endif    /* defined(HASDCACHE) */

#if    AIXV >= 4140
    /*
     * Establish the clone major device for AIX 4.1.4 and above.
     */
        if (stat("/dev/clone", &sb) == 0) {
            cd = sb.st_rdev;
            CloneMaj = GET_MAJ_DEV(cd);
        }
#endif    /* AIXV >=4140 */

    DirStack = (char **) NULL;
    DirStackAlloc = DirStackIndex = 0;
    (void) stkdir("/dev");
/*
 * Unstack the next /dev or /dev/<subdirectory> directory.
 */
    while (--DirStackIndex >= 0) {
        if (!(dfp = opendir(DirStack[DirStackIndex]))) {

#if    defined(WARNDEVACCESS)
            if (!OptWarnings) {
                (void) fprintf(stderr, "%s: WARNING: can't open: ", ProgramName);
                safestrprt(DirStack[DirStackIndex], stderr, 1);
            }
#endif    /* defined(WARNDEVACCESS) */

            (void) free((FREE_P *) DirStack[DirStackIndex]);
            DirStack[DirStackIndex] = (char *) NULL;
            continue;
        }
        if (path) {
            (void) free((FREE_P *) path);
            path = (char *) NULL;
        }
        if (!(path = mkstrcat(DirStack[DirStackIndex], -1, "/", 1, (char *) NULL,
                              -1, &pl))) {
            (void) fprintf(stderr, "%s: no space for: ", ProgramName);
            safestrprt(DirStack[DirStackIndex], stderr, 1);
            Exit(1);
        }
        (void) free((FREE_P *) DirStack[DirStackIndex]);
        DirStack[DirStackIndex] = (char *) NULL;
        /*
         * Scan the directory.
         */
        for (dp = readdir(dfp); dp; dp = readdir(dfp)) {
            if (!dp->d_ino || (dp->d_name[0] == '.'))
                continue;
            /*
             * Form the full path name and get its status.
             */
            if (fp) {
                (void) free((FREE_P *) fp);
                fp = (char *) NULL;
            }
            if (!(fp = mkstrcat(path, (int) pl, dp->d_name, dp->d_namlen,
                                (char *) NULL, -1, (MALLOC_S *) NULL))) {
                (void) fprintf(stderr, "%s: no space for: ", ProgramName);
                safestrprt(path, stderr, 0);
                safestrprt(dp->d_name, stderr, 1);
                Exit(1);
            }

#if    defined(USE_STAT)
            if (stat(fp, &sb) != 0)
#else	/* !defined(USE_STAT) */
            if (lstat(fp, &sb) != 0)
#endif    /* defined(USE_STAT) */

            {
                if (errno == ENOENT)    /* symbolic link to nowhere? */
                    continue;

#if    defined(WARNDEVACCESS)
                if (!OptWarnings) {
                int errno_save = errno;

                (void) fprintf(stderr, "%s: can't stat: ", ProgramName);
                safestrprt(fp, stderr, 0);
                (void) fprintf(stderr, ": %s\n", strerror(errno_save));
                }
#endif    /* defined(WARNDEVACCESS) */

                continue;
            }
            /*
             * If it's a subdirectory, stack its name for later processing.
             */
            if ((sb.st_mode & S_IFMT) == S_IFDIR) {
                (void) stkdir(fp);
                continue;
            }
            if ((sb.st_mode & S_IFMT) == S_IFCHR) {

                /*
                 * Save character device information.
                 */
                if (i >= NumDevices) {
                    NumDevices += DEVINCR;
                    if (!DeviceTable)
                        DeviceTable = (struct l_dev *) malloc(
                                (MALLOC_S)(sizeof(struct l_dev) * NumDevices));
                    else
                        DeviceTable = (struct l_dev *) realloc((MALLOC_P *) DeviceTable,
                                                         (MALLOC_S)(sizeof(struct l_dev) * NumDevices));
                    if (!DeviceTable) {
                        (void) fprintf(stderr,
                                       "%s: no space for character device\n", ProgramName);
                        Exit(1);
                    }
                }
                DeviceTable[i].rdev = sb.st_rdev;
                DeviceTable[i].inode = (INODETYPE) sb.st_ino;
                if (!(DeviceTable[i].name = mkstrcpy(fp, (MALLOC_S *) NULL))) {
                    (void) fprintf(stderr, "%s: no space for: ", ProgramName);
                    safestrprt(fp, stderr, 1);
                    Exit(1);
                }
                DeviceTable[i].v = 0;
                i++;

#if    AIXV >= 4140
                /*
                 * Save information on AIX 4.1.4 and above clone devices.
                 */
                    if (CloneMaj >= 0 && CloneMaj == GET_MAJ_DEV(sb.st_rdev)) {
                    if (!(c = (struct clone *)malloc(
                          (MALLOC_S)sizeof(struct clone))))
                    {
                        (void) fprintf(stderr,
                        "%s: no space for clone device: ", ProgramName);
                        safestrprt(fp, stderr, 1);
                        exit(1);
                    }
                    if (!(c->cd.name = mkstrcpy(fp, (MALLOC_S)NULL))) {
                        (void) fprintf(stderr,
                        "%s: no space for clone name: ", ProgramName);
                        safestrprt(fp, stderr, 1);
                        exit(1);
                    }
                    c->cd.inode = (INODETYPE)sb.st_ino;
                    c->cd.rdev = sb.st_rdev;
                    c->cd.v = 0;
                    c->next = Clone;
                    Clone = c;
                    if (ClonePtc < 0 && strcmp(path, "/dev/ptc") == 0)
                        ClonePtc = GET_MIN_DEV(sb.st_rdev);
                    }
#endif    /* AIXV >=4140 */

            }

#if    defined(HASBLKDEV)
            if ((sb.st_mode & S_IFMT) == S_IFBLK) {

            /*
             * Save block device information in BlockDeviceTable[].
             */
                if (j >= BlockNumDevices) {
                BlockNumDevices += DEVINCR;
                if (!BlockDeviceTable)
                    BlockDeviceTable = (struct l_dev *)malloc(
                         (MALLOC_S)(sizeof(struct l_dev)*BlockNumDevices));
                else
                    BlockDeviceTable = (struct l_dev *)realloc(
                         (MALLOC_P *)BlockDeviceTable,
                         (MALLOC_S)(sizeof(struct l_dev)*BlockNumDevices));
                if (!BlockDeviceTable) {
                    (void) fprintf(stderr,
                    "%s: no space for block device\n", ProgramName);
                    Exit(1);
                }
                }
                BlockDeviceTable[j].rdev = sb.st_rdev;
                BlockDeviceTable[j].inode = (INODETYPE)sb.st_ino;
                BlockDeviceTable[j].name = fp;
                fp = (char *)NULL;
                BlockDeviceTable[j].v = 0;
                j++;
            }
#endif    /* defined(HASBLKDEV) */

        }
        (void) closedir(dfp);
    }
/*
 * Free any allocated space.
 */
    if (DirStack) {
        (void) free((FREE_P *) DirStack);
        DirStack = (char **) NULL;
        DirStackAlloc = DirStackIndex = 0;
    }
    if (fp)
        (void) free((FREE_P *) fp);
    if (path)
        (void) free((FREE_P *) path);
/*
 * Reduce the BlockDeviceTable[] (optional) and DeviceTable[] tables to their minimum
 * sizes; allocate and build sort pointer lists; and sort the tables by
 * device number.
 */

#if    defined(HASBLKDEV)
    if (BlockNumDevices) {
        if (BlockNumDevices > j) {
        BlockNumDevices = j;
        BlockDeviceTable = (struct l_dev *)realloc((MALLOC_P *)BlockDeviceTable,
             (MALLOC_S)(sizeof(struct l_dev) * BlockNumDevices));
        }
        if (!(BlockSortedDevices = (struct l_dev **)malloc(
              (MALLOC_S)(sizeof(struct l_dev *) * BlockNumDevices))))
        {
        (void) fprintf(stderr,
            "%s: no space for block device sort pointers\n", ProgramName);
        Exit(1);
        }
        for (j = 0; j < BlockNumDevices; j++) {
        BlockSortedDevices[j] = &BlockDeviceTable[j];
        }
        (void) qsort((QSORT_P *)BlockSortedDevices, (size_t)BlockNumDevices,
        (size_t)sizeof(struct l_dev *), compdev);
        BlockNumDevices = rmdupdev(&BlockSortedDevices, BlockNumDevices, "block");
    } else {
        if (!OptWarnings)
        (void) fprintf(stderr,
            "%s: WARNING: no block devices found\n", ProgramName);
    }
#endif    /* defined(HASBLKDEV) */

    if (NumDevices) {
        if (NumDevices > i) {
            NumDevices = i;
            DeviceTable = (struct l_dev *) realloc((MALLOC_P *) DeviceTable,
                                             (MALLOC_S)(sizeof(struct l_dev) * NumDevices));
        }
        if (!(SortedDevices = (struct l_dev **) malloc(
                (MALLOC_S)(sizeof(struct l_dev *) * NumDevices)))) {
            (void) fprintf(stderr,
                           "%s: no space for character device sort pointers\n", ProgramName);
            Exit(1);
        }
        for (i = 0; i < NumDevices; i++) {
            SortedDevices[i] = &DeviceTable[i];
        }
        (void) qsort((QSORT_P *) SortedDevices, (size_t) NumDevices,
                     (size_t)
        sizeof(struct l_dev *), compdev);
        NumDevices = rmdupdev(&SortedDevices, NumDevices, "char");
    } else {
        (void) fprintf(stderr, "%s: no character devices found\n", ProgramName);
        Exit(1);
    }

#if    defined(HASDCACHE)
    /*
     * Write device cache file, as required.
     */
        if (DevCacheState == 1 || (DevCacheState == 3 && dcrd))
            write_dcache();
#endif    /* defined(HASDCACHE) */

}


#if    defined(HASDCACHE)
/*
 * rereaddev() - reread device names, modes and types
 */

void
rereaddev()
{
    (void) clr_devtab();

# if	defined(DCACHE_CLR)
    (void) DCACHE_CLR();
# endif	/* defined(DCACHE_CLR) */

    readdev(1);
    DevCacheUnsafe = 0;
}
#endif    /* defined(HASDCACHE) */


/*
 * rmdupdev() - remove duplicate (major/minor/inode) devices
 */

static int
rmdupdev(dp, n, nm)
        struct l_dev ***dp;    /* device table pointers address */
        int n;            /* number of pointers */
        char *nm;        /* device table name for error message */
{

#if    AIXV >= 4140
    struct clone *c, *cp;
#endif    /* AIXV>=4140 */

    int i, j, k;
    struct l_dev **p;

    for (i = j = 0, p = *dp; i < n;) {
        for (k = i + 1; k < n; k++) {
            if (p[i]->rdev != p[k]->rdev || p[i]->inode != p[k]->inode)
                break;

#if    AIXV >= 4140
            /*
             * See if we're deleting a duplicate clone device.  If so,
             * delete its clone table entry.
             */
            for (c = Clone, cp = (struct clone *)NULL;
                 c;
                 cp = c, c = c->next)
            {
                if (c->cd.rdev != p[k]->rdev
                ||  c->cd.inode != p[k]->inode
                ||  strcmp(c->cd.name, p[k]->name))
                continue;
                if (!cp)
                Clone = c->next;
                else
                cp->next = c->next;
                if (c->cd.name)
                (void) free((FREE_P *)c->cd.name);
                (void) free((FREE_P *)c);
                break;
            }
#endif    /* AIXV>=4140 */

        }
        if (i != j)
            p[j] = p[i];
        j++;
        i = k;
    }
    if (n == j)
        return (n);
    if (!(*dp = (struct l_dev **) realloc((MALLOC_P * ) * dp,
                                          (MALLOC_S)(j * sizeof(struct l_dev *))))) {
        (void) fprintf(stderr, "%s: can't realloc %s device pointers\n",
                       ProgramName, nm);
        Exit(1);
    }
    return (j);
}


#if    defined(HASDCACHE) && AIXV >= 4140
/*
 * rw_clone_sect() - read/write the device cache file clone section
 */

int
rw_clone_sect(m)
    int m;				/* mode: 1 = read; 2 = write */
{
    char buf[MAXPATHLEN*2], *cp;
    struct clone *c;
    int i, len, n;

    if (m == 1) {

    /*
     * Read the clone section header and validate it.
     */
        if (!fgets(buf, sizeof(buf), DevCacheStream)) {

bad_clone_sect:

        if (!OptWarnings) {
            (void) fprintf(stderr,
            "%s: bad clone section header in %s: ",
            ProgramName, DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1);
        }
        return(1);
        }
        (void) crc(buf, strlen(buf), &DevCacheChecksum);
        len = strlen("clone section: ");
        if (strncmp(buf, "clone section: ", len) != 0)
        goto bad_clone_sect;
        if ((n = atoi(&buf[len])) < 0)
        goto bad_clone_sect;
    /*
     * Read the clone section lines and create the Clone list.
     */
        for (i = 0; i < n; i++) {
        if (!fgets(buf, sizeof(buf), DevCacheStream)) {
            if (!OptWarnings) {
            (void) fprintf(stderr,
                "%s: bad clone line in %s: ", ProgramName, DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1);
            }
            return(1);
        }
        (void) crc(buf, strlen(buf), &DevCacheChecksum);
        /*
         * Allocate a clone structure.
         */
        if (!(c = (struct clone *)calloc(1, sizeof(struct clone)))) {
            (void) fprintf(stderr,
            "%s: no space for cached clone: ", ProgramName);
            safestrprt(buf, stderr, 1);
            Exit(1);
        }
        /*
         * Enter the clone device number.
         */
        if (!(cp = x2dev(buf, &c->cd.rdev)) || *cp++ != ' ') {

bad_cached_clone:
            if (!OptWarnings) {
            (void) fprintf(stderr,
                "%s: bad cached clone device: ", ProgramName);
            safestrprt(buf, stderr, 1);
            }
            return(1);
        }
        CloneMaj = GET_MAJ_DEV(c->cd.rdev);
        /*
         * Enter the clone inode number.
         */
        for (c->cd.inode = (INODETYPE)0; *cp != ' '; cp++) {
            if (*cp < '0' || *cp > '9')
            goto bad_cached_clone;
            c->cd.inode = (INODETYPE)((c->cd.inode * 10) + (*cp - '0'));
        }
        /*
         * Enter the clone path name.
         */
        if ((len = strlen(++cp)) < 2 || *(cp + len - 1) != '\n') {
            if (!OptWarnings) {
            (void) fprintf(stderr,
                "%s: bad cached clone path: ", ProgramName);
            safestrprt(buf, stderr, 1);
            }
            return(1);
        }
        *(cp + len - 1) = '\0';
        if (!(c->cd.name = mkstrcpy(cp, (MALLOC_S *)NULL))) {
            (void) fprintf(stderr,
            "%s: no space for cached clone path: ", ProgramName);
            safestrprt(buf, stderr, 1);
            Exit(1);
        }
        c->cd.v = 0;
        c->next = Clone;
        Clone = c;
        if (ClonePtc < 0 && strcmp(c->cd.name, "/dev/ptc") == 0)
            ClonePtc = GET_MIN_DEV(c->cd.rdev);
        }
        return(0);
    } else if (m == 2) {

    /*
     * Write the clone section header.
     */
        for (c = Clone, n = 0; c; c = c->next, n++)
        ;
        (void) snpf(buf, sizeof(buf), "clone section: %d\n", n);
        if (wr2DCfd(buf, &DevCacheChecksum))
        return(1);
    /*
     * Write the clone section lines.
     */
        for (c = Clone; c; c = c->next) {
        (void) snpf(buf, sizeof(buf), "%x %ld %s\n",
            c->cd.rdev, (long)c->cd.inode, c->cd.name);
        if (wr2DCfd(buf, &DevCacheChecksum))
            return(1);
        }
        return(0);
    }
/*
 * A shouldn't-happen case: mode neither 1 nor 2.
 */
    (void) fprintf(stderr, "%s: internal rw_clone_sect error: %d\n",
        ProgramName, m);
    Exit(1);
}
#endif    /* defined(HASDCACHE) && AIXV>=4140 */


#if    defined(HASDCACHE)
/*
 * vfy_dev() - verify a device table entry (usually when DevCacheUnsafe == 1)
 *
 * Note: rereads entire device table when an entry can't be verified.
 */

int
vfy_dev(dp)
    struct l_dev *dp;		/* device table pointer */
{
    struct stat sb;

    if (!DevCacheUnsafe || dp->v)
        return(1);

#if	defined(USE_STAT)
    if (stat(dp->name, &sb) != 0
#else	/* !defined(USE_STAT) */
    if (lstat(dp->name, &sb) != 0
#endif	/* defined(USE_STAT) */

    ||  dp->rdev != sb.st_rdev
    ||  dp->inode != (INODETYPE)sb.st_ino) {
       (void) rereaddev();
       return(0);
    }
    dp->v = 1;
    return(1);
}
#endif    /* defined(HASDCACHE) */
