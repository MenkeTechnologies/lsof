/*
 * test_unit_environ.h - environment variable and process info tests
 *
 * Tests for environment handling, process info utilities
 */

#ifndef TEST_UNIT_ENVIRON_H
#define TEST_UNIT_ENVIRON_H

/* ===== Environment variable lookup ===== */
static const char *test_env_get(const char *name) {
    if (!name)
        return NULL;
    return getenv(name);
}

/* ===== PID validation ===== */
static int test_pid_valid(pid_t pid) {
    return (pid > 0);
}

/* ===== Process exists check ===== */
static int test_process_exists(pid_t pid) {
    if (pid <= 0)
        return 0;
    return (kill(pid, 0) == 0 || errno == EPERM);
}

/* ===== Simple comma-separated PID list parser ===== */
struct test_pid_list {
    pid_t *pids;
    int count;
    int capacity;
};

static struct test_pid_list test_parse_pid_list(const char *str) {
    struct test_pid_list list = {NULL, 0, 0};
    if (!str || !*str)
        return list;

    list.capacity = 16;
    list.pids = (pid_t *)malloc((size_t)list.capacity * sizeof(pid_t));
    if (!list.pids)
        return list;

    const char *p = str;
    while (*p) {
        while (*p == ' ' || *p == ',')
            p++;
        if (!*p)
            break;

        char *end;
        long val = strtol(p, &end, 10);
        if (end == p)
            break;

        if (list.count >= list.capacity) {
            list.capacity *= 2;
            pid_t *newbuf = (pid_t *)realloc(list.pids, (size_t)list.capacity * sizeof(pid_t));
            if (!newbuf)
                break;
            list.pids = newbuf;
        }
        list.pids[list.count++] = (pid_t)val;
        p = end;
    }
    return list;
}

static void test_free_pid_list(struct test_pid_list *list) {
    free(list->pids);
    list->pids = NULL;
    list->count = 0;
}

/* --- environment tests --- */

TEST(env_path_exists) {
    const char *path = test_env_get("PATH");
    ASSERT_NOT_NULL(path);
    ASSERT_GT((long long)strlen(path), 0);
}

TEST(env_home_exists) {
    const char *home = test_env_get("HOME");
    ASSERT_NOT_NULL(home);
    ASSERT_TRUE(home[0] == '/');
}

TEST(env_nonexistent) {
    const char *val = test_env_get("LSOF_NONEXISTENT_VAR_XYZ_12345");
    ASSERT_NULL(val);
}

TEST(env_null_name) {
    ASSERT_NULL(test_env_get(NULL));
}

TEST(env_user_exists) {
    const char *user = test_env_get("USER");
    if (user) {
        ASSERT_GT((long long)strlen(user), 0);
    }
    /* USER might not be set in all environments — that's ok */
}

/* --- PID validation tests --- */

TEST(pid_valid_positive) {
    ASSERT_TRUE(test_pid_valid(1));
    ASSERT_TRUE(test_pid_valid(12345));
}

TEST(pid_valid_zero) {
    ASSERT_FALSE(test_pid_valid(0));
}

TEST(pid_valid_negative) {
    ASSERT_FALSE(test_pid_valid(-1));
    ASSERT_FALSE(test_pid_valid(-12345));
}

TEST(pid_valid_current) {
    ASSERT_TRUE(test_pid_valid(getpid()));
}

/* --- process existence tests --- */

TEST(process_exists_self) {
    ASSERT_TRUE(test_process_exists(getpid()));
}

TEST(process_exists_parent) {
    ASSERT_TRUE(test_process_exists(getppid()));
}

TEST(process_exists_init) {
    ASSERT_TRUE(test_process_exists(1));
}

TEST(process_exists_invalid) {
    ASSERT_FALSE(test_process_exists(0));
    ASSERT_FALSE(test_process_exists(-1));
}

/* --- PID list parser tests --- */

TEST(pid_list_single) {
    struct test_pid_list list = test_parse_pid_list("1234");
    ASSERT_EQ(list.count, 1);
    ASSERT_EQ(list.pids[0], 1234);
    test_free_pid_list(&list);
}

TEST(pid_list_multiple) {
    struct test_pid_list list = test_parse_pid_list("1,2,3,4,5");
    ASSERT_EQ(list.count, 5);
    ASSERT_EQ(list.pids[0], 1);
    ASSERT_EQ(list.pids[4], 5);
    test_free_pid_list(&list);
}

TEST(pid_list_spaces) {
    struct test_pid_list list = test_parse_pid_list("1, 2, 3");
    ASSERT_EQ(list.count, 3);
    ASSERT_EQ(list.pids[0], 1);
    ASSERT_EQ(list.pids[1], 2);
    ASSERT_EQ(list.pids[2], 3);
    test_free_pid_list(&list);
}

TEST(pid_list_empty) {
    struct test_pid_list list = test_parse_pid_list("");
    ASSERT_EQ(list.count, 0);
    test_free_pid_list(&list);
}

TEST(pid_list_null) {
    struct test_pid_list list = test_parse_pid_list(NULL);
    ASSERT_EQ(list.count, 0);
}

TEST(pid_list_single_comma) {
    struct test_pid_list list = test_parse_pid_list(",");
    ASSERT_EQ(list.count, 0);
    test_free_pid_list(&list);
}

TEST(pid_list_trailing_comma) {
    struct test_pid_list list = test_parse_pid_list("1,2,");
    ASSERT_EQ(list.count, 2);
    test_free_pid_list(&list);
}

TEST(pid_list_large) {
    /* build a list of 100 PIDs */
    char buf[1024];
    int pos = 0;
    for (int i = 1; i <= 100; i++) {
        if (i > 1)
            buf[pos++] = ',';
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "%d", i);
    }
    struct test_pid_list list = test_parse_pid_list(buf);
    ASSERT_EQ(list.count, 100);
    ASSERT_EQ(list.pids[0], 1);
    ASSERT_EQ(list.pids[99], 100);
    test_free_pid_list(&list);
}

#endif /* TEST_UNIT_ENVIRON_H */
