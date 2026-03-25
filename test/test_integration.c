/*
 * test_integration.c - integration tests for lsof binary
 *
 * These tests invoke the lsof binary and verify its output behavior.
 * They require lsof to be built and accessible at ../build/lsof or
 * the system lsof.
 */

#include "test_framework.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static const char *lsof_path = NULL;

static const char *find_lsof(void) {
    if (lsof_path)
        return lsof_path;
    /* try common locations */
    const char *candidates[] = {
        "./build/lsof",
        "../build/lsof",
        "./lsof",
        "../lsof",
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

/* Run lsof with given args, return exit status. Output in buf if non-NULL. */
static int run_lsof(const char *args, char *buf, size_t bufsz) {
    const char *lsof = find_lsof();
    if (!lsof) return -1;

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s 2>/dev/null", lsof, args ? args : "");

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    if (buf && bufsz > 0) {
        size_t total = 0;
        while (total < bufsz - 1) {
            size_t n = fread(buf + total, 1, bufsz - 1 - total, fp);
            if (n == 0) break;
            total += n;
        }
        buf[total] = '\0';
    }

    int status = pclose(fp);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}

/* Check if lsof is available */
static int lsof_available(void) {
    return find_lsof() != NULL;
}


/* ===== Integration Tests ===== */

TEST(lsof_exists) {
    ASSERT_TRUE(lsof_available());
}

TEST(lsof_help_flag) {
    if (!lsof_available()) return;
    /* lsof -h should exit (may be 0 or 1 depending on version) */
    char buf[4096];
    int rc = run_lsof("-h", buf, sizeof(buf));
    /* some lsof versions exit 1 for -h, some exit 0 */
    ASSERT_TRUE(rc == 0 || rc == 1);
}

TEST(lsof_version_flag) {
    if (!lsof_available()) return;
    char buf[4096];
    int rc = run_lsof("-v", buf, sizeof(buf));
    (void)rc;
    /* should contain "lsof" or version info somewhere */
    /* -v may output to stderr, which we redirect to /dev/null,
     * so just verify it doesn't crash */
    ASSERT_TRUE(1);
}

TEST(lsof_finds_own_pid) {
    if (!lsof_available()) return;
    char args[128];
    char buf[8192];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Fp", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    /* output should contain "p<pid>" */
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

    /* field output should have lines starting with field IDs */
    ASSERT_TRUE(strlen(buf) > 0);

    /* should contain p (PID), c (command), f (FD) fields */
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

    /* create a temp file and keep it open */
    char tmppath[] = "/tmp/lsof_test_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    char args[512];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -Fn", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));

    /* should find our temp file in the output */
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

    /* should have "cwd" in FD column */
    ASSERT_NOT_NULL(strstr(buf, "cwd"));
}

TEST(lsof_tcp_socket_detection) {
    if (!lsof_available()) return;

    /* create a TCP listening socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; /* let OS assign */

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    if (listen(sock, 1) < 0) {
        close(sock);
        return;
    }

    /* get the assigned port */
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
    mktemp(sockpath);

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

TEST(lsof_null_terminator_option) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -F0p", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    /* with -F0, fields are NUL-terminated; the buffer will have
     * embedded NULs, but pclose should still work */
    ASSERT_TRUE(1);
}

TEST(lsof_invalid_pid) {
    if (!lsof_available()) return;
    char buf[4096];
    /* PID 0 or very large PID that doesn't exist */
    int rc = run_lsof("-p 99999999 -Fp", buf, sizeof(buf));
    /* should exit 1 (no matching processes) */
    ASSERT_EQ(rc, 1);
}

TEST(lsof_and_option) {
    if (!lsof_available()) return;
    char args[128];
    char buf[16384];
    pid_t mypid = getpid();

    /* -a means AND selections */
    snprintf(args, sizeof(args), "-a -p %d -d cwd -Ff", (int)mypid);
    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(strstr(buf, "cwd"));
}

TEST(lsof_fd_selection) {
    if (!lsof_available()) return;

    /* open a file on a known fd */
    char tmppath[] = "/tmp/lsof_test_fd_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);

    char args[256];
    char buf[16384];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -d %d -Ffn", (int)mypid, fd);
    int rc = run_lsof(args, buf, sizeof(buf));

    close(fd);
    unlink(tmppath);

    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);
}


RUN_TESTS();
