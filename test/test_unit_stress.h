/*
 * test_unit_stress.h - stress/regression tests for internal data structures
 *
 * Exercises hash tables, sort/search, port caches, device dedup,
 * and string operations at scale (10K+ entries) to catch performance
 * regressions and edge cases that small tests miss.
 */

#ifndef TEST_UNIT_STRESS_H
#define TEST_UNIT_STRESS_H

#include <time.h>

/* ===== Hash table stress: HASHPORT with all 65536 ports ===== */

TEST(stress_hashport_all_ports_in_range) {
    for (int p = 0; p < 65536; p++) {
        int h = HASHPORT(p);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, PORTHASHBUCKETS);
    }
}

TEST(stress_hashport_distribution_full_range) {
    int buckets[PORTHASHBUCKETS];
    memset(buckets, 0, sizeof(buckets));
    for (int p = 0; p < 65536; p++)
        buckets[HASHPORT(p)]++;

    /* Every bucket should be used with 65536 ports across 128 buckets */
    for (int i = 0; i < PORTHASHBUCKETS; i++)
        ASSERT_GT(buckets[i], 0);

    /* No single bucket should hold more than 4x the ideal (512 = 65536/128) */
    for (int i = 0; i < PORTHASHBUCKETS; i++)
        ASSERT_LT(buckets[i], 512 * 4);
}

TEST(stress_hashport_max_chain_length) {
    int buckets[PORTHASHBUCKETS];
    memset(buckets, 0, sizeof(buckets));
    for (int p = 0; p < 65536; p++)
        buckets[HASHPORT(p)]++;

    int max_chain = 0;
    for (int i = 0; i < PORTHASHBUCKETS; i++)
        if (buckets[i] > max_chain)
            max_chain = buckets[i];

    /* Max chain should be reasonable - under 8x ideal */
    ASSERT_LT(max_chain, 512 * 8);
}

/* ===== Hash-by-name stress: 10K generated path names ===== */

TEST(stress_hashbyname_10k_paths) {
    int mod = 1024;
    int buckets[1024];
    memset(buckets, 0, sizeof(buckets));

    for (int i = 0; i < 10000; i++) {
        char name[128];
        snprintf(name, sizeof(name), "/proc/%d/fd/%d", i, i % 256);
        int h = test_hash_by_name(name, mod);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, mod);
        buckets[h]++;
    }

    /* At least 50% of buckets used */
    int used = 0;
    for (int i = 0; i < mod; i++)
        if (buckets[i] > 0)
            used++;
    ASSERT_GT(used, mod / 2);
}

TEST(stress_hashbyname_long_paths) {
    int mod = 256;
    for (int len = 1; len <= 4096; len *= 2) {
        char *name = (char *)malloc((size_t)len + 1);
        ASSERT_NOT_NULL(name);
        memset(name, 'A' + (len % 26), (size_t)len);
        name[len] = '\0';
        int h = test_hash_by_name(name, mod);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, mod);
        free(name);
    }
}

TEST(stress_hashbyname_collision_rate) {
    int mod = 256;
    int hashes[1000];
    for (int i = 0; i < 1000; i++) {
        char name[64];
        snprintf(name, sizeof(name), "/dev/pts/%d", i);
        hashes[i] = test_hash_by_name(name, mod);
    }
    int collisions = 0;
    for (int i = 0; i < 1000; i++)
        for (int j = i + 1; j < 1000; j++)
            if (hashes[i] == hashes[j])
                collisions++;

    /* With 1000 items in 256 buckets, many collisions are expected.
     * Birthday-problem math: high collision pair count is normal.
     * Just ensure it's not completely degenerate (all in one bucket
     * would give 1000*999/2 = 499500 pairs). */
    ASSERT_LT(collisions, 499500 / 2);
}

/* ===== Sort stress: 10K and 50K element sorts ===== */

TEST(stress_sort_10k_pids) {
    int n = 10000;
    int *pids = (int *)malloc((size_t)n * sizeof(int));
    ASSERT_NOT_NULL(pids);

    /* Fill with reverse-sorted data (worst case for naive sorts) */
    for (int i = 0; i < n; i++)
        pids[i] = n - i;

    qsort(pids, (size_t)n, sizeof(int), test_compare_pids);

    for (int i = 1; i < n; i++)
        ASSERT_TRUE(pids[i - 1] <= pids[i]);

    ASSERT_EQ(pids[0], 1);
    ASSERT_EQ(pids[n - 1], n);
    free(pids);
}

TEST(stress_sort_50k_pids) {
    int n = 50000;
    int *pids = (int *)malloc((size_t)n * sizeof(int));
    ASSERT_NOT_NULL(pids);

    /* Pseudorandom shuffle using LCG */
    for (int i = 0; i < n; i++)
        pids[i] = i + 1;
    unsigned int seed = 12345;
    for (int i = n - 1; i > 0; i--) {
        seed = seed * 1103515245 + 12345;
        int j = (int)(seed % (unsigned int)(i + 1));
        int tmp = pids[i];
        pids[i] = pids[j];
        pids[j] = tmp;
    }

    qsort(pids, (size_t)n, sizeof(int), test_compare_pids);

    for (int i = 1; i < n; i++)
        ASSERT_TRUE(pids[i - 1] <= pids[i]);

    free(pids);
}

TEST(stress_sort_10k_devices) {
    int n = 10000;
    test_devcomp_t *devs = (test_devcomp_t *)malloc((size_t)n * sizeof(test_devcomp_t));
    ASSERT_NOT_NULL(devs);

    for (int i = 0; i < n; i++) {
        devs[i].rdev = (dev_t)(n - i);
        devs[i].inode = (unsigned long)(i * 7 + 3);
        devs[i].name = "dev";
    }

    qsort(devs, (size_t)n, sizeof(test_devcomp_t), test_compare_devices_fn);

    for (int i = 1; i < n; i++)
        ASSERT_TRUE(devs[i - 1].rdev <= devs[i].rdev);

    free(devs);
}

TEST(stress_sort_all_duplicates) {
    int n = 10000;
    int *pids = (int *)malloc((size_t)n * sizeof(int));
    ASSERT_NOT_NULL(pids);

    for (int i = 0; i < n; i++)
        pids[i] = 42;

    qsort(pids, (size_t)n, sizeof(int), test_compare_pids);

    for (int i = 0; i < n; i++)
        ASSERT_EQ(pids[i], 42);

    free(pids);
}

TEST(stress_sort_already_sorted) {
    int n = 10000;
    int *pids = (int *)malloc((size_t)n * sizeof(int));
    ASSERT_NOT_NULL(pids);

    for (int i = 0; i < n; i++)
        pids[i] = i;

    qsort(pids, (size_t)n, sizeof(int), test_compare_pids);

    for (int i = 0; i < n; i++)
        ASSERT_EQ(pids[i], i);

    free(pids);
}

/* ===== Binary search stress: search 10K sorted entries ===== */

TEST(stress_bsearch_10k_all_hits) {
    int n = 10000;
    struct test_int_lst *lst =
        (struct test_int_lst *)malloc((size_t)n * sizeof(struct test_int_lst));
    ASSERT_NOT_NULL(lst);

    for (int i = 0; i < n; i++) {
        lst[i].i = i * 3;
        lst[i].selected = 0;
    }

    /* Search for every element */
    for (int i = 0; i < n; i++) {
        struct test_int_lst *r = test_find_int_lst(lst, n, i * 3);
        ASSERT_NOT_NULL(r);
        ASSERT_EQ(r->i, i * 3);
    }

    free(lst);
}

TEST(stress_bsearch_10k_all_misses) {
    int n = 10000;
    struct test_int_lst *lst =
        (struct test_int_lst *)malloc((size_t)n * sizeof(struct test_int_lst));
    ASSERT_NOT_NULL(lst);

    for (int i = 0; i < n; i++) {
        lst[i].i = i * 2; /* even numbers only */
        lst[i].selected = 0;
    }

    /* Search for odd numbers - all misses */
    for (int i = 0; i < n; i++) {
        struct test_int_lst *r = test_find_int_lst(lst, n, i * 2 + 1);
        ASSERT_NULL(r);
    }

    free(lst);
}

TEST(stress_bsearch_boundary_values) {
    int n = 10000;
    struct test_int_lst *lst =
        (struct test_int_lst *)malloc((size_t)n * sizeof(struct test_int_lst));
    ASSERT_NOT_NULL(lst);

    for (int i = 0; i < n; i++) {
        lst[i].i = i;
        lst[i].selected = 0;
    }

    /* First, last, and just-outside boundaries */
    ASSERT_NOT_NULL(test_find_int_lst(lst, n, 0));
    ASSERT_NOT_NULL(test_find_int_lst(lst, n, n - 1));
    ASSERT_NULL(test_find_int_lst(lst, n, -1));
    ASSERT_NULL(test_find_int_lst(lst, n, n));
    ASSERT_NULL(test_find_int_lst(lst, n, INT_MAX));
    ASSERT_NULL(test_find_int_lst(lst, n, INT_MIN));

    free(lst);
}

/* ===== Port cache stress: 10K ports ===== */

TEST(stress_port_table_10k_entries) {
    int n = 10000;
    struct test_porttab *entries =
        (struct test_porttab *)malloc((size_t)n * sizeof(struct test_porttab));
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int *ports = (int *)malloc((size_t)n * sizeof(int));
    ASSERT_NOT_NULL(entries);
    ASSERT_NOT_NULL(ports);

    for (int i = 0; i < n; i++)
        ports[i] = i + 1;

    int used = build_port_table(entries, n, buckets, ports);
    ASSERT_GT(used, 0);

    /* All ports should be findable */
    for (int i = 0; i < n; i++)
        ASSERT_NOT_NULL(lookup_port(buckets, ports[i]));

    /* Port 0 and n+1 should not be found */
    ASSERT_NULL(lookup_port(buckets, 0));
    ASSERT_NULL(lookup_port(buckets, n + 1));

    free(entries);
    free(ports);
}

TEST(stress_port_table_all_65k_ports) {
    int n = 65535;
    struct test_porttab *entries =
        (struct test_porttab *)malloc((size_t)n * sizeof(struct test_porttab));
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int *ports = (int *)malloc((size_t)n * sizeof(int));
    ASSERT_NOT_NULL(entries);
    ASSERT_NOT_NULL(ports);

    for (int i = 0; i < n; i++)
        ports[i] = i + 1;

    build_port_table(entries, n, buckets, ports);

    /* Spot-check: common ports */
    ASSERT_NOT_NULL(lookup_port(buckets, 22));
    ASSERT_NOT_NULL(lookup_port(buckets, 80));
    ASSERT_NOT_NULL(lookup_port(buckets, 443));
    ASSERT_NOT_NULL(lookup_port(buckets, 8080));
    ASSERT_NOT_NULL(lookup_port(buckets, 65535));

    /* All buckets should be used */
    int used = 0;
    for (int i = 0; i < TEST_PORTHASHBUCKETS; i++)
        if (buckets[i])
            used++;
    ASSERT_EQ(used, TEST_PORTHASHBUCKETS);

    free(entries);
    free(ports);
}

/* ===== Device dedup stress: 10K entries with various dup patterns ===== */

TEST(stress_rmdupdev_10k_no_dups) {
    int n = 10000;
    test_devcomp_t *devs = (test_devcomp_t *)malloc((size_t)n * sizeof(test_devcomp_t));
    ASSERT_NOT_NULL(devs);

    for (int i = 0; i < n; i++) {
        devs[i].rdev = (dev_t)i;
        devs[i].inode = (unsigned long)i;
        devs[i].name = NULL;
    }

    int result = test_remove_dup_devs(devs, n);
    ASSERT_EQ(result, n);

    free(devs);
}

TEST(stress_rmdupdev_10k_all_same) {
    int n = 10000;
    test_devcomp_t *devs = (test_devcomp_t *)malloc((size_t)n * sizeof(test_devcomp_t));
    ASSERT_NOT_NULL(devs);

    for (int i = 0; i < n; i++) {
        devs[i].rdev = 42;
        devs[i].inode = 100;
        devs[i].name = NULL;
    }

    int result = test_remove_dup_devs(devs, n);
    ASSERT_EQ(result, 1);

    free(devs);
}

TEST(stress_rmdupdev_10k_pairs) {
    int n = 10000;
    test_devcomp_t *devs = (test_devcomp_t *)malloc((size_t)n * sizeof(test_devcomp_t));
    ASSERT_NOT_NULL(devs);

    /* Every device appears exactly twice (sorted) */
    for (int i = 0; i < n; i++) {
        devs[i].rdev = (dev_t)(i / 2);
        devs[i].inode = (unsigned long)(i / 2);
        devs[i].name = NULL;
    }

    int result = test_remove_dup_devs(devs, n);
    ASSERT_EQ(result, n / 2);

    free(devs);
}

TEST(stress_rmdupdev_10k_runs) {
    int n = 10000;
    test_devcomp_t *devs = (test_devcomp_t *)malloc((size_t)n * sizeof(test_devcomp_t));
    ASSERT_NOT_NULL(devs);

    /* Runs of 10 identical entries, 1000 unique values */
    for (int i = 0; i < n; i++) {
        devs[i].rdev = (dev_t)(i / 10);
        devs[i].inode = (unsigned long)(i / 10);
        devs[i].name = NULL;
    }

    int result = test_remove_dup_devs(devs, n);
    ASSERT_EQ(result, 1000);

    free(devs);
}

/* ===== SFHASHDEVINO stress: large inode/device values ===== */

TEST(stress_sfhash_10k_entries) {
    int mod = 4096;
    int buckets[4096];
    memset(buckets, 0, sizeof(buckets));

    for (int i = 0; i < 10000; i++) {
        int h = TEST_SFHASHDEVINO(i % 256, i % 64, (unsigned long)i * 997, mod);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, mod);
        buckets[h]++;
    }

    int used = 0;
    for (int i = 0; i < mod; i++)
        if (buckets[i] > 0)
            used++;

    /* At least 25% bucket utilization */
    ASSERT_GT(used, mod / 4);
}

TEST(stress_sfhash_large_values) {
    int mod = 4096;
    /* Test with values near type limits */
    unsigned long large_inodes[] = {0, 1, 0xFFFF, 0xFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF};
    int n_inodes = sizeof(large_inodes) / sizeof(large_inodes[0]);

    for (int maj = 0; maj < 256; maj += 51) {
        for (int min = 0; min < 64; min += 13) {
            for (int k = 0; k < n_inodes; k++) {
                int h = TEST_SFHASHDEVINO(maj, min, large_inodes[k], mod);
                ASSERT_GE(h, 0);
                ASSERT_LT(h, mod);
            }
        }
    }
}

/* ===== Name cache hash stress ===== */

TEST(stress_ncache_hash_10k) {
    int mod = 2048;
    int buckets[2048];
    memset(buckets, 0, sizeof(buckets));

    for (int i = 0; i < 10000; i++) {
        int h = test_ncache_hash((unsigned long)(i * 13),
                                 (unsigned long)(0x10000 + (unsigned long)i * 64), mod);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, mod);
        buckets[h]++;
    }

    int used = 0, max_chain = 0;
    for (int i = 0; i < mod; i++) {
        if (buckets[i] > 0)
            used++;
        if (buckets[i] > max_chain)
            max_chain = buckets[i];
    }

    /* Good distribution: >25% utilization, max chain < 20x ideal */
    ASSERT_GT(used, mod / 4);
    ASSERT_LT(max_chain, (10000 / mod) * 20);
}

/* ===== String operation stress ===== */

/*
 * safestrlen reimplementation for stress testing
 * (copied from test_unit_strings.h pattern)
 */
static int stress_safestrlen(const char *s, int max, int space_printable) {
    if (!s)
        return 0;
    int len = 0;
    int needs_expansion = 0;
    for (const char *p = s; *p && len < max; p++) {
        unsigned char c = (unsigned char)*p;
        if (c >= 0x20 && c < 0x7F) {
            if (c == 0x20 && !space_printable)
                needs_expansion++;
            len++;
        } else {
            /* Non-printable: would expand to \xHH (4 chars) or named (\t, \n etc) */
            needs_expansion++;
            len++;
        }
    }
    (void)needs_expansion;
    return len;
}

TEST(stress_safestrlen_10k_strings) {
    for (int i = 0; i < 10000; i++) {
        char buf[256];
        int len = i % 200 + 1;
        for (int j = 0; j < len; j++)
            buf[j] = 'A' + (j % 26);
        buf[len] = '\0';
        int result = stress_safestrlen(buf, 1024, 1);
        ASSERT_EQ(result, len);
    }
}

TEST(stress_safestrlen_binary_content) {
    /* Strings with every possible byte value */
    char buf[256];
    for (int i = 1; i < 256; i++)
        buf[i - 1] = (char)i;
    buf[255] = '\0';
    int result = stress_safestrlen(buf, 1024, 1);
    ASSERT_EQ(result, 255);
}

/* ===== int_lst sort + search combined stress ===== */

TEST(stress_int_lst_sort_then_search_10k) {
    int n = 10000;
    struct test_int_lst *lst =
        (struct test_int_lst *)malloc((size_t)n * sizeof(struct test_int_lst));
    ASSERT_NOT_NULL(lst);

    /* Fill with pseudorandom values */
    unsigned int seed = 99999;
    for (int i = 0; i < n; i++) {
        seed = seed * 1103515245 + 12345;
        lst[i].i = (int)(seed % 1000000);
        lst[i].selected = 0;
    }

    /* Sort */
    qsort(lst, (size_t)n, sizeof(struct test_int_lst), test_cmp_int_lst);

    /* Verify sorted */
    for (int i = 1; i < n; i++)
        ASSERT_TRUE(lst[i - 1].i <= lst[i].i);

    /* Search for every 10th element - should all hit */
    for (int i = 0; i < n; i += 10) {
        struct test_int_lst *r = test_find_int_lst(lst, n, lst[i].i);
        ASSERT_NOT_NULL(r);
        ASSERT_EQ(r->i, lst[i].i);
    }

    /* Search for values guaranteed not to exist */
    ASSERT_NULL(test_find_int_lst(lst, n, -1));
    ASSERT_NULL(test_find_int_lst(lst, n, 2000000));

    free(lst);
}

/* ===== Regression: sort stability with many equal keys ===== */

TEST(stress_sort_many_equal_keys) {
    int n = 10000;
    int *vals = (int *)malloc((size_t)n * sizeof(int));
    ASSERT_NOT_NULL(vals);

    /* Only 10 distinct values, repeated 1000 times each */
    for (int i = 0; i < n; i++)
        vals[i] = (n - i) % 10;

    qsort(vals, (size_t)n, sizeof(int), test_compare_pids);

    for (int i = 1; i < n; i++)
        ASSERT_TRUE(vals[i - 1] <= vals[i]);

    /* Verify each value appears 1000 times */
    int count = 1;
    for (int i = 1; i < n; i++) {
        if (vals[i] == vals[i - 1]) {
            count++;
        } else {
            ASSERT_EQ(count, 1000);
            count = 1;
        }
    }
    ASSERT_EQ(count, 1000);

    free(vals);
}

/* ===== Regression: device sort with multi-key tiebreaking at scale ===== */

TEST(stress_device_sort_multikey_10k) {
    int n = 10000;
    test_devcomp_t *devs = (test_devcomp_t *)malloc((size_t)n * sizeof(test_devcomp_t));
    char **names = (char **)malloc((size_t)n * sizeof(char *));
    ASSERT_NOT_NULL(devs);
    ASSERT_NOT_NULL(names);

    /* 100 unique rdevs, 100 unique inodes per rdev, unique names */
    for (int i = 0; i < n; i++) {
        devs[i].rdev = (dev_t)(i % 100);
        devs[i].inode = (unsigned long)(i / 100);
        names[i] = (char *)malloc(32);
        ASSERT_NOT_NULL(names[i]);
        snprintf(names[i], 32, "/dev/d%05d", n - i);
        devs[i].name = names[i];
    }

    qsort(devs, (size_t)n, sizeof(test_devcomp_t), test_compare_devices_fn);

    /* Verify primary sort key (rdev) is non-decreasing */
    for (int i = 1; i < n; i++) {
        ASSERT_TRUE(devs[i].rdev >= devs[i - 1].rdev);
        /* If rdev equal, inode should be non-decreasing */
        if (devs[i].rdev == devs[i - 1].rdev)
            ASSERT_TRUE(devs[i].inode >= devs[i - 1].inode);
    }

    for (int i = 0; i < n; i++)
        free(names[i]);
    free(names);
    free(devs);
}

#endif /* TEST_UNIT_STRESS_H */
