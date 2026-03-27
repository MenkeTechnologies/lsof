/*
 * lsof.h - common header file for lsof
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

#ifndef LSOF_H
#define LSOF_H 1

#include "machine.h"

#ifndef FSV_DEFAULT
#define FSV_DEFAULT 0
#endif

#include "lsof_fields.h"

#include <ctype.h>
#include <errno.h>

#if defined(HASSETLOCALE)
#include <locale.h>
#endif

#include <netdb.h>
#include <pwd.h>
#include <stdio.h>

#include <sys/stat.h>
#include <sys/wait.h>

/*
 * Definitions and structures that may be needed by dlsof.h
 */

#ifndef INODETYPE
#define INODETYPE unsigned long /* node number storage type */
#define INODEPSPEC \
    "l" /* node number printf specification
					 * modifier */
#endif

struct l_dev {
    dev_t rdev;      /* device */
    INODETYPE inode; /* inode number */
    char *name;      /* name */
    int v;           /* has been verified
					 * (when DCUnsafe == 1) */
};

/*
 * FILE_FLAG column names
 */

#define FF_AIO          "AIO"
#define FF_APPEND       "AP"
#define FF_ASYNC        "ASYN"
#define FF_BLKANDSET    "BAS"
#define FF_BLKINUSE     "BKIU"
#define FF_BLKSEEK      "BSK"
#define FF_CIO          "CIO"
#define FF_CLONE        "CLON"
#define FF_CLREAD       "CLRD"
#define FF_COPYAVOID    "CA"
#define FF_CREAT        "CR"
#define FF_DATAFLUSH    "DFLU"
#define FF_DEFER        "DF"
#define FF_DEFERIND     "DFI"
#define FF_DELAY        "DLY"
#define FF_DIRECT       "DIR"
#define FF_DIRECTORY    "DTY"
#define FF_DOCLONE      "DOCL"
#define FF_DSYNC        "DSYN"
#define FF_EVTONLY      "EVO"
#define FF_EXCL         "EXCL"
#define FF_EXEC         "EX"
#define FF_EXLOCK       "XL"
#define FF_FILE_MBLK    "MBLK"
#define FF_FSYNC        "FSYN"
#define FF_GCFDEFER     "GCDF"
#define FF_GCFMARK      "GCMK"
#define FF_GENTTY       "GTTY"
#define FF_HASLOCK      "LCK"
#define FF_HUP          "HUP"
#define FF_KERNEL       "KERN"
#define FF_KIOCTL       "KIOC"
#define FF_LARGEFILE    "LG"
#define FF_MARK         "MK"
#define FF_MOUNT        "MNT"
#define FF_MSYNC        "MSYN"
#define FF_NBDRM        "NBDR"
#define FF_NBIO         "NBIO"
#define FF_NBLOCK       "NB"
#define FF_NBUF         "NBF"
#define FF_NMFS         "NMFS"
#define FF_NDELAY       "ND"
#define FF_NET          "NET"
#define FF_NOATM        "NATM"
#define FF_NOCACHE      "NC"
#define FF_NOCTTY       "NTTY"
#define FF_NODSYNC      "NDSY"
#define FF_NOFOLNK      "NFLK"
#define FF_NOTOSTOP     "NOTO"
#define FF_NSHARE       "NSH"
#define FF_OLRMIRROR    "OLRM"
#define FF_POSIX_AIO    "PAIO"
#define FF_POSIX_PIPE   "PP"
#define FF_RAIOSIG      "RAIO"
#define FF_RCACH        "RC"
#define FF_RDWR         "RW"
#define FF_READ         "R"
#define FF_REVOKED      "REV"
#define FF_RSHARE       "RSH"
#define FF_RSYNC        "RSYN"
#define FF_SETBLK       "BL"
#define FF_SHLOCK       "SL"
#define FF_SNAP         "SNAP"
#define FF_SOCKET       "SOCK"
#define FF_SQTSH1       "SQS1"
#define FF_SQTSH2       "SQS2"
#define FF_SQTREPAIR    "SQR"
#define FF_SQTSH        "SQSH"
#define FF_SQTSVM       "SQSV"
#define FF_STOPIO       "STPI"
#define FF_SYNC         "SYN"
#define FF_SYNCRON      "SWR"
#define FF_TCP_MDEVONLY "TCPM"
#define FF_TERMIO       "TIO"
#define FF_TRUNC        "TR"
#define FF_VHANGUP      "VH"
#define FF_VTEXT        "VTXT"
#define FF_WAKEUP       "WKUP"
#define FF_WAITING      "WTG"
#define FF_WRITE        "W"

/*
 * Process open file flag names
 */

#define POF_ALLOCATED "ALLC"
#define POF_BNRD      "BR"
#define POF_BNWR      "BW"
#define POF_BNHUP     "BHUP"
#define POF_CLOEXEC   "CX"
#define POF_CLOSING   "CLSG"
#define POF_FDLOCK    "LCK"
#define POF_INUSE     "USE"
#define POF_MAPPED    "MP"
#define POF_FSHMAT    "SHMT"
#define POF_RESERVED  "OPIP"
#define POF_RSVWT     "RSVW"

/*
 * Cross-over (-x) option values
 */

#define XO_FILESYS 0x1 /* file system mount points */
#define XO_SYMLINK 0x2 /* symbolic links */
#define XO_ALL     (XO_FILESYS | XO_SYMLINK)

#include "dlsof.h"

#include <sys/types.h> /* just in case -- because utmp.h
					 * may need it */
#include "./regex.h"

#if defined(EMPTY)
#undef EMPTY
#endif

#if defined(HASUTMPX)
#include <utmpx.h>
#else

#include <utmp.h>

#endif

extern int errno;
extern char *optarg;
extern int optind;

#define ACCESSERRFMT "%s: WARNING: access %s: %s\n"

#if defined(HASDCACHE)
#define CRC_POLY 0120001 /* CRC-16 polynomial */
#define CRC_TBLL 256     /* crc table length for software */
#define CRC_BITS 8       /* number of bits contributing */
#endif
#define CMDL \
    9                         /* maximum number of characters from
					 * command name to print in COMMAND
					 * column */
#define CWD            " cwd" /* current working directory fd name */
#define FDLEN          8      /* fd printing array length */
#define FSV_FILE_ADDR  0x1    /* file struct addr status */
#define FSV_FILE_COUNT 0x2    /* file struct count status */
#define FSV_FILE_FLAGS 0x4    /* file struct flags */
#define FSV_NODE_ID    0x8    /* file struct node ID status */

#ifndef GET_MAJ_DEV
#define GET_MAJ_DEV \
    major /* if no dialect specific macro has
					 * been defined, use standard major()
					 * macro */
#endif

#ifndef GET_MIN_DEV
#define GET_MIN_DEV \
    minor /* if no dialect specific macro has
					 * been defined, use standard minor()
					 * macro */
#endif

#if defined(HASSELINUX)
#define HASHCNTX \
    128 /* security context hash bucket count
* -- MUST BE A POWER OF 2!!! */
#endif

#if defined(HASZONES)
#define HASHZONE \
    128 /* zone hash bucket count -- MUST BE
* A POWER OF 2!!! */
#endif

#define IDINCR 10 /* PID/PGID table malloc() increment */

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK (u_long)0x7f000001
#endif

#define IPROTOL 8 /* Internet protocol length */

#ifndef KA_T_FMT_X
#define KA_T_FMT_X \
    "0x%08lx" /* format for printing kernel
					 * addresses in 0x... format */
#endif

#ifndef LOGINML
#if defined(HASUTMPX)
static struct utmpx dummy_utmp; /* to get login name length */
#define LOGINML sizeof(dummy_utmp.ut_user)
/* login name length */
#else /* !defined(HASUTMPX) */
static struct utmp dummy_utmp; /* to get login name length */
#define LOGINML sizeof(dummy_utmp.ut_name)
/* login name length */
#endif /* defined(HASUTMPX) */
#endif

#define LPROCINCR   128 /* LocalProcTable[] allocation increment */
#define LSOF_URL    "https://github.com/lsof-org/lsof"
#define MIN_AF_ADDR sizeof(struct in_addr)
/* minimum AF_* address length */

#if defined(HASIPv6)
#define MAX_AF_ADDR sizeof(struct in6_addr)
/* maximum AF_* address length */
#else
#define MAX_AF_ADDR MIN_AF_ADDR /* maximum AF_* address length */
#endif

#define MAXDCPATH 4   /* paths in DevCachePath[] */
#define MAXNWAD   100 /* maximum network addresses */

#ifndef MEMMOVE
#define MEMMOVE memmove
#endif

#define N_REGLR    0  /* regular file system node */
#define N_AFS      1  /* AFS node */
#define N_AFPFS    2  /* Apple Darwin AppleShare */
#define N_AUSX     3  /* Auspex LFS node */
#define N_AUTO     4  /* automount node */
#define N_BLK      5  /* block device node */
#define N_CACHE    6  /* cached file system node */
#define N_CDFS     7  /* CD-ROM node */
#define N_CFS      8  /* CFS node */
#define N_CHR      9  /* character device node */
#define N_COM      10 /* streams common device node */
#define N_CTFSADIR 11 /* Solaris CTFS adir node */
#define N_CTFSBUND 12 /* Solaris CTFS bundle node */
#define N_CTFSCDIR 13 /* Solaris CTFS cdir node */
#define N_CTFSCTL  14 /* Solaris CTFS ctl node */
#define N_CTFSEVT  15 /* Solaris CTFS events node */
#define N_CTFSLATE 16 /* Solaris CTFS latest node */
#define N_CTFSROOT 17 /* Solaris CTFS root node */
#define N_CTFSSTAT 18 /* Solaris CTFS status node */
#define N_CTFSSYM  19 /* Solaris CTFS symbolic node */
#define N_CTFSTDIR 20 /* Solaris CTFS type node */
#define N_CTFSTMPL 21 /* Solaris CTFS template node */
#define N_DEV      22 /* DEV FS node */
#define N_DOOR     23 /* DOOR node */
#define N_FD       24 /* FD node */
#define N_FIFO     25 /* FIFO node */
#define N_HSFS     26 /* High Sierra node */
#define N_KERN     27 /* BSD /kern node */
#define N_LOFS     28 /* loopback node */
#define N_MNT      29 /* mount file system device node */
#define N_MPC      30 /* multiplexed device node */
#define N_MVFS     31 /* multi-volume file system node (?) */
#define N_NFS      32 /* NFS node */
#define N_NFS4     33 /* NFS version 4 node */
#define N_NM       34 /* named file system node */
#define N_OBJF     35 /* objfs file system node */
#define N_PCFS     36 /* PC file system node */
#define N_PIPE     37 /* pipe device node */
#define N_PORT     38 /* port node */
#define N_PROC     39 /* /proc node */
#define N_PSEU     49 /* pseudofs node */
#define N_SAMFS    41 /* Solaris SAM-FS */
#define N_SANFS    42 /* AIX SANFS */
#define N_SDEV     43 /* Solaris sdev file system node */
#define N_SHARED   44 /* Solaris sharedfs */
#define N_SOCK     45 /* sock_vnodeops node */
#define N_SPEC     46 /* spec_vnodeops node */
#define N_STREAM   47 /* stream node */
#define N_TMP      48 /* tmpfs node */
#define N_UFS      49 /* UNIX file system node */
#define N_UNKN     50 /* unknown node type */
#define N_VXFS     51 /* Veritas file system node */
#define N_XFS      52 /* XFS node */
#define N_ZFS      53 /* ZFS node */

#ifndef OFFDECDIG
#define OFFDECDIG \
    8 /* maximum number of digits in the
					 * offset decimal form (0t...) */
#endif

#ifndef USELOCALREADDIR
#define CloseDir closedir /* use standard closedir() */
#define OpenDir  opendir  /* use standard opendir() */
#define ReadDir  readdir  /* use standard readdir() */
#endif

#define RPTTM 15     /* default repeat seconds */
#define RTD   " rtd" /* root directory fd name */
#define TCPTPI_FLAGS \
    0x0001                    /* report TCP/TPI socket options and
					 * state, and TCP_NODELAY state */
#define TCPTPI_QUEUES  0x0002 /* report TCP/TPI queue lengths */
#define TCPTPI_STATE   0x0004 /* report TCP/TPI state */
#define TCPTPI_WINDOWS 0x0008 /* report TCP/TPI window sizes */
#define TCPTPI_ALL     (TCPTPI_QUEUES | TCPTPI_STATE | TCPTPI_WINDOWS)
/* report all TCP/TPI info */
#define TCPUDPALLOC \
    32                 /* allocation amount for TCP and UDP
					 * state tables */
#define TMLIMIT   15   /* readlink() & stat() timeout sec */
#define TMLIMMIN  2    /* minimum timeout */
#define TYPEL     8    /* type character length */
#define UIDCACHEL 1024 /* UID cache length */
#define UIDINCR   10   /* UID table malloc() increment */
#define USERPRTL  8    /* UID/login print length limit */

#ifndef SZOFFTYPE
#define SZOFFTYPE unsigned long /* type for size and offset */
#undef SZOFFPSPEC
#define SZOFFPSPEC \
    "l" /* SZOFFTYPE printf specification
					 * modifier */
#endif

#ifndef TIMEVAL_LSOF
#define TIMEVAL_LSOF timeval
#endif

#ifndef XDR_PMAPLIST
#define XDR_PMAPLIST xdr_pmaplist
#endif

#ifndef XDR_VOID
#define XDR_VOID xdr_void
#endif

/*
 * Output title definitions
 */

/* Cyberpunk TTY detection flag */
extern int CyberpunkTTY;

/* ANSI color codes for cyberpunk theme — empty strings when piped */
#define CP_RESET         (CyberpunkTTY ? "\033[0m" : "")
#define CP_NEON_CYAN     (CyberpunkTTY ? "\033[1;96m" : "")
#define CP_NEON_MAGENTA  (CyberpunkTTY ? "\033[1;95m" : "")
#define CP_NEON_GREEN    (CyberpunkTTY ? "\033[1;92m" : "")
#define CP_NEON_YELLOW   (CyberpunkTTY ? "\033[1;93m" : "")
#define CP_NEON_RED      (CyberpunkTTY ? "\033[1;91m" : "")
#define CP_NEON_BLUE     (CyberpunkTTY ? "\033[1;94m" : "")
#define CP_DIM           (CyberpunkTTY ? "\033[2m" : "")
#define CP_BOLD          (CyberpunkTTY ? "\033[1m" : "")
#define CP_HDR_BG        (CyberpunkTTY ? "\033[48;5;234m" : "")
#define CP_ROW_ALT       (CyberpunkTTY ? "\033[48;5;233m" : "")

/* Column titles — cyberpunk when TTY, plain when piped */
#define CMDTTL   (CyberpunkTTY ? "PROCESS"  : "COMMAND")
#define CNTXTTL  (CyberpunkTTY ? "SEC-CNTX" : "SECURITY-CONTEXT")
#define DEVTTL   (CyberpunkTTY ? "DEV/ICE"  : "DEVICE")
#define FCTTL    "FCT"
#define FDTTL    "FD"
#define FGTTL    (CyberpunkTTY ? "FL4GS"    : "FILE-FLAG")
#define FSTTL    (CyberpunkTTY ? "F-ADDR"   : "FILE-ADDR")
#define NLTTL    (CyberpunkTTY ? "NLNK"     : "NLINK")
#define NMTTL    (CyberpunkTTY ? "T4RGET"   : "NAME")
#define NODETTL  (CyberpunkTTY ? "N0DE"     : "NODE")
#define OFFTTL   (CyberpunkTTY ? "0FFSET"   : "OFFSET")
#define PGIDTTL  "PGID"
#define PIDTTL   (CyberpunkTTY ? "PRC"      : "PID")
#define PPIDTTL  (CyberpunkTTY ? "PPRC"     : "PPID")
#define SZTTL    (CyberpunkTTY ? "BYT3S"    : "SIZE")
#define SZOFFTTL (CyberpunkTTY ? "BYT3/0FF" : "SIZE/OFF")
#define TIDTTL   (CyberpunkTTY ? "THR"      : "TID")
#define TYPETTL  (CyberpunkTTY ? "CL4SS"    : "TYPE")
#define USERTTL  (CyberpunkTTY ? "H4XOR"    : "USER")
#define ZONETTL  (CyberpunkTTY ? "Z0NE"     : "ZONE")

extern int CommandColWidth;
extern int ContextColWidth;
extern int DeviceColWidth;
extern int FileCountColWidth;
extern int FileDescColWidth;
extern int FileFlagColWidth;
extern int FileStructAddrColWidth;
#define NODE_ID_TITLE (CyberpunkTTY ? "NOD3-ID" : "NODE-ID")
extern int NodeIdColWidth;
extern char *NodeIdTitle;
extern int LinkCountColWidth;
extern int NameColWidth;
extern int NodeColWidth;
extern int PgidColWidth;
extern int PidColWidth;
extern int PpidColWidth;
extern int SizeOffColWidth;
extern int TidColWidth;
extern int TypeColWidth;
extern int UserColWidth;
extern int ZoneColWidth;

/*
 * Selection flags
 */

#define PS_PRI \
    1 /* primary process selection -- e.g.,
					 * by PID or UID */
#define PS_SEC \
    2                   /* secondary process selection -- e.g.,
					 * by directory or file */
#define SELCMD   0x0001 /* select process by command name */
#define SELCNTX  0x0002 /* select security context (-Z) */
#define SELFD    0x0004 /* select file by descriptor name */
#define SELNA    0x0008 /* select socket by address (-i@...) */
#define SELNET   0x0010 /* select Internet socket files (-i) */
#define SELNFS   0x0020 /* select NFS files (-N) */
#define SELNLINK 0x0040 /* select based on link count */
#define SELNM    0x0080 /* select by name */
#define SELPGID  0x0100 /* select process group IDs (-g) */
#define SELPID   0x0200 /* select PIDs (-p) */
#define SELUID   0x0400 /* select UIDs (-u) */
#define SELUNX   0x0800 /* select UNIX socket (-U) */
#define SELZONE  0x1000 /* select zone (-z) */
#define SELEXCLF 0x2000 /* file selection excluded */
#define SELTASK  0x4000 /* select tasks (-K) */
#define SELALL                                                                               \
    (SELCMD | SELCNTX | SELFD | SELNA | SELNET | SELNM | SELNFS | SELPID | SELUID | SELUNX | \
     SELZONE | SELTASK)
#define SELPROC (SELCMD | SELCNTX | SELPGID | SELPID | SELUID | SELZONE | SELTASK)
/* process selecters */
#define SELFILE (SELFD | SELNFS | SELNLINK | SELNM) /* file selecters */
#define SELNW   (SELNA | SELNET | SELUNX)           /* network selecters */

/*
 * Structure definitions
 */

#if defined(HAS_AFS)
struct afsnode { /* AFS pseudo-node structure */
    dev_t dev;
    unsigned char ino_st;   /* 1 if inode has a value */
    unsigned char nlink_st; /* 1 if nlink has a value */
    INODETYPE inode;
    unsigned long size;
    long nlink;
};
#endif

#if defined(HAS_STD_CLONE)
struct clone {
    int dx;             /* index of device entry in DeviceTable[] */
    struct clone *next; /* forward link */
};
extern struct clone *Clone;
#endif

#if defined(HASNLIST)
struct drive_Nl { /* data to drive build_Nl() */
    char *nn;     /* nickname for lookups */
    char *knm;    /* kernel variable for name list */
};
extern struct drive_Nl Drive_Nl[]; /* defined in dstore.c */
#endif

/*
 * Global storage definitions (including their structure definitions)
 */

typedef struct efsys_list {
    char *path;              /* path to file system for which kernel
					 * blocks are to be eliminated */
    int pathl;               /* path length */
    int rdlnk;               /* avoid readlink(2) if non-zero */
    struct mounts *mp;       /* local mount table entry pointer */
    struct efsys_list *next; /* next efsys_list entry pointer */
} efsys_list_t;
extern efsys_list_t *ExcludedFileSysList; /* file systems for which kernel blocks
					 * are to be eliminated */

struct int_lst {
    int i; /* integer argument */
    int f; /* find state -- meaningful only if
					 * x == 0 */
    int x; /* excluded state */
};

typedef struct lsof_rx { /* regular expression table entry */
    char *exp;           /* original regular expression */
    regex_t cx;          /* compiled expression */
    int mc;              /* match count */
} lsof_rx_t;
extern lsof_rx_t *CommandRegexTable;
extern int NumCommandRegexUsed;

#if defined(HASFSTRUCT)
struct pff_tab { /* print file flags table structure */
    long val;    /* flag value */
    char *nm;    /* name to print for flag */
};
#endif

struct seluid {
    uid_t uid;          /* User ID */
    char *lnm;          /* specified login name (NULL = none) */
    unsigned char excl; /* excluded state */
    unsigned char f;    /* selected User ID find state
					 * (meaningful only if excl == 0) */
};

#if defined(HASBLKDEV)
extern struct l_dev *BlockDeviceTable, **BlockSortedDevices;
extern int BlockNumDevices;
#endif

extern int CheckPasswdChange;

struct str_lst {
    char *str;            /* string */
    int len;              /* string length */
    short f;              /* selected string find state */
    short x;              /* exclusion (if non-zero) */
    struct str_lst *next; /* next list entry */
};
extern struct str_lst *CommandNameList;
extern int CommandColLimit;
extern int CommandNameInclusions;
extern int CommandNameExclusions;

#if defined(HASSELINUX)
typedef struct cntxlist {
    char *cntx;            /* zone name */
    int f;                 /* "find" flag (used only in ContextArgList) */
    struct cntxlist *next; /* next zone hash entry */
} cntxlist_t;
extern cntxlist_t *ContextArgList;
extern int ContextStatus;
#endif

#if defined(HASDCACHE)
extern unsigned DevCacheChecksum;
extern int DevCacheFd;
extern FILE *DevCacheStream;
extern char *DevCachePathArg;
extern char *DevCachePath[];
extern int DevCachePathIndex;
extern int DevCacheRebuilt;
extern int DevCacheState;
extern int DevCacheUnsafe;
#endif

extern int DevCacheHelp;
extern dev_t DeviceOfDev;
extern struct l_dev *DeviceTable;
extern char **DirStack;
extern int DirStackAlloc;
extern int DirStackIndex;
extern int PathStatErrorCount;
extern uid_t EffectiveUid;
extern int OptAndSelection;
extern int OptBlockDevice;
extern int OptSecContext;
extern int OptFieldOutput;
extern int OptFileSystem;
extern int OptHelp;
extern int OptHostLookup;

#if defined(HASNCACHE)
extern int OptNameCache;
extern int NameCacheReload;
#endif

extern int OptNetwork;
extern int OptNetworkType;
extern int OptNfs;
extern int OptLinkCount;
extern int OptOffset;
extern int OptOverhead;
extern int OptPortLookup;

#ifndef HASNORPC_H
extern int OptPortMap;
#endif

extern int OptProcessGroup;
extern int OptParentPid;
extern int OptSize;
extern int OptFileStructValues;
extern int OptFileStructSetByFlag;
extern int OptFileStructFlagHex;
extern int OptTask;
extern int OptTcpTpiInfo;
extern int OptTerse;
extern int OptUnixSocket;
extern int OptUserToLogin;
extern int OptVerbose;
extern int OptWarnings;

#if defined(HASXOPT_VALUE)
extern int OptCrossoverExt;
#endif

extern int OptCrossover;
extern int OptZone;

struct fd_lst {
    char *nm; /* file descriptor name -- range if
					 * NULL */
    int lo;   /* range start (if nm NULL) */
    int hi;   /* range end (if nm NULL) */
    struct fd_lst *next;
};
extern struct fd_lst *FdList;
extern int FdListType; /* FdList[] type: -1 == none
					 *		0 == include
					 *		1 == exclude */

struct fieldsel {
    char id;          /* field ID character */
    unsigned char st; /* field status */
    char *nm;         /* field name */
    int *opt;         /* option variable address */
    int ov;           /* value to OR with option variable */
};
extern struct fieldsel FieldSelection[];

extern int HeaderPrinted;

enum IDType { PGID, PID };
extern char *InodeFormatDecimal;
extern char *InodeFormatHex;

struct lfile {
    char access;
    char lock;
    unsigned char dev_def;   /* device number definition status */
    unsigned char inp_ty;    /* inode/iproto type
					 *	0: neither inode nor iproto
					 *	1: print inode in decimal
					 *	2: iproto contains string
					 *      3: print inode in hex
					 */
    unsigned char is_com;    /* common stream status */
    unsigned char is_nfs;    /* NFS file status */
    unsigned char is_stream; /* stream device status */

#if defined(HASVXFS) && defined(HASVXFSDNLC)
    unsigned char is_vxfs; /* VxFS file status */
#endif

    unsigned char lmi_srch; /* local mount info search status:
					 * 1 = printname() search required */

#if defined(HASMNTSTAT)
    unsigned char mnt_stat; /* mount point stat(2) status */
#endif

    unsigned char nlink_def; /* link count definition status */
    unsigned char off_def;   /* offset definition status */
    unsigned char rdev_def;  /* rdev definition status */
    unsigned char sz_def;    /* size definition status */

#if defined(HASFSTRUCT)
    unsigned char fsv; /* file struct value status */
#endif

    char fd[FDLEN];
    char iproto[IPROTOL];
    char type[TYPEL];
    short sel_flags; /* select flags -- SEL* symbols */
    int channel;     /* VMPC channel: -1 = none */
    int ntype;       /* node type -- N_* value */
    SZOFFTYPE off;
    SZOFFTYPE sz;
    dev_t dev;
    dev_t rdev;
    INODETYPE inode;
    long nlink; /* link count */
    char *dev_ch;
    char *fsdir; /* file system directory */
    char *fsdev; /* file system device */

#if defined(HASFSINO)
    INODETYPE fs_ino; /* file system inode number */
#endif

    struct linaddr {     /* local Internet address information */
        int addr_family; /* address family: 0 for none; AF_INET;
					 * or AF_INET6 */
        int port;        /* port */
        union {
            struct in_addr a4; /* AF_INET Internet address */

#if defined(HASIPv6)
            struct in6_addr a6; /* AF_INET6 Internet address */
#endif

        } ia;
    } li[2];         /* li[0]: local
					 * li[1]: foreign */
    struct ltstate { /* local TCP/TPI state */
        int type;    /* state type:
					 *   -1 == none
					 *    0 == TCP
					 *    1 == TPI or socket (SS_*) */
        union {
            int val;           /* integer state */
            unsigned int uval; /* unsigned integer state */
        } state;

#if defined(HASSOOPT)
        unsigned char pqlens; /* pqlen status: 0 = none */
        unsigned char qlens;  /* qlen status:  0 = none */
        unsigned char qlims;  /* qlim status:  0 = none */
        unsigned char rbszs;  /* rbsz status:  0 = none */
        unsigned char sbszs;  /* sbsz status:  0 = none */
        int kai;              /* TCP keep-alive interval */
        int ltm;              /* TCP linger time */
        unsigned int opt;     /* socket options */
        unsigned int pqlen;   /* partial connection queue length */
        unsigned int qlen;    /* connection queue length */
        unsigned int qlim;    /* connection queue limit */
        unsigned long rbsz;   /* receive buffer size */
        unsigned long sbsz;   /* send buffer size */
#endif

#if defined(HASSOSTATE)
        unsigned int sock_state; /* socket state */
#if defined(HASSBSTATE)
        unsigned int sbs_rcv; /* receive socket buffer state */
        unsigned int sbs_snd; /* send socket buffer state */
#endif                        /* defined(HASSBSTATE) */
#endif

#if defined(HASTCPOPT)
        unsigned int topt;  /* TCP options */
        unsigned char msss; /* mss status: 0 = none */
        unsigned long mss;  /* TCP maximum segment size */
#endif

#if defined(HASTCPTPIQ)
        unsigned long recv_queue;    /* receive queue length */
        unsigned long send_queue;    /* send queue length */
        unsigned char recv_queue_st; /* recv_queue status: 0 = none */
        unsigned char send_queue_st; /* send_queue status: 0 = none */
#endif

#if defined(HASTCPTPIW)
        unsigned char read_win_st;  /* read_win status: 0 = none */
        unsigned char write_win_st; /* write_win status: 0 = none */
        unsigned long read_win;     /* read window size */
        unsigned long write_win;    /* write window size */
#endif

    } lts;
    char *name;
    char *name_append; /* NAME column addition */

#if defined(HASNCACHE) && HASNCACHE < 2
    KA_T node_addr; /* file structure's node address */
#endif

#if defined(HASNCACHE) && defined(HASNCVPID)
    unsigned long cap_id; /* capability ID */
#endif

#if defined(HASLFILEADD)
    HASLFILEADD
#endif

#if defined(HASFSTRUCT)
    KA_T fsa; /* file structure address */
    long fct; /* file structure's f_count */
    long ffg; /* file structure's f_flag */
    long pof; /* process open-file flags */
    KA_T fna; /* file structure node address */
#endif

    struct lfile *next;
};
extern struct lfile *CurrentLocalFile, *PrevLocalFile;

struct lproc {
    char *cmd; /* command name */

#if defined(HASSELINUX)
    char *cntx; /* security context */
#endif

    short sel_flags; /* select flags -- SEL* symbols */
    short sel_state; /* state: 0 = not selected
				 	 *	  1 = wholly selected
				 	 *	  2 = partially selected */
    int pid;         /* process ID */

#if defined(HASTASKS)
    int tid; /* task ID */
#endif

    int pgid;  /* process group ID */
    int ppid;  /* parent process ID */
    uid_t uid; /* user ID */

#if defined(HASZONES)
    char *zn; /* zone name */
#endif

    struct lfile *file; /* open files of process */
};
extern struct lproc *CurrentLocalProc, *LocalProcTable;
extern int OptJsonOutput;

extern int LeakDetectMode;
extern int LeakDetectInterval;
extern int LeakDetectThreshold;

extern char *Memory;
extern int MountSupplementState;
extern char *MountSupplementPath;

#if defined(HASPROCFS)
extern struct mounts *Mtprocfs;
#endif

extern int MaxPgidEntries;
extern int MaxPidEntries;
extern int MaxUidEntries;
extern gid_t MyRealGid;
extern int MyProcessId;
extern uid_t MyRealUid;
extern char *NameChars;
extern size_t NameCharsLength;
extern int NumDevices;

#if defined(HASNLIST)
#if !defined(NLIST_TYPE)
#define NLIST_TYPE nlist
#endif /* !defined(NLIST_TYPE) */
extern struct NLIST_TYPE *NlistTable;
extern int NlistLength;
#endif
extern long LinkCountThreshold;
extern int NumLocalProcs;
extern char *NamelistFilePath;
extern int NumPgidSelections;
extern int NumPgidInclusions;
extern int NumPgidExclusions;
extern int NumPidSelections;
extern int NumPidInclusions;
extern int NumPidExclusions;
extern int NumUnselectedPids;
extern int NodeType;
extern int NumUidSelections;
extern int NumUidExclusions;
extern int NumUidInclusions;

struct nwad {
    char *arg;                       /* argument */
    char *proto;                     /* protocol */
    int addr_family;                 /* address family -- e.g.,
					 * AF_INET, AF_INET6 */
    unsigned char addr[MAX_AF_ADDR]; /* address */
    int sport;                       /* starting port */
    int eport;                       /* ending port */
    int found;                       /* find state */
    struct nwad *next;               /* forward link */
};
extern struct nwad *NetworkAddrList;

extern int OffsetDecDigitLimit;
extern char *ProgramName;

#if defined(HASFSTRUCT)
extern struct pff_tab Pff_tab[]; /* file flags table */
extern struct pff_tab Pof_tab[]; /* process open file flags table */
#endif

#if defined(HASPROCFS)
struct procfsid {
    pid_t pid;       /* search PID */
    char *nm;        /* search name */
    unsigned char f; /* match found if == 1 */

#if defined(HASPINODEN)
    INODETYPE inode; /* search inode number */
#endif               /* defined(HASPINODEN) */

    struct procfsid *next; /* forward link */
};

extern int ProcFsFound;
extern struct procfsid *ProcFsIdTable;
extern int ProcFsSearching;
#endif

extern int PrintPass;
extern int RepeatTime;
extern struct l_dev **SortedDevices;
extern int SelectAll;
extern int SelectionFlags;
extern int SetgidState;
extern int SelectInetOnly;
extern int SetuidRootState;
extern struct sfile *SearchFileChain;
extern struct int_lst *SearchPgidList;
extern struct int_lst *SearchPidList;
extern struct seluid *SearchUidList;
extern char *SizeOffFormat0t;
extern char *SizeOffFormatD;
extern char *SizeOffFormatDv;
extern char *SizeOffFormatX;
extern int TaskPrintFlag;
extern int TcpStateAlloc;
extern unsigned char *TcpStateInclude;
extern int TcpStateIncludeCount;
extern int TcpStateOffset;
extern unsigned char *TcpStateExclude;
extern int TcpStateExcludeCount;
extern int TcpNumStates;
extern char **TcpStateNames;
extern char Terminator;
extern int TimeoutLimit;
extern int UdpStateAlloc;
extern unsigned char *UdpStateInclude;
extern int UdpStateIncludeCount;
extern int UdpStateOffset;
extern unsigned char *UdpStateExclude;
extern int UdpStateExcludeCount;
extern int UdpNumStates;
extern char **UdpStateNames;

#if defined(HASZONES)
typedef struct znhash {
    char *zn;            /* zone name */
    int f;               /* "find" flag (used only in ZoneArg) */
    struct znhash *next; /* next zone hash entry */
} znhash_t;
extern znhash_t **ZoneArg;
#endif

#include "proto.h"
#include "dproto.h"

#endif
