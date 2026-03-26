/*
 * bench_core.c - benchmarks for lsof core operations
 *
 * Measures performance of key internal algorithms and operations.
 */

#include "bench.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../src/lsof_fields.h"

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


/* ===== HASHPORT benchmark ===== */
#define PORTHASHBUCKETS 128
#define HASHPORT(p) (((((int)(p)) * 31415) >> 3) & (PORTHASHBUCKETS - 1))

BENCH(hashport_all_ports, 100) {
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (int p = 0; p < 65536; p++) {
            sum += HASHPORT(p);
        }
        BENCH_SINK_INT(sum);
    }
}

BENCH(hashport_common_ports, 10000000) {
    int common[] = {22, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017, 53};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(HASHPORT(common[i % 10]));
    }
}


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


/* ===== compdev (qsort) benchmark ===== */
typedef unsigned long bench_INODETYPE;
struct bench_l_dev {
    dev_t rdev;
    bench_INODETYPE inode;
    char *name;
    int v;
};

static int
bench_compare_devices(const void *first, const void *second)
{
    struct bench_l_dev **dev_ptr1 = (struct bench_l_dev **)first;
    struct bench_l_dev **dev_ptr2 = (struct bench_l_dev **)second;
    if ((*dev_ptr1)->rdev < (*dev_ptr2)->rdev) return -1;
    if ((*dev_ptr1)->rdev > (*dev_ptr2)->rdev) return 1;
    if ((*dev_ptr1)->inode < (*dev_ptr2)->inode) return -1;
    if ((*dev_ptr1)->inode > (*dev_ptr2)->inode) return 1;
    return strcmp((*dev_ptr1)->name, (*dev_ptr2)->name);
}

BENCH(compdev_sort_100, 100000) {
    static struct bench_l_dev devs[100];
    static struct bench_l_dev *ptrs[100];
    static char names[100][16];
    static int initialized = 0;

    if (!initialized) {
        for (int i = 0; i < 100; i++) {
            snprintf(names[i], 16, "dev%03d", i);
            devs[i].rdev = (dev_t)(99 - i);
            devs[i].inode = (bench_INODETYPE)(1000 + (i * 7) % 100);
            devs[i].name = names[i];
        }
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        for (int i = 0; i < 100; i++) ptrs[i] = &devs[i];
        qsort(ptrs, 100, sizeof(struct bench_l_dev *), bench_compare_devices);
        BENCH_SINK_PTR(ptrs[0]);
    }
}

BENCH(compdev_sort_1000, 10000) {
    static struct bench_l_dev devs[1000];
    static struct bench_l_dev *ptrs[1000];
    static char names[1000][16];
    static int initialized = 0;

    if (!initialized) {
        for (int i = 0; i < 1000; i++) {
            snprintf(names[i], 16, "dev%04d", i);
            devs[i].rdev = (dev_t)(999 - i);
            devs[i].inode = (bench_INODETYPE)(10000 + (i * 37) % 1000);
            devs[i].name = names[i];
        }
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        for (int i = 0; i < 1000; i++) ptrs[i] = &devs[i];
        qsort(ptrs, 1000, sizeof(struct bench_l_dev *), bench_compare_devices);
        BENCH_SINK_PTR(ptrs[0]);
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


/* ===== I/O benchmarks ===== */
BENCH(open_close_file, 100000) {
    for (int i = 0; i < bf_iters; i++) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd >= 0) close(fd);
        BENCH_SINK_INT(fd);
    }
}

BENCH(stat_file, 100000) {
    struct stat st;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(stat("/dev/null", &st));
    }
}

BENCH(getpid_call, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(getpid());
    }
}


/* ===== String operation benchmarks (common lsof patterns) ===== */
BENCH(strlen_short, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strlen("hello"));
    }
}

BENCH(strlen_path, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strlen("/usr/local/share/applications/file.txt"));
    }
}

BENCH(strcmp_match, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strcmp("TCP", "TCP"));
    }
}

BENCH(strcmp_mismatch, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strcmp("TCP", "UDP"));
    }
}

BENCH(snprintf_format, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "0x%08x", i);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(isprint_scan, 5000000) {
    char str[] = "Hello, World! This is a test string 12345";
    for (int j = 0; j < bf_iters; j++) {
        int count = 0;
        for (char *p = str; *p; p++) {
            if (isprint((unsigned char)*p)) count++;
        }
        BENCH_SINK_INT(count);
    }
}


/* ===== Socket creation benchmark (relevant to lsof socket detection) ===== */
BENCH(socket_create_close, 50000) {
    for (int i = 0; i < bf_iters; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd >= 0) close(fd);
        BENCH_SINK_INT(fd);
    }
}


/* ===== Memory allocation benchmarks ===== */
BENCH(malloc_free_small, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(64);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BENCH(malloc_free_medium, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(4096);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BENCH(realloc_grow, 1000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(64);
        p = realloc(p, 128);
        p = realloc(p, 256);
        p = realloc(p, 512);
        BENCH_SINK_PTR(p);
        free(p);
    }
}

BENCH(safe_realloc_pattern, 1000000) {
    for (int i = 0; i < bf_iters; i++) {
        void *p = malloc(64);
        void *tmp = realloc(p, 128);
        if (tmp) p = tmp;
        tmp = realloc(p, 256);
        if (tmp) p = tmp;
        tmp = realloc(p, 512);
        if (tmp) p = tmp;
        BENCH_SINK_PTR(p);
        free(p);
    }
}


/* ===== String copy benchmarks (mkstrcpy pattern) ===== */
static char *bench_make_string_copy(const char *source) {
    size_t length = source ? strlen(source) : 0;
    char *new_str = (char *)malloc(length + 1);
    if (new_str) {
        if (source) memcpy(new_str, source, length + 1);
        else *new_str = '\0';
    }
    return new_str;
}

BENCH(mkstrcpy_short, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *copy = bench_make_string_copy("hello");
        BENCH_SINK_PTR(copy);
        free(copy);
    }
}

BENCH(mkstrcpy_path, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *copy = bench_make_string_copy("/usr/local/share/applications/very/long/path/to/some/file.txt");
        BENCH_SINK_PTR(copy);
        free(copy);
    }
}

BENCH(mkstrcpy_null, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *copy = bench_make_string_copy(NULL);
        BENCH_SINK_PTR(copy);
        free(copy);
    }
}


/* ===== Linked list traversal benchmark (common lsof pattern) ===== */
struct bench_node {
    int value;
    struct bench_node *next;
};

BENCH(linked_list_traverse_100, 1000000) {
    static struct bench_node nodes[100];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 99; i++) {
            nodes[i].value = i;
            nodes[i].next = &nodes[i + 1];
        }
        nodes[99].value = 99;
        nodes[99].next = NULL;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (struct bench_node *node = &nodes[0]; node; node = node->next)
            sum += node->value;
        BENCH_SINK_INT(sum);
    }
}

BENCH(linked_list_traverse_1000, 100000) {
    static struct bench_node nodes[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 999; i++) {
            nodes[i].value = i;
            nodes[i].next = &nodes[i + 1];
        }
        nodes[999].value = 999;
        nodes[999].next = NULL;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (struct bench_node *node = &nodes[0]; node; node = node->next)
            sum += node->value;
        BENCH_SINK_INT(sum);
    }
}


/* ===== Hash table lookup benchmark (port hash pattern) ===== */
struct bench_porttab {
    int port;
    char *name;
    struct bench_porttab *next;
};

BENCH(hash_lookup_hit, 5000000) {
    static struct bench_porttab entries[10];
    static struct bench_porttab *buckets[PORTHASHBUCKETS];
    static int initialized = 0;
    static int ports[] = {22, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017, 53};

    if (!initialized) {
        memset(buckets, 0, sizeof(buckets));
        for (int i = 0; i < 10; i++) {
            entries[i].port = ports[i];
            entries[i].name = "service";
            int hash_bucket = HASHPORT(ports[i]);
            entries[i].next = buckets[hash_bucket];
            buckets[hash_bucket] = &entries[i];
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int port_num = ports[i % 10];
        int hash_bucket = HASHPORT(port_num);
        struct bench_porttab *port_entry;
        for (port_entry = buckets[hash_bucket]; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == port_num) break;
        }
        BENCH_SINK_PTR(port_entry);
    }
}

BENCH(hash_lookup_miss, 5000000) {
    static struct bench_porttab entries[10];
    static struct bench_porttab *buckets[PORTHASHBUCKETS];
    static int initialized = 0;
    static int ports[] = {22, 80, 443, 8080, 8443, 3306, 5432, 6379, 27017, 53};

    if (!initialized) {
        memset(buckets, 0, sizeof(buckets));
        for (int i = 0; i < 10; i++) {
            entries[i].port = ports[i];
            entries[i].name = "service";
            int hash_bucket = HASHPORT(ports[i]);
            entries[i].next = buckets[hash_bucket];
            buckets[hash_bucket] = &entries[i];
        }
        initialized = 1;
    }
    /* Look up ports that don't exist */
    for (int i = 0; i < bf_iters; i++) {
        int port_num = 60000 + (i % 1000);
        int hash_bucket = HASHPORT(port_num);
        struct bench_porttab *port_entry;
        for (port_entry = buckets[hash_bucket]; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == port_num) break;
        }
        BENCH_SINK_PTR(port_entry);
    }
}


/* ===== PID binary search benchmark (compare_pids / qsort + bsearch) ===== */
static int bench_compare_pids(const void *first, const void *second) {
    int pid_a = *(const int *)first;
    int pid_b = *(const int *)second;
    if (pid_a < pid_b) return -1;
    if (pid_a > pid_b) return 1;
    return 0;
}

BENCH(pid_sort_1000, 50000) {
    static int pids[1000];
    for (int j = 0; j < bf_iters; j++) {
        for (int i = 0; i < 1000; i++) pids[i] = 1000 - i;
        qsort(pids, 1000, sizeof(int), bench_compare_pids);
        BENCH_SINK_INT(pids[0]);
    }
}

BENCH(pid_bsearch_1000, 5000000) {
    static int pids[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) pids[i] = i * 3;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int key = (i % 1000) * 3;
        int *found = (int *)bsearch(&key, pids, 1000, sizeof(int), bench_compare_pids);
        BENCH_SINK_PTR(found);
    }
}


/* ===== Field ID lookup benchmark ===== */
BENCH(field_id_lookup, 10000000) {
    char fields[] = "acdfginoprstunR";
    int num_fields = (int)strlen(fields);
    for (int i = 0; i < bf_iters; i++) {
        char target = fields[i % num_fields];
        int found = 0;
        for (int j = 0; j < num_fields; j++) {
            if (fields[j] == target) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}


/* ===== strncmp prefix match benchmark (command matching pattern) ===== */
BENCH(strncmp_prefix_match, 10000000) {
    const char *cmds[] = {"systemd", "sshd", "bash", "python3", "node",
                          "postgres", "nginx", "docker", "java", "chrome"};
    for (int i = 0; i < bf_iters; i++) {
        const char *cmd = cmds[i % 10];
        int match = (strncmp(cmd, "ssh", 3) == 0);
        BENCH_SINK_INT(match);
    }
}

BENCH(strcasecmp_proto, 10000000) {
    const char *protos[] = {"TCP", "tcp", "UDP", "udp", "Tcp", "Udp"};
    for (int i = 0; i < bf_iters; i++) {
        int is_tcp = (strcasecmp(protos[i % 6], "tcp") == 0);
        BENCH_SINK_INT(is_tcp);
    }
}


/* ===== readdir benchmark (relevant to device/directory scanning) ===== */
#include <dirent.h>

BENCH(readdir_dev, 1000) {
    for (int i = 0; i < bf_iters; i++) {
        int count = 0;
        DIR *dir_handle = opendir("/dev");
        if (dir_handle) {
            struct dirent *dir_entry;
            while ((dir_entry = readdir(dir_handle)) != NULL) count++;
            closedir(dir_handle);
        }
        BENCH_SINK_INT(count);
    }
}


/* ===== lstat benchmark (relevant to Readlink/path resolution) ===== */
BENCH(lstat_file, 100000) {
    struct stat st;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(lstat("/dev/null", &st));
    }
}


/* ===== pipe create/close benchmark ===== */
BENCH(pipe_create_close, 100000) {
    for (int i = 0; i < bf_iters; i++) {
        int fds[2];
        if (pipe(fds) == 0) {
            close(fds[0]);
            close(fds[1]);
        }
        BENCH_SINK_INT(fds[0]);
    }
}


/* ===== safestrprt printable classification benchmark ===== */
/* Compare isprint() vs lookup table for character classification */
BENCH(isprint_per_char, 10000000) {
    unsigned char data[] = "Hello\tWorld\n/usr/local/bin/test\x80\xff";
    int dlen = (int)sizeof(data) - 1;
    for (int i = 0; i < bf_iters; i++) {
        int count = 0;
        for (int j = 0; j < dlen; j++) {
            if (isprint(data[j])) count++;
        }
        BENCH_SINK_INT(count);
    }
}

BENCH(lookup_table_per_char, 10000000) {
    static const unsigned char printable[256] = {
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,
    };
    unsigned char data[] = "Hello\tWorld\n/usr/local/bin/test\x80\xff";
    int dlen = (int)sizeof(data) - 1;
    for (int i = 0; i < bf_iters; i++) {
        int count = 0;
        for (int j = 0; j < dlen; j++) {
            if (printable[data[j]]) count++;
        }
        BENCH_SINK_INT(count);
    }
}


/* ===== x2dev validation: branch vs lookup table ===== */
BENCH(hex_validate_branching, 5000000) {
    char *inputs[] = {"0xdeadbeef", "0x1a2b3c4d", "ff", "0xCAFE", "12ab"};
    for (int i = 0; i < bf_iters; i++) {
        char *s = inputs[i % 5];
        if (strncasecmp(s, "0x", 2) == 0) s += 2;
        int n = 0;
        for (char *cp = s; *cp; cp++, n++) {
            if (isdigit((unsigned char)*cp)) continue;
            if ((unsigned char)*cp >= 'a' && (unsigned char)*cp <= 'f') continue;
            if ((unsigned char)*cp >= 'A' && (unsigned char)*cp <= 'F') continue;
            if (*cp == ' ' || *cp == ',') break;
            break;
        }
        BENCH_SINK_INT(n);
    }
}

BENCH(hex_validate_lookup, 5000000) {
    static const unsigned char cls[256] = {
        ['0']=1,['1']=1,['2']=1,['3']=1,['4']=1,['5']=1,['6']=1,['7']=1,
        ['8']=1,['9']=1,['a']=1,['b']=1,['c']=1,['d']=1,['e']=1,['f']=1,
        ['A']=1,['B']=1,['C']=1,['D']=1,['E']=1,['F']=1,[' ']=2,[',']=2,
    };
    char *inputs[] = {"0xdeadbeef", "0x1a2b3c4d", "ff", "0xCAFE", "12ab"};
    for (int i = 0; i < bf_iters; i++) {
        char *s = inputs[i % 5];
        if (strncasecmp(s, "0x", 2) == 0) s += 2;
        int n = 0;
        for (char *cp = s; *cp; cp++, n++) {
            unsigned char c = cls[(unsigned char)*cp];
            if (c == 1) continue;
            break;
        }
        BENCH_SINK_INT(n);
    }
}


/* ===== Integer to string: snprintf vs manual ===== */
BENCH(itoa_snprintf, 5000000) {
    char buf[32];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "%d", i % 65536);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(itoa_manual, 5000000) {
    char buf[32];
    for (int i = 0; i < bf_iters; i++) {
        int v = i % 65536;
        char *ep = &buf[31];
        *ep = '\0';
        if (v == 0) { *--ep = '0'; }
        else { do { *--ep = '0' + (v % 10); v /= 10; } while (v); }
        BENCH_SINK_PTR(ep);
    }
}


/* ===== Process selection: linear scan vs binary search ===== */
BENCH(linear_pid_scan_100, 5000000) {
    static int pids[100];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 100; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 200) * 5; /* ~50% hit rate */
        int found = 0;
        for (int j = 0; j < 100; j++) {
            if (pids[j] == target) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

BENCH(bsearch_pid_scan_100, 5000000) {
    static int pids[100];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 100; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 200) * 5;
        int *found = (int *)bsearch(&target, pids, 100, sizeof(int), bench_compare_pids);
        BENCH_SINK_PTR(found);
    }
}

BENCH(linear_pid_scan_1000, 1000000) {
    static int pids[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 2000) * 5;
        int found = 0;
        for (int j = 0; j < 1000; j++) {
            if (pids[j] == target) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

BENCH(bsearch_pid_scan_1000, 5000000) {
    static int pids[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) pids[i] = i * 10;
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int target = (i % 2000) * 5;
        int *found = (int *)bsearch(&target, pids, 1000, sizeof(int), bench_compare_pids);
        BENCH_SINK_PTR(found);
    }
}


/* ===== FD status check: linked list traversal patterns ===== */
struct bench_fd_lst {
    char *nm;
    int lo, hi;
    struct bench_fd_lst *next;
};

BENCH(fd_status_check_numeric, 5000000) {
    static struct bench_fd_lst nodes[20];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 20; i++) {
            nodes[i].nm = NULL;
            nodes[i].lo = nodes[i].hi = i * 5;
            nodes[i].next = (i < 19) ? &nodes[i + 1] : NULL;
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        int num = (i % 100);
        int found = 0;
        for (struct bench_fd_lst *fp = &nodes[0]; fp; fp = fp->next) {
            if (num >= fp->lo && num <= fp->hi) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}

BENCH(fd_status_check_named, 5000000) {
    static struct bench_fd_lst nodes[10];
    static char *names[] = {"cwd", "rtd", "txt", "mem", "DEL", "0", "1", "2", "3", "4"};
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 10; i++) {
            nodes[i].nm = names[i];
            nodes[i].lo = 1;
            nodes[i].hi = 0;
            nodes[i].next = (i < 9) ? &nodes[i + 1] : NULL;
        }
        initialized = 1;
    }
    char *targets[] = {"cwd", "txt", "4", "mem", "99"};
    for (int i = 0; i < bf_iters; i++) {
        char *nm = targets[i % 5];
        int found = 0;
        for (struct bench_fd_lst *fp = &nodes[0]; fp; fp = fp->next) {
            if (fp->nm && strcmp(fp->nm, nm) == 0) { found = 1; break; }
        }
        BENCH_SINK_INT(found);
    }
}


/* ===== safestrlen: original vs optimized comparison ===== */
BENCH(safestrlen_original, 5000000) {
    /* Original implementation using isprint() */
    char *test = "/usr/local/bin/test\twith\ttabs\nand\nnewlines\x80\xfe";
    for (int i = 0; i < bf_iters; i++) {
        int len = 0;
        for (char *sp = test; *sp; sp++) {
            if (!isprint((unsigned char)*sp)) {
                if (*sp < 0x20 || (unsigned char)*sp == 0xff)
                    len += 2;
                else
                    len += 4;
            } else
                len++;
        }
        BENCH_SINK_INT(len);
    }
}

BENCH(safestrlen_lookup_table, 5000000) {
    /* Optimized implementation using lookup table */
    static const unsigned char explen[256] = {
        2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
        2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,
        4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,
        2,
    };
    char *test = "/usr/local/bin/test\twith\ttabs\nand\nnewlines\x80\xfe";
    for (int i = 0; i < bf_iters; i++) {
        int len = 0;
        for (unsigned char *sp = (unsigned char *)test; *sp; sp++) {
            len += explen[*sp];
        }
        BENCH_SINK_INT(len);
    }
}


/* ===== memcpy vs snpf for short fixed strings ===== */
BENCH(arrow_snprintf, 10000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        char *np = buf;
        snprintf(np, 64, "->");
        np += 2;
        BENCH_SINK_PTR(np);
    }
}

BENCH(arrow_direct, 10000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        char *np = buf;
        *np++ = '-';
        *np++ = '>';
        BENCH_SINK_PTR(np);
    }
}


/* ===== Regex matching benchmark (is_cmd_excl pattern) ===== */
#include <regex.h>

BENCH(regex_match_simple, 500000) {
    static regex_t rx;
    static int compiled = 0;
    if (!compiled) {
        regcomp(&rx, "^ssh", REG_NOSUB);
        compiled = 1;
    }
    char *cmds[] = {"sshd", "bash", "python3", "sshd-keygen", "node",
                    "nginx", "ssh-agent", "systemd", "docker", "sshfs"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(regexec(&rx, cmds[i % 10], 0, NULL, 0));
    }
}

BENCH(strncmp_vs_regex, 500000) {
    /* Same matching task but with strncmp instead of regex */
    char *cmds[] = {"sshd", "bash", "python3", "sshd-keygen", "node",
                    "nginx", "ssh-agent", "systemd", "docker", "sshfs"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(strncmp(cmds[i % 10], "ssh", 3));
    }
}


/* ===== UID cache lookup benchmark ===== */
#include <pwd.h>

BENCH(getpwuid_cached, 100000) {
    /* First call caches; subsequent calls should be fast */
    uid_t uid = getuid();
    for (int i = 0; i < bf_iters; i++) {
        struct passwd *pw = getpwuid(uid);
        BENCH_SINK_PTR(pw);
    }
}


/* ===== Process iteration: array vs pointer chasing ===== */
BENCH(array_iteration_1000, 1000000) {
    static int data[1000];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 1000; i++) data[i] = i;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (int i = 0; i < 1000; i++) sum += data[i];
        BENCH_SINK_INT(sum);
    }
}

BENCH(pointer_chase_1000, 1000000) {
    /* Simulate linked list pointer chasing (cache-unfriendly) */
    static struct bench_node nodes[1000];
    static int initialized = 0;
    if (!initialized) {
        /* Shuffle order to simulate real linked list (non-sequential memory) */
        int order[1000];
        for (int i = 0; i < 1000; i++) order[i] = i;
        for (int i = 999; i > 0; i--) {
            int j = i / 2; /* deterministic shuffle */
            int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }
        for (int i = 0; i < 999; i++) {
            nodes[order[i]].value = order[i];
            nodes[order[i]].next = &nodes[order[i + 1]];
        }
        nodes[order[999]].value = order[999];
        nodes[order[999]].next = NULL;
        initialized = 1;
    }
    for (int j = 0; j < bf_iters; j++) {
        int sum = 0;
        for (struct bench_node *n = &nodes[0]; n; n = n->next)
            sum += n->value;
        BENCH_SINK_INT(sum);
    }
}


/* ===== strncasecmp vs manual lowercase compare ===== */
BENCH(strncasecmp_call, 10000000) {
    char *protos[] = {"TCP", "tcp", "UDP", "udp", "Tcp", "Udp",
                      "tcP", "uDP", "TCp", "UdP"};
    for (int i = 0; i < bf_iters; i++) {
        int r = strncasecmp(protos[i % 10], "tcp", 3);
        BENCH_SINK_INT(r);
    }
}

BENCH(manual_lowercase_cmp, 10000000) {
    char *protos[] = {"TCP", "tcp", "UDP", "udp", "Tcp", "Udp",
                      "tcP", "uDP", "TCp", "UdP"};
    for (int i = 0; i < bf_iters; i++) {
        char *s = protos[i % 10];
        int r = ((s[0] | 0x20) == 't' && (s[1] | 0x20) == 'c' && (s[2] | 0x20) == 'p') ? 0 : 1;
        BENCH_SINK_INT(r);
    }
}


/* ===== hashbyname benchmark (name-based hashing used in lsof) ===== */
static int
bench_hashbyname(char *nm, int mod)
{
    int i, j;
    for (i = j = 0; *nm; nm++) {
        i ^= (int)*nm << j;
        if (++j > 7)
            j = 0;
    }
    return (((int)(i * 31415)) & (mod - 1));
}

BENCH(hashbyname_short, 10000000) {
    char *names[] = {"tcp", "udp", "pipe", "unix", "fifo"};
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_hashbyname(names[i % 5], 128));
    }
}

BENCH(hashbyname_path, 5000000) {
    char *paths[] = {
        "/usr/local/bin/node", "/var/log/syslog", "/dev/null",
        "/proc/self/fd/3", "/tmp/socket.sock"
    };
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_hashbyname(paths[i % 5], 256));
    }
}

BENCH(hashbyname_long, 2000000) {
    char *names[] = {
        "/usr/local/share/applications/very/deep/nested/path/to/resource.dat",
        "/home/user/.config/some-application/settings/profile/default.json",
        "/var/lib/docker/overlay2/abc123def456/merged/usr/bin/entrypoint.sh",
    };
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(bench_hashbyname(names[i % 3], 512));
    }
}


/* ===== mkstrcat benchmark (string concatenation used in path building) ===== */
static char *
bench_mkstrcat(char *s1, int l1, char *s2, int l2, char *s3, int l3)
{
    size_t len1 = s1 ? ((l1 >= 0) ? (size_t)l1 : strlen(s1)) : 0;
    size_t len2 = s2 ? ((l2 >= 0) ? (size_t)l2 : strlen(s2)) : 0;
    size_t len3 = s3 ? ((l3 >= 0) ? (size_t)l3 : strlen(s3)) : 0;
    char *cp = (char *)malloc(len1 + len2 + len3 + 1);
    if (cp) {
        char *tp = cp;
        if (s1 && len1) { memcpy(tp, s1, len1); tp += len1; }
        if (s2 && len2) { memcpy(tp, s2, len2); tp += len2; }
        if (s3 && len3) { memcpy(tp, s3, len3); tp += len3; }
        *tp = '\0';
    }
    return cp;
}

BENCH(mkstrcat_two, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcat("/usr/local", -1, "/bin/lsof", -1, NULL, 0);
        BENCH_SINK_PTR(s);
        free(s);
    }
}

BENCH(mkstrcat_three, 2000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcat("/proc/", -1, "1234", -1, "/fd/0", -1);
        BENCH_SINK_PTR(s);
        free(s);
    }
}

BENCH(mkstrcat_with_lengths, 5000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *s = bench_mkstrcat("/proc/", 6, "1234", 4, "/fd/0", 5);
        BENCH_SINK_PTR(s);
        free(s);
    }
}


/* ===== safestrprt benchmark (safe printing to stream) ===== */
BENCH(safestrprt_clean, 500000) {
    FILE *f = fopen("/dev/null", "w");
    if (!f) return;
    for (int i = 0; i < bf_iters; i++) {
        char *sp = "/usr/local/bin/lsof";
        for (; *sp; sp++)
            putc((int)(*sp & 0xff), f);
    }
    fclose(f);
}

BENCH(safestrprt_dirty, 500000) {
    FILE *f = fopen("/dev/null", "w");
    if (!f) return;
    char *test = "/usr/bin/test\twith\ttabs\nand\x80\xfe";
    for (int i = 0; i < bf_iters; i++) {
        for (char *sp = test; *sp; sp++) {
            unsigned char c = (unsigned char)*sp;
            if (c >= 0x20 && c <= 0x7e)
                putc((int)c, f);
            else {
                int cl;
                char *r = bench_safe_print_unprintable((unsigned int)c, &cl);
                fputs(r, f);
            }
        }
    }
    fclose(f);
}


/* ===== strftime benchmark (time formatting for lsof -T output) ===== */
#include <time.h>

BENCH(strftime_default, 500000) {
    char buf[128];
    time_t tm = time(NULL);
    struct tm *lt = localtime(&tm);
    for (int i = 0; i < bf_iters; i++) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(strftime_epoch, 500000) {
    char buf[128];
    time_t tm = time(NULL);
    struct tm *lt = localtime(&tm);
    for (int i = 0; i < bf_iters; i++) {
        strftime(buf, sizeof(buf), "%s", lt);
        BENCH_SINK_PTR(buf);
    }
}


/* ===== readlink benchmark (symlink resolution used in Readlink) ===== */
BENCH(readlink_dev_fd, 100000) {
    char buf[1024];
    char path[64];
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;
    snprintf(path, sizeof(path), "/dev/fd/%d", fd);
    for (int i = 0; i < bf_iters; i++) {
        ssize_t r = readlink(path, buf, sizeof(buf) - 1);
        BENCH_SINK_INT((int)r);
    }
    close(fd);
}


/* ===== access / is_readable benchmark ===== */
BENCH(access_readable, 500000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(access("/dev/null", R_OK));
    }
}

BENCH(access_nonexistent, 500000) {
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(access("/nonexistent/path/file", R_OK));
    }
}


/* ===== dup/dup2 benchmark (file descriptor operations) ===== */
BENCH(dup_close, 100000) {
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;
    for (int i = 0; i < bf_iters; i++) {
        int fd2 = dup(fd);
        if (fd2 >= 0) close(fd2);
        BENCH_SINK_INT(fd2);
    }
    close(fd);
}


/* ===== fopen/fclose vs open/close overhead ===== */
BENCH(fopen_fclose, 100000) {
    for (int i = 0; i < bf_iters; i++) {
        FILE *f = fopen("/dev/null", "r");
        if (f) fclose(f);
        BENCH_SINK_PTR(f);
    }
}


/* ===== write syscall overhead ===== */
BENCH(write_devnull, 500000) {
    int fd = open("/dev/null", O_WRONLY);
    if (fd < 0) return;
    char buf[] = "p1234\nc/usr/bin/lsof\n";
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT((int)write(fd, buf, sizeof(buf) - 1));
    }
    close(fd);
}


/* ===== strtol benchmark (numeric string parsing in lsof) ===== */
BENCH(strtol_pid, 10000000) {
    char *pids[] = {"1", "42", "1337", "65535", "100000"};
    for (int i = 0; i < bf_iters; i++) {
        long v = strtol(pids[i % 5], NULL, 10);
        BENCH_SINK_INT((int)v);
    }
}

BENCH(strtol_hex, 10000000) {
    char *hexs[] = {"ff", "1a2b", "deadbeef", "cafe", "0"};
    for (int i = 0; i < bf_iters; i++) {
        long v = strtol(hexs[i % 5], NULL, 16);
        BENCH_SINK_INT((int)v);
    }
}


/* ===== memset/memcpy at different sizes ===== */
BENCH(memset_64, 10000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        memset(buf, 0, 64);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(memset_4096, 5000000) {
    char buf[4096];
    for (int i = 0; i < bf_iters; i++) {
        memset(buf, 0, 4096);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(memcpy_64, 10000000) {
    char src[64], dst[64];
    memset(src, 'A', 64);
    for (int i = 0; i < bf_iters; i++) {
        memcpy(dst, src, 64);
        BENCH_SINK_PTR(dst);
    }
}

BENCH(memcpy_4096, 5000000) {
    char src[4096], dst[4096];
    memset(src, 'A', 4096);
    for (int i = 0; i < bf_iters; i++) {
        memcpy(dst, src, 4096);
        BENCH_SINK_PTR(dst);
    }
}


/* ===== getenv benchmark (environment variable lookup) ===== */
BENCH(getenv_hit, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *v = getenv("PATH");
        BENCH_SINK_PTR(v);
    }
}

BENCH(getenv_miss, 10000000) {
    for (int i = 0; i < bf_iters; i++) {
        char *v = getenv("LSOF_NONEXISTENT_VAR_12345");
        BENCH_SINK_PTR(v);
    }
}


/* ===== gethostname benchmark ===== */
BENCH(gethostname_call, 1000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        gethostname(buf, sizeof(buf));
        BENCH_SINK_PTR(buf);
    }
}


/* ===== sprintf PID/FD formatting (common lsof output pattern) ===== */
BENCH(sprintf_pid_field, 5000000) {
    char buf[64];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "p%d\n", i % 100000);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(sprintf_field_output, 2000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "p%d\nc%s\nf%d\nn%s\n",
                 i % 65536, "bash", i % 256,
                 "/usr/local/bin/lsof");
        BENCH_SINK_PTR(buf);
    }
}

BENCH(manual_field_output, 2000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        char *p = buf;
        /* p<pid>\n */
        *p++ = 'p';
        int v = i % 65536;
        char tmp[16]; int tl = 0;
        if (v == 0) tmp[tl++] = '0';
        else { while (v) { tmp[tl++] = '0' + (v % 10); v /= 10; } }
        while (tl > 0) *p++ = tmp[--tl];
        *p++ = '\n';
        /* c<cmd>\n */
        *p++ = 'c';
        memcpy(p, "bash", 4); p += 4;
        *p++ = '\n';
        /* f<fd>\n */
        *p++ = 'f';
        v = i % 256; tl = 0;
        if (v == 0) tmp[tl++] = '0';
        else { while (v) { tmp[tl++] = '0' + (v % 10); v /= 10; } }
        while (tl > 0) *p++ = tmp[--tl];
        *p++ = '\n';
        /* n<name>\n */
        *p++ = 'n';
        memcpy(p, "/usr/local/bin/lsof", 19); p += 19;
        *p++ = '\n';
        *p = '\0';
        BENCH_SINK_PTR(buf);
    }
}


/* ===== command name truncation (strncpy pattern used in lsof) ===== */
BENCH(strncpy_cmd_short, 10000000) {
    char dst[16];
    char *cmds[] = {"bash", "sh", "sshd", "node", "python3"};
    for (int i = 0; i < bf_iters; i++) {
        strncpy(dst, cmds[i % 5], 15);
        dst[15] = '\0';
        BENCH_SINK_PTR(dst);
    }
}

BENCH(strncpy_cmd_long, 5000000) {
    char dst[16];
    char *cmds[] = {
        "chromium-browser-stable",
        "gnome-terminal-server",
        "firefox-developer-edition",
        "docker-containerd-shim",
        "systemd-resolved.service"
    };
    for (int i = 0; i < bf_iters; i++) {
        strncpy(dst, cmds[i % 5], 15);
        dst[15] = '\0';
        BENCH_SINK_PTR(dst);
    }
}


/* ===== path building: snprintf vs manual concat ===== */
BENCH(path_snprintf, 5000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        snprintf(buf, sizeof(buf), "/proc/%d/fd/%d", i % 65536, i % 256);
        BENCH_SINK_PTR(buf);
    }
}

BENCH(path_manual, 5000000) {
    char buf[256];
    for (int i = 0; i < bf_iters; i++) {
        char *p = buf;
        memcpy(p, "/proc/", 6); p += 6;
        /* pid */
        int v = i % 65536;
        char tmp[16]; int tl = 0;
        if (v == 0) tmp[tl++] = '0';
        else { while (v) { tmp[tl++] = '0' + (v % 10); v /= 10; } }
        while (tl > 0) *p++ = tmp[--tl];
        memcpy(p, "/fd/", 4); p += 4;
        /* fd */
        v = i % 256; tl = 0;
        if (v == 0) tmp[tl++] = '0';
        else { while (v) { tmp[tl++] = '0' + (v % 10); v /= 10; } }
        while (tl > 0) *p++ = tmp[--tl];
        *p = '\0';
        BENCH_SINK_PTR(buf);
    }
}


/* ===== hash table with deeper collision chains ===== */
BENCH(hash_lookup_deep_chain, 2000000) {
    /* Simulate a bucket with 50 entries chained (worst case) */
    static struct bench_porttab entries[50];
    static struct bench_porttab *head = NULL;
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 50; i++) {
            entries[i].port = i * 100;
            entries[i].name = "service";
            entries[i].next = head;
            head = &entries[i];
        }
        initialized = 1;
    }
    for (int i = 0; i < bf_iters; i++) {
        /* Search for entry near the end of the chain */
        int target = (i % 50) * 100;
        struct bench_porttab *port_entry;
        for (port_entry = head; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == target) break;
        }
        BENCH_SINK_PTR(port_entry);
    }
}


/* ===== fcntl getfl benchmark (file descriptor flag inspection) ===== */
BENCH(fcntl_getfl, 500000) {
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return;
    for (int i = 0; i < bf_iters; i++) {
        BENCH_SINK_INT(fcntl(fd, F_GETFL));
    }
    close(fd);
}


/* ===== socketpair benchmark (UNIX domain socket creation) ===== */
BENCH(socketpair_create_close, 50000) {
    for (int i = 0; i < bf_iters; i++) {
        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
            close(fds[0]);
            close(fds[1]);
        }
        BENCH_SINK_INT(fds[0]);
    }
}


/* ===== process self inspection (/proc or equivalent) ===== */
BENCH(read_proc_self, 50000) {
    for (int i = 0; i < bf_iters; i++) {
        char buf[4096];
        int fd = open("/dev/null", O_RDONLY);
        if (fd >= 0) {
            ssize_t n = read(fd, buf, sizeof(buf));
            close(fd);
            BENCH_SINK_INT((int)n);
        }
    }
}


RUN_BENCHMARKS()
