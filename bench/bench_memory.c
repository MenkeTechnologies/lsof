#include "bench.h"

#undef BF_SECTION
#define BF_SECTION "MEMORY ALLOCATION"

/* ===== Memory allocation benchmarks ===== */
BENCH(malloc_free_small, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(64);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BENCH(malloc_free_medium, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(4096);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BENCH(realloc_grow, 1000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(64);
        p = realloc(p, 128);
        p = realloc(p, 256);
        p = realloc(p, 512);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BENCH(safe_realloc_pattern, 1000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(64);
        void *tmp = realloc(p, 128);
        if (tmp) p = tmp;
        tmp = realloc(p, 256);
        if (tmp) p = tmp;
        tmp = realloc(p, 512);
        if (tmp) p = tmp;
        BENCH_SINK_PTR(p);
        free(p);
    }
}

/* ===== memset/memcpy at different sizes ===== */
BENCH(memset_64, 10000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        memset(buf, 0, 64);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(memset_4096, 5000000) {
    char buf[4096];
    for (int i = 0; i < bf_iters; i++) {
        memset(buf, 0, 4096);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(memcpy_64, 10000000) {
    char src[64], dst[64];
    memset(src, 'A', 64);
    for (int i = 0; i < bf_iters; i++) {
        memcpy(dst, src, 64);
        BENCH_SINK_PTR(dst);
    }
}

BENCH(memcpy_4096, 5000000) {
    char src[4096], dst[4096];
    memset(src, 'A', 4096);
    for (int i = 0; i < bf_iters; i++) {
        memcpy(dst, src, 4096);
        BENCH_SINK_PTR(dst);
    }
}


BF_SECTIONS("MEMORY ALLOCATION")

RUN_BENCHMARKS()
