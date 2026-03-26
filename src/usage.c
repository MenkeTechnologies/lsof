/*
 * usage.c - usage functions for lsof
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
        "@(#) Copyright 1998 lsof contributors.\nAll rights reserved.\n";
#endif


#include "lsof.h"
#include "version.h"

/* ANSI color codes for cyberpunk-themed output */
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_MAGENTA "\033[1;35m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"
#define ANSI_RESET   "\033[0m"


/*
 * Local function prototypes
 */

_PROTOTYPE(static char *isnullstr, (char *s));

_PROTOTYPE(static int print_in_col, (int col, char *cp));

_PROTOTYPE(static void report_HASDCACHE, (int type, char *ttl, char *det));

_PROTOTYPE(static void report_HASKERNIDCK, (char *pfx, char *verb));

_PROTOTYPE(static void report_SECURITY, (char *pfx, char *punct));

_PROTOTYPE(static void report_WARNDEVACCESS, (char *pfx, char *verb,
        char *punct));


/*
 * isnullstr() - is it a null string?
 */

static char *
isnullstr(s)
        char *s;            /* string pointer */
{
    if (!s)
        return ((char *) NULL);
    while (*s) {
        if (*s != ' ')
            return (s);
        s++;
    }
    return ((char *) NULL);
}


/*
 * print_in_col() -- print character string in help column
 */

static int
print_in_col(col, cp)
        int col;                /* column number */
        char *cp;                /* what to print */
{
    if (cp && *cp) {
        switch (col) {
            case 1:
                (void) fprintf(stderr, "  %-23.23s", cp);
                break;
            case 2:
                (void) fprintf(stderr, "  %-25.25s", cp);
                break;
            default:
                (void) fprintf(stderr, "  %s\n", cp);
                col = 0;
        }
        col++;
    }
    return (col);
}


/*
 * report_HASDCACHE() -- report device cache file state
 */

static void
report_HASDCACHE(type, ttl, det)
        int type;                /* type: 0 == read path report
						 *       1 == full report */
        char *ttl;                /* title lines prefix
						 * (NULL if none) */
        char *det;                /* detail lines prefix
						 * (NULL if none) */
{

#if    defined(HASDCACHE)
    char *cp;
    int dx;

# if	defined(WILLDROPGID)
    int saved_Setgid = SetgidState;

    SetgidState = 0;
# endif	/* defined(WILLDROPGID) */

    if (type) {

    /*
     * Report full device cache information.
     */
        (void) fprintf(stderr, "%sDevice cache file read-only paths:\n",
        ttl ? ttl : "");
        if ((dx = dcpath(1, 0)) < 0)
        (void) fprintf(stderr, "%snone\n", det ? det : "");
        else {
        (void) fprintf(stderr, "%sNamed via -D: %s\n",
            det ? det : "",
            DevCachePath[0] ? DevCachePath[0] : "none");

# if	defined(HASENVDC)
        (void) fprintf(stderr,
            "%sNamed in environment variable %s: %s\n",
            det ? det : "",
            HASENVDC, DevCachePath[1] ? DevCachePath[1] : "none");
# endif	/* defined(HASENVDC) */

# if	defined(HASSYSDC)
        if (DevCachePath[2])
            (void) fprintf(stderr,
            "%sSystem-wide device cache: %s\n",
            det ? det : "",
            DevCachePath[2]);
# endif	/* defined(HASSYSDC) */

# if	defined(HASPERSDC)
        (void) fprintf(stderr,
            "%sPersonal path format (HASPERSDC): \"%s\"\n",
            det ? det : "",
            HASPERSDC);
#  if	defined(HASPERSDCPATH)
        (void) fprintf(stderr,
            "%sModified personal path environment variable: %s\n",
            det ? det : "",
            HASPERSDCPATH);
        cp = getenv(HASPERSDCPATH);
        (void) fprintf(stderr, "%s%s value: %s\n",
            det ? det : "",
            HASPERSDCPATH, cp ? cp : "none");
#  endif	/* defined(HASPERSDCPATH) */
        (void) fprintf(stderr, "%sPersonal path: %s\n",
            det ? det : "",
            DevCachePath[3] ? DevCachePath[3] : "none");
# endif	/* defined(HASPERSDC) */
        }
        (void) fprintf(stderr, "%sDevice cache file write paths:\n",
        ttl ? ttl : "");
        if ((dx = dcpath(2, 0)) < 0)
        (void) fprintf(stderr, "%snone\n", det ? det : "");
        else {
        (void) fprintf(stderr, "%sNamed via -D: %s\n",
            det ? det : "",
            DevCacheState == 2 ? "none"
                 : DevCachePath[0] ? DevCachePath[0] : "none");

# if	defined(HASENVDC)
        (void) fprintf(stderr,
            "%sNamed in environment variable %s: %s\n",
            det ? det : "",
            HASENVDC, DevCachePath[1] ? DevCachePath[1] : "none");
# endif	/* defined(HASENVDC) */

# if	defined(HASPERSDC)
        (void) fprintf(stderr,
            "%sPersonal path format (HASPERSDC): \"%s\"\n",
            det ? det : "",
            HASPERSDC);
#  if	defined(HASPERSDCPATH)
        (void) fprintf(stderr,
            "%sModified personal path environment variable: %s\n",
            det ? det : "",
            HASPERSDCPATH);
        cp = getenv(HASPERSDCPATH);
        (void) fprintf(stderr, "%s%s value: %s\n",
            det ? det : "",
            HASPERSDCPATH, cp ? cp : "none");
#  endif	/* defined(HASPERSDCPATH) */
         (void) fprintf(stderr, "%sPersonal path: %s\n",
            det ? det : "",
            DevCachePath[3] ? DevCachePath[3] : "none");
# endif	/* defined(HASPERSDC) */
        }
    } else {

    /*
     * Report device cache read file path.
     */

# if	defined(HASENVDC) || defined(HASPERSDC) || defined(HASSYSDC)
        cp = NULL;
#  if	defined(HASENVDC)
        if ((dx = dcpath(1, 0)) >= 0)
        cp = DevCachePath[1];
#  endif	/* defined(HASENVDC) */
#  if	defined(HASSYSDC)
        if (!cp)
        cp = HASSYSDC;
#  endif	/* defined(HASSYSDC) */
#  if	defined(HASPERSDC)
        if (!cp && dx != -1 && (dx = dcpath(1, 0)) >= 0)
        cp = DevCachePath[3];
#  endif	/* defined(HASPERSDC) */
        if (cp)
        (void) fprintf(stderr,
            "%s%s is the default device cache file read path.\n",
            ttl ? ttl : "",
            cp
        );
# endif    /* defined(HASENVDC) || defined(HASPERSDC) || defined(HASSYSDC) */
    }

# if	defined(WILLDROPGID)
    SetgidState = saved_Setgid;
# endif	/* defined(WILLDROPGID) */

#endif    /* defined(HASDCACHE) */

}


/*
 * report_HASKERNIDCK() -- report HASKERNIDCK state
 */

static void
report_HASKERNIDCK(pfx, verb)
        char *pfx;                /* prefix (NULL if none) */
        char *verb;                /* verb (NULL if none) */
{
    (void) fprintf(stderr, "%sernel ID check %s%s%s.\n",
                   pfx ? pfx : "",
                   verb ? verb : "",
                   verb ? " " : "",

#if    defined(HASKERNIDCK)
            "enabled"
#else	/* !defined(HASKERNIDCK) */
                   "disabled"
#endif    /* defined(HASKERNIDCK) */

    );
}


/*
 * report_SECURITY() -- report *SECURITY states
 */

static void
report_SECURITY(pfx, punct)
        char *pfx;                /* prefix (NULL if none) */
        char *punct;                /* short foem punctuation
						 * (NULL if none) */
{
    fprintf(stderr, "%s%s can list all files%s",
            pfx ? pfx : "",

#if    defined(HASSECURITY)
    "Only root",
# if	defined(HASNOSOCKSECURITY)
    ", but anyone can list socket files.\n"
# else	/* !defined(HASNOSOCKSECURITY) */
    punct ? punct : ""
# endif	/* defined(HASNOSOCKSECURITY) */
#else	/* !defined(HASSECURITY) */
            "Anyone",
            punct ? punct : ""
#endif    /* defined(HASSECURITY) */

    );
}


/*
 * report_WARNDEVACCESS() -- report WEARNDEVACCESS state
 */

static void
report_WARNDEVACCESS(pfx, verb, punct)
        char *pfx;                /* prefix (NULL if none) */
        char *verb;                /* verb (NULL if none) */
        char *punct;                /* punctuation */
{
    (void) fprintf(stderr, "%s/dev warnings %s%s%s%s",
                   pfx ? pfx : "",
                   verb ? verb : "",
                   verb ? " " : "",

#if    defined(WARNDEVACCESS)
            "enabled",
#else	/* !defined(WARNDEVACCESS) */
                   "disabled",
#endif    /* defined(WARNDEVACCESS) */

                   punct);
}


/*
 * usage() - display usage and exit
 */

void
usage(xv, fh, version)
        int xv;                /* exit value */
        int fh;                /* ``-F ?'' status */
        int version;            /* ``-v'' status */
{
    char buf[MAXPATHLEN + 1], *cp, *cp1, *cp2;
    int i;

    if (OptHelp || xv) {
        (void) fprintf(stderr,
            "\n"
            "\033[36m  ██▓     ██████  ▒█████    █████▒\033[0m\n"
            "\033[36m ▓██▒   ▒██    ▒ ▒██▒  ██▒▓██   ▒ \033[0m\n"
            "\033[35m ▒██░   ░ ▓██▄   ▒██░  ██▒▒████ ░ \033[0m\n"
            "\033[35m ░██░     ▒   ██▒▒██   ██░░▓█▒  ░ \033[0m\n"
            "\033[31m ░██░   ▒██████▒░░ ████▓▒░░▒█░    \033[0m\n"
            "\033[31m ░▓     ▒ ▒▓▒ ▒ ░░ ▒░▒░▒░  ▒ ░   \033[0m\n"
            "\033[33m  ▒ ░   ░ ░▒  ░ ░  ░ ▒ ▒░  ░     \033[0m\n"
            "\033[33m  ▒ ░   ░  ░  ░  ░ ░ ░ ▒   ░ ░   \033[0m\n"
            "\033[33m  ░           ░      ░ ░          \033[0m\n"
            "\n"
            ANSI_CYAN "  >> FILE DESCRIPTOR SCANNER v" LSOF_VERSION " << " ANSI_RESET "\n"
            ANSI_MAGENTA "  [ mapping the topology of open files ]" ANSI_RESET "\n"
            "\n"
            ANSI_YELLOW "  USAGE:" ANSI_RESET " %s [OPTION]... [FILE]...\n",
            ProgramName);
    }
    if (xv && !OptHelp) {
        (void) fprintf(stderr,
            "\n  " ANSI_YELLOW ">>" ANSI_RESET " Try '" ANSI_CYAN "%s -h" ANSI_RESET "' for more information.\n", ProgramName);
        if (!fh)
            Exit(xv);
    }
    if (OptHelp) {
        (void) fprintf(stderr, "\n"
            ANSI_CYAN "  ── SELECTION ──────────────────────────────────────" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -?, -h            " ANSI_RESET "display this transmission\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -a                " ANSI_RESET "AND selections " ANSI_MAGENTA "(default: OR)" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -c COMMAND        " ANSI_RESET "select by command name " ANSI_MAGENTA "(prefix, ^exclude, /regex/)" ANSI_RESET "\n");
        (void) snpf(buf, sizeof(buf),
            ANSI_GREEN "   +c WIDTH          " ANSI_RESET "set COMMAND column width " ANSI_MAGENTA "(default: %d)" ANSI_RESET "\n", CMDL);
        (void) fprintf(stderr, "%s", buf);
        (void) fprintf(stderr,
            ANSI_GREEN "   -d FD             " ANSI_RESET "select by file descriptor set\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -g [PGID]         " ANSI_RESET "exclude(^) or select process group IDs\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -p PID            " ANSI_RESET "select by PID " ANSI_MAGENTA "(comma-separated, ^excludes)" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -u USER           " ANSI_RESET "select by login name or UID " ANSI_MAGENTA "(comma-separated, ^excludes)" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -x [fl]           " ANSI_RESET "cross over +d|+D file systems (f) or symbolic links (l)\n");

        (void) fprintf(stderr, "\n"
            ANSI_CYAN "  ── NETWORK ───────────────────────────────────────" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -i [ADDR]         " ANSI_RESET "select IPv%s files\n",
#if    defined(HASIPv6)
            "4/6"
#else	/* !defined(HASIPv6) */
            "4"
#endif    /* defined(HASIPv6) */
        );
        (void) fprintf(stderr,
            "                     " ANSI_MAGENTA "[%s][proto][@host|addr][:svc|port]" ANSI_RESET "\n",
#if    defined(HASIPv6)
            "46"
#else	/* !defined(HASIPv6) */
            "4"
#endif    /* defined(HASIPv6) */
        );
        (void) fprintf(stderr,
            ANSI_GREEN "   -n                " ANSI_RESET "inhibit host name resolution\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -N                " ANSI_RESET "select NFS files\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -P                " ANSI_RESET "inhibit port number to name conversion\n");

#if    defined(HASTCPUDPSTATE)
        (void) fprintf(stderr,
            ANSI_GREEN "   -s [PROTO:STATE]  " ANSI_RESET "exclude(^) or select protocol states by name\n");
#else	/* !defined(HASTCPUDPSTATE) */
        (void) fprintf(stderr,
            ANSI_GREEN "   -s [PROTO:STATE]  " ANSI_RESET "display file size\n");
#endif    /* defined(HASTCPUDPSTATE) */

        (void) fprintf(stderr,
            ANSI_GREEN "   -T [LEVEL]        " ANSI_RESET "disable or select TCP/TPI info\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -U                " ANSI_RESET "select UNIX domain socket files\n");

#if    !defined(HASNORPC_H)
        (void) fprintf(stderr,
            ANSI_GREEN "   +|-M              " ANSI_RESET "enable (+) or disable (-) portmap registration");
        (void) fprintf(stderr, " " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n",
# if    defined(HASPMAPENABLED)
            "+"
# else	/* !defined(HASPMAPENABLED) */
            "-"
# endif    /* defined(HASPMAPENABLED) */
        );
#endif    /* !defined(HASNORPC_H) */

        (void) fprintf(stderr, "\n"
            ANSI_CYAN "  ── FILES & DIRECTORIES ───────────────────────────" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   +d DIR            " ANSI_RESET "list open files in DIR\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   +D DIR            " ANSI_RESET "recursively list open files in DIR " ANSI_MAGENTA "(slow)" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   +|-f              " ANSI_RESET "+filesystem or -file names\n");

#if    defined(HASFSTRUCT)
        (void) fprintf(stderr,
            ANSI_GREEN "   +|-f [cfgGn]      " ANSI_RESET "file struct info: count, addr, flags, node\n");
#endif    /* defined(HASFSTRUCT) */

        (void) fprintf(stderr,
            ANSI_GREEN "   +|-L [COUNT]      " ANSI_RESET "list (+) or suppress (-) link counts < COUNT\n");

#if    defined(HASDCACHE)
        if (SetuidRootState)
            cp = "?|i|r";
# if	!defined(WILLDROPGID)
        else if (MyRealUid)
            cp = "?|i|r<path>";
# endif	/* !defined(WILLDROPGID) */
        else
            cp = "?|i|b|r|u[path]";
        (void) fprintf(stderr,
            ANSI_GREEN "   -D ACTION         " ANSI_RESET "device cache control " ANSI_MAGENTA "(%s)" ANSI_RESET "\n", cp);
#endif    /* defined(HASDCACHE) */

#if    defined(HASEOPT)
        (void) fprintf(stderr,
            ANSI_GREEN "   +|-e PATH         " ANSI_RESET "exempt filesystem " ANSI_MAGENTA "(risky)" ANSI_RESET "\n");
#endif    /* defined(HASEOPT) */

        (void) fprintf(stderr, "\n"
            ANSI_CYAN "  ── DISPLAY ───────────────────────────────────────" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -F [FIELDS]       " ANSI_RESET "select output fields; -F ? for help\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -l                " ANSI_RESET "display UID numbers instead of login names\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -o [DIGITS]       " ANSI_RESET "display file offset " ANSI_MAGENTA "(0t format, default: %d digits)" ANSI_RESET "\n",
            OFFDECDIG);

#if    defined(HASPPID)
        (void) fprintf(stderr,
            ANSI_GREEN "   -R                " ANSI_RESET "list parent PID\n");
#endif    /* defined(HASPPID) */

        (void) fprintf(stderr,
            ANSI_GREEN "   -t                " ANSI_RESET "terse output " ANSI_MAGENTA "(PID only)" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -v                " ANSI_RESET "display version information\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -V                " ANSI_RESET "verbose search\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   +|-w              " ANSI_RESET "enable (+) or suppress (-) warnings");
        (void) fprintf(stderr, " " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n",
#if    defined(WARNINGSTATE)
            "-"
#else	/* !defined(WARNINGSTATE) */
            "+"
#endif    /* defined(WARNINGSTATE) */
        );

        (void) fprintf(stderr, "\n"
            ANSI_CYAN "  ── SYSTEM ────────────────────────────────────────" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -b                " ANSI_RESET "avoid kernel blocks\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -O                " ANSI_RESET "avoid overhead of extra processing " ANSI_MAGENTA "(risky)" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   -S [SECONDS]      " ANSI_RESET "stat(2) timeout " ANSI_MAGENTA "(default: %d)" ANSI_RESET "\n",
            TMLIMIT);
        (void) fprintf(stderr,
            ANSI_GREEN "   +|-r [SECONDS]    " ANSI_RESET "repeat mode: +until no files, -forever");
        (void) fprintf(stderr, " " ANSI_MAGENTA "(default: %d)" ANSI_RESET "\n", RPTTM);

#if    defined(HAS_STRFTIME)
        (void) fprintf(stderr,
            "                     " ANSI_MAGENTA "optional suffix: m<fmt> for strftime(3) markers" ANSI_RESET "\n");
#endif    /* defined(HAS_STRFTIME) */

#if    defined(HASNCACHE)
        (void) fprintf(stderr,
            ANSI_GREEN "   -C                " ANSI_RESET "disable kernel name cache\n");
#endif    /* defined(HASNCACHE) */

#if    defined(HASKOPT)
        (void) fprintf(stderr,
            ANSI_GREEN "   -k FILE           " ANSI_RESET "kernel symbols file");
        (void) fprintf(stderr, " " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n",
            NamelistFilePath ? NamelistFilePath
# if	defined(N_UNIX)
                  : N_UNIX
# else	/* !defined(N_UNIX) */
                  : (NamelistFilePath = get_nlist_path(1)) ? NamelistFilePath : "none found"
# endif	/* defined(N_UNIX) */
        );
#endif    /* defined(HASKOPT) */

#if    defined(HASTASKS)
        (void) fprintf(stderr,
            ANSI_GREEN "   -K                " ANSI_RESET "list tasks (threads)\n");
#endif    /* defined(HASTASKS) */

#if    defined(HASMOPT)
        (void) fprintf(stderr,
            ANSI_GREEN "   -m FILE           " ANSI_RESET "kernel memory file " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n", KMEM);
#endif    /* defined(HASMOPT) */

#if    defined(HASMNTSUP)
        (void) fprintf(stderr,
            ANSI_GREEN "   +m [FILE]         " ANSI_RESET "use or create mount supplement\n");
#endif    /* defined(HASMNTSUP) */

        (void) fprintf(stderr,
            ANSI_GREEN "   --                " ANSI_RESET "end option processing\n");

#if    defined(HASXOPT)
# if	defined(HASXOPT_ROOT)
        if (MyRealUid == 0)
# endif	/* defined(HASXOPT_ROOT) */
        (void) fprintf(stderr,
            ANSI_GREEN "   -X                " ANSI_RESET "%s\n", HASXOPT);
#endif    /* defined(HASXOPT) */

#if    defined(HASZONES)
        (void) fprintf(stderr,
            ANSI_GREEN "   -z [ZONE]         " ANSI_RESET "select by zone name\n");
#endif    /* defined(HASZONES) */

#if    defined(HASSELINUX)
        if (ContextStatus)
        (void) fprintf(stderr,
            ANSI_GREEN "   -Z [CONTEXT]      " ANSI_RESET "select by security context\n");
#endif    /* defined(HASSELINUX) */

#if    defined(HAS_AFS) && defined(HASAOPT)
        (void) fprintf(stderr,
            ANSI_GREEN "   -A FILE           " ANSI_RESET "AFS name list file " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n",
            AFSAPATHDEF);
#endif    /* defined(HAS_AFS) && defined(HASAOPT) */

        (void) fprintf(stderr, "\n"
            ANSI_CYAN "  ── EXAMPLES ──────────────────────────────────────" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_GREEN "   %s -i :8080       " ANSI_RESET "list files using port 8080\n", ProgramName);
        (void) fprintf(stderr,
            ANSI_GREEN "   %s -p 1234        " ANSI_RESET "list files opened by PID 1234\n", ProgramName);
        (void) fprintf(stderr,
            ANSI_GREEN "   %s -u root        " ANSI_RESET "list files opened by root\n", ProgramName);
        (void) fprintf(stderr,
            ANSI_GREEN "   %s /var/log/syslog" ANSI_RESET "  list processes using this file\n", ProgramName);
        (void) fprintf(stderr,
            ANSI_GREEN "   %s -i TCP         " ANSI_RESET "list all TCP connections\n", ProgramName);

        (void) fprintf(stderr, "\n"
            ANSI_CYAN "  ── INFO ──────────────────────────────────────────" ANSI_RESET "\n");
        (void) fprintf(stderr,
            ANSI_MAGENTA "  v" LSOF_VERSION " " ANSI_RESET "// " ANSI_YELLOW "(c) lsof contributors" ANSI_RESET "\n");
        (void) report_SECURITY("", "; ");
        (void) report_WARNDEVACCESS("", NULL, ";");
        (void) report_HASKERNIDCK(" k", NULL);
        (void) report_HASDCACHE(0, NULL, NULL);
        (void) fprintf(stderr,
            ANSI_MAGENTA "  Every open file tells a story." ANSI_RESET "\n");

#if    defined(DIALECT_WARNING)
        (void) fprintf(stderr, ANSI_YELLOW "WARNING: %s" ANSI_RESET "\n", DIALECT_WARNING);
#endif    /* defined(DIALECT_WARNING) */

    }
    if (fh) {
        (void) fprintf(stderr, "\n%s: output field IDs:\n", ProgramName);
        (void) fprintf(stderr, "  ID    Description\n");
        for (i = 0; FieldSelection[i].nm; i++) {

#if    !defined(HASPPID)
            if (FieldSelection[i].id == LSOF_FID_PPID)
                continue;
#endif    /* !defined(HASPPID) */

#if    !defined(HASFSTRUCT)
            if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_ADDR
                || FieldSelection[i].id == LSOF_FID_FILE_STRUCT_COUNT
                || FieldSelection[i].id == LSOF_FID_FILE_FLAGS
                || FieldSelection[i].id == LSOF_FID_NODE_ID)
                continue;
#else	/* defined(HASFSTRUCT) */
# if	defined(HASNOFSADDR)
            if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_ADDR)
                continue;
# endif	/* defined(HASNOFSADDR) */

# if	defined(HASNOFSCOUNT)
            if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_COUNT)
                continue;
# endif	/* !defined(HASNOFSCOUNT) */

# if	defined(HASNOFSFLAGS)
            if (FieldSelection[i].id == LSOF_FID_FILE_FLAGS)
                continue;
# endif	/* defined(HASNOFSFLAGS) */

# if	defined(HASNOFSNADDR)
            if (FieldSelection[i].id == LSOF_FID_NODE_ID)
                continue;
# endif	/* defined(HASNOFSNADDR) */
#endif    /* !defined(HASFSTRUCT) */

#if    !defined(HASZONES)
            if (FieldSelection[i].id == LSOF_FID_ZONE)
                continue;
#endif    /* !defined(HASZONES) */

#if    defined(HASSELINUX)
            if ((FieldSelection[i].id == LSOF_FID_SEC_CONTEXT) && !ContextStatus)
                continue;
#else	/* !defined(HASSELINUX) */
            if (FieldSelection[i].id == LSOF_FID_SEC_CONTEXT)
                continue;
#endif    /* !defined(HASSELINUX) */

            (void) fprintf(stderr, "   %c    %s\n",
                           FieldSelection[i].id, FieldSelection[i].nm);
        }
    }

#if    defined(HASDCACHE)
    if (DevCacheHelp)
        report_HASDCACHE(1, NULL, "    ");
#endif    /* defined(HASDCACHE) */

    if (version) {
        (void) fprintf(stderr, "\n%s %s\n", ProgramName, LSOF_VERSION);

#if    defined(LSOF_CINFO)
        if ((cp = isnullstr(LSOF_CINFO)))
            (void) fprintf(stderr, "Configuration: %s\n", cp);
#endif    /* defined(LSOF_CINFO) */

        if ((cp = isnullstr(LSOF_CCDATE)))
            (void) fprintf(stderr, "Built: %s\n", cp);
        cp = isnullstr(LSOF_HOST);
        if (!(cp1 = isnullstr(LSOF_LOGNAME)))
            cp1 = isnullstr(LSOF_USER);
        if (cp || cp1) {
            if (cp && cp1)
                cp2 = "by and on";
            else if (cp)
                cp2 = "on";
            else
                cp2 = "by";
            (void) fprintf(stderr, "Built %s: %s%s%s\n",
                           cp2,
                           cp1 ? cp1 : "",
                           (cp && cp1) ? "@" : "",
                           cp ? cp : ""
            );
        }

#if    defined(LSOF_BLDCMT)
        if ((cp = isnullstr(LSOF_BLDCMT)))
            (void) fprintf(stderr, "Comment: %s\n", cp);
#endif    /* defined(LSOF_BLDCMT) */

        if ((cp = isnullstr(LSOF_CC)))
            (void) fprintf(stderr, "Compiler: %s\n", cp);
        if ((cp = isnullstr(LSOF_CCV)))
            (void) fprintf(stderr, "Compiler version: %s\n", cp);
        if ((cp = isnullstr(LSOF_CCFLAGS)))
            (void) fprintf(stderr, "Compiler flags: %s\n", cp);
        if ((cp = isnullstr(LSOF_LDFLAGS)))
            (void) fprintf(stderr, "Loader flags: %s\n", cp);
        if ((cp = isnullstr(LSOF_SYSINFO)))
            (void) fprintf(stderr, "System info: %s\n", cp);
        (void) report_SECURITY("", ".\n");
        (void) report_WARNDEVACCESS("", "are", ".\n");
        (void) report_HASKERNIDCK(" K", "is");

#if    defined(DIALECT_WARNING)
        (void) fprintf(stderr, "WARNING: %s\n", DIALECT_WARNING);
#endif    /* defined(DIALECT_WARNING) */

        (void) report_HASDCACHE(1, "", "  ");
    }
    Exit(xv);
}
