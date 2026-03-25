#!/bin/sh
set -e
cd "/Users/wizard/forkedRepos/lsof"

echo '=== check_unit ==='
./check_unit > check_unit.log 2>&1 && echo PASS >> check_unit.log || echo FAIL >> check_unit.log
cat check_unit.log

echo ''
echo '=== check_integration ==='
./check_integration > check_integration.log 2>&1 && echo PASS >> check_integration.log || echo FAIL >> check_integration.log
cat check_integration.log

# Exit with failure if any test failed
grep -q FAIL check_unit.log check_integration.log && exit 1 || exit 0
