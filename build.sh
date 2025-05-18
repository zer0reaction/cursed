#!/bin/bash

CC='gcc'
CFLAGS='-Wall -Wextra -pedantic -std=c99 -fsanitize=address,undefined -pipe'
LDFLAGS='-lncursesw'

OBJ='main.o utility.o buffer.o editor.o'

if [ ! -f config.h ]; then
    cp config.def.h config.h
fi

set -xe

$CC $CFLAGS -c -o main.o    main.c
$CC $CFLAGS -c -o utility.o utility.c
$CC $CFLAGS -c -o buffer.o  buffer.c
$CC $CFLAGS -c -o editor.o  editor.c

$CC $CFLAGS -o cursed $OBJ $LDFLAGS
