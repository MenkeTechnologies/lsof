#!/bin/sh
# Run test binaries under macOS `leaks` tool to detect memory leaks

set -e

BUILD_DIR="${1:-.}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

C='\033[1;36m'
M='\033[1;35m'
G='\033[1;32m'
Y='\033[1;33m'
RED='\033[1;31m'
B='\033[1m'
R='\033[0m'

printf '\n'
printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
printf "  ${C}░░${R}  ${B}INITIATING MEMORY LEAK SCAN // DEEP ANALYSIS${R}                  ${C}░░${R}\n"
printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
printf '\n'

PASS=0
FAIL=0

run_leaks() {
    name="$1"
    binary="$2"
    printf "  ${M}>>${R} ${B}%s${R} ... " "$name"

    output=$(leaks --atExit -- "$binary" 2>&1) || true

    if echo "$output" | grep -q "0 leaks"; then
        printf "${G}0 LEAKS${R}\n"
        PASS=$((PASS + 1))
    elif echo "$output" | grep -q "leaks for"; then
        leak_count=$(echo "$output" | grep "leaks for" | sed 's/.*: \([0-9]*\) leak.*/\1/')
        printf "${RED}%s LEAK(S) DETECTED${R}\n" "$leak_count"
        echo "$output" | grep -A2 "leaked" | head -10
        FAIL=$((FAIL + 1))
    else
        printf "${Y}UNKNOWN${R}\n"
        echo "$output" | tail -5
        FAIL=$((FAIL + 1))
    fi
}

cd "$BUILD_DIR"

run_leaks "check_unit" "./check_unit"
run_leaks "check_integration" "./check_integration"

printf '\n'

if [ "$FAIL" -eq 0 ]; then
    printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
    printf "  ${C}░░${R}  ${G}▓▓ MEMORY CLEAN // 0 LEAKS ACROSS %d BINARIES${R}              ${C}░░${R}\n" "$PASS"
    printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
    printf '\n'
    exit 0
else
    printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
    printf "  ${C}░░${R}  ${RED}▓▓ MEMORY BREACH // %d BINARY(S) LEAKING${R}                    ${C}░░${R}\n" "$FAIL"
    printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
    printf '\n'
    exit 1
fi
