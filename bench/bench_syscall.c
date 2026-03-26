#include "bench.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>

#undef BF_SECTION
#define BF_SECTION "SYSCALL OVERHEAD"

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

/* ===== Socket creation benchmark (relevant to lsof socket detection) ===== */
BENCH(socket_create_close, 50000) {
    for (int i = 0; i < bf_iters; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd >= 0) close(fd);
        BENCH_SINK_INT(fd);
    }
}

/* ===== readdir benchmark (relevant to device/directory scanning) ===== */
BENCH(readdir_dev, 1000) {
    for (int i = 0; i < bf_iters; i++) {
        int count = 0;
        DIR *dir_handle = opendir("/dev");
        if (dir_handle) {
            struct dirent *dir_entry;
            while ((dir_entry = readdir(dir_handle)) != NULL) count++;
            closedir(dir_handle);
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

/* ===== readlink benchmark (symlink resolution used in Readlink) ===== */
BENCH(readlink_dev_fd, 100000) {
    char buf[1024];
    char path[64];
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;
    snprintf(path, sizeof(path), "/dev/fd/%d", fd);
    for (int i = 0; i < bf_iters; i++) {
        ssize_t r = readlink(path, buf, sizeof(buf) - 1);
        BENCH_SINK_INT((int)r);
    }
    close(fd);
}


/* ===== access / is_readable benchmark ===== */
BENCH(access_readable, 500000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(access("/dev/null", R_OK));
    }
}

BENCH(access_nonexistent, 500000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(access("/nonexistent/path/file", R_OK));
    }
}


/* ===== dup/dup2 benchmark (file descriptor operations) ===== */
BENCH(dup_close, 100000) {
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;
    for (int i = 0; i < bf_iters; i++) {
        int fd2 = dup(fd);
        if (fd2 >= 0) close(fd2);
        BENCH_SINK_INT(fd2);
    }
    close(fd);
}


/* ===== fopen/fclose vs open/close overhead ===== */
BENCH(fopen_fclose, 100000) {
    for (int i = 0; i < bf_iters; i++) {
        FILE *f = fopen("/dev/null", "r");
        if (f) fclose(f);
        BENCH_SINK_PTR(f);
    }
}


/* ===== write syscall overhead ===== */
BENCH(write_devnull, 500000) {
    int fd = open("/dev/null", O_WRONLY);
    if (fd < 0) return;
    char buf[] = "p1234\nc/usr/bin/lsof\n";
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT((int)write(fd, buf, sizeof(buf) - 1));
    }
    close(fd);
}

/* ===== fcntl getfl benchmark (file descriptor flag inspection) ===== */
BENCH(fcntl_getfl, 500000) {
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(fcntl(fd, F_GETFL));
    }
    close(fd);
}


/* ===== socketpair benchmark (UNIX domain socket creation) ===== */
BENCH(socketpair_create_close, 50000) {
    for (int i = 0; i < bf_iters; i++) {
        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
            close(fds[0]);
            close(fds[1]);
        }
        BENCH_SINK_INT(fds[0]);
    }
}


/* ===== process self inspection (/proc or equivalent) ===== */
BENCH(read_proc_self, 50000) {
    for (int i = 0; i < bf_iters; i++) {
        char buf[4096];
        int fd = open("/dev/null", O_RDONLY);
        if (fd >= 0) {
            ssize_t n = read(fd, buf, sizeof(buf));
            close(fd);
            BENCH_SINK_INT((int)n);
        }
    }
}


BF_SECTIONS("SYSCALL OVERHEAD")

RUN_BENCHMARKS()
