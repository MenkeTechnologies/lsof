/*
 * test_unit_network.h - network address parsing, socket, and AF tests
 *
 * Tests for isIPv4addr, IPv6 detection, address families, socket types
 */

#ifndef TEST_UNIT_NETWORK_H
#define TEST_UNIT_NETWORK_H

#include <sys/un.h>
#include <netinet/in.h>

/* ===== isIPv4addr() reimplementation ===== */
#define TEST_MIN_AF_ADDR 4

static char *
test_is_ipv4_address(char *hostname, unsigned char *addr_out, int addr_len)
{
    int dot_count = 0;
    int octet_idx;
    int octet_values[TEST_MIN_AF_ADDR];
    int current_octet = 0;

    if ((*hostname < '0') || (*hostname > '9'))
        return NULL;
    if (!addr_out || (addr_len < TEST_MIN_AF_ADDR))
        return NULL;

    octet_values[0] = (int)(*hostname++ - '0');
    while (*hostname && (*hostname != ':')) {
        if (*hostname == '.') {
            dot_count++;
            if ((octet_values[current_octet] < 0) || (octet_values[current_octet] > 255))
                return NULL;
            if (++current_octet > (TEST_MIN_AF_ADDR - 1))
                return NULL;
            octet_values[current_octet] = -1;
        } else if ((*hostname >= '0') && (*hostname <= '9')) {
            if (octet_values[current_octet] < 0)
                octet_values[current_octet] = (int)(*hostname - '0');
            else
                octet_values[current_octet] = (octet_values[current_octet] * 10) + (int)(*hostname - '0');
        } else {
            return NULL;
        }
        hostname++;
    }
    if ((dot_count != 3) || (current_octet != (TEST_MIN_AF_ADDR - 1))
        || (octet_values[current_octet] < 0) || (octet_values[current_octet] > 255))
        return NULL;

    for (octet_idx = 0; octet_idx < TEST_MIN_AF_ADDR; octet_idx++)
        addr_out[octet_idx] = (unsigned char)octet_values[octet_idx];
    return hostname;
}

TEST(ipv4_basic) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("192.168.1.1", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 192);
    ASSERT_EQ(a[1], 168);
    ASSERT_EQ(a[2], 1);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_loopback) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("127.0.0.1", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 127);
    ASSERT_EQ(a[1], 0);
    ASSERT_EQ(a[2], 0);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_all_zeros) {
    unsigned char a[4] = {0xff, 0xff, 0xff, 0xff};
    char *result = test_is_ipv4_address("0.0.0.0", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 0);
    ASSERT_EQ(a[1], 0);
    ASSERT_EQ(a[2], 0);
    ASSERT_EQ(a[3], 0);
}

TEST(ipv4_broadcast) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("255.255.255.255", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 255);
    ASSERT_EQ(a[1], 255);
    ASSERT_EQ(a[2], 255);
    ASSERT_EQ(a[3], 255);
}

TEST(ipv4_with_port) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("10.0.0.1:8080", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(*result, ':');
    ASSERT_EQ(a[0], 10);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_stops_at_colon) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("1.2.3.4:http", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(*result, ':');
}

TEST(ipv4_too_few_octets) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("192.168.1", a, 4));
}

TEST(ipv4_too_many_octets) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("1.2.3.4.5", a, 4));
}

TEST(ipv4_octet_too_large) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("256.0.0.1", a, 4));
}

TEST(ipv4_empty) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("", a, 4));
}

TEST(ipv4_alpha) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("abc.def.ghi.jkl", a, 4));
}

TEST(ipv4_leading_alpha) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("x1.2.3.4", a, 4));
}

TEST(ipv4_null_addr) {
    ASSERT_NULL(test_is_ipv4_address("1.2.3.4", NULL, 4));
}

TEST(ipv4_short_buffer) {
    unsigned char a[2] = {0};
    ASSERT_NULL(test_is_ipv4_address("1.2.3.4", a, 2));
}

TEST(ipv4_trailing_dot) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("1.2.3.", a, 4));
}

TEST(ipv4_class_a) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("10.0.0.1", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 10);
    ASSERT_EQ(a[1], 0);
    ASSERT_EQ(a[2], 0);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_class_c) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("192.168.0.1", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 192);
    ASSERT_EQ(a[1], 168);
    ASSERT_EQ(a[2], 0);
    ASSERT_EQ(a[3], 1);
}

TEST(ipv4_single_digit_octets) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("1.2.3.4", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 1);
    ASSERT_EQ(a[1], 2);
    ASSERT_EQ(a[2], 3);
    ASSERT_EQ(a[3], 4);
}

TEST(ipv4_octet_255_boundary) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("255.0.0.255", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(a[0], 255);
    ASSERT_EQ(a[3], 255);
}

TEST(ipv4_second_octet_too_large) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("1.256.3.4", a, 4));
}

TEST(ipv4_last_octet_too_large) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("1.2.3.999", a, 4));
}

TEST(ipv4_double_dot) {
    unsigned char a[4] = {0};
    ASSERT_NULL(test_is_ipv4_address("1..2.3", a, 4));
}

TEST(ipv4_port_extraction) {
    unsigned char a[4] = {0};
    char *result = test_is_ipv4_address("172.16.0.1:443", a, 4);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(*result, ':');
    ASSERT_STR_EQ(result + 1, "443");
}


/* ===== IPv6 address detection ===== */
static int test_has_ipv6_colons(const char *s) {
    int colons = 0;
    for (; *s; s++) if (*s == ':') colons++;
    return colons >= 2;
}

TEST(ipv6_detect_full) {
    ASSERT_TRUE(test_has_ipv6_colons("2001:db8::1"));
}

TEST(ipv6_detect_loopback) {
    ASSERT_TRUE(test_has_ipv6_colons("::1"));
}

TEST(ipv6_detect_ipv4) {
    ASSERT_FALSE(test_has_ipv6_colons("192.168.1.1"));
}

TEST(ipv6_detect_single_colon) {
    ASSERT_FALSE(test_has_ipv6_colons("host:port"));
}

TEST(ipv6_detect_full_expanded) {
    ASSERT_TRUE(test_has_ipv6_colons("2001:0db8:0000:0000:0000:0000:0000:0001"));
}

TEST(ipv6_detect_link_local) {
    ASSERT_TRUE(test_has_ipv6_colons("fe80::1%eth0"));
}

TEST(ipv6_detect_all_zeros) {
    ASSERT_TRUE(test_has_ipv6_colons("::"));
}

TEST(ipv6_detect_empty) {
    ASSERT_FALSE(test_has_ipv6_colons(""));
}

TEST(ipv6_detect_no_colon) {
    ASSERT_FALSE(test_has_ipv6_colons("hostname"));
}


/* ===== Address family names ===== */
static const char *
test_af_name(int fam)
{
    switch (fam) {
    case AF_UNSPEC: return "UNSPEC";
    case AF_UNIX:   return "unix";
    case AF_INET:   return "INET";
#ifdef AF_INET6
    case AF_INET6:  return "INET6";
#endif
    default:        return NULL;
    }
}

TEST(af_name_unspec) {
    ASSERT_STR_EQ(test_af_name(AF_UNSPEC), "UNSPEC");
}

TEST(af_name_unix) {
    ASSERT_STR_EQ(test_af_name(AF_UNIX), "unix");
}

TEST(af_name_inet) {
    ASSERT_STR_EQ(test_af_name(AF_INET), "INET");
}

TEST(af_name_inet6) {
#ifdef AF_INET6
    ASSERT_STR_EQ(test_af_name(AF_INET6), "INET6");
#endif
}

TEST(af_name_unknown) {
    ASSERT_NULL(test_af_name(9999));
}


/* ===== Socket type names ===== */
static const char *test_socktype_name(int type) {
    switch (type) {
        case SOCK_STREAM: return "SOCK_STREAM";
        case SOCK_DGRAM: return "SOCK_DGRAM";
        case SOCK_RAW: return "SOCK_RAW";
        default: return NULL;
    }
}

TEST(socktype_stream) {
    ASSERT_STR_EQ(test_socktype_name(SOCK_STREAM), "SOCK_STREAM");
}

TEST(socktype_dgram) {
    ASSERT_STR_EQ(test_socktype_name(SOCK_DGRAM), "SOCK_DGRAM");
}

TEST(socktype_raw) {
    ASSERT_STR_EQ(test_socktype_name(SOCK_RAW), "SOCK_RAW");
}

TEST(socktype_unknown) {
    ASSERT_NULL(test_socktype_name(9999));
}

#endif /* TEST_UNIT_NETWORK_H */
