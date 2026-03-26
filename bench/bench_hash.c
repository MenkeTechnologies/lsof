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


/* ===== Multi-field device/inode hash (SFHASHDEVINO from isfn.c) ===== */
#define SFHASHDEVINO(maj, min, ino, mod) \
    ((int)(((int)((((int)(maj+1))*((int)((min+1))))+ino)*31415)&(mod-1)))

BENCH(sfhash_devino, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(SFHASHDEVINO(8, i & 0xff, 1000 + i, 4096));
    }
}

BENCH(sfhash_devino_collisions, 5000000) {
    /* Measure collision rate: how many inputs map to same bucket */
    int buckets[4096];
    for (int j = 0; j < bf_iters; j++) {
        memset(buckets, 0, sizeof(buckets));
        for (int i = 0; i < 1000; i++) {
            buckets[SFHASHDEVINO(8, i % 16, i * 7, 4096)]++;
        }
        int max = 0;
        for (int i = 0; i < 4096; i++)
            if (buckets[i] > max) max = buckets[i];
        BENCH_SINK_INT(max);
    }
}

/* ===== Name cache hash (ncache pattern from rnmh.c) ===== */
static int ncache_hash(unsigned long inode, unsigned long node_addr, int mod) {
    return ((((int)(node_addr >> 2) + (int)(inode)) * 31415) & (mod - 1));
}

BENCH(ncache_hash_lookup, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(ncache_hash((unsigned long)(i * 7), (unsigned long)(0x1000 + i * 64), 2048));
    }
}

/* ===== Port service cache (lkup_svcnam pattern from print.c) ===== */
struct bench_svc_entry {
    int port;
    const char *name;
    struct bench_svc_entry *next;
};

BENCH(port_service_cache, 5000000) {
    static struct bench_svc_entry entries[20];
    static struct bench_svc_entry *buckets[PORTHASHBUCKETS];
    static int initialized = 0;
    static int ports[] = {22, 53, 80, 110, 143, 443, 993, 995, 3306, 5432,
                          6379, 8080, 8443, 9090, 27017, 11211, 6380, 1433, 3389, 5900};
    static const char *names[] = {"ssh", "dns", "http", "pop3", "imap", "https", "imaps", "pop3s",
                                  "mysql", "postgres", "redis", "http-alt", "https-alt", "websm",
                                  "mongo", "memcache", "redis2", "mssql", "rdp", "vnc"};
    if (!initialized) {
        memset(buckets, 0, sizeof(buckets));
        for (int i = 0; i < 20; i++) {
            entries[i].port = ports[i];
            entries[i].name = names[i];
            int h = HASHPORT(ports[i]);
            entries[i].next = buckets[h];
            buckets[h] = &entries[i];
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int port = ports[i % 20];
        int h = HASHPORT(port);
        struct bench_svc_entry *e;
        const char *found = NULL;
        for (e = buckets[h]; e; e = e->next) {
            if (e->port == port) { found = e->name; break; }
        }
        BENCH_SINK_PTR((void *)found);
    }
}

BF_SECTIONS("HASH FUNCTIONS")

RUN_BENCHMARKS()
