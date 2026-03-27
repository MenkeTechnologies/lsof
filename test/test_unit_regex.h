/*
 * test_unit_regex.h - regex/command name matching tests
 *
 * Tests for command name pattern matching used in -c option
 */

#ifndef TEST_UNIT_REGEX_H
#define TEST_UNIT_REGEX_H

/* ===== Simple command name matching ===== */
static int test_cmd_match(const char *pattern, const char *cmd) {
    if (!pattern || !cmd)
        return 0;
    /* simple prefix match (like lsof -c behavior) */
    size_t plen = strlen(pattern);
    return (strncmp(cmd, pattern, plen) == 0);
}

/* ===== Glob-style matching ===== */
static int test_glob_match(const char *pattern, const char *str) {
    if (!pattern || !str)
        return 0;
    while (*pattern && *str) {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern)
                return 1;
            while (*str) {
                if (test_glob_match(pattern, str))
                    return 1;
                str++;
            }
            return 0;
        }
        if (*pattern == '?') {
            pattern++;
            str++;
            continue;
        }
        if (*pattern != *str)
            return 0;
        pattern++;
        str++;
    }
    while (*pattern == '*')
        pattern++;
    return (!*pattern && !*str);
}

/* ===== Regex-like character class matching ===== */
static int test_char_in_class(char c, const char *cls_start, const char *cls_end) {
    for (const char *p = cls_start; p < cls_end; p++) {
        if (p + 2 < cls_end && p[1] == '-') {
            if (c >= p[0] && c <= p[2])
                return 1;
            p += 2;
        } else {
            if (c == *p)
                return 1;
        }
    }
    return 0;
}

/* --- command name prefix matching tests --- */

TEST(cmd_match_exact) {
    ASSERT_TRUE(test_cmd_match("sshd", "sshd"));
}

TEST(cmd_match_prefix) {
    ASSERT_TRUE(test_cmd_match("ssh", "sshd"));
}

TEST(cmd_match_no_match) {
    ASSERT_FALSE(test_cmd_match("http", "sshd"));
}

TEST(cmd_match_empty_pattern) {
    /* empty pattern matches any prefix */
    ASSERT_TRUE(test_cmd_match("", "sshd"));
}

TEST(cmd_match_empty_cmd) {
    ASSERT_FALSE(test_cmd_match("ssh", ""));
}

TEST(cmd_match_both_empty) {
    ASSERT_TRUE(test_cmd_match("", ""));
}

TEST(cmd_match_null_pattern) {
    ASSERT_FALSE(test_cmd_match(NULL, "cmd"));
}

TEST(cmd_match_null_cmd) {
    ASSERT_FALSE(test_cmd_match("pattern", NULL));
}

TEST(cmd_match_longer_pattern) {
    ASSERT_FALSE(test_cmd_match("longcommandname", "long"));
}

TEST(cmd_match_case_sensitive) {
    ASSERT_FALSE(test_cmd_match("SSH", "sshd"));
}

TEST(cmd_match_with_numbers) {
    ASSERT_TRUE(test_cmd_match("python3", "python3.11"));
}

TEST(cmd_match_with_special_chars) {
    ASSERT_TRUE(test_cmd_match("my-", "my-daemon"));
}

/* --- glob matching tests --- */

TEST(glob_match_exact) {
    ASSERT_TRUE(test_glob_match("hello", "hello"));
}

TEST(glob_match_star_all) {
    ASSERT_TRUE(test_glob_match("*", "anything"));
}

TEST(glob_match_star_prefix) {
    ASSERT_TRUE(test_glob_match("hel*", "hello"));
}

TEST(glob_match_star_suffix) {
    ASSERT_TRUE(test_glob_match("*llo", "hello"));
}

TEST(glob_match_star_middle) {
    ASSERT_TRUE(test_glob_match("h*o", "hello"));
}

TEST(glob_match_question) {
    ASSERT_TRUE(test_glob_match("h?llo", "hello"));
}

TEST(glob_match_no_match) {
    ASSERT_FALSE(test_glob_match("world", "hello"));
}

TEST(glob_match_star_empty) {
    ASSERT_TRUE(test_glob_match("hello*", "hello"));
}

TEST(glob_match_multiple_stars) {
    ASSERT_TRUE(test_glob_match("*ll*", "hello"));
}

TEST(glob_match_question_no_match) {
    ASSERT_FALSE(test_glob_match("h?llo", "hllo"));
}

TEST(glob_match_empty_pattern) {
    ASSERT_TRUE(test_glob_match("", ""));
}

TEST(glob_match_star_only) {
    ASSERT_TRUE(test_glob_match("*", ""));
}

TEST(glob_match_double_star) {
    ASSERT_TRUE(test_glob_match("**", "anything"));
}

TEST(glob_match_complex) {
    ASSERT_TRUE(test_glob_match("*.log", "error.log"));
}

TEST(glob_match_complex_no_match) {
    ASSERT_FALSE(test_glob_match("*.log", "error.txt"));
}

/* --- character class tests --- */

TEST(charclass_single) {
    const char cls1[] = "abc";
    ASSERT_TRUE(test_char_in_class('a', cls1, cls1 + 3));
}

TEST(charclass_range) {
    const char cls[] = "a-z";
    ASSERT_TRUE(test_char_in_class('d', cls, cls + 3));
}

TEST(charclass_range_boundary_low) {
    const char cls[] = "a-z";
    ASSERT_TRUE(test_char_in_class('a', cls, cls + 3));
}

TEST(charclass_range_boundary_high) {
    const char cls[] = "a-z";
    ASSERT_TRUE(test_char_in_class('z', cls, cls + 3));
}

TEST(charclass_range_miss) {
    const char cls[] = "a-z";
    ASSERT_FALSE(test_char_in_class('A', cls, cls + 3));
}

TEST(charclass_digit_range) {
    const char cls[] = "0-9";
    ASSERT_TRUE(test_char_in_class('5', cls, cls + 3));
}

TEST(charclass_digit_range_miss) {
    const char cls[] = "0-9";
    ASSERT_FALSE(test_char_in_class('a', cls, cls + 3));
}

TEST(charclass_multiple_ranges) {
    const char cls[] = "a-zA-Z";
    ASSERT_TRUE(test_char_in_class('M', cls, cls + 6));
    ASSERT_TRUE(test_char_in_class('m', cls, cls + 6));
    ASSERT_FALSE(test_char_in_class('5', cls, cls + 6));
}

#endif /* TEST_UNIT_REGEX_H */
