/*
 * summary.c -- aggregate FD summary for lsofng (--summary / --stats)
 *
 * Displays: total open files, per-type breakdown with bar chart,
 * top processes by FD count, and per-user totals.
 * Supports plain text (with cyberpunk TTY theming) and JSON output.
 */

#include "lsof.h"

#include <string.h>
#include <stdlib.h>

/*
 * Hash table sizes
 */
#define SUMMARY_TYPE_HASH   64
#define SUMMARY_USER_HASH   256
#define SUMMARY_PROC_MAX    4096
#define SUMMARY_TOP_N       10
#define SUMMARY_BAR_MAX     20

/*
 * Per-type entry
 */
struct summary_type {
    char type[TYPEL];
    int count;
    struct summary_type *next;
};

/*
 * Per-user entry
 */
struct summary_user {
    uid_t uid;
    int file_count;
    int proc_count;
    struct summary_user *next;
};

/*
 * Per-process entry (flat array, sorted later)
 */
struct summary_proc {
    int pid;
    char cmd[CMDL + 1];
    uid_t uid;
    int fd_count;
};

/*
 * Module state
 */
static struct summary_type *type_table[SUMMARY_TYPE_HASH];
static struct summary_user *user_table[SUMMARY_USER_HASH];
static struct summary_proc *proc_array;
static int proc_count;
static int proc_alloc;
static int total_files;
static int total_procs;
static int num_types;

/*
 * Simple hash for short strings
 */
static unsigned int type_hash(const char *s) {
    unsigned int h = 0;
    while (*s)
        h = h * 31 + (unsigned char)*s++;
    return h % SUMMARY_TYPE_HASH;
}

static unsigned int uid_hash(uid_t uid) {
    return (unsigned int)uid % SUMMARY_USER_HASH;
}

/*
 * Comparison for qsort -- descending by fd_count
 */
static int cmp_proc_desc(const void *a, const void *b) {
    const struct summary_proc *pa = (const struct summary_proc *)a;
    const struct summary_proc *pb = (const struct summary_proc *)b;
    if (pb->fd_count != pa->fd_count)
        return pb->fd_count - pa->fd_count;
    return pa->pid - pb->pid;
}

/*
 * Comparison for type entries -- descending by count
 */
static int cmp_type_desc(const void *a, const void *b) {
    const struct summary_type *const *pa = (const struct summary_type *const *)a;
    const struct summary_type *const *pb = (const struct summary_type *const *)b;
    if ((*pb)->count != (*pa)->count)
        return (*pb)->count - (*pa)->count;
    return strcmp((*pa)->type, (*pb)->type);
}

/*
 * Comparison for user entries -- descending by file_count
 */
static int cmp_user_desc(const void *a, const void *b) {
    const struct summary_user *const *pa = (const struct summary_user *const *)a;
    const struct summary_user *const *pb = (const struct summary_user *const *)b;
    if ((*pb)->file_count != (*pa)->file_count)
        return (*pb)->file_count - (*pa)->file_count;
    return (int)(*pa)->uid - (int)(*pb)->uid;
}

/*
 * Format number with comma separators (rotating static buffers)
 */
static const char *fmt_num(int n) {
    static char bufs[4][32];
    static int idx = 0;
    char *buf = bufs[idx++ & 3];
    char tmp[32];
    int len, i, j, neg;

    neg = (n < 0);
    if (neg) n = -n;
    snprintf(tmp, sizeof(tmp), "%d", n);
    len = (int)strlen(tmp);

    j = 0;
    if (neg) buf[j++] = '-';
    for (i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0)
            buf[j++] = ',';
        buf[j++] = tmp[i];
    }
    buf[j] = '\0';
    return buf;
}

/*
 * summary_collect() -- walk LocalProcTable and aggregate stats
 */
void summary_collect(void) {
    int i;
    struct lfile *lf;
    struct lproc *p;
    unsigned int h;

    memset(type_table, 0, sizeof(type_table));
    memset(user_table, 0, sizeof(user_table));
    proc_array = NULL;
    proc_count = 0;
    proc_alloc = 0;
    total_files = 0;
    total_procs = 0;
    num_types = 0;

    for (i = 0; i < NumLocalProcs; i++) {
        p = &LocalProcTable[i];
        if (!p->sel_state)
            continue;

        total_procs++;

        /* Count FDs for this process */
        int fd_count = 0;
        for (lf = p->file; lf; lf = lf->next) {
            fd_count++;
            total_files++;

            /* Type breakdown */
            h = type_hash(lf->type);
            struct summary_type *te;
            for (te = type_table[h]; te; te = te->next) {
                if (strcmp(te->type, lf->type) == 0)
                    break;
            }
            if (!te) {
                te = (struct summary_type *)malloc(sizeof(struct summary_type));
                if (!te) continue;
                memset(te, 0, sizeof(*te));
                strncpy(te->type, lf->type, TYPEL - 1);
                te->type[TYPEL - 1] = '\0';
                te->next = type_table[h];
                type_table[h] = te;
                num_types++;
            }
            te->count++;
        }

        /* Per-user tracking */
        h = uid_hash(p->uid);
        struct summary_user *ue;
        for (ue = user_table[h]; ue; ue = ue->next) {
            if (ue->uid == p->uid)
                break;
        }
        if (!ue) {
            ue = (struct summary_user *)malloc(sizeof(struct summary_user));
            if (!ue) continue;
            memset(ue, 0, sizeof(*ue));
            ue->uid = p->uid;
            ue->next = user_table[h];
            user_table[h] = ue;
        }
        ue->file_count += fd_count;
        ue->proc_count++;

        /* Per-process tracking */
        if (proc_count >= proc_alloc) {
            proc_alloc = proc_alloc ? proc_alloc * 2 : 256;
            struct summary_proc *tmp = (struct summary_proc *)realloc(
                proc_array, proc_alloc * sizeof(struct summary_proc));
            if (!tmp) continue;
            proc_array = tmp;
        }
        proc_array[proc_count].pid = p->pid;
        proc_array[proc_count].uid = p->uid;
        proc_array[proc_count].fd_count = fd_count;
        memset(proc_array[proc_count].cmd, 0, CMDL + 1);
        if (p->cmd)
            strncpy(proc_array[proc_count].cmd, p->cmd, CMDL);
        proc_count++;
    }

    /* Sort processes by FD count descending */
    if (proc_count > 1)
        qsort(proc_array, proc_count, sizeof(struct summary_proc), cmp_proc_desc);
}

/*
 * summary_report_text() -- print human-readable summary
 */
void summary_report_text(void) {
    int i, top_n, bar_len;
    struct summary_type *te;
    struct summary_type **type_arr;
    struct summary_user *ue;
    struct summary_user **user_arr;
    int user_count;

    /* Header */
    printf("\n");
    if (CyberpunkTTY) {
        printf(" %s%sSYSTEM FILE DESCRIPTOR SUMMARY%s\n", CP_BOLD, CP_NEON_CYAN, CP_RESET);
        printf(" %s═══════════════════════════════════════════════════%s\n", CP_NEON_CYAN, CP_RESET);
    } else {
        printf(" SYSTEM FILE DESCRIPTOR SUMMARY\n");
        printf(" ===================================================\n");
    }

    printf("  Total open files:  %s\n", fmt_num(total_files));
    printf("  Total processes:   %s\n", fmt_num(total_procs));

    /* ── TYPE BREAKDOWN ── */
    printf("\n");
    if (CyberpunkTTY) {
        printf(" %s── TYPE BREAKDOWN ──────────────────────────────────%s\n",
               CP_NEON_MAGENTA, CP_RESET);
    } else {
        printf(" -- TYPE BREAKDOWN ---------------------------------\n");
    }

    /* Collect types into a sorted array */
    type_arr = (struct summary_type **)malloc(num_types * sizeof(struct summary_type *));
    if (type_arr) {
        int idx = 0;
        for (i = 0; i < SUMMARY_TYPE_HASH; i++) {
            for (te = type_table[i]; te; te = te->next) {
                if (idx < num_types)
                    type_arr[idx++] = te;
            }
        }
        qsort(type_arr, idx, sizeof(struct summary_type *), cmp_type_desc);

        for (i = 0; i < idx && i < 15; i++) {
            double pct = total_files > 0 ? 100.0 * type_arr[i]->count / total_files : 0.0;
            bar_len = (int)(pct * SUMMARY_BAR_MAX / 100.0 + 0.5);
            if (bar_len < 1 && type_arr[i]->count > 0)
                bar_len = 1;

            if (CyberpunkTTY) {
                printf("  %s%-8s%s %6s  %s", CP_NEON_GREEN, type_arr[i]->type, CP_RESET,
                       fmt_num(type_arr[i]->count), CP_NEON_CYAN);
                for (int b = 0; b < bar_len; b++)
                    printf("█");
                printf("%s  %5.1f%%%s\n", CP_DIM, pct, CP_RESET);
            } else {
                printf("  %-8s %6s  ", type_arr[i]->type, fmt_num(type_arr[i]->count));
                for (int b = 0; b < bar_len; b++)
                    printf("#");
                printf("  %5.1f%%\n", pct);
            }
        }

        /* If more than 15 types, show "(other)" */
        if (idx > 15) {
            int other = 0;
            for (i = 15; i < idx; i++)
                other += type_arr[i]->count;
            double pct = total_files > 0 ? 100.0 * other / total_files : 0.0;
            if (CyberpunkTTY) {
                printf("  %s%-8s%s %6s  %s  %5.1f%%%s\n",
                       CP_DIM, "(other)", CP_RESET, fmt_num(other),
                       CP_DIM, pct, CP_RESET);
            } else {
                printf("  %-8s %6s    %5.1f%%\n", "(other)", fmt_num(other), pct);
            }
        }
        free(type_arr);
    }

    /* ── TOP PROCESSES BY FD COUNT ── */
    printf("\n");
    if (CyberpunkTTY) {
        printf(" %s── TOP PROCESSES BY FD COUNT ───────────────────────%s\n",
               CP_NEON_MAGENTA, CP_RESET);
    } else {
        printf(" -- TOP PROCESSES BY FD COUNT ----------------------\n");
    }

    top_n = proc_count < SUMMARY_TOP_N ? proc_count : SUMMARY_TOP_N;

    if (CyberpunkTTY) {
        printf("  %s%s%-8s %-16s %-10s %6s%s\n",
               CP_HDR_BG, CP_NEON_YELLOW, "PID", "COMMAND", "USER", "FDs", CP_RESET);
    } else {
        printf("  %-8s %-16s %-10s %6s\n", "PID", "COMMAND", "USER", "FDs");
    }

    for (i = 0; i < top_n; i++) {
        char *uname = printuid((UID_ARG)proc_array[i].uid, NULL);
        if (CyberpunkTTY) {
            if (i % 2)
                printf("  %s%-8d %-16.16s %-10.10s %6s%s\n",
                       CP_ROW_ALT, proc_array[i].pid, proc_array[i].cmd,
                       uname, fmt_num(proc_array[i].fd_count), CP_RESET);
            else
                printf("  %-8d %-16.16s %-10.10s %6s\n",
                       proc_array[i].pid, proc_array[i].cmd,
                       uname, fmt_num(proc_array[i].fd_count));
        } else {
            printf("  %-8d %-16.16s %-10.10s %6s\n",
                   proc_array[i].pid, proc_array[i].cmd,
                   uname, fmt_num(proc_array[i].fd_count));
        }
    }

    /* ── PER-USER TOTALS ── */
    printf("\n");
    if (CyberpunkTTY) {
        printf(" %s── PER-USER TOTALS ─────────────────────────────────%s\n",
               CP_NEON_MAGENTA, CP_RESET);
    } else {
        printf(" -- PER-USER TOTALS --------------------------------\n");
    }

    /* Collect users into sorted array */
    user_count = 0;
    for (i = 0; i < SUMMARY_USER_HASH; i++) {
        for (ue = user_table[i]; ue; ue = ue->next)
            user_count++;
    }

    user_arr = (struct summary_user **)malloc(user_count * sizeof(struct summary_user *));
    if (user_arr) {
        int idx = 0;
        for (i = 0; i < SUMMARY_USER_HASH; i++) {
            for (ue = user_table[i]; ue; ue = ue->next) {
                if (idx < user_count)
                    user_arr[idx++] = ue;
            }
        }
        qsort(user_arr, idx, sizeof(struct summary_user *), cmp_user_desc);

        if (CyberpunkTTY) {
            printf("  %s%s%-12s %6s %8s%s\n",
                   CP_HDR_BG, CP_NEON_YELLOW, "USER", "PROCS", "FILES", CP_RESET);
        } else {
            printf("  %-12s %6s %8s\n", "USER", "PROCS", "FILES");
        }

        top_n = idx < 15 ? idx : 15;
        for (i = 0; i < top_n; i++) {
            char *uname = printuid((UID_ARG)user_arr[i]->uid, NULL);
            if (CyberpunkTTY) {
                if (i % 2)
                    printf("  %s%-12.12s %6s %8s%s\n",
                           CP_ROW_ALT, uname, fmt_num(user_arr[i]->proc_count),
                           fmt_num(user_arr[i]->file_count), CP_RESET);
                else
                    printf("  %-12.12s %6s %8s\n",
                           uname, fmt_num(user_arr[i]->proc_count),
                           fmt_num(user_arr[i]->file_count));
            } else {
                printf("  %-12.12s %6s %8s\n",
                       uname, fmt_num(user_arr[i]->proc_count),
                       fmt_num(user_arr[i]->file_count));
            }
        }
        free(user_arr);
    }

    printf("\n");
}

/*
 * json_esc() -- local JSON string escaper (mirrors json.c)
 */
static void json_esc(const char *s) {
    if (!s) { fprintf(stdout, "null"); return; }
    fputc('"', stdout);
    for (; *s; s++) {
        switch (*s) {
        case '"':  fprintf(stdout, "\\\""); break;
        case '\\': fprintf(stdout, "\\\\"); break;
        case '\n': fprintf(stdout, "\\n"); break;
        case '\r': fprintf(stdout, "\\r"); break;
        case '\t': fprintf(stdout, "\\t"); break;
        default:
            if ((unsigned char)*s < 0x20)
                fprintf(stdout, "\\u%04x", (unsigned char)*s);
            else
                fputc(*s, stdout);
            break;
        }
    }
    fputc('"', stdout);
}

/*
 * summary_report_json() -- print summary as a JSON object
 */
void summary_report_json(void) {
    int i, top_n;
    struct summary_type *te;
    struct summary_type **type_arr;
    struct summary_user *ue;
    struct summary_user **user_arr;
    int user_count, first;

    printf("{\n");
    printf("  \"summary\": {\n");
    printf("    \"total_files\": %d,\n", total_files);
    printf("    \"total_processes\": %d,\n", total_procs);

    /* Types */
    printf("    \"types\": {\n");
    type_arr = (struct summary_type **)malloc(num_types * sizeof(struct summary_type *));
    if (type_arr) {
        int idx = 0;
        for (i = 0; i < SUMMARY_TYPE_HASH; i++) {
            for (te = type_table[i]; te; te = te->next)
                if (idx < num_types)
                    type_arr[idx++] = te;
        }
        qsort(type_arr, idx, sizeof(struct summary_type *), cmp_type_desc);
        for (i = 0; i < idx; i++) {
            printf("      ");
            json_esc(type_arr[i]->type);
            printf(": %d%s\n", type_arr[i]->count, i < idx - 1 ? "," : "");
        }
        free(type_arr);
    }
    printf("    },\n");

    /* Top processes */
    printf("    \"top_processes\": [\n");
    top_n = proc_count < SUMMARY_TOP_N ? proc_count : SUMMARY_TOP_N;
    for (i = 0; i < top_n; i++) {
        char *uname = printuid((UID_ARG)proc_array[i].uid, NULL);
        printf("      {\"pid\": %d, \"command\": ", proc_array[i].pid);
        json_esc(proc_array[i].cmd);
        printf(", \"uid\": %lu, \"user\": ", (unsigned long)proc_array[i].uid);
        json_esc(uname);
        printf(", \"fd_count\": %d}%s\n", proc_array[i].fd_count,
               i < top_n - 1 ? "," : "");
    }
    printf("    ],\n");

    /* Per-user */
    printf("    \"users\": [\n");
    user_count = 0;
    for (i = 0; i < SUMMARY_USER_HASH; i++) {
        for (ue = user_table[i]; ue; ue = ue->next)
            user_count++;
    }
    user_arr = (struct summary_user **)malloc(user_count * sizeof(struct summary_user *));
    first = 1;
    if (user_arr) {
        int idx = 0;
        for (i = 0; i < SUMMARY_USER_HASH; i++) {
            for (ue = user_table[i]; ue; ue = ue->next)
                if (idx < user_count)
                    user_arr[idx++] = ue;
        }
        qsort(user_arr, idx, sizeof(struct summary_user *), cmp_user_desc);
        for (i = 0; i < idx; i++) {
            char *uname = printuid((UID_ARG)user_arr[i]->uid, NULL);
            if (!first) printf(",\n");
            printf("      {\"uid\": %lu, \"user\": ",
                   (unsigned long)user_arr[i]->uid);
            json_esc(uname);
            printf(", \"process_count\": %d, \"file_count\": %d}",
                   user_arr[i]->proc_count, user_arr[i]->file_count);
            first = 0;
        }
        free(user_arr);
    }
    printf("\n    ]\n");
    printf("  }\n");
    printf("}\n");
}

/*
 * summary_cleanup() -- free all allocated memory
 */
void summary_cleanup(void) {
    int i;
    struct summary_type *te, *tn;
    struct summary_user *ue, *un;

    for (i = 0; i < SUMMARY_TYPE_HASH; i++) {
        for (te = type_table[i]; te; te = tn) {
            tn = te->next;
            free(te);
        }
        type_table[i] = NULL;
    }

    for (i = 0; i < SUMMARY_USER_HASH; i++) {
        for (ue = user_table[i]; ue; ue = un) {
            un = ue->next;
            free(ue);
        }
        user_table[i] = NULL;
    }

    if (proc_array) {
        free(proc_array);
        proc_array = NULL;
    }
    proc_count = 0;
    proc_alloc = 0;
}
