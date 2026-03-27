/*
 * test_unit_strftime.h - time formatting tests
 *
 * Tests for util_strftime wrapper and time-related conversions
 */

#ifndef TEST_UNIT_STRFTIME_H
#define TEST_UNIT_STRFTIME_H

#include <time.h>

/* ===== util_strftime() reimplementation ===== */
static int test_util_strftime(char *buf, size_t bufsz, const char *fmt, time_t t) {
    if (!buf || bufsz == 0 || !fmt)
        return -1;
    struct tm *tm = localtime(&t);
    if (!tm)
        return -1;
    size_t n = strftime(buf, bufsz, fmt, tm);
    if (n == 0) {
        buf[0] = '\0';
        return -1;
    }
    return (int)n;
}

TEST(strftime_basic) {
    char buf[64];
    time_t t = 0; /* epoch */
    int n = test_util_strftime(buf, sizeof(buf), "%Y", t);
    ASSERT_GT(n, 0);
    /* localtime(0) may be 1969 or 1970 depending on timezone */
    int year = atoi(buf);
    ASSERT_TRUE(year == 1969 || year == 1970);
}

TEST(strftime_full_date) {
    char buf[64];
    /* 2024-01-15 00:00:00 UTC */
    time_t t = 1705276800;
    int n = test_util_strftime(buf, sizeof(buf), "%Y-%m-%d", t);
    ASSERT_GT(n, 0);
    /* the date depends on timezone, but should be non-empty */
    ASSERT_GT((long long)strlen(buf), 0);
    ASSERT_TRUE(buf[4] == '-');
    ASSERT_TRUE(buf[7] == '-');
}

TEST(strftime_time_only) {
    char buf[64];
    time_t t = time(NULL);
    int n = test_util_strftime(buf, sizeof(buf), "%H:%M:%S", t);
    ASSERT_GT(n, 0);
    ASSERT_EQ((long long)strlen(buf), 8);
    ASSERT_EQ(buf[2], ':');
    ASSERT_EQ(buf[5], ':');
}

TEST(strftime_null_buf) {
    ASSERT_EQ(test_util_strftime(NULL, 64, "%Y", time(NULL)), -1);
}

TEST(strftime_zero_bufsz) {
    char buf[64];
    ASSERT_EQ(test_util_strftime(buf, 0, "%Y", time(NULL)), -1);
}

TEST(strftime_null_fmt) {
    char buf[64];
    ASSERT_EQ(test_util_strftime(buf, sizeof(buf), NULL, time(NULL)), -1);
}

TEST(strftime_small_buffer) {
    char buf[2];
    /* "%Y" needs 4+1 bytes, buffer has 2 — should fail or truncate */
    int n = test_util_strftime(buf, sizeof(buf), "%Y", 0);
    ASSERT_EQ(n, -1);
}

TEST(strftime_epoch) {
    char buf[64];
    time_t t = 0;
    int n = test_util_strftime(buf, sizeof(buf), "%s", t);
    if (n > 0) {
        /* %s produces seconds since epoch */
        ASSERT_STR_EQ(buf, "0");
    }
    /* some platforms may not support %s — that's ok */
}

TEST(strftime_day_of_week) {
    char buf[64];
    time_t t = time(NULL);
    int n = test_util_strftime(buf, sizeof(buf), "%A", t);
    ASSERT_GT(n, 0);
    ASSERT_GT((long long)strlen(buf), 0);
}

TEST(strftime_percent_literal) {
    char buf[64];
    int n = test_util_strftime(buf, sizeof(buf), "%%", time(NULL));
    ASSERT_GT(n, 0);
    ASSERT_STR_EQ(buf, "%");
}

TEST(strftime_mixed_text) {
    char buf[128];
    time_t t = time(NULL);
    int n = test_util_strftime(buf, sizeof(buf), "date: %Y year", t);
    ASSERT_GT(n, 0);
    ASSERT_TRUE(strstr(buf, "date: ") == buf);
    ASSERT_NOT_NULL(strstr(buf, " year"));
}

TEST(strftime_current_time_reasonable) {
    char buf[64];
    time_t t = time(NULL);
    int n = test_util_strftime(buf, sizeof(buf), "%Y", t);
    ASSERT_GT(n, 0);
    int year = atoi(buf);
    ASSERT_GE(year, 2024);
    ASSERT_LT(year, 2100);
}

#endif /* TEST_UNIT_STRFTIME_H */
