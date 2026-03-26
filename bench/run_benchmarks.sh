#!/bin/bash
# Run all lsof benchmarks
# Usage: ./bench/run_benchmarks.sh [filter]
#   filter: optional substring to match benchmark names (e.g. "hash", "sort", "syscall", "optduel")

set -e

# ANSI codes
C="\033[0;36;1m"  # bold cyan
G="\033[0;32;1m"  # bold green
M="\033[0;35;1m"  # bold magenta
B="\033[1m"       # bold
D="\033[2m"       # dim
R="\033[0m"       # reset

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo ""
echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo -e "  ${C}░░${R}  ${B}COMPILING BENCHMARK ICE...${R}                                        ${C}░░${R}"
echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo ""

cmake -S "$PROJECT_DIR" -B "$PROJECT_DIR" > /dev/null 2>&1
cmake --build "$PROJECT_DIR" --target benchmark_hex benchmark_hash benchmark_strsafe benchmark_strops benchmark_strbuild benchmark_sort benchmark_datastruct benchmark_memory benchmark_syscall benchmark_output benchmark_optduel benchmark_environ -j$(sysctl -n hw.ncpu 2>/dev/null || nproc) 2>&1 | tail -1

SYS="$(uname -ms)"
CPU="$(sysctl -n machdep.cpu.brand_string 2>/dev/null || cat /proc/cpuinfo 2>/dev/null | grep 'model name' | head -1 | cut -d: -f2)"
DATE="$(date '+%Y-%m-%d %H:%M:%S')"

echo ""
echo -e "  ${D}┌──────────────────────────────────────────────────────────────────────┐${R}"
echo -e "  ${D}│${R}  ${M}SYSTEM${R}  $SYS"
echo -e "  ${D}│${R}  ${M}CPU${R}     $CPU"
echo -e "  ${D}│${R}  ${M}DATE${R}    $DATE"
echo -e "  ${D}└──────────────────────────────────────────────────────────────────────┘${R}"
echo ""

FILTER="${1:-}"
BENCHMARKS="benchmark_hex benchmark_hash benchmark_strsafe benchmark_strops benchmark_strbuild benchmark_sort benchmark_datastruct benchmark_memory benchmark_syscall benchmark_output benchmark_optduel benchmark_environ"

for bench in $BENCHMARKS; do
    if [ -n "$FILTER" ] && [[ "$bench" != *"$FILTER"* ]]; then
        continue
    fi
    if [ -x "$PROJECT_DIR/$bench" ]; then
        "$PROJECT_DIR/$bench"
        echo ""
    fi
done

echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo -e "  ${C}░░${R}  ${G}▓▓ ALL BENCHMARKS COMPLETE // CONNECTION TERMINATED${R}          ${C}░░${R}"
echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo ""
