#!/bin/sh
# Run all lsof tests with cyberpunk output

set -e

BUILD_DIR="${1:-.}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

"$SCRIPT_DIR/banner.sh" start

cd "$BUILD_DIR"

echo '=== check_unit ==='
./check_unit > check_unit.log 2>&1 && echo PASS >> check_unit.log || echo FAIL >> check_unit.log
cat check_unit.log

echo ''
echo '=== check_integration ==='
./check_integration > check_integration.log 2>&1 && echo PASS >> check_integration.log || echo FAIL >> check_integration.log
cat check_integration.log

# Exit with failure if any test failed
if grep -q FAIL check_unit.log check_integration.log; then
    exit 1
else
    "$SCRIPT_DIR/banner.sh" end
    exit 0
fi
