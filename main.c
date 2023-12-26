/*
 * main.c
 * Copyright (C) 2023 rzavalet <rzavalet@noemail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mymalloc.h"

typedef struct
{
  int (*fp)(void);
} Test;

/* Allocate until space is exhausted */
int test0()
{
  char *c;
  int cnt = 100;

  while ((c = mymalloc(10)) != NULL && cnt -- > 0) {
    printf("Allocated a chunk of memory: %p\n", c);

    if (rand() % 2 == 0) {
      myfree(c);
    }
  }

  return 0;
}

/* Allocate and verify blocks can be reused */
int test1()
{
  char *c1, *c2, *c3, *c4;

  c1 = mymalloc(10);
  c2 = mymalloc(10);
  c3 = mymalloc(10);
  c4 = mymalloc(10);

  myfree(c3);
  myfree(c1);
  myfree(c4);
  myfree(c2);

  c1 = mymalloc(10);

  return 0;
}

/* Allocate and see if compaction works */
int test2()
{
  char *c1, *c2, *c3, *c4;

  c1 = mymalloc(10);
  c2 = mymalloc(10);
  c3 = mymalloc(10);
  c4 = mymalloc(10);

  myfree(c3);
  c3 = mymalloc(20);

  myfree(c2);
  c2 = mymalloc(20);

  myfree(c1);
  c1 = mymalloc(20);

  return 0;
}

Test tests[] = 
{
  {test0},
  {test1},
  {test2},
};

int main(void)
{
  for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
    assert(tests[i].fp() == 0);
  }
  return 0;
}

