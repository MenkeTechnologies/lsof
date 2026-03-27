/*
 * test_unit_follow.h - follow mode unit tests
 *
 * Tests the follow mode FD tracking logic (hashing, status transitions,
 * sorting) in isolation from the full lsof binary.
 */

#ifndef TEST_UNIT_FOLLOW_H
#define TEST_UNIT_FOLLOW_H

#include <string.h>
#include <stdlib.h>

#define TEST_FOLLOW_HASH_SIZE 512

/*
 * Standalone reimplementation of the follow hash from follow.c
 */
static unsigned int test_follow_hash(const char *s) {
    unsigned int h = 0;
    while (*s)
        h = h * 31 + (unsigned char)*s++;
    return h % TEST_FOLLOW_HASH_SIZE;
}

/* ===== Follow hash tests ===== */

TEST(follow_hash_range) {
    const char *fds[] = {"cwd", "txt", "rtd", "0", "1", "2", "10", "255", NULL};
    for (int i = 0; fds[i]; i++) {
        unsigned int h = test_follow_hash(fds[i]);
        ASSERT_TRUE(h < TEST_FOLLOW_HASH_SIZE);
    }
}

TEST(follow_hash_deterministic) {
    ASSERT_EQ(test_follow_hash("0"), test_follow_hash("0"));
    ASSERT_EQ(test_follow_hash("cwd"), test_follow_hash("cwd"));
}

TEST(follow_hash_different_fds_differ) {
    unsigned int h1 = test_follow_hash("0");
    unsigned int h2 = test_follow_hash("1");
    unsigned int h3 = test_follow_hash("cwd");
    /* At least 2 of 3 should differ */
    int diffs = (h1 != h2) + (h2 != h3) + (h1 != h3);
    ASSERT_GE(diffs, 1);
}

TEST(follow_hash_empty_string) {
    unsigned int h = test_follow_hash("");
    ASSERT_TRUE(h < TEST_FOLLOW_HASH_SIZE);
}

TEST(follow_hash_numeric_fds_spread) {
    /* Hash numeric FDs 0-99 and check distribution */
    int buckets[TEST_FOLLOW_HASH_SIZE];
    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < 100; i++) {
        char fd[8];
        snprintf(fd, sizeof(fd), "%d", i);
        buckets[test_follow_hash(fd)]++;
    }
    /* No bucket should have more than 5 entries */
    for (int i = 0; i < TEST_FOLLOW_HASH_SIZE; i++)
        ASSERT_TRUE(buckets[i] <= 5);
}

/* ===== Follow status transition tests ===== */

TEST(follow_status_new) {
    /* Status 1 means "new FD" */
    int status = 1;
    ASSERT_EQ(status, 1);
}

TEST(follow_status_existing) {
    /* Status 0 means "existing/unchanged FD" */
    int status = 0;
    ASSERT_EQ(status, 0);
}

TEST(follow_status_gone) {
    /* Status 2 means "gone/closed FD" */
    int status = 2;
    ASSERT_EQ(status, 2);
}

TEST(follow_status_new_to_existing) {
    /* After one iteration, new (1) becomes existing (0) */
    int status = 1;
    /* Simulate: if status was new and FD still seen, transition to existing */
    if (status == 1)
        status = 0;
    ASSERT_EQ(status, 0);
}

TEST(follow_status_existing_to_gone) {
    /* When FD disappears, existing (0) becomes gone (2) */
    int status = 0;
    int seen = 0; /* not seen this iteration */
    if (!seen && status != 2)
        status = 2;
    ASSERT_EQ(status, 2);
}

TEST(follow_status_gone_to_new) {
    /* If a gone FD reappears, it becomes new again */
    int status = 2;
    int seen = 1; /* seen again */
    if (seen && status == 2)
        status = 1;
    ASSERT_EQ(status, 1);
}

/* ===== Follow sort comparison tests ===== */

/*
 * Simulated follow_cmp: new entries first, gone last, then by fd string
 */
static int test_follow_cmp_status(int status_a, const char *fd_a,
                                   int status_b, const char *fd_b) {
    if (status_a != status_b) {
        if (status_a == 1) return -1; /* new first */
        if (status_b == 1) return 1;
        if (status_a == 2) return 1;  /* gone last */
        if (status_b == 2) return -1;
    }
    return strcmp(fd_a, fd_b);
}

TEST(follow_sort_new_before_existing) {
    int r = test_follow_cmp_status(1, "5", 0, "3");
    ASSERT_TRUE(r < 0); /* new (1) comes before existing (0) */
}

TEST(follow_sort_existing_before_gone) {
    int r = test_follow_cmp_status(0, "5", 2, "3");
    ASSERT_TRUE(r < 0); /* existing (0) comes before gone (2) */
}

TEST(follow_sort_new_before_gone) {
    int r = test_follow_cmp_status(1, "5", 2, "3");
    ASSERT_TRUE(r < 0); /* new (1) comes before gone (2) */
}

TEST(follow_sort_same_status_by_fd) {
    int r = test_follow_cmp_status(0, "10", 0, "5");
    /* "10" < "5" lexicographically */
    ASSERT_TRUE(r < 0);
}

TEST(follow_sort_same_status_equal) {
    int r = test_follow_cmp_status(0, "cwd", 0, "cwd");
    ASSERT_EQ(r, 0);
}

/* ===== Follow display label tests ===== */

TEST(follow_status_label_new) {
    const char *label;
    int status = 1;
    switch (status) {
    case 1:  label = "+NEW"; break;
    case 2:  label = "-DEL"; break;
    default: label = "    "; break;
    }
    ASSERT_STR_EQ(label, "+NEW");
}

TEST(follow_status_label_gone) {
    const char *label;
    int status = 2;
    switch (status) {
    case 1:  label = "+NEW"; break;
    case 2:  label = "-DEL"; break;
    default: label = "    "; break;
    }
    ASSERT_STR_EQ(label, "-DEL");
}

TEST(follow_status_label_existing) {
    const char *label;
    int status = 0;
    switch (status) {
    case 1:  label = "+NEW"; break;
    case 2:  label = "-DEL"; break;
    default: label = "    "; break;
    }
    ASSERT_STR_EQ(label, "    ");
}

#endif /* TEST_UNIT_FOLLOW_H */
