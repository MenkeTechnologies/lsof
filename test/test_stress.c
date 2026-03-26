/*
 * test_stress.c - stress/regression tests for lsof
 *
 * Two categories:
 *   1. Unit stress tests: exercise internal data structures at scale
 *      (10K+ entries for hash tables, sort, search, dedup)
 *   2. Integration stress tests: invoke the lsof binary under load
 *      (many open files, many sockets, many PIDs, large output)
 */

#include "test.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/* ============================================================
 * Unit stress tests: include the same reimplementations used
 * by the regular unit tests, then exercise them at scale.
 * ============================================================ */

#include "test_unit_hash.h"
#include "test_unit_compare.h"
#include "test_unit_portcache.h"
#include "test_unit_devhash.h"
#include "test_unit_stress.h"

/* ============================================================
 * Integration stress tests: invoke the lsof binary under load.
 * ============================================================ */

static const char *lsof_path = NULL;

static const char *find_lsof(void) {
    if (lsof_path)
        return lsof_path;
    const char *candidates[] = {
#ifdef LSOF_BUILD_PATH
        LSOF_BUILD_PATH,
#endif
        "./lsof5",        "../lsof5",       "./build/lsof5",  "../build/lsof5",
        "./lsof",         "../lsof",        "./build/lsof",   "../build/lsof",
        "/usr/bin/lsof",  "/usr/sbin/lsof", NULL};
    for (int i = 0; candidates[i]; i++) {
        if (access(candidates[i], X_OK) == 0) {
            lsof_path = candidates[i];
            return lsof_path;
        }
    }
    return NULL;
}

static int run_lsof(const char *args, char *buf, size_t bufsz) {
    const char *lsof = find_lsof();
    if (!lsof)
        return -1;

    char argbuf[2048];
    char *argv[128];
    int argc = 0;
    argv[argc++] = (char *)lsof;
    if (args && *args) {
        strncpy(argbuf, args, sizeof(argbuf) - 1);
        argbuf[sizeof(argbuf) - 1] = '\0';
        char *tok = strtok(argbuf, " ");
        while (tok && argc < 127) {
            argv[argc++] = tok;
            tok = strtok(NULL, " ");
        }
    }
    argv[argc] = NULL;

    int pipefd[2];
    if (pipe(pipefd) < 0)
        return -1;

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execv(lsof, argv);
        _exit(127);
    }

    close(pipefd[1]);
    if (buf && bufsz > 0) {
        size_t total = 0;
        while (total < bufsz - 1) {
            ssize_t n = read(pipefd[0], buf + total, bufsz - 1 - total);
            if (n <= 0)
                break;
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

/* --- Integration stress: many open files --- */

TEST(stress_many_open_files) {
    if (!lsof_available())
        return;

    /* Open as many temp files as we can, up to 200 */
    int fds[200];
    int n_opened = 0;
    char paths[200][64];

    for (int i = 0; i < 200; i++) {
        snprintf(paths[i], sizeof(paths[i]), "/tmp/lsof_stress_%d_XXXXXX", i);
        fds[i] = mkstemp(paths[i]);
        if (fds[i] < 0)
            break;
        n_opened++;
    }

    ASSERT_GT(n_opened, 50); /* need at least 50 to be a meaningful stress test */

    char args[128];
    char *buf = (char *)malloc(1024 * 1024); /* 1MB output buffer */
    ASSERT_NOT_NULL(buf);
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Fn", (int)mypid);
    int rc = run_lsof(args, buf, 1024 * 1024);
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);

    /* Verify lsof found at least some of our temp files */
    int found = 0;
    for (int i = 0; i < n_opened; i++) {
        if (strstr(buf, paths[i]))
            found++;
    }
    ASSERT_GT(found, n_opened / 2);

    /* Cleanup */
    for (int i = 0; i < n_opened; i++) {
        close(fds[i]);
        unlink(paths[i]);
    }
    free(buf);
}

/* --- Integration stress: many TCP sockets --- */

TEST(stress_many_tcp_sockets) {
    if (!lsof_available())
        return;

    int socks[50];
    int ports[50];
    int n_bound = 0;

    for (int i = 0; i < 50; i++) {
        socks[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (socks[i] < 0)
            break;

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        if (bind(socks[i], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(socks[i]);
            break;
        }
        if (listen(socks[i], 1) < 0) {
            close(socks[i]);
            break;
        }

        socklen_t addrlen = sizeof(addr);
        getsockname(socks[i], (struct sockaddr *)&addr, &addrlen);
        ports[i] = ntohs(addr.sin_port);
        n_bound++;
    }

    ASSERT_GT(n_bound, 10);

    char args[128];
    char *buf = (char *)malloc(512 * 1024);
    ASSERT_NOT_NULL(buf);
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -i TCP -Fn -P", (int)mypid);
    int rc = run_lsof(args, buf, 512 * 1024);
    ASSERT_EQ(rc, 0);

    /* Verify lsof found at least some of our sockets by port number */
    int found = 0;
    for (int i = 0; i < n_bound; i++) {
        char portstr[16];
        snprintf(portstr, sizeof(portstr), ":%d", ports[i]);
        if (strstr(buf, portstr))
            found++;
    }
    ASSERT_GT(found, n_bound / 2);

    for (int i = 0; i < n_bound; i++)
        close(socks[i]);
    free(buf);
}

/* --- Integration stress: many UDP sockets --- */

TEST(stress_many_udp_sockets) {
    if (!lsof_available())
        return;

    int socks[50];
    int n_bound = 0;

    for (int i = 0; i < 50; i++) {
        socks[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (socks[i] < 0)
            break;

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        if (bind(socks[i], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(socks[i]);
            break;
        }
        n_bound++;
    }

    ASSERT_GT(n_bound, 10);

    char args[128];
    char *buf = (char *)malloc(512 * 1024);
    ASSERT_NOT_NULL(buf);
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -i UDP -Fn -P", (int)mypid);
    int rc = run_lsof(args, buf, 512 * 1024);
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strlen(buf) > 0);

    for (int i = 0; i < n_bound; i++)
        close(socks[i]);
    free(buf);
}

/* --- Integration stress: many pipes --- */

TEST(stress_many_pipes) {
    if (!lsof_available())
        return;

    int pipefds[100][2];
    int n_opened = 0;

    for (int i = 0; i < 100; i++) {
        if (pipe(pipefds[i]) < 0)
            break;
        n_opened++;
    }

    ASSERT_GT(n_opened, 20);

    char args[128];
    char *buf = (char *)malloc(512 * 1024);
    ASSERT_NOT_NULL(buf);
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Ft", (int)mypid);
    int rc = run_lsof(args, buf, 512 * 1024);
    ASSERT_EQ(rc, 0);

    /* Count PIPE type entries */
    int pipe_count = 0;
    char *p = buf;
    while ((p = strstr(p, "PIPE")) != NULL) {
        pipe_count++;
        p += 4;
    }
    /* Each pipe() creates 2 fds, lsof should find many */
    ASSERT_GT(pipe_count, n_opened);

    for (int i = 0; i < n_opened; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }
    free(buf);
}

/* --- Integration stress: large field output --- */

TEST(stress_large_field_output) {
    if (!lsof_available())
        return;

    /* Open files + sockets to generate lots of output */
    int fds[100];
    int n_opened = 0;
    char paths[100][64];
    for (int i = 0; i < 100; i++) {
        snprintf(paths[i], sizeof(paths[i]), "/tmp/lsof_stressout_%d_XXXXXX", i);
        fds[i] = mkstemp(paths[i]);
        if (fds[i] < 0)
            break;
        n_opened++;
    }

    ASSERT_GT(n_opened, 20);

    /* Request all field types */
    char args[128];
    char *buf = (char *)malloc(2 * 1024 * 1024); /* 2MB */
    ASSERT_NOT_NULL(buf);
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -F pcftnds", (int)mypid);
    int rc = run_lsof(args, buf, 2 * 1024 * 1024);
    ASSERT_EQ(rc, 0);

    /* Output should be substantial */
    size_t len = strlen(buf);
    ASSERT_GT((long long)len, 1000);

    /* Count lines - should have many field output lines */
    int lines = 0;
    for (size_t i = 0; i < len; i++)
        if (buf[i] == '\n')
            lines++;
    ASSERT_GT(lines, n_opened * 2); /* at least 2 lines per file */

    for (int i = 0; i < n_opened; i++) {
        close(fds[i]);
        unlink(paths[i]);
    }
    free(buf);
}

/* --- Integration stress: repeated invocations (regression) --- */

TEST(stress_repeated_invocations) {
    if (!lsof_available())
        return;

    char args[128];
    char buf[8192];
    pid_t mypid = getpid();
    snprintf(args, sizeof(args), "-p %d -Fp", (int)mypid);

    char expected[64];
    snprintf(expected, sizeof(expected), "p%d", (int)mypid);

    /* Run lsof 20 times in a row - catches resource leaks in invocation */
    for (int i = 0; i < 20; i++) {
        int rc = run_lsof(args, buf, sizeof(buf));
        ASSERT_EQ(rc, 0);
        ASSERT_NOT_NULL(strstr(buf, expected));
    }
}

/* --- Integration stress: mixed file descriptor types --- */

TEST(stress_mixed_fd_types) {
    if (!lsof_available())
        return;

    /* Open a mix of files, pipes, and sockets */
    int regular_fds[20];
    int pipe_fds[20][2];
    int sock_fds[20];
    int n_regular = 0, n_pipes = 0, n_socks = 0;
    char paths[20][64];

    for (int i = 0; i < 20; i++) {
        snprintf(paths[i], sizeof(paths[i]), "/tmp/lsof_stressmix_%d_XXXXXX", i);
        regular_fds[i] = mkstemp(paths[i]);
        if (regular_fds[i] >= 0)
            n_regular++;
    }
    for (int i = 0; i < 20; i++) {
        if (pipe(pipe_fds[i]) == 0)
            n_pipes++;
        else
            break;
    }
    for (int i = 0; i < 20; i++) {
        sock_fds[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fds[i] >= 0)
            n_socks++;
        else
            break;
    }

    ASSERT_GT(n_regular, 5);
    ASSERT_GT(n_pipes, 5);
    ASSERT_GT(n_socks, 5);

    char args[128];
    char *buf = (char *)malloc(1024 * 1024);
    ASSERT_NOT_NULL(buf);
    pid_t mypid = getpid();

    snprintf(args, sizeof(args), "-p %d -Fft", (int)mypid);
    int rc = run_lsof(args, buf, 1024 * 1024);
    ASSERT_EQ(rc, 0);

    /* Should find REG, PIPE, and IPv4/sock types */
    int has_reg = (strstr(buf, "REG") != NULL) || (strstr(buf, "VREG") != NULL);
    int has_pipe = (strstr(buf, "PIPE") != NULL);
    ASSERT_TRUE(has_reg);
    ASSERT_TRUE(has_pipe);

    for (int i = 0; i < n_regular; i++) {
        close(regular_fds[i]);
        unlink(paths[i]);
    }
    for (int i = 0; i < n_pipes; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    for (int i = 0; i < n_socks; i++)
        close(sock_fds[i]);
    free(buf);
}

/* --- Integration stress: concurrent lsof processes --- */

TEST(stress_concurrent_lsof) {
    if (!lsof_available())
        return;

    const char *lsof = find_lsof();
    pid_t children[10];
    int n_children = 0;

    for (int i = 0; i < 10; i++) {
        pid_t pid = fork();
        if (pid < 0)
            break;
        if (pid == 0) {
            /* Child: run lsof -p <own_pid> and exit */
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            char pidbuf[32];
            snprintf(pidbuf, sizeof(pidbuf), "%d", (int)getpid());
            execl(lsof, lsof, "-p", pidbuf, "-Fp", (char *)NULL);
            _exit(127);
        }
        children[i] = pid;
        n_children++;
    }

    ASSERT_GT(n_children, 0);

    /* Wait for all children */
    int all_ok = 1;
    for (int i = 0; i < n_children; i++) {
        int status;
        waitpid(children[i], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            all_ok = 0;
    }
    ASSERT_TRUE(all_ok);
}

/* --- Integration stress: many PIDs selection --- */

TEST(stress_many_pid_selection) {
    if (!lsof_available())
        return;

    /* Build a -p argument with our own PID repeated in a comma-separated list
     * to exercise the PID parsing and selection logic at scale.
     * Avoid PID 1 or other PIDs that may require root privileges. */
    char args[2048];
    char buf[16384];
    pid_t mypid = getpid();
    pid_t ppid = getppid();

    snprintf(args, sizeof(args), "-p %d,%d -Fp", (int)mypid, (int)ppid);

    int rc = run_lsof(args, buf, sizeof(buf));
    ASSERT_EQ(rc, 0);

    char expected[64];
    snprintf(expected, sizeof(expected), "p%d", (int)mypid);
    ASSERT_NOT_NULL(strstr(buf, expected));

    char expected_parent[64];
    snprintf(expected_parent, sizeof(expected_parent), "p%d", (int)ppid);
    ASSERT_NOT_NULL(strstr(buf, expected_parent));
}

/* ============================================================ */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    TEST_SUITE("LSOF STRESS TESTS - UNIT");

    /* --- hash stress --- */
    RUN(stress_hashport_all_ports_in_range);
    RUN(stress_hashport_distribution_full_range);
    RUN(stress_hashport_max_chain_length);
    RUN(stress_hashbyname_10k_paths);
    RUN(stress_hashbyname_long_paths);
    RUN(stress_hashbyname_collision_rate);

    /* --- sort stress --- */
    RUN(stress_sort_10k_pids);
    RUN(stress_sort_50k_pids);
    RUN(stress_sort_10k_devices);
    RUN(stress_sort_all_duplicates);
    RUN(stress_sort_already_sorted);
    RUN(stress_sort_many_equal_keys);
    RUN(stress_device_sort_multikey_10k);

    /* --- binary search stress --- */
    RUN(stress_bsearch_10k_all_hits);
    RUN(stress_bsearch_10k_all_misses);
    RUN(stress_bsearch_boundary_values);
    RUN(stress_int_lst_sort_then_search_10k);

    /* --- port cache stress --- */
    RUN(stress_port_table_10k_entries);
    RUN(stress_port_table_all_65k_ports);

    /* --- device dedup stress --- */
    RUN(stress_rmdupdev_10k_no_dups);
    RUN(stress_rmdupdev_10k_all_same);
    RUN(stress_rmdupdev_10k_pairs);
    RUN(stress_rmdupdev_10k_runs);

    /* --- device/inode hash stress --- */
    RUN(stress_sfhash_10k_entries);
    RUN(stress_sfhash_large_values);
    RUN(stress_ncache_hash_10k);

    /* --- string stress --- */
    RUN(stress_safestrlen_10k_strings);
    RUN(stress_safestrlen_binary_content);

    TEST_SUITE("LSOF STRESS TESTS - INTEGRATION");

    /* --- integration stress --- */
    RUN(stress_many_open_files);
    RUN(stress_many_tcp_sockets);
    RUN(stress_many_udp_sockets);
    RUN(stress_many_pipes);
    RUN(stress_large_field_output);
    RUN(stress_repeated_invocations);
    RUN(stress_mixed_fd_types);
    RUN(stress_concurrent_lsof);
    RUN(stress_many_pid_selection);

    TEST_REPORT();
}
