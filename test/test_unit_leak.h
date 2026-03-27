/*
 * test_unit_leak.h - FD leak detection module unit tests
 *
 * Tests the leak detection hash table, recording, and threshold logic
 * in isolation (standalone reimplementation of the core algorithm).
 */

#ifndef TEST_UNIT_LEAK_H
#define TEST_UNIT_LEAK_H

#include <stdlib.h>
#include <string.h>

/* Replicate leak.c constants for standalone testing */
#define TEST_LEAK_HASH_SIZE  1024
#define TEST_LEAK_HISTORY    16

struct test_leak_sample {
    int fd_count;
};

struct test_leak_entry {
    int pid;
    char cmd[64];
    int uid;
    struct test_leak_sample history[TEST_LEAK_HISTORY];
    int history_count;
    int history_index;
    int consecutive_increases;
    int flagged;
    int seen;
    struct test_leak_entry *next;
};

static struct test_leak_entry *test_leak_table[TEST_LEAK_HASH_SIZE];

static unsigned int test_leak_hash(int pid) {
    return ((unsigned int)(pid * 31415) >> 3) & (TEST_LEAK_HASH_SIZE - 1);
}

static void test_leak_init(void) {
    memset(test_leak_table, 0, sizeof(test_leak_table));
}

static struct test_leak_entry *test_leak_find_or_create(int pid,
                                                         const char *cmd,
                                                         int uid) {
    unsigned int h = test_leak_hash(pid);
    struct test_leak_entry *e;

    for (e = test_leak_table[h]; e; e = e->next) {
        if (e->pid == pid) {
            if (strncmp(e->cmd, cmd, 63) != 0) {
                memset(e->history, 0, sizeof(e->history));
                e->history_count = 0;
                e->history_index = 0;
                e->consecutive_increases = 0;
                e->flagged = 0;
                strncpy(e->cmd, cmd, 63);
                e->cmd[63] = '\0';
                e->uid = uid;
            }
            return e;
        }
    }

    e = (struct test_leak_entry *)calloc(1, sizeof(*e));
    if (!e)
        return NULL;
    e->pid = pid;
    strncpy(e->cmd, cmd, 63);
    e->cmd[63] = '\0';
    e->uid = uid;
    e->next = test_leak_table[h];
    test_leak_table[h] = e;
    return e;
}

static void test_leak_record(int pid, const char *cmd, int uid,
                              int fd_count, int threshold) {
    struct test_leak_entry *e = test_leak_find_or_create(pid, cmd, uid);
    if (!e)
        return;

    e->seen = 1;

    if (e->history_count > 0) {
        int prev_idx =
            (e->history_index + TEST_LEAK_HISTORY - 1) % TEST_LEAK_HISTORY;
        if (fd_count > e->history[prev_idx].fd_count)
            e->consecutive_increases++;
        else
            e->consecutive_increases = 0;
    }

    e->history[e->history_index].fd_count = fd_count;
    e->history_index = (e->history_index + 1) % TEST_LEAK_HISTORY;
    if (e->history_count < TEST_LEAK_HISTORY)
        e->history_count++;

    if (e->consecutive_increases >= threshold && !e->flagged)
        e->flagged = 1;
}

static void test_leak_cleanup(void) {
    unsigned int h;
    struct test_leak_entry *e, *next;
    for (h = 0; h < TEST_LEAK_HASH_SIZE; h++) {
        for (e = test_leak_table[h]; e; e = next) {
            next = e->next;
            free(e);
        }
        test_leak_table[h] = NULL;
    }
}

/* ===== Hash function tests ===== */

TEST(leak_hash_range) {
    for (int pid = 0; pid < 100000; pid += 1000) {
        unsigned int h = test_leak_hash(pid);
        ASSERT_LT(h, TEST_LEAK_HASH_SIZE);
    }
}

TEST(leak_hash_deterministic) {
    ASSERT_EQ(test_leak_hash(1234), test_leak_hash(1234));
    ASSERT_EQ(test_leak_hash(0), test_leak_hash(0));
    ASSERT_EQ(test_leak_hash(99999), test_leak_hash(99999));
}

TEST(leak_hash_distribution) {
    int buckets[TEST_LEAK_HASH_SIZE];
    memset(buckets, 0, sizeof(buckets));
    for (int pid = 1; pid <= 10000; pid++) {
        buckets[test_leak_hash(pid)]++;
    }
    int used = 0;
    for (int i = 0; i < TEST_LEAK_HASH_SIZE; i++) {
        if (buckets[i] > 0)
            used++;
    }
    /* Expect at least half the buckets used for 10k PIDs */
    ASSERT_GT(used, TEST_LEAK_HASH_SIZE / 2);
}

TEST(leak_hash_adjacent_pids_differ) {
    int same = 0;
    for (int pid = 1; pid < 1000; pid++) {
        if (test_leak_hash(pid) == test_leak_hash(pid - 1))
            same++;
    }
    ASSERT_LT(same, 50);
}

/* ===== Entry creation tests ===== */

TEST(leak_find_or_create_new) {
    test_leak_init();
    struct test_leak_entry *e = test_leak_find_or_create(42, "testcmd", 501);
    ASSERT_NOT_NULL(e);
    ASSERT_EQ(e->pid, 42);
    ASSERT_STR_EQ(e->cmd, "testcmd");
    ASSERT_EQ(e->uid, 501);
    ASSERT_EQ(e->history_count, 0);
    ASSERT_EQ(e->consecutive_increases, 0);
    ASSERT_EQ(e->flagged, 0);
    test_leak_cleanup();
}

TEST(leak_find_or_create_existing) {
    test_leak_init();
    struct test_leak_entry *e1 = test_leak_find_or_create(42, "cmd", 0);
    struct test_leak_entry *e2 = test_leak_find_or_create(42, "cmd", 0);
    ASSERT_TRUE(e1 == e2);
    test_leak_cleanup();
}

TEST(leak_find_or_create_pid_reuse_resets) {
    test_leak_init();
    struct test_leak_entry *e = test_leak_find_or_create(42, "old_cmd", 0);
    e->consecutive_increases = 5;
    e->flagged = 1;
    e->history_count = 3;

    /* Same PID, different command = PID reuse */
    e = test_leak_find_or_create(42, "new_cmd", 0);
    ASSERT_STR_EQ(e->cmd, "new_cmd");
    ASSERT_EQ(e->consecutive_increases, 0);
    ASSERT_EQ(e->flagged, 0);
    ASSERT_EQ(e->history_count, 0);
    test_leak_cleanup();
}

TEST(leak_find_or_create_different_pids) {
    test_leak_init();
    struct test_leak_entry *e1 = test_leak_find_or_create(100, "a", 0);
    struct test_leak_entry *e2 = test_leak_find_or_create(200, "b", 0);
    ASSERT_TRUE(e1 != e2);
    ASSERT_EQ(e1->pid, 100);
    ASSERT_EQ(e2->pid, 200);
    test_leak_cleanup();
}

/* ===== Recording and threshold tests ===== */

TEST(leak_record_single_sample) {
    test_leak_init();
    test_leak_record(1, "proc", 0, 10, 3);
    struct test_leak_entry *e = test_leak_find_or_create(1, "proc", 0);
    ASSERT_EQ(e->history_count, 1);
    ASSERT_EQ(e->history[0].fd_count, 10);
    ASSERT_EQ(e->consecutive_increases, 0);
    ASSERT_EQ(e->flagged, 0);
    test_leak_cleanup();
}

TEST(leak_record_increasing_triggers_flag) {
    test_leak_init();
    /* Record 4 monotonically increasing FD counts with threshold=3 */
    test_leak_record(1, "leaker", 0, 10, 3);
    test_leak_record(1, "leaker", 0, 15, 3);
    test_leak_record(1, "leaker", 0, 20, 3);
    test_leak_record(1, "leaker", 0, 25, 3);

    struct test_leak_entry *e = test_leak_find_or_create(1, "leaker", 0);
    ASSERT_EQ(e->consecutive_increases, 3);
    ASSERT_EQ(e->flagged, 1);
    test_leak_cleanup();
}

TEST(leak_record_decrease_resets_count) {
    test_leak_init();
    test_leak_record(1, "proc", 0, 10, 3);
    test_leak_record(1, "proc", 0, 20, 3);
    test_leak_record(1, "proc", 0, 15, 3); /* decrease */
    test_leak_record(1, "proc", 0, 25, 3);

    struct test_leak_entry *e = test_leak_find_or_create(1, "proc", 0);
    ASSERT_EQ(e->consecutive_increases, 1);
    ASSERT_EQ(e->flagged, 0);
    test_leak_cleanup();
}

TEST(leak_record_equal_resets_count) {
    test_leak_init();
    test_leak_record(1, "proc", 0, 10, 3);
    test_leak_record(1, "proc", 0, 20, 3);
    test_leak_record(1, "proc", 0, 20, 3); /* equal, not increase */

    struct test_leak_entry *e = test_leak_find_or_create(1, "proc", 0);
    ASSERT_EQ(e->consecutive_increases, 0);
    ASSERT_EQ(e->flagged, 0);
    test_leak_cleanup();
}

TEST(leak_record_threshold_2) {
    test_leak_init();
    test_leak_record(1, "proc", 0, 5, 2);
    test_leak_record(1, "proc", 0, 10, 2);
    test_leak_record(1, "proc", 0, 15, 2);

    struct test_leak_entry *e = test_leak_find_or_create(1, "proc", 0);
    ASSERT_EQ(e->flagged, 1);
    test_leak_cleanup();
}

TEST(leak_record_just_below_threshold) {
    test_leak_init();
    test_leak_record(1, "proc", 0, 10, 5);
    test_leak_record(1, "proc", 0, 20, 5);
    test_leak_record(1, "proc", 0, 30, 5);
    test_leak_record(1, "proc", 0, 40, 5);

    struct test_leak_entry *e = test_leak_find_or_create(1, "proc", 0);
    ASSERT_EQ(e->consecutive_increases, 3);
    ASSERT_EQ(e->flagged, 0);
    test_leak_cleanup();
}

/* ===== Circular buffer tests ===== */

TEST(leak_history_circular_buffer) {
    test_leak_init();
    /* Fill more than LEAK_HISTORY entries */
    for (int i = 0; i < TEST_LEAK_HISTORY + 5; i++) {
        test_leak_record(1, "proc", 0, 100 + i, 999);
    }
    struct test_leak_entry *e = test_leak_find_or_create(1, "proc", 0);
    ASSERT_EQ(e->history_count, TEST_LEAK_HISTORY);
    test_leak_cleanup();
}

TEST(leak_history_wraps_correctly) {
    test_leak_init();
    for (int i = 0; i < TEST_LEAK_HISTORY * 2; i++) {
        test_leak_record(1, "proc", 0, i, 999);
    }
    struct test_leak_entry *e = test_leak_find_or_create(1, "proc", 0);
    /* The history_index should have wrapped */
    ASSERT_EQ(e->history_count, TEST_LEAK_HISTORY);
    /* Most recent value should be TEST_LEAK_HISTORY*2 - 1 */
    int prev = (e->history_index + TEST_LEAK_HISTORY - 1) % TEST_LEAK_HISTORY;
    ASSERT_EQ(e->history[prev].fd_count, TEST_LEAK_HISTORY * 2 - 1);
    test_leak_cleanup();
}

/* ===== Multiple process tests ===== */

TEST(leak_multiple_processes_independent) {
    test_leak_init();
    /* Process 1: leaking */
    test_leak_record(1, "leaker", 0, 10, 3);
    test_leak_record(1, "leaker", 0, 20, 3);
    test_leak_record(1, "leaker", 0, 30, 3);
    test_leak_record(1, "leaker", 0, 40, 3);

    /* Process 2: stable */
    test_leak_record(2, "stable", 0, 50, 3);
    test_leak_record(2, "stable", 0, 50, 3);
    test_leak_record(2, "stable", 0, 50, 3);

    struct test_leak_entry *e1 = test_leak_find_or_create(1, "leaker", 0);
    struct test_leak_entry *e2 = test_leak_find_or_create(2, "stable", 0);
    ASSERT_EQ(e1->flagged, 1);
    ASSERT_EQ(e2->flagged, 0);
    test_leak_cleanup();
}

/* ===== Cleanup tests ===== */

TEST(leak_cleanup_frees_all) {
    test_leak_init();
    for (int pid = 1; pid <= 100; pid++) {
        test_leak_find_or_create(pid, "cmd", 0);
    }
    test_leak_cleanup();
    /* After cleanup, all buckets should be NULL */
    for (int i = 0; i < TEST_LEAK_HASH_SIZE; i++) {
        ASSERT_NULL(test_leak_table[i]);
    }
}

TEST(leak_flag_stays_set_after_more_records) {
    test_leak_init();
    /* Trigger flag */
    test_leak_record(1, "leaker", 0, 10, 3);
    test_leak_record(1, "leaker", 0, 20, 3);
    test_leak_record(1, "leaker", 0, 30, 3);
    test_leak_record(1, "leaker", 0, 40, 3);

    struct test_leak_entry *e = test_leak_find_or_create(1, "leaker", 0);
    ASSERT_EQ(e->flagged, 1);

    /* Even if FD count decreases, flag stays set */
    test_leak_record(1, "leaker", 0, 5, 3);
    ASSERT_EQ(e->flagged, 1);
    test_leak_cleanup();
}

#endif /* TEST_UNIT_LEAK_H */
