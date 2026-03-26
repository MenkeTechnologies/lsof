#include "bench.h"

#include <time.h>

#undef BF_SECTION
#define BF_SECTION "OUTPUT & FORMATTING"

/* ===== safe_print_unprintable (duplicated, needed for safestrprt_dirty) ===== */
static char unprintable_buf[8];
static char *bench_safe_print_unprintable(unsigned int char_code, int *output_length) {
    int encoded_len;
    char *result_str;
    if (char_code < 0x20) {
        switch (char_code) {
        case '\b':
            result_str = "\\b";
            break;
        case '\f':
            result_str = "\\f";
            break;
        case '\n':
            result_str = "\\n";
            break;
        case '\r':
            result_str = "\\r";
            break;
        case '\t':
            result_str = "\\t";
            break;
        default:
            unprintable_buf[0] = '^';
            unprintable_buf[1] = (char)(char_code + 0x40);
            unprintable_buf[2] = '\0';
            result_str = unprintable_buf;
        }
        encoded_len = 2;
    } else if (char_code == 0xff) {
        result_str = "^?";
        encoded_len = 2;
    } else {
        {
            static const char hex_digits[] = "0123456789abcdef";
            unsigned int byte_val = char_code & 0xff;
            unprintable_buf[0] = '\\';
            unprintable_buf[1] = 'x';
            unprintable_buf[2] = hex_digits[(byte_val >> 4) & 0xf];
            unprintable_buf[3] = hex_digits[byte_val & 0xf];
            unprintable_buf[4] = '\0';
        }
        result_str = unprintable_buf;
        encoded_len = 4;
    }
    if (output_length)
        *output_length = encoded_len;
    return result_str;
}

/* ===== safestrprt benchmark (safe printing to stream) ===== */
BENCH(safestrprt_clean, 500000) {
    FILE *f = fopen("/dev/null", "w");
    if (!f)
        return;
    for (int i = 0; i < bf_iters; i++) {
        char *sp = "/usr/local/bin/lsof";
        for (; *sp; sp++)
            putc((int)(*sp & 0xff), f);
    }
    fclose(f);
}

BENCH(safestrprt_dirty, 500000) {
    FILE *f = fopen("/dev/null", "w");
    if (!f)
        return;
    char *test = "/usr/bin/test\twith\ttabs\nand\x80\xfe";
    for (int i = 0; i < bf_iters; i++) {
        for (char *sp = test; *sp; sp++) {
            unsigned char c = (unsigned char)*sp;
            if (c >= 0x20 && c <= 0x7e)
                putc((int)c, f);
            else {
                int cl;
                char *r = bench_safe_print_unprintable((unsigned int)c, &cl);
                fputs(r, f);
            }
        }
    }
    fclose(f);
}

/* ===== strftime benchmark (time formatting for lsof -T output) ===== */
BENCH(strftime_default, 500000) {
    char buf[128];
    time_t tm = time(NULL);
    struct tm *lt = localtime(&tm);
    for (int i = 0; i < bf_iters; i++) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(strftime_epoch, 500000) {
    char buf[128];
    time_t tm = time(NULL);
    struct tm *lt = localtime(&tm);
    for (int i = 0; i < bf_iters; i++) {
        strftime(buf, sizeof(buf), "%s", lt);
        BENCH_SINK_PTR(buf);
    }
}

/* ===== snpf (counted sprintf) vs snprintf benchmark ===== */
BENCH(snpf_short_string, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "%s", "hello");
        BENCH_SINK_INT(buf[0]);
    }
}

BENCH(snpf_pid_format, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "p%d", 12345 + (i & 0xff));
        BENCH_SINK_INT(buf[0]);
    }
}

BENCH(snpf_path_format, 2000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "/proc/%d/fd/%d", 1234 + (i & 0xff), i & 0x3ff);
        BENCH_SINK_INT(buf[0]);
    }
}

BENCH(snpf_hex_device, 5000000) {
    char buf[32];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "0x%lx", (unsigned long)(0xdeadbeef + i));
        BENCH_SINK_INT(buf[0]);
    }
}

/* ===== sprintf PID/FD formatting (common lsof output pattern) ===== */
BENCH(sprintf_pid_field, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "p%d\n", i % 100000);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(sprintf_field_output, 2000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "p%d\nc%s\nf%d\nn%s\n", i % 65536, "bash", i % 256,
                 "/usr/local/bin/lsof");
        BENCH_SINK_PTR(buf);
    }
}

BENCH(manual_field_output, 2000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        char *p = buf;
        /* p<pid>\n */
        *p++ = 'p';
        int v = i % 65536;
        char tmp[16];
        int tl = 0;
        if (v == 0)
            tmp[tl++] = '0';
        else {
            while (v) {
                tmp[tl++] = '0' + (v % 10);
                v /= 10;
            }
        }
        while (tl > 0)
            *p++ = tmp[--tl];
        *p++ = '\n';
        /* c<cmd>\n */
        *p++ = 'c';
        memcpy(p, "bash", 4);
        p += 4;
        *p++ = '\n';
        /* f<fd>\n */
        *p++ = 'f';
        v = i % 256;
        tl = 0;
        if (v == 0)
            tmp[tl++] = '0';
        else {
            while (v) {
                tmp[tl++] = '0' + (v % 10);
                v /= 10;
            }
        }
        while (tl > 0)
            *p++ = tmp[--tl];
        *p++ = '\n';
        /* n<name>\n */
        *p++ = 'n';
        memcpy(p, "/usr/local/bin/lsof", 19);
        p += 19;
        *p++ = '\n';
        *p = '\0';
        BENCH_SINK_PTR(buf);
    }
}

/* ===== Command name truncation at different widths ===== */
BENCH(cmd_truncate_16, 10000000) {
    char dst[17];
    char *cmds[] = {"chromium-browser-stable", "gnome-terminal-server", "bash", "sshd",
                    "python3.11-multiprocessing"};
    for (int i = 0; i < bf_iters; i++) {
        int len = (int)strlen(cmds[i % 5]);
        int w = len < 16 ? len : 16;
        memcpy(dst, cmds[i % 5], (size_t)w);
        dst[w] = '\0';
        BENCH_SINK_PTR(dst);
    }
}

BENCH(cmd_truncate_32, 10000000) {
    char dst[33];
    char *cmds[] = {"chromium-browser-stable", "gnome-terminal-server", "bash", "sshd",
                    "python3.11-multiprocessing"};
    for (int i = 0; i < bf_iters; i++) {
        int len = (int)strlen(cmds[i % 5]);
        int w = len < 32 ? len : 32;
        memcpy(dst, cmds[i % 5], (size_t)w);
        dst[w] = '\0';
        BENCH_SINK_PTR(dst);
    }
}

/* ===== IPv4 address formatting ===== */
BENCH(ipv4_format_snprintf, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d", 192, 168, (i >> 8) & 0xff, i & 0xff);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(ipv4_format_manual, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        char *p = buf;
        int octets[4] = {192, 168, (i >> 8) & 0xff, i & 0xff};
        for (int j = 0; j < 4; j++) {
            int v = octets[j];
            if (v >= 100)
                *p++ = '0' + v / 100;
            if (v >= 10)
                *p++ = '0' + (v / 10) % 10;
            *p++ = '0' + v % 10;
            if (j < 3)
                *p++ = '.';
        }
        *p = '\0';
        BENCH_SINK_PTR(buf);
    }
}

/* ===== Multi-field lsof output line formatting ===== */
BENCH(full_line_snprintf, 2000000) {
    char buf[512];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "%-9s %5d %8s %4s %4s %7s %18s %s", "bash", 1234 + (i % 1000),
                 "root", "3u", "REG", "8,1", "192.168.1.1:8080", "/usr/local/bin/lsof");
        BENCH_SINK_PTR(buf);
    }
}

BF_SECTIONS("OUTPUT & FORMATTING")

RUN_BENCHMARKS()
