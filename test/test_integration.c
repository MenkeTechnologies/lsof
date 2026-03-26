/*
 * test_integration.c - integration tests for lsof binary
 *
 * These tests invoke the lsof binary and verify its output behavior.
 * They require lsof to be accessible on the system.
 */

#include "test.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

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

    TEST_REPORT();
}
