/*
 * test_unit_hash.h - HASHPORT macro and hashbyname() tests
 */

#ifndef TEST_UNIT_HASH_H
#define TEST_UNIT_HASH_H

/* ===== HASHPORT macro test ===== */
#define PORTHASHBUCKETS 128
#define HASHPORT(p)     (((((int)(p)) * 31415) >> 3) & (PORTHASHBUCKETS - 1))

TEST(hashport_range) {
    for (int port_num = 0; port_num < 65536; port_num++) {
        int hash_val = HASHPORT(port_num);
        ASSERT_GE(hash_val, 0);
        ASSERT_LT(hash_val, PORTHASHBUCKETS);
    }
}

TEST(hashport_distribution) {
    int buckets[PORTHASHBUCKETS];
    memset(buckets, 0, sizeof(buckets));
    for (int port_num = 0; port_num < 1024; port_num++) {
        buckets[HASHPORT(port_num)]++;
    }
    int used = 0;
    for (int i = 0; i < PORTHASHBUCKETS; i++) {
        if (buckets[i] > 0)
            used++;
    }
    ASSERT_GT(used, PORTHASHBUCKETS / 2);
}

TEST(hashport_deterministic) {
    ASSERT_EQ(HASHPORT(80), HASHPORT(80));
    ASSERT_EQ(HASHPORT(443), HASHPORT(443));
    ASSERT_EQ(HASHPORT(0), HASHPORT(0));
}

TEST(hashport_adjacent_ports_differ) {
    int same = 0;
    for (int port_num = 1; port_num < 1024; port_num++) {
        if (HASHPORT(port_num) == HASHPORT(port_num - 1))
            same++;
    }
    ASSERT_LT(same, 100);
}

TEST(hashport_max_port) {
    int hash_val = HASHPORT(65535);
    ASSERT_GE(hash_val, 0);
    ASSERT_LT(hash_val, PORTHASHBUCKETS);
}

TEST(hashport_zero) {
    int hash_val = HASHPORT(0);
    ASSERT_EQ(hash_val, 0);
}

TEST(hashport_common_ports_in_range) {
    int common[] = {22, 53, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017};
    int n = sizeof(common) / sizeof(common[0]);
    for (int i = 0; i < n; i++) {
        int hash_val = HASHPORT(common[i]);
        ASSERT_GE(hash_val, 0);
        ASSERT_LT(hash_val, PORTHASHBUCKETS);
    }
}

/* ===== hash_by_name() reimplementation test ===== */
static int test_hash_by_name(char *name, int modulus) {
    int hash_accum, shift_pos;
    for (hash_accum = shift_pos = 0; *name; name++) {
        hash_accum ^= (int)*name << shift_pos;
        if (++shift_pos > 7)
            shift_pos = 0;
    }
    return (((int)(hash_accum * 31415)) & (modulus - 1));
}

TEST(hashbyname_range) {
    const char *names[] = {"tcp", "udp", "icmp", "http", "ssh", "/dev/null", "/var/log/syslog",
                           "a",   "",    "Z"};
    int mod = 128;
    for (int i = 0; i < 10; i++) {
        int hash_val = test_hash_by_name((char *)names[i], mod);
        ASSERT_GE(hash_val, 0);
        ASSERT_LT(hash_val, mod);
    }
}

TEST(hashbyname_deterministic) {
    ASSERT_EQ(test_hash_by_name("tcp", 128), test_hash_by_name("tcp", 128));
    ASSERT_EQ(test_hash_by_name("udp", 256), test_hash_by_name("udp", 256));
}

TEST(hashbyname_different_strings_differ) {
    int hash_val1 = test_hash_by_name("tcp", 128);
    int hash_val2 = test_hash_by_name("udp", 128);
    ASSERT_NE(hash_val1, hash_val2);
}

TEST(hashbyname_empty_string) {
    int hash_val = test_hash_by_name("", 128);
    ASSERT_EQ(hash_val, 0);
}

TEST(hashbyname_case_sensitive) {
    int hash_val1 = test_hash_by_name("TCP", 128);
    int hash_val2 = test_hash_by_name("tcp", 128);
    ASSERT_NE(hash_val1, hash_val2);
}

TEST(hashbyname_power_of_two_mod) {
    int mods[] = {16, 32, 64, 128, 256, 512, 1024};
    for (int m = 0; m < 7; m++) {
        int hash_val = test_hash_by_name("/dev/sda1", mods[m]);
        ASSERT_GE(hash_val, 0);
        ASSERT_LT(hash_val, mods[m]);
    }
}

TEST(hashbyname_distribution) {
    int buckets[64];
    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "/proc/%d/fd/%d", i * 7, i * 3);
        buckets[test_hash_by_name(name, 64)]++;
    }
    int used = 0;
    for (int i = 0; i < 64; i++)
        if (buckets[i] > 0)
            used++;
    ASSERT_GT(used, 20);
}

TEST(hashbyname_no_trivial_collision_set) {
    const char *protos[] = {"tcp", "udp",  "icmp", "sctp", "dccp",
                            "raw", "igmp", "esp",  "ah",   "gre"};
    int hashes[10];
    for (int i = 0; i < 10; i++)
        hashes[i] = test_hash_by_name((char *)protos[i], 128);
    int collisions = 0;
    for (int i = 0; i < 10; i++)
        for (int j = i + 1; j < 10; j++)
            if (hashes[i] == hashes[j])
                collisions++;
    ASSERT_LT(collisions, 3);
}

TEST(hashbyname_long_string) {
    char long_name[1024];
    memset(long_name, 'x', 1023);
    long_name[1023] = '\0';
    int hash_val = test_hash_by_name(long_name, 256);
    ASSERT_GE(hash_val, 0);
    ASSERT_LT(hash_val, 256);
}

TEST(hashbyname_single_char_spread) {
    int buckets[64];
    memset(buckets, 0, sizeof(buckets));
    for (char c = 'a'; c <= 'z'; c++) {
        char s[2] = {c, '\0'};
        buckets[test_hash_by_name(s, 64)]++;
    }
    int used = 0;
    for (int i = 0; i < 64; i++)
        if (buckets[i] > 0)
            used++;
    ASSERT_GT(used, 10);
}

#endif /* TEST_UNIT_HASH_H */
