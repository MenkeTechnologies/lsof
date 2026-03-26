/*
 * test_unit_compare.h - comparator, search, and device dedup tests
 *
 * Tests for compdev, comppid, cmp_int_lst, find_int_lst,
 * PID boundaries, device decomposition, rmdupdev
 */

#ifndef TEST_UNIT_COMPARE_H
#define TEST_UNIT_COMPARE_H

/* ===== compare_devices() reimplementation (pointer-to-pointer variant) ===== */
typedef unsigned long test_INODETYPE;

struct test_l_dev {
    dev_t rdev;
    test_INODETYPE inode;
    char *name;
    int v;
};

static int test_compare_devices(const void *first, const void *second) {
    struct test_l_dev **dev_ptr1 = (struct test_l_dev **)first;
    struct test_l_dev **dev_ptr2 = (struct test_l_dev **)second;

    if ((dev_t)((*dev_ptr1)->rdev) < (dev_t)((*dev_ptr2)->rdev))
        return -1;
    if ((dev_t)((*dev_ptr1)->rdev) > (dev_t)((*dev_ptr2)->rdev))
        return 1;
    if ((test_INODETYPE)((*dev_ptr1)->inode) < (test_INODETYPE)((*dev_ptr2)->inode))
        return -1;
    if ((test_INODETYPE)((*dev_ptr1)->inode) > (test_INODETYPE)((*dev_ptr2)->inode))
        return 1;
    return strcmp((*dev_ptr1)->name, (*dev_ptr2)->name);
}

TEST(compdev_equal) {
    struct test_l_dev a = {1, 100, "dev0", 0};
    struct test_l_dev b = {1, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compare_devices(&pa, &pb), 0);
}

TEST(compdev_rdev_less) {
    struct test_l_dev a = {1, 100, "dev0", 0};
    struct test_l_dev b = {2, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compare_devices(&pa, &pb), -1);
}

TEST(compdev_rdev_greater) {
    struct test_l_dev a = {3, 100, "dev0", 0};
    struct test_l_dev b = {2, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compare_devices(&pa, &pb), 1);
}

TEST(compdev_inode_less) {
    struct test_l_dev a = {1, 50, "dev0", 0};
    struct test_l_dev b = {1, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compare_devices(&pa, &pb), -1);
}

TEST(compdev_inode_greater) {
    struct test_l_dev a = {1, 200, "dev0", 0};
    struct test_l_dev b = {1, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compare_devices(&pa, &pb), 1);
}

TEST(compdev_name_compare) {
    struct test_l_dev a = {1, 100, "alpha", 0};
    struct test_l_dev b = {1, 100, "beta", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_LT(test_compare_devices(&pa, &pb), 0);
}

TEST(compdev_sort_array) {
    struct test_l_dev devs[] = {
        {3, 10, "c", 0}, {1, 10, "a", 0}, {2, 10, "b", 0}, {1, 5, "a", 0}, {1, 10, "z", 0},
    };
    struct test_l_dev *ptrs[5];
    for (int i = 0; i < 5; i++)
        ptrs[i] = &devs[i];

    qsort(ptrs, 5, sizeof(struct test_l_dev *), test_compare_devices);

    ASSERT_EQ(ptrs[0]->rdev, 1);
    ASSERT_EQ(ptrs[0]->inode, 5);
    ASSERT_EQ(ptrs[1]->rdev, 1);
    ASSERT_EQ(ptrs[1]->inode, 10);
    ASSERT_STR_EQ(ptrs[1]->name, "a");
    ASSERT_EQ(ptrs[2]->rdev, 1);
    ASSERT_STR_EQ(ptrs[2]->name, "z");
    ASSERT_EQ(ptrs[3]->rdev, 2);
    ASSERT_EQ(ptrs[4]->rdev, 3);
}

/* ===== compare_devices() flat variant (for extended tests) ===== */
typedef struct test_devcomp {
    dev_t rdev;
    unsigned long inode;
    char *name;
} test_devcomp_t;

static int test_compare_devices_fn(const void *a, const void *b) {
    const test_devcomp_t *da = (const test_devcomp_t *)a;
    const test_devcomp_t *db = (const test_devcomp_t *)b;
    if (da->rdev < db->rdev)
        return -1;
    if (da->rdev > db->rdev)
        return 1;
    if (da->inode < db->inode)
        return -1;
    if (da->inode > db->inode)
        return 1;
    if (da->name && db->name)
        return strcmp(da->name, db->name);
    return 0;
}

TEST(compdev_null_names) {
    test_devcomp_t a = {1, 1, NULL};
    test_devcomp_t b = {1, 1, NULL};
    ASSERT_EQ(test_compare_devices_fn(&a, &b), 0);
}

TEST(compdev_stability) {
    test_devcomp_t a = {42, 100, "/dev/sda"};
    ASSERT_EQ(test_compare_devices_fn(&a, &a), 0);
}

TEST(compdev_sort_by_rdev_first) {
    test_devcomp_t a = {1, 999, "zzz"};
    test_devcomp_t b = {2, 1, "aaa"};
    ASSERT_LT(test_compare_devices_fn(&a, &b), 0);
}

TEST(compdev_sort_by_inode_second) {
    test_devcomp_t a = {5, 10, "zzz"};
    test_devcomp_t b = {5, 20, "aaa"};
    ASSERT_LT(test_compare_devices_fn(&a, &b), 0);
}

TEST(compdev_sort_by_name_third) {
    test_devcomp_t a = {5, 10, "alpha"};
    test_devcomp_t b = {5, 10, "beta"};
    ASSERT_LT(test_compare_devices_fn(&a, &b), 0);
}

TEST(compdev_large_rdev) {
    test_devcomp_t a = {(dev_t)999999, 1, "dev"};
    test_devcomp_t b = {(dev_t)1, 1, "dev"};
    ASSERT_GT(test_compare_devices_fn(&a, &b), 0);
}

TEST(compdev_large_inode) {
    test_devcomp_t a = {1, 999999999UL, "dev"};
    test_devcomp_t b = {1, 1, "dev"};
    ASSERT_GT(test_compare_devices_fn(&a, &b), 0);
}

TEST(compdev_empty_names) {
    test_devcomp_t a = {1, 1, ""};
    test_devcomp_t b = {1, 1, ""};
    ASSERT_EQ(test_compare_devices_fn(&a, &b), 0);
}

TEST(compdev_one_null_name) {
    test_devcomp_t a = {1, 1, "dev"};
    test_devcomp_t b = {1, 1, NULL};
    ASSERT_EQ(test_compare_devices_fn(&a, &b), 0);
}

/* ===== compare_pids() reimplementation ===== */
static int test_compare_pids(const void *first, const void *second) {
    int *pid_ptr1 = (int *)first;
    int *pid_ptr2 = (int *)second;

    if (*pid_ptr1 < *pid_ptr2)
        return -1;
    if (*pid_ptr1 > *pid_ptr2)
        return 1;
    return 0;
}

TEST(comppid_equal) {
    int a = 42, b = 42;
    ASSERT_EQ(test_compare_pids(&a, &b), 0);
}

TEST(comppid_less) {
    int a = 1, b = 100;
    ASSERT_EQ(test_compare_pids(&a, &b), -1);
}

TEST(comppid_greater) {
    int a = 100, b = 1;
    ASSERT_EQ(test_compare_pids(&a, &b), 1);
}

TEST(comppid_sort) {
    int pids[] = {500, 1, 300, 42, 100};
    qsort(pids, 5, sizeof(int), test_compare_pids);
    ASSERT_EQ(pids[0], 1);
    ASSERT_EQ(pids[1], 42);
    ASSERT_EQ(pids[2], 100);
    ASSERT_EQ(pids[3], 300);
    ASSERT_EQ(pids[4], 500);
}

TEST(comppid_negative_pids) {
    int pids[] = {100, -1, 50, -100, 0};
    int n = sizeof(pids) / sizeof(pids[0]);
    qsort(pids, (size_t)n, sizeof(int), test_compare_pids);
    ASSERT_EQ(pids[0], -100);
    ASSERT_EQ(pids[1], -1);
    ASSERT_EQ(pids[2], 0);
    ASSERT_EQ(pids[3], 50);
    ASSERT_EQ(pids[4], 100);
}

TEST(comppid_duplicate_pids) {
    int pids[] = {5, 5, 5};
    int n = sizeof(pids) / sizeof(pids[0]);
    qsort(pids, (size_t)n, sizeof(int), test_compare_pids);
    ASSERT_EQ(pids[0], 5);
    ASSERT_EQ(pids[1], 5);
    ASSERT_EQ(pids[2], 5);
}

TEST(pid_max_int) {
    int a = INT_MAX, b = 1;
    ASSERT_EQ(test_compare_pids(&a, &b), 1);
    ASSERT_EQ(test_compare_pids(&b, &a), -1);
}

TEST(pid_zero_vs_negative) {
    int a = 0, b = -1;
    ASSERT_EQ(test_compare_pids(&a, &b), 1);
}

/* ===== cmp_int_lst() reimplementation ===== */
struct test_int_lst {
    int i;
    int selected;
};

static int test_cmp_int_lst(const void *lhs, const void *rhs) {
    const struct test_int_lst *a = (const struct test_int_lst *)lhs;
    const struct test_int_lst *b = (const struct test_int_lst *)rhs;
    if (a->i < b->i)
        return -1;
    if (a->i > b->i)
        return 1;
    return 0;
}

TEST(cmp_int_lst_equal) {
    struct test_int_lst a = {42, 0}, b = {42, 0};
    ASSERT_EQ(test_cmp_int_lst(&a, &b), 0);
}

TEST(cmp_int_lst_less) {
    struct test_int_lst a = {10, 0}, b = {20, 0};
    ASSERT_EQ(test_cmp_int_lst(&a, &b), -1);
}

TEST(cmp_int_lst_greater) {
    struct test_int_lst a = {20, 0}, b = {10, 0};
    ASSERT_EQ(test_cmp_int_lst(&a, &b), 1);
}

TEST(cmp_int_lst_negative) {
    struct test_int_lst a = {-5, 0}, b = {5, 0};
    ASSERT_LT(test_cmp_int_lst(&a, &b), 0);
}

TEST(cmp_int_lst_sort) {
    struct test_int_lst lst[] = {{500, 0}, {1, 0}, {42, 0}, {-1, 0}, {100, 0}};
    qsort(lst, 5, sizeof(struct test_int_lst), test_cmp_int_lst);
    ASSERT_EQ(lst[0].i, -1);
    ASSERT_EQ(lst[1].i, 1);
    ASSERT_EQ(lst[2].i, 42);
    ASSERT_EQ(lst[3].i, 100);
    ASSERT_EQ(lst[4].i, 500);
}

TEST(cmp_int_lst_zero) {
    struct test_int_lst a = {0, 0}, b = {0, 0};
    ASSERT_EQ(test_cmp_int_lst(&a, &b), 0);
}

TEST(cmp_int_lst_int_extremes) {
    struct test_int_lst a = {INT_MIN, 0}, b = {INT_MAX, 0};
    ASSERT_LT(test_cmp_int_lst(&a, &b), 0);
    ASSERT_GT(test_cmp_int_lst(&b, &a), 0);
}

/* ===== find_int_lst() reimplementation (binary search) ===== */
static struct test_int_lst *test_find_int_lst(struct test_int_lst *list, int num_entries, int val) {
    int low = 0, high = num_entries - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (list[mid].i == val)
            return &list[mid];
        if (list[mid].i < val)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return NULL;
}

TEST(find_int_lst_found) {
    struct test_int_lst lst[] = {{1, 0}, {5, 0}, {10, 0}, {20, 0}, {50, 0}};
    struct test_int_lst *r = test_find_int_lst(lst, 5, 10);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(r->i, 10);
}

TEST(find_int_lst_not_found) {
    struct test_int_lst lst[] = {{1, 0}, {5, 0}, {10, 0}, {20, 0}, {50, 0}};
    ASSERT_NULL(test_find_int_lst(lst, 5, 7));
}

TEST(find_int_lst_first) {
    struct test_int_lst lst[] = {{1, 0}, {5, 0}, {10, 0}};
    struct test_int_lst *r = test_find_int_lst(lst, 3, 1);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(r->i, 1);
}

TEST(find_int_lst_last) {
    struct test_int_lst lst[] = {{1, 0}, {5, 0}, {10, 0}};
    struct test_int_lst *r = test_find_int_lst(lst, 3, 10);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(r->i, 10);
}

TEST(find_int_lst_single_hit) {
    struct test_int_lst lst[] = {{42, 0}};
    struct test_int_lst *r = test_find_int_lst(lst, 1, 42);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(r->i, 42);
}

TEST(find_int_lst_single_miss) {
    struct test_int_lst lst[] = {{42, 0}};
    ASSERT_NULL(test_find_int_lst(lst, 1, 99));
}

TEST(find_int_lst_large_sorted) {
    struct test_int_lst lst[100];
    for (int i = 0; i < 100; i++) {
        lst[i].i = i * 10;
        lst[i].selected = 0;
    }
    for (int i = 0; i < 100; i += 3) {
        struct test_int_lst *r = test_find_int_lst(lst, 100, i * 10);
        ASSERT_NOT_NULL(r);
        ASSERT_EQ(r->i, i * 10);
    }
    ASSERT_NULL(test_find_int_lst(lst, 100, 5));
    ASSERT_NULL(test_find_int_lst(lst, 100, -1));
    ASSERT_NULL(test_find_int_lst(lst, 100, 1000));
}

/* ===== Device number decomposition ===== */
TEST(devnum_major_minor) {
    dev_t d = makedev(8, 1);
    ASSERT_EQ((int)major(d), 8);
    ASSERT_EQ((int)minor(d), 1);
}

TEST(devnum_makedev) {
    unsigned int maj = 253, min = 7;
    dev_t d = makedev(maj, min);
    ASSERT_EQ((unsigned int)major(d), maj);
    ASSERT_EQ((unsigned int)minor(d), min);
}

TEST(devnum_zero) {
    dev_t d = 0;
    ASSERT_EQ((int)major(d), 0);
    ASSERT_EQ((int)minor(d), 0);
}

/* ===== rmdupdev logic ===== */
static int test_remove_dup_devs(test_devcomp_t *devs, int n) {
    if (n <= 1)
        return n;
    int out = 1;
    for (int i = 1; i < n; i++) {
        if (devs[i].rdev != devs[i - 1].rdev || devs[i].inode != devs[i - 1].inode) {
            devs[out++] = devs[i];
        }
    }
    return out;
}

TEST(rmdupdev_no_dups) {
    test_devcomp_t devs[] = {{1, 1, NULL}, {2, 2, NULL}, {3, 3, NULL}};
    int n = test_remove_dup_devs(devs, 3);
    ASSERT_EQ(n, 3);
}

TEST(rmdupdev_all_same) {
    test_devcomp_t devs[] = {{1, 1, NULL}, {1, 1, NULL}, {1, 1, NULL}};
    int n = test_remove_dup_devs(devs, 3);
    ASSERT_EQ(n, 1);
}

TEST(rmdupdev_adjacent_dups) {
    test_devcomp_t devs[] = {{1, 1, NULL}, {1, 1, NULL}, {2, 2, NULL}, {2, 2, NULL}, {3, 3, NULL}};
    int n = test_remove_dup_devs(devs, 5);
    ASSERT_EQ(n, 3);
}

TEST(rmdupdev_single) {
    test_devcomp_t devs[] = {{1, 1, NULL}};
    int n = test_remove_dup_devs(devs, 1);
    ASSERT_EQ(n, 1);
}

TEST(rmdupdev_empty) {
    int n = test_remove_dup_devs(NULL, 0);
    ASSERT_EQ(n, 0);
}

TEST(rmdupdev_same_rdev_diff_inode) {
    test_devcomp_t devs[] = {{1, 1, NULL}, {1, 2, NULL}, {1, 3, NULL}};
    int n = test_remove_dup_devs(devs, 3);
    ASSERT_EQ(n, 3);
}

TEST(rmdupdev_interleaved) {
    test_devcomp_t devs[] = {{1, 1, NULL}, {1, 1, NULL}, {2, 2, NULL}, {3, 3, NULL},
                             {3, 3, NULL}, {3, 3, NULL}, {4, 4, NULL}};
    int n = test_remove_dup_devs(devs, 7);
    ASSERT_EQ(n, 4);
    ASSERT_EQ((int)devs[0].rdev, 1);
    ASSERT_EQ((int)devs[1].rdev, 2);
    ASSERT_EQ((int)devs[2].rdev, 3);
    ASSERT_EQ((int)devs[3].rdev, 4);
}

TEST(rmdupdev_two_elements_same) {
    test_devcomp_t devs[] = {{5, 5, NULL}, {5, 5, NULL}};
    int n = test_remove_dup_devs(devs, 2);
    ASSERT_EQ(n, 1);
}

TEST(rmdupdev_two_elements_different) {
    test_devcomp_t devs[] = {{5, 5, NULL}, {6, 6, NULL}};
    int n = test_remove_dup_devs(devs, 2);
    ASSERT_EQ(n, 2);
}

#endif /* TEST_UNIT_COMPARE_H */
