#!/bin/sh
# Run all lsof benchmarks with cyberpunk output

set -e

BUILD_DIR="${1:-.}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# ANSI codes
M='\033[0;35;1m'
D='\033[2m'
R='\033[0m'

"$SCRIPT_DIR/banner.sh" start

SYS="$(uname -ms)"
CPU="$(sysctl -n machdep.cpu.brand_string 2>/dev/null || cat /proc/cpuinfo 2>/dev/null | grep 'model name' | head -1 | cut -d: -f2)"
DATE="$(date '+%Y-%m-%d %H:%M:%S')"

printf "  ${D}┌──────────────────────────────────────────────────────────────────────┐${R}\n"
printf "  ${D}│${R}  ${M}SYSTEM${R}  %s\n" "$SYS"
printf "  ${D}│${R}  ${M}CPU${R}     %s\n" "$CPU"
printf "  ${D}│${R}  ${M}DATE${R}    %s\n" "$DATE"
printf "  ${D}└──────────────────────────────────────────────────────────────────────┘${R}\n"
printf '\n'

cd "$BUILD_DIR"
./benchmark_core > benchmark_core.log 2>&1
cat benchmark_core.log

"$SCRIPT_DIR/banner.sh" end
