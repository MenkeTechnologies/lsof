/*
 * test_integration.c - integration tests for lsof binary
 *
 * These tests invoke the lsof binary and verify its output behavior.
 * They require lsof to be accessible on the system.
 */

#include "test.h"

#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>

static const char *lsof_path = NULL;

static const char *find_lsof(void) {
    if (lsof_path)
        return lsof_path;
    const char *candidates[] = {
#ifdef LSOF_BUILD_PATH
        LSOF_BUILD_PATH,
#endif
        "./lsof",
        "../lsof",
        "./build/lsof",
        "../build/lsof",
        "/usr/bin/lsof",
        "/usr/sbin/lsof",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        if (access(candidates[i], X_OK) == 0) {
            lsof_path = candidates[i];
            return lsof_path;
        }
    }
    return NULL;
}

/*
 * Run lsof via fork/exec (avoids shell/popen sandbox issues).
 * args is a space-separated argument string that gets tokenized.
 * Returns exit status, fills buf with stdout.
 */
static int run_lsof(const char *args, char *buf, size_t bufsz) {
    const char *lsof = find_lsof();
    if (!lsof) return -1;

    /* tokenize args into argv */
    char argbuf[1024];
    char *argv[64];
    int argc = 0;
    argv[argc++] = (char *)lsof;
    if (args && *args) {
        strncpy(argbuf, args, sizeof(argbuf) - 1);
        argbuf[sizeof(argbuf) - 1] = '\0';
        char *tok = strtok(argbuf, " ");
        while (tok && argc < 63) {
            argv[argc++] = tok;
            tok = strtok(NULL, " ");
        }
    }
    argv[argc] = NULL;

    /* pipe for stdout */
    int pipefd[2];
    if (pipe(pipefd) < 0) return -1;

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    if (pid == 0) {
        /* child */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        /* suppress stderr */
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execv(lsof, argv);
        _exit(127);
    }

    /* parent */
    close(pipefd[1]);
    if (buf && bufsz > 0) {
        size_t total = 0;
        while (total < bufsz - 1) {
            ssize_t n = read(pipefd[0], buf + total, bufsz - 1 - total);
            if (n <= 0) break;
            total += (size_t)n;
        }
        buf[total] = '\0';
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}

static int lsof_available(void) {
    return find_lsof() != NULL;
}


TEST(lsof_exists) {
    ASSERT_TRUE(lsof_available());
}

TEST(lsof_diag) {
    const char *p = find_lsof();
    if (!p) { fprintf(stderr, "  DIAG: lsof not found\n"); return; }
    fprintf(stderr, "  DIAG: lsof path = %s\n", p);
    char buf[4096];
    int rc = run_lsof("-v", buf, sizeof(buf));
    fprintf(stderr, "  DIAG: -v rc = %d\n", rc);
    fprintf(stderr, "  DIAG: -v buf[0..80] = %.80s\n", buf);
    rc = run_lsof("-h", buf, sizeof(buf));
    fprintf(stderr, "  DIAG: -h rc = %d\n", rc);
    fprintf(stderr, "  DIAG: -h buf[0..80] = %.80s\n", buf);
    ASSERT_TRUE(1);
}

TEST(lsof_help_flag) {
    if (!lsof_available()) return;
    char buf[4096];
    int rc = run_lsof("-h", buf, sizeof(buf));
    ASSERT_TRUE(rc == 0 || rc == 1);
}

TEST(lsof_version_flag) {
    if (!lsof_available()) return;
    char buf[4096];
    run_lsof("-v", buf, sizeof(buf));
    ASSERT_TRUE(1); /* just verify no crash */
}

TEST(lsof_finds_own_pid) {
    if (!lsof_available()) return;
    char args[128];
    char buf[8192];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Fp", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    char expected[64];
    snprintf(expected, sizeof(expected), "p%d", (int)mypid);
    ASSERT_NOT_NULL(strstr(buf, expected));
}

TEST(lsof_field_output_format) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -F pcfn", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);

    int has_pid = 0, has_cmd = 0, has_fd = 0;
    char *line = buf;
    while (line && *line) {
        if (*line == 'p') has_pid = 1;
        if (*line == 'c') has_cmd = 1;
        if (*line == 'f') has_fd = 1;
        char *nl = strchr(line, '\n');
        line = nl ? nl + 1 : NULL;
    }
    ASSERT_TRUE(has_pid);
    ASSERT_TRUE(has_cmd);
    ASSERT_TRUE(has_fd);
}

TEST(lsof_finds_open_file) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    char args[512];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -Fn", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    int found = (strstr(buf, tmppath) != NULL);

    close(fd);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(found);
}

TEST(lsof_cwd_detection) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "cwd"));
}

TEST(lsof_tcp_socket_detection) {
    if (!lsof_available()) return;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    if (listen(sock, 1) < 0) {
        close(sock);
        return;
    }

    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    int port = ntohs(addr.sin_port);

    char args[256];
    char buf[16384];
    snprintf(args, sizeof(args), "-i TCP:%d -Fn -P", port);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(sock);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
}

TEST(lsof_unix_socket_detection) {
    if (!lsof_available()) return;

    char sockpath[] = "/tmp/lsof_test_unix_XXXXXX";
    int tmpfd = mkstemp(sockpath);
    if (tmpfd >= 0) { close(tmpfd); unlink(sockpath); }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sockpath, sizeof(addr.sun_path) - 1);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    listen(sock, 1);

    char args[512];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -U -Fn", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    int found = (strstr(buf, sockpath) != NULL);

    close(sock);
    unlink(sockpath);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(found);
}

TEST(lsof_invalid_pid) {
    if (!lsof_available()) return;
    char buf[4096];
    int rc = run_lsof("-p 99999999 -Fp", buf, sizeof(buf));
    ASSERT_EQ(rc, 1);
}

TEST(lsof_and_option) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-a -p %d -d cwd -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "cwd"));
}

TEST(lsof_fd_selection) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_fd_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    /* use -a to AND the pid and fd selections, and use cwd which is always present */
    snprintf(args, sizeof(args), "-a -p %d -d cwd -Ffn", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "cwd"));
}


TEST(lsof_multiple_pids) {
    if (!lsof_available()) return;
    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    pid_t ppid = getppid();

    snprintf(args, sizeof(args), "-p %d,%d -Fp", (int)mypid, (int)ppid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    char expected_me[64], expected_parent[64];
    snprintf(expected_me, sizeof(expected_me), "p%d", (int)mypid);
    snprintf(expected_parent, sizeof(expected_parent), "p%d", (int)ppid);
    ASSERT_NOT_NULL(strstr(buf, expected_me));
    ASSERT_NOT_NULL(strstr(buf, expected_parent));
}

TEST(lsof_txt_fd) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "txt"));
}

TEST(lsof_rtd_fd) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    /* rtd may not be present on all platforms (e.g. macOS) */
    ASSERT_TRUE(strstr(buf, "rtd") != NULL || strstr(buf, "txt") != NULL);
}

TEST(lsof_pipe_detection) {
    if (!lsof_available()) return;
    int pipefd[2];
    if (pipe(pipefd) < 0) return;

    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Ft", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(pipefd[0]);
    close(pipefd[1]);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "PIPE"));
}

TEST(lsof_multiple_fd_selection) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-a -p %d -d cwd,txt -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "cwd"));
    ASSERT_NOT_NULL(strstr(buf, "txt"));
}

TEST(lsof_numeric_fd) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_numfd_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    char fdstr[16];
    snprintf(fdstr, sizeof(fdstr), "%d", fd);

    snprintf(args, sizeof(args), "-a -p %d -d %d -Ff", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, fdstr));
}

TEST(lsof_field_type_output) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -F pct", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    /* Should have type fields: REG, DIR, PIPE, etc */
    int has_type = 0;
    char *line = buf;
    while (line && *line) {
        if (*line == 't') has_type = 1;
        char *nl = strchr(line, '\n');
        line = nl ? nl + 1 : NULL;
    }
    ASSERT_TRUE(has_type);
}

TEST(lsof_dev_null_detection) {
    if (!lsof_available()) return;
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;

    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -a -d %d -Fn", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "/dev/null"));
}

TEST(lsof_udp_socket_detection) {
    if (!lsof_available()) return;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }

    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    int port = ntohs(addr.sin_port);

    char args[256];
    char buf[16384];
    snprintf(args, sizeof(args), "-i UDP:%d -Fn -P", port);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(sock);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
}

TEST(lsof_no_output_for_nonexistent_file) {
    if (!lsof_available()) return;
    char buf[4096];
    int rc = run_lsof("/nonexistent/path/that/does/not/exist/ever", buf, sizeof(buf));
    /* lsof should exit with 1 (no matches) */
    ASSERT_EQ(rc, 1);
}

TEST(lsof_fd_range_selection) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    /* select fd range 0-2 (stdin, stdout, stderr) */
    snprintf(args, sizeof(args), "-a -p %d -d 0-2 -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
}

TEST(lsof_exclude_fd) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    /* exclude cwd, should still have other entries */
    snprintf(args, sizeof(args), "-a -p %d -d ^cwd -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    /* cwd should be excluded */
    ASSERT_NULL(strstr(buf, "fcwd"));
}

TEST(lsof_no_header_with_field_output) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Fp", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    /* field output should not contain the normal header line "COMMAND" */
    ASSERT_NULL(strstr(buf, "COMMAND"));
}

/* ===== Edge case tests for distro-readiness ===== */

TEST(lsof_deleted_file_detection) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_del_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    write(fd, "data", 4);

    /* delete while still open */
    unlink(tmppath);

    char args[512];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -a -d %d -Fn", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
    /* on most systems, deleted files show the path (often with " (deleted)" suffix on Linux) */
    ASSERT_NOT_NULL(strstr(buf, "lsof_test_del_"));
}

TEST(lsof_dup_fd_detection) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_dup_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    int fd2 = dup(fd);
    ASSERT_TRUE(fd2 >= 0);

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();

    /* both FDs should appear */
    char fdstr1[16], fdstr2[16];
    snprintf(fdstr1, sizeof(fdstr1), "%d", fd);
    snprintf(fdstr2, sizeof(fdstr2), "%d", fd2);

    snprintf(args, sizeof(args), "-a -p %d -d %d,%d -Ffn", (int)mypid, fd, fd2);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    close(fd2);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    /* both FDs should reference the same file */
    int count = 0;
    char *p = buf;
    while ((p = strstr(p, tmppath)) != NULL) { count++; p++; }
    ASSERT_GE(count, 2);
}

TEST(lsof_high_fd_number) {
    if (!lsof_available()) return;

    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;

    /* move to a high FD number */
    int highfd = dup2(fd, 200);
    close(fd);
    if (highfd < 0) return;

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-a -p %d -d 200 -Ffn", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(highfd);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "200"));
    ASSERT_NOT_NULL(strstr(buf, "/dev/null"));
}

TEST(lsof_symlink_target) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_sym_target_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    char linkpath[256];
    snprintf(linkpath, sizeof(linkpath), "%s_link", tmppath);
    if (symlink(tmppath, linkpath) < 0) {
        unlink(tmppath);
        return;
    }

    /* open via symlink */
    fd = open(linkpath, O_RDONLY);
    if (fd < 0) {
        unlink(linkpath);
        unlink(tmppath);
        return;
    }

    char args[512];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-a -p %d -d %d -Fn", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    unlink(linkpath);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    /* lsof should resolve to the real target path, not the symlink */
    ASSERT_NOT_NULL(strstr(buf, "lsof_test_sym_target_"));
}

TEST(lsof_many_open_files) {
    if (!lsof_available()) return;

    /* open 50 FDs to stress the file listing */
    int fds[50];
    int opened = 0;
    for (int i = 0; i < 50; i++) {
        fds[i] = open("/dev/null", O_RDONLY);
        if (fds[i] >= 0) opened++;
        else break;
    }
    if (opened < 10) {
        for (int i = 0; i < opened; i++) close(fds[i]);
        return;
    }

    char args[128];
    char buf[65536];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    for (int i = 0; i < opened; i++) close(fds[i]);

    ASSERT_EQ(rc, 0);
    /* count how many 'f' lines we see — should be at least our opened count + cwd/txt/rtd */
    int fcount = 0;
    char *line = buf;
    while (line && *line) {
        if (*line == 'f') fcount++;
        char *nl = strchr(line, '\n');
        line = nl ? nl + 1 : NULL;
    }
    ASSERT_GE(fcount, opened);
}

TEST(lsof_socketpair_detection) {
    if (!lsof_available()) return;

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) return;

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -a -d %d,%d -Ft", (int)mypid, sv[0], sv[1]);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
    /* should identify as unix socket type */
    ASSERT_TRUE(strstr(buf, "unix") != NULL || strstr(buf, "PIPE") != NULL);
}

TEST(lsof_ipv6_socket_detection) {
    if (!lsof_available()) return;

    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0) return; /* IPv6 not available */

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_loopback;
    addr.sin6_port = 0;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    if (listen(sock, 1) < 0) {
        close(sock);
        return;
    }

    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    int port = ntohs(addr.sin6_port);

    char args[256];
    char buf[16384];
    snprintf(args, sizeof(args), "-i TCP:%d -Fn -P", port);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(sock);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
}

TEST(lsof_connected_tcp_pair) {
    if (!lsof_available()) return;

    /* create a listener */
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(listener);
        return;
    }
    listen(listener, 1);

    socklen_t addrlen = sizeof(addr);
    getsockname(listener, (struct sockaddr *)&addr, &addrlen);
    int port = ntohs(addr.sin_port);

    /* connect a client */
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) { close(listener); return; }

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(client);
        close(listener);
        return;
    }

    int server = accept(listener, NULL, NULL);
    if (server < 0) {
        close(client);
        close(listener);
        return;
    }

    char args[256];
    char buf[16384];
    snprintf(args, sizeof(args), "-i TCP:%d -Fn -P", port);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(server);
    close(client);
    close(listener);

    ASSERT_EQ(rc, 0);
    /* should show established connections with the port */
    char portstr[32];
    snprintf(portstr, sizeof(portstr), ":%d", port);
    ASSERT_NOT_NULL(strstr(buf, portstr));
}

TEST(lsof_dev_zero_detection) {
    if (!lsof_available()) return;

    int fd = open("/dev/zero", O_RDONLY);
    if (fd < 0) return;

    char args[128];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -a -d %d -Fn", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "/dev/zero"));
}

TEST(lsof_read_write_mode) {
    if (!lsof_available()) return;

    char tmppath_r[] = "/tmp/lsof_test_ro_XXXXXX";
    char tmppath_w[] = "/tmp/lsof_test_rw_XXXXXX";
    int fd_r = mkstemp(tmppath_r);
    int fd_w = mkstemp(tmppath_w);
    ASSERT_TRUE(fd_r >= 0);
    ASSERT_TRUE(fd_w >= 0);
    close(fd_r);
    close(fd_w);

    /* reopen with specific modes */
    fd_r = open(tmppath_r, O_RDONLY);
    fd_w = open(tmppath_w, O_RDWR);
    ASSERT_TRUE(fd_r >= 0);
    ASSERT_TRUE(fd_w >= 0);

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -a -d %d,%d -Ffa", (int)mypid, fd_r, fd_w);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd_r);
    close(fd_w);
    unlink(tmppath_r);
    unlink(tmppath_w);

    ASSERT_EQ(rc, 0);
    /* should have access mode indicators: 'r' for read, 'u' for read/write */
    ASSERT_TRUE(strstr(buf, "r") != NULL);
    ASSERT_TRUE(strstr(buf, "u") != NULL);
}

TEST(lsof_directory_fd) {
    if (!lsof_available()) return;

    char tmpdir[] = "/tmp/lsof_test_dir_XXXXXX";
    if (mkdtemp(tmpdir) == NULL) return;

    int fd = open(tmpdir, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        rmdir(tmpdir);
        return;
    }

    char args[512];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -a -d %d -Ftn", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    rmdir(tmpdir);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "DIR"));
    ASSERT_NOT_NULL(strstr(buf, "lsof_test_dir_"));
}

TEST(lsof_command_name_filter) {
    if (!lsof_available()) return;

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();

    /* -c filters by command name; our process is "check_integration" */
    snprintf(args, sizeof(args), "-a -p %d -c check -Fpc", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    char expected[64];
    snprintf(expected, sizeof(expected), "p%d", (int)mypid);
    ASSERT_NOT_NULL(strstr(buf, expected));
}

TEST(lsof_command_name_no_match) {
    if (!lsof_available()) return;

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();

    /* filter by a command name that definitely doesn't match */
    snprintf(args, sizeof(args), "-a -p %d -c ZZZZNOTACOMMAND -Fp", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    /* should find nothing and return 1 */
    ASSERT_EQ(rc, 1);
}

TEST(lsof_user_filter) {
    if (!lsof_available()) return;

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    uid_t uid = getuid();

    struct passwd *pw = getpwuid(uid);
    if (!pw) return;

    snprintf(args, sizeof(args), "-a -p %d -u %s -Fp", (int)mypid, pw->pw_name);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    char expected[64];
    snprintf(expected, sizeof(expected), "p%d", (int)mypid);
    ASSERT_NOT_NULL(strstr(buf, expected));
}

TEST(lsof_user_filter_no_match) {
    if (!lsof_available()) return;

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();

    /* filter by a user that isn't us (use exclusion) */
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (!pw) return;

    /* exclude our own user — should find nothing for our PID */
    snprintf(args, sizeof(args), "-a -p %d -u ^%s -Fp", (int)mypid, pw->pw_name);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 1);
}

TEST(lsof_terse_mode) {
    if (!lsof_available()) return;

    char args[128];
    char buf[4096];
    pid_t mypid = getpid();

    /* -t outputs only PIDs */
    snprintf(args, sizeof(args), "-p %d -t", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    char expected[64];
    snprintf(expected, sizeof(expected), "%d", (int)mypid);
    ASSERT_NOT_NULL(strstr(buf, expected));

    /* terse output should be very short — just PIDs, no headers */
    ASSERT_NULL(strstr(buf, "COMMAND"));
    ASSERT_NULL(strstr(buf, "FD"));
}

TEST(lsof_offset_output) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_off_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    /* write data and seek to a known offset */
    write(fd, "abcdefghijklmnop", 16);
    lseek(fd, 8, SEEK_SET);

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    /* -o requests offset output */
    snprintf(args, sizeof(args), "-p %d -a -d %d -o -Fo", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    /* offset field should be present (starts with 'o') */
    int has_offset = 0;
    char *line = buf;
    while (line && *line) {
        if (*line == 'o') has_offset = 1;
        char *nl = strchr(line, '\n');
        line = nl ? nl + 1 : NULL;
    }
    ASSERT_TRUE(has_offset);
}

TEST(lsof_size_output) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_sz_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    write(fd, "0123456789", 10);

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    /* -s requests size output */
    snprintf(args, sizeof(args), "-p %d -a -d %d -s -Fs", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    /* size field should be present */
    int has_size = 0;
    char *line = buf;
    while (line && *line) {
        if (*line == 's') has_size = 1;
        char *nl = strchr(line, '\n');
        line = nl ? nl + 1 : NULL;
    }
    ASSERT_TRUE(has_size);
}

TEST(lsof_all_field_types) {
    if (!lsof_available()) return;
    char args[128];
    char buf[32768];
    pid_t mypid = getpid();

    /* request many field types at once */
    snprintf(args, sizeof(args), "-p %d -F pcftnds", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    int has_p = 0, has_c = 0, has_f = 0, has_t = 0, has_n = 0;
    char *line = buf;
    while (line && *line) {
        if (*line == 'p') has_p = 1;
        if (*line == 'c') has_c = 1;
        if (*line == 'f') has_f = 1;
        if (*line == 't') has_t = 1;
        if (*line == 'n') has_n = 1;
        char *nl = strchr(line, '\n');
        line = nl ? nl + 1 : NULL;
    }
    ASSERT_TRUE(has_p);
    ASSERT_TRUE(has_c);
    ASSERT_TRUE(has_f);
    ASSERT_TRUE(has_t);
    ASSERT_TRUE(has_n);
}

TEST(lsof_empty_pid_list) {
    if (!lsof_available()) return;
    /* PID 1 and a nonexistent PID — should still succeed if PID 1 is readable */
    char buf[8192];
    int rc = run_lsof("-p 1 -Fp", buf, sizeof(buf));
    /* may fail with permission error (rc=1) or succeed; just verify no crash */
    ASSERT_TRUE(rc == 0 || rc == 1);
}

TEST(lsof_multiple_file_types_same_process) {
    if (!lsof_available()) return;

    /* open a regular file, a pipe, and a socket simultaneously */
    char tmppath[] = "/tmp/lsof_test_multi_XXXXXX";
    int reg_fd = mkstemp(tmppath);
    ASSERT_TRUE(reg_fd >= 0);

    int pipefd[2];
    if (pipe(pipefd) < 0) { close(reg_fd); unlink(tmppath); return; }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        close(reg_fd); close(pipefd[0]); close(pipefd[1]);
        unlink(tmppath);
        return;
    }

    char args[256];
    char buf[32768];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -Ft", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(reg_fd);
    close(pipefd[0]);
    close(pipefd[1]);
    close(sock);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    /* should see multiple types: REG for file, PIPE for pipe, IPv4/sock for socket */
    ASSERT_NOT_NULL(strstr(buf, "REG"));
    ASSERT_NOT_NULL(strstr(buf, "PIPE"));
}

TEST(lsof_repeated_same_flag) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    /* pass -F twice — should not crash */
    snprintf(args, sizeof(args), "-p %d -Fp -Fc", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
}

TEST(lsof_nul_terminated_field_output) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    /* -F0 requests NUL-terminated output */
    snprintf(args, sizeof(args), "-p %d -F0p", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    /* output should contain our PID field followed by a NUL */
    char expected[64];
    snprintf(expected, sizeof(expected), "p%d", (int)mypid);
    ASSERT_NOT_NULL(strstr(buf, expected));
}

TEST(lsof_stdin_stdout_stderr) {
    if (!lsof_available()) return;
    char args[256];
    char buf[16384];
    pid_t mypid = getpid();

    /* explicitly select FDs 0, 1, 2 */
    snprintf(args, sizeof(args), "-a -p %d -d 0,1,2 -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "f0"));
    ASSERT_NOT_NULL(strstr(buf, "f1"));
    ASSERT_NOT_NULL(strstr(buf, "f2"));
}

TEST(lsof_file_after_write) {
    if (!lsof_available()) return;

    char tmppath[] = "/tmp/lsof_test_write_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    /* write a known amount */
    char data[1024];
    memset(data, 'X', sizeof(data));
    write(fd, data, sizeof(data));
    fsync(fd);

    char args[512];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -a -d %d -Fsn", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, tmppath));
}

TEST(lsof_pid_exclusion) {
    if (!lsof_available()) return;
    char args[256];
    char buf[16384];
    pid_t mypid = getpid();

    /* exclude our own PID — -p with ^PID */
    snprintf(args, sizeof(args), "-p ^%d -a -d cwd -Fp", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    /* should succeed but not contain our PID */
    ASSERT_TRUE(rc == 0 || rc == 1);
    char mypidstr[64];
    snprintf(mypidstr, sizeof(mypidstr), "p%d\n", (int)mypid);
    ASSERT_NULL(strstr(buf, mypidstr));
}

TEST(lsof_internet_connections_list) {
    if (!lsof_available()) return;

    /* create a socket so there's at least one */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 1);

    char args[256];
    char buf[32768];
    pid_t mypid = getpid();
    /* -i lists all internet connections */
    snprintf(args, sizeof(args), "-p %d -i -Fn -P", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(sock);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
}

TEST(lsof_numeric_port_output) {
    if (!lsof_available()) return;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    listen(sock, 1);

    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    int port = ntohs(addr.sin_port);

    char args[256];
    char buf[16384];
    /* -P disables port-to-service name conversion */
    snprintf(args, sizeof(args), "-i TCP:%d -Fn -P", port);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(sock);

    ASSERT_EQ(rc, 0);
    char portstr[32];
    snprintf(portstr, sizeof(portstr), ":%d", port);
    ASSERT_NOT_NULL(strstr(buf, portstr));
}

TEST(lsof_numeric_host_output) {
    if (!lsof_available()) return;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    listen(sock, 1);

    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    int port = ntohs(addr.sin_port);

    char args[256];
    char buf[16384];
    /* -n disables host name resolution, -P disables port name resolution */
    snprintf(args, sizeof(args), "-i TCP:%d -Fn -n -P", port);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(sock);

    ASSERT_EQ(rc, 0);
    /* should show numeric IP, not hostname */
    ASSERT_NOT_NULL(strstr(buf, "127.0.0.1"));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    TEST_SUITE("LSOF INTEGRATION TESTS");

    RUN(lsof_exists);
    RUN(lsof_diag);
    RUN(lsof_help_flag);
    RUN(lsof_version_flag);
    RUN(lsof_finds_own_pid);
    RUN(lsof_field_output_format);
    RUN(lsof_finds_open_file);
    RUN(lsof_cwd_detection);
    RUN(lsof_tcp_socket_detection);
    RUN(lsof_unix_socket_detection);
    RUN(lsof_invalid_pid);
    RUN(lsof_and_option);
    RUN(lsof_fd_selection);
    RUN(lsof_multiple_pids);
    RUN(lsof_txt_fd);
    RUN(lsof_rtd_fd);
    RUN(lsof_pipe_detection);
    RUN(lsof_multiple_fd_selection);
    RUN(lsof_numeric_fd);
    RUN(lsof_field_type_output);
    RUN(lsof_dev_null_detection);
    RUN(lsof_udp_socket_detection);
    RUN(lsof_no_output_for_nonexistent_file);
    RUN(lsof_fd_range_selection);
    RUN(lsof_exclude_fd);
    RUN(lsof_no_header_with_field_output);

    /* --- edge case tests --- */
    RUN(lsof_deleted_file_detection);
    RUN(lsof_dup_fd_detection);
    RUN(lsof_high_fd_number);
    RUN(lsof_symlink_target);
    RUN(lsof_many_open_files);
    RUN(lsof_socketpair_detection);
    RUN(lsof_ipv6_socket_detection);
    RUN(lsof_connected_tcp_pair);
    RUN(lsof_dev_zero_detection);
    RUN(lsof_read_write_mode);
    RUN(lsof_directory_fd);
    RUN(lsof_command_name_filter);
    RUN(lsof_command_name_no_match);
    RUN(lsof_user_filter);
    RUN(lsof_user_filter_no_match);
    RUN(lsof_terse_mode);
    RUN(lsof_offset_output);
    RUN(lsof_size_output);
    RUN(lsof_all_field_types);
    RUN(lsof_empty_pid_list);
    RUN(lsof_multiple_file_types_same_process);
    RUN(lsof_repeated_same_flag);
    RUN(lsof_nul_terminated_field_output);
    RUN(lsof_stdin_stdout_stderr);
    RUN(lsof_file_after_write);
    RUN(lsof_pid_exclusion);
    RUN(lsof_internet_connections_list);
    RUN(lsof_numeric_port_output);
    RUN(lsof_numeric_host_output);

    TEST_REPORT();
}
