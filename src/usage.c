/*
 * usage.c - usage functions for lsof
 */


/*
 * Copyright 1998 Purdue Research Foundation, West Lafayette, Indiana
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
        "@(#) Copyright 1998 Purdue Research Foundation.\nAll rights reserved.\n";
static char *rcsid = "$Id: usage.c,v 1.30 2011/09/07 19:13:49 abe Exp $";
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
    int saved_Setgid = Setgid;

    Setgid = 0;
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
            DCpath[0] ? DCpath[0] : "none");

# if	defined(HASENVDC)
        (void) fprintf(stderr,
            "%sNamed in environment variable %s: %s\n",
            det ? det : "",
            HASENVDC, DCpath[1] ? DCpath[1] : "none");
# endif	/* defined(HASENVDC) */

# if	defined(HASSYSDC)
        if (DCpath[2])
            (void) fprintf(stderr,
            "%sSystem-wide device cache: %s\n",
            det ? det : "",
            DCpath[2]);
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
            DCpath[3] ? DCpath[3] : "none");
# endif	/* defined(HASPERSDC) */
        }
        (void) fprintf(stderr, "%sDevice cache file write paths:\n",
        ttl ? ttl : "");
        if ((dx = dcpath(2, 0)) < 0)
        (void) fprintf(stderr, "%snone\n", det ? det : "");
        else {
        (void) fprintf(stderr, "%sNamed via -D: %s\n",
            det ? det : "",
            DCstate == 2 ? "none"
                 : DCpath[0] ? DCpath[0] : "none");

# if	defined(HASENVDC)
        (void) fprintf(stderr,
            "%sNamed in environment variable %s: %s\n",
            det ? det : "",
            HASENVDC, DCpath[1] ? DCpath[1] : "none");
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
            DCpath[3] ? DCpath[3] : "none");
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
        cp = DCpath[1];
#  endif	/* defined(HASENVDC) */
#  if	defined(HASSYSDC)
        if (!cp)
        cp = HASSYSDC;
#  endif	/* defined(HASSYSDC) */
#  if	defined(HASPERSDC)
        if (!cp && dx != -1 && (dx = dcpath(1, 0)) >= 0)
        cp = DCpath[3];
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
    Setgid = saved_Setgid;
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

    if (Fhelp || xv) {
        (void) fprintf(stderr, "\n");
        (void) fprintf(stderr,
            "  " ANSI_MAGENTA "╔══════════════════════════════════════════════════════════════════════╗" ANSI_RESET "\n");
        (void) fprintf(stderr,
            "  " ANSI_MAGENTA "║" ANSI_RESET "  " ANSI_CYAN "▓▓ FILE DESCRIPTOR SCANNER // %s" ANSI_RESET "                              " ANSI_MAGENTA "║" ANSI_RESET "\n", Pn);
        (void) fprintf(stderr,
            "  " ANSI_MAGENTA "║" ANSI_RESET "  " ANSI_YELLOW ">> SCANNING OPEN FILES ACROSS ALL RUNNING PROCESSES" ANSI_RESET "          " ANSI_MAGENTA "║" ANSI_RESET "\n");
        (void) fprintf(stderr,
            "  " ANSI_MAGENTA "╚══════════════════════════════════════════════════════════════════════╝" ANSI_RESET "\n");
        (void) fprintf(stderr, "\n");
        (void) fprintf(stderr,
            "  " ANSI_DIM "USAGE:" ANSI_RESET " %s [OPTION]... [FILE]...\n",
            Pn);
    }
    if (xv && !Fhelp) {
        (void) fprintf(stderr,
            "\n  " ANSI_YELLOW ">>" ANSI_RESET " Try '" ANSI_CYAN "%s -h" ANSI_RESET "' for more information.\n", Pn);
        if (!fh)
            Exit(xv);
    }
    if (Fhelp) {
        (void) fprintf(stderr,
            "\n  " ANSI_CYAN "░░" ANSI_RESET " " ANSI_BOLD "OPTIONS" ANSI_RESET " " ANSI_CYAN "░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░" ANSI_RESET "\n\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-?, -h" ANSI_RESET "              display this help and exit\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-a" ANSI_RESET "                  AND selections (default: OR)\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-b" ANSI_RESET "                  avoid kernel blocks\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-c" ANSI_RESET " COMMAND          select by command name (prefix, ^exclude, /regex/)\n");
        (void) snpf(buf, sizeof(buf),
            "  " ANSI_GREEN "+c" ANSI_RESET " WIDTH            set COMMAND column width (default: %d)\n", CMDL);
        (void) fprintf(stderr, "%s", buf);

#if    defined(HASNCACHE)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-C" ANSI_RESET "                  disable kernel name cache\n");
#endif    /* defined(HASNCACHE) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "+d" ANSI_RESET " DIR              list open files in DIR\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-d" ANSI_RESET " FD               select by file descriptor set\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "+D" ANSI_RESET " DIR              recursively list open files in DIR (slow)\n");

#if    defined(HASDCACHE)
        if (Setuidroot)
            cp = "?|i|r";
# if	!defined(WILLDROPGID)
        else if (Myuid)
            cp = "?|i|r<path>";
# endif	/* !defined(WILLDROPGID) */
        else
            cp = "?|i|b|r|u[path]";
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-D" ANSI_RESET " ACTION           device cache control (%s)\n", cp);
#endif    /* defined(HASDCACHE) */

#if    defined(HASEOPT)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "+|-e" ANSI_RESET " PATH           exempt filesystem (risky)\n");
#endif    /* defined(HASEOPT) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "+|-f" ANSI_RESET "                +filesystem or -file names\n");

#if    defined(HASFSTRUCT)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "+|-f" ANSI_RESET " [cfgGn]        file struct info: count, addr, flags, node\n");
#endif    /* defined(HASFSTRUCT) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "-F" ANSI_RESET " [FIELDS]         select output fields; -F ? for help\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-g" ANSI_RESET " [PGID]           exclude(^) or select process group IDs\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-i" ANSI_RESET " [ADDR]           select IPv%s files",
#if    defined(HASIPv6)
            "4/6"
#else	/* !defined(HASIPv6) */
            "4"
#endif    /* defined(HASIPv6) */
        );
        (void) fprintf(stderr,
            "\n                        [%s][proto][@host|addr][:svc|port]\n",
#if    defined(HASIPv6)
            "46"
#else	/* !defined(HASIPv6) */
            "4"
#endif    /* defined(HASIPv6) */
        );

#if    defined(HASKOPT)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-k" ANSI_RESET " FILE             kernel symbols file");
        (void) fprintf(stderr, " (default: %s)\n",
            Nmlst ? Nmlst
# if	defined(N_UNIX)
                  : N_UNIX
# else	/* !defined(N_UNIX) */
                  : (Nmlst = get_nlist_path(1)) ? Nmlst : "none found"
# endif	/* defined(N_UNIX) */
        );
#endif    /* defined(HASKOPT) */

#if    defined(HASTASKS)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-K" ANSI_RESET "                  list tasks (threads)\n");
#endif    /* defined(HASTASKS) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "-l" ANSI_RESET "                  display UID numbers instead of login names\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "+|-L" ANSI_RESET " [COUNT]        list (+) or suppress (-) link counts < COUNT\n");

#if    defined(HASMOPT)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-m" ANSI_RESET " FILE             kernel memory file (default: %s)\n", KMEM);
#endif    /* defined(HASMOPT) */

#if    defined(HASMNTSUP)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "+m" ANSI_RESET " [FILE]           use or create mount supplement\n");
#endif    /* defined(HASMNTSUP) */

#if    !defined(HASNORPC_H)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "+|-M" ANSI_RESET "                enable (+) or disable (-) portmap registration");
        (void) fprintf(stderr, " (default: %s)\n",
# if    defined(HASPMAPENABLED)
            "+"
# else	/* !defined(HASPMAPENABLED) */
            "-"
# endif    /* defined(HASPMAPENABLED) */
        );
#endif    /* !defined(HASNORPC_H) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "-n" ANSI_RESET "                  inhibit host name resolution\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-N" ANSI_RESET "                  select NFS files\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-o" ANSI_RESET " [DIGITS]         display file offset (0t format, default: %d digits)\n",
            OFFDECDIG);
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-O" ANSI_RESET "                  avoid overhead of extra processing (risky)\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-p" ANSI_RESET " PID              select by PID (comma-separated, ^excludes)\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-P" ANSI_RESET "                  inhibit port number to name conversion\n");

#if    defined(HASPPID)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-R" ANSI_RESET "                  list parent PID\n");
#endif    /* defined(HASPPID) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "+|-r" ANSI_RESET " [SECONDS]      repeat mode: +until no files, -forever");
        (void) fprintf(stderr, " (default: %d)\n", RPTTM);

#if    defined(HAS_STRFTIME)
        (void) fprintf(stderr,
            "                        optional suffix: m<fmt> for strftime(3) markers\n");
#endif    /* defined(HAS_STRFTIME) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "-s" ANSI_RESET " [PROTO:STATE]    ");

#if    defined(HASTCPUDPSTATE)
        (void) fprintf(stderr,
            "exclude(^) or select protocol states by name\n");
#else	/* !defined(HASTCPUDPSTATE) */
        (void) fprintf(stderr,
            "display file size\n");
#endif    /* defined(HASTCPUDPSTATE) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "-S" ANSI_RESET " [SECONDS]        stat(2) timeout (default: %d)\n",
            TMLIMIT);
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-t" ANSI_RESET "                  terse output (PID only)\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-T" ANSI_RESET " [LEVEL]          disable or select TCP/TPI info\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-u" ANSI_RESET " USER             select by login name or UID (comma-separated, ^excludes)\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-U" ANSI_RESET "                  select UNIX domain socket files\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-v" ANSI_RESET "                  display version information\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-V" ANSI_RESET "                  verbose search\n");
        (void) fprintf(stderr,
            "  " ANSI_GREEN "+|-w" ANSI_RESET "                enable (+) or suppress (-) warnings");
        (void) fprintf(stderr, " (default: %s)\n",
#if    defined(WARNINGSTATE)
            "-"
#else	/* !defined(WARNINGSTATE) */
            "+"
#endif    /* defined(WARNINGSTATE) */
        );

        (void) fprintf(stderr,
            "  " ANSI_GREEN "-x" ANSI_RESET " [fl]             cross over +d|+D file systems (f) or symbolic links (l)\n");

#if    defined(HASXOPT)
# if	defined(HASXOPT_ROOT)
        if (Myuid == 0)
# endif	/* defined(HASXOPT_ROOT) */
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-X" ANSI_RESET "                  %s\n", HASXOPT);
#endif    /* defined(HASXOPT) */

#if    defined(HASZONES)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-z" ANSI_RESET " [ZONE]           select by zone name\n");
#endif    /* defined(HASZONES) */

#if    defined(HASSELINUX)
        if (CntxStatus)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-Z" ANSI_RESET " [CONTEXT]        select by security context\n");
#endif    /* defined(HASSELINUX) */

        (void) fprintf(stderr,
            "  " ANSI_GREEN "--" ANSI_RESET "                  end option processing\n");

#if    defined(HAS_AFS) && defined(HASAOPT)
        (void) fprintf(stderr,
            "  " ANSI_GREEN "-A" ANSI_RESET " FILE             AFS name list file (default: %s)\n",
            AFSAPATHDEF);
#endif    /* defined(HAS_AFS) && defined(HASAOPT) */

        (void) fprintf(stderr,
            "\n  " ANSI_CYAN "░░" ANSI_RESET " " ANSI_BOLD "EXAMPLES" ANSI_RESET " " ANSI_CYAN "░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░" ANSI_RESET "\n\n");
        (void) fprintf(stderr,
            "  " ANSI_YELLOW ">>" ANSI_RESET " %s " ANSI_CYAN "-i :8080" ANSI_RESET "         list files using port 8080\n", Pn);
        (void) fprintf(stderr,
            "  " ANSI_YELLOW ">>" ANSI_RESET " %s " ANSI_CYAN "-p 1234" ANSI_RESET "          list files opened by PID 1234\n", Pn);
        (void) fprintf(stderr,
            "  " ANSI_YELLOW ">>" ANSI_RESET " %s " ANSI_CYAN "-u root" ANSI_RESET "          list files opened by root\n", Pn);
        (void) fprintf(stderr,
            "  " ANSI_YELLOW ">>" ANSI_RESET " %s " ANSI_CYAN "/var/log/syslog" ANSI_RESET "  list processes using this file\n", Pn);
        (void) fprintf(stderr,
            "  " ANSI_YELLOW ">>" ANSI_RESET " %s " ANSI_CYAN "-i TCP" ANSI_RESET "           list all TCP connections\n", Pn);

        (void) fprintf(stderr, "\n");
        (void) report_SECURITY("", "; ");
        (void) report_WARNDEVACCESS("", NULL, ";");
        (void) report_HASKERNIDCK(" k", NULL);
        (void) report_HASDCACHE(0, NULL, NULL);

#if    defined(DIALECT_WARNING)
        (void) fprintf(stderr, "WARNING: %s\n", DIALECT_WARNING);
#endif    /* defined(DIALECT_WARNING) */

    }
    if (fh) {
        (void) fprintf(stderr, "\n%s: output field IDs:\n", Pn);
        (void) fprintf(stderr, "  ID    Description\n");
        for (i = 0; FieldSel[i].nm; i++) {

#if    !defined(HASPPID)
            if (FieldSel[i].id == LSOF_FID_PPID)
                continue;
#endif    /* !defined(HASPPID) */

#if    !defined(HASFSTRUCT)
            if (FieldSel[i].id == LSOF_FID_FA
                || FieldSel[i].id == LSOF_FID_CT
                || FieldSel[i].id == LSOF_FID_FG
                || FieldSel[i].id == LSOF_FID_NI)
                continue;
#else	/* defined(HASFSTRUCT) */
# if	defined(HASNOFSADDR)
            if (FieldSel[i].id == LSOF_FID_FA)
                continue;
# endif	/* defined(HASNOFSADDR) */

# if	defined(HASNOFSCOUNT)
            if (FieldSel[i].id == LSOF_FID_CT)
                continue;
# endif	/* !defined(HASNOFSCOUNT) */

# if	defined(HASNOFSFLAGS)
            if (FieldSel[i].id == LSOF_FID_FG)
                continue;
# endif	/* defined(HASNOFSFLAGS) */

# if	defined(HASNOFSNADDR)
            if (FieldSel[i].id == LSOF_FID_NI)
                continue;
# endif	/* defined(HASNOFSNADDR) */
#endif    /* !defined(HASFSTRUCT) */

#if    !defined(HASZONES)
            if (FieldSel[i].id == LSOF_FID_ZONE)
                continue;
#endif    /* !defined(HASZONES) */

#if    defined(HASSELINUX)
            if ((FieldSel[i].id == LSOF_FID_CNTX) && !CntxStatus)
                continue;
#else	/* !defined(HASSELINUX) */
            if (FieldSel[i].id == LSOF_FID_CNTX)
                continue;
#endif    /* !defined(HASSELINUX) */

            (void) fprintf(stderr, "   %c    %s\n",
                           FieldSel[i].id, FieldSel[i].nm);
        }
    }

#if    defined(HASDCACHE)
    if (DChelp)
        report_HASDCACHE(1, NULL, "    ");
#endif    /* defined(HASDCACHE) */

    if (version) {
        (void) fprintf(stderr, "\n%s %s\n", Pn, LSOF_VERSION);

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
