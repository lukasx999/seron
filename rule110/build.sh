#!/bin/sh
set -euxo pipefail

make -C ../seronc/
../seronc/seronc -t asm ./rule110.sn
../seronc/seronc -t obj ./rule110.sn
cc ./rule110.o -lraylib -lc -no-pie -o rule110
./rule110
