/*
 * test_framework.h - minimal C test framework for lsof
 *
 * Usage:
 *   TEST(test_name) { ... ASSERT_*(...); ... }
 *   REGISTER_TEST(test_name);  // in TEST_LIST
 *   RUN_TESTS();
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int tf_tests_run = 0;
static int tf_tests_passed = 0;
static int tf_tests_failed = 0;
static int tf_current_failed = 0;

typedef void (*tf_test_fn)(void);

typedef struct {
    const char *name;
    tf_test_fn fn;
} tf_test_entry;

#define TEST(name) static void test_##name(void)

#define REGISTER_TEST(name) { #name, test_##name }

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #expr); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_FALSE(expr) do { \
    if ((expr)) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_FALSE(%s)\n", __FILE__, __LINE__, #expr); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_EQ(%s, %s) => %lld != %lld\n", \
            __FILE__, __LINE__, #a, #b, _a, _b); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a == _b) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_NE(%s, %s) => both %lld\n", \
            __FILE__, __LINE__, #a, #b, _a); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    const char *_a = (a), *_b = (b); \
    if (!_a || !_b || strcmp(_a, _b) != 0) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_STR_EQ(%s, %s) => \"%s\" != \"%s\"\n", \
            __FILE__, __LINE__, #a, #b, _a ? _a : "(null)", _b ? _b : "(null)"); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_NOT_NULL(p) do { \
    if ((p) == NULL) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_NOT_NULL(%s)\n", __FILE__, __LINE__, #p); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_NULL(p) do { \
    if ((p) != NULL) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_NULL(%s)\n", __FILE__, __LINE__, #p); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_GT(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (!(_a > _b)) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_GT(%s, %s) => %lld <= %lld\n", \
            __FILE__, __LINE__, #a, #b, _a, _b); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_GE(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (!(_a >= _b)) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_GE(%s, %s) => %lld < %lld\n", \
            __FILE__, __LINE__, #a, #b, _a, _b); \
        tf_current_failed = 1; \
    } \
} while(0)

#define ASSERT_LT(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (!(_a < _b)) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_LT(%s, %s) => %lld >= %lld\n", \
            __FILE__, __LINE__, #a, #b, _a, _b); \
        tf_current_failed = 1; \
    } \
} while(0)

#define RUN_TESTS_FROM(test_list) \
    int main(int argc, char **argv) { \
        (void)argc; (void)argv; \
        int count = (int)(sizeof(test_list) / sizeof(test_list[0])); \
        printf("Running %d tests...\n\n", count); \
        for (int i = 0; i < count; i++) { \
            tf_current_failed = 0; \
            tf_tests_run++; \
            test_list[i].fn(); \
            if (tf_current_failed) { \
                tf_tests_failed++; \
                printf("  FAIL: %s\n", test_list[i].name); \
            } else { \
                tf_tests_passed++; \
                printf("  PASS: %s\n", test_list[i].name); \
            } \
        } \
        printf("\n%d tests, %d passed, %d failed\n", \
            tf_tests_run, tf_tests_passed, tf_tests_failed); \
        return tf_tests_failed ? 1 : 0; \
    }

#endif /* TEST_FRAMEWORK_H */
