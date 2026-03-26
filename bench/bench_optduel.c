#include "bench.h"

#include <ctype.h>
#include <regex.h>

#undef BF_SECTION
#define BF_SECTION "OPTIMIZATION DUELS"

/* ===== safestrprt printable classification benchmark ===== */
/* Compare isprint() vs lookup table for character classification */
BENCH(isprint_per_char, 10000000) {
    unsigned char data[] = "Hello\tWorld\n/usr/local/bin/test\x80\xff";
    int dlen = (int)sizeof(data) - 1;
    for (int i = 0; i < bf_iters; i++) {
        int count = 0;
        for (int j = 0; j < dlen; j++) {
            if (isprint(data[j])) count++;
        }
        BENCH_SINK_INT(count);
    }
}

BENCH(lookup_table_per_char, 10000000) {
    static const unsigned char printable[256] = {
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,
    };
    unsigned char data[] = "Hello\tWorld\n/usr/local/bin/test\x80\xff";
    int dlen = (int)sizeof(data) - 1;
    for (int i = 0; i < bf_iters; i++) {
        int count = 0;
        for (int j = 0; j < dlen; j++) {
            if (printable[data[j]]) count++;
        }
        BENCH_SINK_INT(count);
    }
}


/* ===== x2dev validation: branch vs lookup table ===== */
BENCH(hex_validate_branching, 5000000) {
    char *inputs[] = {"0xdeadbeef", "0x1a2b3c4d", "ff", "0xCAFE", "12ab"};
    for (int i = 0; i < bf_iters; i++) {
        char *s = inputs[i % 5];
        if (strncasecmp(s, "0x", 2) == 0) s += 2;
        int n = 0;
        for (char *cp = s; *cp; cp++, n++) {
            if (isdigit((unsigned char)*cp)) continue;
            if ((unsigned char)*cp >= 'a' && (unsigned char)*cp <= 'f') continue;
            if ((unsigned char)*cp >= 'A' && (unsigned char)*cp <= 'F') continue;
            if (*cp == ' ' || *cp == ',') break;
            break;
        }
        BENCH_SINK_INT(n);
    }
}

BENCH(hex_validate_lookup, 5000000) {
    static const unsigned char cls[256] = {
        ['0']=1,['1']=1,['2']=1,['3']=1,['4']=1,['5']=1,['6']=1,['7']=1,
        ['8']=1,['9']=1,['a']=1,['b']=1,['c']=1,['d']=1,['e']=1,['f']=1,
        ['A']=1,['B']=1,['C']=1,['D']=1,['E']=1,['F']=1,[' ']=2,[',']=2,
    };
    char *inputs[] = {"0xdeadbeef", "0x1a2b3c4d", "ff", "0xCAFE", "12ab"};
    for (int i = 0; i < bf_iters; i++) {
        char *s = inputs[i % 5];
        if (strncasecmp(s, "0x", 2) == 0) s += 2;
        int n = 0;
        for (char *cp = s; *cp; cp++, n++) {
            unsigned char c = cls[(unsigned char)*cp];
            if (c == 1) continue;
            break;
        }
        BENCH_SINK_INT(n);
    }
}


/* ===== Integer to string: snprintf vs manual ===== */
BENCH(itoa_snprintf, 5000000) {
    char buf[32];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "%d", i % 65536);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(itoa_manual, 5000000) {
    char buf[32];
    for (int i = 0; i < bf_iters; i++) {
        int v = i % 65536;
        char *ep = &buf[31];
        *ep = '\0';
        if (v == 0) { *--ep = '0'; }
        else { do { *--ep = '0' + (v % 10); v /= 10; } while (v); }
        BENCH_SINK_PTR(ep);
    }
}


/* ===== memcpy vs snpf for short fixed strings ===== */
BENCH(arrow_snprintf, 10000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        char *np = buf;
        snprintf(np, 64, "->");
        np += 2;
        BENCH_SINK_PTR(np);
    }
}

BENCH(arrow_direct, 10000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        char *np = buf;
        *np++ = '-';
        *np++ = '>';
        BENCH_SINK_PTR(np);
    }
}


/* ===== Regex matching benchmark (is_cmd_excl pattern) ===== */
BENCH(regex_match_simple, 500000) {
    static regex_t rx;
    static int compiled = 0;
    if (!compiled) {
        regcomp(&rx, "^ssh", REG_NOSUB);
        compiled = 1;
    }
    char *cmds[] = {"sshd", "bash", "python3", "sshd-keygen", "node",
                    "nginx", "ssh-agent", "systemd", "docker", "sshfs"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(regexec(&rx, cmds[i % 10], 0, NULL, 0));
    }
}

BENCH(strncmp_vs_regex, 500000) {
    /* Same matching task but with strncmp instead of regex */
    char *cmds[] = {"sshd", "bash", "python3", "sshd-keygen", "node",
                    "nginx", "ssh-agent", "systemd", "docker", "sshfs"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strncmp(cmds[i % 10], "ssh", 3));
    }
}

/* ===== safestrlen: original vs optimized comparison ===== */
BENCH(safestrlen_original, 5000000) {
    /* Original implementation using isprint() */
    char *test = "/usr/local/bin/test\twith\ttabs\nand\nnewlines\x80\xfe";
    for (int i = 0; i < bf_iters; i++) {
        int len = 0;
        for (char *sp = test; *sp; sp++) {
            if (!isprint((unsigned char)*sp)) {
                if (*sp < 0x20 || (unsigned char)*sp == 0xff)
                    len += 2;
                else
                    len += 4;
            } else
                len++;
        }
        BENCH_SINK_INT(len);
    }
}

BENCH(safestrlen_lookup_table, 5000000) {
    /* Optimized implementation using lookup table */
    static const unsigned char explen[256] = {
        2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
        2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,
        4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,
        2,
    };
    char *test = "/usr/local/bin/test\twith\ttabs\nand\nnewlines\x80\xfe";
    for (int i = 0; i < bf_iters; i++) {
        int len = 0;
        for (unsigned char *sp = (unsigned char *)test; *sp; sp++) {
            len += explen[*sp];
        }
        BENCH_SINK_INT(len);
    }
}

/* ===== strncasecmp vs manual lowercase compare ===== */
BENCH(strncasecmp_call, 10000000) {
    char *protos[] = {"TCP", "tcp", "UDP", "udp", "Tcp", "Udp",
                      "tcP", "uDP", "TCp", "UdP"};
    for (int i = 0; i < bf_iters; i++) {
        int r = strncasecmp(protos[i % 10], "tcp", 3);
        BENCH_SINK_INT(r);
    }
}

BENCH(manual_lowercase_cmp, 10000000) {
    char *protos[] = {"TCP", "tcp", "UDP", "udp", "Tcp", "Udp",
                      "tcP", "uDP", "TCp", "UdP"};
    for (int i = 0; i < bf_iters; i++) {
        char *s = protos[i % 10];
        int r = ((s[0] | 0x20) == 't' && (s[1] | 0x20) == 'c' && (s[2] | 0x20) == 'p') ? 0 : 1;
        BENCH_SINK_INT(r);
    }
}

/* ===== path building: snprintf vs manual concat ===== */
BENCH(path_snprintf, 5000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "/proc/%d/fd/%d", i % 65536, i % 256);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(path_manual, 5000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        char *p = buf;
        memcpy(p, "/proc/", 6); p += 6;
        /* pid */
        int v = i % 65536;
        char tmp[16]; int tl = 0;
        if (v == 0) tmp[tl++] = '0';
        else { while (v) { tmp[tl++] = '0' + (v % 10); v /= 10; } }
        while (tl > 0) *p++ = tmp[--tl];
        memcpy(p, "/fd/", 4); p += 4;
        /* fd */
        v = i % 256; tl = 0;
        if (v == 0) tmp[tl++] = '0';
        else { while (v) { tmp[tl++] = '0' + (v % 10); v /= 10; } }
        while (tl > 0) *p++ = tmp[--tl];
        *p = '\0';
        BENCH_SINK_PTR(buf);
    }
}


BF_SECTIONS("OPTIMIZATION DUELS")

RUN_BENCHMARKS()
