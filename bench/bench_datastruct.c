#include "bench.h"

#include "../src/lsof_fields.h"

#undef BF_SECTION
#define BF_SECTION "DATA STRUCTURES"

#define PORTHASHBUCKETS 128
#define HASHPORT(p) (((((int)(p)) * 31415) >> 3) & (PORTHASHBUCKETS - 1))

/* ===== Linked list traversal benchmark (common lsof pattern) ===== */
struct bench_node {
    int value;
    struct bench_node *next;
};

BENCH(linked_list_traverse_100, 1000000) {
    static struct bench_node nodes[100];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 99; i++) {
            nodes[i].value = i;
            nodes[i].next = &nodes[i + 1];
        }
        nodes[99].value = 99;
        nodes[99].next = NULL;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (struct bench_node *node = &nodes[0]; node; node = node->next)
            sum += node->value;
        BENCH_SINK_INT(sum);
    }
}

BENCH(linked_list_traverse_1000, 100000) {
    static struct bench_node nodes[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 999; i++) {
            nodes[i].value = i;
            nodes[i].next = &nodes[i + 1];
        }
        nodes[999].value = 999;
        nodes[999].next = NULL;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (struct bench_node *node = &nodes[0]; node; node = node->next)
            sum += node->value;
        BENCH_SINK_INT(sum);
    }
}


/* ===== Hash table lookup benchmark (port hash pattern) ===== */
struct bench_porttab {
    int port;
    char *name;
    struct bench_porttab *next;
};

BENCH(hash_lookup_hit, 5000000) {
    static struct bench_porttab entries[10];
    static struct bench_porttab *buckets[PORTHASHBUCKETS];
    static int initialized = 0;
    static int ports[] = {22, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017, 53};

    if (!initialized) {
        memset(buckets, 0, sizeof(buckets));
        for (int i = 0; i < 10; i++) {
            entries[i].port = ports[i];
            entries[i].name = "service";
            int hash_bucket = HASHPORT(ports[i]);
            entries[i].next = buckets[hash_bucket];
            buckets[hash_bucket] = &entries[i];
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int port_num = ports[i % 10];
        int hash_bucket = HASHPORT(port_num);
        struct bench_porttab *port_entry;
        for (port_entry = buckets[hash_bucket]; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == port_num) break;
        }
        BENCH_SINK_PTR(port_entry);
    }
}

BENCH(hash_lookup_miss, 5000000) {
    static struct bench_porttab entries[10];
    static struct bench_porttab *buckets[PORTHASHBUCKETS];
    static int initialized = 0;
    static int ports[] = {22, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017, 53};

    if (!initialized) {
        memset(buckets, 0, sizeof(buckets));
        for (int i = 0; i < 10; i++) {
            entries[i].port = ports[i];
            entries[i].name = "service";
            int hash_bucket = HASHPORT(ports[i]);
            entries[i].next = buckets[hash_bucket];
            buckets[hash_bucket] = &entries[i];
        }
        initialized = 1;
    }
    /* Look up ports that don't exist */
    for (int i = 0; i < bf_iters; i++) {
        int port_num = 60000 + (i % 1000);
        int hash_bucket = HASHPORT(port_num);
        struct bench_porttab *port_entry;
        for (port_entry = buckets[hash_bucket]; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == port_num) break;
        }
        BENCH_SINK_PTR(port_entry);
    }
}

/* ===== Field ID lookup benchmark ===== */
BENCH(field_id_lookup, 10000000) {
    char fields[] = "acdfginoprstunR";
    int num_fields = (int)strlen(fields);
    for (int i = 0; i < bf_iters; i++) {
        char target = fields[i % num_fields];
        int found = 0;
        for (int j = 0; j < num_fields; j++) {
            if (fields[j] == target) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

/* ===== FD status check: linked list traversal patterns ===== */
struct bench_fd_lst {
    char *nm;
    int lo, hi;
    struct bench_fd_lst *next;
};

BENCH(fd_status_check_numeric, 5000000) {
    static struct bench_fd_lst nodes[20];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 20; i++) {
            nodes[i].nm = NULL;
            nodes[i].lo = nodes[i].hi = i * 5;
            nodes[i].next = (i < 19) ? &nodes[i + 1] : NULL;
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int num = (i % 100);
        int found = 0;
        for (struct bench_fd_lst *fp = &nodes[0]; fp; fp = fp->next) {
            if (num >= fp->lo && num <= fp->hi) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

BENCH(fd_status_check_named, 5000000) {
    static struct bench_fd_lst nodes[10];
    static char *names[] = {"cwd", "rtd", "txt", "mem", "DEL", "0", "1", "2", "3", "4"};
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 10; i++) {
            nodes[i].nm = names[i];
            nodes[i].lo = 1;
            nodes[i].hi = 0;
            nodes[i].next = (i < 9) ? &nodes[i + 1] : NULL;
        }
        initialized = 1;
    }
    char *targets[] = {"cwd", "txt", "4", "mem", "99"};
    for (int i = 0; i < bf_iters; i++) {
        char *nm = targets[i % 5];
        int found = 0;
        for (struct bench_fd_lst *fp = &nodes[0]; fp; fp = fp->next) {
            if (fp->nm && strcmp(fp->nm, nm) == 0) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

/* ===== Process iteration: array vs pointer chasing ===== */
BENCH(array_iteration_1000, 1000000) {
    static int data[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) data[i] = i;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (int i = 0; i < 1000; i++) sum += data[i];
        BENCH_SINK_INT(sum);
    }
}

BENCH(pointer_chase_1000, 1000000) {
    /* Simulate linked list pointer chasing (cache-unfriendly) */
    static struct bench_node nodes[1000];
    static int initialized = 0;
    if (!initialized) {
        /* Shuffle order to simulate real linked list (non-sequential memory) */
        int order[1000];
        for (int i = 0; i < 1000; i++) order[i] = i;
        for (int i = 999; i > 0; i--) {
            int j = i / 2; /* deterministic shuffle */
            int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }
        for (int i = 0; i < 999; i++) {
            nodes[order[i]].value = order[i];
            nodes[order[i]].next = &nodes[order[i + 1]];
        }
        nodes[order[999]].value = order[999];
        nodes[order[999]].next = NULL;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (struct bench_node *n = &nodes[0]; n; n = n->next)
            sum += n->value;
        BENCH_SINK_INT(sum);
    }
}

/* ===== hash table with deeper collision chains ===== */
BENCH(hash_lookup_deep_chain, 2000000) {
    /* Simulate a bucket with 50 entries chained (worst case) */
    static struct bench_porttab entries[50];
    static struct bench_porttab *head = NULL;
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 50; i++) {
            entries[i].port = i * 100;
            entries[i].name = "service";
            entries[i].next = head;
            head = &entries[i];
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        /* Search for entry near the end of the chain */
        int target = (i % 50) * 100;
        struct bench_porttab *port_entry;
        for (port_entry = head; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == target) break;
        }
        BENCH_SINK_PTR(port_entry);
    }
}

/* ===== Duplicate device removal benchmark ===== */
BENCH(rmdupdev_100, 100000) {
    /* Simulate removing dups from sorted device array */
    static struct { unsigned long rdev; unsigned long inode; } devs[100];
    for (int i = 0; i < bf_iters; i++) {
        /* populate with ~30% dups */
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


BF_SECTIONS("DATA STRUCTURES")

RUN_BENCHMARKS()
