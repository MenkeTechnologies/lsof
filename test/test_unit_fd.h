/*
 * test_unit_fd.h - file descriptor parsing, status, and string list tests
 *
 * Tests for ckfd_range, FD number parsing, ck_fd_status, enter_str_lst
 */

#ifndef TEST_UNIT_FD_H
#define TEST_UNIT_FD_H

/* ===== ckfd_range() reimplementation ===== */
static int test_ckfd_range(char *first, char *dash, char *last, int *lo, int *hi) {
    char *cursor;
    if (first >= dash || dash >= last)
        return 1;
    for (cursor = first, *lo = 0; *cursor && cursor < dash; cursor++) {
        if (!isdigit((unsigned char)*cursor))
            return 1;
        *lo = (*lo * 10) + (int)(*cursor - '0');
    }
    for (cursor = dash + 1, *hi = 0; *cursor && cursor < last; cursor++) {
        if (!isdigit((unsigned char)*cursor))
            return 1;
        *hi = (*hi * 10) + (int)(*cursor - '0');
    }
    if (*lo >= *hi)
        return 1;
    return 0;
}

TEST(ckfd_range_basic) {
    char s[] = "3-10";
    int lo = -1, hi = -1;
    ASSERT_EQ(test_ckfd_range(s, s + 1, s + 4, &lo, &hi), 0);
    ASSERT_EQ(lo, 3);
    ASSERT_EQ(hi, 10);
}

TEST(ckfd_range_single_digits) {
    char s[] = "0-9";
    int lo, hi;
    ASSERT_EQ(test_ckfd_range(s, s + 1, s + 3, &lo, &hi), 0);
    ASSERT_EQ(lo, 0);
    ASSERT_EQ(hi, 9);
}

TEST(ckfd_range_large) {
    char s[] = "100-999";
    int lo, hi;
    ASSERT_EQ(test_ckfd_range(s, s + 3, s + 7, &lo, &hi), 0);
    ASSERT_EQ(lo, 100);
    ASSERT_EQ(hi, 999);
}

TEST(ckfd_range_equal_rejected) {
    char s[] = "5-5";
    int lo, hi;
    ASSERT_EQ(test_ckfd_range(s, s + 1, s + 3, &lo, &hi), 1);
}

TEST(ckfd_range_reversed_rejected) {
    char s[] = "10-3";
    int lo, hi;
    ASSERT_EQ(test_ckfd_range(s, s + 2, s + 4, &lo, &hi), 1);
}

TEST(ckfd_range_non_digit_rejected) {
    char s[] = "3a-10";
    int lo, hi;
    ASSERT_EQ(test_ckfd_range(s, s + 2, s + 5, &lo, &hi), 1);
}

TEST(ckfd_range_non_digit_hi_rejected) {
    char s[] = "3-1x";
    int lo, hi;
    ASSERT_EQ(test_ckfd_range(s, s + 1, s + 4, &lo, &hi), 1);
}

TEST(ckfd_range_bad_pointers) {
    char s[] = "3-10";
    int lo, hi;
    ASSERT_EQ(test_ckfd_range(s, s, s + 4, &lo, &hi), 1);
}

/* ===== FD number parsing ===== */
static int test_parse_fd_num(const char *s, int *fd_out) {
    if (!s || !*s)
        return -1;
    char *end;
    long val = strtol(s, &end, 10);
    if (*end != '\0')
        return -1;
    if (val < 0 || val > 65535)
        return -1;
    *fd_out = (int)val;
    return 0;
}

TEST(parse_fd_zero) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("0", &fd), 0);
    ASSERT_EQ(fd, 0);
}

TEST(parse_fd_normal) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("42", &fd), 0);
    ASSERT_EQ(fd, 42);
}

TEST(parse_fd_max) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("65535", &fd), 0);
    ASSERT_EQ(fd, 65535);
}

TEST(parse_fd_overflow) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("65536", &fd), -1);
}

TEST(parse_fd_negative) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("-1", &fd), -1);
}

TEST(parse_fd_alpha) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("abc", &fd), -1);
}

TEST(parse_fd_mixed) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("3abc", &fd), -1);
}

TEST(parse_fd_empty) {
    int fd;
    ASSERT_EQ(test_parse_fd_num("", &fd), -1);
}

/* ===== ck_fd_status() reimplementation ===== */
struct test_fd_entry {
    char *name;
    int lo, hi;
    int exclude;
    struct test_fd_entry *next;
};

static int test_ck_fd_status(struct test_fd_entry *fdlist, const char *name, int num) {
    if (!fdlist)
        return 0;

    while (name && *name == ' ')
        name++;

    for (struct test_fd_entry *f = fdlist; f; f = f->next) {
        if (name && f->name) {
            if (strcmp(name, f->name) == 0)
                return f->exclude ? 1 : 2;
        } else if (!name && !f->name) {
            if (num >= f->lo && num <= f->hi)
                return f->exclude ? 1 : 2;
        }
    }
    return 0;
}

TEST(ck_fd_status_empty_list) {
    ASSERT_EQ(test_ck_fd_status(NULL, "cwd", 0), 0);
}

TEST(ck_fd_status_name_included) {
    struct test_fd_entry e = {"cwd", 0, 0, 0, NULL};
    ASSERT_EQ(test_ck_fd_status(&e, "cwd", 0), 2);
}

TEST(ck_fd_status_name_excluded) {
    struct test_fd_entry e = {"mem", 0, 0, 1, NULL};
    ASSERT_EQ(test_ck_fd_status(&e, "mem", 0), 1);
}

TEST(ck_fd_status_num_in_range) {
    struct test_fd_entry e = {NULL, 3, 10, 0, NULL};
    ASSERT_EQ(test_ck_fd_status(&e, NULL, 5), 2);
}

TEST(ck_fd_status_num_out_of_range) {
    struct test_fd_entry e = {NULL, 3, 10, 0, NULL};
    ASSERT_EQ(test_ck_fd_status(&e, NULL, 11), 0);
}

TEST(ck_fd_status_name_not_found) {
    struct test_fd_entry e = {"cwd", 0, 0, 0, NULL};
    ASSERT_EQ(test_ck_fd_status(&e, "txt", 0), 0);
}

TEST(ck_fd_status_leading_spaces) {
    struct test_fd_entry e = {"cwd", 0, 0, 0, NULL};
    ASSERT_EQ(test_ck_fd_status(&e, "  cwd", 0), 2);
}

TEST(ck_fd_status_chain) {
    struct test_fd_entry e2 = {"txt", 0, 0, 0, NULL};
    struct test_fd_entry e1 = {"cwd", 0, 0, 1, &e2};
    ASSERT_EQ(test_ck_fd_status(&e1, "cwd", 0), 1);
    ASSERT_EQ(test_ck_fd_status(&e1, "txt", 0), 2);
}

/* ===== enter_str_lst() reimplementation ===== */
struct test_str_lst {
    char *str;
    int exclude;
    struct test_str_lst *next;
};

static int test_enter_str_lst(const char *str_val, struct test_str_lst **list_ptr, int *incl,
                              int *excl) {
    if (!str_val || !*str_val)
        return 1;

    struct test_str_lst *entry = (struct test_str_lst *)malloc(sizeof(*entry));
    if (!entry)
        return 1;

    if (*str_val == '^') {
        entry->exclude = 1;
        str_val++;
        if (excl)
            (*excl)++;
    } else {
        entry->exclude = 0;
        if (incl)
            (*incl)++;
    }

    entry->str = (char *)malloc(strlen(str_val) + 1);
    if (!entry->str) {
        free(entry);
        return 1;
    }
    strcpy(entry->str, str_val);
    entry->next = *list_ptr;
    *list_ptr = entry;
    return 0;
}

static void test_free_str_lst(struct test_str_lst *list) {
    while (list) {
        struct test_str_lst *next = list->next;
        free(list->str);
        free(list);
        list = next;
    }
}

TEST(str_lst_include) {
    struct test_str_lst *list = NULL;
    int incl = 0, excl = 0;
    ASSERT_EQ(test_enter_str_lst("hello", &list, &incl, &excl), 0);
    ASSERT_NOT_NULL(list);
    ASSERT_STR_EQ(list->str, "hello");
    ASSERT_EQ(list->exclude, 0);
    ASSERT_EQ(incl, 1);
    ASSERT_EQ(excl, 0);
    test_free_str_lst(list);
}

TEST(str_lst_exclude) {
    struct test_str_lst *list = NULL;
    int incl = 0, excl = 0;
    ASSERT_EQ(test_enter_str_lst("^badcmd", &list, &incl, &excl), 0);
    ASSERT_NOT_NULL(list);
    ASSERT_STR_EQ(list->str, "badcmd");
    ASSERT_EQ(list->exclude, 1);
    ASSERT_EQ(incl, 0);
    ASSERT_EQ(excl, 1);
    test_free_str_lst(list);
}

TEST(str_lst_multiple) {
    struct test_str_lst *list = NULL;
    int incl = 0, excl = 0;
    ASSERT_EQ(test_enter_str_lst("alpha", &list, &incl, &excl), 0);
    ASSERT_EQ(test_enter_str_lst("^beta", &list, &incl, &excl), 0);
    ASSERT_EQ(test_enter_str_lst("gamma", &list, &incl, &excl), 0);
    ASSERT_EQ(incl, 2);
    ASSERT_EQ(excl, 1);
    ASSERT_STR_EQ(list->str, "gamma");
    ASSERT_STR_EQ(list->next->str, "beta");
    ASSERT_STR_EQ(list->next->next->str, "alpha");
    test_free_str_lst(list);
}

TEST(str_lst_empty_rejected) {
    struct test_str_lst *list = NULL;
    int incl = 0, excl = 0;
    ASSERT_EQ(test_enter_str_lst("", &list, &incl, &excl), 1);
    ASSERT_NULL(list);
}

TEST(str_lst_null_rejected) {
    struct test_str_lst *list = NULL;
    int incl = 0, excl = 0;
    ASSERT_EQ(test_enter_str_lst(NULL, &list, &incl, &excl), 1);
    ASSERT_NULL(list);
}

TEST(str_lst_caret_only) {
    struct test_str_lst *list = NULL;
    int incl = 0, excl = 0;
    ASSERT_EQ(test_enter_str_lst("^", &list, &incl, &excl), 0);
    ASSERT_NOT_NULL(list);
    ASSERT_STR_EQ(list->str, "");
    ASSERT_EQ(list->exclude, 1);
    test_free_str_lst(list);
}

#endif /* TEST_UNIT_FD_H */
