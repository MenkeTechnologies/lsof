/*
 * isfn.c -- is_file_named() function for lsof library
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
 * To use this source file:
 *
 * 1. Define USE_LIB_IS_FILE_NAMED.
 *
 * 2. If clone support is required:
 *
 *    a.  Define HAVECLONEMAJ to be the name of the variable that
 *	  contains the status of the clone major device -- e.g.,
 *
 *		#define HAVECLONEMAJ HaveCloneMaj
 *
 *    b.  Define CLONEMAJ to be the name of the constant or
 *	  variable that defines the clone major device -- e.g.,
 *
 *		#define CLONEMAJ CloneMaj
 *
 *    c.  Make sure that clone devices are identified by an lfile
 *	  element is_stream value of 1.
 *
 *    d.  Accept clone searching by device number only.
 */

#include "../machine.h"

#if defined(USE_LIB_IS_FILE_NAMED)

#include "../lsof.h"

/*
 * Local structures
 */

struct hsfile {
    struct sfile *s;     /* the SearchFileChain table address */
    struct hsfile *next; /* the next hash bucket entry */
};

/*
 * Local static variables
 */

#if defined(HAVECLONEMAJ)
static struct hsfile *HbyCd = /* hash by clone buckets */
    (struct hsfile *)NULL;
static int HbyCdCt = 0; /* HbyCd entry count */
#endif                  /* defined(HAVECLONEMAJ) */

static struct hsfile *HbyFdi = /* hash by file (dev,ino) buckets */
    (struct hsfile *)NULL;
static int HbyFdiCt = 0;       /* HbyFdi entry count */
static struct hsfile *HbyFrd = /* hash by file raw device buckets */
    (struct hsfile *)NULL;
static int HbyFrdCt = 0;       /* HbyFrd entry count */
static struct hsfile *HbyFsd = /* hash by file system buckets */
    (struct hsfile *)NULL;
static int HbyFsdCt = 0;      /* HbyFsd entry count */
static struct hsfile *HbyNm = /* hash by name buckets */
    (struct hsfile *)NULL;
static int HbyNmCt = 0; /* HbyNm entry count */

/*
 * Local definitions
 */

#if defined(HAVECLONEMAJ)
#define SFCDHASH \
    1024 /* SearchFileChain hash by clone device (power
                     * of 2!) */
#endif   /* defined(HAVECLONEMAJ) */

#define SFDIHASH \
    4094 /* SearchFileChain hash by (device,inode) number
                     * pair bucket count (power of 2!) */
#define SFFSHASH \
    1024 /* SearchFileChain hash by file system device
                     * number bucket count (power of 2!) */
#define SFHASHDEVINO(maj, min, ino, mod) \
    ((int)(((int)((((int)(maj + 1)) * ((int)((min + 1)))) + ino) * 31415) & (mod - 1)))
/* hash for SearchFileChain by major device,
                     * minor device, and inode, modulo mod
                     * (mod must be a power of 2) */
#define SFRDHASH \
    1024 /* SearchFileChain hash by raw device number
                     * bucket count (power of 2!) */
#define SFHASHRDEVI(maj, min, rmaj, rmin, ino, mod)                                               \
    ((int)(((int)((((int)(maj + 1)) * ((int)((min + 1)))) + ((int)(rmaj + 1) * (int)(rmin + 1)) + \
                  ino) *                                                                          \
            31415) &                                                                              \
           (mod - 1)))
/* hash for SearchFileChain by major device,
                     * minor device, major raw device,
                     * minor raw device, and inode, modulo
                     * mod (mod must be a power of 2) */
#define SFNMHASH \
    4096 /* SearchFileChain hash by name bucket count
                     * (must be a power of 2!) */

/*
 * hashSfile() - hash SearchFileChain entries for use in is_file_named() searches
 */

void hashSfile() {
    static int hs = 0;
    int i;
    int sfplm = 3;
    struct sfile *s;
    struct hsfile *sh, *sn;
    /*
 * Do nothing if there are no file search arguments cached or if the
 * hashes have already been constructed.
 */
    if (!SearchFileChain || hs)
        return;
    /*
 * Allocate hash buckets by (device,inode), file system device, and file name.
 */

#if defined(HAVECLONEMAJ)
    if (HAVECLONEMAJ) {
        if (!(HbyCd = (struct hsfile *)calloc((MALLOC_S)SFCDHASH, sizeof(struct hsfile)))) {
            (void)fprintf(stderr, "%s: can't allocate space for %d clone hash buckets\n",
                          ProgramName, SFCDHASH);
            Exit(1);
        }
        sfplm++;
    }
#endif /* defined(HAVECLONEMAJ) */

    if (!(HbyFdi = (struct hsfile *)calloc((MALLOC_S)SFDIHASH, sizeof(struct hsfile)))) {
        (void)fprintf(stderr, "%s: can't allocate space for %d (dev,ino) hash buckets\n",
                      ProgramName, SFDIHASH);
        Exit(1);
    }
    if (!(HbyFrd = (struct hsfile *)calloc((MALLOC_S)SFRDHASH, sizeof(struct hsfile)))) {
        (void)fprintf(stderr, "%s: can't allocate space for %d rdev hash buckets\n", ProgramName,
                      SFRDHASH);
        Exit(1);
    }
    if (!(HbyFsd = (struct hsfile *)calloc((MALLOC_S)SFFSHASH, sizeof(struct hsfile)))) {
        (void)fprintf(stderr, "%s: can't allocate space for %d file sys hash buckets\n",
                      ProgramName, SFFSHASH);
        Exit(1);
    }
    if (!(HbyNm = (struct hsfile *)calloc((MALLOC_S)SFNMHASH, sizeof(struct hsfile)))) {
        (void)fprintf(stderr, "%s: can't allocate space for %d name hash buckets\n", ProgramName,
                      SFNMHASH);
        Exit(1);
    }
    hs++;
    /*
 * Scan the SearchFileChain chain, building file, file system, raw device, and file
 * name hash bucket chains.
 */
    for (s = SearchFileChain; s; s = s->next) {
        for (i = 0; i < sfplm; i++) {
            if (i == 0) {
                if (!s->aname)
                    continue;
                sh = &HbyNm[hashbyname(s->aname, SFNMHASH)];
                HbyNmCt++;
            } else if (i == 1) {
                if (s->type) {
                    sh = &HbyFdi[SFHASHDEVINO(GET_MAJ_DEV(s->dev), GET_MIN_DEV(s->dev), s->i,
                                              SFDIHASH)];
                    HbyFdiCt++;
                } else {
                    sh = &HbyFsd[SFHASHDEVINO(GET_MAJ_DEV(s->dev), GET_MIN_DEV(s->dev), 0,
                                              SFFSHASH)];
                    HbyFsdCt++;
                }
            } else if (i == 2) {
                if ((s->mode == S_IFCHR) || (s->mode == S_IFBLK)) {
                    sh = &HbyFrd[SFHASHRDEVI(GET_MAJ_DEV(s->dev), GET_MIN_DEV(s->dev),
                                             GET_MAJ_DEV(s->rdev), GET_MIN_DEV(s->rdev), s->i,
                                             SFRDHASH)];
                    HbyFrdCt++;
                } else
                    continue;
            }

#if defined(HAVECLONEMAJ)
            else {
                if (!HAVECLONEMAJ || (GET_MAJ_DEV(s->rdev) != CLONEMAJ))
                    continue;
                sh = &HbyCd[SFHASHDEVINO(0, GET_MIN_DEV(s->rdev), 0, SFCDHASH)];
                HbyCdCt++;
            }
#else  /* ! defined(HAVECLONEMAJ) */
            else
                continue;
#endif /* defined(HAVECLONEMAJ) */

            if (!sh->s) {
                sh->s = s;
                sh->next = (struct hsfile *)NULL;
                continue;
            } else {
                if (!(sn = (struct hsfile *)malloc((MALLOC_S)sizeof(struct hsfile)))) {
                    (void)fprintf(stderr, "%s: can't allocate hsfile bucket for: %s\n", ProgramName,
                                  s->aname);
                    Exit(1);
                }
                sn->s = s;
                sn->next = sh->next;
                sh->next = sn;
            }
        }
    }
}

/*
 * is_file_named() - is this file named?
 */

int is_file_named(char *path, int cdev) {
    char *ep;
    int found = 0;
    struct sfile *s = (struct sfile *)NULL;
    struct hsfile *sh;
    size_t sz;
    /*
 * Check for a path name match, as requested.
 */
    if (path && HbyNmCt) {
        for (sh = &HbyNm[hashbyname(path, SFNMHASH)]; sh; sh = sh->next) {
            if ((s = sh->s) && strcmp(path, s->aname) == 0) {
                found = 2;
                break;
            }
        }
    }

#if defined(HAVECLONEMAJ)
    /*
 * If this is a stream, check for a clone device match.
 */
    if (!found && HbyCdCt && CurrentLocalFile->is_stream && CurrentLocalFile->dev_def &&
        CurrentLocalFile->rdev_def && (CurrentLocalFile->dev == DeviceOfDev)) {
        for (sh = &HbyCd[SFHASHDEVINO(0, GET_MAJ_DEV(CurrentLocalFile->rdev), 0, SFCDHASH)]; sh;
             sh = sh->next) {
            if ((s = sh->s) && (GET_MAJ_DEV(CurrentLocalFile->rdev) == GET_MIN_DEV(s->rdev))) {
                found = 3;
                break;
            }
        }
    }
#endif /* defined(HAVECLONEMAJ) */

    /*
 * Check for a regular file.
 */
    if (!found && HbyFdiCt && CurrentLocalFile->dev_def &&
        (CurrentLocalFile->inp_ty == 1 || CurrentLocalFile->inp_ty == 3)) {
        for (sh = &HbyFdi[SFHASHDEVINO(GET_MAJ_DEV(CurrentLocalFile->dev),
                                       GET_MIN_DEV(CurrentLocalFile->dev), CurrentLocalFile->inode,
                                       SFDIHASH)];
             sh; sh = sh->next) {
            if ((s = sh->s) && (CurrentLocalFile->dev == s->dev) &&
                (CurrentLocalFile->inode == s->i)) {
                found = 1;
                break;
            }
        }
    }
    /*
 * Check for a file system match.
 */
    if (!found && HbyFsdCt && CurrentLocalFile->dev_def) {
        for (sh = &HbyFsd[SFHASHDEVINO(GET_MAJ_DEV(CurrentLocalFile->dev),
                                       GET_MIN_DEV(CurrentLocalFile->dev), 0, SFFSHASH)];
             sh; sh = sh->next) {
            if ((s = sh->s) && (s->dev == CurrentLocalFile->dev)) {
                found = 1;
                break;
            }
        }
    }
    /*
 * Check for a character or block device match.
 */
    if (!found && HbyFrdCt && cdev && CurrentLocalFile->dev_def &&
        (CurrentLocalFile->dev == DeviceOfDev) && CurrentLocalFile->rdev_def &&
        (CurrentLocalFile->inp_ty == 1 || CurrentLocalFile->inp_ty == 3)) {
        for (sh = &HbyFrd[SFHASHRDEVI(
                 GET_MAJ_DEV(CurrentLocalFile->dev), GET_MIN_DEV(CurrentLocalFile->dev),
                 GET_MAJ_DEV(CurrentLocalFile->rdev), GET_MIN_DEV(CurrentLocalFile->rdev),
                 CurrentLocalFile->inode, SFRDHASH)];
             sh; sh = sh->next) {
            if ((s = sh->s) && (s->dev == CurrentLocalFile->dev) &&
                (s->rdev == CurrentLocalFile->rdev) && (s->i == CurrentLocalFile->inode)) {
                found = 1;
                break;
            }
        }
    }
    /*
 * Convert the name if a match occurred.
 */
    switch (found) {
    case 0:
        return (0);
    case 1:
        if (s->type) {

            /*
         * If the search argument isn't a file system, propagate it
         * to NameChars[]; otherwise, let printname() compose the name.
         */
            (void)snpf(NameChars, NameCharsLength, "%s", s->name);
            if (s->devnm) {
                ep = endnm(&sz);
                (void)snpf(ep, sz, " (%s)", s->devnm);
            }
        }
        break;
    case 2:
        (void)strcpy(NameChars, path);
        break;

#if defined(HAVECLONEMAJ)
        /* case 3:		do nothing for stream clone matches */
#endif /* defined(HAVECLONEMAJ) */
    }
    if (s)
        s->f = 1;
    return (1);
}
#else  /* !defined(USE_LIB_IS_FILE_NAMED) */
char isfn_d1[] = "d";
char *isfn_d2 = isfn_d1;
#endif /* defined(USE_LIB_IS_FILE_NAMED) */
