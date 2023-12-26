#! /bin/sh
#
# build.sh
# Copyright (C) 2023 rzavalet <rzavalet@noemail.com>
#
# Distributed under terms of the MIT license.
#


CC=clang
CFLAGS="-Wall -ggdb -fsanitize=address"
#CFLAGS="-Wall -ggdb"

${CC} ${CFLAGS} -DDEBUG -fpic -shared -o libmymalloc.so mymalloc.c

${CC} ${CFLAGS} -L. -o main main.c -lmymalloc
