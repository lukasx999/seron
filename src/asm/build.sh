#!/bin/sh
set -euxo pipefail

nasm main.asm -felf64 -gdwarf
ld main.o -o out
