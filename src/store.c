/*
 * store.c - common global storage for lsof
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
 * Global storage definitions
 */

#if defined(HASBLKDEV)
struct l_dev *BlockDeviceTable = (struct l_dev *)NULL;
/* block device table pointer */
int BlockNumDevices = 0; /* number of entries in BlockDeviceTable[] */
struct l_dev **BlockSortedDevices = (struct l_dev **)NULL;
/* pointer to BlockDeviceTable[] pointers, sorted
                 * by device */
#endif

int CheckPasswdChange = 0; /* time to check /etc/passwd for change */

#if defined(HAS_STD_CLONE)
struct clone *Clone = (struct clone *)NULL;
/* clone device list */
#endif

int CommandColWidth; /* COMMAND column width */
struct str_lst *CommandNameList = NULL;
/* command names selected with -c */
int CommandColLimit = CMDL;    /* COMMAND column width limit */
int CommandNameInclusions = 0; /* command name inclusions selected with -c */
int CommandNameExclusions = 0; /* command name exclusions selected with -c */
lsof_rx_t *CommandRegexTable = (lsof_rx_t *)NULL;
/* command regular expression table */

#if defined(HASSELINUX)
cntxlist_t *ContextArgList = (cntxlist_t *)NULL;
/* security context arguments supplied with
                 * -Z */
int ContextColWidth;   /* security context column width */
int ContextStatus = 0; /* security context status: 0 == disabled,
				 * 1 == enabled */
#endif

#if defined(HASDCACHE)
unsigned DevCacheChecksum;           /* device cache file checksum */
int DevCacheFd = -1;                 /* device cache file descriptor */
FILE *DevCacheStream = (FILE *)NULL; /* stream pointer for DevCacheFd */
char *DevCachePathArg = NULL;        /* device cache path from -D[b|r|u]<path> */
char *DevCachePath[] = {             /* device cache paths, indexed by DevCachePathIndex
				 *when it's >= 0 */
                        NULL, NULL, NULL, NULL};
int DevCachePathIndex = -1; /* device cache path index:
				 *	-1 = path not defined
				 *	 0 = defined via -D
				 *	 1 = defined via HASENVDC
				 *	 2 = defined via HASSYSDC
				 *	 3 = defined via HASPERSDC and
				 *	     HASPERSDCPATH */
int DevCacheRebuilt = 0;    /* an unsafe device cache file has been
				 * rebuilt */
int DevCacheState = 3;      /* device cache state:
				 *	0 = ignore (-Di)
				 *	1 = build (-Db[path])
				 *	2 = read; don't rebuild (-Dr[path])
				 *	3 = update; read and rebuild if
				 *	    necessary (-Du[path])
				 */
int DevCacheUnsafe = 0;     /* device cache file is potentially unsafe,
				 * (The [cm]time check failed.) */
#endif

int DevCacheHelp = 0; /* -D? status */

int DeviceColWidth; /* DEVICE column width */
dev_t DeviceOfDev;  /* device number of /dev or its equivalent */
struct l_dev *DeviceTable = NULL;
/* device table pointer */

/*
 * Externals for a stkdir(), dumbed-down for older AIX compilers.
 */

char **DirStack = (char **)NULL; /* the directory stack */
int DirStackIndex = 0;           /* DirStack[] index */
int DirStackAlloc = 0;           /* DirStack[] entries allocated */
efsys_list_t *ExcludedFileSysList = (efsys_list_t *)NULL;
/* file systems for which kernel blocks are
 * to be eliminated */
int PathStatErrorCount = 0; /* path stat() error count */
uid_t EffectiveUid;         /* effective UID of this lsof process */
int OptAndSelection = 0;    /* -a option status */
int OptBlockDevice = 0;     /* -b option status */
int FileCountColWidth;      /* FCT column width */
int OptSecContext = 0;      /* -Z option status */
int FileDescColWidth;       /* FD column width */
int OptFileSystem = 0;      /* -f option status:
				 *    0 = paths may be file systems
				 *    1 = paths are just files
				 *    2 = paths must be file systems */

#if defined(HASNCACHE)
int OptNameCache = 1;    /* -C option status */
int NameCacheReload = 1; /* 1 == call ncache_load() */
#endif

int OptFieldOutput = 0; /* -f and -F status */
int FileFlagColWidth;   /* FILE-FLAG column width */
int OptHelp = 0;        /* -h option status */
int OptHostLookup = 1;  /* -H option status */
int OptNetwork = 0;     /* -i option status: 0==none
				 *		     1==find all
				 *		     2==some found*/
int OptNetworkType = 0; /* OptNetwork type request: 0==all
				 *		      4==IPv4
				 *		      6==IPv6 */
int OptNfs = 0;         /* -N option status: 0==none, 1==find all,
				 * 2==some found*/
int OptLinkCount = 0;   /* -L option status */
int OptOffset = 0;      /* -o option status */
int OptOverhead = 0;    /* -O option status */
int OptPortLookup = 1;  /* -P option status */

#ifndef HASNORPC_H
#if defined(HASPMAPENABLED)
int OptPortMap = 1; /* +|-M option status */
#else
int OptPortMap = 0; /* +|-M option status */
#endif
#endif

int OptProcessGroup = 0;               /* -g option status */
int OptParentPid = 0;                  /* -R option status */
int OptSize = 0;                       /* -s option status */
int FileStructAddrColWidth;            /* FSTR-ADDR column width */
int OptFileStructValues = FSV_DEFAULT; /* file struct value selections */
int OptFileStructSetByFlag = 0;        /* OptFileStructValues was set by +f */
int OptFileStructFlagHex = 0;          /* hex format status for FSV_FILE_FLAGS */
int OptTask = 0;                       /* -K option value */
int NodeIdColWidth;                    /* NODE-ID column width */
char *NodeIdTitle = NULL;              /* NODE-ID column title, set at runtime */
int OptTcpTpiInfo = TCPTPI_STATE;      /* -T option status */
int OptTerse = 0;                      /* -t option status */
int OptUnixSocket = 0;                 /* -U option status */
int OptUserToLogin = 1;                /* -l option status */
int OptVerbose = 0;                    /* -V option status */

#if defined(WARNINGSTATE)
int OptWarnings = 1; /* +|-w option status */
#else
int OptWarnings = 0; /* +|-w option status */
#endif

#if defined(HASXOPT_VALUE)
int OptCrossoverExt = HASXOPT_VALUE; /* -X option status */
#endif

int OptCrossover = 0; /* -x option value */
int OptZone = 0;      /* -z option status */

struct fd_lst *FdList = NULL;
/* file descriptors selected with -d */
int FdListType = -1; /* FdList[] type: -1 == none
				 *		0 == include
				 *		1 == exclude */

struct fieldsel FieldSelection[] = {
    {LSOF_FID_ACCESS, 0, LSOF_FNM_ACCESS, NULL, 0}, /*  0 */
    {LSOF_FID_CMD, 0, LSOF_FNM_CMD, NULL, 0},       /*  1 */
    {LSOF_FID_FILE_STRUCT_COUNT, 0, LSOF_FNM_FILE_STRUCT_COUNT, &OptFileStructValues,
     FSV_FILE_COUNT},                                     /*  2 */
    {LSOF_FID_DEV_CHAR, 0, LSOF_FNM_DEV_CHAR, NULL, 0},   /*  3 */
    {LSOF_FID_DEV_NUM, 0, LSOF_FNM_DEV_NUM, NULL, 0},     /*  4 */
    {LSOF_FID_FILE_DESC, 0, LSOF_FNM_FILE_DESC, NULL, 0}, /*  5 */
    {LSOF_FID_FILE_STRUCT_ADDR, 0, LSOF_FNM_FILE_STRUCT_ADDR, &OptFileStructValues,
     FSV_FILE_ADDR},                                                                     /*  6 */
    {LSOF_FID_FILE_FLAGS, 0, LSOF_FNM_FILE_FLAGS, &OptFileStructValues, FSV_FILE_FLAGS}, /*  7 */
    {LSOF_FID_INODE, 0, LSOF_FNM_INODE, NULL, 0},                                        /*  8 */
    {LSOF_FID_NLINK, 0, LSOF_FNM_NLINK, &OptLinkCount, 1},                               /*  9 */
    {LSOF_FID_TID, 0, LSOF_FNM_TID, NULL, 0},                                            /* 11 */
    {LSOF_FID_LOCK, 0, LSOF_FNM_LOCK, NULL, 0},                                          /* 11 */
    {LSOF_FID_LOGIN, 0, LSOF_FNM_LOGIN, NULL, 0},                                        /* 12 */
    {LSOF_FID_MARK, 1, LSOF_FNM_MARK, NULL, 0},                                          /* 13 */
    {LSOF_FID_NAME, 0, LSOF_FNM_NAME, NULL, 0},                                          /* 14 */
    {LSOF_FID_NODE_ID, 0, LSOF_FNM_NODE_ID, &OptFileStructValues, FSV_NODE_ID},          /* 15 */
    {LSOF_FID_OFFSET, 0, LSOF_FNM_OFFSET, NULL, 0},                                      /* 16 */
    {LSOF_FID_PID, 1, LSOF_FNM_PID, NULL, 0},                                            /* 17 */
    {LSOF_FID_PGID, 0, LSOF_FNM_PGID, &OptProcessGroup, 1},                              /* 18 */
    {LSOF_FID_PROTO, 0, LSOF_FNM_PROTO, NULL, 0},                                        /* 19 */
    {LSOF_FID_RDEV, 0, LSOF_FNM_RDEV, NULL, 0},                                          /* 20 */
    {LSOF_FID_PPID, 0, LSOF_FNM_PPID, &OptParentPid, 1},                                 /* 21 */
    {LSOF_FID_SIZE, 0, LSOF_FNM_SIZE, NULL, 0},                                          /* 22 */
    {LSOF_FID_STREAM, 0, LSOF_FNM_STREAM, NULL, 0},                                      /* 23 */
    {LSOF_FID_TYPE, 0, LSOF_FNM_TYPE, NULL, 0},                                          /* 24 */
    {LSOF_FID_TCP_TPI_INFO, 0, LSOF_FNM_TCP_TPI_INFO, &OptTcpTpiInfo, TCPTPI_ALL},       /* 25 */
    {LSOF_FID_UID, 0, LSOF_FNM_UID, NULL, 0},                                            /* 26 */
    {LSOF_FID_ZONE, 0, LSOF_FNM_ZONE, &OptZone, 1},                                      /* 27 */
    {LSOF_FID_SEC_CONTEXT, 0, LSOF_FNM_SEC_CONTEXT, &OptSecContext, 1},                  /* 28 */
    {LSOF_FID_TERM, 0, LSOF_FNM_TERM, NULL, 0},                                          /* 29 */
    {' ', 0, NULL, NULL, 0}};

int CyberpunkTTY = 0;  /* 1 if stdout is a TTY (cyberpunk mode) */
int HeaderPrinted = 0; /* header print status */
char *InodeFormatDecimal = NULL;
/* INODETYPE decimal printf specification */
char *InodeFormatHex = NULL;
/* INODETYPE hexadecimal printf specification */
struct lfile *CurrentLocalFile = NULL;
/* current local file structure */
struct lproc *CurrentLocalProc = NULL;
/* current local process table entry */
struct lproc *LocalProcTable = NULL;
/* local process table */
char *Memory = NULL;              /* core file path */
int MountSupplementState = 0;     /* mount supplement state: 0 == none
				 *			   1 == create
				 *			   2 == read */
char *MountSupplementPath = NULL; /* mount supplement path -- if MountSupplementState == 2 */

#if defined(HASPROCFS)
struct mounts *Mtprocfs = (struct mounts *)NULL;
/* /proc mount entry */
#endif

int MaxPgidEntries = 0;             /* maximum process group ID table entries */
int MaxPidEntries = 0;              /* maximum PID table entries */
int MaxUidEntries = 0;              /* maximum UID table entries */
gid_t MyRealGid;                    /* real GID of this lsof process */
int MyProcessId;                    /* lsof's process ID */
uid_t MyRealUid;                    /* real UID of this lsof process */
char *NameChars = NULL;             /* name characters for printing */
size_t NameCharsLength = (size_t)0; /* sizeof(NameChars) */
int NumCommandRegexUsed = 0;        /* number of CommandRegexTable[] entries */
int NumDevices = 0;                 /* number of entries in DeviceTable[] */

#if defined(HASNLIST)
struct NLIST_TYPE *NlistTable = (struct NLIST_TYPE *)NULL;
/* kernel name list */
int NlistLength = 0; /* NlistTable calloc'd length */
#endif

long LinkCountThreshold = 0l;  /* report nlink values below this number
				 * (0 = report all nlink values) */
int NumLocalProcs = 0;         /* number of entries in LocalProcTable[] */
int LinkCountColWidth;         /* NLINK column width */
int NameColWidth;              /* NAME column width */
char *NamelistFilePath = NULL; /* namelist file path */
int NodeColWidth;              /* NODE column width */
int NumPgidSelections = 0;     /* -g option count */
int NumPgidInclusions = 0;     /* -g option inclusion count */
int NumPgidExclusions = 0;     /* -g option exclusion count */
int NumPidSelections = 0;      /* -p option count */
int NumPidInclusions = 0;      /* -p option inclusion count */
int NumPidExclusions = 0;      /* -p option exclusion count */
int NumUnselectedPids;         /* number of unselected PIDs (starts at NumPidSelections) */
int NodeType;                  /* node type (see N_* symbols) */
int NumUidSelections = 0;      /* -u option count */
int NumUidExclusions = 0;      /* -u option count of UIDs excluded */
int NumUidInclusions = 0;      /* -u option count of UIDs included */
struct nwad *NetworkAddrList = NULL;
/* list of network addresses */
int OffsetDecDigitLimit = OFFDECDIG; /* offset decimal form (0t...) digit limit */
int OffsetColWidth;                  /* OFFSET column width */
int PgidColWidth;                    /* PGID column width */
int PidColWidth;                     /* PID column width */
struct lfile *PrevLocalFile = NULL;
/* previous local file structure */
char *ProgramName; /* program name */
int PpidColWidth;  /* PPID column width */

#if defined(HASPROCFS)
int ProcFsFound = 0; /* 1 when searching for an proc file system
				 * file and one was found */
struct procfsid *ProcFsIdTable = (struct procfsid *)NULL;
/* proc file system PID search table */
int ProcFsSearching = 0; /* 1 if searching for any proc file system
				 * file */
#endif

int PrintPass = 0;  /* print pass: 0 = compute column widths
				 *	       1 = print */
int RepeatTime = 0; /* repeat time -- set by -r */
struct l_dev **SortedDevices = NULL;
/* pointer to DeviceTable[] pointers, sorted
 * by device */
int SelectAll = 1;       /* all processes are selected (default) */
int SelectionFlags = 0;  /* selection flags -- see SEL* in lsof.h */
int SetgidState = 0;     /* setgid state */
int SelectInetOnly = 0;  /* select only Internet socket files */
int SetuidRootState = 0; /* setuid-root state */
struct sfile *SearchFileChain = NULL;
/* chain of files to search for */
struct int_lst *SearchPgidList = NULL;
/* process group IDs to search for */
struct int_lst *SearchPidList = NULL;
/* Process IDs to search for */
struct seluid *SearchUidList = NULL;
/* User IDs to include or exclude */
int SizeColWidth;    /* SIZE column width */
int SizeOffColWidth; /* SIZE/OFF column width */
char *SizeOffFormat0t = NULL;
/* SZOFFTYPE 0t%u printf specification */
char *SizeOffFormatD = NULL;
/* SZOFFTYPE %d printf  specification */
char *SizeOffFormatDv = NULL;
/* SZOFFTYPE %*d printf  specification */
char *SizeOffFormatX = NULL;
/* SZOFFTYPE %#x printf  specification */
int TaskPrintFlag = 0; /* task print flag */
int TcpStateAlloc = 0; /* allocated (possibly unused) entries in TCP
				 * state tables */
unsigned char *TcpStateInclude = NULL;
/* included TCP states */
int TcpStateIncludeCount = 0; /* number of entries in TcpStateInclude[] */
int TcpStateOffset = 0;       /* offset for TCP state number to adjust
				 * negative numbers to an index into TcpStateNames[],
				 * TcpStateInclude[] and TcpStateExclude[] */
unsigned char *TcpStateExclude = NULL;
/* excluded TCP states */
int TcpStateExcludeCount = 0;         /* number of entries in TcpStateExclude[] */
int TcpNumStates = 0;                 /* number of TCP states -- either in
				 * tcpstates[] or TcpStateNames[] */
char **TcpStateNames = (char **)NULL; /* local TCP state names, indexed by system
				 * state value */
char Terminator = '\n';               /* output field terminator */
int TidColWidth = 0;                  /* TID column width */
int TimeoutLimit = TMLIMIT;           /* Readlink() and stat() timeout (seconds) */
int TypeColWidth;                     /* TYPE column width */
int UdpStateAlloc = 0;                /* allocated (possibly unused) entries in UDP
				 * state tables */
unsigned char *UdpStateInclude = NULL;
/* included UDP states */
int UdpStateIncludeCount = 0; /* number of entries in UdpStateInclude[] */
int UdpStateOffset = 0;       /* offset for UDP state number to adjust
				 * negative numbers to an index into UdpStateNames[],
				 * UdpStateInclude[] and UdpStateExclude[] */
unsigned char *UdpStateExclude = NULL;
/* excluded UDP states */
int UdpStateExcludeCount = 0;         /* number of entries in UdpStateExclude[] */
int UdpNumStates = 0;                 /* number of UDP states  in UdpStateNames[] */
char **UdpStateNames = (char **)NULL; /* local UDP state names, indexed by system
				 * state number */
int UserColWidth;                     /* USER column width */

#if defined(HASZONES)
znhash_t **ZoneArg = (znhash_t **)NULL;
/* zone arguments supplied with -z */
#endif

int ZoneColWidth; /* ZONE column width */

int OptJsonOutput = 0;

int LeakDetectMode = 0;
int LeakDetectInterval = 5;
int LeakDetectThreshold = 3;

int OptDeltaHighlight = 0;
int OptMonitorMode = 0;       /* --monitor / -W live refresh mode */
int MonitorTermRows = 24;     /* terminal rows (updated by SIGWINCH) */
int MonitorTermCols = 80;     /* terminal cols (updated by SIGWINCH) */
int MonitorRowsRemaining = 0; /* rows left to print in current frame */
