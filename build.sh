#!/bin/bash

CC='gcc'
CFLAGS='-Wall -Wextra -pedantic -std=c99 -fsanitize=address,undefined -pipe'
LDFLAGS='-lncursesw'

OBJ='bin/main.o bin/utility.o bin/buffer.o bin/editor.o'

set -e

if [ ! -f src/config.h ]; then
    cp src/config.def.h src/config.h
fi

mkdir -p bin
echo '*' > bin/.gitignore

set -xe

$CC $CFLAGS -c -o bin/main.o    src/main.c
$CC $CFLAGS -c -o bin/utility.o src/utility.c
$CC $CFLAGS -c -o bin/buffer.o  src/buffer.c
$CC $CFLAGS -c -o bin/editor.o  src/editor.c

$CC $CFLAGS -o cursed $OBJ $LDFLAGS
