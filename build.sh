#!/bin/bash

CC='gcc'
CFLAGS='-Wall -Wextra -pedantic -std=c89 -ggdb -fsanitize=address'
LDFLAGS='-lncurses'

set -xe

$CC $CFLAGS -o cursed main.c $LDFLAGS
