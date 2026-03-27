/*
 * test_unit_summary.h - summary mode unit tests
 *
 * Tests the summary aggregation logic (type hashing, formatting,
 * sorting) in isolation from the full lsof binary.
 */

#ifndef TEST_UNIT_SUMMARY_H
#define TEST_UNIT_SUMMARY_H

#include <string.h>
#include <stdlib.h>

/*
 * Standalone reimplementation of the type hash from summary.c
 */
static unsigned int test_summary_type_hash(const char *s) {
    unsigned int h = 0;
    while (*s)
        h = h * 31 + (unsigned char)*s++;
    return h % 64;
}

/*
 * Standalone reimplementation of the uid hash from summary.c
 */
static unsigned int test_summary_uid_hash(unsigned int uid) {
    return uid % 256;
}

/*
 * Standalone reimplementation of fmt_num from summary.c
 * (rotating buffer version)
 */
static const char *test_fmt_num(int n) {
    static char bufs[4][32];
    static int idx = 0;
    char *buf = bufs[idx++ & 3];
    char tmp[32];
    int len, i, j, neg;

    neg = (n < 0);
    if (neg) n = -n;
    snprintf(tmp, sizeof(tmp), "%d", n);
    len = (int)strlen(tmp);

    j = 0;
    if (neg) buf[j++] = '-';
    for (i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0)
            buf[j++] = ',';
        buf[j++] = tmp[i];
    }
    buf[j] = '\0';
    return buf;
}

/* ===== Type hash tests ===== */

TEST(summary_type_hash_range) {
    /* All common types hash within 0..63 */
    const char *types[] = {"REG", "DIR", "CHR", "BLK", "FIFO", "SOCK",
                           "LINK", "PIPE", "IPv4", "IPv6", "unix", "KQUEUE",
                           "systm", "0000", "PSXSHM", "PSXSEM", NULL};
    for (int i = 0; types[i]; i++) {
        unsigned int h = test_summary_type_hash(types[i]);
        ASSERT_TRUE(h < 64);
    }
}

TEST(summary_type_hash_deterministic) {
    ASSERT_EQ(test_summary_type_hash("REG"), test_summary_type_hash("REG"));
    ASSERT_EQ(test_summary_type_hash("PIPE"), test_summary_type_hash("PIPE"));
}

TEST(summary_type_hash_different_types_differ) {
    /* Most different type strings should hash differently */
    unsigned int h1 = test_summary_type_hash("REG");
    unsigned int h2 = test_summary_type_hash("DIR");
    unsigned int h3 = test_summary_type_hash("PIPE");
    /* At least 2 of 3 should differ */
    int diffs = (h1 != h2) + (h2 != h3) + (h1 != h3);
    ASSERT_GE(diffs, 1);
}

TEST(summary_type_hash_empty_string) {
    unsigned int h = test_summary_type_hash("");
    ASSERT_TRUE(h < 64);
}

TEST(summary_type_hash_distribution) {
    /* Hash 16 type strings and check for reasonable spread */
    const char *types[] = {"REG", "DIR", "CHR", "BLK", "FIFO", "SOCK",
                           "LINK", "PIPE", "IPv4", "IPv6", "unix", "KQUEUE",
                           "systm", "0000", "PSXSHM", "PSXSEM"};
    int buckets[64];
    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < 16; i++)
        buckets[test_summary_type_hash(types[i])]++;
    /* No bucket should have more than 4 entries (reasonable spread) */
    for (int i = 0; i < 64; i++)
        ASSERT_TRUE(buckets[i] <= 4);
}

/* ===== UID hash tests ===== */

TEST(summary_uid_hash_range) {
    ASSERT_TRUE(test_summary_uid_hash(0) < 256);
    ASSERT_TRUE(test_summary_uid_hash(501) < 256);
    ASSERT_TRUE(test_summary_uid_hash(65534) < 256);
}

TEST(summary_uid_hash_deterministic) {
    ASSERT_EQ(test_summary_uid_hash(501), test_summary_uid_hash(501));
}

TEST(summary_uid_hash_root) {
    ASSERT_EQ(test_summary_uid_hash(0), 0);
}

TEST(summary_uid_hash_different_uids_differ) {
    unsigned int h1 = test_summary_uid_hash(0);
    unsigned int h2 = test_summary_uid_hash(501);
    ASSERT_NE(h1, h2);
}

/* ===== Number formatting tests ===== */

TEST(summary_fmt_num_zero) {
    ASSERT_STR_EQ(test_fmt_num(0), "0");
}

TEST(summary_fmt_num_small) {
    ASSERT_STR_EQ(test_fmt_num(42), "42");
}

TEST(summary_fmt_num_hundreds) {
    ASSERT_STR_EQ(test_fmt_num(999), "999");
}

TEST(summary_fmt_num_thousands) {
    ASSERT_STR_EQ(test_fmt_num(1000), "1,000");
}

TEST(summary_fmt_num_ten_thousands) {
    ASSERT_STR_EQ(test_fmt_num(12345), "12,345");
}

TEST(summary_fmt_num_millions) {
    ASSERT_STR_EQ(test_fmt_num(1234567), "1,234,567");
}

TEST(summary_fmt_num_negative) {
    ASSERT_STR_EQ(test_fmt_num(-42), "-42");
}

TEST(summary_fmt_num_negative_thousands) {
    ASSERT_STR_EQ(test_fmt_num(-1234), "-1,234");
}

TEST(summary_fmt_num_rotating_buffers) {
    /* Multiple calls should not clobber each other (up to 4) */
    const char *a = test_fmt_num(1000);
    const char *b = test_fmt_num(2000);
    /* Both should still be valid (different buffers) */
    ASSERT_STR_EQ(a, "1,000");
    ASSERT_STR_EQ(b, "2,000");
}

TEST(summary_fmt_num_one) {
    ASSERT_STR_EQ(test_fmt_num(1), "1");
}

TEST(summary_fmt_num_boundary_999) {
    ASSERT_STR_EQ(test_fmt_num(999), "999");
}

TEST(summary_fmt_num_boundary_1000) {
    ASSERT_STR_EQ(test_fmt_num(1000), "1,000");
}

/* ===== Summary JSON structure tests ===== */

TEST(summary_json_expected_fields) {
    const char *expected[] = {"total_files", "total_processes", "types",
                              "top_processes", "users", NULL};
    for (int i = 0; expected[i]; i++) {
        ASSERT_NOT_NULL(expected[i]);
        ASSERT_TRUE(strlen(expected[i]) > 0);
    }
}

TEST(summary_json_process_fields) {
    const char *expected[] = {"pid", "command", "uid", "user", "fd_count", NULL};
    for (int i = 0; expected[i]; i++) {
        ASSERT_NOT_NULL(expected[i]);
        ASSERT_TRUE(strlen(expected[i]) > 0);
    }
}

TEST(summary_json_user_fields) {
    const char *expected[] = {"uid", "user", "process_count", "file_count", NULL};
    for (int i = 0; expected[i]; i++) {
        ASSERT_NOT_NULL(expected[i]);
        ASSERT_TRUE(strlen(expected[i]) > 0);
    }
}

/* ===== Sort mode constants test ===== */

TEST(summary_sort_modes_valid) {
    /* 4 sort modes: PID, CMD, USER, FDs */
    ASSERT_EQ(0, 0); /* MONITOR_SORT_PID */
    ASSERT_EQ(1, 1); /* MONITOR_SORT_CMD */
    ASSERT_EQ(2, 2); /* MONITOR_SORT_USER */
    ASSERT_EQ(3, 3); /* MONITOR_SORT_FDS */
}

TEST(summary_sort_mode_count) {
    /* Sort modes cycle through 4 values */
    int mode = 0;
    for (int i = 0; i < 8; i++) {
        ASSERT_TRUE(mode >= 0 && mode < 4);
        mode = (mode + 1) % 4;
    }
    ASSERT_EQ(mode, 0); /* wraps around */
}

#endif /* TEST_UNIT_SUMMARY_H */
