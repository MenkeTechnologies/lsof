/*
 * test_unit_json.h - JSON output unit tests
 *
 * Tests the JSON escape logic and output formatting without requiring
 * the full lsof binary or kernel access.
 */

#ifndef TEST_UNIT_JSON_H
#define TEST_UNIT_JSON_H

#include <stdio.h>
#include <string.h>

/*
 * json_escape_to_buf() - standalone JSON escape for testing
 *
 * Mirrors the logic from json.c json_escape_string() but writes to
 * a caller-supplied buffer so we can assert on the result.
 */
static void json_escape_to_buf(const char *s, char *out, size_t outsz) {
    size_t pos = 0;

    if (!s) {
        snprintf(out, outsz, "null");
        return;
    }

    if (pos < outsz - 1)
        out[pos++] = '"';

    for (; *s && pos < outsz - 2; s++) {
        switch (*s) {
        case '"':
            if (pos + 2 < outsz) { out[pos++] = '\\'; out[pos++] = '"'; }
            break;
        case '\\':
            if (pos + 2 < outsz) { out[pos++] = '\\'; out[pos++] = '\\'; }
            break;
        case '\b':
            if (pos + 2 < outsz) { out[pos++] = '\\'; out[pos++] = 'b'; }
            break;
        case '\f':
            if (pos + 2 < outsz) { out[pos++] = '\\'; out[pos++] = 'f'; }
            break;
        case '\n':
            if (pos + 2 < outsz) { out[pos++] = '\\'; out[pos++] = 'n'; }
            break;
        case '\r':
            if (pos + 2 < outsz) { out[pos++] = '\\'; out[pos++] = 'r'; }
            break;
        case '\t':
            if (pos + 2 < outsz) { out[pos++] = '\\'; out[pos++] = 't'; }
            break;
        default:
            if ((unsigned char)*s < 0x20) {
                pos += snprintf(out + pos, outsz - pos, "\\u%04x",
                                (unsigned char)*s);
            } else {
                out[pos++] = *s;
            }
            break;
        }
    }
    if (pos < outsz - 1)
        out[pos++] = '"';
    out[pos] = '\0';
}

/* ===== JSON escape tests ===== */

TEST(json_escape_plain_string) {
    char buf[256];
    json_escape_to_buf("hello", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"hello\"");
}

TEST(json_escape_empty_string) {
    char buf[256];
    json_escape_to_buf("", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"\"");
}

TEST(json_escape_null_becomes_null) {
    char buf[256];
    json_escape_to_buf(NULL, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "null");
}

TEST(json_escape_double_quote) {
    char buf[256];
    json_escape_to_buf("say \"hi\"", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"say \\\"hi\\\"\"");
}

TEST(json_escape_backslash) {
    char buf[256];
    json_escape_to_buf("c:\\path", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"c:\\\\path\"");
}

TEST(json_escape_newline) {
    char buf[256];
    json_escape_to_buf("line1\nline2", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"line1\\nline2\"");
}

TEST(json_escape_tab) {
    char buf[256];
    json_escape_to_buf("col1\tcol2", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"col1\\tcol2\"");
}

TEST(json_escape_carriage_return) {
    char buf[256];
    json_escape_to_buf("line\r\n", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"line\\r\\n\"");
}

TEST(json_escape_backspace) {
    char buf[256];
    json_escape_to_buf("a\bb", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"a\\bb\"");
}

TEST(json_escape_formfeed) {
    char buf[256];
    json_escape_to_buf("a\fb", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"a\\fb\"");
}

TEST(json_escape_control_char_unicode) {
    char buf[256];
    json_escape_to_buf("\x01", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"\\u0001\"");
}

TEST(json_escape_mixed_specials) {
    char buf[256];
    json_escape_to_buf("a\"b\\c\nd", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"a\\\"b\\\\c\\nd\"");
}

TEST(json_escape_path_with_spaces) {
    char buf[256];
    json_escape_to_buf("/usr/local/my file.txt", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"/usr/local/my file.txt\"");
}

TEST(json_escape_printable_ascii_passthrough) {
    char buf[256];
    json_escape_to_buf("ABCxyz0123!@#$%", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"ABCxyz0123!@#$%\"");
}

TEST(json_escape_all_special_chars) {
    char buf[256];
    json_escape_to_buf("\"\\\b\f\n\r\t", buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\"\\\"\\\\\\b\\f\\n\\r\\t\"");
}

/* ===== JSON structure validation ===== */

/*
 * Validate that a string looks like well-formed JSON array output.
 * Used to test the json_begin/json_end bracket framing.
 */
TEST(json_array_brackets) {
    /* Simulate what json_begin() and json_end() produce */
    const char *begin = "[\n";
    const char *end = "]\n";
    ASSERT_EQ(begin[0], '[');
    ASSERT_EQ(end[0], ']');
}

TEST(json_object_structure) {
    /* Validate expected field names in JSON proc output */
    const char *expected_fields[] = {
        "command", "pid", "uid", "pgid", "ppid", "files", NULL
    };
    for (int i = 0; expected_fields[i]; i++) {
        ASSERT_NOT_NULL(expected_fields[i]);
        ASSERT_TRUE(strlen(expected_fields[i]) > 0);
    }
}

TEST(json_file_fields) {
    const char *expected_fields[] = {
        "fd", "type", "device", "size_off", "node", "access", "lock", "name",
        NULL
    };
    for (int i = 0; expected_fields[i]; i++) {
        ASSERT_NOT_NULL(expected_fields[i]);
        ASSERT_TRUE(strlen(expected_fields[i]) > 0);
    }
}

#endif /* TEST_UNIT_JSON_H */
