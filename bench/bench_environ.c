#include "bench.h"

#include <pwd.h>
#include <unistd.h>

#undef BF_SECTION
#define BF_SECTION "ENVIRONMENT & IDENTITY"

/* ===== UID cache lookup benchmark ===== */
BENCH(getpwuid_cached, 100000) {
    /* First call caches; subsequent calls should be fast */
    uid_t uid = getuid();
    for (int i = 0; i < bf_iters; i++) {
        struct passwd *pw = getpwuid(uid);
        BENCH_SINK_PTR(pw);
    }
}

/* ===== getenv benchmark (environment variable lookup) ===== */
BENCH(getenv_hit, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *v = getenv("PATH");
        BENCH_SINK_PTR(v);
    }
}

BENCH(getenv_miss, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *v = getenv("LSOF_NONEXISTENT_VAR_12345");
        BENCH_SINK_PTR(v);
    }
}


/* ===== gethostname benchmark ===== */
BENCH(gethostname_call, 1000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        gethostname(buf, sizeof(buf));
        BENCH_SINK_PTR(buf);
    }
}

/* ===== UID to username cache lookup benchmark ===== */
BENCH(uid_cache_linear_scan, 5000000) {
    static struct { int uid; const char *name; } cache[32];
    for (int j = 0; j < 32; j++) { cache[j].uid = j * 100; cache[j].name = "user"; }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 32) * 100;
        const char *found = NULL;
        for (int j = 0; j < 32; j++) {
            if (cache[j].uid == target) { found = cache[j].name; break; }
        }
        BENCH_SINK_PTR((void *)found);
    }
}

BENCH(uid_cache_hash_lookup, 5000000) {
    static struct { int uid; const char *name; } buckets[64];
    for (int j = 0; j < 32; j++) { int h = (j * 100 * 31415) & 63; buckets[h].uid = j * 100; buckets[h].name = "user"; }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 32) * 100;
        int h = (target * 31415) & 63;
        const char *found = (buckets[h].uid == target) ? buckets[h].name : NULL;
        BENCH_SINK_PTR((void *)found);
    }
}


BF_SECTIONS("ENVIRONMENT & IDENTITY")

RUN_BENCHMARKS()
