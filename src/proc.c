/*
 * proc.c - common process and file structure functions for lsof
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
 * Local function prototypes
 */

static int is_file_sel(struct lproc *lp, struct lfile *lf);
static struct int_lst *find_int_lst(struct int_lst *list, int num_entries, int val);

/*
 * add_nma() - add to NAME column addition
 */

void add_nma(char *str, int len) {
    int name_len;

    if (!str || !len)
        return;
    if (CurrentLocalFile->name_append) {
        char *tmp;
        name_len = (int)strlen(CurrentLocalFile->name_append);
        tmp = (char *)realloc((MALLOC_P *)CurrentLocalFile->name_append,
                              (MALLOC_S)(len + name_len + 2));
        if (tmp)
            CurrentLocalFile->name_append = tmp;
        else {
            free((FREE_P *)CurrentLocalFile->name_append);
            CurrentLocalFile->name_append = NULL;
        }
    } else {
        name_len = 0;
        CurrentLocalFile->name_append = (char *)malloc((MALLOC_S)(len + 1));
    }
    if (!CurrentLocalFile->name_append) {
        fprintf(stderr, "%s: no name addition space: PID %ld, FD %s", ProgramName,
                (long)CurrentLocalProc->pid, CurrentLocalFile->fd);
        Exit(1);
    }
    if (name_len) {
        CurrentLocalFile->name_append[name_len] = ' ';
        strncpy(&CurrentLocalFile->name_append[name_len + 1], str, len);
        CurrentLocalFile->name_append[name_len + 1 + len] = '\0';
    } else {
        strncpy(CurrentLocalFile->name_append, str, len);
        CurrentLocalFile->name_append[len] = '\0';
    }
}

#if defined(HASFSTRUCT)
static char *alloc_fflbuf(char **buf_ptr, int *alloc_len, int len_required);

/*
 * alloc_fflbuf() - allocate file flags print buffer
 */

static char *alloc_fflbuf(char **buf_ptr, int *alloc_len, int len_required) {
    int alloc_size;

    alloc_size = (int)(len_required + 1); /* allocate '\0' space */
    if (*buf_ptr && (alloc_size <= *alloc_len))
        return (*buf_ptr);
    if (*buf_ptr)
        *buf_ptr = (char *)realloc((MALLOC_P *)*buf_ptr, (MALLOC_S)alloc_size);
    else
        *buf_ptr = (char *)malloc((MALLOC_S)alloc_size);
    if (!*buf_ptr) {
        fprintf(stderr, "%s: no space (%d) for print flags\n", ProgramName, alloc_size);
        Exit(1);
    }
    *alloc_len = alloc_size;
    return (*buf_ptr);
}
#endif

/*
 * alloc_lfile() - allocate local file structure space
 */

void alloc_lfile(char *name, int num) {
    int fds;

    if (CurrentLocalFile) {
        /*
 * If reusing a previously allocated structure, release any allocated
 * space it was using.
 */
        if (CurrentLocalFile->dev_ch)
            free((FREE_P *)CurrentLocalFile->dev_ch);
        if (CurrentLocalFile->name)
            free((FREE_P *)CurrentLocalFile->name);
        if (CurrentLocalFile->name_append)
            free((FREE_P *)CurrentLocalFile->name_append);

#if defined(HASLFILEADD) && defined(CLRLFILEADD)
        CLRLFILEADD(CurrentLocalFile)
#endif

        /*
 * Othwerise, allocate a new structure.
 */
    } else if (!(CurrentLocalFile = (struct lfile *)malloc(sizeof(struct lfile)))) {
        fprintf(stderr, "%s: no local file space at PID %d\n", ProgramName, CurrentLocalProc->pid);
        Exit(1);
    }
    /*
 * Initialize the structure.
 */
    CurrentLocalFile->access = CurrentLocalFile->lock = ' ';
    CurrentLocalFile->dev_def = CurrentLocalFile->inp_ty = CurrentLocalFile->is_com =
        CurrentLocalFile->is_nfs = CurrentLocalFile->is_stream = CurrentLocalFile->lmi_srch =
            CurrentLocalFile->nlink_def = CurrentLocalFile->off_def = CurrentLocalFile->sz_def =
                CurrentLocalFile->rdev_def = (unsigned char)0;
    CurrentLocalFile->li[0].addr_family = CurrentLocalFile->li[1].addr_family = 0;
    CurrentLocalFile->lts.type = -1;
    CurrentLocalFile->nlink = 0l;

#if defined(HASMNTSTAT)
    CurrentLocalFile->mnt_stat = (unsigned char)0;
#endif

#if defined(HASSOOPT)
    CurrentLocalFile->lts.kai = CurrentLocalFile->lts.ltm = 0;
    CurrentLocalFile->lts.opt = CurrentLocalFile->lts.qlen = CurrentLocalFile->lts.qlim =
        CurrentLocalFile->lts.pqlen = (unsigned int)0;
    CurrentLocalFile->lts.rbsz = CurrentLocalFile->lts.sbsz = (unsigned long)0;
    CurrentLocalFile->lts.qlens = CurrentLocalFile->lts.qlims = CurrentLocalFile->lts.pqlens =
        CurrentLocalFile->lts.rbszs = CurrentLocalFile->lts.sbszs = (unsigned char)0;
#endif

#if defined(HASSOSTATE)
    CurrentLocalFile->lts.sock_state = 0;
#endif

#if defined(HASTCPOPT)
    CurrentLocalFile->lts.mss = (unsigned long)0;
    CurrentLocalFile->lts.msss = (unsigned char)0;
    CurrentLocalFile->lts.topt = (unsigned int)0;
#endif

#if defined(HASTCPTPIQ)
    CurrentLocalFile->lts.recv_queue_st = CurrentLocalFile->lts.send_queue_st = (unsigned char)0;
#endif

#if defined(HASTCPTPIW)
    CurrentLocalFile->lts.read_win_st = CurrentLocalFile->lts.write_win_st = (unsigned char)0;
#endif

#if defined(HASFSINO)
    CurrentLocalFile->fs_ino = 0;
#endif

#if defined(HASVXFS) && defined(HASVXFSDNLC)
    CurrentLocalFile->is_vxfs = 0;
#endif

    CurrentLocalFile->inode = (INODETYPE)0;
    CurrentLocalFile->off = (SZOFFTYPE)0;
    if (CurrentLocalProc->sel_state & PS_PRI)
        CurrentLocalFile->sel_flags = CurrentLocalProc->sel_flags;
    else
        CurrentLocalFile->sel_flags = 0;
    CurrentLocalFile->iproto[0] = CurrentLocalFile->type[0] = '\0';
    if (name) {
        strncpy(CurrentLocalFile->fd, name, FDLEN - 1);
        CurrentLocalFile->fd[FDLEN - 1] = '\0';
    } else if (num >= 0) {
        if (num < 10000)
            snpf(CurrentLocalFile->fd, sizeof(CurrentLocalFile->fd), "%4d", num);
        else
            snpf(CurrentLocalFile->fd, sizeof(CurrentLocalFile->fd), "*%03d", num % 1000);
    } else
        CurrentLocalFile->fd[0] = '\0';
    CurrentLocalFile->dev_ch = CurrentLocalFile->fsdir = CurrentLocalFile->fsdev =
        CurrentLocalFile->name = CurrentLocalFile->name_append = NULL;
    CurrentLocalFile->channel = -1;

#if defined(HASNCACHE) && HASNCACHE < 2
    CurrentLocalFile->node_addr = (KA_T)NULL;
#endif

    CurrentLocalFile->next = NULL;
    CurrentLocalFile->ntype = NodeType = N_REGLR;
    NameChars[0] = '\0';

#if defined(HASFSTRUCT)
    CurrentLocalFile->fct = CurrentLocalFile->ffg = CurrentLocalFile->pof = (long)0;
    CurrentLocalFile->fna = (KA_T)NULL;
    CurrentLocalFile->fsv = (unsigned char)0;
#endif

#if defined(HASLFILEADD) && defined(SETLFILEADD)
    /*
     * Do local initializations.
     */
    SETLFILEADD
#endif

    /*
 * See if the file descriptor has been selected.
 */
    if (!FdList || (!name && num < 0))
        return;
    fds = ck_fd_status(name, num);
    switch (FdListType) {
    case 0: /* inclusion list */
        if (fds == 2)
            CurrentLocalFile->sel_flags |= SELFD;
        break;
    case 1: /* exclusion list */
        if (fds != 1)
            CurrentLocalFile->sel_flags |= SELFD;
    }
}

/*
 * alloc_lproc() - allocate local proc structure space
 */

void alloc_lproc(int pid, int pgid, int ppid, UID_ARG uid, char *cmd, int pss, int sel_flags) {
    static int size = 0;

    if (!LocalProcTable) {
        if (!(LocalProcTable =
                  (struct lproc *)malloc((MALLOC_S)(LPROCINCR * sizeof(struct lproc))))) {
            fprintf(stderr, "%s: no malloc space for %d local proc structures\n", ProgramName,
                    LPROCINCR);
            Exit(1);
        }
        size = LPROCINCR;
    } else if ((NumLocalProcs + 1) > size) {
        struct lproc *tmp;
        size += LPROCINCR;
        tmp = (struct lproc *)realloc((MALLOC_P *)LocalProcTable,
                                      (MALLOC_S)(size * sizeof(struct lproc)));
        if (!tmp) {
            fprintf(stderr, "%s: no realloc space for %d local proc structures\n", ProgramName,
                    size);
            free((FREE_P *)LocalProcTable);
            LocalProcTable = NULL;
            Exit(1);
        }
        LocalProcTable = tmp;
    }
    CurrentLocalProc = &LocalProcTable[NumLocalProcs++];
    CurrentLocalProc->pid = pid;

#if defined(HASTASKS)
    CurrentLocalProc->tid = 0;
#endif

    CurrentLocalProc->pgid = pgid;
    CurrentLocalProc->ppid = ppid;
    CurrentLocalProc->file = NULL;
    CurrentLocalProc->sel_flags = (short)sel_flags;
    CurrentLocalProc->sel_state = (short)pss;
    CurrentLocalProc->uid = (uid_t)uid;
    /*
 * Allocate space for the full command name and copy it there.
 */
    if (!(CurrentLocalProc->cmd = mkstrcpy(cmd, NULL))) {
        fprintf(stderr, "%s: PID %d, no space for command name: ", ProgramName, pid);
        safestrprt(cmd, stderr, 1);
        Exit(1);
    }

#if defined(HASZONES)
    /*
     * Clear the zone name pointer.  The dialect's own code will set it.
     */
    CurrentLocalProc->zn = NULL;
#endif

#if defined(HASSELINUX)
    /*
     * Clear the security context pointer.  The dialect's own code will
     * set it.
     */
    CurrentLocalProc->cntx = NULL;
#endif
}

/*
 * ck_fd_status() - check FD status
 *
 * return: 0 == FD is neither included nor excluded
 *	   1 == FD is excluded
 *	   2 == FD is included
 */

extern int ck_fd_status(char *name, int num) {
    char *cp;
    struct fd_lst *fp;

    if (!(fp = FdList) || (!name && num < 0))
        return (0);
    if ((cp = name)) {
        while (*cp && *cp == ' ')
            cp++;
    }
    /*
 * Check for an exclusion match.
 */
    if (FdListType == 1) {
        for (; fp; fp = fp->next) {
            if (cp) {
                if (fp->nm && strcmp(fp->nm, cp) == 0)
                    return (1);
                continue;
            }
            if (num >= fp->lo && num <= fp->hi)
                return (1);
        }
        return (0);
    }
    /*
 * If FdList isn't an exclusion list, check for an inclusion match.
 */
    for (; fp; fp = fp->next) {
        if (cp) {
            if (fp->nm && strcmp(fp->nm, cp) == 0)
                return (2);
            continue;
        }
        if (num >= fp->lo && num <= fp->hi)
            return (2);
    }
    return (0);
}

/*
 * comppid() - compare PIDs
 */

int comppid(COMP_P *lhs, COMP_P *rhs) {
    struct lproc **p1 = (struct lproc **)lhs;
    struct lproc **p2 = (struct lproc **)rhs;

    if ((*p1)->pid < (*p2)->pid)
        return (-1);
    if ((*p1)->pid > (*p2)->pid)
        return (1);

#if defined(HASTASKS)
    if ((*p1)->tid < (*p2)->tid)
        return (-1);
    if ((*p1)->tid > (*p2)->tid)
        return (1);
#endif

    return (0);
}

/*
 * ent_inaddr() - enter Internet addresses
 */

void ent_inaddr(unsigned char *local_addr, int local_port, unsigned char *foreign_addr,
                int foreign_port, int addr_family) {
    int match;

    if (local_addr) {
        CurrentLocalFile->li[0].addr_family = addr_family;

#if defined(HASIPv6)
        if (addr_family == AF_INET6)
            CurrentLocalFile->li[0].ia.a6 = *(struct in6_addr *)local_addr;
        else
#endif

            CurrentLocalFile->li[0].ia.a4 = *(struct in_addr *)local_addr;
        CurrentLocalFile->li[0].port = local_port;
    } else
        CurrentLocalFile->li[0].addr_family = 0;
    if (foreign_addr) {
        CurrentLocalFile->li[1].addr_family = addr_family;

#if defined(HASIPv6)
        if (addr_family == AF_INET6)
            CurrentLocalFile->li[1].ia.a6 = *(struct in6_addr *)foreign_addr;
        else
#endif

            CurrentLocalFile->li[1].ia.a4 = *(struct in_addr *)foreign_addr;
        CurrentLocalFile->li[1].port = foreign_port;
    } else
        CurrentLocalFile->li[1].addr_family = 0;
    /*
 * If network address matching has been selected, check both addresses.
 */
    if ((SelectionFlags & SELNA) && NetworkAddrList) {
        match = (foreign_addr && is_nw_addr(foreign_addr, foreign_port, addr_family)) ? 1 : 0;
        match |= (local_addr && is_nw_addr(local_addr, local_port, addr_family)) ? 1 : 0;
        if (match)
            CurrentLocalFile->sel_flags |= SELNA;
    }
}

/*
 * examine_lproc() - examine local process
 *
 * return: 1 = last process
 */

int examine_lproc() {
    int sbp = 0;

    if (RepeatTime)
        return (0);
    /*
 * List the process if the process is selected and:
 *
 *	o  listing is limited to a single PID selection -- this one;
 *
 *	o  listing is selected by an ANDed option set (not all options)
 *	   that includes a single PID selection -- this one.
 */
    if ((CurrentLocalProc->sel_flags & SELPID) && !SelectAll) {
        if ((SelectionFlags == SELPID) || (OptAndSelection && (SelectionFlags & SELPID))) {
            sbp = 1;
            NumUnselectedPids--;
        }
    }
    if (CurrentLocalProc->sel_state && NumPidSelections == 1 && sbp) {
        print_init();
        print_proc();
        PrintPass++;
        if (PrintPass < 2)
            print_proc();
        CurrentLocalProc->sel_state = 0;
    }
    /*
 * Deprecate an unselected (or listed) process.
 */
    if (!CurrentLocalProc->sel_state) {
        free_lproc(CurrentLocalProc);
        NumLocalProcs--;
    }
    /*
 * Indicate last-process if listing is limited to PID selections,
 * and all selected processes have been listed.
 */
    return ((sbp && NumUnselectedPids == 0) ? 1 : 0);
}

/*
 * free_lproc() - free lproc entry and its associated malloc'd space
 */

void free_lproc(struct lproc *proc) {
    struct lfile *lf, *nf;

    for (lf = proc->file; lf; lf = nf) {
        if (lf->dev_ch) {
            free((FREE_P *)lf->dev_ch);
            lf->dev_ch = NULL;
        }
        if (lf->name) {
            free((FREE_P *)lf->name);
            lf->name = NULL;
        }
        if (lf->name_append) {
            free((FREE_P *)lf->name_append);
            lf->name_append = NULL;
        }

#if defined(HASLFILEADD) && defined(CLRLFILEADD)
        CLRLFILEADD(lf)
#endif

        nf = lf->next;
        free((FREE_P *)lf);
    }
    proc->file = NULL;
    if (proc->cmd) {
        free((FREE_P *)proc->cmd);
        proc->cmd = NULL;
    }

#if defined(HASSELINUX)
    if (proc->cntx) {
        free((FREE_P *)proc->cntx);
        proc->cntx = NULL;
    }
#endif
}

/*
 * is_cmd_excl() - is command excluded?
 */

int is_cmd_excl(char *cmd, short *pss, short *select_flags) {
    int i;
    struct str_lst *sp;
    /*
 * See if the command is excluded by a "-c^<command>" option.
 */
    if (CommandNameList && CommandNameExclusions) {
        for (sp = CommandNameList; sp; sp = sp->next) {
            if (sp->x && !strncmp(sp->str, cmd, sp->len))
                return (1);
        }
    }
    /*
 * The command is not excluded if no command selection was requested,
 * or if its name matches any -c <command> specification.
 * 
 */
    if ((SelectionFlags & SELCMD) == 0)
        return (0);
    for (sp = CommandNameList; sp; sp = sp->next) {
        if (!sp->x && !strncmp(sp->str, cmd, sp->len)) {
            sp->f = 1;
            *pss |= PS_PRI;
            *select_flags |= SELCMD;
            return (0);
        }
    }
    /*
 * The command name doesn't match any -c <command> specification.  See if it
 * matches a -c /RE/[bix] specification.
 */
    for (i = 0; i < NumCommandRegexUsed; i++) {
        if (!regexec(&CommandRegexTable[i].cx, cmd, 0, NULL, 0)) {
            CommandRegexTable[i].mc = 1;
            *pss |= PS_PRI;
            *select_flags |= SELCMD;
            return (0);
        }
    }
    /*
 * The command name matches no -c specification.
 *
 * It's excluded if the only selection condition is command name,
 * or if command name selection is part of an ANDed set.
 */
    if (SelectionFlags == SELCMD)
        return (1);
    return (OptAndSelection ? 1 : 0);
}

/*
 * is_file_sel() - is file selected?
 */

static int is_file_sel(struct lproc *proc, struct lfile *file) {
    if (!file || !file->sel_flags)
        return (0);
    if (CurrentLocalFile->sel_flags & SELEXCLF)
        return (0);

#if defined(HASSECURITY) && defined(HASNOSOCKSECURITY)
    if (MyRealUid && (MyRealUid != proc->uid)) {
        if (!(file->sel_flags & (SELNA | SELNET)))
            return (0);
    }
#endif

    if (SelectAll)
        return (1);
    if (OptAndSelection && ((file->sel_flags & SelectionFlags) != SelectionFlags))
        return (0);
    return (1);
}

/*
 * find_int_lst() - binary search for value in sorted int_lst array
 */

static struct int_lst *find_int_lst(struct int_lst *list, int num_entries, int val) {
    int low = 0, high = num_entries - 1, midpoint;

    while (low <= high) {
        midpoint = (low + high) >> 1;
        if (list[midpoint].i == val)
            return &list[midpoint];
        if (list[midpoint].i < val)
            low = midpoint + 1;
        else
            high = midpoint - 1;
    }
    return (struct int_lst *)NULL;
}

/*
 * is_proc_excl() - is process excluded?
 */

int

#if defined(HASTASKS)
is_proc_excl(int pid, int pgid, UID_ARG uid, short *pss, short *select_flags,
             int tid)
#else
is_proc_excl(int pid, int pgid, UID_ARG uid, short *pss, short *select_flags)
#endif

{
    int i, j;

    *pss = *select_flags = 0;

#if defined(HASSECURITY)
    /*
     * The process is excluded by virtue of the security option if it
     * isn't owned by the owner of this lsof process, unless the
     * HASNOSOCKSECURITY option is also specified.  In that case the
     * selected socket files of any process may be listed.
     */
#ifndef HASNOSOCKSECURITY
    if (MyRealUid && MyRealUid != (uid_t)uid)
        return (1);
#endif
#endif

    /*
 * If the excluding of process listing by UID has been specified, see if the
 * owner of this process is excluded.
 */
    if (NumUidExclusions) {
        for (i = j = 0; (i < NumUidSelections) && (j < NumUidExclusions); i++) {
            if (!SearchUidList[i].excl)
                continue;
            if (SearchUidList[i].uid == (uid_t)uid)
                return (1);
            j++;
        }
    }
    /*
 * If the excluding of process listing by PGID has been specified, see if this
 * PGID is excluded.
 */
    if (NumPgidExclusions) {
        struct int_lst *match = find_int_lst(SearchPgidList, NumPgidSelections, pgid);
        if (match && match->x)
            return (1);
    }
    /*
 * If the excluding of process listing by PID has been specified, see if this
 * PID is excluded.
 */
    if (NumPidExclusions) {
        struct int_lst *match = find_int_lst(SearchPidList, NumPidSelections, pid);
        if (match && match->x)
            return (1);
    }
    /*
 * If the listing of all processes is selected, then this one is not excluded.
 *
 * However, if HASSECURITY and HASNOSOCKSECURITY are both specified, exclude
 * network selections from the file flags, so that the tests in is_file_sel()
 * work as expected.
 */
    if (SelectAll) {
        *pss = PS_PRI;

#if defined(HASSECURITY) && defined(HASNOSOCKSECURITY)
        *select_flags = SELALL & ~(SELNA | SELNET);
#else
        *select_flags = SELALL;
#endif

        return (0);
    }
    /*
 * If the listing of processes has been specified by process group ID, see
 * if this one is included or excluded.
 */
    if (NumPgidInclusions && (SelectionFlags & SELPGID)) {
        struct int_lst *match = find_int_lst(SearchPgidList, NumPgidSelections, pgid);
        if (match && !match->x) {
            match->f = 1;
            *pss = PS_PRI;
            *select_flags = SELPGID;
            if (SelectionFlags == SELPGID)
                return (0);
        }
        if ((SelectionFlags == SELPGID) && !*select_flags)
            return (1);
    }
    /*
 * If the listing of processes has been specified by PID, see if this one is
 * included or excluded.
 */
    if (NumPidInclusions && (SelectionFlags & SELPID)) {
        struct int_lst *match = find_int_lst(SearchPidList, NumPidSelections, pid);
        if (match && !match->x) {
            match->f = 1;
            *pss = PS_PRI;
            *select_flags |= SELPID;
            if (SelectionFlags == SELPID)
                return (0);
        }
        if ((SelectionFlags == SELPID) && !*select_flags)
            return (1);
    }
    /*
 * If the listing of processes has been specified by UID, see if the owner of
 * this process has been included.
 */
    if (NumUidInclusions && (SelectionFlags & SELUID)) {
        for (i = j = 0; (i < NumUidSelections) && (j < NumUidInclusions); i++) {
            if (SearchUidList[i].excl)
                continue;
            if (SearchUidList[i].uid == (uid_t)uid) {
                SearchUidList[i].f = 1;
                *pss = PS_PRI;
                *select_flags |= SELUID;
                if (SelectionFlags == SELUID)
                    return (0);
                break;
            }
            j++;
        }
        if (SelectionFlags == SELUID && (*select_flags & SELUID) == 0)
            return (1);
    }

#if defined(HASTASKS)
    if ((SelectionFlags & SELTASK) && tid) {

        /*
     * This is a task and tasks are selected.
     */
        *pss = PS_PRI;
        *select_flags |= SELTASK;
        if ((SelectionFlags == SELTASK) ||
            (OptAndSelection && ((*select_flags & SelectionFlags) == SelectionFlags)))
            return (0);
    }
#endif

    /*
 * When neither the process group ID, nor the PID, nor the task, nor the UID
 * is selected:
 *
 *	If list option ANDing of process group IDs, PIDs, UIDs or tasks is
 *	specified, the process is excluded;
 *
 *	Otherwise, it's not excluded by the tests of this function.
 */
    if (!*select_flags)
        return ((OptAndSelection && (SelectionFlags & (SELPGID | SELPID | SELUID | SELTASK))) ? 1
                                                                                              : 0);
    /*
 * When the process group ID, PID, task or UID is selected and the process
 * group ID, PID, task or UID list option has been specified:
 *
 *	If list option ANDing has been specified, and the correct
 *	combination of selections are in place, reply that the process is no
 *	excluded;
 * or
 *	If list option ANDing has not been specified, reply that the
 *	process is not excluded by the tests of this function.
 */
    if (SelectionFlags & (SELPGID | SELPID | SELUID | SELTASK)) {
        if (OptAndSelection)
            return (((SelectionFlags & (SELPGID | SELPID | SELUID | SELTASK)) != *select_flags)
                        ? 1
                        : 0);
        return (0);
    }
    /*
 * Finally, when neither the process group ID, nor the PID, nor the UID, nor
 * the task is selected, and no applicable list option has been specified:
 *
 *	If list option ANDing has been specified, this process is
 *	excluded;
 *
 *	Otherwise, it isn't excluded by the tests of this function.
 */
    return (OptAndSelection ? 1 : 0);
}

/*
 * link_lfile() - link local file structures
 */

void link_lfile() {
    if (CurrentLocalFile->sel_flags & SELEXCLF)
        return;
    CurrentLocalProc->sel_state |= PS_SEC;
    if (PrevLocalFile)
        PrevLocalFile->next = CurrentLocalFile;
    else
        CurrentLocalProc->file = CurrentLocalFile;
    PrevLocalFile = CurrentLocalFile;
    if (OptNetwork && (CurrentLocalFile->sel_flags & SELNET))
        OptNetwork = 2;
    if (OptNfs && (CurrentLocalFile->sel_flags & SELNFS))
        OptNfs = 2;
    if (OptTask && (CurrentLocalFile->sel_flags & SELTASK))
        OptTask = 2;
    CurrentLocalFile = NULL;
}

#if defined(HASFSTRUCT)
/*
 * print_fflags() - print interpreted f_flag[s]
 */

char *print_fflags(long ffg, long pof) {
    int alloc_len, count, file_flags;
    static int buf_len = 0;
    static char *buf_ptr = NULL;
    char *sep;
    int sepl;
    struct pff_tab *tp;
    long work_flags;
    char xbuf[64];
    /*
 * Reduce the supplied flags according to the definitions in Pff_tab[] and
 * Pof_tab[].
 */
    for (count = file_flags = 0; file_flags < 2; file_flags++) {
        if (file_flags == 0) {
            sep = "";
            sepl = 0;
            tp = Pff_tab;
            work_flags = ffg;
        } else {
            sep = ";";
            sepl = 1;
            tp = Pof_tab;
            work_flags = pof;
        }
        for (; work_flags && !OptFileStructFlagHex; count += alloc_len) {
            while (tp->nm) {
                if (work_flags & tp->val)
                    break;
                tp++;
            }
            if (!tp->nm)
                break;
            alloc_len = (int)strlen(tp->nm) + sepl;
            buf_ptr = alloc_fflbuf(&buf_ptr, &buf_len, alloc_len + count);
            snpf(buf_ptr + count, alloc_len + 1, "%s%s", sep, tp->nm);
            sep = ",";
            sepl = 1;
            work_flags &= ~(tp->val);
        }
        /*
     * If flag bits remain, print them in hex.  If hex output was
     * specified with +fG, print all flag values, including zero,
     * in hex.
     */
        if (work_flags || OptFileStructFlagHex) {
            snpf(xbuf, sizeof(xbuf), "0x%lx", work_flags);
            alloc_len = (int)strlen(xbuf) + sepl;
            buf_ptr = alloc_fflbuf(&buf_ptr, &buf_len, alloc_len + count);
            snpf(buf_ptr + count, alloc_len + 1, "%s%s", sep, xbuf);
            count += alloc_len;
        }
    }
    /*
 * Make sure there is at least a NUL terminated reply.
 */
    if (!buf_ptr) {
        buf_ptr = alloc_fflbuf(&buf_ptr, &buf_len, 0);
        *buf_ptr = '\0';
    }
    return (buf_ptr);
}
#endif

/*
 * print_proc() - print process
 */

int print_proc() {
    char buf[128], *cp;
    int line_count, len, status, type;
    int rv = 0;
    unsigned long ulong_val;
    /*
 * If nothing in the process has been selected, skip it.
 */
    if (!CurrentLocalProc->sel_state)
        return (0);
    if (OptTerse) {

#if defined(HASTASKS)
        /*
         * If this is a task of a process, skip it.
         */
        if (CurrentLocalProc->tid)
            return (0);
#endif

        /*
         * The mode is terse and something in the process appears to have
         * been selected.  Make sure of that by looking for a selected file,
         * so that the HASSECURITY and HASNOSOCKSECURITY option combination
         * won't produce a false positive result.
         */
        for (CurrentLocalFile = CurrentLocalProc->file; CurrentLocalFile;
             CurrentLocalFile = CurrentLocalFile->next) {
            if (is_file_sel(CurrentLocalProc, CurrentLocalFile)) {
                printf("%d\n", CurrentLocalProc->pid);
                return (1);
            }
        }
        return (0);
    }
    /*
 * If fields have been selected, output the process-only ones, provided
 * that some file has also been selected.
 */
    if (OptFieldOutput) {
        for (CurrentLocalFile = CurrentLocalProc->file; CurrentLocalFile;
             CurrentLocalFile = CurrentLocalFile->next) {
            if (is_file_sel(CurrentLocalProc, CurrentLocalFile))
                break;
        }
        if (!CurrentLocalFile)
            return (rv);
        rv = 1;
        printf("%c%d%c", LSOF_FID_PID, CurrentLocalProc->pid, Terminator);

#if defined(HASTASKS)
        if (FieldSelection[LSOF_FIX_TID].st && CurrentLocalProc->tid)
            printf("%c%d%c", LSOF_FID_TID, CurrentLocalProc->tid, Terminator);
#endif

#if defined(HASZONES)
        if (FieldSelection[LSOF_FIX_ZONE].st && OptZone && CurrentLocalProc->zn)
            printf("%c%s%c", LSOF_FID_ZONE, CurrentLocalProc->zn, Terminator);
#endif

#if defined(HASSELINUX)
        if (FieldSelection[LSOF_FIX_SEC_CONTEXT].st && OptSecContext && CurrentLocalProc->cntx &&
            ContextStatus)
            printf("%c%s%c", LSOF_FID_SEC_CONTEXT, CurrentLocalProc->cntx, Terminator);
#endif

        if (FieldSelection[LSOF_FIX_PGID].st && OptProcessGroup)
            printf("%c%d%c", LSOF_FID_PGID, CurrentLocalProc->pgid, Terminator);

#if defined(HASPPID)
        if (FieldSelection[LSOF_FIX_PPID].st && OptParentPid)
            printf("%c%d%c", LSOF_FID_PPID, CurrentLocalProc->ppid, Terminator);
#endif

        if (FieldSelection[LSOF_FIX_CMD].st) {
            putchar(LSOF_FID_CMD);
            safestrprt(CurrentLocalProc->cmd ? CurrentLocalProc->cmd : "(unknown)", stdout, 0);
            putchar(Terminator);
        }
        if (FieldSelection[LSOF_FIX_UID].st)
            printf("%c%d%c", LSOF_FID_UID, (int)CurrentLocalProc->uid, Terminator);
        if (FieldSelection[LSOF_FIX_LOGIN].st) {
            cp = printuid((UID_ARG)CurrentLocalProc->uid, &type);
            if (type == 0)
                printf("%c%s%c", LSOF_FID_LOGIN, cp, Terminator);
        }
        if (Terminator == '\0')
            putchar('\n');
    }
    /*
 * Print files.
 */
    for (CurrentLocalFile = CurrentLocalProc->file; CurrentLocalFile;
         CurrentLocalFile = CurrentLocalFile->next) {
        if (!is_file_sel(CurrentLocalProc, CurrentLocalFile))
            continue;
        rv = 1;
        /*
         * If no field output selected, print dialects-specific formatted
         * output.
         */
        if (!OptFieldOutput) {
            print_file();
            continue;
        }
        /*
         * Print selected fields.
         */
        line_count = status = 0;
        if (FieldSelection[LSOF_FIX_FILE_DESC].st) {
            for (cp = CurrentLocalFile->fd; *cp == ' '; cp++)
                ;
            if (*cp) {
                printf("%c%s%c", LSOF_FID_FILE_DESC, cp, Terminator);
                line_count++;
            }
        }
        if (FieldSelection[LSOF_FIX_ACCESS].st) {
            printf("%c%c%c", LSOF_FID_ACCESS, CurrentLocalFile->access, Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_LOCK].st) {
            printf("%c%c%c", LSOF_FID_LOCK, CurrentLocalFile->lock, Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_TYPE].st) {
            for (cp = CurrentLocalFile->type; *cp == ' '; cp++)
                ;
            if (*cp) {
                printf("%c%s%c", LSOF_FID_TYPE, cp, Terminator);
                line_count++;
            }
        }

#if defined(HASFSTRUCT)
        if (FieldSelection[LSOF_FIX_FILE_STRUCT_ADDR].st && (OptFileStructValues & FSV_FILE_ADDR) &&
            (CurrentLocalFile->fsv & FSV_FILE_ADDR)) {
            printf("%c%s%c", LSOF_FID_FILE_STRUCT_ADDR, print_kptr(CurrentLocalFile->fsa, NULL, 0),
                   Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_FILE_STRUCT_COUNT].st &&
            (OptFileStructValues & FSV_FILE_COUNT) && (CurrentLocalFile->fsv & FSV_FILE_COUNT)) {
            printf("%c%ld%c", LSOF_FID_FILE_STRUCT_COUNT, CurrentLocalFile->fct, Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_FILE_FLAGS].st && (OptFileStructValues & FSV_FILE_FLAGS) &&
            (CurrentLocalFile->fsv & FSV_FILE_FLAGS) &&
            (OptFileStructFlagHex || CurrentLocalFile->ffg || CurrentLocalFile->pof)) {
            printf("%c%s%c", LSOF_FID_FILE_FLAGS,
                   print_fflags(CurrentLocalFile->ffg, CurrentLocalFile->pof), Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_NODE_ID].st && (OptFileStructValues & FSV_NODE_ID) &&
            (CurrentLocalFile->fsv & FSV_NODE_ID)) {
            printf("%c%s%c", LSOF_FID_NODE_ID, print_kptr(CurrentLocalFile->fna, NULL, 0),
                   Terminator);
            line_count++;
        }
#endif

        if (FieldSelection[LSOF_FIX_DEV_CHAR].st && CurrentLocalFile->dev_ch &&
            CurrentLocalFile->dev_ch[0]) {
            for (cp = CurrentLocalFile->dev_ch; *cp == ' '; cp++)
                ;
            if (*cp) {
                printf("%c%s%c", LSOF_FID_DEV_CHAR, cp, Terminator);
                line_count++;
            }
        }
        if (FieldSelection[LSOF_FIX_DEV_NUM].st && CurrentLocalFile->dev_def) {
            if (sizeof(unsigned long) > sizeof(dev_t))
                ulong_val = (unsigned long)((unsigned int)CurrentLocalFile->dev);
            else
                ulong_val = (unsigned long)CurrentLocalFile->dev;
            printf("%c0x%lx%c", LSOF_FID_DEV_NUM, ulong_val, Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_RDEV].st && CurrentLocalFile->rdev_def) {
            if (sizeof(unsigned long) > sizeof(dev_t))
                ulong_val = (unsigned long)((unsigned int)CurrentLocalFile->rdev);
            else
                ulong_val = (unsigned long)CurrentLocalFile->rdev;
            printf("%c0x%lx%c", LSOF_FID_RDEV, ulong_val, Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_SIZE].st && CurrentLocalFile->sz_def) {
            putchar(LSOF_FID_SIZE);

#if defined(HASPRINTSZ)
            cp = HASPRINTSZ(CurrentLocalFile);
#else
            snpf(buf, sizeof(buf), SizeOffFormatD, CurrentLocalFile->sz);
            cp = buf;
#endif

            printf("%s", cp);
            putchar(Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_OFFSET].st && CurrentLocalFile->off_def) {
            putchar(LSOF_FID_OFFSET);

#if defined(HASPRINTOFF)
            cp = HASPRINTOFF(CurrentLocalFile, 0);
#else
            snpf(buf, sizeof(buf), SizeOffFormat0t, CurrentLocalFile->off);
            cp = buf;
#endif

            len = strlen(cp);
            if (OffsetDecDigitLimit && len > (OffsetDecDigitLimit + 2)) {

#if defined(HASPRINTOFF)
                cp = HASPRINTOFF(CurrentLocalFile, 1);
#else
                snpf(buf, sizeof(buf), SizeOffFormatX, CurrentLocalFile->off);
                cp = buf;
#endif
            }
            printf("%s", cp);
            putchar(Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_INODE].st && CurrentLocalFile->inp_ty == 1) {
            putchar(LSOF_FID_INODE);
            printf(InodeFormatDecimal, CurrentLocalFile->inode);
            putchar(Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_NLINK].st && CurrentLocalFile->nlink_def) {
            printf("%c%ld%c", LSOF_FID_NLINK, CurrentLocalFile->nlink, Terminator);
            line_count++;
        }
        if (FieldSelection[LSOF_FIX_PROTO].st && CurrentLocalFile->inp_ty == 2) {
            for (cp = CurrentLocalFile->iproto; *cp == ' '; cp++)
                ;
            if (*cp) {
                printf("%c%s%c", LSOF_FID_PROTO, cp, Terminator);
                line_count++;
            }
        }
        if (FieldSelection[LSOF_FIX_STREAM].st && CurrentLocalFile->name &&
            CurrentLocalFile->is_stream) {
            if (strncmp(CurrentLocalFile->name, "STR:", 4) == 0 ||
                strcmp(CurrentLocalFile->iproto, "STR") == 0) {
                putchar(LSOF_FID_STREAM);
                printname(0);
                putchar(Terminator);
                line_count++;
                status++;
            }
        }
        if (status == 0 && FieldSelection[LSOF_FIX_NAME].st) {
            putchar(LSOF_FID_NAME);
            printname(0);
            putchar(Terminator);
            line_count++;
        }
        if (CurrentLocalFile->lts.type >= 0 && FieldSelection[LSOF_FIX_TCP_TPI_INFO].st) {
            print_tcptpi(0);
            line_count++;
        }
        if (Terminator == '\0' && line_count)
            putchar('\n');
    }
    return (rv);
}
