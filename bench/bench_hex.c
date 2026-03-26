#include "bench.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#undef BF_SECTION
#define BF_SECTION "HEX & DEVICE PARSING"

/* ===== hex_to_device() benchmark ===== */
static char *
bench_hex_to_device(char *hex_input, dev_t *device_out)
{
    char *scan_pos, *prefix_check;
    int digit_count;
    dev_t accumulated;

    if (strncasecmp(hex_input, "0x", 2) == 0)
        hex_input += 2;
    for (scan_pos = hex_input, digit_count = 0; *scan_pos; scan_pos++, digit_count++) {
        if (isdigit((unsigned char)*scan_pos)) continue;
        if ((unsigned char)*scan_pos >= 'a' && (unsigned char)*scan_pos <= 'f') continue;
        if ((unsigned char)*scan_pos >= 'A' && (unsigned char)*scan_pos <= 'F') continue;
        if (*scan_pos == ' ' || *scan_pos == ',') break;
        return NULL;
    }
    if (!digit_count) return NULL;
    if (digit_count > (2 * (int)sizeof(dev_t))) {
        prefix_check = hex_input;
        hex_input += (digit_count - (2 * sizeof(dev_t)));
        while (prefix_check < hex_input) {
            if (*prefix_check != 'f' && *prefix_check != 'F') return NULL;
            prefix_check++;
        }
    }
    {
        static const signed char hex_values[256] = {
            ['0']=0, ['1']=1, ['2']=2, ['3']=3, ['4']=4,
            ['5']=5, ['6']=6, ['7']=7, ['8']=8, ['9']=9,
            ['a']=10,['b']=11,['c']=12,['d']=13,['e']=14,['f']=15,
            ['A']=10,['B']=11,['C']=12,['D']=13,['E']=14,['F']=15,
        };
        for (accumulated = 0; hex_input < scan_pos; hex_input++) {
            accumulated = (accumulated << 4) | (hex_values[(unsigned char)*hex_input] & 0xf);
        }
    }
    *device_out = accumulated;
    return hex_input;
}

BENCH(x2dev_short, 1000000) {
    dev_t device;
    for (int i = 0; i < bf_iters; i++) {
        bench_hex_to_device("ff", &device);
        BENCH_SINK_INT(device);
    }
}

BENCH(x2dev_with_prefix, 1000000) {
    dev_t device;
    for (int i = 0; i < bf_iters; i++) {
        bench_hex_to_device("0x1a2b3c4d", &device);
        BENCH_SINK_INT(device);
    }
}

BENCH(x2dev_long_hex, 1000000) {
    dev_t device;
    for (int i = 0; i < bf_iters; i++) {
        bench_hex_to_device("0xdeadbeefcafe1234", &device);
        BENCH_SINK_INT(device);
    }
}

/* ===== Device number decomposition benchmark ===== */
BENCH(major_minor_extract, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        dev_t d = (dev_t)(0x0802 + (i & 0xfff));
        BENCH_SINK_INT((int)(major(d) + minor(d)));
    }
}

BENCH(makedev_roundtrip, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        dev_t d = makedev(8, i & 0xff);
        BENCH_SINK_INT((int)(major(d) ^ minor(d)));
    }
}


/* ===== Hex string generation (device number to hex) ===== */
BENCH(dev_to_hex_snprintf, 5000000) {
    char buf[32];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "%x,%x", (i >> 8) & 0xff, i & 0xff);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(dev_to_hex_manual, 5000000) {
    static const char hex[] = "0123456789abcdef";
    char buf[32];
    for (int i = 0; i < bf_iters; i++) {
        int maj = (i >> 8) & 0xff;
        int min = i & 0xff;
        char *p = buf;
        if (maj >= 16) *p++ = hex[maj >> 4];
        *p++ = hex[maj & 0xf];
        *p++ = ',';
        if (min >= 16) *p++ = hex[min >> 4];
        *p++ = hex[min & 0xf];
        *p = '\0';
        BENCH_SINK_PTR(buf);
    }
}

/* ===== x2dev with max-length input ===== */
BENCH(x2dev_max_length, 1000000) {
    dev_t device;
    for (int i = 0; i < bf_iters; i++) {
        bench_hex_to_device("ffffffffffffffff", &device);
        BENCH_SINK_INT(device);
    }
}

BF_SECTIONS("HEX & DEVICE PARSING")

RUN_BENCHMARKS()
