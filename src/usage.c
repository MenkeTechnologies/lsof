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

static char *isnullstr(char *str);

static void report_HASDCACHE(int type, char *ttl, char *det);

static void report_HASKERNIDCK(char *pfx, char *verb);

static void report_SECURITY(char *pfx, char *punct);

static void report_WARNDEVACCESS(char *pfx, char *verb, char *punct);

/*
 * isnullstr() - is it a null string?
 */

static char *isnullstr(char *str) {
    if (!str)
        return (NULL);
    while (*str) {
        if (*str != ' ')
            return (str);
        str++;
    }
    return (NULL);
}

/*
 * report_HASDCACHE() -- report device cache file state
 */

static void report_HASDCACHE(int type, char *ttl, char *det) {

#if defined(HASDCACHE)
    char *cache_path;
    int dx;

#if defined(WILLDROPGID)
    int saved_Setgid = SetgidState;

    SetgidState = 0;
#endif

    if (type) {

        /*
     * Report full device cache information.
     */
        fprintf(stderr, "%sDevice cache file read-only paths:\n", ttl ? ttl : "");
        if ((dx = dcpath(1, 0)) < 0)
            fprintf(stderr, "%snone\n", det ? det : "");
        else {
            fprintf(stderr, "%sNamed via -D: %s\n", det ? det : "",
                    DevCachePath[0] ? DevCachePath[0] : "none");

#if defined(HASENVDC)
            fprintf(stderr, "%sNamed in environment variable %s: %s\n", det ? det : "", HASENVDC,
                    DevCachePath[1] ? DevCachePath[1] : "none");
#endif

#if defined(HASSYSDC)
            if (DevCachePath[2])
                fprintf(stderr, "%sSystem-wide device cache: %s\n", det ? det : "",
                        DevCachePath[2]);
#endif

#if defined(HASPERSDC)
            fprintf(stderr, "%sPersonal path format (HASPERSDC): \"%s\"\n", det ? det : "",
                    HASPERSDC);
#if defined(HASPERSDCPATH)
            fprintf(stderr, "%sModified personal path environment variable: %s\n", det ? det : "",
                    HASPERSDCPATH);
            cache_path = getenv(HASPERSDCPATH);
            fprintf(stderr, "%s%s value: %s\n", det ? det : "", HASPERSDCPATH,
                    cache_path ? cache_path : "none");
#endif /* defined(HASPERSDCPATH) */
            fprintf(stderr, "%sPersonal path: %s\n", det ? det : "",
                    DevCachePath[3] ? DevCachePath[3] : "none");
#endif
        }
        fprintf(stderr, "%sDevice cache file write paths:\n", ttl ? ttl : "");
        if ((dx = dcpath(2, 0)) < 0)
            fprintf(stderr, "%snone\n", det ? det : "");
        else {
            fprintf(stderr, "%sNamed via -D: %s\n", det ? det : "",
                    DevCacheState == 2 ? "none"
                    : DevCachePath[0]  ? DevCachePath[0]
                                       : "none");

#if defined(HASENVDC)
            fprintf(stderr, "%sNamed in environment variable %s: %s\n", det ? det : "", HASENVDC,
                    DevCachePath[1] ? DevCachePath[1] : "none");
#endif

#if defined(HASPERSDC)
            fprintf(stderr, "%sPersonal path format (HASPERSDC): \"%s\"\n", det ? det : "",
                    HASPERSDC);
#if defined(HASPERSDCPATH)
            fprintf(stderr, "%sModified personal path environment variable: %s\n", det ? det : "",
                    HASPERSDCPATH);
            cache_path = getenv(HASPERSDCPATH);
            fprintf(stderr, "%s%s value: %s\n", det ? det : "", HASPERSDCPATH,
                    cache_path ? cache_path : "none");
#endif /* defined(HASPERSDCPATH) */
            fprintf(stderr, "%sPersonal path: %s\n", det ? det : "",
                    DevCachePath[3] ? DevCachePath[3] : "none");
#endif
        }
    } else {

        /*
     * Report device cache read file path.
     */

#if defined(HASENVDC) || defined(HASPERSDC) || defined(HASSYSDC)
        cache_path = NULL;
#if defined(HASENVDC)
        if ((dx = dcpath(1, 0)) >= 0)
            cache_path = DevCachePath[1];
#endif /* defined(HASENVDC) */
#if defined(HASSYSDC)
        if (!cache_path)
            cache_path = HASSYSDC;
#endif /* defined(HASSYSDC) */
#if defined(HASPERSDC)
        if (!cache_path && dx != -1 && (dx = dcpath(1, 0)) >= 0)
            cache_path = DevCachePath[3];
#endif /* defined(HASPERSDC) */
        if (cache_path)
            fprintf(stderr, "%s%s is the default device cache file read path.\n", ttl ? ttl : "",
                    cache_path);
#endif
    }

#if defined(WILLDROPGID)
    SetgidState = saved_Setgid;
#endif

#endif
}

/*
 * report_HASKERNIDCK() -- report HASKERNIDCK state
 */

static void report_HASKERNIDCK(char *pfx, char *verb) {
    fprintf(stderr, "%sernel ID check %s%s%s.\n", pfx ? pfx : "", verb ? verb : "", verb ? " " : "",

#if defined(HASKERNIDCK)
            "enabled"
#else
            "disabled"
#endif

    );
}

/*
 * report_SECURITY() -- report *SECURITY states
 */

static void report_SECURITY(char *pfx, char *punct) {
    fprintf(stderr, "%s%s can list all files%s", pfx ? pfx : "",

#if defined(HASSECURITY)
            "Only root",
#if defined(HASNOSOCKSECURITY)
            ", but anyone can list socket files.\n"
#else
            punct ? punct : ""
#endif
#else
            "Anyone", punct ? punct : ""
#endif

    );
}

/*
 * report_WARNDEVACCESS() -- report WEARNDEVACCESS state
 */

static void report_WARNDEVACCESS(char *pfx, char *verb, char *punct) {
    fprintf(stderr, "%s/dev warnings %s%s%s%s", pfx ? pfx : "", verb ? verb : "", verb ? " " : "",

#if defined(WARNDEVACCESS)
            "enabled",
#else
            "disabled",
#endif

            punct);
}

/*
 * usage() - display usage and exit
 */

void usage(int exit_val, int field_help, int version) {
    char buf[MAXPATHLEN + 1], *str, *str1, *str2;
    int i;

    if (OptHelp || exit_val) {
        fprintf(stderr,
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
                "\n" ANSI_CYAN "  >> FILE DESCRIPTOR SCANNER v" LSOF_VERSION " << " ANSI_RESET
                "\n" ANSI_MAGENTA "  [ mapping the topology of open files ]" ANSI_RESET "\n"
                "\n" ANSI_YELLOW "  USAGE:" ANSI_RESET " %s [OPTION]... [FILE]...\n",
                ProgramName);
    }
    if (exit_val && !OptHelp) {
        fprintf(stderr,
                "\n  " ANSI_YELLOW ">>" ANSI_RESET " Try '" ANSI_CYAN "%s -h" ANSI_RESET
                "' for more information.\n",
                ProgramName);
        if (!field_help)
            Exit(exit_val);
    }
    if (OptHelp) {
        fprintf(stderr, "\n" ANSI_CYAN
                        "  ── SELECTION ──────────────────────────────────────" ANSI_RESET "\n");
        fprintf(stderr,
                ANSI_GREEN "   -?, -h            " ANSI_RESET "display this transmission\n");
        fprintf(stderr, ANSI_GREEN "   -a                " ANSI_RESET "AND selections " ANSI_MAGENTA
                                   "(default: OR)" ANSI_RESET "\n");
        fprintf(stderr,
                ANSI_GREEN "   -c COMMAND        " ANSI_RESET "select by command name " ANSI_MAGENTA
                           "(prefix, ^exclude, /regex/)" ANSI_RESET "\n");
        snpf(buf, sizeof(buf),
             ANSI_GREEN "   +c WIDTH          " ANSI_RESET "set COMMAND column width " ANSI_MAGENTA
                        "(default: %d)" ANSI_RESET "\n",
             CMDL);
        fprintf(stderr, "%s", buf);
        fprintf(stderr,
                ANSI_GREEN "   -d FD             " ANSI_RESET "select by file descriptor set\n");
        fprintf(stderr, ANSI_GREEN "   -g [PGID]         " ANSI_RESET
                                   "exclude(^) or select process group IDs\n");
        fprintf(stderr, ANSI_GREEN "   -p PID            " ANSI_RESET "select by PID " ANSI_MAGENTA
                                   "(comma-separated, ^excludes)" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   -u USER           " ANSI_RESET
                                   "select by login name or UID " ANSI_MAGENTA
                                   "(comma-separated, ^excludes)" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   -x [fl]           " ANSI_RESET
                                   "cross over +d|+D file systems (f) or symbolic links (l)\n");

        fprintf(stderr, "\n" ANSI_CYAN
                        "  ── NETWORK ───────────────────────────────────────" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   -i [ADDR]         " ANSI_RESET "select IPv%s files\n",
#if defined(HASIPv6)
                "4/6"
#else
                "4"
#endif
        );
        fprintf(stderr,
                "                     " ANSI_MAGENTA "[%s][proto][@host|addr][:svc|port]" ANSI_RESET
                "\n",
#if defined(HASIPv6)
                "46"
#else
                "4"
#endif
        );
        fprintf(stderr,
                ANSI_GREEN "   -n                " ANSI_RESET "inhibit host name resolution\n");
        fprintf(stderr, ANSI_GREEN "   -N                " ANSI_RESET "select NFS files\n");
        fprintf(stderr, ANSI_GREEN "   -P                " ANSI_RESET
                                   "inhibit port number to name conversion\n");

#if defined(HASTCPUDPSTATE)
        fprintf(stderr, ANSI_GREEN "   -s [PROTO:STATE]  " ANSI_RESET
                                   "exclude(^) or select protocol states by name\n");
#else
        fprintf(stderr, ANSI_GREEN "   -s [PROTO:STATE]  " ANSI_RESET "display file size\n");
#endif

        fprintf(stderr,
                ANSI_GREEN "   -T [LEVEL]        " ANSI_RESET "disable or select TCP/TPI info\n");
        fprintf(stderr,
                ANSI_GREEN "   -U                " ANSI_RESET "select UNIX domain socket files\n");

#ifndef HASNORPC_H
        fprintf(stderr, ANSI_GREEN "   +|-M              " ANSI_RESET
                                   "enable (+) or disable (-) portmap registration");
        fprintf(stderr, " " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n",
#if defined(HASPMAPENABLED)
                "+"
#else
                "-"
#endif
        );
#endif

        fprintf(stderr, "\n" ANSI_CYAN
                        "  ── FILES & DIRECTORIES ───────────────────────────" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   +d DIR            " ANSI_RESET "list open files in DIR\n");
        fprintf(stderr, ANSI_GREEN "   +D DIR            " ANSI_RESET
                                   "recursively list open files in DIR " ANSI_MAGENTA
                                   "(slow)" ANSI_RESET "\n");
        fprintf(stderr,
                ANSI_GREEN "   +|-f              " ANSI_RESET "+filesystem or -file names\n");

#if defined(HASFSTRUCT)
        fprintf(stderr, ANSI_GREEN "   +|-f [cfgGn]      " ANSI_RESET
                                   "file struct info: count, addr, flags, node\n");
#endif

        fprintf(stderr, ANSI_GREEN "   +|-L [COUNT]      " ANSI_RESET
                                   "list (+) or suppress (-) link counts < COUNT\n");

#if defined(HASDCACHE)
        if (SetuidRootState)
            str = "?|i|r";
#ifndef WILLDROPGID
        else if (MyRealUid)
            str = "?|i|r<path>";
#endif
        else
            str = "?|i|b|r|u[path]";
        fprintf(stderr,
                ANSI_GREEN "   -D ACTION         " ANSI_RESET "device cache control " ANSI_MAGENTA
                           "(%s)" ANSI_RESET "\n",
                str);
#endif

#if defined(HASEOPT)
        fprintf(stderr, ANSI_GREEN "   +|-e PATH         " ANSI_RESET
                                   "exempt filesystem " ANSI_MAGENTA "(risky)" ANSI_RESET "\n");
#endif

        fprintf(stderr, "\n" ANSI_CYAN
                        "  ── DISPLAY ───────────────────────────────────────" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   -F [FIELDS]       " ANSI_RESET
                                   "select output fields; -F ? for help\n");
        fprintf(stderr, ANSI_GREEN "   -J, --json        " ANSI_RESET
                        "output in JSON format\n");
        fprintf(stderr, ANSI_GREEN "   -l                " ANSI_RESET
                                   "display UID numbers instead of login names\n");
        fprintf(stderr,
                ANSI_GREEN "   -o [DIGITS]       " ANSI_RESET "display file offset " ANSI_MAGENTA
                           "(0t format, default: %d digits)" ANSI_RESET "\n",
                OFFDECDIG);

#if defined(HASPPID)
        fprintf(stderr, ANSI_GREEN "   -R                " ANSI_RESET "list parent PID\n");
#endif

        fprintf(stderr, ANSI_GREEN "   -t                " ANSI_RESET "terse output " ANSI_MAGENTA
                                   "(PID only)" ANSI_RESET "\n");
        fprintf(stderr,
                ANSI_GREEN "   -v                " ANSI_RESET "display version information\n");
        fprintf(stderr, ANSI_GREEN "   -V                " ANSI_RESET "verbose search\n");
        fprintf(stderr, ANSI_GREEN "   +|-w              " ANSI_RESET
                                   "enable (+) or suppress (-) warnings");
        fprintf(stderr, " " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n",
#if defined(WARNINGSTATE)
                "-"
#else
                "+"
#endif
        );

        fprintf(stderr, "\n" ANSI_CYAN
                        "  ── SYSTEM ────────────────────────────────────────" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   -b                " ANSI_RESET "avoid kernel blocks\n");
        fprintf(stderr, ANSI_GREEN "   -O                " ANSI_RESET
                                   "avoid overhead of extra processing " ANSI_MAGENTA
                                   "(risky)" ANSI_RESET "\n");
        fprintf(stderr,
                ANSI_GREEN "   -S [SECONDS]      " ANSI_RESET "stat(2) timeout " ANSI_MAGENTA
                           "(default: %d)" ANSI_RESET "\n",
                TMLIMIT);
        fprintf(stderr, ANSI_GREEN "   +|-r [SECONDS]    " ANSI_RESET
                                   "repeat mode: +until no files, -forever");
        fprintf(stderr, " " ANSI_MAGENTA "(default: %d)" ANSI_RESET "\n", RPTTM);

#if defined(HAS_STRFTIME)
        fprintf(stderr, "                     " ANSI_MAGENTA
                        "optional suffix: m<fmt> for strftime(3) markers" ANSI_RESET "\n");
#endif

        fprintf(stderr, ANSI_GREEN "   --leak-detect[=I[,N]] " ANSI_RESET
                        "detect FD leaks: poll every I secs " ANSI_MAGENTA
                        "(default: 5,3)" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   --delta           " ANSI_RESET
                        "highlight new/gone FDs in repeat mode " ANSI_MAGENTA
                        "(implies +r 2)" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   -W, --monitor     " ANSI_RESET
                        "live full-screen refresh mode " ANSI_MAGENTA
                        "(like top, implies +r 2)" ANSI_RESET "\n");
        fprintf(stderr, ANSI_GREEN "   --summary, --stats " ANSI_RESET
                        "aggregate FD summary: type breakdown, top processes, per-user\n");
        fprintf(stderr, ANSI_GREEN "   --follow PID      " ANSI_RESET
                        "watch a single process's FDs, highlight opens/closes\n");

#if defined(HASNCACHE)
        fprintf(stderr,
                ANSI_GREEN "   -C                " ANSI_RESET "disable kernel name cache\n");
#endif

#if defined(HASKOPT)
        fprintf(stderr, ANSI_GREEN "   -k FILE           " ANSI_RESET "kernel symbols file");
        fprintf(stderr, " " ANSI_MAGENTA "(default: %s)" ANSI_RESET "\n",
                NamelistFilePath ? NamelistFilePath
#if defined(N_UNIX)
                                 : N_UNIX
#else
                : (NamelistFilePath = get_nlist_path(1)) ? NamelistFilePath
                                                         : "none found"
#endif
        );
#endif

#if defined(HASTASKS)
        fprintf(stderr, ANSI_GREEN "   -K                " ANSI_RESET "list tasks (threads)\n");
#endif

#if defined(HASMOPT)
        fprintf(stderr,
                ANSI_GREEN "   -m FILE           " ANSI_RESET "kernel memory file " ANSI_MAGENTA
                           "(default: %s)" ANSI_RESET "\n",
                KMEM);
#endif

#if defined(HASMNTSUP)
        fprintf(stderr,
                ANSI_GREEN "   +m [FILE]         " ANSI_RESET "use or create mount supplement\n");
#endif

        fprintf(stderr, ANSI_GREEN "   --                " ANSI_RESET "end option processing\n");

#if defined(HASXOPT)
#if defined(HASXOPT_ROOT)
        if (MyRealUid == 0)
#endif
            fprintf(stderr, ANSI_GREEN "   -X                " ANSI_RESET "%s\n", HASXOPT);
#endif

#if defined(HASZONES)
        fprintf(stderr, ANSI_GREEN "   -z [ZONE]         " ANSI_RESET "select by zone name\n");
#endif

#if defined(HASSELINUX)
        if (ContextStatus)
            fprintf(stderr,
                    ANSI_GREEN "   -Z [CONTEXT]      " ANSI_RESET "select by security context\n");
#endif

#if defined(HAS_AFS) && defined(HASAOPT)
        fprintf(stderr,
                ANSI_GREEN "   -A FILE           " ANSI_RESET "AFS name list file " ANSI_MAGENTA
                           "(default: %s)" ANSI_RESET "\n",
                AFSAPATHDEF);
#endif

        fprintf(stderr, "\n" ANSI_CYAN
                        "  ── EXAMPLES ──────────────────────────────────────" ANSI_RESET "\n");
        fprintf(stderr,
                ANSI_GREEN "   %s -i :8080       " ANSI_RESET "list files using port 8080\n",
                ProgramName);
        fprintf(stderr,
                ANSI_GREEN "   %s -p 1234        " ANSI_RESET "list files opened by PID 1234\n",
                ProgramName);
        fprintf(stderr, ANSI_GREEN "   %s -u root        " ANSI_RESET "list files opened by root\n",
                ProgramName);
        fprintf(stderr,
                ANSI_GREEN "   %s /var/log/syslog" ANSI_RESET "  list processes using this file\n",
                ProgramName);
        fprintf(stderr, ANSI_GREEN "   %s -i TCP         " ANSI_RESET "list all TCP connections\n",
                ProgramName);

        fprintf(stderr, "\n" ANSI_CYAN
                        "  ── INFO ──────────────────────────────────────────" ANSI_RESET "\n");
        fprintf(stderr, ANSI_MAGENTA "  v" LSOF_VERSION " " ANSI_RESET "// " ANSI_YELLOW
                                     "(c) lsof contributors" ANSI_RESET "\n");
        report_SECURITY("", "; ");
        report_WARNDEVACCESS("", NULL, ";");
        report_HASKERNIDCK(" k", NULL);
        report_HASDCACHE(0, NULL, NULL);
        fprintf(stderr, ANSI_MAGENTA "  Every open file tells a story." ANSI_RESET "\n");

#if defined(DIALECT_WARNING)
        fprintf(stderr, ANSI_YELLOW "WARNING: %s" ANSI_RESET "\n", DIALECT_WARNING);
#endif
    }
    if (field_help) {
        fprintf(stderr, "\n%s: output field IDs:\n", ProgramName);
        fprintf(stderr, "  ID    Description\n");
        for (i = 0; FieldSelection[i].nm; i++) {

#ifndef HASPPID
            if (FieldSelection[i].id == LSOF_FID_PPID)
                continue;
#endif

#ifndef HASFSTRUCT
            if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_ADDR ||
                FieldSelection[i].id == LSOF_FID_FILE_STRUCT_COUNT ||
                FieldSelection[i].id == LSOF_FID_FILE_FLAGS ||
                FieldSelection[i].id == LSOF_FID_NODE_ID)
                continue;
#else
#if defined(HASNOFSADDR)
            if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_ADDR)
                continue;
#endif

#if defined(HASNOFSCOUNT)
            if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_COUNT)
                continue;
#endif

#if defined(HASNOFSFLAGS)
            if (FieldSelection[i].id == LSOF_FID_FILE_FLAGS)
                continue;
#endif

#if defined(HASNOFSNADDR)
            if (FieldSelection[i].id == LSOF_FID_NODE_ID)
                continue;
#endif
#endif

#ifndef HASZONES
            if (FieldSelection[i].id == LSOF_FID_ZONE)
                continue;
#endif

#if defined(HASSELINUX)
            if ((FieldSelection[i].id == LSOF_FID_SEC_CONTEXT) && !ContextStatus)
                continue;
#else
            if (FieldSelection[i].id == LSOF_FID_SEC_CONTEXT)
                continue;
#endif

            fprintf(stderr, "   %c    %s\n", FieldSelection[i].id, FieldSelection[i].nm);
        }
    }

#if defined(HASDCACHE)
    if (DevCacheHelp)
        report_HASDCACHE(1, NULL, "    ");
#endif

    if (version) {
        fprintf(stderr, "\n%s %s\n", ProgramName, LSOF_VERSION);

#if defined(LSOF_CINFO)
        if ((str = isnullstr(LSOF_CINFO)))
            fprintf(stderr, "Configuration: %s\n", str);
#endif

        if ((str = isnullstr(LSOF_CCDATE)))
            fprintf(stderr, "Built: %s\n", str);
        str = isnullstr(LSOF_HOST);
        if (!(str1 = isnullstr(LSOF_LOGNAME)))
            str1 = isnullstr(LSOF_USER);
        if (str || str1) {
            if (str && str1)
                str2 = "by and on";
            else if (str)
                str2 = "on";
            else
                str2 = "by";
            fprintf(stderr, "Built %s: %s%s%s\n", str2, str1 ? str1 : "", (str && str1) ? "@" : "",
                    str ? str : "");
        }

#if defined(LSOF_BLDCMT)
        if ((str = isnullstr(LSOF_BLDCMT)))
            fprintf(stderr, "Comment: %s\n", str);
#endif

        if ((str = isnullstr(LSOF_CC)))
            fprintf(stderr, "Compiler: %s\n", str);
        if ((str = isnullstr(LSOF_CCV)))
            fprintf(stderr, "Compiler version: %s\n", str);
        if ((str = isnullstr(LSOF_CCFLAGS)))
            fprintf(stderr, "Compiler flags: %s\n", str);
        if ((str = isnullstr(LSOF_LDFLAGS)))
            fprintf(stderr, "Loader flags: %s\n", str);
        if ((str = isnullstr(LSOF_SYSINFO)))
            fprintf(stderr, "System info: %s\n", str);
        report_SECURITY("", ".\n");
        report_WARNDEVACCESS("", "are", ".\n");
        report_HASKERNIDCK(" K", "is");

#if defined(DIALECT_WARNING)
        fprintf(stderr, "WARNING: %s\n", DIALECT_WARNING);
#endif

        report_HASDCACHE(1, "", "  ");
    }
    Exit(exit_val);
}
