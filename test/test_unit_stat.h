/*
 * test_unit_stat.h - stat/lstat wrapper and file type detection tests
 *
 * Tests for safe stat wrappers, file type macros, and permission handling
 */

#ifndef TEST_UNIT_STAT_H
#define TEST_UNIT_STAT_H

#include <sys/stat.h>
#include <fcntl.h>

/* ===== statsafely() reimplementation ===== */
static int test_stat_safely(const char *path, struct stat *buf) {
    if (!path || !buf)
        return -1;
    return stat(path, buf);
}

static int test_lstat_safely(const char *path, struct stat *buf) {
    if (!path || !buf)
        return -1;
    return lstat(path, buf);
}

/* ===== File type classification ===== */
static const char *test_file_type_name(mode_t mode) {
    if (S_ISREG(mode))
        return "REG";
    if (S_ISDIR(mode))
        return "DIR";
    if (S_ISCHR(mode))
        return "CHR";
    if (S_ISBLK(mode))
        return "BLK";
    if (S_ISFIFO(mode))
        return "FIFO";
    if (S_ISLNK(mode))
        return "LINK";
    if (S_ISSOCK(mode))
        return "SOCK";
    return "UNKNOWN";
}

/* --- stat tests --- */

TEST(stat_regular_file) {
    char tmppath[] = "/tmp/lsof_test_stat_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    struct stat sb;
    ASSERT_EQ(test_stat_safely(tmppath, &sb), 0);
    ASSERT_TRUE(S_ISREG(sb.st_mode));
    ASSERT_STR_EQ(test_file_type_name(sb.st_mode), "REG");
    unlink(tmppath);
}

TEST(stat_directory) {
    struct stat sb;
    ASSERT_EQ(test_stat_safely("/tmp", &sb), 0);
    ASSERT_TRUE(S_ISDIR(sb.st_mode));
    ASSERT_STR_EQ(test_file_type_name(sb.st_mode), "DIR");
}

TEST(stat_dev_null) {
    struct stat sb;
    ASSERT_EQ(test_stat_safely("/dev/null", &sb), 0);
    ASSERT_TRUE(S_ISCHR(sb.st_mode));
    ASSERT_STR_EQ(test_file_type_name(sb.st_mode), "CHR");
}

TEST(stat_nonexistent) {
    struct stat sb;
    ASSERT_EQ(test_stat_safely("/nonexistent/path/that/does/not/exist_xyz", &sb), -1);
}

TEST(stat_null_path) {
    struct stat sb;
    ASSERT_EQ(test_stat_safely(NULL, &sb), -1);
}

TEST(stat_null_buf) {
    ASSERT_EQ(test_stat_safely("/tmp", NULL), -1);
}

TEST(stat_file_size) {
    char tmppath[] = "/tmp/lsof_test_statsz_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    write(fd, "0123456789", 10);
    close(fd);

    struct stat sb;
    ASSERT_EQ(test_stat_safely(tmppath, &sb), 0);
    ASSERT_EQ(sb.st_size, 10);
    unlink(tmppath);
}

TEST(stat_empty_file) {
    char tmppath[] = "/tmp/lsof_test_statempty_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    struct stat sb;
    ASSERT_EQ(test_stat_safely(tmppath, &sb), 0);
    ASSERT_EQ(sb.st_size, 0);
    unlink(tmppath);
}

TEST(stat_permissions) {
    char tmppath[] = "/tmp/lsof_test_statperm_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);
    chmod(tmppath, 0644);

    struct stat sb;
    ASSERT_EQ(test_stat_safely(tmppath, &sb), 0);
    ASSERT_EQ(sb.st_mode & 0777, 0644);
    unlink(tmppath);
}

TEST(stat_inode_nonzero) {
    struct stat sb;
    ASSERT_EQ(test_stat_safely("/tmp", &sb), 0);
    ASSERT_GT((long long)sb.st_ino, 0);
}

TEST(stat_nlink) {
    char tmppath[] = "/tmp/lsof_test_statnl_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    struct stat sb;
    ASSERT_EQ(test_stat_safely(tmppath, &sb), 0);
    ASSERT_GE((long long)sb.st_nlink, 1);
    unlink(tmppath);
}

TEST(stat_dev_zero) {
    struct stat sb;
    ASSERT_EQ(test_stat_safely("/dev/zero", &sb), 0);
    ASSERT_TRUE(S_ISCHR(sb.st_mode));
}

/* --- lstat tests --- */

TEST(lstat_symlink) {
    char tmppath[] = "/tmp/lsof_test_lstat_target_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    char linkpath[256];
    snprintf(linkpath, sizeof(linkpath), "%s_link", tmppath);
    if (symlink(tmppath, linkpath) < 0) {
        unlink(tmppath);
        return;
    }

    struct stat sb;
    ASSERT_EQ(test_lstat_safely(linkpath, &sb), 0);
    ASSERT_TRUE(S_ISLNK(sb.st_mode));
    ASSERT_STR_EQ(test_file_type_name(sb.st_mode), "LINK");

    unlink(linkpath);
    unlink(tmppath);
}

TEST(lstat_follows_regular) {
    char tmppath[] = "/tmp/lsof_test_lstatreg_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    struct stat sb;
    ASSERT_EQ(test_lstat_safely(tmppath, &sb), 0);
    ASSERT_TRUE(S_ISREG(sb.st_mode));
    unlink(tmppath);
}

TEST(stat_resolves_symlink) {
    char tmppath[] = "/tmp/lsof_test_statsym_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    char linkpath[256];
    snprintf(linkpath, sizeof(linkpath), "%s_symlink", tmppath);
    if (symlink(tmppath, linkpath) < 0) {
        unlink(tmppath);
        return;
    }

    struct stat sb;
    ASSERT_EQ(test_stat_safely(linkpath, &sb), 0);
    /* stat() should resolve symlink — see regular file, not link */
    ASSERT_TRUE(S_ISREG(sb.st_mode));

    unlink(linkpath);
    unlink(tmppath);
}

TEST(lstat_null_path) {
    struct stat sb;
    ASSERT_EQ(test_lstat_safely(NULL, &sb), -1);
}

TEST(lstat_null_buf) {
    ASSERT_EQ(test_lstat_safely("/tmp", NULL), -1);
}

/* --- file type classification tests --- */

TEST(file_type_name_reg) {
    ASSERT_STR_EQ(test_file_type_name(S_IFREG), "REG");
}

TEST(file_type_name_dir) {
    ASSERT_STR_EQ(test_file_type_name(S_IFDIR), "DIR");
}

TEST(file_type_name_chr) {
    ASSERT_STR_EQ(test_file_type_name(S_IFCHR), "CHR");
}

TEST(file_type_name_blk) {
    ASSERT_STR_EQ(test_file_type_name(S_IFBLK), "BLK");
}

TEST(file_type_name_fifo) {
    ASSERT_STR_EQ(test_file_type_name(S_IFIFO), "FIFO");
}

TEST(file_type_name_lnk) {
    ASSERT_STR_EQ(test_file_type_name(S_IFLNK), "LINK");
}

TEST(file_type_name_sock) {
    ASSERT_STR_EQ(test_file_type_name(S_IFSOCK), "SOCK");
}

TEST(file_type_name_unknown) {
    ASSERT_STR_EQ(test_file_type_name(0), "UNKNOWN");
}

/* --- device number extraction --- */

TEST(stat_dev_major_minor) {
    struct stat sb;
    ASSERT_EQ(test_stat_safely("/dev/null", &sb), 0);
    /* device numbers should be valid */
    ASSERT_GE((long long)major(sb.st_rdev), 0);
    ASSERT_GE((long long)minor(sb.st_rdev), 0);
}

TEST(stat_pipe_via_fifo) {
    char fifopath[] = "/tmp/lsof_test_fifo_XXXXXX";
    /* mkstemp creates a file, but we need a unique name for mkfifo */
    int fd = mkstemp(fifopath);
    if (fd >= 0) {
        close(fd);
        unlink(fifopath);
    }

    if (mkfifo(fifopath, 0666) < 0)
        return;

    struct stat sb;
    ASSERT_EQ(test_stat_safely(fifopath, &sb), 0);
    ASSERT_TRUE(S_ISFIFO(sb.st_mode));
    ASSERT_STR_EQ(test_file_type_name(sb.st_mode), "FIFO");
    unlink(fifopath);
}

#endif /* TEST_UNIT_STAT_H */
