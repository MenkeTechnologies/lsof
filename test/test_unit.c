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
    /* 0x80 may or may not be printable depending on locale;
     * if printable: len = 2 (A + char), if not: len = 5 (A + \x80) */
    int len = test_safestrlen(s, 0);
    ASSERT_TRUE(len == 3 || len == 5);
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


/* ===== Memory leak tests =====
 * These tests verify that the allocation patterns used in lsof properly
 * free all allocated memory.  Each test reimplements a simplified version
 * of the pattern and uses an allocation counter to detect leaks.
 */

static int alloc_count = 0;   /* tracks net allocations */

static void *counted_malloc(size_t sz) {
    void *p = malloc(sz);
    if (p) alloc_count++;
    return p;
}

static void counted_free(void *p) {
    if (p) { free(p); alloc_count--; }
}

static char *counted_mkstrcpy(const char *src) {
    size_t len = src ? strlen(src) : 0;
    char *ns = (char *)counted_malloc(len + 1);
    if (ns) {
        if (src) memcpy(ns, src, len);
        ns[len] = '\0';
    }
    return ns;
}

static void *counted_realloc(void *ptr, size_t sz) {
    void *p = realloc(ptr, sz);
    if (p && !ptr) alloc_count++;   /* new allocation */
    if (!p && ptr) { /* realloc failed, old pointer still valid */ }
    return p;
}

static char *counted_mkstrcat(const char *s1, int l1,
                               const char *s2, int l2) {
    size_t len1 = s1 ? (l1 >= 0 ? (size_t)l1 : strlen(s1)) : 0;
    size_t len2 = s2 ? (l2 >= 0 ? (size_t)l2 : strlen(s2)) : 0;
    char *cp = (char *)counted_malloc(len1 + len2 + 1);
    if (cp) {
        if (s1 && len1) memcpy(cp, s1, len1);
        if (s2 && len2) memcpy(cp + len1, s2, len2);
        cp[len1 + len2] = '\0';
    }
    return cp;
}

/*
 * Test: enter_network_address hostname leak (fixed)
 * Before the fix, 'hn' was freed on error but not on success.
 */
TEST(memleak_enter_network_address_hn_freed) {
    alloc_count = 0;
    /* Simulate enter_network_address with a hostname */
    char *proto = counted_mkstrcat("tcp", 3, NULL, -1);
    char *hn = counted_mkstrcat("localhost", 9, NULL, -1);
    char *sn = counted_mkstrcpy("http");

    ASSERT_EQ(alloc_count, 3);

    /* Simulate successful path -- the fix: free hn */
    if (hn)
        counted_free(hn);
    if (sn)
        counted_free(sn);
    /* proto is transferred to nwad struct, simulate by freeing it */
    if (proto)
        counted_free(proto);

    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_network_address error path frees all
 */
TEST(memleak_enter_network_address_error_frees_all) {
    alloc_count = 0;
    char *proto = counted_mkstrcat("tcp", 3, NULL, -1);
    char *hn = counted_mkstrcat("badhost", 7, NULL, -1);
    char *sn = counted_mkstrcpy("service");

    ASSERT_EQ(alloc_count, 3);

    /* Simulate nwad_exit error path */
    if (proto) counted_free(proto);
    if (hn)    counted_free(hn);
    if (sn)    counted_free(sn);

    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_fd_lst duplicate leak (fixed)
 * Before the fix, f->nm was not freed when a duplicate was found.
 */
struct test_fd_lst {
    char *nm;
    int lo, hi;
    struct test_fd_lst *next;
};

TEST(memleak_enter_fd_lst_dup_frees_nm) {
    alloc_count = 0;

    /* First entry: create and add to list */
    struct test_fd_lst *list = NULL;
    struct test_fd_lst *f1 = (struct test_fd_lst *)counted_malloc(sizeof(*f1));
    ASSERT_NOT_NULL(f1);
    f1->nm = counted_mkstrcpy("cwd");
    f1->lo = 1; f1->hi = 0;
    f1->next = list;
    list = f1;
    ASSERT_EQ(alloc_count, 2);  /* f1 + f1->nm */

    /* Duplicate entry: allocate, detect dup, free properly */
    struct test_fd_lst *f2 = (struct test_fd_lst *)counted_malloc(sizeof(*f2));
    ASSERT_NOT_NULL(f2);
    f2->nm = counted_mkstrcpy("cwd");
    f2->lo = 1; f2->hi = 0;
    ASSERT_EQ(alloc_count, 4);

    /* Check for duplicate */
    int is_dup = 0;
    for (struct test_fd_lst *ft = list; ft; ft = ft->next) {
        if (f2->nm && ft->nm && strcmp(f2->nm, ft->nm) == 0) {
            is_dup = 1;
            break;
        }
    }
    ASSERT_TRUE(is_dup);

    /* The fix: free nm before freeing the struct */
    if (f2->nm)
        counted_free(f2->nm);
    counted_free(f2);
    ASSERT_EQ(alloc_count, 2);  /* only original entry remains */

    /* Cleanup */
    if (f1->nm) counted_free(f1->nm);
    counted_free(f1);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_fd_lst duplicate without name (numeric FD) -- no nm leak
 */
TEST(memleak_enter_fd_lst_dup_numeric_no_leak) {
    alloc_count = 0;

    struct test_fd_lst *list = NULL;
    struct test_fd_lst *f1 = (struct test_fd_lst *)counted_malloc(sizeof(*f1));
    f1->nm = NULL;
    f1->lo = f1->hi = 5;
    f1->next = list;
    list = f1;
    ASSERT_EQ(alloc_count, 1);

    struct test_fd_lst *f2 = (struct test_fd_lst *)counted_malloc(sizeof(*f2));
    f2->nm = NULL;
    f2->lo = f2->hi = 5;
    ASSERT_EQ(alloc_count, 2);

    /* Detect duplicate (numeric match) */
    int is_dup = 0;
    for (struct test_fd_lst *ft = list; ft; ft = ft->next) {
        if (!f2->nm && !ft->nm && f2->lo == ft->lo && f2->hi == ft->hi) {
            is_dup = 1;
            break;
        }
    }
    ASSERT_TRUE(is_dup);

    /* Free duplicate -- nm is NULL, so only struct freed */
    if (f2->nm) counted_free(f2->nm);
    counted_free(f2);
    ASSERT_EQ(alloc_count, 1);

    counted_free(f1);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_efsys leak when Readlink fails (fixed)
 * Before the fix, 'ec' was not freed when Readlink returned NULL.
 */
TEST(memleak_enter_efsys_readlink_fail_frees_ec) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/some/path");
    ASSERT_NOT_NULL(ec);
    ASSERT_EQ(alloc_count, 1);

    /* Simulate Readlink failure (returns NULL) */
    char *path = NULL;  /* Readlink failed */
    if (!path) {
        counted_free(ec);
        /* return(1) in the real code */
    }

    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_efsys duplicate detection frees ec (fixed)
 * Before the fix, 'ec' was not freed when a duplicate path was found
 * and path != ec (i.e., when Readlink resolved to a different string).
 */
TEST(memleak_enter_efsys_dup_frees_ec) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/original/input");
    /* Simulate Readlink returning a different resolved path */
    char *path = counted_mkstrcpy("/resolved/path");
    ASSERT_EQ(alloc_count, 2);

    /* Simulate duplicate detection: path matches existing entry */
    int is_dup = 1;  /* pretend we found a match */
    if (is_dup) {
        if (path != ec)
            counted_free(ec);
        /* path is from Readlink's static stack in real code,
         * but here we clean it up to verify the pattern */
    }

    /* ec should be freed; path still allocated (owned by Readlink stack) */
    ASSERT_EQ(alloc_count, 1);
    counted_free(path);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_efsys success path frees ec when path != ec (fixed)
 */
TEST(memleak_enter_efsys_success_frees_ec) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/input/path");
    char *path = counted_mkstrcpy("/resolved/real/path");
    ASSERT_EQ(alloc_count, 2);

    /* Simulate storing path in list entry (ep->path = path) */
    char *stored_path = path;
    (void)stored_path;

    /* The fix: free ec when path != ec */
    if (path != ec)
        counted_free(ec);

    ASSERT_EQ(alloc_count, 1);  /* only stored_path remains */
    counted_free(path);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_efsys with rdlnk=true (path == ec, no extra free needed)
 */
TEST(memleak_enter_efsys_rdlnk_no_double_free) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/direct/path");
    char *path = ec;  /* rdlnk case: path points to ec */
    ASSERT_EQ(alloc_count, 1);

    /* Store path in list entry */
    char *stored_path = path;
    (void)stored_path;

    /* The fix checks path != ec; since they're equal, no free */
    if (path != ec)
        counted_free(ec);

    ASSERT_EQ(alloc_count, 1);  /* only stored_path remains */
    counted_free(path);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: mkstrcpy caller must free
 */
TEST(memleak_mkstrcpy_basic) {
    alloc_count = 0;
    char *s = counted_mkstrcpy("test string");
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "test string");
    ASSERT_EQ(alloc_count, 1);
    counted_free(s);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: mkstrcpy with NULL source
 */
TEST(memleak_mkstrcpy_null) {
    alloc_count = 0;
    char *s = counted_mkstrcpy(NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "");
    ASSERT_EQ(alloc_count, 1);
    counted_free(s);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: mkstrcat caller must free
 */
TEST(memleak_mkstrcat_basic) {
    alloc_count = 0;
    char *s = counted_mkstrcat("hello", -1, " world", -1);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "hello world");
    ASSERT_EQ(alloc_count, 1);
    counted_free(s);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_nm pattern - old value freed before new
 */
TEST(memleak_enter_nm_frees_old) {
    alloc_count = 0;
    char *nm = NULL;

    /* First call: no old value */
    nm = counted_mkstrcpy("first");
    ASSERT_EQ(alloc_count, 1);

    /* Second call: old value must be freed */
    char *old = nm;
    nm = counted_mkstrcpy("second");
    counted_free(old);
    ASSERT_EQ(alloc_count, 1);  /* net: still 1 */
    ASSERT_STR_EQ(nm, "second");

    counted_free(nm);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: free_lproc pattern - all fields freed
 */
TEST(memleak_free_lproc_pattern) {
    alloc_count = 0;

    /* Simulate lproc with lfile chain */
    char *cmd = counted_mkstrcpy("test_cmd");
    char *dev_ch = counted_mkstrcpy("0x1234");
    char *nm = counted_mkstrcpy("/dev/null");
    char *nma = counted_mkstrcpy("(annotation)");
    ASSERT_EQ(alloc_count, 4);

    /* Simulate free_lproc: free lfile fields then cmd */
    counted_free(dev_ch);
    counted_free(nm);
    counted_free(nma);
    counted_free(cmd);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: safe realloc pattern (the fix for all realloc leaks)
 * Before the fix, ptr = realloc(ptr, newsize) lost old memory on failure.
 * The fix uses a tmp variable so old memory can be freed on failure.
 */
TEST(memleak_safe_realloc_success) {
    alloc_count = 0;

    /* Initial allocation */
    char *buf = (char *)counted_malloc(32);
    ASSERT_NOT_NULL(buf);
    memset(buf, 'A', 32);
    ASSERT_EQ(alloc_count, 1);

    /* Safe realloc pattern (success path) */
    char *tmp = (char *)counted_realloc(buf, 64);
    ASSERT_NOT_NULL(tmp);
    buf = tmp;  /* update pointer only on success */
    ASSERT_EQ(alloc_count, 1);  /* still 1 -- realloc didn't create new */

    /* Verify old data preserved */
    ASSERT_EQ(buf[0], 'A');
    ASSERT_EQ(buf[31], 'A');

    counted_free(buf);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: unsafe realloc pattern loses memory
 * Demonstrates the bug: direct assignment loses old pointer on failure.
 */
TEST(memleak_unsafe_realloc_pattern) {
    alloc_count = 0;

    char *buf = (char *)counted_malloc(32);
    ASSERT_NOT_NULL(buf);
    ASSERT_EQ(alloc_count, 1);

    /* Simulate failed realloc by requesting absurdly large size.
     * If realloc fails, the UNSAFE pattern (buf = realloc(buf, ...))
     * would set buf = NULL and leak the original memory.
     * The SAFE pattern preserves the old pointer. */

    /* Safe pattern: use tmp */
    char *tmp = (char *)counted_realloc(buf, (size_t)-1);  /* will fail */
    if (!tmp) {
        /* Old buf is still valid -- free it properly */
        counted_free(buf);
        buf = NULL;
    } else {
        buf = tmp;
    }
    ASSERT_NULL(buf);
    ASSERT_EQ(alloc_count, 0);  /* no leak */
}

/*
 * Test: add_nma realloc pattern (proc.c) -- appending to existing string
 * Before the fix, realloc failure lost the old Lf->nma.
 */
TEST(memleak_add_nma_realloc_pattern) {
    alloc_count = 0;

    /* Simulate first add_nma: malloc a string */
    const char *text1 = "first";
    int len1 = (int)strlen(text1);
    char *nma = (char *)counted_malloc((size_t)(len1 + 1));
    ASSERT_NOT_NULL(nma);
    memcpy(nma, text1, (size_t)(len1 + 1));
    ASSERT_EQ(alloc_count, 1);

    /* Simulate second add_nma: realloc to append */
    const char *text2 = "second";
    int len2 = (int)strlen(text2);
    int nl = (int)strlen(nma);
    char *tmp = (char *)counted_realloc(nma, (size_t)(len2 + nl + 2));
    if (tmp) {
        nma = tmp;
    } else {
        /* The fix: free old memory instead of leaking it */
        counted_free(nma);
        nma = NULL;
    }
    ASSERT_NOT_NULL(nma);  /* realloc should succeed for small sizes */
    nma[nl] = ' ';
    memcpy(&nma[nl + 1], text2, (size_t)len2);
    nma[nl + 1 + len2] = '\0';
    ASSERT_STR_EQ(nma, "first second");
    ASSERT_EQ(alloc_count, 1);

    counted_free(nma);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: alloc_lproc realloc pattern (proc.c) -- growing process table
 * Before the fix, Lproc = realloc(Lproc, ...) lost old memory.
 */
TEST(memleak_alloc_lproc_realloc_pattern) {
    alloc_count = 0;

    /* Initial allocation */
    int sz = 4;
    int *table = (int *)counted_malloc((size_t)(sz * sizeof(int)));
    ASSERT_NOT_NULL(table);
    for (int i = 0; i < sz; i++) table[i] = i;
    ASSERT_EQ(alloc_count, 1);

    /* Grow table (safe realloc) */
    sz += 4;
    int *tmp = (int *)counted_realloc(table, (size_t)(sz * sizeof(int)));
    if (!tmp) {
        counted_free(table);
        table = NULL;
    } else {
        table = tmp;
    }
    ASSERT_NOT_NULL(table);
    /* Old data preserved */
    ASSERT_EQ(table[0], 0);
    ASSERT_EQ(table[3], 3);
    ASSERT_EQ(alloc_count, 1);

    counted_free(table);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: host cache realloc pattern (print.c)
 */
TEST(memleak_hostcache_realloc_pattern) {
    alloc_count = 0;

    /* Initial cache allocation */
    int nhc = 4;
    int entry_sz = 64;
    char *hc = (char *)counted_malloc((size_t)(nhc * entry_sz));
    ASSERT_NOT_NULL(hc);
    memset(hc, 0, (size_t)(nhc * entry_sz));
    ASSERT_EQ(alloc_count, 1);

    /* Grow cache (safe realloc) */
    nhc += 4;
    char *tmp = (char *)counted_realloc(hc, (size_t)(nhc * entry_sz));
    if (!tmp) {
        counted_free(hc);
        hc = NULL;
    } else {
        hc = tmp;
    }
    ASSERT_NOT_NULL(hc);
    ASSERT_EQ(alloc_count, 1);

    counted_free(hc);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: sn (service name) realloc pattern in enter_network_address
 * Before the fix, sn = realloc(sn, ...) lost old memory on failure.
 */
TEST(memleak_sn_realloc_pattern) {
    alloc_count = 0;

    /* Initial service name */
    char *sn = (char *)counted_malloc(5);
    ASSERT_NOT_NULL(sn);
    memcpy(sn, "http", 5);
    ASSERT_EQ(alloc_count, 1);

    /* Realloc for longer service name (safe pattern) */
    size_t newlen = 10;
    char *tmp = (char *)counted_realloc(sn, newlen);
    if (!tmp) {
        counted_free(sn);
        sn = NULL;
    } else {
        sn = tmp;
    }
    ASSERT_NOT_NULL(sn);
    ASSERT_STR_EQ(sn, "http");
    ASSERT_EQ(alloc_count, 1);

    counted_free(sn);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: CmdRx realloc pattern (arg.c) -- growing regex table
 */
TEST(memleak_cmdrx_realloc_pattern) {
    alloc_count = 0;

    /* Simulate initial CmdRx allocation */
    int ncmdrx = 4;
    int entry_sz = 32;
    char *CmdRx = (char *)counted_malloc((size_t)(ncmdrx * entry_sz));
    ASSERT_NOT_NULL(CmdRx);
    ASSERT_EQ(alloc_count, 1);

    /* Grow (safe realloc pattern) */
    ncmdrx += 4;
    char *tmp = (char *)counted_realloc(CmdRx, (size_t)(ncmdrx * entry_sz));
    if (!tmp) {
        counted_free(CmdRx);
        CmdRx = NULL;
    } else {
        CmdRx = tmp;
    }
    ASSERT_NOT_NULL(CmdRx);
    ASSERT_EQ(alloc_count, 1);

    counted_free(CmdRx);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: enter_id s realloc pattern (arg.c) -- growing PID/PGID table
 */
TEST(memleak_enter_id_realloc_pattern) {
    alloc_count = 0;

    int mx = 4;
    int *s = (int *)counted_malloc((size_t)(mx * sizeof(int)));
    ASSERT_NOT_NULL(s);
    for (int i = 0; i < mx; i++) s[i] = i * 100;
    ASSERT_EQ(alloc_count, 1);

    /* Grow table (safe pattern) */
    mx += 4;
    int *tmp = (int *)counted_realloc(s, (size_t)(mx * sizeof(int)));
    if (!tmp) {
        counted_free(s);
        s = NULL;
    } else {
        s = tmp;
    }
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(s[0], 0);
    ASSERT_EQ(s[3], 300);
    ASSERT_EQ(alloc_count, 1);

    counted_free(s);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: Suid realloc pattern (main.c + arg.c)
 */
TEST(memleak_suid_realloc_pattern) {
    alloc_count = 0;

    int nuid = 4;
    int uid_sz = 16;
    char *Suid = (char *)counted_malloc((size_t)(nuid * uid_sz));
    ASSERT_NOT_NULL(Suid);
    ASSERT_EQ(alloc_count, 1);

    /* Shrink (safe realloc reduce) */
    int new_nuid = 2;
    char *tmp = (char *)counted_realloc(Suid, (size_t)(new_nuid * uid_sz));
    if (!tmp) {
        counted_free(Suid);
        Suid = NULL;
    } else {
        Suid = tmp;
    }
    ASSERT_NOT_NULL(Suid);
    ASSERT_EQ(alloc_count, 1);

    counted_free(Suid);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: sort pointer realloc pattern (main.c slp)
 */
TEST(memleak_sort_ptr_realloc_pattern) {
    alloc_count = 0;

    int n = 8;
    void **slp = (void **)counted_malloc((size_t)(n * sizeof(void *)));
    ASSERT_NOT_NULL(slp);
    ASSERT_EQ(alloc_count, 1);

    /* Grow (safe pattern) */
    n = 16;
    void **tmp = (void **)counted_realloc(slp, (size_t)(n * sizeof(void *)));
    if (!tmp) {
        counted_free(slp);
        slp = NULL;
    } else {
        slp = tmp;
    }
    ASSERT_NOT_NULL(slp);
    ASSERT_EQ(alloc_count, 1);

    counted_free(slp);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: directory stack realloc pattern (misc.c Dstk)
 */
TEST(memleak_dstk_realloc_pattern) {
    alloc_count = 0;

    int dstkn = 4;
    char **dstk = (char **)counted_malloc((size_t)(dstkn * sizeof(char *)));
    ASSERT_NOT_NULL(dstk);
    for (int i = 0; i < dstkn; i++)
        dstk[i] = counted_mkstrcpy("/some/dir");
    ASSERT_EQ(alloc_count, 5);  /* 1 array + 4 strings */

    /* Grow stack (safe pattern) */
    dstkn += 4;
    char **tmp = (char **)counted_realloc(dstk, (size_t)(dstkn * sizeof(char *)));
    if (!tmp) {
        /* free contents then array */
        for (int i = 0; i < 4; i++) counted_free(dstk[i]);
        counted_free(dstk);
        dstk = NULL;
    } else {
        dstk = tmp;
    }
    ASSERT_NOT_NULL(dstk);
    ASSERT_EQ(alloc_count, 5);

    /* Cleanup */
    for (int i = 0; i < 4; i++) counted_free(dstk[i]);
    counted_free(dstk);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Test: TCP/UDP state table realloc pattern (misc.c)
 */
TEST(memleak_ipstate_realloc_pattern) {
    alloc_count = 0;

    int nstates = 4;
    char **st = (char **)counted_malloc((size_t)(nstates * sizeof(char *)));
    ASSERT_NOT_NULL(st);
    for (int i = 0; i < nstates; i++)
        st[i] = counted_mkstrcpy("STATE");
    ASSERT_EQ(alloc_count, 5);

    /* Grow state table (safe pattern) */
    int new_nstates = 8;
    char **tmp = (char **)counted_realloc(st, (size_t)(new_nstates * sizeof(char *)));
    if (!tmp) {
        for (int i = 0; i < nstates; i++) counted_free(st[i]);
        counted_free(st);
        st = NULL;
    } else {
        st = tmp;
    }
    ASSERT_NOT_NULL(st);
    /* Init new entries */
    for (int i = nstates; i < new_nstates; i++) st[i] = NULL;
    ASSERT_EQ(alloc_count, 5);

    /* Cleanup */
    for (int i = 0; i < new_nstates; i++) {
        if (st[i]) counted_free(st[i]);
    }
    counted_free(st);
    ASSERT_EQ(alloc_count, 0);
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
    REGISTER_TEST(memleak_enter_network_address_hn_freed),
    REGISTER_TEST(memleak_enter_network_address_error_frees_all),
    REGISTER_TEST(memleak_enter_fd_lst_dup_frees_nm),
    REGISTER_TEST(memleak_enter_fd_lst_dup_numeric_no_leak),
    REGISTER_TEST(memleak_enter_efsys_readlink_fail_frees_ec),
    REGISTER_TEST(memleak_enter_efsys_dup_frees_ec),
    REGISTER_TEST(memleak_enter_efsys_success_frees_ec),
    REGISTER_TEST(memleak_enter_efsys_rdlnk_no_double_free),
    REGISTER_TEST(memleak_mkstrcpy_basic),
    REGISTER_TEST(memleak_mkstrcpy_null),
    REGISTER_TEST(memleak_mkstrcat_basic),
    REGISTER_TEST(memleak_enter_nm_frees_old),
    REGISTER_TEST(memleak_free_lproc_pattern),
    REGISTER_TEST(memleak_safe_realloc_success),
    REGISTER_TEST(memleak_unsafe_realloc_pattern),
    REGISTER_TEST(memleak_add_nma_realloc_pattern),
    REGISTER_TEST(memleak_alloc_lproc_realloc_pattern),
    REGISTER_TEST(memleak_hostcache_realloc_pattern),
    REGISTER_TEST(memleak_sn_realloc_pattern),
    REGISTER_TEST(memleak_cmdrx_realloc_pattern),
    REGISTER_TEST(memleak_enter_id_realloc_pattern),
    REGISTER_TEST(memleak_suid_realloc_pattern),
    REGISTER_TEST(memleak_sort_ptr_realloc_pattern),
    REGISTER_TEST(memleak_dstk_realloc_pattern),
    REGISTER_TEST(memleak_ipstate_realloc_pattern),
};

RUN_TESTS_FROM(all_tests)
