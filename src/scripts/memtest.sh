#!/usr/bin/env bash
set -ueo pipefail

COLOR_GREEN=$'\33[32m'
COLOR_RED=$'\33[31m'
COLOR_BOLD=$'\33[1m'
COLOR_END=$'\33[0m'

bin=$1
status=0
valgrind --leak-check=full --error-exitcode=1 --log-file=/dev/null $bin 1>/dev/null 2>&1 || status=$?

echo "[MEMTEST] Testing for memory leaks..."

if [[ $status != 0 ]] then
    echo "[MEMTEST] ${COLOR_BOLD}${COLOR_RED}FAILED${COLOR_END}"
else
    echo "[MEMTEST] ${COLOR_BOLD}${COLOR_GREEN}PASSED${COLOR_END}"
fi

echo ""
