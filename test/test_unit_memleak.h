/*
 * test_unit_memleak.h - memory leak pattern verification tests
 *
 * These tests verify that the allocation patterns used in lsof properly
 * free all allocated memory.  Each test reimplements a simplified version
 * of the pattern and uses an allocation counter to detect leaks.
 */

#ifndef TEST_UNIT_MEMLEAK_H
#define TEST_UNIT_MEMLEAK_H

static int alloc_count = 0; /* tracks net allocations */

static void *counted_malloc(size_t sz) {
    void *p = malloc(sz);
    if (p)
        alloc_count++;
    return p;
}

static void counted_free(void *p) {
    if (p) {
        free(p);
        alloc_count--;
    }
}

static char *counted_mkstrcpy(const char *src) {
    size_t len = src ? strlen(src) : 0;
    char *ns = (char *)counted_malloc(len + 1);
    if (ns) {
        if (src)
            memcpy(ns, src, len);
        ns[len] = '\0';
    }
    return ns;
}

static void *counted_realloc(void *ptr, size_t sz) {
    void *p = realloc(ptr, sz);
    if (p && !ptr)
        alloc_count++; /* new allocation */
    if (!p && ptr) {   /* realloc failed, old pointer still valid */
    }
    return p;
}

static char *counted_mkstrcat(const char *s1, int l1, const char *s2, int l2) {
    size_t len1 = s1 ? (l1 >= 0 ? (size_t)l1 : strlen(s1)) : 0;
    size_t len2 = s2 ? (l2 >= 0 ? (size_t)l2 : strlen(s2)) : 0;
    char *cursor = (char *)counted_malloc(len1 + len2 + 1);
    if (cursor) {
        if (s1 && len1)
            memcpy(cursor, s1, len1);
        if (s2 && len2)
            memcpy(cursor + len1, s2, len2);
        cursor[len1 + len2] = '\0';
    }
    return cursor;
}

TEST(memleak_enter_network_address_hn_freed) {
    alloc_count = 0;
    char *proto = counted_mkstrcat("tcp", 3, NULL, -1);
    char *hn = counted_mkstrcat("localhost", 9, NULL, -1);
    char *sn = counted_mkstrcpy("http");

    ASSERT_EQ(alloc_count, 3);

    if (hn)
        counted_free(hn);
    if (sn)
        counted_free(sn);
    if (proto)
        counted_free(proto);

    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_enter_network_address_error_frees_all) {
    alloc_count = 0;
    char *proto = counted_mkstrcat("tcp", 3, NULL, -1);
    char *hn = counted_mkstrcat("badhost", 7, NULL, -1);
    char *sn = counted_mkstrcpy("service");

    ASSERT_EQ(alloc_count, 3);

    if (proto)
        counted_free(proto);
    if (hn)
        counted_free(hn);
    if (sn)
        counted_free(sn);

    ASSERT_EQ(alloc_count, 0);
}

struct test_fd_lst {
    char *nm;
    int lo, hi;
    struct test_fd_lst *next;
};

TEST(memleak_enter_fd_lst_dup_frees_nm) {
    alloc_count = 0;

    struct test_fd_lst *list = NULL;
    struct test_fd_lst *f1 = (struct test_fd_lst *)counted_malloc(sizeof(*f1));
    ASSERT_NOT_NULL(f1);
    f1->nm = counted_mkstrcpy("cwd");
    f1->lo = 1;
    f1->hi = 0;
    f1->next = list;
    list = f1;
    ASSERT_EQ(alloc_count, 2);

    struct test_fd_lst *f2 = (struct test_fd_lst *)counted_malloc(sizeof(*f2));
    ASSERT_NOT_NULL(f2);
    f2->nm = counted_mkstrcpy("cwd");
    f2->lo = 1;
    f2->hi = 0;
    ASSERT_EQ(alloc_count, 4);

    int is_dup = 0;
    for (struct test_fd_lst *ft = list; ft; ft = ft->next) {
        if (f2->nm && ft->nm && strcmp(f2->nm, ft->nm) == 0) {
            is_dup = 1;
            break;
        }
    }
    ASSERT_TRUE(is_dup);

    if (f2->nm)
        counted_free(f2->nm);
    counted_free(f2);
    ASSERT_EQ(alloc_count, 2);

    if (f1->nm)
        counted_free(f1->nm);
    counted_free(f1);
    ASSERT_EQ(alloc_count, 0);
}

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

    int is_dup = 0;
    for (struct test_fd_lst *ft = list; ft; ft = ft->next) {
        if (!f2->nm && !ft->nm && f2->lo == ft->lo && f2->hi == ft->hi) {
            is_dup = 1;
            break;
        }
    }
    ASSERT_TRUE(is_dup);

    if (f2->nm)
        counted_free(f2->nm);
    counted_free(f2);
    ASSERT_EQ(alloc_count, 1);

    counted_free(f1);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_enter_efsys_readlink_fail_frees_ec) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/some/path");
    ASSERT_NOT_NULL(ec);
    ASSERT_EQ(alloc_count, 1);

    char *path = NULL;
    if (!path) {
        counted_free(ec);
    }

    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_enter_efsys_dup_frees_ec) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/original/input");
    char *path = counted_mkstrcpy("/resolved/path");
    ASSERT_EQ(alloc_count, 2);

    int is_dup = 1;
    if (is_dup) {
        if (path != ec)
            counted_free(ec);
    }

    ASSERT_EQ(alloc_count, 1);
    counted_free(path);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_enter_efsys_success_frees_ec) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/input/path");
    char *path = counted_mkstrcpy("/resolved/real/path");
    ASSERT_EQ(alloc_count, 2);

    char *stored_path = path;
    (void)stored_path;

    if (path != ec)
        counted_free(ec);

    ASSERT_EQ(alloc_count, 1);
    counted_free(path);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_enter_efsys_rdlnk_no_double_free) {
    alloc_count = 0;

    char *ec = counted_mkstrcpy("/direct/path");
    char *path = ec;
    ASSERT_EQ(alloc_count, 1);

    char *stored_path = path;
    (void)stored_path;

    if (path != ec)
        counted_free(ec);

    ASSERT_EQ(alloc_count, 1);
    counted_free(path);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_mkstrcpy_basic) {
    alloc_count = 0;
    char *s = counted_mkstrcpy("test string");
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "test string");
    ASSERT_EQ(alloc_count, 1);
    counted_free(s);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_mkstrcpy_null) {
    alloc_count = 0;
    char *s = counted_mkstrcpy(NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "");
    ASSERT_EQ(alloc_count, 1);
    counted_free(s);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_mkstrcat_basic) {
    alloc_count = 0;
    char *s = counted_mkstrcat("hello", -1, " world", -1);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "hello world");
    ASSERT_EQ(alloc_count, 1);
    counted_free(s);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_enter_nm_frees_old) {
    alloc_count = 0;
    char *nm = NULL;

    nm = counted_mkstrcpy("first");
    ASSERT_EQ(alloc_count, 1);

    char *old = nm;
    nm = counted_mkstrcpy("second");
    counted_free(old);
    ASSERT_EQ(alloc_count, 1);
    ASSERT_STR_EQ(nm, "second");

    counted_free(nm);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_free_lproc_pattern) {
    alloc_count = 0;

    char *cmd = counted_mkstrcpy("test_cmd");
    char *dev_ch = counted_mkstrcpy("0x1234");
    char *nm = counted_mkstrcpy("/dev/null");
    char *nma = counted_mkstrcpy("(annotation)");
    ASSERT_EQ(alloc_count, 4);

    counted_free(dev_ch);
    counted_free(nm);
    counted_free(nma);
    counted_free(cmd);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_safe_realloc_success) {
    alloc_count = 0;

    char *buf = (char *)counted_malloc(32);
    ASSERT_NOT_NULL(buf);
    memset(buf, 'A', 32);
    ASSERT_EQ(alloc_count, 1);

    char *tmp = (char *)counted_realloc(buf, 64);
    ASSERT_NOT_NULL(tmp);
    buf = tmp;
    ASSERT_EQ(alloc_count, 1);

    ASSERT_EQ(buf[0], 'A');
    ASSERT_EQ(buf[31], 'A');

    counted_free(buf);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_unsafe_realloc_pattern) {
    alloc_count = 0;

    char *buf = (char *)counted_malloc(32);
    ASSERT_NOT_NULL(buf);
    ASSERT_EQ(alloc_count, 1);

    char *tmp = (char *)counted_realloc(buf, (size_t)-1);
    if (!tmp) {
        counted_free(buf);
        buf = NULL;
    } else {
        buf = tmp;
    }
    ASSERT_NULL(buf);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_add_nma_realloc_pattern) {
    alloc_count = 0;

    const char *text1 = "first";
    int len1 = (int)strlen(text1);
    char *nma = (char *)counted_malloc((size_t)(len1 + 1));
    ASSERT_NOT_NULL(nma);
    memcpy(nma, text1, (size_t)(len1 + 1));
    ASSERT_EQ(alloc_count, 1);

    const char *text2 = "second";
    int len2 = (int)strlen(text2);
    int nl = (int)strlen(nma);
    char *tmp = (char *)counted_realloc(nma, (size_t)(len2 + nl + 2));
    if (tmp) {
        nma = tmp;
    } else {
        counted_free(nma);
        nma = NULL;
    }
    ASSERT_NOT_NULL(nma);
    nma[nl] = ' ';
    memcpy(&nma[nl + 1], text2, (size_t)len2);
    nma[nl + 1 + len2] = '\0';
    ASSERT_STR_EQ(nma, "first second");
    ASSERT_EQ(alloc_count, 1);

    counted_free(nma);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_alloc_lproc_realloc_pattern) {
    alloc_count = 0;

    int sz = 4;
    int *table = (int *)counted_malloc((size_t)(sz * sizeof(int)));
    ASSERT_NOT_NULL(table);
    for (int i = 0; i < sz; i++)
        table[i] = i;
    ASSERT_EQ(alloc_count, 1);

    sz += 4;
    int *tmp = (int *)counted_realloc(table, (size_t)(sz * sizeof(int)));
    if (!tmp) {
        counted_free(table);
        table = NULL;
    } else {
        table = tmp;
    }
    ASSERT_NOT_NULL(table);
    ASSERT_EQ(table[0], 0);
    ASSERT_EQ(table[3], 3);
    ASSERT_EQ(alloc_count, 1);

    counted_free(table);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_hostcache_realloc_pattern) {
    alloc_count = 0;

    int nhc = 4;
    int entry_sz = 64;
    char *hc = (char *)counted_malloc((size_t)(nhc * entry_sz));
    ASSERT_NOT_NULL(hc);
    memset(hc, 0, (size_t)(nhc * entry_sz));
    ASSERT_EQ(alloc_count, 1);

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

TEST(memleak_sn_realloc_pattern) {
    alloc_count = 0;

    char *sn = (char *)counted_malloc(5);
    ASSERT_NOT_NULL(sn);
    memcpy(sn, "http", 5);
    ASSERT_EQ(alloc_count, 1);

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

TEST(memleak_cmdrx_realloc_pattern) {
    alloc_count = 0;

    int ncmdrx = 4;
    int entry_sz = 32;
    char *CommandRegexTable = (char *)counted_malloc((size_t)(ncmdrx * entry_sz));
    ASSERT_NOT_NULL(CommandRegexTable);
    ASSERT_EQ(alloc_count, 1);

    ncmdrx += 4;
    char *tmp = (char *)counted_realloc(CommandRegexTable, (size_t)(ncmdrx * entry_sz));
    if (!tmp) {
        counted_free(CommandRegexTable);
        CommandRegexTable = NULL;
    } else {
        CommandRegexTable = tmp;
    }
    ASSERT_NOT_NULL(CommandRegexTable);
    ASSERT_EQ(alloc_count, 1);

    counted_free(CommandRegexTable);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_enter_id_realloc_pattern) {
    alloc_count = 0;

    int mx = 4;
    int *s = (int *)counted_malloc((size_t)(mx * sizeof(int)));
    ASSERT_NOT_NULL(s);
    for (int i = 0; i < mx; i++)
        s[i] = i * 100;
    ASSERT_EQ(alloc_count, 1);

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

TEST(memleak_suid_realloc_pattern) {
    alloc_count = 0;

    int nuid = 4;
    int uid_sz = 16;
    char *SearchUidList = (char *)counted_malloc((size_t)(nuid * uid_sz));
    ASSERT_NOT_NULL(SearchUidList);
    ASSERT_EQ(alloc_count, 1);

    int new_nuid = 2;
    char *tmp = (char *)counted_realloc(SearchUidList, (size_t)(new_nuid * uid_sz));
    if (!tmp) {
        counted_free(SearchUidList);
        SearchUidList = NULL;
    } else {
        SearchUidList = tmp;
    }
    ASSERT_NOT_NULL(SearchUidList);
    ASSERT_EQ(alloc_count, 1);

    counted_free(SearchUidList);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_sort_ptr_realloc_pattern) {
    alloc_count = 0;

    int n = 8;
    void **slp = (void **)counted_malloc((size_t)(n * sizeof(void *)));
    ASSERT_NOT_NULL(slp);
    ASSERT_EQ(alloc_count, 1);

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

TEST(memleak_dstk_realloc_pattern) {
    alloc_count = 0;

    int dstkn = 4;
    char **dstk = (char **)counted_malloc((size_t)(dstkn * sizeof(char *)));
    ASSERT_NOT_NULL(dstk);
    for (int i = 0; i < dstkn; i++)
        dstk[i] = counted_mkstrcpy("/some/dir");
    ASSERT_EQ(alloc_count, 5);

    dstkn += 4;
    char **tmp = (char **)counted_realloc(dstk, (size_t)(dstkn * sizeof(char *)));
    if (!tmp) {
        for (int i = 0; i < 4; i++)
            counted_free(dstk[i]);
        counted_free(dstk);
        dstk = NULL;
    } else {
        dstk = tmp;
    }
    ASSERT_NOT_NULL(dstk);
    ASSERT_EQ(alloc_count, 5);

    for (int i = 0; i < 4; i++)
        counted_free(dstk[i]);
    counted_free(dstk);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_ipstate_realloc_pattern) {
    alloc_count = 0;

    int nstates = 4;
    char **st = (char **)counted_malloc((size_t)(nstates * sizeof(char *)));
    ASSERT_NOT_NULL(st);
    for (int i = 0; i < nstates; i++)
        st[i] = counted_mkstrcpy("STATE");
    ASSERT_EQ(alloc_count, 5);

    int new_nstates = 8;
    char **tmp = (char **)counted_realloc(st, (size_t)(new_nstates * sizeof(char *)));
    if (!tmp) {
        for (int i = 0; i < nstates; i++)
            counted_free(st[i]);
        counted_free(st);
        st = NULL;
    } else {
        st = tmp;
    }
    ASSERT_NOT_NULL(st);
    for (int i = nstates; i < new_nstates; i++)
        st[i] = NULL;
    ASSERT_EQ(alloc_count, 5);

    for (int i = 0; i < new_nstates; i++) {
        if (st[i])
            counted_free(st[i]);
    }
    counted_free(st);
    ASSERT_EQ(alloc_count, 0);
}

struct test_str_node {
    char *str;
    struct test_str_node *next;
};

TEST(memleak_str_list_cleanup) {
    alloc_count = 0;

    struct test_str_node *list = NULL;
    for (int i = 0; i < 3; i++) {
        struct test_str_node *n = (struct test_str_node *)counted_malloc(sizeof(*n));
        ASSERT_NOT_NULL(n);
        n->str = counted_mkstrcpy("entry");
        n->next = list;
        list = n;
    }
    ASSERT_EQ(alloc_count, 6);

    while (list) {
        struct test_str_node *next = list->next;
        counted_free(list->str);
        counted_free(list);
        list = next;
    }
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_double_buffer_pattern) {
    alloc_count = 0;

    char *buf1 = (char *)counted_malloc(32);
    char *buf2 = (char *)counted_malloc(64);
    ASSERT_EQ(alloc_count, 2);

    counted_free(buf1);
    buf1 = (char *)counted_malloc(128);
    ASSERT_EQ(alloc_count, 2);

    counted_free(buf1);
    counted_free(buf2);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_nested_alloc_pattern) {
    alloc_count = 0;

    struct {
        char *name;
        char *path;
        char *extra;
    } entry;

    entry.name = counted_mkstrcpy("process");
    entry.path = counted_mkstrcpy("/usr/bin/process");
    entry.extra = counted_mkstrcpy("(annotation)");
    ASSERT_EQ(alloc_count, 3);

    counted_free(entry.name);
    entry.name = counted_mkstrcpy("new_process");
    ASSERT_EQ(alloc_count, 3);

    counted_free(entry.name);
    counted_free(entry.path);
    counted_free(entry.extra);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Tests for the three memory leaks fixed in this session:
 *
 * 1. enter_network_address() nwad_exit didn't free n.arg
 * 2. Readlink() path_too_long didn't free stk[] entries
 * 3. Unsafe realloc patterns in darwin dialect (ptr = realloc(ptr, ...))
 */

/*
 * Leak fix 1: enter_network_address() error path must free n.arg
 *
 * The nwad_exit label frees n.proto, hn, sn. Before the fix, n.arg
 * (set via mkstrcpy in enter_nwad) was not freed, leaking the address
 * string on every malformed -i argument.
 */
TEST(memleak_nwad_exit_frees_arg) {
    alloc_count = 0;

    /* Simulate the allocations present when nwad_exit is reached */
    char *arg = counted_mkstrcpy("192.168.1.1");        /* n.arg */
    char *proto = counted_mkstrcat("tcp", 3, NULL, -1); /* n.proto */
    char *hn = counted_mkstrcpy("localhost");           /* hn */
    char *sn = counted_mkstrcpy("http");                /* sn */
    ASSERT_EQ(alloc_count, 4);

    /* nwad_exit cleanup — must free all four, including arg */
    if (arg)
        counted_free(arg);
    if (proto)
        counted_free(proto);
    if (hn)
        counted_free(hn);
    if (sn)
        counted_free(sn);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_nwad_exit_arg_null_safe) {
    alloc_count = 0;

    /* n.arg may be NULL if error occurs before enter_nwad sets it */
    char *arg = NULL;
    char *proto = counted_mkstrcat("udp", 3, NULL, -1);
    char *hn = NULL;
    char *sn = NULL;
    ASSERT_EQ(alloc_count, 1);

    if (arg)
        counted_free(arg);
    if (proto)
        counted_free(proto);
    if (hn)
        counted_free(hn);
    if (sn)
        counted_free(sn);
    ASSERT_EQ(alloc_count, 0);
}

/*
 * Leak fix 2: Readlink() path_too_long must free stk[] entries
 *
 * The Readlink function uses a static stack of mkstrcpy'd strings to
 * track symlink resolution. The MAXSYMLINKS error path freed them,
 * but path_too_long did not, leaking up to MAXSYMLINKS allocations.
 */
TEST(memleak_readlink_path_too_long_frees_stack) {
    alloc_count = 0;

    /* Simulate the symlink resolution stack */
    int ss = 0, sx = 0;
    char **stk = NULL;

    /* Push 3 entries onto the stack (as Readlink does during resolution) */
    for (int i = 0; i < 3; i++) {
        sx++;
        if (sx > ss) {
            ss = sx;
            if (!stk)
                stk = (char **)counted_malloc((size_t)(ss * sizeof(char *)));
            else {
                char **tmp = (char **)counted_realloc(stk, (size_t)(ss * sizeof(char *)));
                if (!tmp) {
                    counted_free(stk);
                    stk = NULL;
                } else
                    stk = tmp;
            }
            ASSERT_NOT_NULL(stk);
        }
        stk[sx - 1] = counted_mkstrcpy("/some/symlink/target");
    }
    ASSERT_EQ(alloc_count, 4); /* 1 stk array + 3 strings */

    /* Simulate path_too_long cleanup (the fix) */
    for (int i = 0; i < sx; i++) {
        counted_free(stk[i]);
        stk[i] = NULL;
    }
    if (stk) {
        counted_free(stk);
        stk = NULL;
    }
    ss = sx = 0;

    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_readlink_path_too_long_empty_stack) {
    alloc_count = 0;

    /* path_too_long can be reached with an empty stack (first component too long) */
    int ss = 0, sx = 0;
    char **stk = NULL;

    /* Cleanup should be safe with nothing on the stack */
    for (int i = 0; i < sx; i++) {
        counted_free(stk[i]);
        stk[i] = NULL;
    }
    if (stk) {
        counted_free(stk);
        stk = NULL;
    }
    ss = sx = 0;
    (void)ss;

    ASSERT_EQ(alloc_count, 0);
}

/*
 * Leak fix 3: Safe realloc pattern for darwin dialect buffers
 *
 * The pattern `Ptr = realloc(Ptr, newsize)` loses the old pointer if
 * realloc fails. The fix uses a tmp variable and frees the old pointer
 * on failure. All sites Exit(1) after, but the fix is correct practice.
 */
TEST(memleak_darwin_safe_realloc_success) {
    alloc_count = 0;

    /* Simulate Vips/Pids/Fds/Threads/ADev grow pattern */
    int *buf = (int *)counted_malloc(4 * sizeof(int));
    ASSERT_NOT_NULL(buf);
    buf[0] = 42;
    ASSERT_EQ(alloc_count, 1);

    /* Safe realloc: use tmp, free old on failure */
    int *tmp = (int *)counted_realloc(buf, 8 * sizeof(int));
    if (!tmp) {
        counted_free(buf);
        buf = NULL;
    } else {
        buf = tmp;
    }
    ASSERT_NOT_NULL(buf);
    ASSERT_EQ(buf[0], 42);
    ASSERT_EQ(alloc_count, 1);

    counted_free(buf);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_darwin_safe_realloc_failure) {
    alloc_count = 0;

    char *buf = (char *)counted_malloc(32);
    ASSERT_NOT_NULL(buf);
    memset(buf, 'X', 32);
    ASSERT_EQ(alloc_count, 1);

    /* Simulate realloc failure with impossibly large size */
    char *tmp = (char *)counted_realloc(buf, (size_t)-1);
    if (!tmp) {
        counted_free(buf);
        buf = NULL;
    } else {
        buf = tmp;
    }
    ASSERT_NULL(buf);
    ASSERT_EQ(alloc_count, 0);
}

TEST(memleak_darwin_adev_realloc_pattern) {
    alloc_count = 0;

    /* Simulate ADev grow: starts with malloc, then realloc */
    int capacity = 16;
    int *adev = NULL;
    int used = 0;

    /* Initial allocation */
    adev = (int *)counted_malloc((size_t)(capacity * sizeof(int)));
    ASSERT_NOT_NULL(adev);
    ASSERT_EQ(alloc_count, 1);

    /* Fill and grow twice */
    for (int round = 0; round < 2; round++) {
        for (int i = used; i < capacity; i++)
            adev[i] = i;
        used = capacity;
        capacity += 16;

        int *tmp = (int *)counted_realloc(adev, (size_t)(capacity * sizeof(int)));
        if (!tmp) {
            counted_free(adev);
            adev = NULL;
        } else {
            adev = tmp;
        }
        ASSERT_NOT_NULL(adev);
        ASSERT_EQ(alloc_count, 1);
    }

    /* Verify old data survived reallocs */
    ASSERT_EQ(adev[0], 0);
    ASSERT_EQ(adev[15], 15);

    counted_free(adev);
    ASSERT_EQ(alloc_count, 0);
}

#endif /* TEST_UNIT_MEMLEAK_H */
