/*
 * test_unit_x2dev.h - hex to device number conversion tests
 */

#ifndef TEST_UNIT_X2DEV_H
#define TEST_UNIT_X2DEV_H

static char *
test_hex_to_device(char *hex_input, dev_t *device_out)
{
    char *scan_pos, *prefix_check;
    int digit_count;
    dev_t accumulated;

    if (strncasecmp(hex_input, "0x", 2) == 0)
        hex_input += 2;
    for (scan_pos = hex_input, digit_count = 0; *scan_pos; scan_pos++, digit_count++) {
        if (isdigit((unsigned char)*scan_pos))
            continue;
        if ((unsigned char)*scan_pos >= 'a' && (unsigned char)*scan_pos <= 'f')
            continue;
        if ((unsigned char)*scan_pos >= 'A' && (unsigned char)*scan_pos <= 'F')
            continue;
        if (*scan_pos == ' ' || *scan_pos == ',')
            break;
        return NULL;
    }
    if (!digit_count)
        return NULL;
    if (digit_count > (2 * (int)sizeof(dev_t))) {
        prefix_check = hex_input;
        hex_input += (digit_count - (2 * sizeof(dev_t)));
        while (prefix_check < hex_input) {
            if (*prefix_check != 'f' && *prefix_check != 'F')
                return NULL;
            prefix_check++;
        }
    }
    for (accumulated = 0; hex_input < scan_pos; hex_input++) {
        accumulated = accumulated << 4;
        if (isdigit((unsigned char)*hex_input))
            accumulated |= (unsigned char)(*hex_input - '0') & 0xf;
        else {
            if (isupper((unsigned char)*hex_input))
                accumulated |= ((unsigned char)(*hex_input - 'A') + 10) & 0xf;
            else
                accumulated |= ((unsigned char)(*hex_input - 'a') + 10) & 0xf;
        }
    }
    *device_out = accumulated;
    return hex_input;
}

TEST(x2dev_simple_hex) {
    dev_t device = 0;
    char *result = test_hex_to_device("ff", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0xff);
}

TEST(x2dev_with_0x_prefix) {
    dev_t device = 0;
    char *result = test_hex_to_device("0x1a2b", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0x1a2b);
}

TEST(x2dev_with_0X_prefix) {
    dev_t device = 0;
    char *result = test_hex_to_device("0X1A2B", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0x1a2b);
}

TEST(x2dev_uppercase) {
    dev_t device = 0;
    char *result = test_hex_to_device("ABCDEF", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0xabcdef);
}

TEST(x2dev_stops_at_space) {
    dev_t device = 0;
    char *result = test_hex_to_device("0xab cd", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0xab);
    ASSERT_EQ(*result, ' ');
}

TEST(x2dev_stops_at_comma) {
    dev_t device = 0;
    char *result = test_hex_to_device("1f,rest", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0x1f);
    ASSERT_EQ(*result, ',');
}

TEST(x2dev_empty_string) {
    dev_t device = 99;
    char *result = test_hex_to_device("", &device);
    ASSERT_NULL(result);
}

TEST(x2dev_invalid_char) {
    dev_t device = 0;
    char *result = test_hex_to_device("0xGG", &device);
    ASSERT_NULL(result);
}

TEST(x2dev_just_0x) {
    dev_t device = 99;
    char *result = test_hex_to_device("0x", &device);
    ASSERT_NULL(result);
}

TEST(x2dev_zero) {
    dev_t device = 99;
    char *result = test_hex_to_device("0", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0);
}

TEST(x2dev_single_digit) {
    dev_t device = 0;
    char *result = test_hex_to_device("a", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0xa);
}

TEST(x2dev_mixed_case) {
    dev_t device = 0;
    char *result = test_hex_to_device("0xAaBbCc", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0xaabbcc);
}

TEST(x2dev_max_byte) {
    dev_t device = 0;
    char *result = test_hex_to_device("ff", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 255);
}

TEST(x2dev_leading_zeros) {
    dev_t device = 0;
    char *result = test_hex_to_device("00ff", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0xff);
}

TEST(x2dev_stops_at_null_terminator) {
    dev_t device = 0;
    char *result = test_hex_to_device("ab", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(*result, '\0');
    ASSERT_EQ(device, 0xab);
}

TEST(x2dev_two_digits) {
    dev_t device = 0;
    char *result = test_hex_to_device("1f", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0x1f);
}

TEST(x2dev_0x_with_zero) {
    dev_t device = 99;
    char *result = test_hex_to_device("0x0", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0);
}

TEST(x2dev_all_f_prefix) {
    dev_t device = 0;
    char *result = test_hex_to_device("ffffab", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0xffffab);
}

TEST(x2dev_stops_at_invalid_after_valid) {
    dev_t device = 0;
    char *result = test_hex_to_device("1f@", &device);
    ASSERT_NULL(result);
}

TEST(x2dev_consecutive_zeros) {
    dev_t device = 99;
    char *result = test_hex_to_device("0000", &device);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(device, 0);
}

#endif /* TEST_UNIT_X2DEV_H */
