#!/bin/bash
GCC_FLAGS="-lm -O3 -march=native -Wall -Wextra "

rm -f ./build/day*
for file in *.c; do
    gcc $GCC_FLAGS "$file" -o "./build/${file%.c}"
done
