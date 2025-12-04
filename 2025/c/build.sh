#!/bin/bash
GCC_FLAGS="-g -Wall -Wextra -fsanitize=address,undefined"

rm -f ./build/*c
for file in *.c; do
    gcc $GCC_FLAGS "$file" -o "./build/${file%.c}c"
done
