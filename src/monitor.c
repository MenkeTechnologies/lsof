/*
 * monitor.c -- live full-screen refresh mode for lsofng
 *
 * Uses ANSI alternate screen buffer for flicker-free updates,
 * similar to top(1).  Activated with --monitor or -W.
 */

#include "lsof.h"

#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

static int monitor_active = 0; /* nonzero after monitor_enter() */

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
 * Signal handlers
 */
static void monitor_sigwinch_handler(int sig) {
    (void)sig;
    monitor_query_size();
}

static void monitor_signal_exit_handler(int sig) {
    (void)sig;
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
}

/*
 * monitor_leave() -- restore original screen buffer
 *
 * Safe to call multiple times.  The signal-handler path uses the
 * raw write helper; the normal path uses printf for consistency.
 */
void monitor_leave(void) {
    if (!monitor_active)
        return;
    printf("\033[?25h");   /* show cursor */
    printf("\033[?1049l"); /* leave alternate screen buffer */
    fflush(stdout);
    monitor_active = 0;
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

    monitor_query_size(); /* pick up any resize */

    printf("\033[H");  /* cursor to home (0,0) */
    printf("\033[J");  /* clear from cursor to end of screen */

    /* Timestamp */
    now = time(NULL);
    tm_info = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Status bar */
    if (CyberpunkTTY) {
        printf("%s%s╔", CP_HDR_BG, CP_NEON_MAGENTA);
        for (int i = 0; i < MonitorTermCols - 2 && i < 200; i++)
            printf("═");
        printf("╗%s\n", CP_RESET);

        printf("%s%s║%s MONITOR %s│%s %s %s│%s %d open files %s│%s refresh: %ds %s│%s q/Ctrl-C to exit %s",
               CP_HDR_BG, CP_NEON_MAGENTA,
               CP_NEON_CYAN, CP_NEON_MAGENTA,
               CP_NEON_GREEN, timebuf, CP_NEON_MAGENTA,
               CP_NEON_YELLOW, count, CP_NEON_MAGENTA,
               CP_DIM, interval, CP_NEON_MAGENTA,
               CP_DIM, CP_RESET);
        /* Pad to edge */
        printf("\n");

        printf("%s%s╚", CP_HDR_BG, CP_NEON_MAGENTA);
        for (int i = 0; i < MonitorTermCols - 2 && i < 200; i++)
            printf("═");
        printf("╝%s\n", CP_RESET);

        status_lines = 3;
    } else {
        printf("[ MONITOR ] %s | %d open files | refresh: %ds | q/Ctrl-C to exit\n",
               timebuf, count, interval);
        for (int i = 0; i < MonitorTermCols && i < 200; i++)
            printf("-");
        printf("\n");
        status_lines = 2;
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
