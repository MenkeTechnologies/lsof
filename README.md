```
 ██▓      ██████  ▒█████    █████▒
▓██▒    ▒██    ▒ ▒██▒  ██▒▓██   ▒
▒██░    ░ ▓██▄   ▒██░  ██▒▒████ ░
▒██░      ▒   ██▒▒██   ██░░▓█▒  ░
░██████▒▒██████▒▒░ ████▓▒░░▒█░
░ ▒░▓  ░▒ ▒▓▒ ▒ ░░ ▒░▒░▒░  ▒ ░
░ ░ ▒  ░░ ░▒  ░ ░  ░ ▒ ▒░  ░
  ░ ░  ░ ░  ░    ░ ░ ░ ▒   ░ ░
    ░        ░        ░ ░
```

> *"In the sprawl of running processes, every open file is a signal. Every socket, a wire in the dark."*

---

## // WHAT IS THIS

**lsof** — **L**ist **S**ystem **O**pen **F**iles — v5.0

A diagnostic tool forged in the UNIX underground. It maps the invisible topology between processes and the files they hold open: regular files, directories, sockets, pipes, devices, streams — anything the kernel touches.

If a process has a file descriptor, `lsof` sees it.

---

## // JACK IN — BUILD FROM SOURCE

### > CMake (recommended)

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

Installs to `/usr/local/sbin/lsof` by default. To change the prefix:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
```

---

## // SUPPORTED DIALECTS

lsof interfaces directly with kernel data structures. Each target OS is a **dialect** — a bespoke adapter wired into the machine layer.

| Dialect | Platform |
|---|---|
| `linux` | Linux |
| `darwin/libproc` | Apple macOS / Darwin |
| `freebsd` | FreeBSD |
| `n+obsd` | NetBSD / OpenBSD |
| `n+os` | NEXTSTEP / OpenStep |
| `sun` | Solaris / SunOS |
| `aix` | IBM AIX |
| `du` | DEC/Compaq/HP Tru64 UNIX (Digital UNIX) |
| `hpux/pstat` | HP-UX |
| `osr` | SCO OpenServer |
| `uw` | SCO UnixWare |

---

## // QUICKSTART — RUN IT

```bash
# List all open files
lsof

# Who's holding port 8080?
lsof -i :8080

# What files does PID 1337 have open?
lsof -p 1337

# All network connections — TCP & UDP
lsof -i

# Find who's using a specific file
lsof /var/log/syslog

# Trace a user's open files
lsof -u neo
```

---

## // SCREENSHOTS

### > Help screen

```
  ██▓     ██████  ▒█████    █████▒
 ▓██▒   ▒██    ▒ ▒██▒  ██▒▓██   ▒
 ▒██░   ░ ▓██▄   ▒██░  ██▒▒████ ░
 ░██░     ▒   ██▒▒██   ██░░▓█▒  ░
 ░██░   ▒██████▒░░ ████▓▒░░▒█░
 ░▓     ▒ ▒▓▒ ▒ ░░ ▒░▒░▒░  ▒ ░
  ▒ ░   ░ ░▒  ░ ░  ░ ▒ ▒░  ░
  ▒ ░   ░  ░  ░  ░ ░ ░ ▒   ░ ░
  ░           ░      ░ ░

  >> FILE DESCRIPTOR SCANNER v5.7 <<
  [ mapping the topology of open files ]

  USAGE: lsof [OPTION]... [FILE]...

  ── SELECTION ──────────────────────────────────────
   -?, -h            display this transmission
   -a                AND selections (default: OR)
   -c COMMAND        select by command name (prefix, ^exclude, /regex/)
   +c WIDTH          set COMMAND column width (default: 9)
   -d FD             select by file descriptor set
   -g [PGID]         exclude(^) or select process group IDs
   -p PID            select by PID (comma-separated, ^excludes)
   -u USER           select by login name or UID (comma-separated, ^excludes)

  ── NETWORK ───────────────────────────────────────
   -i [ADDR]         select IPv4 files
   -n                inhibit host name resolution
   -P                inhibit port number to name conversion
   -s [PROTO:STATE]  exclude(^) or select protocol states by name
   -U                select UNIX domain socket files

  ── FILES & DIRECTORIES ───────────────────────────
   +d DIR            list open files in DIR
   +D DIR            recursively list open files in DIR (slow)

  ── DISPLAY ───────────────────────────────────────
   -F [FIELDS]       select output fields; -F ? for help
   -l                display UID numbers instead of login names
   -o [DIGITS]       display file offset (0t format, default: 8 digits)
   -R                list parent PID
```

### > Styled terminal output (`lsof -i :5000`)

```
╔════════════════════════════════════════════════════════════════════════╗
║  ██▓     ██████  ▒█████    █████▒    ╔═╗ v5.7             ║
║ ▓██▒   ▒██    ▒ ▒██▒  ██▒▓██   ▒    ║ ║ [ OPEN FILES ]     ║
║ ▒██░   ░ ▓██▄   ▒██░  ██▒▒████ ░    ╚═╝ CYBERDECK          ║
║ ▒██░     ▒   ██▒▒██   ██░░▓█▒   ░        sys.probe          ║
║ ░██████▒██████▒░░ ████▓▒░░▒█████▓        ◉ ACTIVE           ║
╠════════════════════════════════════════════════════════════════════════╣
PROCESS   PRC  H4XOR   FD   CL4SS            DEV/ICE BYT3/0FF N0DE T4RGET
╠────────────────────────────────────────────────────────────────────────╣
ControlCe 775 wizard   11u   IPv4 0xd21f2bc372b773d3      0t0  TCP *:5000 (LISTEN)
ControlCe 775 wizard   12u   IPv6 0x7fc301359d0cc5ef      0t0  TCP *:5000 (LISTEN)
```

### > Network connections (`lsof -i -n -P`)

```
COMMAND     PID   USER   FD   TYPE             DEVICE SIZE/OFF NODE NAME
rapportd    673 wizard   11u  IPv4 0x2764fa8ae4a40797      0t0  TCP *:55619 (LISTEN)
rapportd    673 wizard   17u  IPv6 0xefa4096199955203      0t0  TCP *:55619 (LISTEN)
sharingd    682 wizard   10u  IPv6 0x98d84e04f020c14c      0t0  UDP *:60502
identitys   690 wizard    7u  IPv4 0x311187060b49b308      0t0  UDP *:*
ControlCe   775 wizard    9u  IPv4 0x6d77ee0fe132b49a      0t0  TCP *:7000 (LISTEN)
ControlCe   775 wizard   11u  IPv4 0xd21f2bc372b773d3      0t0  TCP *:5000 (LISTEN)
NTKDaemon  2238 wizard    7u  IPv4 0x143cb80d30705a16      0t0  TCP 127.0.0.1:49245->127.0.0.1:49244 (ESTABLISHED)
NTKDaemon  2238 wizard   51u  IPv4 0xb408514747714089      0t0  TCP 10.59.0.110:62031->34.141.58.12:443 (ESTABLISHED)
```

### > Process file listing (`lsof -p <PID>`)

```
COMMAND   PID   USER   FD   TYPE             DEVICE SIZE/OFF                NODE NAME
zsh     14738 wizard  cwd    DIR               1,16     1248             1946577 /Users/wizard/forkedRepos/lsof
zsh     14738 wizard  txt    REG               1,16   672928             3810830 /opt/homebrew/Cellar/zsh/5.9/bin/zsh
zsh     14738 wizard  txt    REG               1,16   288224             3793369 /opt/homebrew/Cellar/pcre/8.45/lib/libpcre.1.dylib
zsh     14738 wizard  txt    REG               1,16  2357376 1152921500312572606 /usr/lib/dyld
zsh     14738 wizard    0r   CHR                3,2      0t0                 336 /dev/null
zsh     14738 wizard   11u  unix 0x3b9978ceaa505cf3      0t0                     ->0x13e4a72303ddaa25
```

### > Field output — machine-readable mode (`lsof -F pcfn`)

```
p406
cloginwindow
fcwd
n/
ftxt
n/System/Library/CoreServices/loginwindow.app/Contents/MacOS/loginwindow
ftxt
n/usr/lib/dyld
```

---

## // PERFORMANCE

The core engine is tuned for speed where it matters:

- **Fast text region enumeration**: uses `PROC_PIDREGIONPATHINFO2` (flavor 22) on Darwin, which returns only unique text file regions instead of every memory mapping — **330x fewer syscalls** than the legacy `PROC_PIDREGIONPATHINFO` API, dropping `process_text()` from ~22s to ~0.07s and matching Apple's system lsof
- **Compiler optimization**: `-O2` on all targets
- **Lookup tables**: character classification via precomputed `explen[]`, `printable[]`, `cls[]`, and `hv[]` tables — up to **97x** faster than per-character library calls
- **Binary search**: PID and PGID selection lists are sorted at entry and searched via O(log n) binary search — **2.4x** faster at 100 entries, **10x** at 1000
- **Manual formatting**: field output and path construction bypass `snprintf` for **9–15x** throughput
- **Hash tables**: port cache, host cache, device lookup, and file name matching all use power-of-2 hash tables with polynomial hashing

Run `make bench` or `./bench/run_benchmarks.sh` to see all benchmarks on your hardware. Target a specific suite with `./bench/run_benchmarks.sh hash`.

---

## // ARCHITECTURE

```
src/
├── *.c              # Core engine — command-line parser, output formatter,
│                      process walker, and file examiner
├── lib/             # Shared library modules (name cache, device cache, regex, etc.)
└── dialects/        # Per-OS kernel interface adapters
    ├── linux/
    ├── darwin/libproc/
    ├── freebsd/
    ├── n+obsd/
    ├── n+os/
    ├── sun/
    ├── aix/
    ├── du/
    ├── hpux/pstat/
    ├── osr/
    └── uw/
test/                # Unit and integration test suites
bench/               # Performance benchmarks
```

Each dialect provides three key headers — `machine.h`, `dlsof.h`, `dproto.h` — that wire the core engine into the target OS kernel.

---

## // FIELD OUTPUT — MACHINE-READABLE MODE

```bash
# Emit parseable field output
lsof -F pcfn

# Fields: p=PID, c=command, f=fd, n=name
# Pipe it. Parse it. Automate it.
```

---

## // TESTING

lsof ships with a unit test suite and an integration test suite. Run them with:

```bash
make check
```

This builds and executes `check_unit` (102 unit tests) and `check_integration` (13 integration tests), writing results to `check_unit.log` and `check_integration.log` in the build directory.

### Unit tests (`test/test_unit.c`)

Tests core algorithms in isolation — no kernel access or lsof binary required:

- **Field ID constants** — uniqueness, sequential indices, correct character mappings, printable ASCII validation, name string verification, total count
- **x2dev()** — hex string to device number conversion (prefix handling, delimiters, edge cases, leading zeros, boundary values, null terminators)
- **HASHPORT** — port hash macro range, distribution, determinism, adjacent port divergence, max port, common port validation
- **safestrlen()** — safe string length with unprintable character expansion, full printable ASCII range, multiple control chars, mixed content
- **compdev()** — device table comparator, qsort integration, null names, stability, multi-key sort order (rdev, inode, name)
- **comppid()** — PID comparator, sorting, negative PIDs, duplicate handling
- **safepup()** — unprintable character formatting (control chars, escape sequences, high bytes, DEL, printable range verification)
- **Flag constants** — XO_* crossover flags, FSV_* file struct value flags
- **Memory safety** — safe realloc patterns, leak detection for process tables, host cache, regex tables, PID/UID arrays, directory stacks, state tables, service names, FD lists, efsys paths, network addresses

### Integration tests (`test/test_integration.c`)

Invokes the lsof binary and validates output:

- Binary discovery and help/version flags
- PID-based lookup and field output format (`-F`)
- Open file detection, CWD detection
- TCP and UNIX socket detection
- Invalid PID handling, AND selection (`-a`), FD selection (`-d`)

---

## // BENCHMARKS

### Running benchmarks

```sh
# --- FULL BENCHMARK SWEEP ---
make bench

# --- TARGETED STRIKE ---
./bench/run_benchmarks.sh hash
./bench/run_benchmarks.sh sort
```

### Benchmark suites

163 benchmarks across 12 suites.

```
 ┌────────────────────────────┬───────────────────────────────────────────────────────────────────────┐
 │ SUITE                      │ MEASURES                                                              │
 ├────────────────────────────┼───────────────────────────────────────────────────────────────────────┤
 │ benchmark_hex              │ x2dev parsing, major/minor, makedev, hex generation (snprintf/manual) │
 │ benchmark_hash             │ HASHPORT, hashbyname, SFHASHDEVINO, ncache hash, port service cache   │
 │ benchmark_strsafe          │ safestrlen (clean/dirty/mixed), safepup (control/tab/hex encoding)    │
 │ benchmark_strops           │ strlen, strcmp, strncmp, strchr, strtol, tolower, memmove, path ops   │
 │ benchmark_strbuild         │ mkstrcpy (short/path/null), mkstrcat (2/3-part, pre-computed lengths) │
 │ benchmark_sort             │ compdev/PID qsort, bsearch, device bsearch, rmdupdev (30/50/90% dup) │
 │ benchmark_datastruct       │ Linked list traverse/insert, hash build/lookup, FD range matching     │
 │ benchmark_memory           │ malloc/free, realloc chunked growth, calloc vs memset, batch vs indiv │
 │ benchmark_syscall          │ open, stat, fstat, getcwd, getuid, pipe, socket, readdir, readlink    │
 │ benchmark_output           │ safestrprt, strftime, snpf, IPv4 format, cmd truncate, full line fmt  │
 │ benchmark_optduel          │ isprint vs table, snprintf vs manual, regex vs strncmp, ncache linear  │
 │ benchmark_environ          │ getenv, gethostname, getpwuid, getpwnam, getlogin, UID cache          │
 └────────────────────────────┴───────────────────────────────────────────────────────────────────────┘
```

### Optimization comparisons

The benchmark suite includes head-to-head comparisons that measure the impact of optimizations applied to the codebase. Reference numbers from Apple M-series (single core):

| Comparison | Before | After | Speedup |
|---|---|---|---|
| Character classification: `isprint()` vs lookup table | 11.1 ns/op | 0.2 ns/op | **55x** |
| Safe string length: per-char `isprint` vs `explen[]` table | 19.4 ns/op | 0.2 ns/op | **97x** |
| Integer formatting: `snprintf("%d")` vs manual digit extraction | 25.3 ns/op | 1.7 ns/op | **15x** |
| Hex validation: branch chain vs `cls[]` table | 4.3 ns/op | 3.9 ns/op | **1.1x** |
| PID scan (100 entries): linear vs `bsearch` | 23.1 ns/op | 9.7 ns/op | **2.4x** |
| PID scan (1000 entries): linear vs `bsearch` | 189.8 ns/op | 19.0 ns/op | **10x** |
| Command matching: `regexec` vs `strncmp` | 30.8 ns/op | 1.7 ns/op | **18x** |
| Protocol compare: `strncasecmp` vs manual lowercase | 2.9 ns/op | 0.4 ns/op | **7x** |
| Iteration (1000): contiguous array vs pointer chasing | 24.7 ns/op | 723.1 ns/op | **29x** (array) |
| Field output: `snprintf` vs manual formatting | 74.7 ns/op | 4.9 ns/op | **15x** |
| Path building: `snprintf` vs manual concat | 45.9 ns/op | 4.9 ns/op | **9x** |
| String concat: `mkstrcat` auto-length vs pre-computed | 16.1 ns/op | 7.3 ns/op | **2.2x** |
| File open: `fopen`/`fclose` vs `open`/`close` | 4.8 us/op | 4.5 us/op | **1.1x** |
| Environment lookup: `getenv` hit vs miss | 62.6 ns/op | 213.1 ns/op | **3.4x** (hit) |
| Name cache: linear scan vs hash lookup (200 entries) | linear | hash | hash wins |
| IPv4 formatting: `snprintf` vs manual digit extraction | snprintf | manual | manual wins |
| Device hex: `snprintf("%x,%x")` vs manual hex table | snprintf | manual | manual wins |
| Allocation: `calloc` vs `malloc`+`memset` | calloc | malloc+memset | ~equal |
| Allocation: 256 individual `malloc` vs single batch | individual | batch | batch wins |

### End-to-end: lsof v5.0 vs system lsof v4.91

Wall-clock comparison using [hyperfine](https://github.com/sharkdp/hyperfine) (20 runs, 3 warmup) on Apple M-series, Darwin 25.4.0:

| Workload | System lsof v4.91 | lsof v5.0 | Speedup |
|---|---|---|---|
| All open files (`lsof`) | 315.5 ms | 253.7 ms | **1.24x** |
| Internet connections (`lsof -i`) | 359.8 ms | 180.4 ms | **1.99x** |
| Specific file (`lsof /dev/null`) | 224.5 ms | 176.0 ms | **1.28x** |
| No DNS resolution (`lsof -n -P`) | 193.5 ms | 295.9 ms | 0.65x |

The optimized data structures (hash tables, lookup tables, binary search) pay off most when there is real work to do — DNS resolution, port lookups, large output formatting. When that work is bypassed entirely via `-n -P`, the setup overhead is not amortized.

---

## // CREDITS

Originally written by **Jacob Menke**.
Maintained by the open-source community.

---

## // LICENSE

lsof is distributed under a permissive license. See individual source files for details.

---

```
[ END OF LINE ]
```
