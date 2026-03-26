#include "bench.h"

#undef BF_SECTION
#define BF_SECTION "STRING BUILDING"

/* ===== String copy benchmarks (mkstrcpy pattern) ===== */
static char *bench_make_string_copy(const char *source) {
    size_t length = source ? strlen(source) : 0;
    char *new_str = (char *)malloc(length + 1);
    if (new_str) {
        if (source)
            memcpy(new_str, source, length + 1);
        else
            *new_str = '\0';
    }
    return new_str;
}

BENCH(mkstrcpy_short, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *copy = bench_make_string_copy("hello");
        BENCH_SINK_PTR(copy);
        free(copy);
    }
}

BENCH(mkstrcpy_path, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *copy =
            bench_make_string_copy("/usr/local/share/applications/very/long/path/to/some/file.txt");
        BENCH_SINK_PTR(copy);
        free(copy);
    }
}

BENCH(mkstrcpy_null, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *copy = bench_make_string_copy(NULL);
        BENCH_SINK_PTR(copy);
        free(copy);
    }
}

/* ===== mkstrcat benchmark (string concatenation used in path building) ===== */
static char *bench_mkstrcat(char *s1, int l1, char *s2, int l2, char *s3, int l3) {
    size_t len1 = s1 ? ((l1 >= 0) ? (size_t)l1 : strlen(s1)) : 0;
    size_t len2 = s2 ? ((l2 >= 0) ? (size_t)l2 : strlen(s2)) : 0;
    size_t len3 = s3 ? ((l3 >= 0) ? (size_t)l3 : strlen(s3)) : 0;
    char *cp = (char *)malloc(len1 + len2 + len3 + 1);
    if (cp) {
        char *tp = cp;
        if (s1 && len1) {
            memcpy(tp, s1, len1);
            tp += len1;
        }
        if (s2 && len2) {
            memcpy(tp, s2, len2);
            tp += len2;
        }
        if (s3 && len3) {
            memcpy(tp, s3, len3);
            tp += len3;
        }
        *tp = '\0';
    }
    return cp;
}

BENCH(mkstrcat_two, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcat("/usr/local", -1, "/bin/lsof", -1, NULL, 0);
        BENCH_SINK_PTR(s);
        free(s);
    }
}

BENCH(mkstrcat_three, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcat("/proc/", -1, "1234", -1, "/fd/0", -1);
        BENCH_SINK_PTR(s);
        free(s);
    }
}

BENCH(mkstrcat_with_lengths, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcat("/proc/", 6, "1234", 4, "/fd/0", 5);
        BENCH_SINK_PTR(s);
        free(s);
    }
}

BF_SECTIONS("STRING BUILDING")

RUN_BENCHMARKS()
