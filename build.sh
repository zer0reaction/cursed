#!/bin/bash

CC='gcc'
CFLAGS='-Wall -Wextra -pedantic -std=c99 -fsanitize=address,undefined -pipe'
LDFLAGS='-lncursesw'

OBJ='main.o utility.o buffer.o editor.o'

if [ ! -f src/config.h ]; then
    cp src/config.def.h src/config.h
fi

set -xe

$CC $CFLAGS -c -o main.o    src/main.c
$CC $CFLAGS -c -o utility.o src/utility.c
$CC $CFLAGS -c -o buffer.o  src/buffer.c
$CC $CFLAGS -c -o editor.o  src/editor.c

$CC $CFLAGS -o cursed $OBJ $LDFLAGS
