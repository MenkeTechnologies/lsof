/*
 * dvch.c -- device cache functions for lsof library
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

#if defined(HASDCACHE)

#include "../lsof.h"

/*
 * dvch.c - module that contains common device cache functions
 *
 * The caller may define the following:
 *
 *	DCACHE_CLONE	is the name of the function that reads and writes the
 *			clone section of the device cache file.  The clone
 *			section follows the device section.  If DCACHE_CLONE
 *			isn't defined, but HAS_STD_CLONE is defined to be 1,
 *			DCACHE_CLONE defaults to the local static function
 *			rw_clone_sect() that reads and writes a standard
 *			clone cache.
 *
 *	DCACHE_CLR	is the name of the function that clears the clone and
 *			pseudo caches when reading the device cache fails.   If
 *			DCACHE_CLR isn't defined, but HAS_STD_CLONE is defined
 *			to be 1, DCACHE_CLR defaults to the local static
 *			function clr_sect() that clears a standard clone cache.
 *
 *	DCACHE_PSEUDO	is the name of the function that reads and writes
 *			the pseudo section of the device cache file.  The
 *			pseudo section follows the device section and the
 *			clone section, if there is one.
 *
 *	DVCH_CHOWN	if the dialect has no fchown() function, so
 *			chown() must be used instead.
 *
 *	DVCH_DEVPATH	if the path to the device directory isn't "/dev".
 *
 *	DVCH_EXPDEV	if st_rdev must be expanded with the expdev()
 *			macro before use.  (This is an EP/IX artifact.)
 *
 *	HASBLKDEV	if block device information is stored in BlockDeviceTable[].
 */

/*
 * Local definitions
 */

#if !defined(DVCH_DEVPATH)
#define DVCH_DEVPATH "/dev"
#endif /* !defined(DVCH_DEVPATH) */

/*
 * Local storage
 */

static int crctbl[CRC_TBLL]; /* crc partial results table */

/*
 * Local function prototypes
 */

#undef DCACHE_CLR_LOCAL
#if !defined(DCACHE_CLR)
#if defined(HAS_STD_CLONE) && HAS_STD_CLONE == 1
#define DCACHE_CLR       clr_sect
#define DCACHE_CLR_LOCAL 1
_PROTOTYPE(static void clr_sect, (void));
#endif /* defined(HAS_STD_CLONE) && HAS_STD_CLONE==1 */
#endif /* !defined(DCACHE_CLR) */

#undef DCACHE_CLONE_LOCAL
#if !defined(DCACHE_CLONE)
#if defined(HAS_STD_CLONE) && HAS_STD_CLONE == 1
#define DCACHE_CLONE       rw_clone_sect
#define DCACHE_CLONE_LOCAL 1
_PROTOTYPE(static int rw_clone_sect, (int mode));
#endif /* defined(HAS_STD_CLONE) && HAS_STD_CLONE==1 */
#endif /*!defined(DCACHE_CLONE) */

#if defined(HASBLKDEV)
/*
 * alloc_bdcache() - allocate block device cache
 */

void alloc_bdcache() {
    if (!(BlockDeviceTable =
              (struct l_dev *)calloc((MALLOC_S)BlockNumDevices, sizeof(struct l_dev)))) {
        (void)fprintf(stderr, "%s: no space for block devices\n", ProgramName);
        Exit(1);
    }
    if (!(BlockSortedDevices =
              (struct l_dev **)malloc((MALLOC_S)(sizeof(struct l_dev *) * BlockNumDevices)))) {
        (void)fprintf(stderr, "%s: no space for block device pointers\n", ProgramName);
        Exit(1);
    }
}
#endif /* defined(HASBLKDEV) */

/*
 * alloc_dcache() - allocate device cache
 */

void alloc_dcache() {
    if (!(DeviceTable = (struct l_dev *)calloc((MALLOC_S)NumDevices, sizeof(struct l_dev)))) {
        (void)fprintf(stderr, "%s: no space for devices\n", ProgramName);
        Exit(1);
    }
    if (!(SortedDevices =
              (struct l_dev **)malloc((MALLOC_S)(sizeof(struct l_dev *) * NumDevices)))) {
        (void)fprintf(stderr, "%s: no space for device pointers\n", ProgramName);
        Exit(1);
    }
}

/*
 * clr_devtab() - clear the device tables and free their space
 */

void clr_devtab() {
    int i;

    if (DeviceTable) {
        for (i = 0; i < NumDevices; i++) {
            if (DeviceTable[i].name) {
                (void)free((FREE_P *)DeviceTable[i].name);
                DeviceTable[i].name = (char *)NULL;
            }
        }
        (void)free((FREE_P *)DeviceTable);
        DeviceTable = (struct l_dev *)NULL;
    }
    if (SortedDevices) {
        (void)free((FREE_P *)SortedDevices);
        SortedDevices = (struct l_dev **)NULL;
    }
    NumDevices = 0;

#if defined(HASBLKDEV)
    if (BlockDeviceTable) {
        for (i = 0; i < BlockNumDevices; i++) {
            if (BlockDeviceTable[i].name) {
                (void)free((FREE_P *)BlockDeviceTable[i].name);
                BlockDeviceTable[i].name = (char *)NULL;
            }
        }
        (void)free((FREE_P *)BlockDeviceTable);
        BlockDeviceTable = (struct l_dev *)NULL;
    }
    if (BlockSortedDevices) {
        (void)free((FREE_P *)BlockSortedDevices);
        BlockSortedDevices = (struct l_dev **)NULL;
    }
    BlockNumDevices = 0;
#endif /* defined(HASBLKDEV) */
}

#if defined(DCACHE_CLR_LOCAL)
/*
 * clr_sect() - clear cached standard clone sections
 */

static void clr_sect() {
    struct clone *c, *c1;

    if (Clone) {
        for (c = Clone; c; c = c1) {
            c1 = c->next;
            (void)free((FREE_P *)c);
        }
        Clone = (struct clone *)NULL;
    }
}
#endif /* defined(DCACHE_CLR_LOCAL) */

/*
 * crc(b, l, s) - compute a crc for a block of bytes
 */

void crc(char *block, int len, unsigned *sumptr) {
    char *cp;     /* character pointer */
    char *lm;     /* character limit pointer */
    unsigned sum; /* check sum */

    cp = block;
    lm = cp + len;
    sum = *sumptr;
    do {
        sum ^= ((int)*cp++) & 0xff;
        sum = (sum >> 8) ^ crctbl[sum & 0xff];
    } while (cp < lm);
    *sumptr = sum;
}

/*
 * crcbld - build the CRC-16 partial results table
 */

void crcbld() {
    int bit;        /* temporary bit value */
    unsigned entry; /* entry under construction */
    int i;          /* polynomial table index */
    int j;          /* bit shift count */

    for (i = 0; i < CRC_TBLL; i++) {
        entry = i;
        for (j = 1; j <= CRC_BITS; j++) {
            bit = entry & 1;
            entry >>= 1;
            if (bit)
                entry ^= CRC_POLY;
        }
        crctbl[i] = entry;
    }
}

/*
 * dcpath() - define device cache file paths
 */

int dcpath(int read_write, int npw) {
    char buf[MAXPATHLEN + 1], *cp1, *cp2, hn[MAXPATHLEN + 1];
    int endf;
    int i, j;
    int l = 0;
    int ierr = 0; /* intermediate error state */
    int merr = 0; /* malloc error state */
    struct passwd *p = (struct passwd *)NULL;
    static short wenv = 1; /* HASENVDC warning state */
    static short wpp = 1;  /* HASPERSDCPATH warning state */
                           /*
 * Release any space reserved by previous path calls to dcpath().
 */
    if (DevCachePath[1]) {
        (void)free((FREE_P *)DevCachePath[1]);
        DevCachePath[1] = (char *)NULL;
    }
    if (DevCachePath[3]) {
        (void)free((FREE_P *)DevCachePath[3]);
        DevCachePath[3] = (char *)NULL;
    }
    /*
 * If a path was specified via -D, it's character address will have been
 * stored in DevCachePathArg by ctrl_dcache().  Use that address if the real UID
 * of this process is root, or the mode is read, or the process is neither
 * setuid-root nor setgid.
 */
    if (MyRealUid == 0 || read_write == 1 || (!SetuidRootState && !SetgidState))
        DevCachePath[0] = DevCachePathArg;
    else
        DevCachePath[0] = (char *)NULL;

#if defined(HASENVDC)
    /*
 * If HASENVDC is defined, get its value from the environment, unless this
 * is a setuid-root process, or the real UID of the process is 0, or the
 * mode is write and the process is setgid.
 */
    if ((cp1 = getenv(HASENVDC)) && (l = strlen(cp1)) > 0 && !SetuidRootState && MyRealUid &&
        (read_write == 1 || !SetgidState)) {
        if (!(cp2 = mkstrcpy(cp1, (MALLOC_S *)NULL))) {
            (void)fprintf(stderr, "%s: no space for device cache path: %s=", ProgramName, HASENVDC);
            safestrprt(cp1, stderr, 1);
            merr = 1;
        } else
            DevCachePath[1] = cp2;
    } else if (cp1 && l > 0) {
        if (!OptWarnings && wenv) {
            (void)fprintf(stderr, "%s: WARNING: ignoring environment: %s=", ProgramName, HASENVDC);
            safestrprt(cp1, stderr, 1);
        }
        wenv = 0;
    }
#endif /* defined(HASENVDC) */

#if defined(HASSYSDC)
    /*
 * If HASSYSDC is defined, record the path of the system-wide device
 * cache file, unless the mode is write.
 */
    if (read_write != 2)
        DevCachePath[2] = HASSYSDC;
    else
        DevCachePath[2] = (char *)NULL;
#endif /* defined(HASSYSDC) */

#if defined(HASPERSDC)
    /*
 * If HASPERSDC is defined, form a personal device cache path by
 * interpreting the conversions specified in it.
 *
 * Get (HASPERSDCPATH) from the environment and add it to the home directory
 * path, if possible.
 */
    for (cp1 = HASPERSDC, endf = i = 0; *cp1 && !endf; cp1++) {
        if (*cp1 != '%') {

            /*
         * If the format character isn't a `%', copy it.
         */
            if (i < (int)sizeof(buf)) {
                buf[i++] = *cp1;
                continue;
            } else {
                ierr = 2;
                break;
            }
        }
        /*
     * `%' starts a conversion; the next character specifies
     * the conversion type.
     */
        cp1++;
        switch (*cp1) {

            /*
         * Two consecutive `%' characters convert to one `%'
         * character in the output.
         */

        case '%':
            if (i < (int)sizeof(buf))
                buf[i++] = '%';
            else
                ierr = 2;
            break;

            /*
         * ``%0'' defines a root boundary.  If the effective
         * (setuid-root) or real UID of the process is root, any
         * path formed to this point is discarded and path formation
         * begins with the next character.
         *
         * If neither the effective nor the real UID is root, path
         * formation ends.
         *
         * This allows HASPERSDC to specify one path for non-root
         * UIDs and another for the root (effective or real) UID.
         */

        case '0':
            if (SetuidRootState || !MyRealUid)
                i = 0;
            else
                endf = 1;
            break;

            /*
         * ``%h'' converts to the home directory.
         */

        case 'h':
            if (!p && !(p = getpwuid(MyRealUid))) {
                if (!OptWarnings)
                    (void)fprintf(stderr, "%s: WARNING: can't get home dir for UID: %d\n",
                                  ProgramName, (int)MyRealUid);
                ierr = 1;
                break;
            }
            if ((i + (l = strlen(p->pw_dir))) >= (int)sizeof(buf)) {
                ierr = 2;
                break;
            }
            (void)strcpy(&buf[i], p->pw_dir);
            i += l;
            if (i > 0 && buf[i - 1] == '/' && *(cp1 + 1)) {

                /*
         * If the home directory ends in a '/' and the next format
         * character is a '/', delete the '/' at the end of the home
         * directory.
         */
                i--;
                buf[i] = '\0';
            }
            break;

            /*
         * ``%l'' converts to the full host name.
         *
         * ``%L'' converts to the first component (characters up
         * to the first `.') of the host name.
         */

        case 'l':
        case 'L':
            if (gethostname(hn, sizeof(hn) - 1) < 0) {
                if (!OptWarnings)
                    (void)fprintf(stderr, "%s: WARNING: no gethostname for %%l or %%L: %s\n",
                                  ProgramName, strerror(errno));
                ierr = 1;
                break;
            }
            hn[sizeof(hn) - 1] = '\0';
            if (*cp1 == 'L' && (cp2 = strchr(hn, '.')) && cp2 > hn)
                *cp2 = '\0';
            j = strlen(hn);
            if ((j + i) < (int)sizeof(buf)) {
                (void)strcpy(&buf[i], hn);
                i += j;
            } else
                ierr = 2;
            break;

            /*
         * ``%p'' converts to the contents of LSOFPERSDCPATH, followed
         * by a '/'.
         *
         * It is ignored when:
         *
         *    The lsof process is setuid-root;
         *    The real UID of the lsof process is 0;
         *    The mode is write and the process is setgid.
         */

        case 'p':

#if defined(HASPERSDCPATH)
            if ((cp2 = getenv(HASPERSDCPATH)) && (l = strlen(cp2)) > 0 && !SetuidRootState &&
                MyRealUid && (read_write == 1 || !SetgidState)) {
                if (i && buf[i - 1] == '/' && *cp2 == '/') {
                    cp2++;
                    l--;
                }
                if ((i + l) < ((int)sizeof(buf) - 1)) {
                    (void)strcpy(&buf[i], cp2);
                    i += l;
                    if (buf[i - 1] != '/') {
                        if (i < ((int)sizeof(buf) - 2)) {
                            buf[i++] = '/';
                            buf[i] = '\0';
                        } else
                            ierr = 2;
                    }
                } else
                    ierr = 2;
            } else {
                if (cp2 && l > 0) {
                    if (!OptWarnings && wpp) {
                        (void)fprintf(stderr, "%s: WARNING: ignoring environment: %s", ProgramName,
                                      HASPERSDCPATH);
                        safestrprt(cp2, stderr, 1);
                    }
                    wpp = 0;
                }
            }
#else  /* !defined(HASPERSDCPATH) */
            if (!OptWarnings && wpp)
                (void)fprintf(stderr, "%s: WARNING: HASPERSDCPATH disabled: %s\n", ProgramName,
                              HASPERSDC);
            ierr = 1;
            wpp = 0;
#endif /* defined(HASPERSDCPATH) */

            break;

            /*
         * ``%u'' converts to the login name of the real UID of the
         * lsof process.
         */

        case 'u':
            if (!p && !(p = getpwuid(MyRealUid))) {
                if (!OptWarnings)
                    (void)fprintf(stderr, "%s: WARNING: can't get login name for UID: %d\n",
                                  ProgramName, (int)MyRealUid);
                ierr = 1;
                break;
            }
            if ((i + (l = strlen(p->pw_name))) >= (int)sizeof(buf)) {
                ierr = 2;
                break;
            }
            (void)strcpy(&buf[i], p->pw_name);
            i += l;
            break;

            /*
         * ``%U'' converts to the real UID of the lsof process.
         */

        case 'U':
            (void)snpf(hn, sizeof(hn), "%d", (int)MyRealUid);
            if ((i + (l = strlen(hn))) >= (int)sizeof(buf))
                ierr = 2;
            else {
                (void)strcpy(&buf[i], hn);
                i += l;
            }
            break;
        default:
            if (!OptWarnings)
                (void)fprintf(stderr, "%s: WARNING: bad conversion (%%%c): %s\n", ProgramName, *cp1,
                              HASPERSDC);
            ierr = 1;
        }
        if (endf || ierr > 1)
            break;
    }
    if (ierr) {

        /*
     * If there was an intermediate error of some type, handle it.
     * A type 1 intermediate error has already been noted with a
     * warning message.  A type 2 intermediate error requires the
     * issuing of a buffer overlow warning message.
     */
        if (ierr == 2 && !OptWarnings)
            (void)fprintf(stderr, "%s: WARNING: device cache path too large: %s\n", ProgramName,
                          HASPERSDC);
        i = 0;
    }
    buf[i] = '\0';
    /*
 * If there is one, allocate space for the personal device cache path,
 * copy buf[] to it, and store its pointer in DevCachePath[3].
 */
    if (i) {
        if (!(cp1 = mkstrcpy(buf, (MALLOC_S *)NULL))) {
            (void)fprintf(stderr, "%s: no space for device cache path: ", ProgramName);
            safestrprt(buf, stderr, 1);
            merr = 1;
        } else
            DevCachePath[3] = cp1;
    }
#endif /* defined(HASPERSDC) */

    /*
 * Quit if there was a malloc() error.  The appropriate error message
 * will have been issued to stderr.
 */
    if (merr)
        Exit(1);
    /*
 * Return the index of the first defined path.  Since DevCachePath[] is arranged
 * in priority order, searching it beginning to end follows priority.
 * Return an error indication if the search discloses no path name.
 */
    for (i = 0; i < MAXDCPATH; i++) {
        if (DevCachePath[i])
            return (i);
    }
    if (!OptWarnings && npw)
        (void)fprintf(stderr, "%s: WARNING: can't form any device cache path\n", ProgramName);
    return (-1);
}

/*
 * open_dcache() - open device cache file
 */

int open_dcache(int mode, int reuse, struct stat *stat_buf) {
    char buf[128];
    char *w = (char *)NULL;
    /*
 * Get the device cache file paths.
 */
    if (!reuse) {
        if ((DevCachePathIndex = dcpath(mode, 1)) < 0)
            return (1);
    }
    /*
 * Switch to the requested open() action.
 */
    switch (mode) {
    case 1:

        /*
     * Check for access permission.
     */
        if (!is_readable(DevCachePath[DevCachePathIndex], 0)) {
            if (DevCachePathIndex == 2 && errno == ENOENT)
                return (2);
            if (!OptWarnings)
                (void)fprintf(stderr, ACCESSERRFMT, ProgramName, DevCachePath[DevCachePathIndex],
                              strerror(errno));
            return (1);
        }
        /*
     * Open for reading.
     */
        if ((DevCacheFd = open(DevCachePath[DevCachePathIndex], O_RDONLY, 0)) < 0) {
            if (DevCacheState == 3 && errno == ENOENT)
                return (1);

        cant_open:
            (void)fprintf(stderr, "%s: WARNING: can't open %s: %s\n", ProgramName,
                          DevCachePath[DevCachePathIndex], strerror(errno));
            return (1);
        }
        if (stat(DevCachePath[DevCachePathIndex], stat_buf) != 0) {

        cant_stat:
            if (!OptWarnings)
                (void)fprintf(stderr, "%s: WARNING: can't stat(%s): %s\n", ProgramName,
                              DevCachePath[DevCachePathIndex], strerror(errno));
        close_exit:
            (void)close(DevCacheFd);
            DevCacheFd = -1;
            return (1);
        }
        if ((int)(stat_buf->st_mode & 07777) != ((DevCachePathIndex == 2) ? 0644 : 0600)) {
            (void)snpf(buf, sizeof(buf), "doesn't have %04o modes",
                       (DevCachePathIndex == 2) ? 0644 : 0600);
            w = buf;
        } else if ((stat_buf->st_mode & S_IFMT) != S_IFREG)
            w = "isn't a regular file";
        else if (!stat_buf->st_size)
            w = "is empty";
        if (w) {
            if (!OptWarnings)
                (void)fprintf(stderr, "%s: WARNING: %s %s.\n", ProgramName,
                              DevCachePath[DevCachePathIndex], w);
            goto close_exit;
        }
        return (0);
    case 2:

        /*
     * Open for writing: first unlink any previous version; then
     * open exclusively, specifying it's an error if the file exists.
     */
        if (unlink(DevCachePath[DevCachePathIndex]) < 0) {
            if (errno != ENOENT) {
                if (!OptWarnings)
                    (void)fprintf(stderr, "%s: WARNING: can't unlink %s: %s\n", ProgramName,
                                  DevCachePath[DevCachePathIndex], strerror(errno));
                return (1);
            }
        }
        if ((DevCacheFd = open(DevCachePath[DevCachePathIndex], O_RDWR | O_CREAT | O_EXCL, 0600)) <
            0)
            goto cant_open;
        /*
     * If the real user is not root, but the process is setuid-root,
     * change the ownerships of the file to the real ones.
     */
        if (MyRealUid && SetuidRootState) {

#if defined(DVCH_CHOWN)
            if (chown(DevCachePath[DevCachePathIndex], MyRealUid, getgid()) < 0)
#else  /* !defined(DVCH_CHOWN) */
            if (fchown(DevCacheFd, MyRealUid, getgid()) < 0)
#endif /* defined(DVCH_CHOWN) */

            {
                if (!OptWarnings)
                    (void)fprintf(stderr, "%s: WARNING: can't change ownerships of %s: %s\n",
                                  ProgramName, DevCachePath[DevCachePathIndex], strerror(errno));
            }
        }
        if (!OptWarnings && DevCacheState != 1 && !DevCacheUnsafe)
            (void)fprintf(stderr, "%s: WARNING: created device cache file: %s\n", ProgramName,
                          DevCachePath[DevCachePathIndex]);
        if (stat(DevCachePath[DevCachePathIndex], stat_buf) != 0) {
            (void)unlink(DevCachePath[DevCachePathIndex]);
            goto cant_stat;
        }
        return (0);
    default:

        /*
     * Oops!
     */
        (void)fprintf(stderr, "%s: internal error: open_dcache=%d\n", ProgramName, mode);
        Exit(1);
    }
    return (1);
}

/*
 * read_dcache() - read device cache file
 */

int read_dcache() {
    char buf[MAXPATHLEN * 2], cbuf[64], *cp;
    int i, len, ov;
    struct stat sb, devsb;
    /*
 * Open the device cache file.
 *
 * If the open at HASSYSDC fails because the file doesn't exist, and
 * the real UID of this process is not zero, try to open a device cache
 * file at HASPERSDC.
 */
    if ((ov = open_dcache(1, 0, &sb)) != 0) {
        if (DevCachePathIndex == 2) {
            if (ov == 2 && DevCachePath[3]) {
                DevCachePathIndex = 3;
                if (open_dcache(1, 1, &sb) != 0)
                    return (1);
            } else
                return (1);
        } else
            return (1);
    }
    /*
 * If the open device cache file's last mtime/ctime isn't greater than
 * DVCH_DEVPATH's mtime/ctime, ignore it, unless -Dr was specified.
 */
    if (stat(DVCH_DEVPATH, &devsb) != 0) {
        if (!OptWarnings)
            (void)fprintf(stderr, "%s: WARNING: can't stat(%s): %s\n", ProgramName, DVCH_DEVPATH,
                          strerror(errno));
    } else {
        if (sb.st_mtime <= devsb.st_mtime || sb.st_ctime <= devsb.st_ctime)
            DevCacheUnsafe = 1;
    }
    if (!(DevCacheStream = fdopen(DevCacheFd, "r"))) {
        if (!OptWarnings)
            (void)fprintf(stderr, "%s: WARNING: can't fdopen(%s)\n", ProgramName,
                          DevCachePath[DevCachePathIndex]);
        (void)close(DevCacheFd);
        DevCacheFd = -1;
        return (1);
    }
    /*
 * Read the section count line; initialize the CRC table;
 * validate the section count line.
 */
    if (!fgets(buf, sizeof(buf), DevCacheStream)) {

    cant_read:
        if (!OptWarnings)
            (void)fprintf(stderr, "%s: WARNING: can't fread %s: %s\n", ProgramName,
                          DevCachePath[DevCachePathIndex], strerror(errno));
    read_close:
        (void)fclose(DevCacheStream);
        DevCacheFd = -1;
        DevCacheStream = (FILE *)NULL;
        (void)clr_devtab();

#if defined(DCACHE_CLR)
        (void)DCACHE_CLR();
#endif /* defined(DCACHE_CLR) */

        return (1);
    }
    (void)crcbld();
    DevCacheChecksum = 0;
    (void)crc(buf, strlen(buf), &DevCacheChecksum);
    i = 1;
    cp = "";

#if defined(HASBLKDEV)
    i++;
    cp = "s";
#endif /* defined(HASBLKDEV) */

#if defined(DCACHE_CLONE)
    i++;
    cp = "s";
#endif /* defined(DCACHE_CLONE) */

#if defined(DCACHE_PSEUDO)
    i++;
    cp = "s";
#endif /* defined(DCACHE_PSEUDO) */

    (void)snpf(cbuf, sizeof(cbuf), "%d section%s", i, cp);
    len = strlen(cbuf);
    (void)snpf(&cbuf[len], sizeof(cbuf) - len, ", dev=%lx\n", (long)DeviceOfDev);
    if (!strncmp(buf, cbuf, len) && (buf[len] == '\n')) {
        if (!OptWarnings) {
            (void)fprintf(stderr, "%s: WARNING: no /dev device in %s: line ", ProgramName,
                          DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1 + 4 + 8);
        }
        goto read_close;
    }
    if (strcmp(buf, cbuf)) {
        if (!OptWarnings) {
            (void)fprintf(stderr, "%s: WARNING: bad section count line in %s: line ", ProgramName,
                          DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1 + 4 + 8);
        }
        goto read_close;
    }
    /*
 * Read device section header and validate it.
 */
    if (!fgets(buf, sizeof(buf), DevCacheStream))
        goto cant_read;
    (void)crc(buf, strlen(buf), &DevCacheChecksum);
    len = strlen("device section: ");
    if (strncmp(buf, "device section: ", len) != 0) {

    read_dhdr:
        if (!OptWarnings) {
            (void)fprintf(stderr, "%s: WARNING: bad device section header in %s: line ",
                          ProgramName, DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1 + 4 + 8);
        }
        goto read_close;
    }
    /*
 * Compute the device count; allocate SortedDevices[] and DeviceTable[] space.
 */
    if ((NumDevices = atoi(&buf[len])) < 1)
        goto read_dhdr;
    alloc_dcache();
    /*
 * Read the device lines and store their information in DeviceTable[].
 * Construct the SortedDevices[] pointers to DeviceTable[].
 */
    for (i = 0; i < NumDevices; i++) {
        if (!fgets(buf, sizeof(buf), DevCacheStream)) {
            if (!OptWarnings)
                (void)fprintf(stderr, "%s: WARNING: can't read device %d from %s\n", ProgramName,
                              i + 1, DevCachePath[DevCachePathIndex]);
            goto read_close;
        }
        (void)crc(buf, strlen(buf), &DevCacheChecksum);
        /*
     * Convert hexadecimal device number.
     */
        if (!(cp = x2dev(buf, &DeviceTable[i].rdev)) || *cp != ' ') {
            if (!OptWarnings) {
                (void)fprintf(stderr, "%s: device %d: bad device in %s: line ", ProgramName, i + 1,
                              DevCachePath[DevCachePathIndex]);
                safestrprt(buf, stderr, 1 + 4 + 8);
            }
            goto read_close;
        }
        /*
     * Convert inode number.
     */
        for (cp++, DeviceTable[i].inode = (INODETYPE)0; *cp != ' '; cp++) {
            if (*cp < '0' || *cp > '9') {
                if (!OptWarnings) {
                    (void)fprintf(stderr, "%s: WARNING: device %d: bad inode # in %s: line ",
                                  ProgramName, i + 1, DevCachePath[DevCachePathIndex]);
                    safestrprt(buf, stderr, 1 + 4 + 8);
                }
                goto read_close;
            }
            DeviceTable[i].inode = (INODETYPE)((DeviceTable[i].inode * 10) + (int)(*cp - '0'));
        }
        /*
     * Get path name; allocate space for it; copy it; store the
     * pointer in DeviceTable[]; clear verify status; construct the SortedDevices[]
     * pointer to DeviceTable[].
     */
        if ((len = strlen(++cp)) < 2 || *(cp + len - 1) != '\n') {
            if (!OptWarnings) {
                (void)fprintf(stderr, "%s: WARNING: device %d: bad path in %s: line ", ProgramName,
                              i + 1, DevCachePath[DevCachePathIndex]);
                safestrprt(buf, stderr, 1 + 4 + 8);
            }
            goto read_close;
        }
        *(cp + len - 1) = '\0';
        if (!(DeviceTable[i].name = mkstrcpy(cp, (MALLOC_S *)NULL))) {
            (void)fprintf(stderr, "%s: device %d: no space for path: line ", ProgramName, i + 1);
            safestrprt(buf, stderr, 1 + 4 + 8);
            Exit(1);
        }
        DeviceTable[i].v = 0;
        SortedDevices[i] = &DeviceTable[i];
    }

#if defined(HASBLKDEV)
    /*
 * Read block device section header and validate it.
 */
    if (!fgets(buf, sizeof(buf), DevCacheStream))
        goto cant_read;
    (void)crc(buf, strlen(buf), &DevCacheChecksum);
    len = strlen("block device section: ");
    if (strncmp(buf, "block device section: ", len) != 0) {
        if (!OptWarnings) {
            (void)fprintf(stderr, "%s: WARNING: bad block device section header in %s: line ",
                          ProgramName, DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1 + 4 + 8);
        }
        goto read_close;
    }
    /*
 * Compute the block device count; allocate BlockSortedDevices[] and BlockDeviceTable[] space.
 */
    if ((BlockNumDevices = atoi(&buf[len])) > 0) {
        alloc_bdcache();
        /*
     * Read the block device lines and store their information in BlockDeviceTable[].
     * Construct the BlockSortedDevices[] pointers to BlockDeviceTable[].
     */
        for (i = 0; i < BlockNumDevices; i++) {
            if (!fgets(buf, sizeof(buf), DevCacheStream)) {
                if (!OptWarnings)
                    (void)fprintf(stderr, "%s: WARNING: can't read block device %d from %s\n",
                                  ProgramName, i + 1, DevCachePath[DevCachePathIndex]);
                goto read_close;
            }
            (void)crc(buf, strlen(buf), &DevCacheChecksum);
            /*
         * Convert hexadecimal device number.
         */
            if (!(cp = x2dev(buf, &BlockDeviceTable[i].rdev)) || *cp != ' ') {
                if (!OptWarnings) {
                    (void)fprintf(stderr, "%s: block dev %d: bad device in %s: line ", ProgramName,
                                  i + 1, DevCachePath[DevCachePathIndex]);
                    safestrprt(buf, stderr, 1 + 4 + 8);
                }
                goto read_close;
            }
            /*
         * Convert inode number.
         */
            for (cp++, BlockDeviceTable[i].inode = (INODETYPE)0; *cp != ' '; cp++) {
                if (*cp < '0' || *cp > '9') {
                    if (!OptWarnings) {
                        (void)fprintf(stderr, "%s: WARNING: block dev %d: bad inode # in %s: line ",
                                      ProgramName, i + 1, DevCachePath[DevCachePathIndex]);
                        safestrprt(buf, stderr, 1 + 4 + 8);
                    }
                    goto read_close;
                }
                BlockDeviceTable[i].inode =
                    (INODETYPE)((BlockDeviceTable[i].inode * 10) + (int)(*cp - '0'));
            }
            /*
         * Get path name; allocate space for it; copy it; store the
         * pointer in BlockDeviceTable[]; clear verify status; construct the BlockSortedDevices[]
         * pointer to BlockDeviceTable[].
         */
            if ((len = strlen(++cp)) < 2 || *(cp + len - 1) != '\n') {
                if (!OptWarnings) {
                    (void)fprintf(stderr, "%s: WARNING: block dev %d: bad path in %s: line",
                                  ProgramName, i + 1, DevCachePath[DevCachePathIndex]);
                    safestrprt(buf, stderr, 1 + 4 + 8);
                }
                goto read_close;
            }
            *(cp + len - 1) = '\0';
            if (!(BlockDeviceTable[i].name = mkstrcpy(cp, (MALLOC_S *)NULL))) {
                (void)fprintf(stderr, "%s: block dev %d: no space for path: line", ProgramName,
                              i + 1);
                safestrprt(buf, stderr, 1 + 4 + 8);
                Exit(1);
            }
            BlockDeviceTable[i].v = 0;
            BlockSortedDevices[i] = &BlockDeviceTable[i];
        }
    }
#endif /* defined(HASBLKDEV) */

#if defined(DCACHE_CLONE)
    /*
 * Read the clone section.
 */
    if (DCACHE_CLONE(1))
        goto read_close;
#endif /* defined(DCACHE_CLONE) */

#if defined(DCACHE_PSEUDO)
    /*
 * Read the pseudo section.
 */
    if (DCACHE_PSEUDO(1))
        goto read_close;
#endif /* defined(DCACHE_PSEUDO) */

    /*
 * Read and check the CRC section; it must be the last thing in the file.
 */
    (void)snpf(cbuf, sizeof(cbuf), "CRC section: %x\n", DevCacheChecksum);
    if (!fgets(buf, sizeof(buf), DevCacheStream) || strcmp(buf, cbuf) != 0) {
        if (!OptWarnings) {
            (void)fprintf(stderr, "%s: WARNING: bad CRC section in %s: line ", ProgramName,
                          DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1 + 4 + 8);
        }
        goto read_close;
    }
    if (fgets(buf, sizeof(buf), DevCacheStream)) {
        if (!OptWarnings) {
            (void)fprintf(stderr, "%s: WARNING: data follows CRC section in %s: line ", ProgramName,
                          DevCachePath[DevCachePathIndex]);
            safestrprt(buf, stderr, 1 + 4 + 8);
        }
        goto read_close;
    }
    /*
 * Check one device entry at random -- the randomness based on our
 * PID.
 */
    i = (int)(MyProcessId % NumDevices);
    if (stat(DeviceTable[i].name, &sb) != 0

#if defined(DVCH_EXPDEV)
        || expdev(sb.st_rdev) != DeviceTable[i].rdev
#else  /* !defined(DVCH_EXPDEV) */
        || sb.st_rdev != DeviceTable[i].rdev
#endif /* defined(DVCH_EXPDEV) */

        || sb.st_ino != DeviceTable[i].inode) {
        if (!OptWarnings)
            (void)fprintf(stderr, "%s: WARNING: device cache mismatch: %s\n", ProgramName,
                          DeviceTable[i].name);
        goto read_close;
    }
    /*
 * Close the device cache file and return OK.
 */
    (void)fclose(DevCacheStream);
    DevCacheFd = -1;
    DevCacheStream = (FILE *)NULL;
    return (0);
}

#if defined(DCACHE_CLONE_LOCAL)
/*
 * rw_clone_sect() - read/write the device cache file clone section
 */

static int rw_clone_sect(int mode) {
    char buf[MAXPATHLEN * 2], *cp, *cp1;
    struct clone *c;
    struct l_dev *dp;
    int i, j, len, n;

    if (mode == 1) {

        /*
     * Read the clone section header and validate it.
     */
        if (!fgets(buf, sizeof(buf), DevCacheStream)) {

        bad_clone_sect:
            if (!OptWarnings) {
                (void)fprintf(stderr, "%s: bad clone section header in %s: line ", ProgramName,
                              DevCachePath[DevCachePathIndex]);
                safestrprt(buf, stderr, 1 + 4 + 8);
            }
            return (1);
        }
        (void)crc(buf, strlen(buf), &DevCacheChecksum);
        len = strlen("clone section: ");
        if (strncmp(buf, "clone section: ", len) != 0)
            goto bad_clone_sect;
        if ((n = atoi(&buf[len])) < 0)
            goto bad_clone_sect;
        /*
     * Read the clone section lines and create the Clone list.
     */
        for (i = 0; i < n; i++) {
            if (fgets(buf, sizeof(buf), DevCacheStream) == NULL) {
                if (!OptWarnings) {
                    (void)fprintf(stderr, "%s: no %d clone line in %s: line ", ProgramName, i + 1,
                                  DevCachePath[DevCachePathIndex]);
                    safestrprt(buf, stderr, 1 + 4 + 8);
                }
                return (1);
            }
            (void)crc(buf, strlen(buf), &DevCacheChecksum);
            /*
         * Assemble DeviceTable[] index and make sure it's correct.
         */
            for (cp = buf, j = 0; *cp != ' '; cp++) {
                if (*cp < '0' || *cp > '9') {

                bad_clone_index:
                    if (!OptWarnings) {
                        (void)fprintf(stderr, "%s: clone %d: bad cached device index: line ",
                                      ProgramName, i + 1);
                        safestrprt(buf, stderr, 1 + 4 + 8);
                    }
                    return (1);
                }
                j = (j * 10) + (int)(*cp - '0');
            }
            if (j < 0 || j >= NumDevices || (cp1 = strchr(++cp, '\n')) == NULL)
                goto bad_clone_index;
            if (strncmp(cp, DeviceTable[j].name, (cp1 - cp)) != 0)
                goto bad_clone_index;
            /*
         * Allocate and complete a clone structure.
         */
            if (!(c = (struct clone *)malloc(sizeof(struct clone)))) {
                (void)fprintf(stderr, "%s: clone %d: no space for cached clone: line ", ProgramName,
                              i + 1);
                safestrprt(buf, stderr, 1 + 4 + 8);
                Exit(1);
            }
            c->dx = j;
            c->next = Clone;
            Clone = c;
        }
        return (0);
    } else if (mode == 2) {

        /*
     * Write the clone section header.
     */
        for (c = Clone, n = 0; c; c = c->next, n++)
            ;
        (void)snpf(buf, sizeof(buf), "clone section: %d\n", n);
        if (wr2DCfd(buf, &DevCacheChecksum))
            return (1);
        /*
     * Write the clone section lines.
     */
        for (c = Clone; c; c = c->next) {
            for (dp = &DeviceTable[c->dx], j = 0; j < NumDevices; j++) {
                if (dp == SortedDevices[j])
                    break;
            }
            if (j >= NumDevices) {
                if (!OptWarnings) {
                    (void)fprintf(stderr, "%s: can't make index for clone: ", ProgramName);
                    safestrprt(dp->name, stderr, 1);
                }
                (void)unlink(DevCachePath[DevCachePathIndex]);
                (void)close(DevCacheFd);
                DevCacheFd = -1;
                return (1);
            }
            (void)snpf(buf, sizeof(buf), "%d %s\n", j, dp->name);
            if (wr2DCfd(buf, &DevCacheChecksum))
                return (1);
        }
        return (0);
    }
    /*
 * A shouldn't-happen case: mode neither 1 nor 2.
 */
    (void)fprintf(stderr, "%s: internal rw_clone_sect error: %d\n", ProgramName, mode);
    Exit(1);
    return (1); /* This useless return(1) keeps some
				 * compilers happy. */
}
#endif /* defined(DCACHE_CLONE_LOCAL) */

/*
 * write_dcache() - write device cache file
 */

void write_dcache() {
    char buf[MAXPATHLEN * 2], *cp;
    struct l_dev *dp;
    int i;
    struct stat sb;
    /*
 * Open the cache file; set up the CRC table; write the section count.
 */
    if (open_dcache(2, 0, &sb))
        return;
    i = 1;
    cp = "";

#if defined(HASBLKDEV)
    i++;
    cp = "s";
#endif /* defined(HASBLKDEV) */

#if defined(DCACHE_CLONE)
    i++;
    cp = "s";
#endif /* defined(DCACHE_CLONE) */

#if defined(DCACHE_PSEUDO)
    i++;
    cp = "s";
#endif /* defined(DCACHE_PSEUDO) */

    (void)snpf(buf, sizeof(buf), "%d section%s, dev=%lx\n", i, cp, (long)DeviceOfDev);
    (void)crcbld();
    DevCacheChecksum = 0;
    if (wr2DCfd(buf, &DevCacheChecksum))
        return;
    /*
 * Write the device section from the contents of SortedDevices[] and DeviceTable[].
 */
    (void)snpf(buf, sizeof(buf), "device section: %d\n", NumDevices);
    if (wr2DCfd(buf, &DevCacheChecksum))
        return;
    for (i = 0; i < NumDevices; i++) {
        dp = SortedDevices[i];
        (void)snpf(buf, sizeof(buf), "%lx %ld %s\n", (long)dp->rdev, (long)dp->inode, dp->name);
        if (wr2DCfd(buf, &DevCacheChecksum))
            return;
    }

#if defined(HASBLKDEV)
    /*
 * Write the block device section from the contents of BlockSortedDevices[] and BlockDeviceTable[].
 */
    (void)snpf(buf, sizeof(buf), "block device section: %d\n", BlockNumDevices);
    if (wr2DCfd(buf, &DevCacheChecksum))
        return;
    if (BlockNumDevices) {
        for (i = 0; i < BlockNumDevices; i++) {
            dp = BlockSortedDevices[i];
            (void)snpf(buf, sizeof(buf), "%lx %ld %s\n", (long)dp->rdev, (long)dp->inode, dp->name);
            if (wr2DCfd(buf, &DevCacheChecksum))
                return;
        }
    }
#endif /* defined(HASBLKDEV) */

#if defined(DCACHE_CLONE)
    /*
 * Write the clone section.
 */
    if (DCACHE_CLONE(2))
        return;
#endif /* defined(DCACHE_CLONE) */

#if defined(DCACHE_PSEUDO)
    /*
 * Write the pseudo section.
 */
    if (DCACHE_PSEUDO(2))
        return;
#endif /* defined(DCACHE_PSEUDO) */

    /*
 * Write the CRC section and close the file.
 */
    (void)snpf(buf, sizeof(buf), "CRC section: %x\n", DevCacheChecksum);
    if (wr2DCfd(buf, (unsigned *)NULL))
        return;
    if (close(DevCacheFd) != 0) {
        if (!OptWarnings)
            (void)fprintf(stderr, "%s: WARNING: can't close %s: %s\n", ProgramName,
                          DevCachePath[DevCachePathIndex], strerror(errno));
        (void)unlink(DevCachePath[DevCachePathIndex]);
        DevCacheFd = -1;
    }
    DevCacheFd = -1;
    /*
 * If the previous reading of the previous device cache file marked it
 * "unsafe," drop that marking and record that the device cache file was
 * rebuilt.
 */
    if (DevCacheUnsafe) {
        DevCacheUnsafe = 0;
        DevCacheRebuilt = 1;
    }
}

/*
 * wr2DCfd() - write to the DevCacheFd file descriptor
 */

int wr2DCfd(char *buf, unsigned *cnt) {
    int bl, bw;

    bl = strlen(buf);
    if (cnt)
        (void)crc(buf, bl, cnt);
    while (bl > 0) {
        if ((bw = write(DevCacheFd, buf, bl)) < 0) {
            if (!OptWarnings)
                (void)fprintf(stderr, "%s: WARNING: can't write to %s: %s\n", ProgramName,
                              DevCachePath[DevCachePathIndex], strerror(errno));
            (void)unlink(DevCachePath[DevCachePathIndex]);
            (void)close(DevCacheFd);
            DevCacheFd = -1;
            return (1);
        }
        buf += bw;
        bl -= bw;
    }
    return (0);
}
#else  /* !defined(HASDCACHE) */
char dvch_d1[] = "d";
char *dvch_d2 = dvch_d1;
#endif /* defined(HASDCACHE) */
