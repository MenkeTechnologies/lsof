/*
 * bench_framework.h - minimal C benchmark framework for lsof
 *
 * Usage:
 *   BENCH(name, iterations) { ... body ... }
 *   RUN_BENCHMARKS();
 */

#ifndef BENCH_FRAMEWORK_H
#define BENCH_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

typedef void (*bf_bench_fn)(int iterations);

typedef struct {
    const char *name;
    bf_bench_fn fn;
    int iterations;
} bf_bench_entry;

static bf_bench_entry bf_registry[256];
static int bf_registry_count = 0;

static double bf_time_ns(struct timespec *start, struct timespec *end) {
    return (double)(end->tv_sec - start->tv_sec) * 1e9 +
           (double)(end->tv_nsec - start->tv_nsec);
}

static void bf_print_time(double ns_per_op) {
    if (ns_per_op < 1000.0)
        printf("%8.1f ns/op", ns_per_op);
    else if (ns_per_op < 1e6)
        printf("%8.1f us/op", ns_per_op / 1e3);
    else if (ns_per_op < 1e9)
        printf("%8.1f ms/op", ns_per_op / 1e6);
    else
        printf("%8.2f  s/op", ns_per_op / 1e9);
}

#define BENCH(name, iters) \
    static void bench_impl_##name(int); \
    __attribute__((constructor)) static void register_bench_##name(void) { \
        bf_registry[bf_registry_count].name = #name; \
        bf_registry[bf_registry_count].fn = bench_impl_##name; \
        bf_registry[bf_registry_count].iterations = (iters); \
        bf_registry_count++; \
    } \
    static void bench_impl_##name(int bf_iters)

/* volatile sink to prevent dead code elimination */
static volatile int bf_sink_i;
static volatile void *bf_sink_p;
#define BENCH_SINK_INT(x) bf_sink_i = (x)
#define BENCH_SINK_PTR(x) bf_sink_p = (x)

#define RUN_BENCHMARKS() \
    int main(int argc, char **argv) { \
        (void)argc; (void)argv; \
        printf("Running %d benchmarks...\n\n", bf_registry_count); \
        printf("%-40s %10s %16s\n", "Benchmark", "Iterations", "Time"); \
        printf("%-40s %10s %16s\n", \
            "----------------------------------------", \
            "----------", "----------------"); \
        for (int i = 0; i < bf_registry_count; i++) { \
            struct timespec start, end; \
            int iters = bf_registry[i].iterations; \
            /* warmup */ \
            bf_registry[i].fn(iters / 10 > 0 ? iters / 10 : 1); \
            /* timed run */ \
            clock_gettime(CLOCK_MONOTONIC, &start); \
            bf_registry[i].fn(iters); \
            clock_gettime(CLOCK_MONOTONIC, &end); \
            double total_ns = bf_time_ns(&start, &end); \
            double ns_per_op = total_ns / (double)iters; \
            printf("%-40s %10d ", bf_registry[i].name, iters); \
            bf_print_time(ns_per_op); \
            printf("\n"); \
        } \
        printf("\nDone.\n"); \
        return 0; \
    }

#endif /* BENCH_FRAMEWORK_H */
