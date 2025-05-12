#!/bin/bash

CC='gcc'
CFLAGS='-Wall -Wextra -pedantic -std=c89 -fsanitize=address,undefined'
LDFLAGS='-lncursesw'

OBJ='utility.o main.o'

set -xe

$CC $CFLAGS -c -o utility.o utility.c
$CC $CFLAGS -c -o main.o    main.c

$CC $CFLAGS -o cursed $OBJ $LDFLAGS
