/*
 * delta.c -- watch mode delta highlighting for lsof repeat mode
 *
 * Tracks open files between iterations and tags each as NEW, GONE,
 * or UNCHANGED so print_file() can apply color highlighting.
 */

#include "lsof.h"
#include <stdlib.h>
#include <string.h>

#define DELTA_HASH_SIZE 4096

/*
 * Each entry represents a unique open file identified by (pid, fd, name).
 */
struct delta_entry {
    int pid;
    char fd[FDLEN];
    char *name;        /* malloc'd copy */
    char type[16];     /* file type for GONE display */
    char cmd[CMDL + 1];
    uid_t uid;
    struct delta_entry *next;
};

static struct delta_entry *prev_table[DELTA_HASH_SIZE];
static struct delta_entry *curr_table[DELTA_HASH_SIZE];

/* Current file's delta status for print_file() to query */
int DeltaFileStatus = DELTA_UNCHANGED;

/* Global counters reset each iteration */
static int delta_new_count;
static int delta_gone_count;

static unsigned int delta_hash(int pid, const char *fd, const char *name) {
    unsigned int h = (unsigned int)(pid * 31415);
    const char *s;
    for (s = fd; *s; s++)
        h = h * 37 + (unsigned char)*s;
    if (name) {
        for (s = name; *s; s++)
            h = h * 37 + (unsigned char)*s;
    }
    return h & (DELTA_HASH_SIZE - 1);
}

static void delta_free_table(struct delta_entry **table) {
    unsigned int h;
    struct delta_entry *e, *next;
    for (h = 0; h < DELTA_HASH_SIZE; h++) {
        for (e = table[h]; e; e = next) {
            next = e->next;
            if (e->name)
                free(e->name);
            free(e);
        }
        table[h] = NULL;
    }
}

static int delta_lookup(struct delta_entry **table, int pid,
                        const char *fd, const char *name) {
    unsigned int h = delta_hash(pid, fd, name);
    struct delta_entry *e;
    for (e = table[h]; e; e = e->next) {
        if (e->pid == pid &&
            strncmp(e->fd, fd, FDLEN) == 0 &&
            ((e->name == NULL && name == NULL) ||
             (e->name && name && strcmp(e->name, name) == 0)))
            return 1;
    }
    return 0;
}

/*
 * delta_init() -- initialize delta tracking (call once)
 */
void delta_init(void) {
    memset(prev_table, 0, sizeof(prev_table));
    memset(curr_table, 0, sizeof(curr_table));
}

/*
 * delta_begin_iteration() -- start a new iteration: swap tables
 */
void delta_begin_iteration(void) {
    /* Free old prev, move curr to prev, clear curr */
    delta_free_table(prev_table);
    memcpy(prev_table, curr_table, sizeof(curr_table));
    memset(curr_table, 0, sizeof(curr_table));
    delta_new_count = 0;
    delta_gone_count = 0;
}

/*
 * delta_record_file() -- record a file in the current snapshot
 */
void delta_record_file(int pid, const char *fd, const char *name,
                       const char *type, const char *cmd, uid_t uid) {
    unsigned int h = delta_hash(pid, fd, name);
    struct delta_entry *e;

    e = (struct delta_entry *)malloc(sizeof(struct delta_entry));
    if (!e)
        return;
    e->pid = pid;
    memset(e->fd, 0, FDLEN);
    strncpy(e->fd, fd, FDLEN - 1);
    e->name = name ? strdup(name) : NULL;
    memset(e->type, 0, sizeof(e->type));
    if (type)
        strncpy(e->type, type, sizeof(e->type) - 1);
    strncpy(e->cmd, cmd ? cmd : "?", CMDL);
    e->cmd[CMDL] = '\0';
    e->uid = uid;
    e->next = curr_table[h];
    curr_table[h] = e;
}

/*
 * delta_classify() -- classify a file as NEW or UNCHANGED
 *
 * Called from print_file() during the print pass.
 * Sets DeltaFileStatus for the caller.
 */
void delta_classify(int pid, const char *fd, const char *name) {
    if (!delta_lookup(prev_table, pid, fd, name)) {
        DeltaFileStatus = DELTA_NEW;
        delta_new_count++;
    } else {
        DeltaFileStatus = DELTA_UNCHANGED;
    }
}

/*
 * delta_print_gone() -- print entries from prev that are not in curr
 *
 * These are file descriptors that were closed between iterations.
 * Printed with strikethrough/red styling.
 */
void delta_print_gone(void) {
    unsigned int h;
    struct delta_entry *e;

    for (h = 0; h < DELTA_HASH_SIZE; h++) {
        for (e = prev_table[h]; e; e = e->next) {
            if (!delta_lookup(curr_table, e->pid, e->fd, e->name)) {
                delta_gone_count++;
                /* Print a GONE line in red/dim */
                printf("%s", CyberpunkTTY ? "\033[9;91m" : "");
                printf("%-*.*s", CommandColWidth, CommandColWidth,
                       e->cmd[0] ? e->cmd : "?");
                printf(" %*d", PidColWidth, e->pid);
                printf(" %*s", UserColWidth, "");
                printf(" %*.*s  ", FileDescColWidth - 2, FileDescColWidth - 2,
                       e->fd);
                printf(" %*.*s", TypeColWidth, TypeColWidth, e->type);
                printf(" %*s", DeviceColWidth, "");
                printf(" %*s", SizeOffColWidth, "");
                if (OptLinkCount)
                    printf(" %*s", LinkCountColWidth, "");
                printf(" %*s", NodeColWidth, "");
                printf(" %s", e->name ? e->name : "");
                printf("%s", CyberpunkTTY ? "\033[0m" : "");
                printf("  [GONE]\n");
            }
        }
    }
}

/*
 * delta_print_summary() -- print delta summary line
 */
void delta_print_summary(void) {
    if (CyberpunkTTY) {
        printf("%s[delta]%s", "\033[1;95m", "\033[0m");
        printf(" new: %s%d%s", "\033[1;92m", delta_new_count, "\033[0m");
        printf("  gone: %s%d%s", "\033[1;91m", delta_gone_count, "\033[0m");
        printf("\n");
    } else {
        printf("[delta] new: %d  gone: %d\n", delta_new_count, delta_gone_count);
    }
}

/*
 * delta_cleanup() -- free all tracking data
 */
void delta_cleanup(void) {
    delta_free_table(prev_table);
    delta_free_table(curr_table);
}
