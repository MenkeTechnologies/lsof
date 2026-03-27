/*
 * follow.c -- continuously watch a single process's FDs (--follow PID)
 *
 * Highlights opens (new FDs) and closes (gone FDs) in real-time,
 * with color-coded status: green=new, red=closed, dim=unchanged.
 * Runs in the alternate screen buffer with auto-refresh.
 */

#include "lsof.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

/*
 * Tracked file descriptor entry
 */
#define FOLLOW_HASH_SIZE 512

struct follow_fd {
    char fd[FDLEN];
    char type[TYPEL];
    char *name;
    int seen;           /* marked each iteration */
    int status;         /* 0=existing, 1=new, 2=gone */
    struct follow_fd *next;
};

static struct follow_fd *follow_table[FOLLOW_HASH_SIZE];
static int follow_initialized = 0;
static int follow_total = 0;
static int follow_new_count = 0;
static int follow_gone_count = 0;
static int follow_iteration = 0;

/*
 * Simple hash for fd string
 */
static unsigned int follow_hash(const char *s) {
    unsigned int h = 0;
    while (*s)
        h = h * 31 + (unsigned char)*s++;
    return h % FOLLOW_HASH_SIZE;
}

/*
 * follow_collect() -- scan the process table for the target PID
 *                     and update the tracked FD table
 */
void follow_collect(void) {
    int i;
    struct lproc *target = NULL;
    struct lfile *lf;
    struct follow_fd *fe, *prev;
    unsigned int h;

    follow_new_count = 0;
    follow_gone_count = 0;
    follow_iteration++;

    /* Find the target process */
    for (i = 0; i < NumLocalProcs; i++) {
        if (LocalProcTable[i].pid == FollowTargetPid) {
            target = &LocalProcTable[i];
            break;
        }
    }

    /* Mark all existing entries as not-seen */
    for (i = 0; i < FOLLOW_HASH_SIZE; i++) {
        for (fe = follow_table[i]; fe; fe = fe->next)
            fe->seen = 0;
    }

    if (target) {
        /* Walk the file list and update tracking table */
        for (lf = target->file; lf; lf = lf->next) {
            h = follow_hash(lf->fd);
            for (fe = follow_table[h]; fe; fe = fe->next) {
                if (strcmp(fe->fd, lf->fd) == 0)
                    break;
            }

            if (fe) {
                /* Existing FD -- update and mark seen */
                fe->seen = 1;
                if (fe->status == 2) {
                    /* Was gone, now back -- treat as new */
                    fe->status = 1;
                    follow_new_count++;
                } else if (fe->status == 1 && follow_initialized) {
                    /* Was new last iteration, now existing */
                    fe->status = 0;
                }
                /* Update name if changed */
                if (lf->name) {
                    if (!fe->name || strcmp(fe->name, lf->name) != 0) {
                        if (fe->name) free(fe->name);
                        fe->name = strdup(lf->name);
                    }
                }
                /* Update type */
                strncpy(fe->type, lf->type, TYPEL - 1);
                fe->type[TYPEL - 1] = '\0';
            } else {
                /* New FD */
                fe = (struct follow_fd *)malloc(sizeof(struct follow_fd));
                if (!fe) continue;
                memset(fe, 0, sizeof(*fe));
                strncpy(fe->fd, lf->fd, FDLEN - 1);
                fe->fd[FDLEN - 1] = '\0';
                strncpy(fe->type, lf->type, TYPEL - 1);
                fe->type[TYPEL - 1] = '\0';
                fe->name = lf->name ? strdup(lf->name) : NULL;
                fe->seen = 1;
                fe->status = follow_initialized ? 1 : 0;
                fe->next = follow_table[h];
                follow_table[h] = fe;
                if (follow_initialized)
                    follow_new_count++;
            }
        }
    }

    /* Mark unseen entries as gone */
    for (i = 0; i < FOLLOW_HASH_SIZE; i++) {
        for (fe = follow_table[i]; fe; fe = fe->next) {
            if (!fe->seen && fe->status != 2) {
                fe->status = 2;
                follow_gone_count++;
            }
        }
    }

    /* Count totals */
    follow_total = 0;
    for (i = 0; i < FOLLOW_HASH_SIZE; i++) {
        for (fe = follow_table[i]; fe; fe = fe->next) {
            if (fe->status != 2)
                follow_total++;
        }
    }

    /* Remove entries that have been gone for more than 1 iteration */
    for (i = 0; i < FOLLOW_HASH_SIZE; i++) {
        prev = NULL;
        fe = follow_table[i];
        while (fe) {
            struct follow_fd *next = fe->next;
            /* Keep gone entries for one more display, then remove */
            if (fe->status == 2 && !fe->seen) {
                /* Will be displayed this iteration, removed next */
            }
            prev = fe;
            fe = next;
        }
    }

    follow_initialized = 1;
}

/*
 * Comparison for sorted display
 */
static int follow_cmp(const void *a, const void *b) {
    const struct follow_fd *fa = *(const struct follow_fd *const *)a;
    const struct follow_fd *fb = *(const struct follow_fd *const *)b;
    /* Gone entries at bottom, new at top, then by fd */
    if (fa->status != fb->status) {
        if (fa->status == 1) return -1;
        if (fb->status == 1) return 1;
        if (fa->status == 2) return 1;
        if (fb->status == 2) return -1;
    }
    return strcmp(fa->fd, fb->fd);
}

/*
 * follow_report() -- display the tracked FDs with status highlighting
 */
void follow_report(void) {
    int i, count, row;
    struct follow_fd *fe;
    struct follow_fd **sorted;
    time_t now;
    struct tm *tm_info;
    char timebuf[32];
    int max_rows;
    const char *status_mark;
    const char *color_start;

    now = time(NULL);
    tm_info = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Move to home, clear */
    printf("\033[H\033[J");

    /* Header */
    if (CyberpunkTTY) {
        printf("%s%s╔", CP_HDR_BG, CP_NEON_MAGENTA);
        for (i = 0; i < MonitorTermCols - 2 && i < 200; i++)
            printf("═");
        printf("╗%s\n", CP_RESET);

        printf("%s%s║%s FOLLOW PID %d %s│%s %s %s│%s %d open FDs %s│ %s+%d %s-%d %s│%s q to exit %s",
               CP_HDR_BG, CP_NEON_MAGENTA,
               CP_NEON_CYAN, FollowTargetPid, CP_NEON_MAGENTA,
               CP_NEON_GREEN, timebuf, CP_NEON_MAGENTA,
               CP_NEON_YELLOW, follow_total, CP_NEON_MAGENTA,
               CP_NEON_GREEN, follow_new_count,
               CP_NEON_RED, follow_gone_count, CP_NEON_MAGENTA,
               CP_DIM, CP_RESET);
        printf("\n");

        printf("%s%s╚", CP_HDR_BG, CP_NEON_MAGENTA);
        for (i = 0; i < MonitorTermCols - 2 && i < 200; i++)
            printf("═");
        printf("╝%s\n", CP_RESET);
    } else {
        printf("[ FOLLOW PID %d ] %s | %d open FDs | +%d -%d | q to exit\n",
               FollowTargetPid, timebuf, follow_total,
               follow_new_count, follow_gone_count);
        for (i = 0; i < MonitorTermCols && i < 200; i++)
            printf("-");
        printf("\n");
    }

    /* Collect all entries into a sorted array */
    count = 0;
    for (i = 0; i < FOLLOW_HASH_SIZE; i++)
        for (fe = follow_table[i]; fe; fe = fe->next)
            count++;

    if (count == 0) {
        if (CyberpunkTTY)
            printf("\n  %sProcess %d not found or has no open files%s\n",
                   CP_NEON_RED, FollowTargetPid, CP_RESET);
        else
            printf("\n  Process %d not found or has no open files\n",
                   FollowTargetPid);
        fflush(stdout);
        return;
    }

    sorted = (struct follow_fd **)malloc(count * sizeof(struct follow_fd *));
    if (!sorted) return;

    count = 0;
    for (i = 0; i < FOLLOW_HASH_SIZE; i++)
        for (fe = follow_table[i]; fe; fe = fe->next)
            sorted[count++] = fe;

    qsort(sorted, count, sizeof(struct follow_fd *), follow_cmp);

    /* Column header */
    if (CyberpunkTTY)
        printf("  %s%s%-4s %-6s %-8s %s%s\n",
               CP_HDR_BG, CP_NEON_YELLOW, "ST", "FD", "TYPE", "NAME", CP_RESET);
    else
        printf("  %-4s %-6s %-8s %s\n", "ST", "FD", "TYPE", "NAME");

    /* Display entries */
    max_rows = MonitorTermRows - (CyberpunkTTY ? 5 : 4);
    for (row = 0; row < count && row < max_rows; row++) {
        fe = sorted[row];

        switch (fe->status) {
        case 1:  /* new */
            status_mark = "+NEW";
            color_start = CP_NEON_GREEN;
            break;
        case 2:  /* gone */
            status_mark = "-DEL";
            color_start = CP_NEON_RED;
            break;
        default: /* existing */
            status_mark = "    ";
            color_start = CP_DIM;
            break;
        }

        if (CyberpunkTTY) {
            printf("  %s%-4s %-6s %-8s %.*s%s\n",
                   color_start, status_mark, fe->fd, fe->type,
                   MonitorTermCols - 24 > 0 ? MonitorTermCols - 24 : 40,
                   fe->name ? fe->name : "",
                   CP_RESET);
        } else {
            printf("  %-4s %-6s %-8s %.*s\n",
                   status_mark, fe->fd, fe->type,
                   MonitorTermCols - 24 > 0 ? MonitorTermCols - 24 : 40,
                   fe->name ? fe->name : "");
        }
    }

    if (row < count)
        printf("  ... %d more entries ...\n", count - row);

    free(sorted);
    fflush(stdout);

    /* Purge gone entries after display */
    for (i = 0; i < FOLLOW_HASH_SIZE; i++) {
        struct follow_fd *prev_fe = NULL;
        fe = follow_table[i];
        while (fe) {
            struct follow_fd *next = fe->next;
            if (fe->status == 2) {
                if (prev_fe)
                    prev_fe->next = next;
                else
                    follow_table[i] = next;
                if (fe->name) free(fe->name);
                free(fe);
            } else {
                /* Reset new status for next iteration */
                if (fe->status == 1)
                    fe->status = 0;
                prev_fe = fe;
            }
            fe = next;
        }
    }
}

/*
 * follow_cleanup() -- free all tracked entries
 */
void follow_cleanup(void) {
    int i;
    struct follow_fd *fe, *next;

    for (i = 0; i < FOLLOW_HASH_SIZE; i++) {
        for (fe = follow_table[i]; fe; fe = next) {
            next = fe->next;
            if (fe->name) free(fe->name);
            free(fe);
        }
        follow_table[i] = NULL;
    }
    follow_initialized = 0;
    follow_total = 0;
    follow_iteration = 0;
}
