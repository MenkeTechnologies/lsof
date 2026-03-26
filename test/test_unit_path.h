/*
 * test_unit_path.h - path utility tests
 */

#ifndef TEST_UNIT_PATH_H
#define TEST_UNIT_PATH_H

static int test_is_absolute_path(const char *p) {
    return p && p[0] == '/';
}

static int test_path_depth(const char *p) {
    if (!p || !*p) return 0;
    int depth = 0;
    for (; *p; p++) if (*p == '/') depth++;
    return depth;
}

TEST(path_absolute_yes) {
    ASSERT_TRUE(test_is_absolute_path("/usr/bin"));
}

TEST(path_absolute_no) {
    ASSERT_FALSE(test_is_absolute_path("relative/path"));
}

TEST(path_absolute_empty) {
    ASSERT_FALSE(test_is_absolute_path(""));
}

TEST(path_absolute_null) {
    ASSERT_FALSE(test_is_absolute_path(NULL));
}

TEST(path_depth_root) {
    ASSERT_EQ(test_path_depth("/"), 1);
}

TEST(path_depth_deep) {
    ASSERT_EQ(test_path_depth("/a/b/c/d"), 4);
}

TEST(path_depth_empty) {
    ASSERT_EQ(test_path_depth(""), 0);
}

TEST(path_absolute_just_slash) {
    ASSERT_TRUE(test_is_absolute_path("/"));
}

TEST(path_absolute_double_slash) {
    ASSERT_TRUE(test_is_absolute_path("//weird"));
}

TEST(path_absolute_dot) {
    ASSERT_FALSE(test_is_absolute_path("."));
}

TEST(path_absolute_dotdot) {
    ASSERT_FALSE(test_is_absolute_path(".."));
}

TEST(path_depth_trailing_slash) {
    ASSERT_EQ(test_path_depth("/a/b/"), 3);
}

TEST(path_depth_double_slash) {
    ASSERT_EQ(test_path_depth("//a"), 2);
}

TEST(path_depth_null) {
    ASSERT_EQ(test_path_depth(NULL), 0);
}

TEST(path_depth_no_slash) {
    ASSERT_EQ(test_path_depth("file.txt"), 0);
}

#endif /* TEST_UNIT_PATH_H */
