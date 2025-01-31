#!/usr/bin/env bash
set -euxo pipefail

../compiler -W --dump-ast --dump-symboltable ./main.spx --debug-asm
