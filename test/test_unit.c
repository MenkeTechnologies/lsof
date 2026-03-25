/*
 * test_unit.c - unit tests for lsof internal functions
 *
 * Tests pure/isolated functions that can be compiled and tested without
 * the full lsof dialect-specific build infrastructure.
 */

#include "test_framework.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* ===== lsof_fields.h constants ===== */
#include "../src/lsof_fields.h"

TEST(field_ids_are_unique) {
    char ids[] = {
        LSOF_FID_ACCESS, LSOF_FID_CMD, LSOF_FID_CT, LSOF_FID_DEVCH,
        LSOF_FID_DEVN, LSOF_FID_FD, LSOF_FID_FA, LSOF_FID_FG,
        LSOF_FID_INODE, LSOF_FID_NLINK, LSOF_FID_TID, LSOF_FID_LOCK,
        LSOF_FID_LOGIN, LSOF_FID_MARK, LSOF_FID_NAME, LSOF_FID_NI,
        LSOF_FID_OFFSET, LSOF_FID_PID, LSOF_FID_PGID, LSOF_FID_PROTO,
        LSOF_FID_RDEV, LSOF_FID_PPID, LSOF_FID_SIZE, LSOF_FID_STREAM,
        LSOF_FID_TYPE, LSOF_FID_TCPTPI, LSOF_FID_UID, LSOF_FID_ZONE,
        LSOF_FID_CNTX, LSOF_FID_TERM,
    };
    int n = sizeof(ids) / sizeof(ids[0]);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            ASSERT_NE(ids[i], ids[j]);
        }
    }
}

TEST(field_indices_sequential) {
    ASSERT_EQ(LSOF_FIX_ACCESS, 0);
    ASSERT_EQ(LSOF_FIX_CMD, 1);
    ASSERT_EQ(LSOF_FIX_CT, 2);
    ASSERT_EQ(LSOF_FIX_DEVCH, 3);
    ASSERT_EQ(LSOF_FIX_DEVN, 4);
    ASSERT_EQ(LSOF_FIX_FD, 5);
    ASSERT_EQ(LSOF_FIX_FA, 6);
    ASSERT_EQ(LSOF_FIX_FG, 7);
    ASSERT_EQ(LSOF_FIX_INODE, 8);
    ASSERT_EQ(LSOF_FIX_NLINK, 9);
    ASSERT_EQ(LSOF_FIX_TID, 10);
    ASSERT_EQ(LSOF_FIX_LOCK, 11);
    ASSERT_EQ(LSOF_FIX_LOGIN, 12);
    ASSERT_EQ(LSOF_FIX_MARK, 13);
    ASSERT_EQ(LSOF_FIX_NAME, 14);
    ASSERT_EQ(LSOF_FIX_NI, 15);
    ASSERT_EQ(LSOF_FIX_OFFSET, 16);
    ASSERT_EQ(LSOF_FIX_PID, 17);
    ASSERT_EQ(LSOF_FIX_PGID, 18);
    ASSERT_EQ(LSOF_FIX_PROTO, 19);
    ASSERT_EQ(LSOF_FIX_RDEV, 20);
    ASSERT_EQ(LSOF_FIX_PPID, 21);
    ASSERT_EQ(LSOF_FIX_SIZE, 22);
    ASSERT_EQ(LSOF_FIX_STREAM, 23);
    ASSERT_EQ(LSOF_FIX_TYPE, 24);
    ASSERT_EQ(LSOF_FIX_TCPTPI, 25);
    ASSERT_EQ(LSOF_FIX_UID, 26);
    ASSERT_EQ(LSOF_FIX_ZONE, 27);
    ASSERT_EQ(LSOF_FIX_CNTX, 28);
    ASSERT_EQ(LSOF_FIX_TERM, 29);
}

TEST(field_id_values) {
    ASSERT_EQ(LSOF_FID_ACCESS, 'a');
    ASSERT_EQ(LSOF_FID_CMD, 'c');
    ASSERT_EQ(LSOF_FID_PID, 'p');
    ASSERT_EQ(LSOF_FID_UID, 'u');
    ASSERT_EQ(LSOF_FID_FD, 'f');
    ASSERT_EQ(LSOF_FID_NAME, 'n');
    ASSERT_EQ(LSOF_FID_TYPE, 't');
    ASSERT_EQ(LSOF_FID_SIZE, 's');
    ASSERT_EQ(LSOF_FID_TERM, '0');
}


/* ===== x2dev() reimplementation test =====
 * We reimplement x2dev() here to test its logic independently of the
 * lsof global header dependencies.
 */
static char *
test_x2dev(char *s, dev_t *d)
{
    char *cp, *cp1;
    int n;
    dev_t r;

    if (strncasecmp(s, "0x", 2) == 0)
        s += 2;
    for (cp = s, n = 0; *cp; cp++, n++) {
        if (isdigit((unsigned char)*cp))
            continue;
        if ((unsigned char)*cp >= 'a' && (unsigned char)*cp <= 'f')
            continue;
        if ((unsigned char)*cp >= 'A' && (unsigned char)*cp <= 'F')
            continue;
        if (*cp == ' ' || *cp == ',')
            break;
        return NULL;
    }
    if (!n)
        return NULL;
    if (n > (2 * (int)sizeof(dev_t))) {
        cp1 = s;
        s += (n - (2 * sizeof(dev_t)));
        while (cp1 < s) {
            if (*cp1 != 'f' && *cp1 != 'F')
                return NULL;
            cp1++;
        }
    }
    for (r = 0; s < cp; s++) {
        r = r << 4;
        if (isdigit((unsigned char)*s))
            r |= (unsigned char)(*s - '0') & 0xf;
        else {
            if (isupper((unsigned char)*s))
                r |= ((unsigned char)(*s - 'A') + 10) & 0xf;
            else
                r |= ((unsigned char)(*s - 'a') + 10) & 0xf;
        }
    }
    *d = r;
    return s;
}

TEST(x2dev_simple_hex) {
    dev_t d = 0;
    char *r = test_x2dev("ff", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0xff);
}

TEST(x2dev_with_0x_prefix) {
    dev_t d = 0;
    char *r = test_x2dev("0x1a2b", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0x1a2b);
}

TEST(x2dev_with_0X_prefix) {
    dev_t d = 0;
    char *r = test_x2dev("0X1A2B", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0x1a2b);
}

TEST(x2dev_uppercase) {
    dev_t d = 0;
    char *r = test_x2dev("ABCDEF", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0xabcdef);
}

TEST(x2dev_stops_at_space) {
    dev_t d = 0;
    char *r = test_x2dev("0xab cd", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0xab);
    ASSERT_EQ(*r, ' ');
}

TEST(x2dev_stops_at_comma) {
    dev_t d = 0;
    char *r = test_x2dev("1f,rest", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0x1f);
    ASSERT_EQ(*r, ',');
}

TEST(x2dev_empty_string) {
    dev_t d = 99;
    char *r = test_x2dev("", &d);
    ASSERT_NULL(r);
}

TEST(x2dev_invalid_char) {
    dev_t d = 0;
    char *r = test_x2dev("0xGG", &d);
    ASSERT_NULL(r);
}

TEST(x2dev_just_0x) {
    dev_t d = 99;
    char *r = test_x2dev("0x", &d);
    ASSERT_NULL(r);
}

TEST(x2dev_zero) {
    dev_t d = 99;
    char *r = test_x2dev("0", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0);
}

TEST(x2dev_single_digit) {
    dev_t d = 0;
    char *r = test_x2dev("a", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0xa);
}

TEST(x2dev_mixed_case) {
    dev_t d = 0;
    char *r = test_x2dev("0xAaBbCc", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0xaabbcc);
}


/* ===== HASHPORT macro test ===== */
#define PORTHASHBUCKETS 128
#define HASHPORT(p) (((((int)(p)) * 31415) >> 3) & (PORTHASHBUCKETS - 1))

TEST(hashport_range) {
    for (int p = 0; p < 65536; p++) {
        int h = HASHPORT(p);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, PORTHASHBUCKETS);
    }
}

TEST(hashport_distribution) {
    int buckets[PORTHASHBUCKETS];
    memset(buckets, 0, sizeof(buckets));
    for (int p = 0; p < 1024; p++) {
        buckets[HASHPORT(p)]++;
    }
    int used = 0;
    for (int i = 0; i < PORTHASHBUCKETS; i++) {
        if (buckets[i] > 0) used++;
    }
    ASSERT_GT(used, PORTHASHBUCKETS / 2);
}

TEST(hashport_deterministic) {
    ASSERT_EQ(HASHPORT(80), HASHPORT(80));
    ASSERT_EQ(HASHPORT(443), HASHPORT(443));
    ASSERT_EQ(HASHPORT(0), HASHPORT(0));
}


/* ===== safestrlen() reimplementation test ===== */
static int
test_safestrlen(char *sp, int flags)
{
    char c;
    int len = 0;

    c = (flags & 2) ? ' ' : '\0';
    if (sp) {
        for (; *sp; sp++) {
            if (!isprint((unsigned char)*sp) || *sp == c) {
                if (*sp < 0x20 || (unsigned char)*sp == 0xff)
                    len += 2;
                else
                    len += 4;
            } else
                len++;
        }
    }
    return len;
}

TEST(safestrlen_normal_string) {
    ASSERT_EQ(test_safestrlen("hello", 0), 5);
}

TEST(safestrlen_empty) {
    ASSERT_EQ(test_safestrlen("", 0), 0);
}

TEST(safestrlen_null) {
    ASSERT_EQ(test_safestrlen(NULL, 0), 0);
}

TEST(safestrlen_with_tab) {
    ASSERT_EQ(test_safestrlen("a\tb", 0), 4);
}

TEST(safestrlen_with_newline) {
    ASSERT_EQ(test_safestrlen("a\nb", 0), 4);
}

TEST(safestrlen_space_not_printable) {
    ASSERT_EQ(test_safestrlen("a b", 2), 6);
}

TEST(safestrlen_space_printable) {
    ASSERT_EQ(test_safestrlen("a b", 0), 3);
}

TEST(safestrlen_0xff_char) {
    char s[3] = {'A', (char)0xff, '\0'};
    ASSERT_EQ(test_safestrlen(s, 0), 3);
}

TEST(safestrlen_high_byte) {
    char s[3] = {'A', (char)0x80, '\0'};
    ASSERT_EQ(test_safestrlen(s, 0), 5);
}


/* ===== compdev() reimplementation test ===== */
typedef unsigned long test_INODETYPE;

struct test_l_dev {
    dev_t rdev;
    test_INODETYPE inode;
    char *name;
    int v;
};

static int
test_compdev(const void *a1, const void *a2)
{
    struct test_l_dev **p1 = (struct test_l_dev **)a1;
    struct test_l_dev **p2 = (struct test_l_dev **)a2;

    if ((dev_t)((*p1)->rdev) < (dev_t)((*p2)->rdev))
        return -1;
    if ((dev_t)((*p1)->rdev) > (dev_t)((*p2)->rdev))
        return 1;
    if ((test_INODETYPE)((*p1)->inode) < (test_INODETYPE)((*p2)->inode))
        return -1;
    if ((test_INODETYPE)((*p1)->inode) > (test_INODETYPE)((*p2)->inode))
        return 1;
    return strcmp((*p1)->name, (*p2)->name);
}

TEST(compdev_equal) {
    struct test_l_dev a = {1, 100, "dev0", 0};
    struct test_l_dev b = {1, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compdev(&pa, &pb), 0);
}

TEST(compdev_rdev_less) {
    struct test_l_dev a = {1, 100, "dev0", 0};
    struct test_l_dev b = {2, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compdev(&pa, &pb), -1);
}

TEST(compdev_rdev_greater) {
    struct test_l_dev a = {3, 100, "dev0", 0};
    struct test_l_dev b = {2, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compdev(&pa, &pb), 1);
}

TEST(compdev_inode_less) {
    struct test_l_dev a = {1, 50, "dev0", 0};
    struct test_l_dev b = {1, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compdev(&pa, &pb), -1);
}

TEST(compdev_inode_greater) {
    struct test_l_dev a = {1, 200, "dev0", 0};
    struct test_l_dev b = {1, 100, "dev0", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_EQ(test_compdev(&pa, &pb), 1);
}

TEST(compdev_name_compare) {
    struct test_l_dev a = {1, 100, "alpha", 0};
    struct test_l_dev b = {1, 100, "beta", 0};
    struct test_l_dev *pa = &a, *pb = &b;
    ASSERT_LT(test_compdev(&pa, &pb), 0);
}

TEST(compdev_sort_array) {
    struct test_l_dev devs[] = {
        {3, 10, "c", 0},
        {1, 10, "a", 0},
        {2, 10, "b", 0},
        {1, 5, "a", 0},
        {1, 10, "z", 0},
    };
    struct test_l_dev *ptrs[5];
    for (int i = 0; i < 5; i++) ptrs[i] = &devs[i];

    qsort(ptrs, 5, sizeof(struct test_l_dev *), test_compdev);

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


/* ===== comppid() reimplementation test ===== */
static int
test_comppid(const void *a1, const void *a2)
{
    int *p1 = (int *)a1;
    int *p2 = (int *)a2;

    if (*p1 < *p2) return -1;
    if (*p1 > *p2) return 1;
    return 0;
}

TEST(comppid_equal) {
    int a = 42, b = 42;
    ASSERT_EQ(test_comppid(&a, &b), 0);
}

TEST(comppid_less) {
    int a = 1, b = 100;
    ASSERT_EQ(test_comppid(&a, &b), -1);
}

TEST(comppid_greater) {
    int a = 100, b = 1;
    ASSERT_EQ(test_comppid(&a, &b), 1);
}

TEST(comppid_sort) {
    int pids[] = {500, 1, 300, 42, 100};
    qsort(pids, 5, sizeof(int), test_comppid);
    ASSERT_EQ(pids[0], 1);
    ASSERT_EQ(pids[1], 42);
    ASSERT_EQ(pids[2], 100);
    ASSERT_EQ(pids[3], 300);
    ASSERT_EQ(pids[4], 500);
}


/* ===== safepup() reimplementation test ===== */
static char safepup_buf[8];

static char *
test_safepup(unsigned int c, int *cl)
{
    int len;
    char *rp;

    if (c < 0x20) {
        switch (c) {
        case '\b': rp = "\\b"; break;
        case '\f': rp = "\\f"; break;
        case '\n': rp = "\\n"; break;
        case '\r': rp = "\\r"; break;
        case '\t': rp = "\\t"; break;
        default:
            snprintf(safepup_buf, sizeof(safepup_buf), "^%c", c + 0x40);
            rp = safepup_buf;
        }
        len = 2;
    } else if (c == 0xff) {
        rp = "^?";
        len = 2;
    } else {
        snprintf(safepup_buf, sizeof(safepup_buf), "\\x%02x", (int)(c & 0xff));
        rp = safepup_buf;
        len = 4;
    }
    if (cl)
        *cl = len;
    return rp;
}

TEST(safepup_tab) {
    int cl;
    ASSERT_STR_EQ(test_safepup('\t', &cl), "\\t");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_newline) {
    int cl;
    ASSERT_STR_EQ(test_safepup('\n', &cl), "\\n");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_backspace) {
    int cl;
    ASSERT_STR_EQ(test_safepup('\b', &cl), "\\b");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_formfeed) {
    int cl;
    ASSERT_STR_EQ(test_safepup('\f', &cl), "\\f");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_cr) {
    int cl;
    ASSERT_STR_EQ(test_safepup('\r', &cl), "\\r");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_ctrl_a) {
    int cl;
    ASSERT_STR_EQ(test_safepup(0x01, &cl), "^A");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_null_char) {
    int cl;
    ASSERT_STR_EQ(test_safepup(0x00, &cl), "^@");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_0xff) {
    int cl;
    ASSERT_STR_EQ(test_safepup(0xff, &cl), "^?");
    ASSERT_EQ(cl, 2);
}

TEST(safepup_high_byte) {
    int cl;
    ASSERT_STR_EQ(test_safepup(0x80, &cl), "\\x80");
    ASSERT_EQ(cl, 4);
}

TEST(safepup_null_cl) {
    char *r = test_safepup('\n', NULL);
    ASSERT_STR_EQ(r, "\\n");
}


/* ===== Cross-over option value tests ===== */
#define XO_FILESYS  0x1
#define XO_SYMLINK  0x2
#define XO_ALL      (XO_FILESYS | XO_SYMLINK)

TEST(crossover_flags) {
    ASSERT_EQ(XO_FILESYS & XO_SYMLINK, 0);
    ASSERT_EQ(XO_ALL, 0x3);
    ASSERT_TRUE((XO_FILESYS & XO_ALL) != 0);
    ASSERT_TRUE((XO_SYMLINK & XO_ALL) != 0);
}


/* ===== FSV flag tests ===== */
#define FSV_FA 0x1
#define FSV_CT 0x2
#define FSV_FG 0x4
#define FSV_NI 0x8

TEST(fsv_flags_no_overlap) {
    ASSERT_EQ(FSV_FA & FSV_CT, 0);
    ASSERT_EQ(FSV_FA & FSV_FG, 0);
    ASSERT_EQ(FSV_FA & FSV_NI, 0);
    ASSERT_EQ(FSV_CT & FSV_FG, 0);
    ASSERT_EQ(FSV_CT & FSV_NI, 0);
    ASSERT_EQ(FSV_FG & FSV_NI, 0);
}

TEST(fsv_flags_combine) {
    int all = FSV_FA | FSV_CT | FSV_FG | FSV_NI;
    ASSERT_EQ(all, 0xf);
    ASSERT_TRUE((all & FSV_FA) != 0);
    ASSERT_TRUE((all & FSV_CT) != 0);
    ASSERT_TRUE((all & FSV_FG) != 0);
    ASSERT_TRUE((all & FSV_NI) != 0);
}


/* ===== Test Registry ===== */
static tf_test_entry all_tests[] = {
    REGISTER_TEST(field_ids_are_unique),
    REGISTER_TEST(field_indices_sequential),
    REGISTER_TEST(field_id_values),
    REGISTER_TEST(x2dev_simple_hex),
    REGISTER_TEST(x2dev_with_0x_prefix),
    REGISTER_TEST(x2dev_with_0X_prefix),
    REGISTER_TEST(x2dev_uppercase),
    REGISTER_TEST(x2dev_stops_at_space),
    REGISTER_TEST(x2dev_stops_at_comma),
    REGISTER_TEST(x2dev_empty_string),
    REGISTER_TEST(x2dev_invalid_char),
    REGISTER_TEST(x2dev_just_0x),
    REGISTER_TEST(x2dev_zero),
    REGISTER_TEST(x2dev_single_digit),
    REGISTER_TEST(x2dev_mixed_case),
    REGISTER_TEST(hashport_range),
    REGISTER_TEST(hashport_distribution),
    REGISTER_TEST(hashport_deterministic),
    REGISTER_TEST(safestrlen_normal_string),
    REGISTER_TEST(safestrlen_empty),
    REGISTER_TEST(safestrlen_null),
    REGISTER_TEST(safestrlen_with_tab),
    REGISTER_TEST(safestrlen_with_newline),
    REGISTER_TEST(safestrlen_space_not_printable),
    REGISTER_TEST(safestrlen_space_printable),
    REGISTER_TEST(safestrlen_0xff_char),
    REGISTER_TEST(safestrlen_high_byte),
    REGISTER_TEST(compdev_equal),
    REGISTER_TEST(compdev_rdev_less),
    REGISTER_TEST(compdev_rdev_greater),
    REGISTER_TEST(compdev_inode_less),
    REGISTER_TEST(compdev_inode_greater),
    REGISTER_TEST(compdev_name_compare),
    REGISTER_TEST(compdev_sort_array),
    REGISTER_TEST(comppid_equal),
    REGISTER_TEST(comppid_less),
    REGISTER_TEST(comppid_greater),
    REGISTER_TEST(comppid_sort),
    REGISTER_TEST(safepup_tab),
    REGISTER_TEST(safepup_newline),
    REGISTER_TEST(safepup_backspace),
    REGISTER_TEST(safepup_formfeed),
    REGISTER_TEST(safepup_cr),
    REGISTER_TEST(safepup_ctrl_a),
    REGISTER_TEST(safepup_null_char),
    REGISTER_TEST(safepup_0xff),
    REGISTER_TEST(safepup_high_byte),
    REGISTER_TEST(safepup_null_cl),
    REGISTER_TEST(crossover_flags),
    REGISTER_TEST(fsv_flags_no_overlap),
    REGISTER_TEST(fsv_flags_combine),
};

RUN_TESTS_FROM(all_tests);
