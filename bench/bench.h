/*
 * bench_framework.h - minimal C benchmark framework for lsof
 *
 * Usage:
 *   #define BF_SECTION "Section Name"
 *   BENCH(name, iterations) { ... body ... }
 *   #define BF_SECTION "Another Section"
 *   BENCH(name2, iterations) { ... body ... }
 *   BF_SECTIONS("Section Name", "Another Section")
 *   RUN_BENCHMARKS();
 */

#ifndef BENCH_FRAMEWORK_H
#define BENCH_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ANSI color codes */
#define B_RESET    "\033[0m"
#define B_BOLD     "\033[1m"
#define B_DIM      "\033[2m"
#define B_RED      "\033[31m"
#define B_GREEN    "\033[32m"
#define B_YELLOW   "\033[33m"
#define B_BLUE     "\033[34m"
#define B_MAGENTA  "\033[35m"
#define B_CYAN     "\033[36m"
#define B_BRED     "\033[1;31m"
#define B_BGREEN   "\033[1;32m"
#define B_BYELLOW  "\033[1;33m"
#define B_BCYAN    "\033[1;36m"
#define B_BMAGENTA "\033[1;35m"

typedef void (*bf_bench_fn)(int iterations);

typedef struct {
    const char *name;
    const char *section;
    bf_bench_fn fn;
    int iterations;
    int order;
} bf_bench_entry;

static bf_bench_entry bf_registry[256];
static int bf_registry_count = 0;

static double bf_time_ns(struct timespec *start, struct timespec *end) {
    return (double)(end->tv_sec - start->tv_sec) * 1e9 + (double)(end->tv_nsec - start->tv_nsec);
}

/* Classify performance tier by ns/op for color coding */
static inline const char *bf_tier_color(double ns_per_op) {
    if (ns_per_op < 10.0)
        return B_BGREEN; /* blazing */
    if (ns_per_op < 100.0)
        return B_GREEN; /* fast */
    if (ns_per_op < 1000.0)
        return B_BYELLOW; /* moderate */
    if (ns_per_op < 100000.0)
        return B_YELLOW; /* slow */
    return B_RED;        /* critical */
}

static inline const char *bf_ops_color(double ops_sec) {
    if (ops_sec > 100000000.0)
        return B_BGREEN;
    if (ops_sec > 10000000.0)
        return B_GREEN;
    if (ops_sec > 1000000.0)
        return B_BYELLOW;
    if (ops_sec > 10000.0)
        return B_YELLOW;
    return B_RED;
}

/* Default section for benchmarks without an explicit section */
#ifndef BF_SECTION
#define BF_SECTION NULL
#endif

#define BENCH(bench_name, iters)                                                 \
    static void bench_impl_##bench_name(int);                                    \
    __attribute__((constructor)) static void register_bench_##bench_name(void) { \
        bf_registry[bf_registry_count].name = #bench_name;                       \
        bf_registry[bf_registry_count].section = BF_SECTION;                     \
        bf_registry[bf_registry_count].fn = bench_impl_##bench_name;             \
        bf_registry[bf_registry_count].iterations = (iters);                     \
        bf_registry[bf_registry_count].order = bf_registry_count;                \
        bf_registry_count++;                                                     \
    }                                                                            \
    static void bench_impl_##bench_name(int bf_iters)

/* volatile sink to prevent dead code elimination */
static volatile int bf_sink_i;
static volatile void *bf_sink_p;
#define BENCH_SINK_INT(x) bf_sink_i = (x)
#define BENCH_SINK_PTR(x) bf_sink_p = (x)

/* Section display order — define before RUN_BENCHMARKS() */
static const char *bf_section_list[64];
static int bf_section_count = 0;

#define BF_SECTIONS(...)                                              \
    __attribute__((constructor)) static void bf_init_sections(void) { \
        const char *_s[] = {__VA_ARGS__};                             \
        bf_section_count = (int)(sizeof(_s) / sizeof(_s[0]));         \
        for (int _i = 0; _i < bf_section_count; _i++)                 \
            bf_section_list[_i] = _s[_i];                             \
    }

static int bf_section_rank(const char *section) {
    if (!section)
        return bf_section_count;
    for (int i = 0; i < bf_section_count; i++)
        if (strcmp(bf_section_list[i], section) == 0)
            return i;
    return bf_section_count;
}

static int bf_compare_entries(const void *a, const void *b) {
    const bf_bench_entry *ea = (const bf_bench_entry *)a;
    const bf_bench_entry *eb = (const bf_bench_entry *)b;
    int ra = bf_section_rank(ea->section);
    int rb = bf_section_rank(eb->section);
    if (ra != rb)
        return ra - rb;
    return ea->order - eb->order;
}

#define RUN_BENCHMARKS()                                                                           \
    int main(int argc, char **argv) {                                                              \
        (void)argc;                                                                                \
        (void)argv;                                                                                \
        printf(B_BCYAN "\n "                                                                       \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88" B_RESET "\n");                                   \
        printf(B_BCYAN " \xE2\x96\x88\xE2\x96\x88" B_RESET "  " B_BOLD "%-57s" B_RESET B_BCYAN     \
                       "  \xE2\x96\x88\xE2\x96\x88" B_RESET "\n",                                  \
               "LSOF CORE OPERATIONS PROFILER");                                                   \
        printf(B_BCYAN " \xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88" \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88"                                                  \
                       "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"  \
                       "\xE2\x96\x88\xE2\x96\x88" B_RESET "\n");                                   \
        qsort(bf_registry, bf_registry_count, sizeof(bf_bench_entry), bf_compare_entries);         \
        const char *prev_section = NULL;                                                           \
        for (int i = 0; i < bf_registry_count; i++) {                                              \
            if (bf_registry[i].section &&                                                          \
                (prev_section == NULL || strcmp(bf_registry[i].section, prev_section) != 0)) {     \
                printf("\n" B_BMAGENTA " \xE2\x96\x93\xE2\x96\x93\xE2\x96\x93 " B_RESET B_BOLD     \
                       "%s" B_RESET B_BMAGENTA " \xE2\x96\x93\xE2\x96\x93\xE2\x96\x93" B_RESET     \
                       "\n",                                                                       \
                       bf_registry[i].section);                                                    \
                prev_section = bf_registry[i].section;                                             \
            }                                                                                      \
            struct timespec start, end;                                                            \
            int iters = bf_registry[i].iterations;                                                 \
            /* warmup */                                                                           \
            bf_registry[i].fn(iters / 10 > 0 ? iters / 10 : 1);                                    \
            /* timed run */                                                                        \
            clock_gettime(CLOCK_MONOTONIC, &start);                                                \
            bf_registry[i].fn(iters);                                                              \
            clock_gettime(CLOCK_MONOTONIC, &end);                                                  \
            double total_ns = bf_time_ns(&start, &end);                                            \
            double ns_per_op = total_ns / (double)iters;                                           \
            double total_ms = total_ns / 1e6;                                                      \
            double ops_sec = (double)iters / (total_ms / 1000.0);                                  \
            printf("  " B_DIM "\xE2\x96\x90" B_RESET " %-40s " B_CYAN "%10.2f ms" B_RESET "  "     \
                   "%s%8.1f ns/op" B_RESET "  "                                                    \
                   "%s%12.0f ops/s" B_RESET "\n",                                                  \
                   bf_registry[i].name, total_ms, bf_tier_color(ns_per_op), ns_per_op,             \
                   bf_ops_color(ops_sec), ops_sec);                                                \
        }                                                                                          \
        printf("\n" B_DIM " \xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94"  \
                          "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                   \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94"  \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                          \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94"  \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                          \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94"  \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                          \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94"  \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                          \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94"  \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80" B_RESET "\n");                           \
        printf(B_BGREEN " \xE2\x96\x88\xE2\x96\x88 PROFILING COMPLETE" B_RESET B_GREEN             \
                        " // all cycles accounted for" B_RESET "\n");                              \
        return 0;                                                                                  \
    }

#endif /* BENCH_FRAMEWORK_H */
