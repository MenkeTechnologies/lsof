/*
 * test_unit_devlookup.h - device table operations and lookup tests
 *
 * Tests for device table building, binary search, and lookups
 */

#ifndef TEST_UNIT_DEVLOOKUP_H
#define TEST_UNIT_DEVLOOKUP_H

/* ===== Device table reimplementation for testing ===== */
struct test_devlookup_entry {
    dev_t rdev;
    unsigned long long inode;
    char *name;
    int verified;
};

static int test_compdev_for_bsearch(const void *k, const void *e) {
    const struct test_devlookup_entry *key = (const struct test_devlookup_entry *)k;
    const struct test_devlookup_entry *entry = (const struct test_devlookup_entry *)e;
    if (key->rdev < entry->rdev)
        return -1;
    if (key->rdev > entry->rdev)
        return 1;
    if (key->inode < entry->inode)
        return -1;
    if (key->inode > entry->inode)
        return 1;
    return 0;
}

static struct test_devlookup_entry *test_dev_lookup(struct test_devlookup_entry *table, int n, dev_t rdev,
                                          unsigned long long inode) {
    struct test_devlookup_entry key;
    key.rdev = rdev;
    key.inode = inode;
    key.name = NULL;
    key.verified = 0;
    return (struct test_devlookup_entry *)bsearch(&key, table, (size_t)n, sizeof(struct test_devlookup_entry),
                                        test_compdev_for_bsearch);
}

/* ===== Tests ===== */

TEST(devlookup_found) {
    struct test_devlookup_entry table[] = {
        {1, 100, "/dev/a", 0},
        {2, 200, "/dev/b", 0},
        {3, 300, "/dev/c", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 3, 2, 200);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result->name, "/dev/b");
}

TEST(devlookup_not_found) {
    struct test_devlookup_entry table[] = {
        {1, 100, "/dev/a", 0},
        {2, 200, "/dev/b", 0},
        {3, 300, "/dev/c", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 3, 4, 400);
    ASSERT_NULL(result);
}

TEST(devlookup_first_element) {
    struct test_devlookup_entry table[] = {
        {1, 100, "/dev/a", 0},
        {2, 200, "/dev/b", 0},
        {3, 300, "/dev/c", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 3, 1, 100);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result->name, "/dev/a");
}

TEST(devlookup_last_element) {
    struct test_devlookup_entry table[] = {
        {1, 100, "/dev/a", 0},
        {2, 200, "/dev/b", 0},
        {3, 300, "/dev/c", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 3, 3, 300);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result->name, "/dev/c");
}

TEST(devlookup_single_element_found) {
    struct test_devlookup_entry table[] = {{5, 500, "/dev/x", 0}};
    struct test_devlookup_entry *result = test_dev_lookup(table, 1, 5, 500);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result->name, "/dev/x");
}

TEST(devlookup_single_element_miss) {
    struct test_devlookup_entry table[] = {{5, 500, "/dev/x", 0}};
    struct test_devlookup_entry *result = test_dev_lookup(table, 1, 5, 501);
    ASSERT_NULL(result);
}

TEST(devlookup_same_rdev_diff_inode) {
    struct test_devlookup_entry table[] = {
        {1, 100, "/dev/sda1", 0},
        {1, 200, "/dev/sda2", 0},
        {1, 300, "/dev/sda3", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 3, 1, 200);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result->name, "/dev/sda2");
}

TEST(devlookup_same_rdev_miss_inode) {
    struct test_devlookup_entry table[] = {
        {1, 100, "/dev/sda1", 0},
        {1, 200, "/dev/sda2", 0},
        {1, 300, "/dev/sda3", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 3, 1, 150);
    ASSERT_NULL(result);
}

TEST(devlookup_empty_table) {
    struct test_devlookup_entry *result = test_dev_lookup(NULL, 0, 1, 100);
    ASSERT_NULL(result);
}

TEST(devlookup_large_values) {
    struct test_devlookup_entry table[] = {
        {0xFFFFFFF0, 0xFFFFFFFFFFFF0000ULL, "/dev/big1", 0},
        {0xFFFFFFF1, 0xFFFFFFFFFFFF0001ULL, "/dev/big2", 0},
        {0xFFFFFFF2, 0xFFFFFFFFFFFF0002ULL, "/dev/big3", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 3, 0xFFFFFFF1, 0xFFFFFFFFFFFF0001ULL);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result->name, "/dev/big2");
}

TEST(devlookup_zero_device) {
    struct test_devlookup_entry table[] = {
        {0, 0, "/dev/zero_dev", 0},
        {1, 0, "/dev/one_dev", 0},
    };
    struct test_devlookup_entry *result = test_dev_lookup(table, 2, 0, 0);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result->name, "/dev/zero_dev");
}

/* --- comparator for qsort --- */

static int test_compdev_qsort(const void *a, const void *b) {
    return test_compdev_for_bsearch(a, b);
}

TEST(devlookup_sort_then_search) {
    struct test_devlookup_entry table[] = {
        {5, 500, "/dev/e", 0}, {1, 100, "/dev/a", 0}, {3, 300, "/dev/c", 0},
        {2, 200, "/dev/b", 0}, {4, 400, "/dev/d", 0},
    };
    qsort(table, 5, sizeof(struct test_devlookup_entry), test_compdev_qsort);

    /* after sort, should find all */
    ASSERT_NOT_NULL(test_dev_lookup(table, 5, 1, 100));
    ASSERT_NOT_NULL(test_dev_lookup(table, 5, 2, 200));
    ASSERT_NOT_NULL(test_dev_lookup(table, 5, 3, 300));
    ASSERT_NOT_NULL(test_dev_lookup(table, 5, 4, 400));
    ASSERT_NOT_NULL(test_dev_lookup(table, 5, 5, 500));
    ASSERT_NULL(test_dev_lookup(table, 5, 6, 600));
}

TEST(devlookup_sort_preserves_names) {
    struct test_devlookup_entry table[] = {
        {3, 300, "/dev/c", 0},
        {1, 100, "/dev/a", 0},
        {2, 200, "/dev/b", 0},
    };
    qsort(table, 3, sizeof(struct test_devlookup_entry), test_compdev_qsort);

    ASSERT_STR_EQ(table[0].name, "/dev/a");
    ASSERT_STR_EQ(table[1].name, "/dev/b");
    ASSERT_STR_EQ(table[2].name, "/dev/c");
}

TEST(devlookup_binary_search_boundary) {
    /* test searching at boundaries of a larger table */
    struct test_devlookup_entry table[20];
    for (int i = 0; i < 20; i++) {
        table[i].rdev = (dev_t)(i * 10);
        table[i].inode = (unsigned long long)(i * 100);
        table[i].name = "dev";
        table[i].verified = 0;
    }

    /* first */
    ASSERT_NOT_NULL(test_dev_lookup(table, 20, 0, 0));
    /* last */
    ASSERT_NOT_NULL(test_dev_lookup(table, 20, 190, 1900));
    /* middle */
    ASSERT_NOT_NULL(test_dev_lookup(table, 20, 100, 1000));
    /* not found */
    ASSERT_NULL(test_dev_lookup(table, 20, 5, 50));
}

#endif /* TEST_UNIT_DEVLOOKUP_H */
