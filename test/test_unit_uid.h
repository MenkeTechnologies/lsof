/*
 * test_unit_uid.h - UID/user lookup and caching tests
 *
 * Tests for UID to username conversion, cache behavior, and edge cases
 */

#ifndef TEST_UNIT_UID_H
#define TEST_UNIT_UID_H

#include <pwd.h>

/* ===== UID-to-name lookup reimplementation ===== */
struct test_uid_cache_entry {
    uid_t uid;
    char *name;
    struct test_uid_cache_entry *next;
};

#define TEST_UID_CACHE_SIZE 64
static struct test_uid_cache_entry *test_uid_cache[TEST_UID_CACHE_SIZE];
static int test_uid_cache_initialized = 0;

static void test_uid_cache_init(void) {
    if (!test_uid_cache_initialized) {
        memset(test_uid_cache, 0, sizeof(test_uid_cache));
        test_uid_cache_initialized = 1;
    }
}

static void test_uid_cache_cleanup(void) {
    for (int i = 0; i < TEST_UID_CACHE_SIZE; i++) {
        struct test_uid_cache_entry *e = test_uid_cache[i];
        while (e) {
            struct test_uid_cache_entry *next = e->next;
            free(e->name);
            free(e);
            e = next;
        }
        test_uid_cache[i] = NULL;
    }
    test_uid_cache_initialized = 0;
}

static int test_uid_hash(uid_t uid) {
    return (int)(uid % TEST_UID_CACHE_SIZE);
}

static char *test_lookup_uid(uid_t uid) {
    test_uid_cache_init();
    int h = test_uid_hash(uid);
    struct test_uid_cache_entry *e = test_uid_cache[h];
    while (e) {
        if (e->uid == uid)
            return e->name;
        e = e->next;
    }

    /* not cached — look up */
    struct passwd *pw = getpwuid(uid);
    struct test_uid_cache_entry *ne = (struct test_uid_cache_entry *)malloc(sizeof(*ne));
    ne->uid = uid;
    if (pw && pw->pw_name) {
        ne->name = (char *)malloc(strlen(pw->pw_name) + 1);
        strcpy(ne->name, pw->pw_name);
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", (unsigned)uid);
        ne->name = (char *)malloc(strlen(buf) + 1);
        strcpy(ne->name, buf);
    }
    ne->next = test_uid_cache[h];
    test_uid_cache[h] = ne;
    return ne->name;
}

TEST(uid_lookup_root) {
    test_uid_cache_cleanup();
    char *name = test_lookup_uid(0);
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, "root");
    test_uid_cache_cleanup();
}

TEST(uid_lookup_current_user) {
    test_uid_cache_cleanup();
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    ASSERT_NOT_NULL(pw);
    char *name = test_lookup_uid(uid);
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, pw->pw_name);
    test_uid_cache_cleanup();
}

TEST(uid_lookup_nonexistent) {
    test_uid_cache_cleanup();
    /* use a very high UID unlikely to exist */
    uid_t uid = 99999;
    char *name = test_lookup_uid(uid);
    ASSERT_NOT_NULL(name);
    /* should fall back to numeric string */
    char expected[32];
    struct passwd *pw = getpwuid(uid);
    if (pw)
        snprintf(expected, sizeof(expected), "%s", pw->pw_name);
    else
        snprintf(expected, sizeof(expected), "%u", (unsigned)uid);
    ASSERT_STR_EQ(name, expected);
    test_uid_cache_cleanup();
}

TEST(uid_cache_hit) {
    test_uid_cache_cleanup();
    uid_t uid = getuid();
    /* first lookup populates cache */
    char *name1 = test_lookup_uid(uid);
    /* second lookup should return same pointer (cached) */
    char *name2 = test_lookup_uid(uid);
    ASSERT_TRUE(name1 == name2);
    test_uid_cache_cleanup();
}

TEST(uid_hash_range) {
    for (uid_t u = 0; u < 1000; u++) {
        int h = test_uid_hash(u);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, TEST_UID_CACHE_SIZE);
    }
}

TEST(uid_hash_deterministic) {
    for (uid_t u = 0; u < 100; u++) {
        ASSERT_EQ(test_uid_hash(u), test_uid_hash(u));
    }
}

TEST(uid_hash_distribution) {
    int buckets[TEST_UID_CACHE_SIZE];
    memset(buckets, 0, sizeof(buckets));
    for (uid_t u = 0; u < 1000; u++)
        buckets[test_uid_hash(u)]++;

    int used = 0;
    for (int i = 0; i < TEST_UID_CACHE_SIZE; i++)
        if (buckets[i] > 0)
            used++;
    /* with 1000 UIDs across 64 buckets, should fill most */
    ASSERT_GT(used, TEST_UID_CACHE_SIZE / 2);
}

TEST(uid_cache_multiple_users) {
    test_uid_cache_cleanup();
    /* lookup root and current user, both should be cached */
    char *root_name = test_lookup_uid(0);
    uid_t myuid = getuid();
    char *my_name = test_lookup_uid(myuid);
    ASSERT_NOT_NULL(root_name);
    ASSERT_NOT_NULL(my_name);
    if (myuid != 0)
        ASSERT_TRUE(strcmp(root_name, my_name) != 0);
    test_uid_cache_cleanup();
}

TEST(uid_cache_collision_chain) {
    test_uid_cache_cleanup();
    /* UIDs that hash to the same bucket */
    uid_t u1 = 0;
    uid_t u2 = TEST_UID_CACHE_SIZE; /* same hash bucket as 0 */
    char *n1 = test_lookup_uid(u1);
    char *n2 = test_lookup_uid(u2);
    ASSERT_NOT_NULL(n1);
    ASSERT_NOT_NULL(n2);
    /* both should be retrievable */
    ASSERT_STR_EQ(test_lookup_uid(u1), n1);
    ASSERT_STR_EQ(test_lookup_uid(u2), n2);
    test_uid_cache_cleanup();
}

#endif /* TEST_UNIT_UID_H */
