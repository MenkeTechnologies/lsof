#include "bench.h"

#include <ctype.h>

#undef BF_SECTION
#define BF_SECTION "STRING SAFETY"

/* ===== safe_string_length benchmark ===== */
static int
bench_safe_string_length(char *string_ptr, int flags)
{
    char non_print_marker;
    int safe_len = 0;
    non_print_marker = (flags & 2) ? ' ' : '\0';
    if (string_ptr) {
        for (; *string_ptr; string_ptr++) {
            if (!isprint((unsigned char)*string_ptr) || *string_ptr == non_print_marker) {
                if (*string_ptr < 0x20 || (unsigned char)*string_ptr == 0xff)
                    safe_len += 2;
                else
                    safe_len += 4;
            } else
                safe_len++;
        }
    }
    return safe_len;
}

BENCH(safestrlen_short, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safe_string_length("hello world", 0));
    }
}

BENCH(safestrlen_with_escapes, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safe_string_length("hello\tworld\n", 0));
    }
}

BENCH(safestrlen_long_path, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safe_string_length(
            "/usr/local/share/applications/very/long/path/to/some/file.txt", 0));
    }
}

BENCH(safestrlen_binary_data, 2000000) {
    char data[64];
    for (int i = 0; i < 63; i++) data[i] = (char)(i + 1);
    data[63] = '\0';
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_safe_string_length(data, 0));
    }
}

/* ===== safe_print_unprintable benchmark ===== */
static char unprintable_buf[8];
static char *
bench_safe_print_unprintable(unsigned int char_code, int *output_length)
{
    int encoded_len;
    char *result_str;
    if (char_code < 0x20) {
        switch (char_code) {
        case '\b': result_str = "\\b"; break;
        case '\f': result_str = "\\f"; break;
        case '\n': result_str = "\\n"; break;
        case '\r': result_str = "\\r"; break;
        case '\t': result_str = "\\t"; break;
        default:
            unprintable_buf[0] = '^';
            unprintable_buf[1] = (char)(char_code + 0x40);
            unprintable_buf[2] = '\0';
            result_str = unprintable_buf;
        }
        encoded_len = 2;
    } else if (char_code == 0xff) {
        result_str = "^?";
        encoded_len = 2;
    } else {
        {
            static const char hex_digits[] = "0123456789abcdef";
            unsigned int byte_val = char_code & 0xff;
            unprintable_buf[0] = '\\';
            unprintable_buf[1] = 'x';
            unprintable_buf[2] = hex_digits[(byte_val >> 4) & 0xf];
            unprintable_buf[3] = hex_digits[byte_val & 0xf];
            unprintable_buf[4] = '\0';
        }
        result_str = unprintable_buf;
        encoded_len = 4;
    }
    if (output_length) *output_length = encoded_len;
    return result_str;
}

BENCH(safepup_control_chars, 10000000) {
    int char_len;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_PTR(bench_safe_print_unprintable((unsigned int)(i % 0x20), &char_len));
    }
}

BENCH(safepup_high_bytes, 5000000) {
    int char_len;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_PTR(bench_safe_print_unprintable(0x80 + (unsigned int)(i % 0x7f), &char_len));
    }
}


BF_SECTIONS("STRING SAFETY")

RUN_BENCHMARKS()
