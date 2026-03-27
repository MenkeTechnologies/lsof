/*
 * leak.c -- FD leak detection module for lsofng
 *
 * Tracks per-process file descriptor counts over time and flags
 * processes with monotonically increasing FD counts as potential leakers.
 */

#include "lsof.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ANSI_CYAN    "\033[1;36m"
#define ANSI_MAGENTA "\033[1;35m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"
#define ANSI_RESET   "\033[0m"

#define LEAK_HASH_SIZE  1024
#define LEAK_HISTORY    16

struct leak_sample {
    int fd_count;
    time_t timestamp;
};

struct leak_entry {
    int pid;
    char cmd[CMDL + 1];
    uid_t uid;
    struct leak_sample history[LEAK_HISTORY];
    int history_count;
    int history_index;
    int consecutive_increases;
    int flagged;
    int seen;
    struct leak_entry *next;
};

static struct leak_entry *leak_table[LEAK_HASH_SIZE];
static int leak_iteration = 0;

static unsigned int leak_hash(int pid) {
    return ((unsigned int)(pid * 31415) >> 3) & (LEAK_HASH_SIZE - 1);
}

/*
 * leak_detect_init() -- zero the hash table
 */
void leak_detect_init(void) {
    memset(leak_table, 0, sizeof(leak_table));
    leak_iteration = 0;
}

/*
 * leak_find_or_create() -- lookup or insert entry into hash table
 */
static struct leak_entry *leak_find_or_create(int pid, const char *cmd,
                                              uid_t uid) {
    unsigned int h = leak_hash(pid);
    struct leak_entry *e;

    for (e = leak_table[h]; e; e = e->next) {
        if (e->pid == pid) {
            /* If command changed, PID was reused -- reset entry */
            if (strncmp(e->cmd, cmd, CMDL) != 0) {
                memset(e->history, 0, sizeof(e->history));
                e->history_count = 0;
                e->history_index = 0;
                e->consecutive_increases = 0;
                e->flagged = 0;
                strncpy(e->cmd, cmd, CMDL);
                e->cmd[CMDL] = '\0';
                e->uid = uid;
            }
            return e;
        }
    }

    /* Not found -- allocate new entry */
    e = (struct leak_entry *)malloc((MALLOC_S)sizeof(struct leak_entry));
    if (!e) {
        fprintf(stderr, "%s: no space for leak_entry\n", ProgramName);
        return NULL;
    }
    memset(e, 0, sizeof(*e));
    e->pid = pid;
    strncpy(e->cmd, cmd, CMDL);
    e->cmd[CMDL] = '\0';
    e->uid = uid;
    e->next = leak_table[h];
    leak_table[h] = e;
    return e;
}

/*
 * leak_detect_record() -- record an FD count sample for a process
 */
void leak_detect_record(int pid, const char *cmd, uid_t uid, int fd_count) {
    struct leak_entry *e = leak_find_or_create(pid, cmd, uid);

    if (!e)
        return;

    e->seen = 1;

    /* Check if FD count increased compared to previous sample */
    if (e->history_count > 0) {
        int prev_idx = (e->history_index + LEAK_HISTORY - 1) % LEAK_HISTORY;
        if (fd_count > e->history[prev_idx].fd_count)
            e->consecutive_increases++;
        else
            e->consecutive_increases = 0;
    }

    /* Store sample in circular buffer */
    e->history[e->history_index].fd_count = fd_count;
    e->history[e->history_index].timestamp = time(NULL);
    e->history_index = (e->history_index + 1) % LEAK_HISTORY;
    if (e->history_count < LEAK_HISTORY)
        e->history_count++;

    /* Flag if threshold reached */
    if (e->consecutive_increases >= LeakDetectThreshold && !e->flagged)
        e->flagged = 1;
}

/*
 * leak_detect_update() -- called after gather_proc_info() to scan processes
 */
void leak_detect_update(void) {
    int i;
    unsigned int h;
    struct leak_entry *e;

    leak_iteration++;

    /* Clear all "seen" flags */
    for (h = 0; h < LEAK_HASH_SIZE; h++) {
        for (e = leak_table[h]; e; e = e->next)
            e->seen = 0;
    }

    /* Walk LocalProcTable and count FDs per process */
    for (i = 0; i < NumLocalProcs; i++) {
        struct lproc *p = &LocalProcTable[i];
        struct lfile *f;
        int fd_count = 0;

        for (f = p->file; f; f = f->next)
            fd_count++;

        leak_detect_record(p->pid, p->cmd ? p->cmd : "?", p->uid, fd_count);
    }

    /* Sweep: reset consecutive_increases for unseen entries */
    for (h = 0; h < LEAK_HASH_SIZE; h++) {
        for (e = leak_table[h]; e; e = e->next) {
            if (!e->seen)
                e->consecutive_increases = 0;
        }
    }
}

/*
 * leak_detect_report() -- print leak detection report
 */
void leak_detect_report(void) {
    unsigned int h;
    struct leak_entry *e;
    int suspects = 0;
    int scanned = 0;
    int use_color = CyberpunkTTY;

    /* Count suspects and scanned processes */
    for (h = 0; h < LEAK_HASH_SIZE; h++) {
        for (e = leak_table[h]; e; e = e->next) {
            if (e->seen)
                scanned++;
            if (e->flagged)
                suspects++;
        }
    }

    if (suspects > 0) {
        /* Print header */
        if (use_color) {
            fprintf(stdout,
                    "\n" ANSI_CYAN "=== LEAK DETECTION REPORT ===" ANSI_RESET "\n\n");
            fprintf(stdout,
                    ANSI_BOLD "%-8s %-16s %-8s %-8s %s" ANSI_RESET "\n",
                    "PID", "COMMAND", "UID", "SAMPLES", "FD TREND");
        } else {
            fprintf(stdout, "\n=== LEAK DETECTION REPORT ===\n\n");
            fprintf(stdout, "%-8s %-16s %-8s %-8s %s\n",
                    "PID", "COMMAND", "UID", "SAMPLES", "FD TREND");
        }

        if (use_color)
            fprintf(stdout, ANSI_DIM
                    "-------- ---------------- -------- -------- "
                    "----------------" ANSI_RESET "\n");
        else
            fprintf(stdout,
                    "-------- ---------------- -------- -------- "
                    "----------------\n");

        /* Print flagged entries */
        for (h = 0; h < LEAK_HASH_SIZE; h++) {
            for (e = leak_table[h]; e; e = e->next) {
                if (!e->flagged)
                    continue;

                /* Build FD trend string from history */
                char trend[256];
                int pos = 0;
                int j, start;

                if (e->history_count < LEAK_HISTORY)
                    start = 0;
                else
                    start = e->history_index;

                for (j = 0; j < e->history_count && pos < (int)sizeof(trend) - 8; j++) {
                    int idx = (start + j) % LEAK_HISTORY;
                    if (j > 0 && pos < (int)sizeof(trend) - 4) {
                        trend[pos++] = '-';
                        trend[pos++] = '>';
                    }
                    pos += snprintf(trend + pos, sizeof(trend) - pos,
                                    "%d", e->history[idx].fd_count);
                }
                trend[pos] = '\0';

                if (use_color) {
                    fprintf(stdout,
                            ANSI_YELLOW "%-8d" ANSI_RESET
                            " %-16s %-8u %-8d "
                            ANSI_MAGENTA "%s" ANSI_RESET "\n",
                            e->pid, e->cmd, (unsigned)e->uid,
                            e->history_count, trend);
                } else {
                    fprintf(stdout, "%-8d %-16s %-8u %-8d %s\n",
                            e->pid, e->cmd, (unsigned)e->uid,
                            e->history_count, trend);
                }
            }
        }

        fprintf(stdout, "\n");
    }

    /* Summary line */
    if (use_color) {
        fprintf(stdout,
                ANSI_GREEN "[iteration %d]" ANSI_RESET
                " scanned: %d, suspects: "
                ANSI_YELLOW "%d" ANSI_RESET "\n",
                leak_iteration, scanned, suspects);
    } else {
        fprintf(stdout, "[iteration %d] scanned: %d, suspects: %d\n",
                leak_iteration, scanned, suspects);
    }

    fflush(stdout);
}

/*
 * leak_detect_cleanup() -- free all malloc'd entries
 */
void leak_detect_cleanup(void) {
    unsigned int h;
    struct leak_entry *e, *next;

    for (h = 0; h < LEAK_HASH_SIZE; h++) {
        for (e = leak_table[h]; e; e = next) {
            next = e->next;
            free(e);
        }
        leak_table[h] = NULL;
    }
}
