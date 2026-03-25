#!/bin/sh
cd "/Users/wizard/forkedRepos/lsof"

echo '=== benchmark_core ==='
./benchmark_core > benchmark_core.log 2>&1
cat benchmark_core.log
