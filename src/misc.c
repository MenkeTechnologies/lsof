/*
 * misc.c - common miscellaneous functions for lsof
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

#if    defined(HASWIDECHAR) && defined(WIDECHARINCL)
#include WIDECHARINCL
#endif    /* defined(HASWIDECHAR) && defined(WIDECHARINCL) */


/*
 * Local definitions
 */

#if    !defined(MAXSYMLINKS)
#define    MAXSYMLINKS    32
#endif    /* !defined(MAXSYMLINKS) */


/*
 * Local function prototypes
 */

_PROTOTYPE(static void closePipes, (void));

_PROTOTYPE(static int dolstat, (char *path, char *buf, int len));

_PROTOTYPE(static int dostat, (char *path, char *buf, int len));

_PROTOTYPE(static int doreadlink, (char *path, char *buf, int len));

_PROTOTYPE(static int doinchild, (int (*func)(), char *func_arg, char *rbuf, int rbln));

#if    defined(HASINTSIGNAL)
_PROTOTYPE(static int handleint,(int sig));
#else	/* !defined(HASINTSIGNAL) */

_PROTOTYPE(static void handleint, (int sig));

#endif    /* defined(HASINTSIGNAL) */

_PROTOTYPE(static char *safepup, (unsigned int ch, int *char_len));


/*
 * Local variables
 */

static pid_t Cpid = 0;            /* child PID */
static jmp_buf Jmp_buf;            /* jump buffer */
static int Pipes[] =            /* pipes for child process */
        {-1, -1, -1, -1};
static int CtSigs[] = {0, SIGINT, SIGKILL};
/* child termination signals (in order
 * of application) -- the first is a
 * dummy to allow pipe closure to
 * cause the child to exit */
#define    NCTSIGS    (sizeof(CtSigs) / sizeof(int))


#if    defined(HASNLIST)
/*
 * build-NlistTable() - build kernel name list table
 */

static struct drive_Nl *Build_Nl = (struct drive_Nl *)NULL;
                    /* the default Drive_Nl address */

void
build_Nl(drv)
    struct drive_Nl *drv;		/* data to drive the construction */
{
    struct drive_Nl *drive_ptr;
    int i, num;

    for (drive_ptr = drv, num = 0; drive_ptr->nn; drive_ptr++, num++)
        ;
    if (num < 1) {
        (void) fprintf(stderr,
        "%s: can't calculate kernel name list length\n", ProgramName);
        Exit(1);
    }
    if (!(NlistTable = (struct NLIST_TYPE *)calloc((num + 1),
                           sizeof(struct NLIST_TYPE))))
    {
        (void) fprintf(stderr,
        "%s: can't allocate %d bytes to kernel name list structure\n",
        ProgramName, (int)((num + 1) * sizeof(struct NLIST_TYPE)));
        Exit(1);
    }
    for (drive_ptr = drv, i = 0; i < num; drive_ptr++, i++) {
        NlistTable[i].NL_NAME = drive_ptr->knm;
    }
    NlistLength = (int)((num + 1) * sizeof(struct NLIST_TYPE));
    Build_Nl = drv;
}
#endif    /* defined(HASNLIST) */


/*
 * childx() - make child process exit (if possible)
 */

void
childx() {
    static int alarm_time, sig_idx;
    pid_t wpid;

    if (Cpid > 1) {

        /*
         * First close the pipes to and from the child.  That should cause the
         * child to exit.  Compute alarm time shares.
         */
        (void) closePipes();
        if ((alarm_time = TimeoutLimit / NCTSIGS) < TMLIMMIN)
            alarm_time = TMLIMMIN;
        /*
         * Loop, waiting for the child to exit.  After the first pass, help
         * the child exit by sending it signals.
         */
        for (sig_idx = 0; sig_idx < NCTSIGS; sig_idx++) {
            if (setjmp(Jmp_buf)) {

                /*
                 * An alarm has rung.  Disable further alarms.
                 *
                 * If there are more signals to send, continue the signal loop.
                 *
                 * If the last signal has been sent, issue a warning (unless
                 * warninge have been suppressed) and exit the signal loop.
                 */
                (void) alarm(0);
                (void) signal(SIGALRM, SIG_DFL);
                if (sig_idx < (NCTSIGS - 1))
                    continue;
                if (!OptWarnings)
                    (void) fprintf(stderr,
                                   "%s: WARNING -- child process %d may be hung.\n",
                                   ProgramName, (int) Cpid);
                break;
            }
            /*
             * Send the next signal to the child process, after the first pass
             * through the loop.
             *
             * Wrap the wait() with an alarm.
             */
            if (sig_idx)
                (void) kill(Cpid, CtSigs[sig_idx]);
            (void) signal(SIGALRM, handleint);
            (void) alarm(alarm_time);
            wpid = (pid_t) wait(NULL);
            (void) alarm(0);
            (void) signal(SIGALRM, SIG_DFL);
            if (wpid == Cpid)
                break;
        }
        Cpid = 0;
    }
}


/*
 * closePipes() - close open pipe file descriptors
 */

static void
closePipes() {
    int i;

    for (i = 0; i < 4; i++) {
        if (Pipes[i] >= 0) {
            (void) close(Pipes[i]);
            Pipes[i] = -1;
        }
    }
}


/*
 * compdev() - compare DeviceTable[] entries
 */

int
compdev(lhs, rhs)
        COMP_P *lhs, *rhs;
{
    struct l_dev **p1 = (struct l_dev **) lhs;
    struct l_dev **p2 = (struct l_dev **) rhs;

    if ((dev_t)((*p1)->rdev) < (dev_t)((*p2)->rdev))
        return (-1);
    if ((dev_t)((*p1)->rdev) > (dev_t)((*p2)->rdev))
        return (1);
    if ((INODETYPE) ((*p1)->inode) < (INODETYPE) ((*p2)->inode))
        return (-1);
    if ((INODETYPE) ((*p1)->inode) > (INODETYPE) ((*p2)->inode))
        return (1);
    return (strcmp((*p1)->name, (*p2)->name));
}


/*
 * doinchild() -- do a function in a child process
 */

static int
doinchild(func, func_arg, rbuf, rbln)
        int (*func)();            /* function to perform */
        char *func_arg;            /* function parameter */
        char *rbuf;            /* response buffer */
        int rbln;            /* response buffer length */
{
    int errno_val, return_val;
/*
 * Check reply buffer size.
 */
    if (!OptOverhead && rbln > MAXPATHLEN) {
        (void) fprintf(stderr,
                       "%s: doinchild error; response buffer too large: %d\n",
                       ProgramName, rbln);
        Exit(1);
    }
/*
 * Set up to handle an alarm signal; handle an alarm signal; build
 * pipes for exchanging information with a child process; start the
 * child process; and perform functions in the child process.
 */
    if (!OptOverhead) {
        if (setjmp(Jmp_buf)) {

            /*
             * Process an alarm that has rung.
             */
            (void) alarm(0);
            (void) signal(SIGALRM, SIG_DFL);
            (void) childx();
            errno = ETIMEDOUT;
            return (1);
        } else if (!Cpid) {

            /*
             * Create pipes to exchange function information with a child
             * process.
             */
            if (pipe(Pipes) < 0 || pipe(&Pipes[2]) < 0) {
                (void) fprintf(stderr, "%s: can't open pipes: %s\n",
                               ProgramName, strerror(errno));
                Exit(1);
            }
            /*
             * Fork a child to execute functions.
             */
            if ((Cpid = fork()) == 0) {

                /*
                 * Begin the child process.
                 */

                int file_desc, node_desc, r_al, r_rbln;
                char r_arg[MAXPATHLEN + 1], r_rbuf[MAXPATHLEN + 1];
                int (*r_fn)();
                /*
                 * Close all open file descriptors except Pipes[0] and
                 * Pipes[3].
                 */
                for (file_desc = 0, node_desc = GET_MAX_FD(); file_desc < node_desc; file_desc++) {
                    if (file_desc == Pipes[0] || file_desc == Pipes[3])
                        continue;
                    (void) close(file_desc);
                    if (file_desc == Pipes[1])
                        Pipes[1] = -1;
                    else if (file_desc == Pipes[2])
                        Pipes[2] = -1;
                }
                if (Pipes[1] >= 0) {
                    (void) close(Pipes[1]);
                    Pipes[1] = -1;
                }
                if (Pipes[2] >= 0) {
                    (void) close(Pipes[2]);
                    Pipes[2] = -1;
                }
                /*
                 * Read function requests, process them, and return replies.
                 */
                for (;;) {
                    if (read(Pipes[0], (char *) &r_fn, sizeof(r_fn))
                        != (int) sizeof(r_fn)
                        || read(Pipes[0], (char *) &r_al, sizeof(int))
                           != (int) sizeof(int)
                        || r_al < 1
                        || r_al > (int) sizeof(r_arg)
                        || read(Pipes[0], r_arg, r_al) != r_al
                        || read(Pipes[0], (char *) &r_rbln, sizeof(r_rbln))
                           != (int) sizeof(r_rbln)
                        || r_rbln < 1 || r_rbln > (int) sizeof(r_rbuf))
                        break;
                    return_val = r_fn(r_arg, r_rbuf, r_rbln);
                    errno_val = errno;
                    if (write(Pipes[3], (char *) &return_val, sizeof(return_val))
                        != sizeof(return_val)
                        || write(Pipes[3], (char *) &errno_val, sizeof(errno_val))
                           != sizeof(errno_val)
                        || write(Pipes[3], r_rbuf, r_rbln) != r_rbln)
                        break;
                }
                (void) _exit(0);
            }
            /*
             * Continue in the parent process to finish the setup.
             */
            if (Cpid < 0) {
                (void) fprintf(stderr, "%s: can't fork: %s\n",
                               ProgramName, strerror(errno));
                Exit(1);
            }
            (void) close(Pipes[0]);
            (void) close(Pipes[3]);
            Pipes[0] = Pipes[3] = -1;
        }
    }
    if (!OptOverhead) {
        int len;

        /*
         * Send a function to the child and wait for the response.
         */
        len = strlen(func_arg) + 1;
        (void) signal(SIGALRM, handleint);
        (void) alarm(TimeoutLimit);
        if (write(Pipes[1], (char *) &func, sizeof(func)) != sizeof(func)
            || write(Pipes[1], (char *) &len, sizeof(len)) != sizeof(len)
            || write(Pipes[1], func_arg, len) != len
            || write(Pipes[1], (char *) &rbln, sizeof(rbln)) != sizeof(rbln)
            || read(Pipes[2], (char *) &return_val, sizeof(return_val)) != sizeof(return_val)
            || read(Pipes[2], (char *) &errno_val, sizeof(errno_val)) != sizeof(errno_val)
            || read(Pipes[2], rbuf, rbln) != rbln) {
            (void) alarm(0);
            (void) signal(SIGALRM, SIG_DFL);
            (void) childx();
            errno = ECHILD;
            return (-1);
        }
    } else {

        /*
         * Do the operation directly -- not in a child.
         */
        (void) signal(SIGALRM, handleint);
        (void) alarm(TimeoutLimit);
        return_val = func(func_arg, rbuf, rbln);
        errno_val = errno;
    }
/*
 * Function completed, response collected -- complete the operation.
 */
    (void) alarm(0);
    (void) signal(SIGALRM, SIG_DFL);
    errno = errno_val;
    return (return_val);
}


/*
 * dolstat() - do an lstat() function
 */

static int
dolstat(path, rbuf, rbln)
        char *path;            /* path */
        char *rbuf;            /* response buffer */
        int rbln;            /* response buffer length */

/* ARGSUSED */

{
    return (lstat(path, (struct stat *) rbuf));
}


/*
 * doreadlink() -- do a readlink() function
 */

static int
doreadlink(path, rbuf, rbln)
        char *path;            /* path */
        char *rbuf;            /* response buffer */
        int rbln;            /* response buffer length */
{
    return (readlink(path, rbuf, rbln));
}


/*
 * dostat() - do a stat() function
 */

static int
dostat(path, rbuf, rbln)
        char *path;            /* path */
        char *rbuf;            /* response buffer */
        int rbln;            /* response buffer length */

/* ARGSUSED */

{
    return (stat(path, (struct stat *) rbuf));
}


#if    defined(WILLDROPGID)
/*
 * dropgid() - drop setgid permission
 */

void
dropgid()
{
    if (!SetuidRootState && SetgidState) {
        if (setgid(MyRealGid) < 0) {
        (void) fprintf(stderr, "%s: can't setgid(%d): %s\n",
            ProgramName, (int)MyRealGid, strerror(errno));
        Exit(1);
        }
        SetgidState = 0;
    }
}
#endif    /* defined(WILLDROPGID) */


/*
 * enter_dev_ch() - enter device characters in file structure
 */

void
enter_dev_ch(msg)
        char *msg;
{
    char *msg_ptr;

    if (!msg || *msg == '\0')
        return;
    if (!(msg_ptr = mkstrcpy(msg, (MALLOC_S *) NULL))) {
        (void) fprintf(stderr, "%s: no more dev_ch space at PID %d: \n",
                       ProgramName, CurrentLocalProc->pid);
        safestrprt(msg, stderr, 1);
        Exit(1);
    }
    if (CurrentLocalFile->dev_ch)
        (void) free((FREE_P *) CurrentLocalFile->dev_ch);
    CurrentLocalFile->dev_ch = msg_ptr;
}


/*
 * enter_IPstate() -- enter a TCP or UDP state
 */

void
enter_IPstate(type, name, state_num)
        char *type;            /* type -- TCP or UDP */
        char *name;            /* state name (may be NULL) */
        int state_num;                /* state number */
{

#if    defined(USE_LIB_PRINT_TCPTPI)
    TcpNumStates = state_num;
#else	/* !defined(USE_LIB_PRINT_TCPTPI) */

    int alloc_len, i, j, occur_count, new_num, num_states, off, tx;
    char *char_ptr;
    MALLOC_S len;
/*
 * Check the type name and set the type index.
 */
    if (!type) {
        (void) fprintf(stderr,
                       "%s: no type specified to enter_IPstate()\n", ProgramName);
        Exit(1);
    }
    if (!strcmp(type, "TCP"))
        tx = 0;
    else if (!strcmp(type, "UDP"))
        tx = 1;
    else {
        (void) fprintf(stderr, "%s: unknown type for enter_IPstate: %s\n",
                       ProgramName, type);
        Exit(1);
    }
/*
 * If the name argument is NULL, reduce the allocated table to its minimum
 * size.
 */
    if (!name) {
        if (tx) {
            if (UdpStateNames) {
                if (!UdpNumStates) {
                    (void) free((MALLOC_P *) UdpStateNames);
                    UdpStateNames = (char **) NULL;
                }
                if (UdpNumStates < UdpStateAlloc) {
                    char **tmp;
                    len = (MALLOC_S)(UdpNumStates * sizeof(char *));
                    tmp = (char **) realloc((MALLOC_P *) UdpStateNames, len);
                    if (!tmp) {
                        (void) fprintf(stderr,
                                       "%s: can't reduce UdpStateNames[]\n", ProgramName);
                        (void) free((FREE_P *) UdpStateNames);
                        UdpStateNames = (char **) NULL;
                        Exit(1);
                    }
                    UdpStateNames = tmp;
                }
                UdpStateAlloc = UdpNumStates;
            }
        } else {
            if (TcpStateNames) {
                if (!TcpNumStates) {
                    (void) free((MALLOC_P *) TcpStateNames);
                    TcpStateNames = (char **) NULL;
                }
                if (TcpNumStates < TcpStateAlloc) {
                    char **tmp;
                    len = (MALLOC_S)(TcpNumStates * sizeof(char *));
                    tmp = (char **) realloc((MALLOC_P *) TcpStateNames, len);
                    if (!tmp) {
                        (void) fprintf(stderr,
                                       "%s: can't reduce TcpStateNames[]\n", ProgramName);
                        (void) free((FREE_P *) TcpStateNames);
                        TcpStateNames = (char **) NULL;
                        Exit(1);
                    }
                    TcpStateNames = tmp;
                }
                TcpStateAlloc = TcpNumStates;
            }
        }
        return;
    }
/*
 * Check the name and number.
 */
    if ((len = (size_t) strlen(name)) < 1) {
        (void) fprintf(stderr,
                       "%s: bad %s name (\"%s\"), number=%d\n", ProgramName, type, name, state_num);
        Exit(1);
    }
/*
 * Make a copy of the name.
 */
    if (!(char_ptr = mkstrcpy(name, (MALLOC_S *) NULL))) {
        (void) fprintf(stderr,
                       "%s: enter_IPstate(): no %s space for %s\n",
                       ProgramName, type, name);
        Exit(1);
    }
/*
 * Set the necessary offset for using state_num as an index.  If it is
 * a new offset, adjust previous entries.
 */
    if ((state_num < 0) && ((off = -state_num) > (tx ? UdpStateOffset : TcpStateOffset))) {
        if (tx ? UdpStateNames : TcpStateNames) {

            /*
             * A new, larger offset (smaller negative state number) could mean
             * a previously allocated state table must be enlarged and its
             * previous entries moved.
             */
            occur_count = off - (tx ? UdpStateOffset : TcpStateOffset);
            alloc_len = tx ? UdpStateAlloc : TcpStateAlloc;
            num_states = tx ? UdpNumStates : TcpNumStates;
            if ((new_num = num_states + occur_count) >= alloc_len) {
                while ((new_num + 5) > alloc_len) {
                    alloc_len += TCPUDPALLOC;
                }
                len = (MALLOC_S)(alloc_len * sizeof(char *));
                if (tx) {
                    char **tmp = (char **) realloc((MALLOC_P *) UdpStateNames, len);
                    if (!tmp) {
                        (void) free((FREE_P *) UdpStateNames);
                        UdpStateNames = (char **) NULL;
                        goto no_IP_space;
                    }
                    UdpStateNames = tmp;
                    UdpStateAlloc = alloc_len;
                } else {
                    char **tmp = (char **) realloc((MALLOC_P *) TcpStateNames, len);
                    if (!tmp) {
                        (void) free((FREE_P *) TcpStateNames);
                        TcpStateNames = (char **) NULL;
                        goto no_IP_space;
                    }
                    TcpStateNames = tmp;
                    TcpStateAlloc = alloc_len;
                }
                for (i = 0, j = occur_count; i < occur_count; i++, j++) {
                    if (tx) {
                        if (i < UdpNumStates)
                            UdpStateNames[j] = UdpStateNames[i];
                        UdpStateNames[i] = (char *) NULL;
                    } else {
                        if (i < TcpNumStates)
                            TcpStateNames[j] = TcpStateNames[i];
                        TcpStateNames[i] = (char *) NULL;
                    }
                }
                if (tx)
                    UdpNumStates += occur_count;
                else
                    TcpNumStates += occur_count;
            }
        }
        if (tx)
            UdpStateOffset = off;
        else
            TcpStateOffset = off;
    }
/*
 * Enter name as {Tc|Ud}pSt[state_num + {Tc|Ud}pStOff].
 *
 * Allocate space, as required.
 */
    alloc_len = tx ? UdpStateAlloc : TcpStateAlloc;
    off = tx ? UdpStateOffset : TcpStateOffset;
    new_num = state_num + off + 1;
    if (new_num > alloc_len) {
        i = tx ? UdpNumStates : TcpNumStates;
        while ((new_num + 5) > alloc_len) {
            alloc_len += TCPUDPALLOC;
        }
        len = (MALLOC_S)(alloc_len * sizeof(char *));
        if (tx) {
            if (UdpStateNames) {
                char **tmp = (char **) realloc((MALLOC_P *) UdpStateNames, len);
                if (!tmp) {
                    (void) free((FREE_P *) UdpStateNames);
                    UdpStateNames = (char **) NULL;
                } else
                    UdpStateNames = tmp;
            } else
                UdpStateNames = (char **) malloc(len);
            if (!UdpStateNames) {

                no_IP_space:

                (void) fprintf(stderr, "%s: no %s state space\n", ProgramName, type);
                Exit(1);
            }
            UdpNumStates = new_num;
            UdpStateAlloc = alloc_len;
        } else {
            if (TcpStateNames) {
                char **tmp = (char **) realloc((MALLOC_P *) TcpStateNames, len);
                if (!tmp) {
                    (void) free((FREE_P *) TcpStateNames);
                    TcpStateNames = (char **) NULL;
                } else
                    TcpStateNames = tmp;
            } else
                TcpStateNames = (char **) malloc(len);
            if (!TcpStateNames)
                goto no_IP_space;
            TcpNumStates = new_num;
            TcpStateAlloc = alloc_len;
        }
        while (i < alloc_len) {
            if (tx)
                UdpStateNames[i] = (char *) NULL;
            else
                TcpStateNames[i] = (char *) NULL;
            i++;
        }
    } else {
        if (tx) {
            if (new_num > UdpNumStates)
                UdpNumStates = new_num;
        } else {
            if (new_num > TcpNumStates)
                TcpNumStates = new_num;
        }
    }
    if (tx) {
        if (UdpStateNames[state_num + UdpStateOffset]) {

            dup_IP_state:

            (void) fprintf(stderr,
                           "%s: duplicate %s state %d (already %s): %s\n",
                           ProgramName, type, state_num,
                           tx ? UdpStateNames[state_num + UdpStateOffset] : TcpStateNames[state_num + TcpStateOffset],
                           name);
            Exit(1);
        }
        UdpStateNames[state_num + UdpStateOffset] = char_ptr;
    } else {
        if (TcpStateNames[state_num + TcpStateOffset])
            goto dup_IP_state;
        TcpStateNames[state_num + TcpStateOffset] = char_ptr;
    }
#endif    /* defined(USE_LIB_PRINT_TCPTPI) */

}


/*
 * enter_nm() - enter name in local file structure
 */

void
enter_nm(name)
        char *name;
{
    char *name_ptr;

    if (!name || *name == '\0')
        return;
    if (!(name_ptr = mkstrcpy(name, (MALLOC_S *) NULL))) {
        (void) fprintf(stderr, "%s: no more nm space at PID %d for: ",
                       ProgramName, CurrentLocalProc->pid);
        safestrprt(name, stderr, 1);
        Exit(1);
    }
    if (CurrentLocalFile->name)
        (void) free((FREE_P *) CurrentLocalFile->name);
    CurrentLocalFile->name = name_ptr;
}


/*
 * Exit() - do a clean exit()
 */

void
Exit(exit_val)
        int exit_val;                /* exit() value */
{
    (void) childx();

#if    defined(HASDCACHE)
    if (DevCacheRebuilt && !OptWarnings)
        (void) fprintf(stderr, "%s: WARNING: %s was updated.\n",
        ProgramName, DevCachePath[DevCachePathIndex]);
#endif    /* defined(HASDCACHE) */

    exit(exit_val);
}


#if    defined(HASNLIST)
/*
 * get_Nl_value() - get NlistTable value for nickname
 */

int
get_Nl_value(nickname, drv, value)
    char *nickname;			/* nickname of requested entry */
    struct drive_Nl *drv;		/* drive_Nl table that built NlistTable
					 * (if NULL, use Build_Nl) */
    KA_T *value;			/* returned value (if NULL,
					 * return nothing) */
{
    int i;

    if (!NlistTable || !NlistLength)
        return(-1);
    if (!drv)
        drv = Build_Nl;
    for (i = 0; drv->nn; drv++, i++) {
        if (strcmp(drv->nn, nickname) == 0) {
        if (value)
            *value = (KA_T)NlistTable[i].n_value;
        return(i);
        }
    }
    return(-1);
}
#endif    /* defined(HASNLIST) */


/*
 * handleint() - handle an interrupt
 */

#if    defined(HASINTSIGNAL)
static int
#else
static void
#endif

/* ARGSUSED */

handleint(sig)
        int sig;
{
    longjmp(Jmp_buf, 1);
}


/*
 * hashbyname() - hash by name
 */

int
hashbyname(name, mod)
        char *name;            /* pointer to NUL-terminated name */
        int mod;            /* hash modulus */
{
    int i, j;

    for (i = j = 0; *name; name++) {
        i ^= (int) *name << j;
        if (++j > 7)
            j = 0;
    }
    return (((int) (i * 31415)) & (mod - 1));
}


/*
 * is_nw_addr() - is this network address selected?
 */

int
is_nw_addr(inet_addr, port, addr_family)
        unsigned char *inet_addr;        /* Internet address */
        int port;                /* port */
        int addr_family;                /* address family -- e.g., AF_INET,
					 * AF_INET6 */
{
    struct nwad *node;

    if (!(node = NetworkAddrList))
        return (0);
    for (; node; node = node->next) {
        if (node->proto) {
            if (strcasecmp(node->proto, CurrentLocalFile->iproto) != 0)
                continue;
        }
        if (addr_family && node->addr_family && addr_family != node->addr_family)
            continue;

#if    defined(HASIPv6)
        if (addr_family == AF_INET6) {
            static const unsigned char zero_addr[16] = {0};
            if (memcmp(node->addr, zero_addr, 16) != 0) {
                if (memcmp(inet_addr, node->addr, 16) != 0)
                    continue;
            }
        } else if (addr_family == AF_INET)
#endif    /* defined(HASIPv6) */

        {
            static const unsigned char zero_addr[4] = {0};
            if (memcmp(node->addr, zero_addr, 4) != 0) {
                if (memcmp(inet_addr, node->addr, 4) != 0)
                    continue;
            }
        }

#if    defined(HASIPv6)
        else
        continue;
#endif    /* defined(HASIPv6) */

        if (node->sport == -1 || (port >= node->sport && port <= node->eport)) {
            node->found = 1;
            return (1);
        }
    }
    return (0);
}


/*
 * mkstrcpy() - make a string copy in malloc()'d space
 *
 * return: copy pointer
 *	   copy length (optional)
 */

char *
mkstrcpy(src, rlp)
        char *src;            /* source */
        MALLOC_S *rlp;            /* returned length pointer (optional)
					 * The returned length is an strlen()
					 * equivalent */
{
    MALLOC_S len;
    char *ns;

    len = (MALLOC_S)(src ? strlen(src) : 0);
    ns = (char *) malloc(len + 1);
    if (ns) {
        if (src)
            (void) memcpy(ns, src, len + 1);
        else
            *ns = '\0';
    }
    if (rlp)
        *rlp = len;
    return (ns);
}


/*
 * mkstrcat() - make a catenated copy of up to three strings under optional
 *		string-by-string count control
 *
 * return: copy pointer
 *	   copy string length (optional)
 */

char *
mkstrcat(str1, len1, str2, len2, str3, len3, clp)
        char *str1;            /* source string 1 */
        int len1;                /* length of string 1 (-1 if none) */
        char *str2;            /* source string 2 */
        int len2;                /* length of string 2 (-1 if none) */
        char *str3;            /* source string 3 (optional) */
        int len3;            /* length of string 3 (-1 if none) */
        MALLOC_S *clp;            /* pointer to return of copy length
					 * (optional) */
{
    MALLOC_S cat_len, slen1, slen2, slen3;
    char *char_ptr;

    if (str1)
        slen1 = (MALLOC_S)((len1 >= 0) ? len1 : strlen(str1));
    else
        slen1 = (MALLOC_S) 0;
    if (str2)
        slen2 = (MALLOC_S)((len2 >= 0) ? len2 : strlen(str2));
    else
        slen2 = (MALLOC_S) 0;
    if (str3)
        slen3 = (MALLOC_S)((len3 >= 0) ? len3 : strlen(str3));
    else
        slen3 = (MALLOC_S) 0;
    cat_len = slen1 + slen2 + slen3;
    if ((char_ptr = (char *) malloc(cat_len + 1))) {
        char *text_ptr = char_ptr;

        if (str1 && slen1) {
            (void) strncpy(text_ptr, str1, slen1);
            text_ptr += slen1;
        }
        if (str2 && slen2) {
            (void) strncpy(text_ptr, str2, slen2);
            text_ptr += slen2;
        }
        if (str3 && slen3) {
            (void) strncpy(text_ptr, str3, slen3);
            text_ptr += slen3;
        }
        *text_ptr = '\0';
    }
    if (clp)
        *clp = cat_len;
    return (char_ptr);
}


/*
 * is_readable() -- is file readable
 */

int
is_readable(path, msg)
        char *path;            /* file path */
        int msg;            /* issue warning message if 1 */
{
    if (access(path, R_OK) < 0) {
        if (!OptWarnings && msg == 1)
            (void) fprintf(stderr, ACCESSERRFMT, ProgramName, path, strerror(errno));
        return (0);
    }
    return (1);
}


/*
 * lstatsafely() - lstat path safely (i. e., with timeout)
 */

int
lstatsafely(path, buf)
        char *path;            /* file path */
        struct stat *buf;        /* stat buffer address */
{
    if (OptBlockDevice) {
        if (!OptWarnings)
            (void) fprintf(stderr,
                           "%s: avoiding stat(%s): -b was specified.\n",
                           ProgramName, path);
        errno = EWOULDBLOCK;
        return (1);
    }
    return (doinchild(dolstat, path, (char *) buf, sizeof(struct stat)));
}


/*
 * Readlink() - read and interpret file system symbolic links
 */

char *
Readlink(arg)
        char *arg;            /* argument to be interpreted */
{
    char abuf[MAXPATHLEN + 1];
    int alen;
    char *ap;
    char *argp1, *argp2;
    int i, len, llen, slen;
    char lbuf[MAXPATHLEN + 1];
    static char *op = (char *) NULL;
    static int ss = 0;
    char *s1;
    static char **stk = (char **) NULL;
    static int sx = 0;
    char tbuf[MAXPATHLEN + 1];
/*
 * See if avoiding kernel blocks.
 */
    if (OptBlockDevice) {
        if (!OptWarnings) {
            (void) fprintf(stderr, "%s: avoiding readlink(", ProgramName);
            safestrprt(arg, stderr, 0);
            (void) fprintf(stderr, "): -b was specified.\n");
        }
        op = (char *) NULL;
        return (arg);
    }
/*
 * Save the original path.
 */
    if (!op)
        op = arg;
/*
 * Evaluate each component of the argument for a symbolic link.
 */
    for (alen = 0, ap = abuf, argp1 = argp2 = arg; *argp2; argp1 = argp2) {
        for (argp2 = argp1 + 1; *argp2 && *argp2 != '/'; argp2++);
        if ((len = argp2 - arg) >= (int) sizeof(tbuf)) {

            path_too_long:
            if (!OptWarnings) {
                (void) fprintf(stderr,
                               "%s: readlink() path too long: ", ProgramName);
                safestrprt(op ? op : arg, stderr, 1);
            }
            op = (char *) NULL;
            return ((char *) NULL);
        }
        (void) strncpy(tbuf, arg, len);
        tbuf[len] = '\0';
        /*
         * Dereference a symbolic link.
         */
        if ((llen = doinchild(doreadlink, tbuf, lbuf, sizeof(lbuf) - 1)) >= 0) {

            /*
             * If the link is a new absolute path, replace
             * the previous assembly with it.
             */
            if (lbuf[0] == '/') {
                (void) strncpy(abuf, lbuf, llen);
                ap = &abuf[llen];
                *ap = '\0';
                alen = llen;
                continue;
            }
            lbuf[llen] = '\0';
            s1 = lbuf;
        } else {
            llen = argp2 - argp1;
            s1 = argp1;
        }
        /*
         * Make sure two components are separated by a `/'.
         *
         * If the first component is not a link, don't force
         * a leading '/'.
         *
         * If the first component is a link and the source of
         * the link has a leading '/', force a leading '/'.
         */
        if (*s1 == '/')
            slen = 1;
        else {
            if (alen > 0) {

                /*
                 * This is not the first component.
                 */
                if (abuf[alen - 1] == '/')
                    slen = 1;
                else
                    slen = 2;
            } else {

                /*
                 * This is the first component.
                 */
                if (s1 == lbuf && tbuf[0] == '/')
                    slen = 2;
                else
                    slen = 1;
            }
        }
        /*
         * Add to the path assembly.
         */
        if ((alen + llen + slen) >= (int) sizeof(abuf))
            goto path_too_long;
        if (slen == 2)
            *ap++ = '/';
        (void) strncpy(ap, s1, llen);
        ap += llen;
        *ap = '\0';
        alen += (llen + slen - 1);
    }
/*
 * If the assembled path and argument are the same, free all but the
 * last string in the stack, and return the argument.
 */
    if (strcmp(arg, abuf) == 0) {
        for (i = 0; i < sx; i++) {
            if (i < (sx - 1))
                (void) free((FREE_P *) stk[i]);
            stk[i] = (char *) NULL;
        }
        sx = 0;
        op = (char *) NULL;
        return (arg);
    }
/*
 * If the assembled path and argument are different, add it to the
 * string stack, then Readlink() it.
 */
    if (!(s1 = mkstrcpy(abuf, (MALLOC_S *) NULL))) {

        no_readlink_space:

        (void) fprintf(stderr, "%s: no Readlink string space for ", ProgramName);
        safestrprt(abuf, stderr, 1);
        Exit(1);
    }
    if (sx >= MAXSYMLINKS) {

        /*
         * If there are too many symbolic links, report an error, clear
         * the stack, and return no path.
         */
        if (!OptWarnings) {
            (void) fprintf(stderr,
                           "%s: too many (> %d) symbolic links in readlink() path: ",
                           ProgramName, MAXSYMLINKS);
            safestrprt(op ? op : arg, stderr, 1);
        }
        for (i = 0; i < sx; i++) {
            (void) free((FREE_P *) stk[i]);
            stk[i] = (char *) NULL;
        }
        (void) free((FREE_P *) stk);
        stk = (char **) NULL;
        ss = sx = 0;
        op = (char *) NULL;
        return ((char *) NULL);
    }
    if (++sx > ss) {
        if (!stk)
            stk = (char **) malloc((MALLOC_S)(sizeof(char *) * sx));
        else
            stk = (char **) realloc((MALLOC_P *) stk,
                                    (MALLOC_S)(sizeof(char *) * sx));
        if (!stk)
            goto no_readlink_space;
        ss = sx;
    }
    stk[sx - 1] = s1;
    return (Readlink(s1));
}


#if    defined(HASSTREAMS)
/*
 * readstdata() - read stream's stdata structure
 */

int
readstdata(addr, buf)
    KA_T addr;			/* stdata address in kernel*/
    struct stdata *buf;		/* buffer addess */
{
    if (!addr
    ||  kread(addr, (char *)buf, sizeof(struct stdata))) {
        (void) snpf(NameChars, NameCharsLength, "no stream data in %s",
        print_kptr(addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}


/*
 * readsthead() - read stream head
 */

int
readsthead(addr, buf)
    KA_T addr;			/* starting queue pointer in kernel */
    struct queue *buf;		/* buffer for queue head */
{
    KA_T kern_ptr;

    if (!addr) {
        (void) snpf(NameChars, NameCharsLength, "no stream queue head");
        return(1);
    }
    for (kern_ptr = addr; kern_ptr; kern_ptr = (KA_T)buf->q_next) {
        if (kread(kern_ptr, (char *)buf, sizeof(struct queue))) {
        (void) snpf(NameChars, NameCharsLength, "bad stream queue link at %s",
            print_kptr(kern_ptr, (char *)NULL, 0));
        return(1);
        }
    }
    return(0);
}


/*
 * readstidnm() - read stream module ID name
 */

int
readstidnm(addr, buf, len)
    KA_T addr;			/* module ID name address in kernel */
    char *buf;			/* receiving buffer address */
    READLEN_T len;			/* buffer length */
{
    if (!addr || kread(addr, buf, len)) {
        (void) snpf(NameChars, NameCharsLength, "can't read module ID name from %s",
        print_kptr(addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}


/*
 * readstmin() - read stream's module info
 */

int
readstmin(addr, buf)
    KA_T addr;			/* module info address in kernel */
    struct module_info *buf;	/* receiving buffer address */
{
    if (!addr || kread(addr, (char *)buf, sizeof(struct module_info))) {
        (void) snpf(NameChars, NameCharsLength, "can't read module info from %s",
        print_kptr(addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}


/*
 * readstqinit() - read stream's queue information structure
 */

int
readstqinit(addr, buf)
    KA_T addr;			/* queue info address in kernel */
    struct qinit *buf;		/* receiving buffer address */
{
    if (!addr || kread(addr, (char *)buf, sizeof(struct qinit))) {
        (void) snpf(NameChars, NameCharsLength, "can't read queue info from %s",
        print_kptr(addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* HASSTREAMS */


/*
 * safepup() - safely print an unprintable character -- i.e., print it in a
 *	       printable form
 *
 * return: char * to printable equivalent
 *	   cl = strlen(printable equivalent)
 */

static char *
safepup(ch, char_len)
        unsigned int ch;            /* unprintable (i.e., !isprint())
					 * character */
        int *char_len;            /* returned printable strlen -- NULL if
					 * no return needed */
{
    int len;
    char *rp;
    static char up[8];

    if (ch < 0x20) {
        switch (ch) {
            case '\b':
                rp = "\\b";
                break;
            case '\f':
                rp = "\\f";
                break;
            case '\n':
                rp = "\\n";
                break;
            case '\r':
                rp = "\\r";
                break;
            case '\t':
                rp = "\\t";
                break;
            default:
                up[0] = '^';
                up[1] = (char)(ch + 0x40);
                up[2] = '\0';
                rp = up;
        }
        len = 2;
    } else if (ch == 0xff) {
        rp = "^?";
        len = 2;
    } else {
        {
            static const char hex[] = "0123456789abcdef";
            unsigned int v = ch & 0xff;
            up[0] = '\\';
            up[1] = 'x';
            up[2] = hex[(v >> 4) & 0xf];
            up[3] = hex[v & 0xf];
            up[4] = '\0';
        }
        rp = up;
        len = 4;
    }
    if (char_len)
        *char_len = len;
    return (rp);
}


/*
 * safestrlen() - calculate a "safe" string length -- i.e., compute space for
 *		  non-printable characters when printed in a printable form
 */

int
safestrlen(str, flags)
        char *str;            /* string pointer */
        int flags;            /* flags:
					 *   bit 0: 0 (0) = no NL
					 *	    1 (1) = add trailing NL
					 *	 1: 0 (0) = ' ' printable
					 *	    1 (2) = ' ' not printable
					 */
{
    char unprintable_ch;
    int len = 0;

    unprintable_ch = (flags & 2) ? ' ' : '\0';
    if (str) {
    /*
     * Use a lookup table for character expansion length to avoid
     * per-character isprint() calls:
     *   1 = printable (passthrough), 2 = control/0xff (^. form),
     *   4 = other non-printable (\x%02x form).
     */
        static const unsigned char explen[256] = {
            /* 0x00-0x1f: control chars -> 2 */
            2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
            2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
            /* 0x20-0x7e: printable -> 1 */
            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,
            /* 0x7f: DEL -> 4 */
            4,
            /* 0x80-0xfe: high bytes -> 4 */
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,
            /* 0xff -> 2 */
            2,
        };
        for (; *str; str++) {
            unsigned char ch = (unsigned char)*str;
            if (ch == (unsigned char)unprintable_ch)
                len += (ch < 0x20 || ch == 0xff) ? 2 : 4;
            else
                len += explen[ch];
        }
    }
    return (len);
}


/*
 * safestrprt() - print a string "safely" to the indicated stream -- i.e.,
 *		  print unprintable characters in a printable form
 */

void
safestrprt(str, stream, flags)
        char *str;            /* string to print pointer pointer */
        FILE *stream;            /* destination stream -- e.g., stderr
					 * or stdout */
        int flags;            /* flags:
					 *   bit 0: 0 (0) = no NL
					 *	    1 (1) = add trailing NL
					 *	 1: 0 (0) = ' ' printable
					 *	    1 (2) = ' ' not printable
					 *	 2: 0 (0) = print string as is
					 *	    1 (4) = surround string
					 *		    with '"'
					 *	 4: 0 (0) = print ending '\n'
					 *	    1 (8) = don't print ending
					 *		    '\n'
					 */
{
    char ch;
    int new_line_count, total_line_count, sl;

#if    defined(HASWIDECHAR)
    wchar_t wchar;
    int wcmx = MB_CUR_MAX;
#else	/* !defined(HASWIDECHAR) */
    static int wcmx = 1;
#endif    /* defined(HASWIDECHAR) */

    ch = (flags & 2) ? ' ' : '\0';
    if (flags & 4)
        putc('"', stream);
    if (str) {
    /*
     * Use a lookup table to classify printable characters, avoiding
     * per-character isprint() calls.  1 = printable (0x20-0x7e).
     */
        static const unsigned char printable[256] = {
            /* 0x00-0x1f */ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
            /* 0x20-0x7e */ 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,
            /* 0x7f-0xff */ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,
        };

        for (sl = strlen(str); *str; sl -= new_line_count, str += new_line_count) {

#if    defined(HASWIDECHAR)
            if (wcmx > 1) {
                new_line_count = mblen(str, sl);
                if (new_line_count > 1) {
                if ((mbtowc(&wchar, str, sl) == new_line_count) && iswprint(wchar)) {
                    for (total_line_count = 0; total_line_count < new_line_count; total_line_count++) {
                    putc((int)*(str + total_line_count), stream);
                    }
                } else {
                    for (total_line_count = 0; total_line_count < new_line_count; total_line_count++) {
                        fputs(safepup((unsigned int)*(str + total_line_count),
                              (int *)NULL), stream);
                    }
                }
                continue;
                } else
                new_line_count = 1;
            } else
                new_line_count = 1;
#else	/* !defined(HASWIDECHAR) */
            new_line_count = 1;
#endif    /* defined(HASWIDECHAR) */

            if (printable[(unsigned char)*str] && *str != ch)
                putc((int) (*str & 0xff), stream);
            else {
                if ((flags & 8) && (*str == '\n') && !*(str + 1))
                    break;
                fputs(safepup((unsigned int) *str, (int *) NULL), stream);
            }
        }
    }
    if (flags & 4)
        putc('"', stream);
    if (flags & 1)
        putc('\n', stream);
}


/*
 * safestrprtn() - print a specified number of characters from a string
 *		   "safely" to the indicated stream
 */

void
safestrprtn(str, len, stream, flags)
        char *str;            /* string to print pointer pointer */
        int len;            /* safe number of characters to
					 * print */
        FILE *stream;            /* destination stream -- e.g., stderr
					 * or stdout */
        int flags;            /* flags:
					 *   bit 0: 0 (0) = no NL
					 *	    1 (1) = add trailing NL
					 *	 1: 0 (0) = ' ' printable
					 *	    1 (2) = ' ' not printable
					 *	 2: 0 (0) = print string as is
					 *	    1 (4) = surround string
					 *		    with '"'
					 *	 4: 0 (0) = print ending '\n'
					 *	    1 (8) = don't print ending
					 *		    '\n'
					 */
{
    char ch, *uchar_ptr;
    int char_len, i;

    if (flags & 4)
        putc('"', stream);
    if (str) {
    /*
     * Use a lookup table for printable classification (matches safestrprt).
     */
        static const unsigned char printable[256] = {
            /* 0x00-0x1f */ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
            /* 0x20-0x7e */ 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
                            1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,
            /* 0x7f-0xff */ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,
        };
        ch = (flags & 2) ? ' ' : '\0';
        for (i = 0; i < len && *str; str++) {
            if (printable[(unsigned char)*str] && *str != ch) {
                putc((int) (*str & 0xff), stream);
                i++;
            } else {
                if ((flags & 8) && (*str == '\n') && !*(str + 1))
                    break;
                uchar_ptr = safepup((unsigned int) *str, &char_len);
                if ((i + char_len) > len)
                    break;
                fputs(uchar_ptr, stream);
                i += char_len;
            }
        }
    } else
        i = 0;
    for (; i < len; i++)
        putc(' ', stream);
    if (flags & 4)
        putc('"', stream);
    if (flags & 1)
        putc('\n', stream);
}


/*
 * statsafely() - stat path safely (i. e., with timeout)
 */

int
statsafely(path, buf)
        char *path;            /* file path */
        struct stat *buf;        /* stat buffer address */
{
    if (OptBlockDevice) {
        if (!OptWarnings)
            (void) fprintf(stderr,
                           "%s: avoiding stat(%s): -b was specified.\n",
                           ProgramName, path);
        errno = EWOULDBLOCK;
        return (1);
    }
    return (doinchild(dostat, path, (char *) buf, sizeof(struct stat)));
}


/*
 * stkdir() - stack directory name
 */

void
stkdir(path)
        char *path;        /* directory path */
{
    MALLOC_S len;
/*
 * Provide adequate space for directory stack pointers.
 */
    if (DirStackIndex >= DirStackAlloc) {
        DirStackAlloc += 128;
        len = (MALLOC_S)(DirStackAlloc * sizeof(char *));
        if (!DirStack)
            DirStack = (char **) malloc(len);
        else {
            char **tmp = (char **) realloc((MALLOC_P *) DirStack, len);
            if (!tmp) {
                (void) free((FREE_P *) DirStack);
                DirStack = (char **) NULL;
            } else
                DirStack = tmp;
        }
        if (!DirStack) {
            (void) fprintf(stderr,
                           "%s: no space for directory stack at: ", ProgramName);
            safestrprt(path, stderr, 1);
            Exit(1);
        }
    }
/*
 * Allocate space for the name, copy it there and put its pointer on the stack.
 */
    if (!(DirStack[DirStackIndex] = mkstrcpy(path, (MALLOC_S *) NULL))) {
        (void) fprintf(stderr, "%s: no space for: ", ProgramName);
        safestrprt(path, stderr, 1);
        Exit(1);
    }
    DirStackIndex++;
}


/*
 * x2dev() - convert hexadecimal ASCII string to device number
 */

char *
x2dev(hex_str, dev_ptr)
        char *hex_str;            /* ASCII string */
        dev_t *dev_ptr;            /* device receptacle */
{
    char *cp, *cp1;
    int num;
    dev_t result;

/*
 * Skip an optional leading 0x.  Count the number of hex digits up to the end
 * of the string, or to a space, or to a comma.  Return an error if an unknown
 * character is encountered.  If the count is larger than (2 * sizeof(dev_t))
 * -- e.g., because of sign extension -- ignore excess leading hex 0xf digits,
 * but return an error if an excess leading digit isn't 0xf.
 */
    if (strncasecmp(hex_str, "0x", 2) == 0)
        hex_str += 2;
    {
    /*
     * Classify characters via lookup table to avoid per-character branching.
     * 1 = hex digit, 2 = stop character (space/comma), 0 = invalid.
     */
        static const unsigned char cls[256] = {
            ['0']=1,['1']=1,['2']=1,['3']=1,['4']=1,['5']=1,['6']=1,['7']=1,
            ['8']=1,['9']=1,['a']=1,['b']=1,['c']=1,['d']=1,['e']=1,['f']=1,
            ['A']=1,['B']=1,['C']=1,['D']=1,['E']=1,['F']=1,[' ']=2,[',']=2,
        };
        for (cp = hex_str, num = 0; *cp; cp++, num++) {
            unsigned char c = cls[(unsigned char)*cp];
            if (c == 1) continue;
            if (c == 2) break;
            return ((char *) NULL);
        }
    }
    if (!num)
        return ((char *) NULL);
    if (num > (2 * (int) sizeof(dev_t))) {
        cp1 = hex_str;
        hex_str += (num - (2 * sizeof(dev_t)));
        while (cp1 < hex_str) {
            if (*cp1 != 'f' && *cp1 != 'F')
                return ((char *) NULL);
            cp1++;
        }
    }
/*
 * Assemble the validated hex digits of the device number, starting at a point
 * in the string relevant to sizeof(dev_t).
 *
 * Use a lookup table to avoid per-character branching on isdigit/isupper.
 */
    {
        static const signed char hv[256] = {
            ['0']=0, ['1']=1, ['2']=2, ['3']=3, ['4']=4,
            ['5']=5, ['6']=6, ['7']=7, ['8']=8, ['9']=9,
            ['a']=10,['b']=11,['c']=12,['d']=13,['e']=14,['f']=15,
            ['A']=10,['B']=11,['C']=12,['D']=13,['E']=14,['F']=15,
        };
        for (result = 0; hex_str < cp; hex_str++) {
            result = (result << 4) | (hv[(unsigned char)*hex_str] & 0xf);
        }
    }
    *dev_ptr = result;
    return (hex_str);
}
