/*
 * monitor.c -- live full-screen refresh mode for lsofng
 *
 * Uses ANSI alternate screen buffer for flicker-free updates,
 * similar to top(1).  Activated with --monitor or -W.
 *
 * Interactive controls:
 *   s/S    - cycle sort column (PID, COMMAND, USER, FD count)
 *   f/F    - enter type filter (e.g. "REG", "SOCK", empty to clear)
 *   /      - search/highlight by name substring
 *   p/P    - pause/unpause refresh
 *   r/R    - reverse sort order
 *   ?/h    - toggle help bar
 *   q/Q    - quit
 */

#include "lsof.h"

#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <poll.h>
#include <ctype.h>

static int monitor_active = 0; /* nonzero after monitor_enter() */
static struct termios monitor_orig_term; /* saved terminal settings */
static int monitor_raw_mode = 0;        /* raw mode active */

/*
 * Sort modes
 */
#define MONITOR_SORT_PID   0
#define MONITOR_SORT_CMD   1
#define MONITOR_SORT_USER  2
#define MONITOR_SORT_FDS   3
#define MONITOR_SORT_COUNT 4

static const char *sort_names[] = {"PID", "COMMAND", "USER", "FDs"};

/*
 * Raw write helpers -- async-signal-safe (use write(2), not stdio).
 */
static void monitor_raw_write(const char *s) {
    size_t len = 0;
    const char *p = s;
    while (*p++) len++;
    (void)write(STDOUT_FILENO, s, len);
}

/*
 * monitor_query_size() -- read terminal dimensions via ioctl
 */
static void monitor_query_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_row > 0)
            MonitorTermRows = ws.ws_row;
        if (ws.ws_col > 0)
            MonitorTermCols = ws.ws_col;
    }
}

/*
 * Terminal raw mode
 */
static void monitor_raw_enter(void) {
    struct termios raw;
    if (monitor_raw_mode)
        return;
    if (tcgetattr(STDIN_FILENO, &monitor_orig_term) < 0)
        return;
    raw = monitor_orig_term;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
        monitor_raw_mode = 1;
}

static void monitor_raw_leave(void) {
    if (!monitor_raw_mode)
        return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &monitor_orig_term);
    monitor_raw_mode = 0;
}

/*
 * Signal handlers
 */
static void monitor_sigwinch_handler(int sig) {
    (void)sig;
    monitor_query_size();
}

static void monitor_signal_exit_handler(int sig) {
    (void)sig;
    if (monitor_raw_mode) {
        tcsetattr(STDIN_FILENO, TCSANOW, &monitor_orig_term);
        monitor_raw_mode = 0;
    }
    if (monitor_active) {
        monitor_raw_write("\033[?25h");   /* show cursor */
        monitor_raw_write("\033[?1049l"); /* leave alt screen */
        monitor_active = 0;
    }
    _exit(0);
}

/*
 * monitor_init() -- query terminal size and install signal handlers
 */
void monitor_init(void) {
    struct sigaction sa;

    monitor_query_size();

    /* SIGWINCH: re-read terminal size on resize */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = monitor_sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);

    /* SIGINT / SIGTERM: clean exit restoring terminal */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = monitor_signal_exit_handler;
    sa.sa_flags = SA_RESETHAND; /* one-shot: re-raises with default */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/*
 * monitor_enter() -- switch to alternate screen buffer, hide cursor
 */
void monitor_enter(void) {
    if (monitor_active)
        return;
    printf("\033[?1049h"); /* enter alternate screen buffer */
    printf("\033[?25l");   /* hide cursor */
    fflush(stdout);
    monitor_active = 1;
    monitor_raw_enter();
}

/*
 * monitor_leave() -- restore original screen buffer
 */
void monitor_leave(void) {
    monitor_raw_leave();
    if (!monitor_active)
        return;
    printf("\033[?25h");   /* show cursor */
    printf("\033[?1049l"); /* leave alternate screen buffer */
    fflush(stdout);
    monitor_active = 0;
}

/*
 * Read a short string from user in the status bar area.
 * Shows cursor, echoes typed characters, returns on Enter.
 * ESC cancels. Returns 1 on success, 0 on cancel.
 */
static int monitor_read_line(const char *prompt, char *buf, int bufsz) {
    int pos = 0;
    struct pollfd pfd;

    buf[0] = '\0';

    /* Show cursor temporarily */
    printf("\033[?25h");

    /* Move to bottom line and show prompt */
    printf("\033[%d;1H\033[K", MonitorTermRows);
    if (CyberpunkTTY)
        printf("%s%s %s%s ", CP_HDR_BG, CP_NEON_CYAN, prompt, CP_RESET);
    else
        printf(" %s ", prompt);
    fflush(stdout);

    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    for (;;) {
        if (poll(&pfd, 1, -1) <= 0)
            continue;
        char ch;
        if (read(STDIN_FILENO, &ch, 1) != 1)
            continue;

        if (ch == '\n' || ch == '\r') {
            buf[pos] = '\0';
            printf("\033[?25l");
            fflush(stdout);
            return 1;
        }
        if (ch == 27) { /* ESC */
            buf[0] = '\0';
            printf("\033[?25l");
            fflush(stdout);
            return 0;
        }
        if (ch == 127 || ch == '\b') { /* backspace */
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }
        if (pos < bufsz - 1 && ch >= 32) {
            buf[pos++] = ch;
            buf[pos] = '\0';
            fputc(ch, stdout);
            fflush(stdout);
        }
    }
}

/*
 * monitor_begin_frame() -- start a new display frame
 *
 * Moves cursor to home, clears screen, prints a status bar,
 * and sets MonitorRowsRemaining for row limiting in print_file().
 */
void monitor_begin_frame(int count, int interval) {
    time_t now;
    struct tm *tm_info;
    char timebuf[32];
    int status_lines;
    const char *pause_str;
    const char *sort_str;

    monitor_query_size(); /* pick up any resize */

    printf("\033[H");  /* cursor to home (0,0) */
    printf("\033[J");  /* clear from cursor to end of screen */

    /* Timestamp */
    now = time(NULL);
    tm_info = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

    pause_str = MonitorPaused ? " [PAUSED]" : "";
    sort_str = sort_names[MonitorSortMode % MONITOR_SORT_COUNT];

    /* Status bar */
    if (CyberpunkTTY) {
        printf("%s%s╔", CP_HDR_BG, CP_NEON_MAGENTA);
        for (int i = 0; i < MonitorTermCols - 2 && i < 200; i++)
            printf("═");
        printf("╗%s\n", CP_RESET);

        printf("%s%s║%s MONITOR %s│%s %s %s│%s %d files %s│%s sort: %s %s%s│%s %ds%s%s %s",
               CP_HDR_BG, CP_NEON_MAGENTA,
               CP_NEON_CYAN, CP_NEON_MAGENTA,
               CP_NEON_GREEN, timebuf, CP_NEON_MAGENTA,
               CP_NEON_YELLOW, count, CP_NEON_MAGENTA,
               CP_NEON_CYAN, sort_str,
               MonitorSortReverse ? "(rev) " : "",
               CP_NEON_MAGENTA,
               CP_DIM, interval,
               pause_str,
               CP_NEON_MAGENTA,
               CP_RESET);
        printf("\n");

        printf("%s%s╚", CP_HDR_BG, CP_NEON_MAGENTA);
        for (int i = 0; i < MonitorTermCols - 2 && i < 200; i++)
            printf("═");
        printf("╝%s\n", CP_RESET);

        status_lines = 3;
    } else {
        printf("[ MONITOR ] %s | %d files | sort: %s%s | refresh: %ds%s\n",
               timebuf, count, sort_str,
               MonitorSortReverse ? " (rev)" : "",
               interval, pause_str);
        for (int i = 0; i < MonitorTermCols && i < 200; i++)
            printf("-");
        printf("\n");
        status_lines = 2;
    }

    /* Filter/search status */
    if (MonitorTypeFilter[0]) {
        if (CyberpunkTTY)
            printf(" %sfilter: type=%s%s%s\n", CP_DIM, CP_NEON_GREEN,
                   MonitorTypeFilter, CP_RESET);
        else
            printf(" filter: type=%s\n", MonitorTypeFilter);
        status_lines++;
    }
    if (MonitorSearchStr[0]) {
        if (CyberpunkTTY)
            printf(" %ssearch: %s%s%s\n", CP_DIM, CP_NEON_YELLOW,
                   MonitorSearchStr, CP_RESET);
        else
            printf(" search: %s\n", MonitorSearchStr);
        status_lines++;
    }

    /* Help bar */
    if (MonitorShowHelp) {
        if (CyberpunkTTY)
            printf(" %ss%s:sort %sr%s:reverse %sf%s:filter %s/%s:search %sp%s:pause %s?%s:help %sq%s:quit\n",
                   CP_NEON_CYAN, CP_RESET, CP_NEON_CYAN, CP_RESET,
                   CP_NEON_CYAN, CP_RESET, CP_NEON_CYAN, CP_RESET,
                   CP_NEON_CYAN, CP_RESET, CP_NEON_CYAN, CP_RESET,
                   CP_NEON_CYAN, CP_RESET);
        else
            printf(" s:sort r:reverse f:filter /:search p:pause ?:help q:quit\n");
        status_lines++;
    }

    /*
     * Set row budget for print_file().
     * Reserve status_lines at top.  The column header takes ~2 lines
     * (header row + separator).  Leave 1 line at bottom for truncation msg.
     */
    MonitorRowsRemaining = MonitorTermRows - status_lines - 1;
    if (MonitorRowsRemaining < 3)
        MonitorRowsRemaining = 3;
}

/*
 * monitor_sort_procs() -- sort the process table per current sort mode
 */
void monitor_sort_procs(struct lproc **sorted, int count) {
    int sort_mode = MonitorSortMode % MONITOR_SORT_COUNT;
    int rev = MonitorSortReverse;
    int i;

    /* Count FDs per process for FD sort */
    if (sort_mode == MONITOR_SORT_FDS) {
        /* We'll use a comparison that counts on the fly */
    }

    /* Custom sort via qsort with globals */
    /* We use a simple insertion sort approach for flexibility,
     * or better: create comparison function per mode */

    /* Actually use qsort with a unified comparator */
    typedef struct {
        struct lproc *proc;
        int fd_count;
    } sort_entry_t;

    sort_entry_t *entries = (sort_entry_t *)malloc(count * sizeof(sort_entry_t));
    if (!entries) return;

    for (i = 0; i < count; i++) {
        entries[i].proc = sorted[i];
        entries[i].fd_count = 0;
        if (sort_mode == MONITOR_SORT_FDS) {
            struct lfile *lf;
            for (lf = sorted[i]->file; lf; lf = lf->next)
                entries[i].fd_count++;
        }
    }

    /* Sort using shell sort to avoid qsort comparator complexity */
    int gap, j;
    for (gap = count / 2; gap > 0; gap /= 2) {
        for (i = gap; i < count; i++) {
            sort_entry_t tmp = entries[i];
            for (j = i; j >= gap; j -= gap) {
                int cmp = 0;
                switch (sort_mode) {
                case MONITOR_SORT_PID:
                    cmp = entries[j - gap].proc->pid - tmp.proc->pid;
                    break;
                case MONITOR_SORT_CMD:
                    if (entries[j - gap].proc->cmd && tmp.proc->cmd)
                        cmp = strcasecmp(entries[j - gap].proc->cmd, tmp.proc->cmd);
                    else
                        cmp = entries[j - gap].proc->cmd ? 1 : -1;
                    break;
                case MONITOR_SORT_USER:
                    cmp = (int)entries[j - gap].proc->uid - (int)tmp.proc->uid;
                    break;
                case MONITOR_SORT_FDS:
                    cmp = entries[j - gap].fd_count - tmp.fd_count;
                    break;
                }
                if (rev) cmp = -cmp;
                if (cmp <= 0)
                    break;
                entries[j] = entries[j - gap];
            }
            entries[j] = tmp;
        }
    }

    /* Write back */
    for (i = 0; i < count; i++)
        sorted[i] = entries[i].proc;

    free(entries);
}

/*
 * monitor_filter_match() -- check if a process matches current filter/search
 */
int monitor_filter_match(struct lproc *proc) {
    if (MonitorTypeFilter[0]) {
        /* Check if any file in this process matches the type filter */
        struct lfile *lf;
        int found = 0;
        for (lf = proc->file; lf; lf = lf->next) {
            if (strcasecmp(lf->type, MonitorTypeFilter) == 0) {
                found = 1;
                break;
            }
        }
        if (!found)
            return 0;
    }
    return 1;
}

/*
 * monitor_sleep() -- interruptible sleep that handles keypresses
 *
 * Returns: 0 = normal timeout, 1 = quit requested
 */
int monitor_sleep(int seconds) {
    struct pollfd pfd;
    int ms_remaining;
    char ch;
    char buf[64];

    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    ms_remaining = seconds * 1000;

    while (ms_remaining > 0) {
        int chunk = ms_remaining > 100 ? 100 : ms_remaining;
        int ret = poll(&pfd, 1, MonitorPaused ? 100 : chunk);

        if (!MonitorPaused)
            ms_remaining -= chunk;

        if (ret > 0 && (pfd.revents & POLLIN)) {
            if (read(STDIN_FILENO, &ch, 1) == 1) {
                switch (ch) {
                case 'q':
                case 'Q':
                    return 1; /* quit */

                case 's':
                case 'S':
                    MonitorSortMode = (MonitorSortMode + 1) % MONITOR_SORT_COUNT;
                    return 0; /* re-render immediately */

                case 'r':
                case 'R':
                    MonitorSortReverse = !MonitorSortReverse;
                    return 0;

                case 'p':
                case 'P':
                    MonitorPaused = !MonitorPaused;
                    /* Show pause status immediately */
                    printf("\033[%d;1H\033[K", MonitorTermRows);
                    if (MonitorPaused) {
                        if (CyberpunkTTY)
                            printf(" %s%s[PAUSED]%s press p to resume",
                                   CP_BOLD, CP_NEON_RED, CP_RESET);
                        else
                            printf(" [PAUSED] press p to resume");
                    }
                    fflush(stdout);
                    if (MonitorPaused)
                        ms_remaining = seconds * 1000; /* reset timer */
                    else
                        return 0; /* unpause: refresh now */
                    break;

                case 'f':
                case 'F':
                    if (monitor_read_line("Type filter (empty=clear):", buf, sizeof(buf))) {
                        strncpy(MonitorTypeFilter, buf, sizeof(MonitorTypeFilter) - 1);
                        MonitorTypeFilter[sizeof(MonitorTypeFilter) - 1] = '\0';
                    }
                    return 0;

                case '/':
                    if (monitor_read_line("Search name:", buf, sizeof(buf))) {
                        strncpy(MonitorSearchStr, buf, sizeof(MonitorSearchStr) - 1);
                        MonitorSearchStr[sizeof(MonitorSearchStr) - 1] = '\0';
                    }
                    return 0;

                case '?':
                case 'h':
                case 'H':
                    MonitorShowHelp = !MonitorShowHelp;
                    return 0;

                case 3: /* Ctrl-C */
                    return 1;
                }
            }
        }
    }
    return MonitorPaused ? 0 : 0;
}
