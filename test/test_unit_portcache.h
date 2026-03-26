/*
 * test_unit_portcache.h - port hash table and service cache tests
 */

#ifndef TEST_UNIT_PORTCACHE_H
#define TEST_UNIT_PORTCACHE_H

#define TEST_PORTHASHBUCKETS 128
#define TEST_HASHPORT(p)     (((((int)(p)) * 31415) >> 3) & (TEST_PORTHASHBUCKETS - 1))

struct test_porttab {
    int port;
    char *name;
    struct test_porttab *next;
};

/* Build a port hash table, return number of non-empty buckets */
static int build_port_table(struct test_porttab *entries, int count, struct test_porttab **buckets,
                            int *ports) {
    memset(buckets, 0, TEST_PORTHASHBUCKETS * sizeof(struct test_porttab *));
    for (int i = 0; i < count; i++) {
        entries[i].port = ports[i];
        entries[i].name = "svc";
        int h = TEST_HASHPORT(ports[i]);
        entries[i].next = buckets[h];
        buckets[h] = &entries[i];
    }
    int used = 0;
    for (int i = 0; i < TEST_PORTHASHBUCKETS; i++)
        if (buckets[i])
            used++;
    return used;
}

static const char *lookup_port(struct test_porttab **buckets, int port) {
    int h = TEST_HASHPORT(port);
    for (struct test_porttab *e = buckets[h]; e; e = e->next)
        if (e->port == port)
            return e->name;
    return NULL;
}

TEST(port_table_build) {
    struct test_porttab entries[5];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports[] = {22, 80, 443, 8080, 3306};
    int used = build_port_table(entries, 5, buckets, ports);
    ASSERT_GT(used, 0);
    ASSERT_GE(used, 3); /* at least 3 of 5 should be in different buckets */
}

TEST(port_table_lookup_hit) {
    struct test_porttab entries[5];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports[] = {22, 80, 443, 8080, 3306};
    build_port_table(entries, 5, buckets, ports);
    for (int i = 0; i < 5; i++) {
        ASSERT_NOT_NULL(lookup_port(buckets, ports[i]));
    }
}

TEST(port_table_lookup_miss) {
    struct test_porttab entries[5];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports[] = {22, 80, 443, 8080, 3306};
    build_port_table(entries, 5, buckets, ports);
    ASSERT_NULL(lookup_port(buckets, 9999));
    ASSERT_NULL(lookup_port(buckets, 12345));
    ASSERT_NULL(lookup_port(buckets, 0));
}

TEST(port_table_collision_chain) {
    /* Insert many ports and verify all are findable */
    struct test_porttab entries[20];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports[20];
    for (int i = 0; i < 20; i++)
        ports[i] = i * 100 + 1;
    build_port_table(entries, 20, buckets, ports);
    for (int i = 0; i < 20; i++) {
        ASSERT_NOT_NULL(lookup_port(buckets, ports[i]));
    }
}

TEST(port_table_overwrite) {
    /* Second build should replace first */
    struct test_porttab entries[3];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports1[] = {22, 80, 443};
    build_port_table(entries, 3, buckets, ports1);
    ASSERT_NOT_NULL(lookup_port(buckets, 22));
    int ports2[] = {8080, 9090, 3000};
    build_port_table(entries, 3, buckets, ports2);
    ASSERT_NULL(lookup_port(buckets, 22));
    ASSERT_NOT_NULL(lookup_port(buckets, 8080));
}

TEST(port_table_all_common_ports) {
    struct test_porttab entries[10];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports[] = {21, 22, 25, 53, 80, 110, 143, 443, 993, 995};
    build_port_table(entries, 10, buckets, ports);
    for (int i = 0; i < 10; i++) {
        ASSERT_NOT_NULL(lookup_port(buckets, ports[i]));
    }
}

TEST(port_table_max_port) {
    struct test_porttab entries[1];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports[] = {65535};
    build_port_table(entries, 1, buckets, ports);
    ASSERT_NOT_NULL(lookup_port(buckets, 65535));
    ASSERT_NULL(lookup_port(buckets, 65534));
}

TEST(port_table_zero_port) {
    struct test_porttab entries[1];
    struct test_porttab *buckets[TEST_PORTHASHBUCKETS];
    int ports[] = {0};
    build_port_table(entries, 1, buckets, ports);
    ASSERT_NOT_NULL(lookup_port(buckets, 0));
}

#endif /* TEST_UNIT_PORTCACHE_H */
