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

Measure performance of core operations:

```bash
make bench
```

This builds and runs `benchmark_core`, writing results to `benchmark_core.log` in the build directory.

### Benchmarks (`bench/bench_core.c`)

65 benchmarks covering core operations, optimization comparisons, and system call overhead.

| Category | What it measures |
|---|---|
| **x2dev** | Hex parsing — short, prefixed, and long strings |
| **HASHPORT** | Port hashing — full range and common ports |
| **safestrlen** | Safe string length — short, escaped, long paths, binary data |
| **compdev sort** | Device table sorting via qsort — 100 and 1000 elements |
| **safepup** | Unprintable char formatting — control chars and high bytes |
| **I/O** | open/close, stat, lstat, getpid, pipe, readdir |
| **String ops** | strlen, strcmp, strncmp, strcasecmp, snprintf, isprint scanning |
| **Socket** | TCP socket create/close cycle |
| **Memory** | malloc/free (small/medium), realloc growth, safe realloc pattern |
| **String copy** | mkstrcpy — short, path-length, and NULL source |
| **Data structures** | Linked list traversal (100/1000), hash lookup hit/miss |
| **Search** | PID sort (qsort), PID binary search, field ID lookup |
| **Matching** | Regex match, strncmp prefix, FD status check (numeric/named) |
| **UID** | getpwuid cached lookup |

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

---

## // CREDITS

Originally written by **Victor A. Abell** at Purdue University.
Maintained by the open-source community.

---

## // LICENSE

lsof is distributed under a permissive license. See individual source files for details.

---

```
[ END OF LINE ]
```
