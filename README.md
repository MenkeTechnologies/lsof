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

**lsof** — **L**ist **S**ystem **O**pen **F**iles — v4.85

A diagnostic tool forged in the UNIX underground. It maps the invisible topology between processes and the files they hold open: regular files, directories, sockets, pipes, devices, streams — anything the kernel touches.

If a process has a file descriptor, `lsof` sees it.

---

## // JACK IN — BUILD FROM SOURCE

### > CMake (recommended)

```bash
mkdir build && cd build
cmake ..
make
```

### > Legacy Configure

```bash
./Configure <dialect>
make
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
| `sun` | Solaris / SunOS |
| `aix` | IBM AIX |
| `hpux/pstat` | HP-UX |

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
├── *.c              # Core engine — the command-line parser, output formatter,
│                      process walker, and file examiner
├── lib/             # Shared library modules (name cache, device cache, regex, etc.)
└── dialects/        # Per-OS kernel interface adapters
    ├── linux/
    ├── darwin/libproc/
    ├── freebsd/
    ├── n+obsd/
    ├── sun/
    ├── aix/
    └── hpux/pstat/
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

## // DOCS

| File | Description |
|---|---|
| `00README` | Full build & install guide |
| `00FAQ` | Frequently asked questions |
| `00QUICKSTART` | Fast-track setup |
| `00DIALECTS` | Platform-specific notes |
| `00DCACHE` | Device cache documentation |
| `00PORTING` | Guide to adding new dialects |
| `lsof.8` | The man page |

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
