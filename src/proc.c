/*
 * proc.c - common process and file structure functions for lsof
 */


/*
 * Copyright 1994 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Victor A. Abell
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

#ifndef lint
static char copyright[] =
        "@(#) Copyright 1994 Purdue Research Foundation.\nAll rights reserved.\n";
static char *rcsid = "$Id: proc.c,v 1.46 2010/07/29 15:59:28 abe Exp $";
#endif


#include "lsof.h"


/*
 * Local function prototypes
 */

_PROTOTYPE(static int is_file_sel, (struct lproc *lp, struct lfile *lf));


/*
 * add_nma() - add to NAME column addition
 */

void
add_nma(cp, len)
        char *cp;            /* string to add */
        int len;            /* string length */
{
    int nl;

    if (!cp || !len)
        return;
    if (CurrentLocalFile->nma) {
        char *tmp;
        nl = (int) strlen(CurrentLocalFile->nma);
        tmp = (char *) realloc((MALLOC_P *) CurrentLocalFile->nma,
                               (MALLOC_S)(len + nl + 2));
        if (tmp)
            CurrentLocalFile->nma = tmp;
        else {
            (void) free((FREE_P *) CurrentLocalFile->nma);
            CurrentLocalFile->nma = (char *) NULL;
        }
    } else {
        nl = 0;
        CurrentLocalFile->nma = (char *) malloc((MALLOC_S)(len + 1));
    }
    if (!CurrentLocalFile->nma) {
        (void) fprintf(stderr, "%s: no name addition space: PID %ld, FD %s",
                       ProgramName, (long) CurrentLocalProc->pid, CurrentLocalFile->fd);
        Exit(1);
    }
    if (nl) {
        CurrentLocalFile->nma[nl] = ' ';
        (void) strncpy(&CurrentLocalFile->nma[nl + 1], cp, len);
        CurrentLocalFile->nma[nl + 1 + len] = '\0';
    } else {
        (void) strncpy(CurrentLocalFile->nma, cp, len);
        CurrentLocalFile->nma[len] = '\0';
    }
}


#if    defined(HASFSTRUCT)
_PROTOTYPE(static char *alloc_fflbuf,(char **bp, int *al, int lr));


/*
 * alloc_fflbuf() - allocate file flags print buffer
 */

static char *
alloc_fflbuf(bp, al, lr)
    char **bp;			/* current buffer pointer */
    int *al;			/* current allocated length */
    int lr;				/* length required */
{
    int sz;

    sz = (int)(lr + 1);		/* allocate '\0' space */
    if (*bp && (sz <= *al))
        return(*bp);
    if (*bp)
        *bp = (char *)realloc((MALLOC_P *)*bp, (MALLOC_S)sz);
    else
        *bp = (char *)malloc((MALLOC_S)sz);
    if (!*bp) {
        (void) fprintf(stderr, "%s: no space (%d) for print flags\n",
        ProgramName, sz);
        Exit(1);
    }
    *al = sz;
    return(*bp);
}
#endif    /* defined(HASFSTRUCT) */


/*
 * alloc_lfile() - allocate local file structure space
 */

void
alloc_lfile(nm, num)
        char *nm;            /* file descriptor name (may be NULL) */
        int num;            /* file descriptor number -- -1 if
					 * none */
{
    int fds;

    if (CurrentLocalFile) {
/*
 * If reusing a previously allocated structure, release any allocated
 * space it was using.
 */
        if (CurrentLocalFile->dev_ch)
            (void) free((FREE_P *) CurrentLocalFile->dev_ch);
        if (CurrentLocalFile->nm)
            (void) free((FREE_P *) CurrentLocalFile->nm);
        if (CurrentLocalFile->nma)
            (void) free((FREE_P *) CurrentLocalFile->nma);

#if    defined(HASLFILEADD) && defined(CLRLFILEADD)
        CLRLFILEADD(CurrentLocalFile)
#endif    /* defined(HASLFILEADD) && defined(CLRLFILEADD) */

/*
 * Othwerise, allocate a new structure.
 */
    } else if (!(CurrentLocalFile = (struct lfile *) malloc(sizeof(struct lfile)))) {
        (void) fprintf(stderr, "%s: no local file space at PID %d\n",
                       ProgramName, CurrentLocalProc->pid);
        Exit(1);
    }
/*
 * Initialize the structure.
 */
    CurrentLocalFile->access = CurrentLocalFile->lock = ' ';
    CurrentLocalFile->dev_def = CurrentLocalFile->inp_ty = CurrentLocalFile->is_com = CurrentLocalFile->is_nfs = CurrentLocalFile->is_stream
            = CurrentLocalFile->lmi_srch = CurrentLocalFile->nlink_def = CurrentLocalFile->off_def = CurrentLocalFile->sz_def
            = CurrentLocalFile->rdev_def
            = (unsigned char) 0;
    CurrentLocalFile->li[0].af = CurrentLocalFile->li[1].af = 0;
    CurrentLocalFile->lts.type = -1;
    CurrentLocalFile->nlink = 0l;

#if    defined(HASMNTSTAT)
    CurrentLocalFile->mnt_stat = (unsigned char)0;
#endif    /* defined(HASMNTSTAT) */

#if    defined(HASSOOPT)
    CurrentLocalFile->lts.kai = CurrentLocalFile->lts.ltm = 0;
    CurrentLocalFile->lts.opt = CurrentLocalFile->lts.qlen = CurrentLocalFile->lts.qlim = CurrentLocalFile->lts.pqlen
            = (unsigned int)0;
    CurrentLocalFile->lts.rbsz = CurrentLocalFile->lts.sbsz = (unsigned long)0;
    CurrentLocalFile->lts.qlens = CurrentLocalFile->lts.qlims = CurrentLocalFile->lts.pqlens = CurrentLocalFile->lts.rbszs
              = CurrentLocalFile->lts.sbszs = (unsigned char)0;
#endif    /* defined(HASSOOPT) */

#if    defined(HASSOSTATE)
    CurrentLocalFile->lts.ss = 0;
#endif    /* defined(HASSOSTATE) */

#if    defined(HASTCPOPT)
    CurrentLocalFile->lts.mss = (unsigned long)0;
    CurrentLocalFile->lts.msss = (unsigned char)0;
    CurrentLocalFile->lts.topt = (unsigned int)0;
#endif    /* defined(HASTCPOPT) */

#if    defined(HASTCPTPIQ)
    CurrentLocalFile->lts.rqs = CurrentLocalFile->lts.sqs = (unsigned char)0;
#endif    /* defined(HASTCPTPIQ) */

#if    defined(HASTCPTPIW)
    CurrentLocalFile->lts.rws = CurrentLocalFile->lts.wws = (unsigned char)0;
#endif    /* defined(HASTCPTPIW) */

#if    defined(HASFSINO)
    CurrentLocalFile->fs_ino = 0;
#endif    /* defined(HASFSINO) */

#if    defined(HASVXFS) && defined(HASVXFSDNLC)
    CurrentLocalFile->is_vxfs = 0;
#endif    /* defined(HASVXFS) && defined(HASVXFSDNLC) */

    CurrentLocalFile->inode = (INODETYPE) 0;
    CurrentLocalFile->off = (SZOFFTYPE) 0;
    if (CurrentLocalProc->pss & PS_PRI)
        CurrentLocalFile->sf = CurrentLocalProc->sf;
    else
        CurrentLocalFile->sf = 0;
    CurrentLocalFile->iproto[0] = CurrentLocalFile->type[0] = '\0';
    if (nm) {
        (void) strncpy(CurrentLocalFile->fd, nm, FDLEN - 1);
        CurrentLocalFile->fd[FDLEN - 1] = '\0';
    } else if (num >= 0) {
        if (num < 10000)
            (void) snpf(CurrentLocalFile->fd, sizeof(CurrentLocalFile->fd), "%4d", num);
        else
            (void) snpf(CurrentLocalFile->fd, sizeof(CurrentLocalFile->fd), "*%03d", num % 1000);
    } else
        CurrentLocalFile->fd[0] = '\0';
    CurrentLocalFile->dev_ch = CurrentLocalFile->fsdir = CurrentLocalFile->fsdev = CurrentLocalFile->nm = CurrentLocalFile->nma = (char *) NULL;
    CurrentLocalFile->ch = -1;

#if    defined(HASNCACHE) && HASNCACHE < 2
    CurrentLocalFile->na = (KA_T)NULL;
#endif    /* defined(HASNCACHE) && HASNCACHE<2 */

    CurrentLocalFile->next = (struct lfile *) NULL;
    CurrentLocalFile->ntype = NodeType = N_REGLR;
    NameChars[0] = '\0';

#if    defined(HASFSTRUCT)
    CurrentLocalFile->fct = CurrentLocalFile->ffg = CurrentLocalFile->pof = (long)0;
    CurrentLocalFile->fna = (KA_T)NULL;
    CurrentLocalFile->fsv = (unsigned char)0;
#endif    /* defined(HASFSTRUCT) */

#if    defined(HASLFILEADD) && defined(SETLFILEADD)
    /*
     * Do local initializations.
     */
        SETLFILEADD
#endif    /* defined(HASLFILEADD) && defined(SETLFILEADD) */

/*
 * See if the file descriptor has been selected.
 */
    if (!FdList || (!nm && num < 0))
        return;
    fds = ck_fd_status(nm, num);
    switch (FdListType) {
        case 0:            /* inclusion list */
            if (fds == 2)
                CurrentLocalFile->sf |= SELFD;
            break;
        case 1:            /* exclusion list */
            if (fds != 1)
                CurrentLocalFile->sf |= SELFD;
    }
}


/*
 * alloc_lproc() - allocate local proc structure space
 */

void
alloc_lproc(pid, pgid, ppid, uid, cmd, pss, sf)
        int pid;            /* Process ID */
        int pgid;            /* process group ID */
        int ppid;            /* parent process ID */
        UID_ARG uid;            /* User ID */
        char *cmd;            /* command */
        int pss;            /* process select state */
        int sf;                /* process select flags */
{
    static int sz = 0;

    if (!LocalProcTable) {
        if (!(LocalProcTable = (struct lproc *) malloc(
                (MALLOC_S)(LPROCINCR * sizeof(struct lproc))))) {
            (void) fprintf(stderr,
                           "%s: no malloc space for %d local proc structures\n",
                           ProgramName, LPROCINCR);
            Exit(1);
        }
        sz = LPROCINCR;
    } else if ((NumLocalProcs + 1) > sz) {
        struct lproc *tmp;
        sz += LPROCINCR;
        tmp = (struct lproc *) realloc((MALLOC_P *) LocalProcTable,
                                       (MALLOC_S)(sz * sizeof(struct lproc)));
        if (!tmp) {
            (void) fprintf(stderr,
                           "%s: no realloc space for %d local proc structures\n",
                           ProgramName, sz);
            (void) free((FREE_P *) LocalProcTable);
            LocalProcTable = (struct lproc *) NULL;
            Exit(1);
        }
        LocalProcTable = tmp;
    }
    CurrentLocalProc = &LocalProcTable[NumLocalProcs++];
    CurrentLocalProc->pid = pid;

#if    defined(HASTASKS)
    CurrentLocalProc->tid = 0;
#endif    /* defined(HASTASKS) */

    CurrentLocalProc->pgid = pgid;
    CurrentLocalProc->ppid = ppid;
    CurrentLocalProc->file = (struct lfile *) NULL;
    CurrentLocalProc->sf = (short) sf;
    CurrentLocalProc->pss = (short) pss;
    CurrentLocalProc->uid = (uid_t) uid;
/*
 * Allocate space for the full command name and copy it there.
 */
    if (!(CurrentLocalProc->cmd = mkstrcpy(cmd, (MALLOC_S *) NULL))) {
        (void) fprintf(stderr, "%s: PID %d, no space for command name: ",
                       ProgramName, pid);
        safestrprt(cmd, stderr, 1);
        Exit(1);
    }

#if    defined(HASZONES)
    /*
     * Clear the zone name pointer.  The dialect's own code will set it.
     */
        CurrentLocalProc->zn = (char *)NULL;
#endif    /* defined(HASZONES) */

#if    defined(HASSELINUX)
    /*
     * Clear the security context pointer.  The dialect's own code will
     * set it.
     */
        CurrentLocalProc->cntx = (char *)NULL;
#endif    /* defined(HASSELINUX) */

}


/*
 * ck_fd_status() - check FD status
 *
 * return: 0 == FD is neither included nor excluded
 *	   1 == FD is excluded
 *	   2 == FD is included
 */

extern int
ck_fd_status(nm, num)
        char *nm;            /* file descriptor name (may be NULL) */
        int num;            /* file descriptor number -- -1 if
					 * none */
{
    char *cp;
    struct fd_lst *fp;

    if (!(fp = FdList) || (!nm && num < 0))
        return (0);
    if ((cp = nm)) {
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

int
comppid(a1, a2)
        COMP_P *a1, *a2;
{
    struct lproc **p1 = (struct lproc **) a1;
    struct lproc **p2 = (struct lproc **) a2;

    if ((*p1)->pid < (*p2)->pid)
        return (-1);
    if ((*p1)->pid > (*p2)->pid)
        return (1);

#if    defined(HASTASKS)
    if ((*p1)->tid < (*p2)->tid)
        return(-1);
    if ((*p1)->tid > (*p2)->tid)
        return(1);
#endif    /* defined(HASTASKS) */

    return (0);
}


/*
 * ent_inaddr() - enter Internet addresses
 */

void
ent_inaddr(la, lp, fa, fp, af)
        unsigned char *la;        /* local Internet address */
        int lp;                /* local port */
        unsigned char *fa;        /* foreign Internet address -- may
					 * be NULL to indicate no foreign
					 * address is known */
        int fp;                /* foreign port */
        int af;                /* address family -- e.g, AF_INET,
					 * AF_INET */
{
    int m;

    if (la) {
        CurrentLocalFile->li[0].af = af;

#if    defined(HASIPv6)
        if (af == AF_INET6)
        CurrentLocalFile->li[0].ia.a6 = *(struct in6_addr *)la;
        else
#endif    /* defined(HASIPv6) */

        CurrentLocalFile->li[0].ia.a4 = *(struct in_addr *) la;
        CurrentLocalFile->li[0].p = lp;
    } else
        CurrentLocalFile->li[0].af = 0;
    if (fa) {
        CurrentLocalFile->li[1].af = af;

#if    defined(HASIPv6)
        if (af == AF_INET6)
        CurrentLocalFile->li[1].ia.a6 = *(struct in6_addr *)fa;
        else
#endif    /* defined(HASIPv6) */

        CurrentLocalFile->li[1].ia.a4 = *(struct in_addr *) fa;
        CurrentLocalFile->li[1].p = fp;
    } else
        CurrentLocalFile->li[1].af = 0;
/*
 * If network address matching has been selected, check both addresses.
 */
    if ((SelectionFlags & SELNA) && NetworkAddrList) {
        m = (fa && is_nw_addr(fa, fp, af)) ? 1 : 0;
        m |= (la && is_nw_addr(la, lp, af)) ? 1 : 0;
        if (m)
            CurrentLocalFile->sf |= SELNA;
    }
}


/*
 * examine_lproc() - examine local process
 *
 * return: 1 = last process
 */

int
examine_lproc() {
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
    if ((CurrentLocalProc->sf & SELPID) && !SelectAll) {
        if ((SelectionFlags == SELPID)
            || (OptAndSelection && (SelectionFlags & SELPID))) {
            sbp = 1;
            NumUnselectedPids--;
        }
    }
    if (CurrentLocalProc->pss && NumPidSelections == 1 && sbp) {
        print_init();
        (void) print_proc();
        PrintPass++;
        if (PrintPass < 2)
            (void) print_proc();
        CurrentLocalProc->pss = 0;
    }
/*
 * Deprecate an unselected (or listed) process.
 */
    if (!CurrentLocalProc->pss) {
        (void) free_lproc(CurrentLocalProc);
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

void
free_lproc(lp)
        struct lproc *lp;
{
    struct lfile *lf, *nf;

    for (lf = lp->file; lf; lf = nf) {
        if (lf->dev_ch) {
            (void) free((FREE_P *) lf->dev_ch);
            lf->dev_ch = (char *) NULL;
        }
        if (lf->nm) {
            (void) free((FREE_P *) lf->nm);
            lf->nm = (char *) NULL;
        }
        if (lf->nma) {
            (void) free((FREE_P *) lf->nma);
            lf->nma = (char *) NULL;
        }

#if    defined(HASLFILEADD) && defined(CLRLFILEADD)
        CLRLFILEADD(lf)
#endif    /* defined(HASLFILEADD) && defined(CLRLFILEADD) */

        nf = lf->next;
        (void) free((FREE_P *) lf);
    }
    lp->file = (struct lfile *) NULL;
    if (lp->cmd) {
        (void) free((FREE_P *) lp->cmd);
        lp->cmd = (char *) NULL;
    }

#if    defined(HASSELINUX)
    if (lp->cntx) {
        (void) free((FREE_P *) lp->cntx);
        lp->cntx = (char *) NULL;
    }
#endif    /* defined(HASSELINUX) */

}


/*
 * is_cmd_excl() - is command excluded?
 */

int
is_cmd_excl(cmd, pss, sf)
        char *cmd;            /* command name */
        short *pss;            /* process state */
        short *sf;            /* process select flags */
{
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
            *sf |= SELCMD;
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
            *sf |= SELCMD;
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

static int
is_file_sel(lp, lf)
        struct lproc *lp;        /* lproc structure pointer */
        struct lfile *lf;        /* lfile structure pointer */
{
    if (!lf || !lf->sf)
        return (0);
    if (CurrentLocalFile->sf & SELEXCLF)
        return (0);

#if    defined(HASSECURITY) && defined(HASNOSOCKSECURITY)
    if (MyRealUid && (MyRealUid != lp->uid)) {
        if (!(lf->sf & (SELNA | SELNET)))
        return(0);
    }
#endif    /* defined(HASSECURITY) && defined(HASNOSOCKSECURITY) */

    if (SelectAll)
        return (1);
    if (OptAndSelection && ((lf->sf & SelectionFlags) != SelectionFlags))
        return (0);
    return (1);
}


/*
 * is_proc_excl() - is process excluded?
 */

int

#if    defined(HASTASKS)
is_proc_excl(pid, pgid, uid, pss, sf, tid)
#else	/* !defined(HASTASKS) */
is_proc_excl(pid, pgid, uid, pss, sf)
#endif    /* defined(HASTASKS) */

        int pid;            /* Process ID */
        int pgid;            /* process group ID */
        UID_ARG uid;            /* User ID */
        short *pss;            /* process select state for lproc */
        short *sf;            /* select flags for lproc */

#if    defined(HASTASKS)
int tid;			/* task ID (not a task if zero) */
#endif    /* defined(HASTASKS) */

{
    int i, j;

    *pss = *sf = 0;

#if    defined(HASSECURITY)
    /*
     * The process is excluded by virtue of the security option if it
     * isn't owned by the owner of this lsof process, unless the
     * HASNOSOCKSECURITY option is also specified.  In that case the
     * selected socket files of any process may be listed.
     */
# if	!defined(HASNOSOCKSECURITY)
        if (MyRealUid && MyRealUid != (uid_t)uid)
            return(1);
# endif	/* !defined(HASNOSOCKSECURITY) */
#endif    /* defined(HASSECURITY) */

/*
 * If the excluding of process listing by UID has been specified, see if the
 * owner of this process is excluded.
 */
    if (NumUidExclusions) {
        for (i = j = 0; (i < NumUidSelections) && (j < NumUidExclusions); i++) {
            if (!SearchUidList[i].excl)
                continue;
            if (SearchUidList[i].uid == (uid_t) uid)
                return (1);
            j++;
        }
    }
/*
 * If the excluding of process listing by PGID has been specified, see if this
 * PGID is excluded.
 */
    if (NumPgidExclusions) {
        for (i = j = 0; (i < NumPgidSelections) && (j < NumPgidExclusions); i++) {
            if (!SearchPgidList[i].x)
                continue;
            if (SearchPgidList[i].i == pgid)
                return (1);
            j++;
        }
    }
/*
 * If the excluding of process listing by PID has been specified, see if this
 * PID is excluded.
 */
    if (NumPidExclusions) {
        for (i = j = 0; (i < NumPidSelections) && (j < NumPidExclusions); i++) {
            if (!SearchPidList[i].x)
                continue;
            if (SearchPidList[i].i == pid)
                return (1);
            j++;
        }
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

#if    defined(HASSECURITY) && defined(HASNOSOCKSECURITY)
        *sf = SELALL & ~(SELNA | SELNET);
#else	/* !defined(HASSECURITY) || !defined(HASNOSOCKSECURITY) */
        *sf = SELALL;
#endif    /* defined(HASSECURITY) && defined(HASNOSOCKSECURITY) */

        return (0);
    }
/*
 * If the listing of processes has been specified by process group ID, see
 * if this one is included or excluded.
 */
    if (NumPgidInclusions && (SelectionFlags & SELPGID)) {
        for (i = j = 0; (i < NumPgidSelections) && (j < NumPgidInclusions); i++) {
            if (SearchPgidList[i].x)
                continue;
            if (SearchPgidList[i].i == pgid) {
                SearchPgidList[i].f = 1;
                *pss = PS_PRI;
                *sf = SELPGID;
                if (SelectionFlags == SELPGID)
                    return (0);
                break;
            }
            j++;
        }
        if ((SelectionFlags == SELPGID) && !*sf)
            return (1);
    }
/*
 * If the listing of processes has been specified by PID, see if this one is
 * included or excluded.
 */
    if (NumPidInclusions && (SelectionFlags & SELPID)) {
        for (i = j = 0; (i < NumPidSelections) && (j < NumPidInclusions); i++) {
            if (SearchPidList[i].x)
                continue;
            if (SearchPidList[i].i == pid) {
                SearchPidList[i].f = 1;
                *pss = PS_PRI;
                *sf |= SELPID;
                if (SelectionFlags == SELPID)
                    return (0);
                break;
            }
            j++;
        }
        if ((SelectionFlags == SELPID) && !*sf)
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
            if (SearchUidList[i].uid == (uid_t) uid) {
                SearchUidList[i].f = 1;
                *pss = PS_PRI;
                *sf |= SELUID;
                if (SelectionFlags == SELUID)
                    return (0);
                break;
            }
            j++;
        }
        if (SelectionFlags == SELUID && (*sf & SELUID) == 0)
            return (1);
    }

#if    defined(HASTASKS)
    if ((SelectionFlags & SELTASK) && tid) {

    /*
     * This is a task and tasks are selected.
     */
        *pss = PS_PRI;
        *sf |= SELTASK;
        if ((SelectionFlags == SELTASK)
        ||  (OptAndSelection && ((*sf & SelectionFlags) == SelectionFlags)))
        return(0);
    }
#endif    /* defined(HASTASKS) */

/*
 * When neither the process group ID, nor the PID, nor the task, nor the UID
 * is selected:
 *
 *	If list option ANDing of process group IDs, PIDs, UIDs or tasks is
 *	specified, the process is excluded;
 *
 *	Otherwise, it's not excluded by the tests of this function.
 */
    if (!*sf)
        return ((OptAndSelection && (SelectionFlags & (SELPGID | SELPID | SELUID | SELTASK)))
                ? 1 : 0);
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
            return (((SelectionFlags & (SELPGID | SELPID | SELUID | SELTASK)) != *sf)
                    ? 1 : 0);
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

void
link_lfile() {
    if (CurrentLocalFile->sf & SELEXCLF)
        return;
    CurrentLocalProc->pss |= PS_SEC;
    if (PrevLocalFile)
        PrevLocalFile->next = CurrentLocalFile;
    else
        CurrentLocalProc->file = CurrentLocalFile;
    PrevLocalFile = CurrentLocalFile;
    if (OptNetwork && (CurrentLocalFile->sf & SELNET))
        OptNetwork = 2;
    if (OptNfs && (CurrentLocalFile->sf & SELNFS))
        OptNfs = 2;
    if (OptTask && (CurrentLocalFile->sf & SELTASK))
        OptTask = 2;
    CurrentLocalFile = (struct lfile *) NULL;
}


#if    defined(HASFSTRUCT)
/*
 * print_fflags() - print interpreted f_flag[s]
 */

char *
print_fflags(ffg, pof)
    long ffg;		/* file structure's flags value */
    long pof;		/* process open files flags value */
{
    int al, ct, fx;
    static int bl = 0;
    static char *bp = (char *)NULL;
    char *sep;
    int sepl;
    struct pff_tab *tp;
    long wf;
    char xbuf[64];
/*
 * Reduce the supplied flags according to the definitions in Pff_tab[] and
 * Pof_tab[].
 */
    for (ct = fx = 0; fx < 2; fx++) {
        if (fx == 0) {
        sep = "";
        sepl = 0;
        tp = Pff_tab;
        wf = ffg;
        } else {
        sep = ";";
        sepl = 1;
        tp = Pof_tab;
        wf = pof;
        }
        for (; wf && !OptFileStructFlagHex; ct += al ) {
        while (tp->nm) {
            if (wf & tp->val)
            break;
            tp++;
        }
        if (!tp->nm)
            break;
        al = (int)strlen(tp->nm) + sepl;
        bp = alloc_fflbuf(&bp, &bl, al + ct);
        (void) snpf(bp + ct, al + 1, "%s%s", sep, tp->nm);
        sep = ",";
        sepl = 1;
        wf &= ~(tp->val);
        }
    /*
     * If flag bits remain, print them in hex.  If hex output was
     * specified with +fG, print all flag values, including zero,
     * in hex.
     */
        if (wf || OptFileStructFlagHex) {
        (void) snpf(xbuf, sizeof(xbuf), "0x%lx", wf);
        al = (int)strlen(xbuf) + sepl;
        bp = alloc_fflbuf(&bp, &bl, al + ct);
        (void) snpf(bp + ct, al + 1, "%s%s", sep, xbuf);
        ct += al;
        }
    }
/*
 * Make sure there is at least a NUL terminated reply.
 */
    if (!bp) {
        bp = alloc_fflbuf(&bp, &bl, 0);
        *bp = '\0';
    }
    return(bp);
}
#endif    /* defined(HASFSTRUCT) */


/*
 * print_proc() - print process
 */

int
print_proc() {
    char buf[128], *cp;
    int lc, len, st, ty;
    int rv = 0;
    unsigned long ul;
/*
 * If nothing in the process has been selected, skip it.
 */
    if (!CurrentLocalProc->pss)
        return (0);
    if (OptTerse) {

#if    defined(HASTASKS)
        /*
         * If this is a task of a process, skip it.
         */
            if (CurrentLocalProc->tid)
            return(0);
#endif    /* defined(HASTASKS) */

        /*
         * The mode is terse and something in the process appears to have
         * been selected.  Make sure of that by looking for a selected file,
         * so that the HASSECURITY and HASNOSOCKSECURITY option combination
         * won't produce a false positive result.
         */
        for (CurrentLocalFile = CurrentLocalProc->file; CurrentLocalFile; CurrentLocalFile = CurrentLocalFile->next) {
            if (is_file_sel(CurrentLocalProc, CurrentLocalFile)) {
                (void) printf("%d\n", CurrentLocalProc->pid);
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
        for (CurrentLocalFile = CurrentLocalProc->file; CurrentLocalFile; CurrentLocalFile = CurrentLocalFile->next) {
            if (is_file_sel(CurrentLocalProc, CurrentLocalFile))
                break;
        }
        if (!CurrentLocalFile)
            return (rv);
        rv = 1;
        (void) printf("%c%d%c", LSOF_FID_PID, CurrentLocalProc->pid, Terminator);

#if    defined(HASTASKS)
        if (FieldSelection[LSOF_FIX_TID].st && CurrentLocalProc->tid)
        (void) printf("%c%d%c", LSOF_FID_TID, CurrentLocalProc->tid, Terminator);
#endif    /* defined(HASTASKS) */

#if    defined(HASZONES)
        if (FieldSelection[LSOF_FIX_ZONE].st && OptZone && CurrentLocalProc->zn)
        (void) printf("%c%s%c", LSOF_FID_ZONE, CurrentLocalProc->zn, Terminator);
#endif    /* defined(HASZONES) */

#if    defined(HASSELINUX)
        if (FieldSelection[LSOF_FIX_SEC_CONTEXT].st && OptSecContext && CurrentLocalProc->cntx && ContextStatus)
        (void) printf("%c%s%c", LSOF_FID_SEC_CONTEXT, CurrentLocalProc->cntx, Terminator);
#endif    /* defined(HASSELINUX) */

        if (FieldSelection[LSOF_FIX_PGID].st && OptProcessGroup)
            (void) printf("%c%d%c", LSOF_FID_PGID, CurrentLocalProc->pgid, Terminator);

#if    defined(HASPPID)
        if (FieldSelection[LSOF_FIX_PPID].st && OptParentPid)
        (void) printf("%c%d%c", LSOF_FID_PPID, CurrentLocalProc->ppid, Terminator);
#endif    /* defined(HASPPID) */

        if (FieldSelection[LSOF_FIX_CMD].st) {
            putchar(LSOF_FID_CMD);
            safestrprt(CurrentLocalProc->cmd ? CurrentLocalProc->cmd : "(unknown)", stdout, 0);
            putchar(Terminator);
        }
        if (FieldSelection[LSOF_FIX_UID].st)
            (void) printf("%c%d%c", LSOF_FID_UID, (int) CurrentLocalProc->uid, Terminator);
        if (FieldSelection[LSOF_FIX_LOGIN].st) {
            cp = printuid((UID_ARG) CurrentLocalProc->uid, &ty);
            if (ty == 0)
                (void) printf("%c%s%c", LSOF_FID_LOGIN, cp, Terminator);
        }
        if (Terminator == '\0')
            putchar('\n');
    }
/*
 * Print files.
 */
    for (CurrentLocalFile = CurrentLocalProc->file; CurrentLocalFile; CurrentLocalFile = CurrentLocalFile->next) {
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
        lc = st = 0;
        if (FieldSelection[LSOF_FIX_FILE_DESC].st) {
            for (cp = CurrentLocalFile->fd; *cp == ' '; cp++);
            if (*cp) {
                (void) printf("%c%s%c", LSOF_FID_FILE_DESC, cp, Terminator);
                lc++;
            }
        }
        if (FieldSelection[LSOF_FIX_ACCESS].st) {
            (void) printf("%c%c%c",
                          LSOF_FID_ACCESS, CurrentLocalFile->access, Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_LOCK].st) {
            (void) printf("%c%c%c", LSOF_FID_LOCK, CurrentLocalFile->lock, Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_TYPE].st) {
            for (cp = CurrentLocalFile->type; *cp == ' '; cp++);
            if (*cp) {
                (void) printf("%c%s%c", LSOF_FID_TYPE, cp, Terminator);
                lc++;
            }
        }

#if    defined(HASFSTRUCT)
        if (FieldSelection[LSOF_FIX_FILE_STRUCT_ADDR].st && (OptFileStructValues & FSV_FILE_ADDR)
        &&  (CurrentLocalFile->fsv & FSV_FILE_ADDR)) {
        (void) printf("%c%s%c", LSOF_FID_FILE_STRUCT_ADDR,
            print_kptr(CurrentLocalFile->fsa, (char *)NULL, 0), Terminator);
        lc++;
        }
        if (FieldSelection[LSOF_FIX_FILE_STRUCT_COUNT].st && (OptFileStructValues & FSV_FILE_COUNT)
        &&  (CurrentLocalFile->fsv & FSV_FILE_COUNT)) {
        (void) printf("%c%ld%c", LSOF_FID_FILE_STRUCT_COUNT, CurrentLocalFile->fct, Terminator);
        lc++;
        }
        if (FieldSelection[LSOF_FIX_FILE_FLAGS].st && (OptFileStructValues & FSV_FILE_FLAGS)
        &&  (CurrentLocalFile->fsv & FSV_FILE_FLAGS) && (OptFileStructFlagHex || CurrentLocalFile->ffg || CurrentLocalFile->pof)) {
        (void) printf("%c%s%c", LSOF_FID_FILE_FLAGS,
            print_fflags(CurrentLocalFile->ffg, CurrentLocalFile->pof), Terminator);
        lc++;
        }
        if (FieldSelection[LSOF_FIX_NODE_ID].st && (OptFileStructValues & FSV_NODE_ID)
        &&  (CurrentLocalFile->fsv & FSV_NODE_ID)) {
        (void) printf("%c%s%c", LSOF_FID_NODE_ID,
            print_kptr(CurrentLocalFile->fna, (char *)NULL, 0), Terminator);
        lc++;
        }
#endif    /* defined(HASFSTRUCT) */

        if (FieldSelection[LSOF_FIX_DEV_CHAR].st && CurrentLocalFile->dev_ch && CurrentLocalFile->dev_ch[0]) {
            for (cp = CurrentLocalFile->dev_ch; *cp == ' '; cp++);
            if (*cp) {
                (void) printf("%c%s%c", LSOF_FID_DEV_CHAR, cp, Terminator);
                lc++;
            }
        }
        if (FieldSelection[LSOF_FIX_DEV_NUM].st && CurrentLocalFile->dev_def) {
            if (sizeof(unsigned long) > sizeof(dev_t))
                ul = (unsigned long) ((unsigned int) CurrentLocalFile->dev);
            else
                ul = (unsigned long) CurrentLocalFile->dev;
            (void) printf("%c0x%lx%c", LSOF_FID_DEV_NUM, ul, Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_RDEV].st && CurrentLocalFile->rdev_def) {
            if (sizeof(unsigned long) > sizeof(dev_t))
                ul = (unsigned long) ((unsigned int) CurrentLocalFile->rdev);
            else
                ul = (unsigned long) CurrentLocalFile->rdev;
            (void) printf("%c0x%lx%c", LSOF_FID_RDEV, ul, Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_SIZE].st && CurrentLocalFile->sz_def) {
            putchar(LSOF_FID_SIZE);

#if    defined(HASPRINTSZ)
            cp = HASPRINTSZ(CurrentLocalFile);
#else	/* !defined(HASPRINTSZ) */
            (void) snpf(buf, sizeof(buf), SizeOffFormatD, CurrentLocalFile->sz);
            cp = buf;
#endif    /* defined(HASPRINTSZ) */

            (void) printf("%s", cp);
            putchar(Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_OFFSET].st && CurrentLocalFile->off_def) {
            putchar(LSOF_FID_OFFSET);

#if    defined(HASPRINTOFF)
            cp = HASPRINTOFF(CurrentLocalFile, 0);
#else	/* !defined(HASPRINTOFF) */
            (void) snpf(buf, sizeof(buf), SizeOffFormat0t, CurrentLocalFile->off);
            cp = buf;
#endif    /* defined(HASPRINTOFF) */

            len = strlen(cp);
            if (OffsetDecDigitLimit && len > (OffsetDecDigitLimit + 2)) {

#if    defined(HASPRINTOFF)
                cp = HASPRINTOFF(CurrentLocalFile, 1);
#else	/* !defined(HASPRINTOFF) */
                (void) snpf(buf, sizeof(buf), SizeOffFormatX, CurrentLocalFile->off);
                cp = buf;
#endif    /* defined(HASPRINTOFF) */

            }
            (void) printf("%s", cp);
            putchar(Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_INODE].st && CurrentLocalFile->inp_ty == 1) {
            putchar(LSOF_FID_INODE);
            (void) printf(InodeFormatDecimal, CurrentLocalFile->inode);
            putchar(Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_NLINK].st && CurrentLocalFile->nlink_def) {
            (void) printf("%c%ld%c", LSOF_FID_NLINK, CurrentLocalFile->nlink, Terminator);
            lc++;
        }
        if (FieldSelection[LSOF_FIX_PROTO].st && CurrentLocalFile->inp_ty == 2) {
            for (cp = CurrentLocalFile->iproto; *cp == ' '; cp++);
            if (*cp) {
                (void) printf("%c%s%c", LSOF_FID_PROTO, cp, Terminator);
                lc++;
            }
        }
        if (FieldSelection[LSOF_FIX_STREAM].st && CurrentLocalFile->nm && CurrentLocalFile->is_stream) {
            if (strncmp(CurrentLocalFile->nm, "STR:", 4) == 0
                || strcmp(CurrentLocalFile->iproto, "STR") == 0) {
                putchar(LSOF_FID_STREAM);
                printname(0);
                putchar(Terminator);
                lc++;
                st++;
            }
        }
        if (st == 0 && FieldSelection[LSOF_FIX_NAME].st) {
            putchar(LSOF_FID_NAME);
            printname(0);
            putchar(Terminator);
            lc++;
        }
        if (CurrentLocalFile->lts.type >= 0 && FieldSelection[LSOF_FIX_TCP_TPI_INFO].st) {
            print_tcptpi(0);
            lc++;
        }
        if (Terminator == '\0' && lc)
            putchar('\n');
    }
    return (rv);
}
