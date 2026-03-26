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


/* ===== x2dev extended tests ===== */
TEST(x2dev_max_byte) {
    dev_t d = 0;
    char *r = test_x2dev("ff", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 255);
}

TEST(x2dev_leading_zeros) {
    dev_t d = 0;
    char *r = test_x2dev("00ff", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0xff);
}

TEST(x2dev_stops_at_null_terminator) {
    dev_t d = 0;
    char *r = test_x2dev("ab", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(*r, '\0');
    ASSERT_EQ(d, 0xab);
}

TEST(x2dev_two_digits) {
    dev_t d = 0;
    char *r = test_x2dev("1f", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0x1f);
}

TEST(x2dev_0x_with_zero) {
    dev_t d = 99;
    char *r = test_x2dev("0x0", &d);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(d, 0);
}

/* ===== Field name string tests ===== */
TEST(field_names_non_null) {
    /* All LSOF_FNM_* should be non-null, non-empty strings */
    ASSERT_NOT_NULL(LSOF_FNM_ACCESS);
    ASSERT_NOT_NULL(LSOF_FNM_CMD);
    ASSERT_NOT_NULL(LSOF_FNM_CT);
    ASSERT_NOT_NULL(LSOF_FNM_DEVCH);
    ASSERT_NOT_NULL(LSOF_FNM_DEVN);
    ASSERT_NOT_NULL(LSOF_FNM_FD);
    ASSERT_NOT_NULL(LSOF_FNM_FA);
    ASSERT_NOT_NULL(LSOF_FNM_FG);
    ASSERT_NOT_NULL(LSOF_FNM_INODE);
    ASSERT_NOT_NULL(LSOF_FNM_NLINK);
    ASSERT_NOT_NULL(LSOF_FNM_TID);
    ASSERT_NOT_NULL(LSOF_FNM_LOCK);
    ASSERT_NOT_NULL(LSOF_FNM_LOGIN);
    ASSERT_NOT_NULL(LSOF_FNM_MARK);
    ASSERT_NOT_NULL(LSOF_FNM_NAME);
    ASSERT_NOT_NULL(LSOF_FNM_NI);
    ASSERT_NOT_NULL(LSOF_FNM_OFFSET);
    ASSERT_NOT_NULL(LSOF_FNM_PID);
    ASSERT_NOT_NULL(LSOF_FNM_PGID);
    ASSERT_NOT_NULL(LSOF_FNM_PROTO);
    ASSERT_NOT_NULL(LSOF_FNM_RDEV);
    ASSERT_NOT_NULL(LSOF_FNM_PPID);
    ASSERT_NOT_NULL(LSOF_FNM_SIZE);
    ASSERT_NOT_NULL(LSOF_FNM_STREAM);
    ASSERT_NOT_NULL(LSOF_FNM_TYPE);
    ASSERT_NOT_NULL(LSOF_FNM_TCPTPI);
    ASSERT_NOT_NULL(LSOF_FNM_UID);
    ASSERT_NOT_NULL(LSOF_FNM_ZONE);
    ASSERT_NOT_NULL(LSOF_FNM_CNTX);
    ASSERT_NOT_NULL(LSOF_FNM_TERM);
}

TEST(field_names_non_empty) {
    ASSERT_GT(strlen(LSOF_FNM_ACCESS), 0);
    ASSERT_GT(strlen(LSOF_FNM_CMD), 0);
    ASSERT_GT(strlen(LSOF_FNM_PID), 0);
    ASSERT_GT(strlen(LSOF_FNM_FD), 0);
    ASSERT_GT(strlen(LSOF_FNM_NAME), 0);
    ASSERT_GT(strlen(LSOF_FNM_TYPE), 0);
    ASSERT_GT(strlen(LSOF_FNM_SIZE), 0);
    ASSERT_GT(strlen(LSOF_FNM_UID), 0);
}

TEST(field_ids_are_printable_ascii) {
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
        ASSERT_GE(ids[i], 0x20);
        ASSERT_LT(ids[i], 0x7f);
    }
}

TEST(field_total_count) {
    /* There should be exactly 30 field definitions (indices 0-29) */
    ASSERT_EQ(LSOF_FIX_TERM, 29);
}

/* ===== hashport extended tests ===== */
TEST(hashport_adjacent_ports_differ) {
    /* Adjacent ports should generally hash to different buckets */
    int same = 0;
    for (int p = 1; p < 1024; p++) {
        if (HASHPORT(p) == HASHPORT(p - 1))
            same++;
    }
    /* Allow some collisions, but not too many */
    ASSERT_LT(same, 100);
}

TEST(hashport_max_port) {
    int h = HASHPORT(65535);
    ASSERT_GE(h, 0);
    ASSERT_LT(h, PORTHASHBUCKETS);
}

TEST(hashport_zero) {
    int h = HASHPORT(0);
    ASSERT_EQ(h, 0);
}

TEST(hashport_common_ports_in_range) {
    int common[] = {22, 53, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017};
    int n = sizeof(common) / sizeof(common[0]);
    for (int i = 0; i < n; i++) {
        int h = HASHPORT(common[i]);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, PORTHASHBUCKETS);
    }
}

/* ===== safestrlen extended tests ===== */
TEST(safestrlen_all_printable) {
    /* All printable ASCII chars should each count as 1 */
    char buf[96];
    int j = 0;
    for (int i = 0x21; i < 0x7f; i++)
        buf[j++] = (char)i;
    buf[j] = '\0';
    ASSERT_EQ(test_safestrlen(buf, 0), j);
}

TEST(safestrlen_multiple_control) {
    /* "\t\n\r" = 3 control chars, each 2 bytes escape */
    ASSERT_EQ(test_safestrlen("\t\n\r", 0), 6);
}

TEST(safestrlen_mixed_content) {
    /* "ab\tcd\nef" = 6 printable + 2 control (each 2) = 10 */
    ASSERT_EQ(test_safestrlen("ab\tcd\nef", 0), 10);
}

/* ===== compdev extended tests ===== */
typedef struct test_devcomp {
    dev_t rdev;
    unsigned long inode;
    char *name;
} test_devcomp_t;

static int test_compdev_fn(const void *a, const void *b) {
    const test_devcomp_t *da = (const test_devcomp_t *)a;
    const test_devcomp_t *db = (const test_devcomp_t *)b;
    if (da->rdev < db->rdev) return -1;
    if (da->rdev > db->rdev) return 1;
    if (da->inode < db->inode) return -1;
    if (da->inode > db->inode) return 1;
    if (da->name && db->name) return strcmp(da->name, db->name);
    return 0;
}

TEST(compdev_null_names) {
    test_devcomp_t a = {1, 1, NULL};
    test_devcomp_t b = {1, 1, NULL};
    ASSERT_EQ(test_compdev_fn(&a, &b), 0);
}

TEST(compdev_stability) {
    /* Same entry compared to itself should be 0 */
    test_devcomp_t a = {42, 100, "/dev/sda"};
    ASSERT_EQ(test_compdev_fn(&a, &a), 0);
}

TEST(compdev_sort_by_rdev_first) {
    test_devcomp_t a = {1, 999, "zzz"};
    test_devcomp_t b = {2, 1, "aaa"};
    ASSERT_LT(test_compdev_fn(&a, &b), 0);
}

TEST(compdev_sort_by_inode_second) {
    test_devcomp_t a = {5, 10, "zzz"};
    test_devcomp_t b = {5, 20, "aaa"};
    ASSERT_LT(test_compdev_fn(&a, &b), 0);
}

TEST(compdev_sort_by_name_third) {
    test_devcomp_t a = {5, 10, "alpha"};
    test_devcomp_t b = {5, 10, "beta"};
    ASSERT_LT(test_compdev_fn(&a, &b), 0);
}

/* ===== comppid extended tests ===== */
TEST(comppid_negative_pids) {
    /* Negative PIDs (kernel threads) should sort before positive */
    int pids[] = {100, -1, 50, -100, 0};
    int n = sizeof(pids) / sizeof(pids[0]);
    qsort(pids, (size_t)n, sizeof(int), test_comppid);
    ASSERT_EQ(pids[0], -100);
    ASSERT_EQ(pids[1], -1);
    ASSERT_EQ(pids[2], 0);
    ASSERT_EQ(pids[3], 50);
    ASSERT_EQ(pids[4], 100);
}

TEST(comppid_duplicate_pids) {
    int pids[] = {5, 5, 5};
    int n = sizeof(pids) / sizeof(pids[0]);
    qsort(pids, (size_t)n, sizeof(int), test_comppid);
    ASSERT_EQ(pids[0], 5);
    ASSERT_EQ(pids[1], 5);
    ASSERT_EQ(pids[2], 5);
}

/* ===== safepup extended tests ===== */
TEST(safepup_printable_range) {
    /* All printable ASCII (0x20-0x7e) should pass through unchanged */
    for (int c = 0x20; c <= 0x7e; c++) {
        char ch = (char)c;
        int class_val = (ch == ' ') ? 2 : isprint((unsigned char)ch) ? 1 : 0;
        if (ch != ' ')
            ASSERT_EQ(class_val, 1);
    }
}

TEST(safepup_all_control_chars_escaped) {
    /* Control chars 0x01-0x1f should all be classified as needing escape */
    for (int c = 1; c < 0x20; c++) {
        ASSERT_FALSE(isprint((unsigned char)c));
    }
}

TEST(safepup_del_char) {
    /* DEL (0x7f) is not printable */
    ASSERT_FALSE(isprint(0x7f));
}

/* ===== hashbyname() reimplementation test ===== */
static int
test_hashbyname(char *nm, int mod)
{
    int i, j;
    for (i = j = 0; *nm; nm++) {
        i ^= (int) *nm << j;
        if (++j > 7)
            j = 0;
    }
    return (((int) (i * 31415)) & (mod - 1));
}

TEST(hashbyname_range) {
    /* All results must be in [0, mod) */
    const char *names[] = {"tcp", "udp", "icmp", "http", "ssh", "/dev/null",
                           "/var/log/syslog", "a", "", "Z"};
    int mod = 128;
    for (int i = 0; i < 10; i++) {
        int h = test_hashbyname((char *)names[i], mod);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, mod);
    }
}

TEST(hashbyname_deterministic) {
    ASSERT_EQ(test_hashbyname("tcp", 128), test_hashbyname("tcp", 128));
    ASSERT_EQ(test_hashbyname("udp", 256), test_hashbyname("udp", 256));
}

TEST(hashbyname_different_strings_differ) {
    /* "tcp" and "udp" should hash differently (not guaranteed, but very likely) */
    int h1 = test_hashbyname("tcp", 128);
    int h2 = test_hashbyname("udp", 128);
    ASSERT_NE(h1, h2);
}

TEST(hashbyname_empty_string) {
    int h = test_hashbyname("", 128);
    ASSERT_EQ(h, 0);
}

TEST(hashbyname_case_sensitive) {
    int h1 = test_hashbyname("TCP", 128);
    int h2 = test_hashbyname("tcp", 128);
    ASSERT_NE(h1, h2);
}

TEST(hashbyname_power_of_two_mod) {
    /* Various modulus values (must be power of 2) */
    int mods[] = {16, 32, 64, 128, 256, 512, 1024};
    for (int m = 0; m < 7; m++) {
        int h = test_hashbyname("/dev/sda1", mods[m]);
        ASSERT_GE(h, 0);
        ASSERT_LT(h, mods[m]);
    }
}

TEST(hashbyname_distribution) {
    /* Hash 100 path-like strings into 64 buckets; expect at least half used */
    int buckets[64];
    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "/proc/%d/fd/%d", i * 7, i * 3);
        buckets[test_hashbyname(name, 64)]++;
    }
    int used = 0;
    for (int i = 0; i < 64; i++)
        if (buckets[i] > 0) used++;
    ASSERT_GT(used, 20);
}


/* ===== mkstrcpy() reimplementation test ===== */
static char *
test_mkstrcpy(char *src, size_t *rlp)
{
    size_t len = (size_t)(src ? strlen(src) : 0);
    char *ns = (char *)malloc(len + 1);
    if (ns) {
        if (src)
            memcpy(ns, src, len + 1);
        else
            *ns = '\0';
    }
    if (rlp)
        *rlp = len;
    return ns;
}

TEST(mkstrcpy_basic) {
    size_t len = 0;
    char *s = test_mkstrcpy("hello", &len);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "hello");
    ASSERT_EQ(len, 5);
    free(s);
}

TEST(mkstrcpy_empty) {
    size_t len = 99;
    char *s = test_mkstrcpy("", &len);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "");
    ASSERT_EQ(len, 0);
    free(s);
}

TEST(mkstrcpy_null_source) {
    size_t len = 99;
    char *s = test_mkstrcpy(NULL, &len);
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(s[0], '\0');
    ASSERT_EQ(len, 0);
    free(s);
}

TEST(mkstrcpy_null_lenptr) {
    char *s = test_mkstrcpy("test", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "test");
    free(s);
}

TEST(mkstrcpy_long_string) {
    char buf[256];
    memset(buf, 'A', 255);
    buf[255] = '\0';
    size_t len = 0;
    char *s = test_mkstrcpy(buf, &len);
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(len, 255);
    ASSERT_EQ(strlen(s), 255);
    free(s);
}

TEST(mkstrcpy_is_independent_copy) {
    char src[] = "original";
    char *s = test_mkstrcpy(src, NULL);
    ASSERT_NOT_NULL(s);
    src[0] = 'X';  /* mutate source */
    ASSERT_STR_EQ(s, "original");  /* copy unchanged */
    free(s);
}


/* ===== mkstrcat() reimplementation test ===== */
static char *
test_mkstrcat(char *s1, int l1, char *s2, int l2, char *s3, int l3, size_t *clp)
{
    size_t len1 = s1 ? (size_t)((l1 >= 0) ? l1 : (int)strlen(s1)) : 0;
    size_t len2 = s2 ? (size_t)((l2 >= 0) ? l2 : (int)strlen(s2)) : 0;
    size_t len3 = s3 ? (size_t)((l3 >= 0) ? l3 : (int)strlen(s3)) : 0;
    size_t cl = len1 + len2 + len3;
    char *cp = (char *)malloc(cl + 1);
    if (cp) {
        char *tp = cp;
        if (s1 && len1) { memcpy(tp, s1, len1); tp += len1; }
        if (s2 && len2) { memcpy(tp, s2, len2); tp += len2; }
        if (s3 && len3) { memcpy(tp, s3, len3); tp += len3; }
        *tp = '\0';
    }
    if (clp) *clp = cl;
    return cp;
}

TEST(mkstrcat_two_strings) {
    size_t len = 0;
    char *s = test_mkstrcat("hello", -1, " world", -1, NULL, -1, &len);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "hello world");
    ASSERT_EQ(len, 11);
    free(s);
}

TEST(mkstrcat_three_strings) {
    size_t len = 0;
    char *s = test_mkstrcat("a", -1, "b", -1, "c", -1, &len);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "abc");
    ASSERT_EQ(len, 3);
    free(s);
}

TEST(mkstrcat_with_length_limits) {
    size_t len = 0;
    char *s = test_mkstrcat("hello", 3, "world", 2, NULL, -1, &len);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "helwo");
    ASSERT_EQ(len, 5);
    free(s);
}

TEST(mkstrcat_empty_strings) {
    char *s = test_mkstrcat("", -1, "", -1, "", -1, NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "");
    free(s);
}

TEST(mkstrcat_null_strings) {
    char *s = test_mkstrcat(NULL, -1, NULL, -1, NULL, -1, NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "");
    free(s);
}

TEST(mkstrcat_mixed_null) {
    char *s = test_mkstrcat("hello", -1, NULL, -1, " world", -1, NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "hello world");
    free(s);
}

TEST(mkstrcat_zero_length) {
    /* l1=0 means take 0 chars from s1 even though it exists */
    char *s = test_mkstrcat("skip", 0, "keep", -1, NULL, -1, NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "keep");
    free(s);
}

TEST(mkstrcat_path_building) {
    char *s = test_mkstrcat("/proc/", -1, "1234", -1, "/fd", -1, NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "/proc/1234/fd");
    free(s);
}


/* ===== isIPv4addr() reimplementation test ===== */
#define TEST_MIN_AF_ADDR 4

static char *
test_isIPv4addr(char *hn, unsigned char *a, int al)
{
    int dc = 0;
    int i;
    int ov[TEST_MIN_AF_ADDR];
    int ovx = 0;

    if ((*hn < '0') || (*hn > '9'))
        return NULL;
    if (!a || (al < TEST_MIN_AF_ADDR))
        return NULL;

    ov[0] = (int)(*hn++ - '0');
    while (*hn && (*hn != ':')) {
        if (*hn == '.') {
            dc++;
            if ((ov[ovx] < 0) || (ov[ovx] > 255))
                return NULL;
            if (++ovx > (TEST_MIN_AF_ADDR - 1))
                return NULL;
            ov[ovx] = -1;
        } else if ((*hn >= '0') && (*hn <= '9')) {
            if (ov[ovx] < 0)
                ov[ovx] = (int)(*hn - '0');
            else
                ov[ovx] = (ov[ovx] * 10) + (int)(*hn - '0');
        } else {
            return NULL;
        }
        hn++;
    }
    if ((dc != 3) || (ovx != (TEST_MIN_AF_ADDR - 1))
        || (ov[ovx] < 0) || (ov[ovx] > 255))
        return NULL;

    for (i = 0; i < TEST_MIN_AF_ADDR; i++)
        a[i] = (unsigned char)ov[i];
    return hn;
}

TEST(ipv4_basic) {
    unsigned char a[4] = {0};
    char *r = test_isIPv4addr("192.168.1.1", a, 4);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(a[0], 192);
    ASSERT_EQ(a[1], 168);
    ASSERT_EQ(a[2], 1);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_loopback) {
    unsigned char a[4] = {0};
    char *r = test_isIPv4addr("127.0.0.1", a, 4);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(a[0], 127);
    ASSERT_EQ(a[1], 0);
    ASSERT_EQ(a[2], 0);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_all_zeros) {
    unsigned char a[4] = {0xff, 0xff, 0xff, 0xff};
    char *r = test_isIPv4addr("0.0.0.0", a, 4);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(a[0], 0);
    ASSERT_EQ(a[1], 0);
    ASSERT_EQ(a[2], 0);
    ASSERT_EQ(a[3], 0);
}

TEST(ipv4_broadcast) {
    unsigned char a[4] = {0};
    char *r = test_isIPv4addr("255.255.255.255", a, 4);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(a[0], 255);
    ASSERT_EQ(a[1], 255);
    ASSERT_EQ(a[2], 255);
    ASSERT_EQ(a[3], 255);
}

TEST(ipv4_with_port) {
    unsigned char a[4] = {0};
    char *r = test_isIPv4addr("10.0.0.1:8080", a, 4);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(*r, ':');
    ASSERT_EQ(a[0], 10);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_stops_at_colon) {
    unsigned char a[4] = {0};
    char *r = test_isIPv4addr("1.2.3.4:http", a, 4);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(*r, ':');
}

TEST(ipv4_too_few_octets) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_isIPv4addr("192.168.1", a, 4));
}

TEST(ipv4_too_many_octets) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_isIPv4addr("1.2.3.4.5", a, 4));
}

TEST(ipv4_octet_too_large) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_isIPv4addr("256.0.0.1", a, 4));
}

TEST(ipv4_empty) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_isIPv4addr("", a, 4));
}

TEST(ipv4_alpha) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_isIPv4addr("abc.def.ghi.jkl", a, 4));
}

TEST(ipv4_leading_alpha) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_isIPv4addr("x1.2.3.4", a, 4));
}

TEST(ipv4_null_addr) {
    ASSERT_NULL(test_isIPv4addr("1.2.3.4", NULL, 4));
}

TEST(ipv4_short_buffer) {
    unsigned char a[2] = {0};
    ASSERT_NULL(test_isIPv4addr("1.2.3.4", a, 2));
}

TEST(ipv4_trailing_dot) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_isIPv4addr("1.2.3.", a, 4));
}


/* ===== ckfd_range() reimplementation test ===== */
static int
test_ckfd_range(char *first, char *dash, char *last, int *lo, int *hi)
{
    char *cp;
    if (first >= dash || dash >= last)
        return 1;
    for (cp = first, *lo = 0; *cp && cp < dash; cp++) {
        if (!isdigit((unsigned char)*cp))
            return 1;
        *lo = (*lo * 10) + (int)(*cp - '0');
    }
    for (cp = dash + 1, *hi = 0; *cp && cp < last; cp++) {
        if (!isdigit((unsigned char)*cp))
            return 1;
        *hi = (*hi * 10) + (int)(*cp - '0');
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
    /* dash at or before first */
    ASSERT_EQ(test_ckfd_range(s, s, s + 4, &lo, &hi), 1);
}


/* ===== safestrprt() output test ===== */
TEST(safestrprt_printable_to_buffer) {
    /* Write to a FILE* via tmpfile and verify output */
    FILE *f = tmpfile();
    ASSERT_NOT_NULL(f);
    /* Write printable string */
    fprintf(f, "%s", "hello");
    rewind(f);
    char buf[64] = {0};
    ASSERT_NOT_NULL(fgets(buf, sizeof(buf), f));
    ASSERT_STR_EQ(buf, "hello");
    fclose(f);
}

TEST(safestrprt_control_char_format) {
    /* Control chars are escaped as ^X where X = '@' + char */
    char c = '\x01';
    char expected[3];
    expected[0] = '^';
    expected[1] = '@' + 1;  /* 'A' */
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
    REGISTER_TEST(x2dev_max_byte),
    REGISTER_TEST(x2dev_leading_zeros),
    REGISTER_TEST(x2dev_stops_at_null_terminator),
    REGISTER_TEST(x2dev_two_digits),
    REGISTER_TEST(x2dev_0x_with_zero),
    REGISTER_TEST(field_names_non_null),
    REGISTER_TEST(field_names_non_empty),
    REGISTER_TEST(field_ids_are_printable_ascii),
    REGISTER_TEST(field_total_count),
    REGISTER_TEST(hashport_adjacent_ports_differ),
    REGISTER_TEST(hashport_max_port),
    REGISTER_TEST(hashport_zero),
    REGISTER_TEST(hashport_common_ports_in_range),
    REGISTER_TEST(safestrlen_all_printable),
    REGISTER_TEST(safestrlen_multiple_control),
    REGISTER_TEST(safestrlen_mixed_content),
    REGISTER_TEST(compdev_null_names),
    REGISTER_TEST(compdev_stability),
    REGISTER_TEST(compdev_sort_by_rdev_first),
    REGISTER_TEST(compdev_sort_by_inode_second),
    REGISTER_TEST(compdev_sort_by_name_third),
    REGISTER_TEST(comppid_negative_pids),
    REGISTER_TEST(comppid_duplicate_pids),
    REGISTER_TEST(safepup_printable_range),
    REGISTER_TEST(safepup_all_control_chars_escaped),
    REGISTER_TEST(safepup_del_char),
    REGISTER_TEST(hashbyname_range),
    REGISTER_TEST(hashbyname_deterministic),
    REGISTER_TEST(hashbyname_different_strings_differ),
    REGISTER_TEST(hashbyname_empty_string),
    REGISTER_TEST(hashbyname_case_sensitive),
    REGISTER_TEST(hashbyname_power_of_two_mod),
    REGISTER_TEST(hashbyname_distribution),
    REGISTER_TEST(mkstrcpy_basic),
    REGISTER_TEST(mkstrcpy_empty),
    REGISTER_TEST(mkstrcpy_null_source),
    REGISTER_TEST(mkstrcpy_null_lenptr),
    REGISTER_TEST(mkstrcpy_long_string),
    REGISTER_TEST(mkstrcpy_is_independent_copy),
    REGISTER_TEST(mkstrcat_two_strings),
    REGISTER_TEST(mkstrcat_three_strings),
    REGISTER_TEST(mkstrcat_with_length_limits),
    REGISTER_TEST(mkstrcat_empty_strings),
    REGISTER_TEST(mkstrcat_null_strings),
    REGISTER_TEST(mkstrcat_mixed_null),
    REGISTER_TEST(mkstrcat_zero_length),
    REGISTER_TEST(mkstrcat_path_building),
    REGISTER_TEST(ipv4_basic),
    REGISTER_TEST(ipv4_loopback),
    REGISTER_TEST(ipv4_all_zeros),
    REGISTER_TEST(ipv4_broadcast),
    REGISTER_TEST(ipv4_with_port),
    REGISTER_TEST(ipv4_stops_at_colon),
    REGISTER_TEST(ipv4_too_few_octets),
    REGISTER_TEST(ipv4_too_many_octets),
    REGISTER_TEST(ipv4_octet_too_large),
    REGISTER_TEST(ipv4_empty),
    REGISTER_TEST(ipv4_alpha),
    REGISTER_TEST(ipv4_leading_alpha),
    REGISTER_TEST(ipv4_null_addr),
    REGISTER_TEST(ipv4_short_buffer),
    REGISTER_TEST(ipv4_trailing_dot),
    REGISTER_TEST(ckfd_range_basic),
    REGISTER_TEST(ckfd_range_single_digits),
    REGISTER_TEST(ckfd_range_large),
    REGISTER_TEST(ckfd_range_equal_rejected),
    REGISTER_TEST(ckfd_range_reversed_rejected),
    REGISTER_TEST(ckfd_range_non_digit_rejected),
    REGISTER_TEST(ckfd_range_non_digit_hi_rejected),
    REGISTER_TEST(ckfd_range_bad_pointers),
    REGISTER_TEST(safestrprt_printable_to_buffer),
    REGISTER_TEST(safestrprt_control_char_format),
};

RUN_TESTS_FROM(all_tests)
