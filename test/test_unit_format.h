/*
 * test_unit_format.h - output formatting tests
 *
 * Tests for snpf, fmtnum, fmtstr, print_kptr
 */

#ifndef TEST_UNIT_FORMAT_H
#define TEST_UNIT_FORMAT_H

/* ===== snpf() reimplementation ===== */
static int test_snpf(char *buf, int count, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, (size_t)count, fmt, ap);
    va_end(ap);
    return len;
}

TEST(snpf_basic) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "hello %s", "world");
    ASSERT_STR_EQ(buf, "hello world");
}

TEST(snpf_truncation) {
    char buf[6];
    test_snpf(buf, sizeof(buf), "hello world");
    ASSERT_STR_EQ(buf, "hello");
}

TEST(snpf_zero_buffer) {
    char buf[1] = {'X'};
    int len = test_snpf(buf, 0, "hello world");
    ASSERT_GE(len, 11);
    ASSERT_EQ(buf[0], 'X');
}

TEST(snpf_integer) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%d", 42);
    ASSERT_STR_EQ(buf, "42");
}

TEST(snpf_hex) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "0x%x", 0xdead);
    ASSERT_STR_EQ(buf, "0xdead");
}

TEST(snpf_multiple_args) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%s:%d", "tcp", 80);
    ASSERT_STR_EQ(buf, "tcp:80");
}

TEST(snpf_empty_format) {
    char buf[64];
    buf[0] = 'X';
    test_snpf(buf, sizeof(buf), "");
    ASSERT_STR_EQ(buf, "");
}

TEST(snpf_percent_literal) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%%");
    ASSERT_STR_EQ(buf, "%");
}

TEST(snpf_long_integer) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%ld", (long)1234567890L);
    ASSERT_STR_EQ(buf, "1234567890");
}

TEST(snpf_negative_integer) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%d", -999);
    ASSERT_STR_EQ(buf, "-999");
}

TEST(snpf_unsigned) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%u", 4294967295U);
    ASSERT_STR_EQ(buf, "4294967295");
}

TEST(snpf_octal) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%o", 255);
    ASSERT_STR_EQ(buf, "377");
}

TEST(snpf_char) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%c", 'A');
    ASSERT_STR_EQ(buf, "A");
}

TEST(snpf_width_right) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%10d", 42);
    ASSERT_STR_EQ(buf, "        42");
}

TEST(snpf_width_left) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%-10d!", 42);
    ASSERT_STR_EQ(buf, "42        !");
}

TEST(snpf_zero_pad) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%06d", 42);
    ASSERT_STR_EQ(buf, "000042");
}

TEST(snpf_string_precision) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "%.5s", "hello world");
    ASSERT_STR_EQ(buf, "hello");
}

TEST(snpf_combined_format) {
    char buf[64];
    test_snpf(buf, sizeof(buf), "[%s] pid=%d port=%d", "tcp", 1234, 80);
    ASSERT_STR_EQ(buf, "[tcp] pid=1234 port=80");
}

TEST(snpf_exact_fit) {
    char buf[12];
    test_snpf(buf, sizeof(buf), "hello world");
    ASSERT_STR_EQ(buf, "hello world");
}

TEST(snpf_one_byte_buffer) {
    char buf[1];
    test_snpf(buf, sizeof(buf), "hello");
    ASSERT_STR_EQ(buf, "");
}


/* ===== fmtnum() reimplementation ===== */
static int
test_fmtnum(char *buf, int bufsz, long value, int base,
            int dosign, int ljust, int width, int zpad)
{
    char tmp[64];
    int neg = 0;
    int pos = 0;
    unsigned long uval;

    if (dosign && value < 0) {
        neg = 1;
        uval = (unsigned long)(-value);
    } else {
        uval = (unsigned long)value;
    }

    if (uval == 0) {
        tmp[pos++] = '0';
    } else {
        while (uval > 0) {
            int digit = (int)(uval % (unsigned long)base);
            tmp[pos++] = (digit < 10) ? (char)('0' + digit) : (char)('a' + digit - 10);
            uval /= (unsigned long)base;
        }
    }

    int numlen = pos + neg;
    int padlen = (width > numlen) ? width - numlen : 0;

    char *out = buf;
    char *end = buf + bufsz - 1;

    if (!ljust && !zpad) {
        for (int i = 0; i < padlen && out < end; i++) *out++ = ' ';
    }
    if (neg && out < end) *out++ = '-';
    if (!ljust && zpad) {
        for (int i = 0; i < padlen && out < end; i++) *out++ = '0';
    }
    for (int i = pos - 1; i >= 0 && out < end; i--) *out++ = tmp[i];
    if (ljust) {
        for (int i = 0; i < padlen && out < end; i++) *out++ = ' ';
    }
    *out = '\0';
    return (int)(out - buf);
}

TEST(fmtnum_decimal_positive) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 42, 10, 1, 0, 0, 0);
    ASSERT_STR_EQ(buf, "42");
}

TEST(fmtnum_decimal_negative) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), -42, 10, 1, 0, 0, 0);
    ASSERT_STR_EQ(buf, "-42");
}

TEST(fmtnum_decimal_zero) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 0, 10, 1, 0, 0, 0);
    ASSERT_STR_EQ(buf, "0");
}

TEST(fmtnum_hex) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 255, 16, 0, 0, 0, 0);
    ASSERT_STR_EQ(buf, "ff");
}

TEST(fmtnum_octal) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 8, 8, 0, 0, 0, 0);
    ASSERT_STR_EQ(buf, "10");
}

TEST(fmtnum_width_right_justify) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 42, 10, 1, 0, 6, 0);
    ASSERT_STR_EQ(buf, "    42");
}

TEST(fmtnum_width_left_justify) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 42, 10, 1, 1, 6, 0);
    ASSERT_STR_EQ(buf, "42    ");
}

TEST(fmtnum_width_zero_pad) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 42, 10, 1, 0, 6, 1);
    ASSERT_STR_EQ(buf, "000042");
}

TEST(fmtnum_neg_with_zero_pad) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), -7, 10, 1, 0, 5, 1);
    ASSERT_STR_EQ(buf, "-0007");
}

TEST(fmtnum_hex_large) {
    char buf[64];
    test_fmtnum(buf, sizeof(buf), 0xDEAD, 16, 0, 0, 0, 0);
    ASSERT_STR_EQ(buf, "dead");
}


/* ===== fmtstr() reimplementation ===== */
static int
test_fmtstr(char *buf, int bufsz, const char *value, int ljust, int width, int maxwidth)
{
    if (!value) value = "<NULL>";
    int slen = (int)strlen(value);
    if (maxwidth > 0 && slen > maxwidth) slen = maxwidth;
    int padlen = (width > slen) ? width - slen : 0;

    char *out = buf;
    char *end = buf + bufsz - 1;

    if (!ljust) {
        for (int i = 0; i < padlen && out < end; i++) *out++ = ' ';
    }
    for (int i = 0; i < slen && out < end; i++) *out++ = value[i];
    if (ljust) {
        for (int i = 0; i < padlen && out < end; i++) *out++ = ' ';
    }
    *out = '\0';
    return (int)(out - buf);
}

TEST(fmtstr_basic) {
    char buf[64];
    test_fmtstr(buf, sizeof(buf), "hello", 0, 0, 0);
    ASSERT_STR_EQ(buf, "hello");
}

TEST(fmtstr_null_becomes_label) {
    char buf[64];
    test_fmtstr(buf, sizeof(buf), NULL, 0, 0, 0);
    ASSERT_STR_EQ(buf, "<NULL>");
}

TEST(fmtstr_right_justify) {
    char buf[64];
    test_fmtstr(buf, sizeof(buf), "hi", 0, 8, 0);
    ASSERT_STR_EQ(buf, "      hi");
}

TEST(fmtstr_left_justify) {
    char buf[64];
    test_fmtstr(buf, sizeof(buf), "hi", 1, 8, 0);
    ASSERT_STR_EQ(buf, "hi      ");
}

TEST(fmtstr_maxwidth_truncate) {
    char buf[64];
    test_fmtstr(buf, sizeof(buf), "hello world", 0, 0, 5);
    ASSERT_STR_EQ(buf, "hello");
}

TEST(fmtstr_empty) {
    char buf[64];
    test_fmtstr(buf, sizeof(buf), "", 0, 0, 0);
    ASSERT_STR_EQ(buf, "");
}

TEST(fmtstr_width_with_truncate) {
    char buf[64];
    test_fmtstr(buf, sizeof(buf), "longstring", 0, 15, 4);
    ASSERT_STR_EQ(buf, "           long");
}


/* ===== print_kptr() reimplementation ===== */
static char kptr_static_buf[32];

static char *
test_print_kptr(unsigned long kp, char *buf, size_t bufl)
{
    if (!buf) {
        buf = kptr_static_buf;
        bufl = sizeof(kptr_static_buf);
    }
    if (kp == 0) {
        snprintf(buf, bufl, "(none)");
    } else {
        snprintf(buf, bufl, "0x%lx", kp);
    }
    return buf;
}

TEST(kptr_nonzero) {
    char buf[32];
    char *r = test_print_kptr(0xdeadbeef, buf, sizeof(buf));
    ASSERT_STR_EQ(r, "0xdeadbeef");
    ASSERT_TRUE(r == buf);
}

TEST(kptr_zero) {
    char buf[32];
    char *r = test_print_kptr(0, buf, sizeof(buf));
    ASSERT_STR_EQ(r, "(none)");
}

TEST(kptr_null_buf_uses_static) {
    char *r = test_print_kptr(0x1234, NULL, 0);
    ASSERT_STR_EQ(r, "0x1234");
    ASSERT_TRUE(r == kptr_static_buf);
}

TEST(kptr_small_buf_truncates) {
    char buf[8];
    test_print_kptr(0xdeadbeef, buf, sizeof(buf));
    ASSERT_EQ(strlen(buf), 7);
}

#endif /* TEST_UNIT_FORMAT_H */
