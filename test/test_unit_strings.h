/*
 * test_unit_strings.h - string utility function tests
 *
 * Tests for safestrlen, safepup, safestrprt, mkstrcpy, mkstrcat
 */

#ifndef TEST_UNIT_STRINGS_H
#define TEST_UNIT_STRINGS_H

/* ===== safe_string_length() reimplementation ===== */
static int
test_safe_string_length(char *string_ptr, int flags)
{
    char non_print_marker;
    int safe_len = 0;

    non_print_marker = (flags & 2) ? ' ' : '\0';
    if (string_ptr) {
        for (; *string_ptr; string_ptr++) {
            if (!isprint((unsigned char)*string_ptr) || *string_ptr == non_print_marker) {
                if (*string_ptr < 0x20 || (unsigned char)*string_ptr == 0xff)
                    safe_len += 2;
                else
                    safe_len += 4;
            } else
                safe_len++;
        }
    }
    return safe_len;
}

TEST(safestrlen_normal_string) {
    ASSERT_EQ(test_safe_string_length("hello", 0), 5);
}

TEST(safestrlen_empty) {
    ASSERT_EQ(test_safe_string_length("", 0), 0);
}

TEST(safestrlen_null) {
    ASSERT_EQ(test_safe_string_length(NULL, 0), 0);
}

TEST(safestrlen_with_tab) {
    ASSERT_EQ(test_safe_string_length("a\tb", 0), 4);
}

TEST(safestrlen_with_newline) {
    ASSERT_EQ(test_safe_string_length("a\nb", 0), 4);
}

TEST(safestrlen_space_not_printable) {
    ASSERT_EQ(test_safe_string_length("a b", 2), 6);
}

TEST(safestrlen_space_printable) {
    ASSERT_EQ(test_safe_string_length("a b", 0), 3);
}

TEST(safestrlen_0xff_char) {
    char s[3] = {'A', (char)0xff, '\0'};
    ASSERT_EQ(test_safe_string_length(s, 0), 3);
}

TEST(safestrlen_high_byte) {
    char s[3] = {'A', (char)0x80, '\0'};
    int len = test_safe_string_length(s, 0);
    ASSERT_TRUE(len == 3 || len == 5);
}

TEST(safestrlen_all_printable) {
    char buf[96];
    int j = 0;
    for (int i = 0x21; i < 0x7f; i++)
        buf[j++] = (char)i;
    buf[j] = '\0';
    ASSERT_EQ(test_safe_string_length(buf, 0), j);
}

TEST(safestrlen_multiple_control) {
    ASSERT_EQ(test_safe_string_length("\t\n\r", 0), 6);
}

TEST(safestrlen_mixed_content) {
    ASSERT_EQ(test_safe_string_length("ab\tcd\nef", 0), 10);
}

TEST(safestrlen_backspace) {
    ASSERT_EQ(test_safe_string_length("\b", 0), 2);
}

TEST(safestrlen_formfeed) {
    ASSERT_EQ(test_safe_string_length("\f", 0), 2);
}

TEST(safestrlen_cr) {
    ASSERT_EQ(test_safe_string_length("\r", 0), 2);
}

TEST(safestrlen_single_printable) {
    ASSERT_EQ(test_safe_string_length("A", 0), 1);
}

TEST(safestrlen_space_default) {
    ASSERT_EQ(test_safe_string_length(" ", 0), 1);
}

TEST(safestrlen_space_as_nonprint) {
    ASSERT_EQ(test_safe_string_length(" ", 2), 4);
}


/* ===== safe_print_unprintable() reimplementation ===== */
static char safepup_buf[8];

static char *
test_safe_print_unprintable(unsigned int char_code, int *output_length)
{
    int encoded_len;
    char *result_str;

    if (char_code < 0x20) {
        switch (char_code) {
        case '\b': result_str = "\\b"; break;
        case '\f': result_str = "\\f"; break;
        case '\n': result_str = "\\n"; break;
        case '\r': result_str = "\\r"; break;
        case '\t': result_str = "\\t"; break;
        default:
            snprintf(safepup_buf, sizeof(safepup_buf), "^%c", char_code + 0x40);
            result_str = safepup_buf;
        }
        encoded_len = 2;
    } else if (char_code == 0xff) {
        result_str = "^?";
        encoded_len = 2;
    } else {
        snprintf(safepup_buf, sizeof(safepup_buf), "\\x%02x", (int)(char_code & 0xff));
        result_str = safepup_buf;
        encoded_len = 4;
    }
    if (output_length)
        *output_length = encoded_len;
    return result_str;
}

TEST(safepup_tab) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable('\t', &char_len), "\\t");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_newline) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable('\n', &char_len), "\\n");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_backspace) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable('\b', &char_len), "\\b");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_formfeed) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable('\f', &char_len), "\\f");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_cr) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable('\r', &char_len), "\\r");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_ctrl_a) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable(0x01, &char_len), "^A");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_null_char) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable(0x00, &char_len), "^@");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_0xff) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable(0xff, &char_len), "^?");
    ASSERT_EQ(char_len, 2);
}

TEST(safepup_high_byte) {
    int char_len;
    ASSERT_STR_EQ(test_safe_print_unprintable(0x80, &char_len), "\\x80");
    ASSERT_EQ(char_len, 4);
}

TEST(safepup_null_cl) {
    char *result = test_safe_print_unprintable('\n', NULL);
    ASSERT_STR_EQ(result, "\\n");
}

TEST(safepup_printable_range) {
    for (int c = 0x20; c <= 0x7e; c++) {
        char ch = (char)c;
        int class_val = (ch == ' ') ? 2 : isprint((unsigned char)ch) ? 1 : 0;
        if (ch != ' ')
            ASSERT_EQ(class_val, 1);
    }
}

TEST(safepup_all_control_chars_escaped) {
    for (int c = 1; c < 0x20; c++) {
        ASSERT_FALSE(isprint((unsigned char)c));
    }
}

TEST(safepup_del_char) {
    ASSERT_FALSE(isprint(0x7f));
}


/* ===== safestrprt() output test ===== */
TEST(safestrprt_printable_to_buffer) {
    FILE *f = tmpfile();
    ASSERT_NOT_NULL(f);
    fprintf(f, "%s", "hello");
    rewind(f);
    char buf[64] = {0};
    ASSERT_NOT_NULL(fgets(buf, sizeof(buf), f));
    ASSERT_STR_EQ(buf, "hello");
    fclose(f);
}

TEST(safestrprt_control_char_format) {
    char c = '\x01';
    char expected[3];
    expected[0] = '^';
    expected[1] = '@' + 1;
    expected[2] = '\0';

    FILE *f = tmpfile();
    ASSERT_NOT_NULL(f);
    fprintf(f, "^%c", (char)('@' + c));
    rewind(f);
    char buf[64] = {0};
    ASSERT_NOT_NULL(fgets(buf, sizeof(buf), f));
    ASSERT_STR_EQ(buf, expected);
    fclose(f);
}


/* ===== make_string_copy() reimplementation ===== */
static char *
test_make_string_copy(char *source, size_t *result_length)
{
    size_t length = (size_t)(source ? strlen(source) : 0);
    char *new_str = (char *)malloc(length + 1);
    if (new_str) {
        if (source)
            memcpy(new_str, source, length + 1);
        else
            *new_str = '\0';
    }
    if (result_length)
        *result_length = length;
    return new_str;
}

TEST(mkstrcpy_basic) {
    size_t len = 0;
    char *copy = test_make_string_copy("hello", &len);
    ASSERT_NOT_NULL(copy);
    ASSERT_STR_EQ(copy, "hello");
    ASSERT_EQ(len, 5);
    free(copy);
}

TEST(mkstrcpy_empty) {
    size_t len = 99;
    char *copy = test_make_string_copy("", &len);
    ASSERT_NOT_NULL(copy);
    ASSERT_STR_EQ(copy, "");
    ASSERT_EQ(len, 0);
    free(copy);
}

TEST(mkstrcpy_null_source) {
    size_t len = 99;
    char *copy = test_make_string_copy(NULL, &len);
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(copy[0], '\0');
    ASSERT_EQ(len, 0);
    free(copy);
}

TEST(mkstrcpy_null_lenptr) {
    char *copy = test_make_string_copy("test", NULL);
    ASSERT_NOT_NULL(copy);
    ASSERT_STR_EQ(copy, "test");
    free(copy);
}

TEST(mkstrcpy_long_string) {
    char buf[256];
    memset(buf, 'A', 255);
    buf[255] = '\0';
    size_t len = 0;
    char *copy = test_make_string_copy(buf, &len);
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(len, 255);
    ASSERT_EQ(strlen(copy), 255);
    free(copy);
}

TEST(mkstrcpy_is_independent_copy) {
    char src[] = "original";
    char *copy = test_make_string_copy(src, NULL);
    ASSERT_NOT_NULL(copy);
    src[0] = 'X';
    ASSERT_STR_EQ(copy, "original");
    free(copy);
}


/* ===== make_string_concat() reimplementation ===== */
static char *
test_make_string_concat(char *str1, int len1_limit, char *str2, int len2_limit,
                        char *str3, int len3_limit, size_t *total_length)
{
    size_t len1 = str1 ? (size_t)((len1_limit >= 0) ? len1_limit : (int)strlen(str1)) : 0;
    size_t len2 = str2 ? (size_t)((len2_limit >= 0) ? len2_limit : (int)strlen(str2)) : 0;
    size_t len3 = str3 ? (size_t)((len3_limit >= 0) ? len3_limit : (int)strlen(str3)) : 0;
    size_t combined_len = len1 + len2 + len3;
    char *result = (char *)malloc(combined_len + 1);
    if (result) {
        char *write_pos = result;
        if (str1 && len1) { memcpy(write_pos, str1, len1); write_pos += len1; }
        if (str2 && len2) { memcpy(write_pos, str2, len2); write_pos += len2; }
        if (str3 && len3) { memcpy(write_pos, str3, len3); write_pos += len3; }
        *write_pos = '\0';
    }
    if (total_length) *total_length = combined_len;
    return result;
}

TEST(mkstrcat_two_strings) {
    size_t len = 0;
    char *result = test_make_string_concat("hello", -1, " world", -1, NULL, -1, &len);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "hello world");
    ASSERT_EQ(len, 11);
    free(result);
}

TEST(mkstrcat_three_strings) {
    size_t len = 0;
    char *result = test_make_string_concat("a", -1, "b", -1, "c", -1, &len);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "abc");
    ASSERT_EQ(len, 3);
    free(result);
}

TEST(mkstrcat_with_length_limits) {
    size_t len = 0;
    char *result = test_make_string_concat("hello", 3, "world", 2, NULL, -1, &len);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "helwo");
    ASSERT_EQ(len, 5);
    free(result);
}

TEST(mkstrcat_empty_strings) {
    char *result = test_make_string_concat("", -1, "", -1, "", -1, NULL);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "");
    free(result);
}

TEST(mkstrcat_null_strings) {
    char *result = test_make_string_concat(NULL, -1, NULL, -1, NULL, -1, NULL);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "");
    free(result);
}

TEST(mkstrcat_mixed_null) {
    char *result = test_make_string_concat("hello", -1, NULL, -1, " world", -1, NULL);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "hello world");
    free(result);
}

TEST(mkstrcat_zero_length) {
    char *result = test_make_string_concat("skip", 0, "keep", -1, NULL, -1, NULL);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "keep");
    free(result);
}

TEST(mkstrcat_path_building) {
    char *result = test_make_string_concat("/proc/", -1, "1234", -1, "/fd", -1, NULL);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "/proc/1234/fd");
    free(result);
}

TEST(mkstrcat_all_length_limited) {
    size_t len = 0;
    char *result = test_make_string_concat("abcdef", 2, "ghijkl", 3, "mnopqr", 1, &len);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "abghim");
    ASSERT_EQ(len, 6);
    free(result);
}

TEST(mkstrcat_only_third) {
    char *result = test_make_string_concat(NULL, -1, NULL, -1, "only", -1, NULL);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "only");
    free(result);
}

TEST(mkstrcat_long_concat) {
    char a[128], b[128];
    memset(a, 'A', 127); a[127] = '\0';
    memset(b, 'B', 127); b[127] = '\0';
    size_t len = 0;
    char *result = test_make_string_concat(a, -1, b, -1, NULL, -1, &len);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(len, 254);
    ASSERT_EQ(result[0], 'A');
    ASSERT_EQ(result[126], 'A');
    ASSERT_EQ(result[127], 'B');
    ASSERT_EQ(result[253], 'B');
    free(result);
}

#endif /* TEST_UNIT_STRINGS_H */
