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
        if (tmp)
            p = tmp;
        tmp = realloc(p, 256);
        if (tmp)
            p = tmp;
        tmp = realloc(p, 512);
        if (tmp)
            p = tmp;
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

/* ===== Chunked growth benchmark (alloc_lproc pattern from proc.c) ===== */
#define LPROCINCR 32

BENCH(chunked_realloc_grow, 100000) {
    for (int j = 0; j < bf_iters; j++) {
        int *table = NULL;
        int size = 0, count = 0;
        for (int i = 0; i < 256; i++) {
            if (count >= size) {
                size += LPROCINCR;
                int *tmp = (int *)realloc(table, (size_t)size * sizeof(int));
                if (tmp)
                    table = tmp;
            }
            table[count++] = i;
        }
        BENCH_SINK_INT(count);
        free(table);
    }
}

/* ===== Batch allocation vs individual (process table pattern) ===== */
BENCH(malloc_individual_256, 50000) {
    for (int j = 0; j < bf_iters; j++) {
        void *ptrs[256];
        for (int i = 0; i < 256; i++)
            ptrs[i] = malloc(sizeof(int) * 4);
        for (int i = 0; i < 256; i++)
            free(ptrs[i]);
        BENCH_SINK_PTR(ptrs[0]);
    }
}

BENCH(malloc_batch_256, 50000) {
    for (int j = 0; j < bf_iters; j++) {
        void *block = malloc(sizeof(int) * 4 * 256);
        BENCH_SINK_PTR(block);
        free(block);
    }
}

/* ===== calloc vs malloc+memset ===== */
BENCH(calloc_4096, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = calloc(1, 4096);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BENCH(malloc_memset_4096, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(4096);
        memset(p, 0, 4096);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BF_SECTIONS("MEMORY ALLOCATION")

RUN_BENCHMARKS()
