#include "bench.h"

#include <ctype.h>

#undef BF_SECTION
#define BF_SECTION "STRING OPERATIONS"

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
            if (isprint((unsigned char)*p))
                count++;
        }
        BENCH_SINK_INT(count);
    }
}

/* ===== strncmp prefix match benchmark (command matching pattern) ===== */
BENCH(strncmp_prefix_match, 10000000) {
    const char *cmds[] = {"systemd",  "sshd",  "bash",   "python3", "node",
                          "postgres", "nginx", "docker", "java",    "chrome"};
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

/* ===== strtol benchmark (numeric string parsing in lsof) ===== */
BENCH(strtol_pid, 10000000) {
    char *pids[] = {"1", "42", "1337", "65535", "100000"};
    for (int i = 0; i < bf_iters; i++) {
        long v = strtol(pids[i % 5], NULL, 10);
        BENCH_SINK_INT((int)v);
    }
}

BENCH(strtol_hex, 10000000) {
    char *hexs[] = {"ff", "1a2b", "deadbeef", "cafe", "0"};
    for (int i = 0; i < bf_iters; i++) {
        long v = strtol(hexs[i % 5], NULL, 16);
        BENCH_SINK_INT((int)v);
    }
}

/* ===== command name truncation (strncpy pattern used in lsof) ===== */
BENCH(strncpy_cmd_short, 10000000) {
    char dst[16];
    char *cmds[] = {"bash", "sh", "sshd", "node", "python3"};
    for (int i = 0; i < bf_iters; i++) {
        strncpy(dst, cmds[i % 5], 15);
        dst[15] = '\0';
        BENCH_SINK_PTR(dst);
    }
}

BENCH(strncpy_cmd_long, 5000000) {
    char dst[16];
    char *cmds[] = {"chromium-browser-stable", "gnome-terminal-server", "firefox-developer-edition",
                    "docker-containerd-shim", "systemd-resolved.service"};
    for (int i = 0; i < bf_iters; i++) {
        strncpy(dst, cmds[i % 5], 15);
        dst[15] = '\0';
        BENCH_SINK_PTR(dst);
    }
}

/* ===== FD string parsing benchmark ===== */
BENCH(fd_parse_strtol, 10000000) {
    const char *fds[] = {"0", "1", "2", "42", "255", "1024", "cwd", "txt", "mem", "rtd"};
    for (int i = 0; i < bf_iters; i++) {
        const char *s = fds[i % 10];
        char *end;
        long v = strtol(s, &end, 10);
        BENCH_SINK_INT(*end == '\0' ? (int)v : -1);
    }
}

BENCH(fd_classify, 10000000) {
    const char *fds[] = {"0", "1", "2", "42", "255", "1024", "cwd", "txt", "mem", "rtd"};
    for (int i = 0; i < bf_iters; i++) {
        const char *s = fds[i % 10];
        int numeric = (s[0] >= '0' && s[0] <= '9');
        BENCH_SINK_INT(numeric);
    }
}

/* ===== IPv6 address detection benchmark ===== */
BENCH(ipv6_colon_count, 5000000) {
    const char *addrs[] = {
        "2001:0db8:85a3:0000:0000:8a2e:0370:7334", "::1", "fe80::1%lo0", "192.168.1.1", "10.0.0.1",
    };
    for (int i = 0; i < bf_iters; i++) {
        const char *a = addrs[i % 5];
        int colons = 0;
        for (const char *p = a; *p; p++)
            if (*p == ':')
                colons++;
        BENCH_SINK_INT(colons);
    }
}

/* ===== Path classification benchmark ===== */
BENCH(path_is_absolute, 10000000) {
    const char *paths[] = {"/dev/null", "/proc/1/fd/0", "relative",       "./local",
                           "/",         "../up",        "/var/log/syslog"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(paths[i % 7][0] == '/');
    }
}

BENCH(path_depth_count, 5000000) {
    const char *paths[] = {"/", "/dev/null", "/proc/1234/fd/0", "/usr/local/share/man/man1/lsof.1"};
    for (int i = 0; i < bf_iters; i++) {
        const char *p = paths[i % 4];
        int depth = 0;
        for (; *p; p++)
            if (*p == '/')
                depth++;
        BENCH_SINK_INT(depth);
    }
}

BENCH(path_basename_search, 5000000) {
    const char *paths[] = {"/dev/null", "/proc/1234/fd/0", "/usr/bin/lsof", "/var/log/syslog.1.gz"};
    for (int i = 0; i < bf_iters; i++) {
        const char *p = paths[i % 4];
        const char *last_slash = p;
        for (; *p; p++)
            if (*p == '/')
                last_slash = p;
        BENCH_SINK_PTR((void *)last_slash);
    }
}

/* ===== Process name matching benchmark ===== */
BENCH(cmd_exact_match, 10000000) {
    const char *cmds[] = {"sshd", "nginx",   "postgres", "lsof",
                          "bash", "systemd", "init",     "kworker"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strcmp(cmds[i % 8], "lsof") == 0);
    }
}

BENCH(cmd_prefix_match, 10000000) {
    const char *cmds[] = {"sshd", "nginx",   "postgres", "lsof",
                          "bash", "systemd", "init",     "kworker"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strncmp(cmds[i % 8], "ls", 2) == 0);
    }
}

/* ===== memmove vs memcpy (overlapping vs non-overlapping) ===== */
BENCH(memmove_64, 10000000) {
    char buf[128];
    memset(buf, 'A', 128);
    for (int i = 0; i < bf_iters; i++) {
        memmove(buf + 32, buf, 64);
        BENCH_SINK_PTR(buf);
    }
}

/* ===== strchr vs manual scan ===== */
BENCH(strchr_colon, 10000000) {
    const char *addrs[] = {"192.168.1.1:8080", "10.0.0.1:443", "0.0.0.0:22", "127.0.0.1:3306"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_PTR((void *)strchr(addrs[i % 4], ':'));
    }
}

BENCH(manual_colon_scan, 10000000) {
    const char *addrs[] = {"192.168.1.1:8080", "10.0.0.1:443", "0.0.0.0:22", "127.0.0.1:3306"};
    for (int i = 0; i < bf_iters; i++) {
        const char *p = addrs[i % 4];
        while (*p && *p != ':')
            p++;
        BENCH_SINK_PTR((void *)p);
    }
}

/* ===== tolower loop vs bitwise OR ===== */
BENCH(tolower_loop, 10000000) {
    char buf[8];
    const char *protos[] = {"TCP", "UDP", "Tcp", "Udp", "tcp"};
    for (int i = 0; i < bf_iters; i++) {
        const char *s = protos[i % 5];
        for (int j = 0; j < 3; j++)
            buf[j] = (char)tolower((unsigned char)s[j]);
        buf[3] = '\0';
        BENCH_SINK_INT(buf[0]);
    }
}

BF_SECTIONS("STRING OPERATIONS")

RUN_BENCHMARKS()
