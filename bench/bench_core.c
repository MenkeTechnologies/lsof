/*
 * bench_core.c - benchmarks for lsof core operations
 *
 * Measures performance of key internal algorithms and operations.
 */

#include "bench_framework.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../src/lsof_fields.h"

/* ===== x2dev() benchmark ===== */
static char *
bench_x2dev(char *s, dev_t *d)
{
    char *cp, *cp1;
    int n;
    dev_t r;

    if (strncasecmp(s, "0x", 2) == 0)
        s += 2;
    for (cp = s, n = 0; *cp; cp++, n++) {
        if (isdigit((unsigned char)*cp)) continue;
        if ((unsigned char)*cp >= 'a' && (unsigned char)*cp <= 'f') continue;
        if ((unsigned char)*cp >= 'A' && (unsigned char)*cp <= 'F') continue;
        if (*cp == ' ' || *cp == ',') break;
        return NULL;
    }
    if (!n) return NULL;
    if (n > (2 * (int)sizeof(dev_t))) {
        cp1 = s;
        s += (n - (2 * sizeof(dev_t)));
        while (cp1 < s) {
            if (*cp1 != 'f' && *cp1 != 'F') return NULL;
            cp1++;
        }
    }
    {
        static const signed char hv[256] = {
            ['0']=0, ['1']=1, ['2']=2, ['3']=3, ['4']=4,
            ['5']=5, ['6']=6, ['7']=7, ['8']=8, ['9']=9,
            ['a']=10,['b']=11,['c']=12,['d']=13,['e']=14,['f']=15,
            ['A']=10,['B']=11,['C']=12,['D']=13,['E']=14,['F']=15,
        };
        for (r = 0; s < cp; s++) {
            r = (r << 4) | (hv[(unsigned char)*s] & 0xf);
        }
    }
    *d = r;
    return s;
}

BENCH(x2dev_short, 1000000) {
    dev_t d;
    for (int i = 0; i < bf_iters; i++) {
        bench_x2dev("ff", &d);
        BENCH_SINK_INT(d);
    }
}

BENCH(x2dev_with_prefix, 1000000) {
    dev_t d;
    for (int i = 0; i < bf_iters; i++) {
        bench_x2dev("0x1a2b3c4d", &d);
        BENCH_SINK_INT(d);
    }
}

BENCH(x2dev_long_hex, 1000000) {
    dev_t d;
    for (int i = 0; i < bf_iters; i++) {
        bench_x2dev("0xdeadbeefcafe1234", &d);
        BENCH_SINK_INT(d);
    }
}


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


/* ===== safestrlen benchmark ===== */
static int
bench_safestrlen(char *sp, int flags)
{
    char c;
    int len = 0;
    c = (flags & 2) ? ' ' : '\0';
    if (sp) {
        for (; *sp; sp++) {
            if (!isprint((unsigned char)*sp) || *sp == c) {
                if (*sp < 0x20 || (unsigned char)*sp == 0xff)
                    len += 2;
                else
                    len += 4;
            } else
                len++;
        }
    }
    return len;
}

BENCH(safestrlen_short, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safestrlen("hello world", 0));
    }
}

BENCH(safestrlen_with_escapes, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safestrlen("hello\tworld\n", 0));
    }
}

BENCH(safestrlen_long_path, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safestrlen(
            "/usr/local/share/applications/very/long/path/to/some/file.txt", 0));
    }
}

BENCH(safestrlen_binary_data, 2000000) {
    char data[64];
    for (int i = 0; i < 63; i++) data[i] = (char)(i + 1);
    data[63] = '\0';
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safestrlen(data, 0));
    }
}


/* ===== compdev (qsort) benchmark ===== */
typedef unsigned long bench_INODETYPE;
struct bench_l_dev {
    dev_t rdev;
    bench_INODETYPE inode;
    char *name;
    int v;
};

static int
bench_compdev(const void *a1, const void *a2)
{
    struct bench_l_dev **p1 = (struct bench_l_dev **)a1;
    struct bench_l_dev **p2 = (struct bench_l_dev **)a2;
    if ((*p1)->rdev < (*p2)->rdev) return -1;
    if ((*p1)->rdev > (*p2)->rdev) return 1;
    if ((*p1)->inode < (*p2)->inode) return -1;
    if ((*p1)->inode > (*p2)->inode) return 1;
    return strcmp((*p1)->name, (*p2)->name);
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
        qsort(ptrs, 100, sizeof(struct bench_l_dev *), bench_compdev);
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
        qsort(ptrs, 1000, sizeof(struct bench_l_dev *), bench_compdev);
        BENCH_SINK_PTR(ptrs[0]);
    }
}


/* ===== safepup benchmark ===== */
static char safepup_buf[8];
static char *
bench_safepup(unsigned int c, int *cl)
{
    int len;
    char *rp;
    if (c < 0x20) {
        switch (c) {
        case '\b': rp = "\\b"; break;
        case '\f': rp = "\\f"; break;
        case '\n': rp = "\\n"; break;
        case '\r': rp = "\\r"; break;
        case '\t': rp = "\\t"; break;
        default:
            safepup_buf[0] = '^';
            safepup_buf[1] = (char)(c + 0x40);
            safepup_buf[2] = '\0';
            rp = safepup_buf;
        }
        len = 2;
    } else if (c == 0xff) {
        rp = "^?";
        len = 2;
    } else {
        {
            static const char hex[] = "0123456789abcdef";
            unsigned int v = c & 0xff;
            safepup_buf[0] = '\\';
            safepup_buf[1] = 'x';
            safepup_buf[2] = hex[(v >> 4) & 0xf];
            safepup_buf[3] = hex[v & 0xf];
            safepup_buf[4] = '\0';
        }
        rp = safepup_buf;
        len = 4;
    }
    if (cl) *cl = len;
    return rp;
}

BENCH(safepup_control_chars, 10000000) {
    int cl;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_PTR(bench_safepup((unsigned int)(i % 0x20), &cl));
    }
}

BENCH(safepup_high_bytes, 5000000) {
    int cl;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_PTR(bench_safepup(0x80 + (unsigned int)(i % 0x7f), &cl));
    }
}


/* ===== I/O benchmarks ===== */
BENCH(open_close_file, 100000) {
    for (int i = 0; i < bf_iters; i++) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd >= 0) close(fd);
        BENCH_SINK_INT(fd);
    }
}

BENCH(stat_file, 100000) {
    struct stat st;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(stat("/dev/null", &st));
    }
}

BENCH(getpid_call, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(getpid());
    }
}


/* ===== String operation benchmarks (common lsof patterns) ===== */
BENCH(strlen_short, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strlen("hello"));
    }
}

BENCH(strlen_path, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strlen("/usr/local/share/applications/file.txt"));
    }
}

BENCH(strcmp_match, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strcmp("TCP", "TCP"));
    }
}

BENCH(strcmp_mismatch, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strcmp("TCP", "UDP"));
    }
}

BENCH(snprintf_format, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "0x%08x", i);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(isprint_scan, 5000000) {
    char str[] = "Hello, World! This is a test string 12345";
    for (int j = 0; j < bf_iters; j++) {
        int count = 0;
        for (char *p = str; *p; p++) {
            if (isprint((unsigned char)*p)) count++;
        }
        BENCH_SINK_INT(count);
    }
}


/* ===== Socket creation benchmark (relevant to lsof socket detection) ===== */
BENCH(socket_create_close, 50000) {
    for (int i = 0; i < bf_iters; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd >= 0) close(fd);
        BENCH_SINK_INT(fd);
    }
}


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


/* ===== String copy benchmarks (mkstrcpy pattern) ===== */
static char *bench_mkstrcpy(const char *src) {
    size_t len = src ? strlen(src) : 0;
    char *ns = (char *)malloc(len + 1);
    if (ns) {
        if (src) memcpy(ns, src, len + 1);
        else *ns = '\0';
    }
    return ns;
}

BENCH(mkstrcpy_short, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcpy("hello");
        BENCH_SINK_PTR(s);
        free(s);
    }
}

BENCH(mkstrcpy_path, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcpy("/usr/local/share/applications/very/long/path/to/some/file.txt");
        BENCH_SINK_PTR(s);
        free(s);
    }
}

BENCH(mkstrcpy_null, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcpy(NULL);
        BENCH_SINK_PTR(s);
        free(s);
    }
}


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
        for (struct bench_node *n = &nodes[0]; n; n = n->next)
            sum += n->value;
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
        for (struct bench_node *n = &nodes[0]; n; n = n->next)
            sum += n->value;
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
            int h = HASHPORT(ports[i]);
            entries[i].next = buckets[h];
            buckets[h] = &entries[i];
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int p = ports[i % 10];
        int h = HASHPORT(p);
        struct bench_porttab *pt;
        for (pt = buckets[h]; pt; pt = pt->next) {
            if (pt->port == p) break;
        }
        BENCH_SINK_PTR(pt);
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
            int h = HASHPORT(ports[i]);
            entries[i].next = buckets[h];
            buckets[h] = &entries[i];
        }
        initialized = 1;
    }
    /* Look up ports that don't exist */
    for (int i = 0; i < bf_iters; i++) {
        int p = 60000 + (i % 1000);
        int h = HASHPORT(p);
        struct bench_porttab *pt;
        for (pt = buckets[h]; pt; pt = pt->next) {
            if (pt->port == p) break;
        }
        BENCH_SINK_PTR(pt);
    }
}


/* ===== PID binary search benchmark (comppid / qsort + bsearch) ===== */
static int bench_comppid(const void *a, const void *b) {
    int pa = *(const int *)a;
    int pb = *(const int *)b;
    if (pa < pb) return -1;
    if (pa > pb) return 1;
    return 0;
}

BENCH(pid_sort_1000, 50000) {
    static int pids[1000];
    for (int j = 0; j < bf_iters; j++) {
        for (int i = 0; i < 1000; i++) pids[i] = 1000 - i;
        qsort(pids, 1000, sizeof(int), bench_comppid);
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
        int *found = (int *)bsearch(&key, pids, 1000, sizeof(int), bench_comppid);
        BENCH_SINK_PTR(found);
    }
}


/* ===== Field ID lookup benchmark ===== */
BENCH(field_id_lookup, 10000000) {
    char fields[] = "acdfginoprstunR";
    int nf = (int)strlen(fields);
    for (int i = 0; i < bf_iters; i++) {
        char target = fields[i % nf];
        int found = 0;
        for (int j = 0; j < nf; j++) {
            if (fields[j] == target) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}


/* ===== strncmp prefix match benchmark (command matching pattern) ===== */
BENCH(strncmp_prefix_match, 10000000) {
    const char *cmds[] = {"systemd", "sshd", "bash", "python3", "node",
                          "postgres", "nginx", "docker", "java", "chrome"};
    for (int i = 0; i < bf_iters; i++) {
        const char *cmd = cmds[i % 10];
        int match = (strncmp(cmd, "ssh", 3) == 0);
        BENCH_SINK_INT(match);
    }
}

BENCH(strcasecmp_proto, 10000000) {
    const char *protos[] = {"TCP", "tcp", "UDP", "udp", "Tcp", "Udp"};
    for (int i = 0; i < bf_iters; i++) {
        int is_tcp = (strcasecmp(protos[i % 6], "tcp") == 0);
        BENCH_SINK_INT(is_tcp);
    }
}


/* ===== readdir benchmark (relevant to device/directory scanning) ===== */
#include <dirent.h>

BENCH(readdir_dev, 1000) {
    for (int i = 0; i < bf_iters; i++) {
        int count = 0;
        DIR *d = opendir("/dev");
        if (d) {
            struct dirent *e;
            while ((e = readdir(d)) != NULL) count++;
            closedir(d);
        }
        BENCH_SINK_INT(count);
    }
}


/* ===== lstat benchmark (relevant to Readlink/path resolution) ===== */
BENCH(lstat_file, 100000) {
    struct stat st;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(lstat("/dev/null", &st));
    }
}


/* ===== pipe create/close benchmark ===== */
BENCH(pipe_create_close, 100000) {
    for (int i = 0; i < bf_iters; i++) {
        int fds[2];
        if (pipe(fds) == 0) {
            close(fds[0]);
            close(fds[1]);
        }
        BENCH_SINK_INT(fds[0]);
    }
}


RUN_BENCHMARKS();
