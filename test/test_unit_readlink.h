/*
 * test_unit_readlink.h - readlink wrapper and symlink resolution tests
 */

#ifndef TEST_UNIT_READLINK_H
#define TEST_UNIT_READLINK_H

/* ===== Readlink() reimplementation ===== */
static char *test_readlink_safe(const char *path) {
    if (!path)
        return NULL;

    char *buf = (char *)malloc(PATH_MAX + 1);
    if (!buf)
        return NULL;

    ssize_t len = readlink(path, buf, PATH_MAX);
    if (len < 0) {
        free(buf);
        return NULL;
    }
    buf[len] = '\0';
    return buf;
}

TEST(readlink_valid_symlink) {
    char tmppath[] = "/tmp/lsof_test_rl_target_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    char linkpath[256];
    snprintf(linkpath, sizeof(linkpath), "%s_rl", tmppath);
    if (symlink(tmppath, linkpath) < 0) {
        unlink(tmppath);
        return;
    }

    char *target = test_readlink_safe(linkpath);
    ASSERT_NOT_NULL(target);
    ASSERT_STR_EQ(target, tmppath);
    free(target);

    unlink(linkpath);
    unlink(tmppath);
}

TEST(readlink_not_a_symlink) {
    char tmppath[] = "/tmp/lsof_test_rl_nolink_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    char *target = test_readlink_safe(tmppath);
    ASSERT_NULL(target);

    unlink(tmppath);
}

TEST(readlink_nonexistent) {
    char *target = test_readlink_safe("/nonexistent/path/rl_test_xyz");
    ASSERT_NULL(target);
}

TEST(readlink_null_path) {
    char *target = test_readlink_safe(NULL);
    ASSERT_NULL(target);
}

TEST(readlink_chain) {
    /* create target -> link1 -> link2, readlink should get one level */
    char tmppath[] = "/tmp/lsof_test_rl_chain_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    char link1[256], link2[256];
    snprintf(link1, sizeof(link1), "%s_chain1", tmppath);
    snprintf(link2, sizeof(link2), "%s_chain2", tmppath);

    if (symlink(tmppath, link1) < 0) {
        unlink(tmppath);
        return;
    }
    if (symlink(link1, link2) < 0) {
        unlink(link1);
        unlink(tmppath);
        return;
    }

    /* readlink on link2 should return link1, not tmppath */
    char *target = test_readlink_safe(link2);
    ASSERT_NOT_NULL(target);
    ASSERT_STR_EQ(target, link1);
    free(target);

    unlink(link2);
    unlink(link1);
    unlink(tmppath);
}

TEST(readlink_dev_stdin) {
    /* /dev/stdin is typically a symlink */
    char *target = test_readlink_safe("/dev/stdin");
    if (target) {
        /* if it resolves, it should be non-empty */
        ASSERT_GT((long long)strlen(target), 0);
        free(target);
    }
    /* it's ok if /dev/stdin is not a symlink on some platforms */
}

TEST(readlink_relative_target) {
    /* create a symlink with a relative target */
    char tmppath[] = "/tmp/lsof_test_rl_rel_XXXXXX";
    int fd = mkstemp(tmppath);
    ASSERT_TRUE(fd >= 0);
    close(fd);

    char linkpath[256];
    snprintf(linkpath, sizeof(linkpath), "%s_rel_link", tmppath);

    /* extract just the filename for relative symlink */
    char *base = strrchr(tmppath, '/');
    ASSERT_NOT_NULL(base);
    base++;

    if (symlink(base, linkpath) < 0) {
        unlink(tmppath);
        return;
    }

    char *target = test_readlink_safe(linkpath);
    ASSERT_NOT_NULL(target);
    ASSERT_STR_EQ(target, base);
    free(target);

    unlink(linkpath);
    unlink(tmppath);
}

TEST(readlink_self_link) {
    /* a symlink pointing to itself — readlink should still resolve one level */
    char linkpath[] = "/tmp/lsof_test_rl_self_XXXXXX";
    int fd = mkstemp(linkpath);
    if (fd >= 0) {
        close(fd);
        unlink(linkpath);
    }

    if (symlink(linkpath, linkpath) < 0)
        return;

    char *target = test_readlink_safe(linkpath);
    ASSERT_NOT_NULL(target);
    ASSERT_STR_EQ(target, linkpath);
    free(target);

    unlink(linkpath);
}

#endif /* TEST_UNIT_READLINK_H */
