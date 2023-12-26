/*
 * mymalloc.h
 * Copyright (C) 2023 rzavalet <rzavalet@noemail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MYMALLOC_H
#define MYMALLOC_H

#include <stdio.h>

void *mymalloc(size_t size);
void myfree(void *ptr);
void reset_heap();

#endif /* !MYMALLOC_H */
