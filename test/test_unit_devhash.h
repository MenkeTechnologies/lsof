/*
 * test_unit_devhash.h - device/inode hash and name cache hash tests
 */

#ifndef TEST_UNIT_DEVHASH_H
#define TEST_UNIT_DEVHASH_H

/* SFHASHDEVINO from isfn.c */
#define TEST_SFHASHDEVINO(maj, min, ino, mod) \
    ((int)(((int)((((int)(maj + 1)) * ((int)((min + 1)))) + ino) * 31415) & (mod - 1)))

/* ncache hash from rnmh.c */
static int test_ncache_hash(unsigned long inode, unsigned long addr, int mod) {
    return ((int)(((unsigned int)((unsigned int)(addr >> 2) + (unsigned int)(inode)) * 31415u) &
                  (unsigned int)(mod - 1)));
}

TEST(sfhash_range) {
    for (int maj = 0; maj < 16; maj++) {
        for (int min = 0; min < 16; min++) {
            int h = TEST_SFHASHDEVINO(maj, min, 1000, 4096);
            ASSERT_GE(h, 0);
            ASSERT_LT(h, 4096);
        }
    }
}

TEST(sfhash_deterministic) {
    int h1 = TEST_SFHASHDEVINO(8, 1, 12345, 4096);
    int h2 = TEST_SFHASHDEVINO(8, 1, 12345, 4096);
    ASSERT_EQ(h1, h2);
}

TEST(sfhash_different_inodes_differ) {
    int h1 = TEST_SFHASHDEVINO(8, 1, 100, 4096);
    int h2 = TEST_SFHASHDEVINO(8, 1, 200, 4096);
    ASSERT_NE(h1, h2);
}

TEST(sfhash_different_majors_differ) {
    int h1 = TEST_SFHASHDEVINO(1, 0, 100, 4096);
    int h2 = TEST_SFHASHDEVINO(2, 0, 100, 4096);
    ASSERT_NE(h1, h2);
}

TEST(sfhash_different_minors_differ) {
    int h1 = TEST_SFHASHDEVINO(8, 0, 100, 4096);
    int h2 = TEST_SFHASHDEVINO(8, 1, 100, 4096);
    ASSERT_NE(h1, h2);
}

TEST(sfhash_power_of_two_mod) {
    /* Hash should stay in range for different power-of-2 mods */
    int mods[] = {64, 128, 256, 512, 1024, 2048, 4096};
    for (int m = 0; m < 7; m++) {
        for (int i = 0; i < 100; i++) {
            int h = TEST_SFHASHDEVINO(i % 16, i % 8, i * 7, mods[m]);
            ASSERT_GE(h, 0);
            ASSERT_LT(h, mods[m]);
        }
    }
}

TEST(sfhash_distribution) {
    /* Check that 1000 entries spread across at least 25% of 4096 buckets */
    int buckets[4096];
    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < 1000; i++) {
        int h = TEST_SFHASHDEVINO(i / 100, i % 100, i * 7, 4096);
        buckets[h]++;
    }
    int used = 0;
    for (int i = 0; i < 4096; i++)
        if (buckets[i] > 0)
            used++;
    ASSERT_GT(used, 250);
}

TEST(sfhash_zero_inputs) {
    int h = TEST_SFHASHDEVINO(0, 0, 0, 4096);
    ASSERT_GE(h, 0);
    ASSERT_LT(h, 4096);
}

TEST(ncache_hash_range) {
    for (int i = 0; i < 1000; i++) {
        int h = test_ncache_hash((unsigned long)(i * 7), (unsigned long)(0x1000 + i * 64), 2048);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, 2048);
    }
}

TEST(ncache_hash_deterministic) {
    int h1 = test_ncache_hash(12345, 0xdeadbeef, 2048);
    int h2 = test_ncache_hash(12345, 0xdeadbeef, 2048);
    ASSERT_EQ(h1, h2);
}

TEST(ncache_hash_different_inodes_differ) {
    int h1 = test_ncache_hash(100, 0x1000, 2048);
    int h2 = test_ncache_hash(200, 0x1000, 2048);
    ASSERT_NE(h1, h2);
}

TEST(ncache_hash_different_addrs_differ) {
    int h1 = test_ncache_hash(100, 0x1000, 2048);
    int h2 = test_ncache_hash(100, 0x2000, 2048);
    ASSERT_NE(h1, h2);
}

TEST(ncache_hash_distribution) {
    int buckets[2048];
    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < 500; i++) {
        int h = test_ncache_hash((unsigned long)(i * 13), (unsigned long)(0x4000 + i * 128), 2048);
        buckets[h]++;
    }
    int used = 0;
    for (int i = 0; i < 2048; i++)
        if (buckets[i] > 0)
            used++;
    ASSERT_GT(used, 100);
}

#endif /* TEST_UNIT_DEVHASH_H */
