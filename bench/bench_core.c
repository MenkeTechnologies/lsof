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
    for (r = 0; s < cp; s++) {
        r = r << 4;
        if (isdigit((unsigned char)*s))
            r |= (unsigned char)(*s - '0') & 0xf;
        else if (isupper((unsigned char)*s))
            r |= ((unsigned char)(*s - 'A') + 10) & 0xf;
        else
            r |= ((unsigned char)(*s - 'a') + 10) & 0xf;
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
            snprintf(safepup_buf, sizeof(safepup_buf), "^%c", c + 0x40);
            rp = safepup_buf;
        }
        len = 2;
    } else if (c == 0xff) {
        rp = "^?";
        len = 2;
    } else {
        snprintf(safepup_buf, sizeof(safepup_buf), "\\x%02x", (int)(c & 0xff));
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


RUN_BENCHMARKS();
