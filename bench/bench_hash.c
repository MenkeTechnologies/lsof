#include "bench.h"

#undef BF_SECTION
#define BF_SECTION "HASH FUNCTIONS"

/* ===== HASHPORT benchmark ===== */
#define PORTHASHBUCKETS 128
#define HASHPORT(p) (((((int)(p)) * 31415) >> 3) & (PORTHASHBUCKETS - 1))

BENCH(hashport_all_ports, 100) {
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (int p = 0; p < 65536; p++) {
            sum += HASHPORT(p);
        }
        BENCH_SINK_INT(sum);
    }
}

BENCH(hashport_common_ports, 10000000) {
    int common[] = {22, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017, 53};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(HASHPORT(common[i % 10]));
    }
}

/* ===== hashbyname benchmark (name-based hashing used in lsof) ===== */
static int
bench_hashbyname(char *nm, int mod)
{
    int i, j;
    for (i = j = 0; *nm; nm++) {
        i ^= (int)*nm << j;
        if (++j > 7)
            j = 0;
    }
    return (((int)(i * 31415)) & (mod - 1));
}

BENCH(hashbyname_short, 10000000) {
    char *names[] = {"tcp", "udp", "pipe", "unix", "fifo"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_hashbyname(names[i % 5], 128));
    }
}

BENCH(hashbyname_path, 5000000) {
    char *paths[] = {
        "/usr/local/bin/node", "/var/log/syslog", "/dev/null",
        "/proc/self/fd/3", "/tmp/socket.sock"
    };
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_hashbyname(paths[i % 5], 256));
    }
}

BENCH(hashbyname_long, 2000000) {
    char *names[] = {
        "/usr/local/share/applications/very/deep/nested/path/to/resource.dat",
        "/home/user/.config/some-application/settings/profile/default.json",
        "/var/lib/docker/overlay2/abc123def456/merged/usr/bin/entrypoint.sh",
    };
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_hashbyname(names[i % 3], 512));
    }
}


BF_SECTIONS("HASH FUNCTIONS")

RUN_BENCHMARKS()
