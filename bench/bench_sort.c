#include "bench.h"

#include <sys/types.h>

#undef BF_SECTION
#define BF_SECTION "SORTING & SEARCH"

/* ===== compdev (qsort) benchmark ===== */
typedef unsigned long bench_INODETYPE;
struct bench_l_dev {
    dev_t rdev;
    bench_INODETYPE inode;
    char *name;
    int v;
};

static int
bench_compare_devices(const void *first, const void *second)
{
    struct bench_l_dev **dev_ptr1 = (struct bench_l_dev **)first;
    struct bench_l_dev **dev_ptr2 = (struct bench_l_dev **)second;
    if ((*dev_ptr1)->rdev < (*dev_ptr2)->rdev) return -1;
    if ((*dev_ptr1)->rdev > (*dev_ptr2)->rdev) return 1;
    if ((*dev_ptr1)->inode < (*dev_ptr2)->inode) return -1;
    if ((*dev_ptr1)->inode > (*dev_ptr2)->inode) return 1;
    return strcmp((*dev_ptr1)->name, (*dev_ptr2)->name);
}

BENCH(compdev_sort_100, 100000) {
    static struct bench_l_dev devs[100];
    static struct bench_l_dev *ptrs[100];
    static char names[100][16];
    static int initialized = 0;

    if (!initialized) {
        for (int i = 0; i < 100; i++) {
            snprintf(names[i], 16, "dev%03d", i);
            devs[i].rdev = (dev_t)(99 - i);
            devs[i].inode = (bench_INODETYPE)(1000 + (i * 7) % 100);
            devs[i].name = names[i];
        }
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        for (int i = 0; i < 100; i++) ptrs[i] = &devs[i];
        qsort(ptrs, 100, sizeof(struct bench_l_dev *), bench_compare_devices);
        BENCH_SINK_PTR(ptrs[0]);
    }
}

BENCH(compdev_sort_1000, 10000) {
    static struct bench_l_dev devs[1000];
    static struct bench_l_dev *ptrs[1000];
    static char names[1000][16];
    static int initialized = 0;

    if (!initialized) {
        for (int i = 0; i < 1000; i++) {
            snprintf(names[i], 16, "dev%04d", i);
            devs[i].rdev = (dev_t)(999 - i);
            devs[i].inode = (bench_INODETYPE)(10000 + (i * 37) % 1000);
            devs[i].name = names[i];
        }
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        for (int i = 0; i < 1000; i++) ptrs[i] = &devs[i];
        qsort(ptrs, 1000, sizeof(struct bench_l_dev *), bench_compare_devices);
        BENCH_SINK_PTR(ptrs[0]);
    }
}

/* ===== PID binary search benchmark (compare_pids / qsort + bsearch) ===== */
static int bench_compare_pids(const void *first, const void *second) {
    int pid_a = *(const int *)first;
    int pid_b = *(const int *)second;
    if (pid_a < pid_b) return -1;
    if (pid_a > pid_b) return 1;
    return 0;
}

BENCH(pid_sort_1000, 50000) {
    static int pids[1000];
    for (int j = 0; j < bf_iters; j++) {
        for (int i = 0; i < 1000; i++) pids[i] = 1000 - i;
        qsort(pids, 1000, sizeof(int), bench_compare_pids);
        BENCH_SINK_INT(pids[0]);
    }
}

BENCH(pid_bsearch_1000, 5000000) {
    static int pids[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) pids[i] = i * 3;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int key = (i % 1000) * 3;
        int *found = (int *)bsearch(&key, pids, 1000, sizeof(int), bench_compare_pids);
        BENCH_SINK_PTR(found);
    }
}

/* ===== Process selection: linear scan vs binary search ===== */
BENCH(linear_pid_scan_100, 5000000) {
    static int pids[100];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 100; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 200) * 5; /* ~50% hit rate */
        int found = 0;
        for (int j = 0; j < 100; j++) {
            if (pids[j] == target) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

BENCH(bsearch_pid_scan_100, 5000000) {
    static int pids[100];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 100; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 200) * 5;
        int *found = (int *)bsearch(&target, pids, 100, sizeof(int), bench_compare_pids);
        BENCH_SINK_PTR(found);
    }
}

BENCH(linear_pid_scan_1000, 1000000) {
    static int pids[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 2000) * 5;
        int found = 0;
        for (int j = 0; j < 1000; j++) {
            if (pids[j] == target) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

BENCH(bsearch_pid_scan_1000, 5000000) {
    static int pids[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 2000) * 5;
        int *found = (int *)bsearch(&target, pids, 1000, sizeof(int), bench_compare_pids);
        BENCH_SINK_PTR(found);
    }
}


/* ===== Device binary search (find_ch_ino pattern from fino.c) ===== */
BENCH(device_bsearch_100, 5000000) {
    static dev_t devices[100];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 100; i++) devices[i] = (dev_t)(i * 10);
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        dev_t target = (dev_t)((i % 200) * 5);
        int lo = 0, hi = 99;
        int found = 0;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (devices[mid] < target) lo = mid + 1;
            else if (devices[mid] > target) hi = mid - 1;
            else { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

BENCH(device_bsearch_1000, 5000000) {
    static dev_t devices[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) devices[i] = (dev_t)(i * 10);
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        dev_t target = (dev_t)((i % 2000) * 5);
        int lo = 0, hi = 999;
        int found = 0;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (devices[mid] < target) lo = mid + 1;
            else if (devices[mid] > target) hi = mid - 1;
            else { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

/* ===== Dedup with varying duplicate density ===== */
BENCH(rmdupdev_30pct_dups, 100000) {
    static struct { unsigned long rdev; unsigned long inode; } devs[100];
    for (int i = 0; i < bf_iters; i++) {
        for (int j = 0; j < 100; j++) {
            devs[j].rdev = (unsigned long)(j * 7 / 10);
            devs[j].inode = (unsigned long)(j * 7 / 10);
        }
        int out = 1;
        for (int j = 1; j < 100; j++) {
            if (devs[j].rdev != devs[j-1].rdev || devs[j].inode != devs[j-1].inode)
                devs[out++] = devs[j];
        }
        BENCH_SINK_INT(out);
    }
}

BENCH(rmdupdev_50pct_dups, 100000) {
    static struct { unsigned long rdev; unsigned long inode; } devs[100];
    for (int i = 0; i < bf_iters; i++) {
        for (int j = 0; j < 100; j++) {
            devs[j].rdev = (unsigned long)(j / 2);
            devs[j].inode = (unsigned long)(j / 2);
        }
        int out = 1;
        for (int j = 1; j < 100; j++) {
            if (devs[j].rdev != devs[j-1].rdev || devs[j].inode != devs[j-1].inode)
                devs[out++] = devs[j];
        }
        BENCH_SINK_INT(out);
    }
}

BENCH(rmdupdev_90pct_dups, 100000) {
    static struct { unsigned long rdev; unsigned long inode; } devs[100];
    for (int i = 0; i < bf_iters; i++) {
        for (int j = 0; j < 100; j++) {
            devs[j].rdev = (unsigned long)(j / 10);
            devs[j].inode = (unsigned long)(j / 10);
        }
        int out = 1;
        for (int j = 1; j < 100; j++) {
            if (devs[j].rdev != devs[j-1].rdev || devs[j].inode != devs[j-1].inode)
                devs[out++] = devs[j];
        }
        BENCH_SINK_INT(out);
    }
}

BF_SECTIONS("SORTING & SEARCH")

RUN_BENCHMARKS()
