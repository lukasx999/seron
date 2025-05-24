#!/bin/sh
set -euxo pipefail

../seronc/seronc fib.sn
time python3 fib.py
time go run fib.go
time ./fib
