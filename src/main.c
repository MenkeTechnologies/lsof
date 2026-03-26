/*
 * main.c - common main function for lsof
 *
 * J. Menke
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
 * Local definitions
 */

static int GObk[] = {1, 1}; /* option backspace values */
static char GOp;            /* option prefix -- '+' or '-' */
static char *GOv = NULL;    /* option `:' value pointer */
static int GOx1 = 1;        /* first opt[][] index */
static int GOx2 = 0;        /* second opt[][] index */

static int GetOpt(int opt_count, char *opt[], char *rules, int *err);

static char *sv_fmt_str(char *fmt_str);

/*
 * main() - main function for lsof
 */

int main(int argc, char *argv[]) {
    int alt_dev, opt_char, i, num_parsed, return_val, stat_errno1, stat_errno2, stat_status;
    char *char_ptr;
    int err = 0;
    int exit_val = 0;
    int field_help = 0;
    char *fmtr = NULL;
    long long_val;
    MALLOC_S len;
    struct lfile *saved_lfile;
    struct nwad *net_addr_ptr, *net_addr_next;
    char options[128];
    int repeat_cond = 0;
    struct stat sb;
    struct sfile *search_file_ptr;
    struct lproc **sorted_procs = NULL;
    int sorted_alloc = 0;
    struct str_lst *str_entry, *str_test;
    int version = 0;
    int xover = 0;

#if defined(HAS_STRFTIME)
    char *fmt = NULL;
    size_t fmtl;
#endif

#if defined(HASZONES)
    znhash_t *zp;
#endif

#if defined(HASSELINUX)
    /*
     * This stanza must be immediately before the "Save progam name." code, since
     * it contains code itself.
     */
    cntxlist_t *cntxp;

    ContextStatus = is_selinux_enabled() ? 1 : 0;
#endif

    /*
 * Save program name.
 */
    if ((ProgramName = strrchr(argv[0], '/')))
        ProgramName++;
    else
        ProgramName = argv[0];
    /*
 * Close all file descriptors above 2.
 *
 * Make sure stderr, stdout, and stdin are open descriptors.  Open /dev/null
 * for ones that aren't.  Be terse.
 *
 * Make sure umask allows lsof to define its own file permissions.
 */
    for (i = 3, num_parsed = GET_MAX_FD(); i < num_parsed; i++)
        close(i);
    while (((i = open("/dev/null", O_RDWR, 0)) >= 0) && (i < 2))
        ;
    if (i < 0)
        Exit(1);
    if (i > 2)
        close(i);
    umask(0);

#if defined(HASSETLOCALE)
    /*
     * Set locale to environment's definition.
     */
    setlocale(LC_CTYPE, "");
#endif

    /*
 * Common initialization.
 */
    MyProcessId = getpid();
    if ((MyRealGid = (gid_t)getgid()) != getegid())
        SetgidState = 1;
    EffectiveUid = geteuid();
    if ((MyRealUid = (uid_t)getuid()) && !EffectiveUid)
        SetuidRootState = 1;
    if (!(NameChars = (char *)malloc(MAXPATHLEN + 1))) {
        fprintf(stderr, "%s: no space for name buffer\n", ProgramName);
        Exit(1);
    }
    NameCharsLength = (size_t)(MAXPATHLEN + 1);
    /*
 * Create option mask.
 */
    snpf(options, sizeof(options),
         "?a%sbc:%sD:d:%sf:F:g:hi:%s%slL:%s%snNo:Op:Pr:%ss:S:tT:u:UvVwx:%s%s%s",

#if defined(HAS_AFS) && defined(HASAOPT)
         "A:",
#else
         "",
#endif

#if defined(HASNCACHE)
         "C",
#else
         "",
#endif

#if defined(HASEOPT)
         "e:",
#else
         "",
#endif

#if defined(HASKOPT)
         "k:",
#else
         "",
#endif

#if defined(HASTASKS)
         "K",
#else
         "",
#endif

#if defined(HASMOPT) || defined(HASMNTSUP)
         "m:",
#else
         "",
#endif

#if defined(HASNORPC_H)
         "",
#else
         "M",
#endif

#if defined(HASPPID)
         "R",
#else
         "",
#endif

#if defined(HASXOPT)
#if defined(HASXOPT_ROOT)
         (MyRealUid == 0) ? "X" : "",
#else
         "X",
#endif
#else
         "",
#endif

#if defined(HASZONES)
         "z:",
#else
         "",
#endif

#if defined(HASSELINUX)
         "Z:"
#else
         ""
#endif

    );
    /*
 * Loop through options.
 */
    while ((opt_char = GetOpt(argc, argv, options, &return_val)) != EOF) {
        if (return_val) {
            err = 1;
            continue;
        }
        switch (opt_char) {
        case 'a':
            OptAndSelection = 1;
            break;

#if defined(HAS_AFS) && defined(HASAOPT)
        case 'A':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                fprintf(stderr, "%s: -A not followed by path\n", ProgramName);
                err = 1;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
            } else
                AFSApath = GOv;
            break;
#endif

        case 'b':
            OptBlockDevice = 1;
            break;
        case 'c':
            if (GOp == '+') {
                if (!GOv || (*GOv == '-') || (*GOv == '+') || !isdigit((int)*GOv)) {
                    fprintf(stderr, "%s: +c not followed by width number\n", ProgramName);
                    err = 1;
                    if (GOv) {
                        GOx1 = GObk[0];
                        GOx2 = GObk[1];
                    }
                } else {
                    CommandColLimit = atoi(GOv);

#if defined(MAXSYSCMDL)
                    if (CommandColLimit > MAXSYSCMDL) {
                        fprintf(stderr, "%s: +c %d > what system provides (%d)\n", ProgramName,
                                CommandColLimit, MAXSYSCMDL);
                        err = 1;
                    }
#endif
                }
                break;
            }
            if (GOv && (*GOv == '/')) {
                if (enter_cmd_rx(GOv))
                    err = 1;
            } else {
                if (enter_str_lst("-c", GOv, &CommandNameList, &CommandNameInclusions,
                                  &CommandNameExclusions))
                    err = 1;

#if defined(MAXSYSCMDL)
                else if (CommandNameList->len > MAXSYSCMDL) {
                    fprintf(stderr, "%s: \"-c ", ProgramName);
                    safestrprt(CommandNameList->str, stderr, 2);
                    fprintf(stderr, "\" length (%d) > what system", CommandNameList->len);
                    fprintf(stderr, " provides (%d)\n", MAXSYSCMDL);
                    CommandNameList->len = 0; /* (to avoid later error report) */
                    err = 1;
                }
#endif
            }
            break;

#if defined(HASNCACHE)
        case 'C':
            OptNameCache = (GOp == '-') ? 0 : 1;
            break;
#endif

#if defined(HASEOPT)
        case 'e':
            if (enter_efsys(GOv, ((GOp == '+') ? 1 : 0)))
                err = 1;
            break;
#endif

        case 'd':
            if (GOp == '+') {
                if (enter_dir(GOv, 0))
                    err = 1;
                else {
                    SelectionFlags |= SELNM;
                    xover = 1;
                }
            } else {
                if (enter_fd(GOv))
                    err = 1;
            }
            break;
        case 'D':
            if (GOp == '+') {
                if (enter_dir(GOv, 1))
                    err = 1;
                else {
                    SelectionFlags |= SELNM;
                    xover = 1;
                }
            } else {

#if defined(HASDCACHE)
                if (ctrl_dcache(GOv))
                    err = 1;
#else
                fprintf(stderr, "%s: unsupported option: -D\n", ProgramName);
                err = 1;
#endif
            }
            break;
        case 'f':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                OptFileSystem = (GOp == '+') ? 2 : 1;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            }

#if defined(HASFSTRUCT)
            for (; *GOv; GOv++) {
                switch (*GOv) {

#ifndef HASNOFSCOUNT
                case 'c':
                case 'C':
                    if (GOp == '+') {
                        OptFileStructValues |= FSV_FILE_COUNT;
                        OptFileStructSetByFlag = 1;
                    } else
                        OptFileStructValues &= (unsigned char)~FSV_FILE_COUNT;
                    break;
#endif

#ifndef HASNOFSADDR
                case 'f':
                case 'F':
                    if (GOp == '+') {
                        OptFileStructValues |= FSV_FILE_ADDR;
                        OptFileStructSetByFlag = 1;
                    } else
                        OptFileStructValues &= (unsigned char)~FSV_FILE_ADDR;
                    break;
#endif

#ifndef HASNOFSFLAGS
                case 'g':
                case 'G':
                    if (GOp == '+') {
                        OptFileStructValues |= FSV_FILE_FLAGS;
                        OptFileStructSetByFlag = 1;
                    } else
                        OptFileStructValues &= (unsigned char)~FSV_FILE_FLAGS;
                    OptFileStructFlagHex = (*GOv == 'G') ? 1 : 0;
                    break;
#endif

#ifndef HASNOFSNADDR
                case 'n':
                case 'N':
                    if (GOp == '+') {
                        OptFileStructValues |= FSV_NODE_ID;
                        OptFileStructSetByFlag = 1;
                    } else
                        OptFileStructValues &= (unsigned char)~FSV_NODE_ID;
                    break;
#endif

                default:
                    fprintf(stderr, "%s: unknown file struct option: %c\n", ProgramName, *GOv);
                    err++;
                }
            }
#else
            fprintf(stderr, "%s: unknown string for %cf: %s\n", ProgramName, GOp, GOv);
            err++;
#endif

            break;
        case 'F':
            if (!GOv || *GOv == '-' || *GOv == '+' || strcmp(GOv, "0") == 0) {
                if (GOv) {
                    if (*GOv == '-' || *GOv == '+') {
                        GOx1 = GObk[0];
                        GOx2 = GObk[1];
                    } else if (*GOv == '0')
                        Terminator = '\0';
                }
                for (i = 0; FieldSelection[i].nm; i++) {

#ifndef HASPPID
                    if (FieldSelection[i].id == LSOF_FID_PPID)
                        continue;
#endif

#ifndef HASFSTRUCT
                    if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_COUNT ||
                        FieldSelection[i].id == LSOF_FID_FILE_STRUCT_ADDR ||
                        FieldSelection[i].id == LSOF_FID_FILE_FLAGS ||
                        FieldSelection[i].id == LSOF_FID_NODE_ID)
                        continue;
#endif

#if defined(HASSELINUX)
                    if ((FieldSelection[i].id == LSOF_FID_SEC_CONTEXT) && !ContextStatus)
                        continue;
#else
                    if (FieldSelection[i].id == LSOF_FID_SEC_CONTEXT)
                        continue;
#endif

                    if (FieldSelection[i].id == LSOF_FID_RDEV)
                        continue; /* for compatibility */

#ifndef HASTASKS
                    if (FieldSelection[i].id == LSOF_FID_TID)
                        continue;
#endif

#ifndef HASZONES
                    if (FieldSelection[i].id == LSOF_FID_ZONE)
                        continue;
#endif

                    FieldSelection[i].st = 1;
                    if (FieldSelection[i].opt && FieldSelection[i].ov)
                        *(FieldSelection[i].opt) |= FieldSelection[i].ov;
                }

#if defined(HASFSTRUCT)
                OptFieldOutput = OptFileStructFlagHex = 1;
#else
                OptFieldOutput = 1;
#endif

                break;
            }
            if (strcmp(GOv, "?") == 0) {
                field_help = 1;
                break;
            }
            for (; *GOv; GOv++) {
                for (i = 0; FieldSelection[i].nm; i++) {

#ifndef HASPPID
                    if (FieldSelection[i].id == LSOF_FID_PPID)
                        continue;
#endif

#ifndef HASFSTRUCT
                    if (FieldSelection[i].id == LSOF_FID_FILE_STRUCT_COUNT ||
                        FieldSelection[i].id == LSOF_FID_FILE_STRUCT_ADDR ||
                        FieldSelection[i].id == LSOF_FID_FILE_FLAGS ||
                        FieldSelection[i].id == LSOF_FID_NODE_ID)
                        continue;
#endif

#ifndef HASTASKS
                    if (FieldSelection[i].id == LSOF_FID_TID)
                        continue;
#endif

                    if (FieldSelection[i].id == *GOv) {
                        FieldSelection[i].st = 1;
                        if (FieldSelection[i].opt && FieldSelection[i].ov)
                            *(FieldSelection[i].opt) |= FieldSelection[i].ov;

#if defined(HASFSTRUCT)
                        if (i == LSOF_FIX_FILE_FLAGS)
                            OptFileStructFlagHex = 1;
#endif

                        if (i == LSOF_FIX_TERM)
                            Terminator = '\0';
                        break;
                    }
                }
                if (!FieldSelection[i].nm) {
                    fprintf(stderr, "%s: unknown field: %c\n", ProgramName, *GOv);
                    err++;
                }
            }
            OptFieldOutput = 1;
            break;
        case 'g':
            if (GOv) {
                if (*GOv == '-' || *GOv == '+') {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                } else if (enter_id(PGID, GOv))
                    err = 1;
            }
            OptProcessGroup = 1;
            break;
        case 'h':
        case '?':
            OptHelp = 1;
            break;
        case 'i':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                OptNetwork = 1;
                OptNetworkType = 0;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            }
            if (enter_network_address(GOv))
                err = 1;
            break;

#if defined(HASKOPT)
        case 'k':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                fprintf(stderr, "%s: -k not followed by path\n", ProgramName);
                err = 1;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
            } else
                NamelistFilePath = GOv;
            break;
#endif

#if defined(HASTASKS)
        case 'K':
            OptTask = 1;
            SelectionFlags |= SELTASK;
            break;
#endif

        case 'l':
            OptUserToLogin = 0;
            break;
        case 'L':
            OptLinkCount = (GOp == '+') ? 1 : 0;
            if (!GOv || *GOv == '-' || *GOv == '+') {
                LinkCountThreshold = 0l;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            }
            for (char_ptr = GOv, long_val = 0l, num_parsed = 0; *char_ptr; char_ptr++) {
                if (!isdigit((unsigned char)*char_ptr))
                    break;
                long_val = (long_val * 10l) + ((long)*char_ptr - (long)'0');
                num_parsed++;
            }
            if (num_parsed) {
                if (GOp != '+') {
                    fprintf(stderr, "%s: no number may follow -L\n", ProgramName);
                    err = 1;
                } else {
                    LinkCountThreshold = long_val;
                    SelectionFlags |= SELNLINK;
                }
            } else
                LinkCountThreshold = 0l;
            if (*char_ptr) {
                GOx1 = GObk[0];
                GOx2 = GObk[1] + num_parsed;
            }
            break;

#if defined(HASMOPT) || defined(HASMNTSUP)
        case 'm':
            if (GOp == '-') {

#if defined(HASMOPT)
                if (!GOv || *GOv == '-' || *GOv == '+') {
                    fprintf(stderr, "%s: -m not followed by path\n", ProgramName);
                    err = 1;
                    if (GOv) {
                        GOx1 = GObk[0];
                        GOx2 = GObk[1];
                    }
                } else
                    Memory = GOv;
#else
                fprintf(stderr, "%s: -m not supported\n", ProgramName);
                err = 1;
#endif

            } else if (GOp == '+') {

#if defined(HASMNTSUP)
                if (!GOv || *GOv == '-' || *GOv == '+') {
                    MountSupplementState = 1;
                    if (GOv) {
                        GOx1 = GObk[0];
                        GOx2 = GObk[1];
                    }
                } else {
                    MountSupplementState = 2;
                    MountSupplementPath = GOv;
                }
#else
                fprintf(stderr, "%s: +m not supported\n", ProgramName);
                err = 1;
#endif

            } else {
                fprintf(stderr, "%s: %cm not supported\n", ProgramName, GOp);
                err = 1;
            }
            break;
#endif

#ifndef HASNORPC_H
        case 'M':
            OptPortMap = (GOp == '+') ? 1 : 0;
            break;
#endif

        case 'n':
            OptHostLookup = (GOp == '-') ? 0 : 1;
            break;
        case 'N':
            OptNfs = 1;
            break;
        case 'o':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                OptOffset = 1;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            }
            for (char_ptr = GOv, i = num_parsed = 0; *char_ptr; char_ptr++) {
                if (!isdigit((unsigned char)*char_ptr))
                    break;
                i = (i * 10) + ((int)*char_ptr - '0');
                num_parsed++;
            }
            if (num_parsed)
                OffsetDecDigitLimit = i;
            else
                OptOffset = 1;
            if (*char_ptr) {
                GOx1 = GObk[0];
                GOx2 = GObk[1] + num_parsed;
            }
            break;
        case 'O':
            OptOverhead = (GOp == '-') ? 1 : 0;
            break;
        case 'p':
            if (enter_id(PID, GOv))
                err = 1;
            break;
        case 'P':
            OptPortLookup = (GOp == '-') ? 0 : 1;
            break;
        case 'r':
            if (GOp == '+')
                exit_val = repeat_cond = 1;
            if (!GOv || *GOv == '-' || *GOv == '+') {
                RepeatTime = RPTTM;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            }
            for (char_ptr = GOv, i = num_parsed = 0; *char_ptr; char_ptr++) {
                if (!isdigit((unsigned char)*char_ptr))
                    break;
                i = (i * 10) + ((int)*char_ptr - '0');
                num_parsed++;
            }
            if (num_parsed)
                RepeatTime = i;
            else
                RepeatTime = RPTTM;
            if (!*char_ptr)
                break;
            while (*char_ptr && (*char_ptr == ' '))
                char_ptr++;
            if (*char_ptr != LSOF_FID_MARK) {
                GOx1 = GObk[0];
                GOx2 = GObk[1] + num_parsed;
                break;
            }

#if defined(HAS_STRFTIME)

            /*
             * Collect the strftime(3) format and test it.
             */
            char_ptr++;
            if ((fmtl = strlen(char_ptr) + 1) < 1) {
                fprintf(stderr, "%s: <fmt> too short: \"%s\"\n", ProgramName, char_ptr);
                err = 1;
            } else {
                fmt = char_ptr;
                fmtl = (fmtl * 8) + 1;
                if (!(fmtr = (char *)malloc((MALLOC_S)fmtl))) {
                    fprintf(stderr, "%s: no space (%d) for <fmt> result: \"%s\"\n", ProgramName,
                            (int)fmtl, char_ptr);
                    Exit(1);
                }
                if (util_strftime(fmtr, fmtl - 1, fmt) < 1) {
                    fprintf(stderr, "%s: illegal <fmt>: \"%s\"\n", ProgramName, fmt);
                    err = 1;
                }
            }

#else
            fprintf(stderr, "%s: m<fmt> not supported: \"%s\"\n", ProgramName, char_ptr);
            err = 1;
#endif

            break;

#if defined(HASPPID)
        case 'R':
            OptParentPid = 1;
            break;
#endif

        case 's':

#if defined(HASTCPUDPSTATE)
            if (!GOv || *GOv == '-' || *GOv == '+') {
                OptSize = 1;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
            } else {
                if (enter_state_spec(GOv))
                    err = 1;
            }
#else
            OptSize = 1;
#endif

            break;
        case 'S':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                TimeoutLimit = TMLIMIT;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            }
            for (char_ptr = GOv, i = num_parsed = 0; *char_ptr; char_ptr++) {
                if (!isdigit((unsigned char)*char_ptr))
                    break;
                i = (i * 10) + ((int)*char_ptr - '0');
                num_parsed++;
            }
            if (num_parsed)
                TimeoutLimit = i;
            else
                TimeoutLimit = TMLIMIT;
            if (*char_ptr) {
                GOx1 = GObk[0];
                GOx2 = GObk[1] + num_parsed;
            }
            if (TimeoutLimit < TMLIMMIN) {
                fprintf(stderr, "%s: WARNING: -S time (%d) changed to %d\n", ProgramName,
                        TimeoutLimit, TMLIMMIN);
                TimeoutLimit = TMLIMMIN;
            }
            break;
        case 't':
            OptTerse = OptWarnings = 1;
            break;
        case 'T':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                OptTcpTpiInfo = (GOp == '-') ? 0 : TCPTPI_STATE;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            }
            for (OptTcpTpiInfo = 0; *GOv; GOv++) {
                switch (*GOv) {

#if defined(HASSOOPT) || defined(HASSOSTATE) || defined(HASTCPOPT)
                case 'f':
                    OptTcpTpiInfo |= TCPTPI_FLAGS;
                    break;
#endif

#if defined(HASTCPTPIQ)
                case 'q':
                    OptTcpTpiInfo |= TCPTPI_QUEUES;
                    break;
#endif

                case 's':
                    OptTcpTpiInfo |= TCPTPI_STATE;
                    break;

#if defined(HASTCPTPIW)
                case 'w':
                    OptTcpTpiInfo |= TCPTPI_WINDOWS;
                    break;
#endif

                default:
                    fprintf(stderr, "%s: unsupported TCP/TPI info selection: %c\n", ProgramName,
                            *GOv);
                    err = 1;
                }
            }
            break;
        case 'u':
            if (enter_uid(GOv))
                err = 1;
            break;
        case 'U':
            OptUnixSocket = 1;
            break;
        case 'v':
            version = 1;
            break;
        case 'V':
            OptVerbose = 1;
            break;
        case 'w':
            OptWarnings = (GOp == '+') ? 0 : 1;
            break;
        case 'x':
            if (!GOv || *GOv == '-' || *GOv == '+') {
                OptCrossover = XO_ALL;
                if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
                break;
            } else {
                for (; *GOv; GOv++) {
                    switch (*GOv) {
                    case 'f':
                        OptCrossover |= XO_FILESYS;
                        break;
                    case 'l':
                        OptCrossover |= XO_SYMLINK;
                        break;
                    default:
                        fprintf(stderr, "%s: unknown cross-over option: %c\n", ProgramName, *GOv);
                        err++;
                    }
                }
            }
            break;

#if defined(HASXOPT)
        case 'X':
            OptCrossoverExt = OptCrossoverExt ? 0 : 1;
            break;
#endif

#if defined(HASZONES)
        case 'z':
            OptZone = 1;
            if (GOv && (*GOv != '-') && (*GOv != '+')) {

                /*
             * Add to the zone name argument hash.
             */
                if (enter_zone_arg(GOv))
                    err = 1;
            } else if (GOv) {
                GOx1 = GObk[0];
                GOx2 = GObk[1];
            }
            break;
#endif

#if defined(HASSELINUX)
        case 'Z':
            if (!ContextStatus) {
                fprintf(stderr, "%s: -Z limited to SELinux\n", ProgramName);
                err = 1;
            } else {
                OptSecContext = 1;
                if (GOv && (*GOv != '-') && (*GOv != '+')) {

                    /*
                 * Add to the context name argument hash.
                 */
                    if (enter_cntx_arg(GOv))
                        err = 1;
                } else if (GOv) {
                    GOx1 = GObk[0];
                    GOx2 = GObk[1];
                }
            }
            break;
#endif

        default:
            fprintf(stderr, "%s: unknown option (%c)\n", ProgramName, opt_char);
            err = 1;
        }
    }
    /*
 * Check for argument consistency.
 */
    if (CommandNameExclusions && CommandNameInclusions) {

        /*
         * Check for command inclusion/exclusion conflicts.
         */
        for (str_entry = CommandNameList; str_entry; str_entry = str_entry->next) {
            if (str_entry->x) {
                for (str_test = CommandNameList; str_test; str_test = str_test->next) {
                    if (!str_test->x) {
                        if (!strcmp(str_entry->str, str_test->str)) {
                            fprintf(stderr, "%s: -c^%s and -c%s conflict.\n", ProgramName,
                                    str_entry->str, str_test->str);
                            err++;
                        }
                    }
                }
            }
        }
    }

#if defined(HASTCPUDPSTATE)
    if (TcpStateExcludeCount && TcpStateIncludeCount) {

        /*
     * Check for excluded and included TCP states.
     */
        for (i = 0; i < TcpNumStates; i++) {
            if (TcpStateExclude[i] && TcpStateInclude[i]) {
                fprintf(stderr, "%s: can't include and exclude TCP state: %s\n", ProgramName,
                        TcpStateNames[i]);
                err = 1;
            }
        }
    }
    if (UdpStateExcludeCount && UdpStateIncludeCount) {

        /*
     * Check for excluded and included UDP states.
     */
        for (i = 0; i < UdpNumStates; i++) {
            if (UdpStateExclude[i] && UdpStateInclude[i]) {
                fprintf(stderr, "%s: can't include and exclude UDP state: %s\n", ProgramName,
                        UdpStateNames[i]);
                err = 1;
            }
        }
    }
#endif

    if (OptSize && OptOffset) {
        fprintf(stderr, "%s: -o and -s are mutually exclusive\n", ProgramName);
        err++;
    }
    if (OptFieldOutput) {
        if (OptTerse) {
            fprintf(stderr, "%s: -F and -t are mutually exclusive\n", ProgramName);
            err++;
        }
        FieldSelection[LSOF_FIX_PID].st = 1;

#if defined(HAS_STRFTIME)
        if (fmtr) {

            /*
         * The field output marker format can't contain "%n" new line
         * requests.
         */
            for (char_ptr = strchr(fmt, '%'); char_ptr; char_ptr = strchr(char_ptr, '%')) {
                if (*++char_ptr == 'n') {
                    fprintf(stderr, "%s: %%n illegal in -r m<fmt> when -F has", ProgramName);
                    fprintf(stderr, " been specified: \"%s\"\n", fmt);
                    err++;
                    break;
                } else if (*char_ptr == '%')
                    char_ptr++;
            }
        }
#endif
    }
    if (OptCrossover && !xover) {
        fprintf(stderr, "%s: -x must accompany +d or +D\n", ProgramName);
        err++;
    }

#if defined(HASEOPT)
    if (ExcludedFileSysList) {

        /*
     * If there are file systems specified by -e options, check them.
     */
        efsys_list_t *ep;        /* ExcludedFileSysList pointer */
        struct mounts *mp, *mpw; /* local mount table pointers */

        if ((mp = readmnt())) {
            for (ep = ExcludedFileSysList; ep; ep = ep->next) {
                for (mpw = mp; mpw; mpw = mpw->next) {
                    if (!strcmp(mpw->dir, ep->path)) {
                        ep->mp = mpw;
                        break;
                    }
                }
                if (!ep->mp) {
                    fprintf(stderr, "%s: \"-e %s\" is not a mounted file system.\n", ProgramName,
                            ep->path);
                    err++;
                }
            }
        }
    }
#endif

    if (DevCacheHelp || err || OptHelp || field_help || version)
        usage(err ? 1 : 0, field_help, version);
    /*
 * Reduce the size of SearchUidList[], if necessary.
 */
    if (SearchUidList && NumUidSelections && NumUidSelections < MaxUidEntries) {
        struct seluid *tmp = (struct seluid *)realloc(
            (MALLOC_P *)SearchUidList, (MALLOC_S)(sizeof(struct seluid) * NumUidSelections));
        if (!tmp) {
            fprintf(stderr, "%s: can't realloc UID table\n", ProgramName);
            free((FREE_P *)SearchUidList);
            SearchUidList = NULL;
            Exit(1);
        }
        SearchUidList = tmp;
        MaxUidEntries = NumUidSelections;
    }
    /*
 * Compute the selection flags.
 */
    if ((CommandNameList && CommandNameInclusions) || CommandRegexTable)
        SelectionFlags |= SELCMD;

#if defined(HASSELINUX)
    if (ContextArgList)
        SelectionFlags |= SELCNTX;
#endif

    if (FdList)
        SelectionFlags |= SELFD;
    if (OptNetwork)
        SelectionFlags |= SELNET;
    if (OptNfs)
        SelectionFlags |= SELNFS;
    if (OptUnixSocket)
        SelectionFlags |= SELUNX;
    if (NumPgidSelections && NumPgidInclusions)
        SelectionFlags |= SELPGID;
    if (NumPidSelections && NumPidInclusions)
        SelectionFlags |= SELPID;
    if (NumUidSelections && NumUidInclusions)
        SelectionFlags |= SELUID;
    if (NetworkAddrList)
        SelectionFlags |= SELNA;

#if defined(HASZONES)
    if (ZoneArg)
        SelectionFlags |= SELZONE;
#endif

    if (GOx1 < argc)
        SelectionFlags |= SELNM;
    if (SelectionFlags == 0) {
        if (OptAndSelection) {
            fprintf(stderr, "%s: no select options to AND via -a\n", ProgramName);
            usage(1, 0, 0);
        }
        SelectionFlags = SELALL;
    } else {
        if (GOx1 >= argc && (SelectionFlags & (SELNA | SELNET)) != 0 &&
            (SelectionFlags & ~(SELNA | SELNET)) == 0)
            SelectInetOnly = 1;
        SelectAll = 0;
    }
    /*
 * Get the device for DEVDEV_PATH.
 */
    if (stat(DEVDEV_PATH, &sb)) {
        stat_errno1 = errno;
        if ((alt_dev = strcmp(DEVDEV_PATH, "/dev"))) {
            if ((stat_status = stat("/dev", &sb)))
                stat_errno2 = errno;
            else
                stat_errno2 = 0;
        } else {
            stat_errno2 = 0;
            stat_status = 1;
        }
        if (stat_status) {
            fprintf(stderr, "%s: can't stat(%s): %s\n", ProgramName, DEVDEV_PATH,
                    strerror(stat_errno1));
            if (alt_dev) {
                fprintf(stderr, "%s: can't stat(/dev): %s\n", ProgramName, strerror(stat_errno2));
            }
            Exit(1);
        }
    }
    DeviceOfDev = sb.st_dev;
    /*
 * Process the file arguments.
 */
    if (GOx1 < argc) {
        if (ck_file_arg(GOx1, argc, argv, OptFileSystem, 0, NULL))
            usage(1, 0, 0);
    }
    /*
 * Do dialect-specific initialization.
 */
    initialize();
    if (SearchFileChain)
        hashSfile();

#if defined(WILLDROPGID)
    /*
     * If this process isn't setuid(root), but it is setgid(not_real_gid),
     * relinquish the setgid power.  (If it hasn't already been done.)
     */
    dropgid();
#endif

#if defined(HASDCACHE)
    /*
     * If there is a device cache, prepare the device table.
     */
    if (DevCacheState)
        readdev(0);
#endif

    /*
 * Define the size and offset print formats.
 */
    snpf(options, sizeof(options), "%%%su", INODEPSPEC);
    InodeFormatDecimal = sv_fmt_str(options);
    snpf(options, sizeof(options), "%%#%sx", INODEPSPEC);
    InodeFormatHex = sv_fmt_str(options);
    snpf(options, sizeof(options), "0t%%%su", SZOFFPSPEC);
    SizeOffFormat0t = sv_fmt_str(options);
    snpf(options, sizeof(options), "%%%su", SZOFFPSPEC);
    SizeOffFormatD = sv_fmt_str(options);
    snpf(options, sizeof(options), "%%*%su", SZOFFPSPEC);
    SizeOffFormatDv = sv_fmt_str(options);
    snpf(options, sizeof(options), "%%#%sx", SZOFFPSPEC);
    SizeOffFormatX = sv_fmt_str(options);

#if defined(HASMNTSUP)
    /*
     * Report mount supplement information, as requested.
     */
    if (MountSupplementState == 1) {
        readmnt();
        Exit(0);
    }
#endif

    /*
 * Gather and report process information every RepeatTime seconds.
 */
    if (RepeatTime)
        CheckPasswdChange = 1;
    do {

        /*
         * Gather information about processes.
         */
        gather_proc_info();
        /*
         * If the local process table has more than one entry, sort it by PID.
         */
        if (NumLocalProcs > 1) {
            if (NumLocalProcs > sorted_alloc) {
                len = (MALLOC_S)(NumLocalProcs * sizeof(struct lproc *));
                sorted_alloc = NumLocalProcs;
                if (!sorted_procs)
                    sorted_procs = (struct lproc **)malloc(len);
                else {
                    struct lproc **tmp = (struct lproc **)realloc((MALLOC_P *)sorted_procs, len);
                    if (!tmp) {
                        free((FREE_P *)sorted_procs);
                        sorted_procs = NULL;
                    } else
                        sorted_procs = tmp;
                }
                if (!sorted_procs) {
                    fprintf(stderr, "%s: no space for %d sort pointers\n", ProgramName,
                            NumLocalProcs);
                    Exit(1);
                }
            }
            for (i = 0; i < NumLocalProcs; i++) {
                sorted_procs[i] = &LocalProcTable[i];
            }
            qsort((QSORT_P *)sorted_procs, (size_t)NumLocalProcs, (size_t)sizeof(struct lproc *),
                  comppid);
        }
        if ((num_parsed = NumLocalProcs)) {

#if defined(HASNCACHE)
            /*
             * If using the kernel name cache, force its reloading.
             */
            NameCacheReload = 1;
#endif

            /*
             * Print the selected processes and count them.
             *
             * CurrentLocalFile contents must be preserved, since they may point to a
             * malloc()'d area, and since CurrentLocalFile is used throughout the print
             * process.
             */
            for (saved_lfile = CurrentLocalFile, print_init(); PrintPass < 2; PrintPass++) {
                for (i = num_parsed = 0; i < NumLocalProcs; i++) {
                    CurrentLocalProc = (NumLocalProcs > 1) ? sorted_procs[i] : &LocalProcTable[i];
                    if (CurrentLocalProc->sel_state) {
                        if (print_proc())
                            num_parsed++;
                    }
                    if (RepeatTime && PrintPass)
                        free_lproc(CurrentLocalProc);
                }
            }
            CurrentLocalFile = saved_lfile;
        }
        /*
         * If a repeat time is set, sleep for the specified time.
         *
         * If conditional repeat mode is in effect, see if it's time to exit.
         */
        if (RepeatTime) {
            if (repeat_cond) {
                if (!num_parsed)
                    break;
                else
                    exit_val = 0;
            }

#if defined(HAS_STRFTIME)
            if (fmt && fmtr) {

                /*
             * Format the marker line.
             */
                util_strftime(fmtr, fmtl - 1, fmt);
                fmtr[fmtl - 1] = '\0';
            }
#endif

            if (OptFieldOutput) {
                putchar(LSOF_FID_MARK);

#if defined(HAS_STRFTIME)
                if (fmtr)
                    printf("%s", fmtr);
#endif

                putchar(Terminator);
                if (Terminator != '\n')
                    putchar('\n');
            } else {

#if defined(HAS_STRFTIME)
                if (fmtr)
                    char_ptr = fmtr;
                else
#endif

                    char_ptr = "=======";
                puts(char_ptr);
            }
            fflush(stdout);
            childx();
            sleep(RepeatTime);
            HeaderPrinted = NumLocalProcs = 0;
            CheckPasswdChange = 1;
        }
    } while (RepeatTime);
    /*
 * See if all requested information was displayed.  Return zero if it
 * was; one, if not.  If -V was specified, report what was not displayed.
 */
    childx();
    return_val = 0;
    for (str_entry = CommandNameList; str_entry; str_entry = str_entry->next) {

        /*
         * Check command specifications.
         */
        if (str_entry->f)
            continue;
        return_val = 1;
        if (OptVerbose) {
            printf("%s: command not located: ", ProgramName);
            safestrprt(str_entry->str, stdout, 1);
        }
    }
    for (i = 0; i < NumCommandRegexUsed; i++) {

        /*
         * Check command regular expressions.
         */
        if (CommandRegexTable[i].mc)
            continue;
        return_val = 1;
        if (OptVerbose) {
            printf("%s: no command found for regex: ", ProgramName);
            safestrprt(CommandRegexTable[i].exp, stdout, 1);
        }
    }
    for (search_file_ptr = SearchFileChain; search_file_ptr;
         search_file_ptr = search_file_ptr->next) {

        /*
         * Check file specifications.
         */
        if (search_file_ptr->f)
            continue;
        return_val = 1;
        if (OptVerbose) {
            printf("%s: no file%s use located: ", ProgramName,
                   search_file_ptr->type ? "" : " system");
            safestrprt(search_file_ptr->aname, stdout, 1);
        }
    }

#if defined(HASPROCFS)
    /*
     * Report on proc file system search results.
     */
    if (ProcFsSearching && !ProcFsFound) {
        return_val = 1;
        if (OptVerbose) {
            printf("%s: no file system use located: ", ProgramName);
            safestrprt(Mtprocfs ? Mtprocfs->dir : HASPROCFS, stdout, 1);
        }
    }
    {
        struct procfsid *pfi;

        for (pfi = ProcFsIdTable; pfi; pfi = pfi->next) {
            if (!pfi->f) {
                return_val = 1;
                if (OptVerbose) {
                    printf("%s: no file use located: ", ProgramName);
                    safestrprt(pfi->nm, stdout, 1);
                }
            }
        }
    }
#endif

    if ((net_addr_ptr = NetworkAddrList)) {

        /*
         * Check Internet address specifications.
         *
         * If any Internet address derived from the same argument was found,
         * consider all derivations found.  If no derivation from the same
         * argument was found, report only the first failure.
         *
         */
        for (; net_addr_ptr; net_addr_ptr = net_addr_ptr->next) {
            if (!(char_ptr = net_addr_ptr->arg))
                continue;
            for (net_addr_next = net_addr_ptr->next; net_addr_next;
                 net_addr_next = net_addr_next->next) {
                if (!net_addr_next->arg)
                    continue;
                if (!strcmp(char_ptr, net_addr_next->arg)) {

                    /*
                     * If either of the duplicate specifications was found,
                     * mark them both found.  If neither was found, mark all
                     * but the first one found.
                     */
                    if (net_addr_ptr->found)
                        net_addr_next->found = net_addr_ptr->found;
                    else if (net_addr_next->found)
                        net_addr_ptr->found = net_addr_next->found;
                    else
                        net_addr_next->found = 1;
                }
            }
        }
        for (net_addr_ptr = NetworkAddrList; net_addr_ptr; net_addr_ptr = net_addr_ptr->next) {
            if (!net_addr_ptr->found && (char_ptr = net_addr_ptr->arg)) {
                return_val = 1;
                if (OptVerbose) {
                    printf("%s: Internet address not located: ", ProgramName);
                    safestrprt(char_ptr ? char_ptr : "(unknown)", stdout, 1);
                }
            }
        }
    }
    if (OptNetwork && OptNetwork < 2) {

        /*
         * Report no Internet files located.
         */
        return_val = 1;
        if (OptVerbose)
            printf("%s: no Internet files located\n", ProgramName);
    }

#if defined(HASTCPUDPSTATE)
    if (TcpStateIncludeCount) {

        /*
     * Check for included TCP states not located.
     */
        for (i = 0; i < TcpNumStates; i++) {
            if (TcpStateInclude[i] == 1) {
                return_val = 1;
                if (OptVerbose)
                    printf("%s: TCP state not located: %s\n", ProgramName, TcpStateNames[i]);
            }
        }
    }
    if (UdpStateIncludeCount) {

        /*
     * Check for included UDP states not located.
     */
        for (i = 0; i < UdpNumStates; i++) {
            if (UdpStateInclude[i] == 1) {
                return_val = 1;
                if (OptVerbose)
                    printf("%s: UDP state not located: %s\n", ProgramName, UdpStateNames[i]);
            }
        }
    }
#endif

    if (OptNfs && OptNfs < 2) {

        /*
         * Report no NFS files located.
         */
        return_val = 1;
        if (OptVerbose)
            printf("%s: no NFS files located\n", ProgramName);
    }
    for (i = 0; i < NumPidSelections; i++) {

        /*
         * Check inclusionary process ID specifications.
         */
        if (SearchPidList[i].f || SearchPidList[i].x)
            continue;
        return_val = 1;
        if (OptVerbose)
            printf("%s: process ID not located: %d\n", ProgramName, SearchPidList[i].i);
    }

#if defined(HASTASKS)
    if (OptTask && OptTask < 2) {

        /*
     * Report no tasks located.
     */
        return_val = 1;
        if (OptVerbose)
            printf("%s: no tasks located\n", ProgramName);
    }
#endif

#if defined(HASZONES)
    if (ZoneArg) {

        /*
     * Check zone argument results.
     */
        for (i = 0; i < HASHZONE; i++) {
            for (zp = ZoneArg[i]; zp; zp = zp->next) {
                if (!zp->f) {
                    return_val = 1;
                    if (OptVerbose) {
                        printf("%s: zone not located: ", ProgramName);
                        safestrprt(zp->zn, stdout, 1);
                    }
                }
            }
        }
    }
#endif

#if defined(HASSELINUX)
    if (ContextArgList) {

        /*
     * Check context argument results.
     */
        for (cntxp = ContextArgList; cntxp; cntxp = cntxp->next) {
            if (!cntxp->f) {
                return_val = 1;
                if (OptVerbose) {
                    printf("%s: context not located: ", ProgramName);
                    safestrprt(cntxp->cntx, stdout, 1);
                }
            }
        }
    }
#endif

    for (i = 0; i < NumPgidSelections; i++) {

        /*
         * Check inclusionary process group ID specifications.
         */
        if (SearchPgidList[i].f || SearchPgidList[i].x)
            continue;
        return_val = 1;
        if (OptVerbose)
            printf("%s: process group ID not located: %d\n", ProgramName, SearchPgidList[i].i);
    }
    for (i = 0; i < NumUidSelections; i++) {

        /*
         * Check inclusionary user ID specifications.
         */
        if (SearchUidList[i].excl || SearchUidList[i].f)
            continue;
        return_val = 1;
        if (OptVerbose) {
            if (SearchUidList[i].lnm) {
                printf("%s: login name (UID %lu) not located: ", ProgramName,
                       (unsigned long)SearchUidList[i].uid);
                safestrprt(SearchUidList[i].lnm, stdout, 1);
            } else
                printf("%s: user ID not located: %lu\n", ProgramName,
                       (unsigned long)SearchUidList[i].uid);
        }
    }
    if (!return_val && repeat_cond)
        return_val = exit_val;
    if (!return_val && PathStatErrorCount)
        return_val = 1;
    Exit(return_val);
    return (return_val); /* to make code analyzers happy */
}

/*
 * GetOpt() -- Local get option
 *
 * Liberally adapted from the public domain AT&T getopt() source,
 * distributed at the 1985 UNIFORM conference in Dallas
 *
 * The modifications allow `?' to be an option character and allow
 * the caller to decide that an option that may be followed by a
 * value doesn't have one -- e.g., has a default instead.
 */

static int GetOpt(int opt_count, char *opt[], char *rules, int *err) {
    register int opt_char;
    register char *rule_ptr = NULL;

    if (GOx2 == 0) {

        /*
         * Move to a new entry of the option array.
         *
         * EOF if:
         *
         *	Option list has been exhausted;
         *	Next option doesn't start with `-' or `+';
         *	Next option has nothing but `-' or `+';
         *	Next option is ``--'' or ``++''.
         */
        if (GOx1 >= opt_count || (opt[GOx1][0] != '-' && opt[GOx1][0] != '+') || !opt[GOx1][1])
            return (EOF);
        if (strcmp(opt[GOx1], "--") == 0 || strcmp(opt[GOx1], "++") == 0) {
            GOx1++;
            return (EOF);
        }
        GOp = opt[GOx1][0];
        GOx2 = 1;
    }
    /*
 * Flag `:' option character as an error.
 *
 * Check for a rule on this option character.
 */
    *err = 0;
    if ((opt_char = opt[GOx1][GOx2]) == ':') {
        fprintf(stderr, "%s: colon is an illegal option character.\n", ProgramName);
        *err = 1;
    } else if (!(rule_ptr = strchr(rules, opt_char))) {
        fprintf(stderr, "%s: illegal option character: %c\n", ProgramName, opt_char);
        *err = 2;
    }
    if (*err) {

        /*
         * An error was detected.
         *
         * Advance to the next option character.
         *
         * Return the character causing the error.
         */
        if (opt[GOx1][++GOx2] == '\0') {
            GOx1++;
            GOx2 = 0;
        }
        return (opt_char);
    }
    if (*(rule_ptr + 1) == ':') {

        /*
         * The option may have a following value.  The caller decides
         * if it does.
         *
         * Save the position of the possible value in case the caller
         * decides it does not belong to the option and wants it
         * reconsidered as an option character.  The caller does that
         * with:
         *		GOx1 = GObk[0]; GOx2 = GObk[1];
         *
         * Don't indicate that an option of ``--'' is a possible value.
         *
         * Finally, on the assumption that the caller will decide that
         * the possible value belongs to the option, position to the
         * option following the possible value, so that the next call
         * to GetOpt() will find it.
         */
        if (opt[GOx1][GOx2 + 1] != '\0') {
            GObk[0] = GOx1;
            GObk[1] = ++GOx2;
            GOv = &opt[GOx1++][GOx2];
        } else if (++GOx1 >= opt_count)
            GOv = NULL;
        else {
            GObk[0] = GOx1;
            GObk[1] = 0;
            GOv = opt[GOx1];
            if (strcmp(GOv, "--") == 0)
                GOv = NULL;
            else
                GOx1++;
        }
        GOx2 = 0;
    } else {

        /*
         * The option character stands alone with no following value.
         *
         * Advance to the next option character.
         */
        if (opt[GOx1][++GOx2] == '\0') {
            GOx2 = 0;
            GOx1++;
        }
        GOv = NULL;
    }
    /*
 * Return the option character.
 */
    return (opt_char);
}

/*
 * sv_fmt_str() - save format string
 */

static char *sv_fmt_str(char *fmt_str) {
    char *result;
    MALLOC_S len;

    len = (MALLOC_S)(strlen(fmt_str) + 1);
    if (!(result = (char *)malloc(len))) {
        fprintf(stderr, "%s: can't allocate %d bytes for format: %s\n", ProgramName, (int)len,
                fmt_str);
        Exit(1);
    }
    snpf(result, len, "%s", fmt_str);
    return (result);
}
