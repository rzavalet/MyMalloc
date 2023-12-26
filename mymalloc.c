/*
 * mymalloc.c
 * Copyright (C) 2023 rzavalet <rzavalet@noemail.com>
 *
 * Distributed under terms of the MIT license.
 *
 * This is my attempt on how to implement a memory allocator.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

/*-----------------------------------------
 * The Heap struct in the top most structure. It contains a pointer to the
 * memory area that is being managed.
 *---------------------------------------*/
typedef struct
{
  size_t  size;
  size_t  available_size;
  void   *start_of_heap;
} Heap;

/*------------------------------------
 * The important metadata for an allocated block is:
 * 1) Size
 * 2) Start
 * 3) Next block
 * 4) Whether the block is free
 *
 * In this allocator, I chose not to declare explicit fields for (1) and (4).
 * (1) can be derived using pointer arithmetic (e.g. start of the next block
 * minus the start of data of the current block. (4) is encoded as the last bit
 * in the `next` field (3). If the bit is ON, then this block is used.
 *
 *----------------------------------*/
typedef struct
{
  void  *next;
} allocHdr;

typedef struct
{
  allocHdr  hdr;
  char      start[1];
} allocation;

#define MY_ALIGNMENT_SZ   (0x00000008)
#define IN_USE_BIT        (0x00000001)

#define SZ_HEAP_METADATA    sizeof(Heap)
#define SZ_BLOCK_METADATA   sizeof(allocHdr)

#define ALIGN(_x)         ((_x + MY_ALIGNMENT_SZ) & ~(MY_ALIGNMENT_SZ - 1))

#define UNSET_INUSE(_x)   (_x->hdr.next = (void *)((uintptr_t)_x->hdr.next & ~IN_USE_BIT))
#define SET_INUSE(_x)     (_x->hdr.next = (void *)((uintptr_t)_x->hdr.next | IN_USE_BIT))
#define IS_INUSE(_x)      (int)((uintptr_t)(_x->hdr.next) & IN_USE_BIT)
#define NEXT_ADDRESS(_x)  (void *)(((uintptr_t) _x->hdr.next) & ~IN_USE_BIT) 

#define BLOCK_SIZE(_x)    ((uintptr_t) NEXT_ADDRESS(_x)- (uintptr_t)&_x->start)

#ifdef DEBUG
#define CHECK_HEAP()  check_heap()
#define DPRINTF(...)   printf(__VA_ARGS__)
#else
#define CHECK_HEAP()
#define DPRINTF(...)
#endif

Heap _h;

#ifdef DEBUG
/*-----------------------------------------
 * Debugging function to explore the entire heap, printing relevant
 * information.
 *---------------------------------------*/
static void check_heap(void)
{
  int cnt;
  allocation *tmp;

  printf("Size of heap: %zu\n", _h.size);
  printf("Size of allocation metadata: %zu\n", SZ_BLOCK_METADATA);
  printf("Available size: %zu\n", _h.available_size);
  printf("Start of heap: %p\n", _h.start_of_heap);

  for (tmp = _h.start_of_heap, cnt = 0; tmp; tmp = NEXT_ADDRESS(tmp), cnt++) {
    size_t alloc_size = 0;
    if (NEXT_ADDRESS(tmp) == NULL) {
      alloc_size = (size_t) ((uintptr_t)_h.start_of_heap + _h.size - (uintptr_t)(tmp->start));
    }
    else {
      alloc_size = (size_t) ((uintptr_t)NEXT_ADDRESS(tmp) - (uintptr_t)(tmp->start));
    }
    printf("\t[%p] {in_use: %d, alloc_size: %zu, user_address: %p, next: %p}\n", 
        tmp, IS_INUSE(tmp), alloc_size, tmp->start, NEXT_ADDRESS(tmp));
  }

  printf("Allocated blocks: %d\n", cnt);
  printf("--------------------------------\n\n");
}
#endif


/*-----------------------------------------
 * If there are `n` contiguous free blocks, the allocator can go ahead and
 * compact them. Just make sure that the resulting `free` block points to
 * whatever the last coalesced block was previously pointing to.
 *---------------------------------------*/
static void do_compact(allocation *block)
{
  allocation *last;

  DPRINTF("Running compact!\n");
  for (last = block; last && !IS_INUSE(last); last = NEXT_ADDRESS(last));
  
  block->hdr.next = last;
}



/*-----------------------------------------
 * Upon the first allocation, go ahead and initialize the heap.
 * That implies requesting more memory to the OS (see sbrk).
 *---------------------------------------*/
static void init_heap(Heap *h)
{

  h->size = h->available_size = 4096;
  h->start_of_heap = sbrk(h->size);

  ((allocation *)h->start_of_heap)->hdr.next = NULL;

  DPRINTF("Start from data segment: %p\n", h->start_of_heap);
}


/*-----------------------------------------
 * Utility function to mark all allocated bloks as `unused`.
 *---------------------------------------*/
void reset_heap()
{
  allocation *tmp;
  for (tmp = _h.start_of_heap; tmp; tmp = NEXT_ADDRESS(tmp)) {
    UNSET_INUSE(tmp);
  }

  do_compact(_h.start_of_heap);
}


/*-----------------------------------------
 * Malloc:
 * Finds the first fit for the requested memory size. If the found block is
 * bigger, the it is split. The returned size is always a multiple of the word
 * size.
 *---------------------------------------*/
void *mymalloc(size_t size)
{
  size_t aux_size = ALIGN(size + SZ_BLOCK_METADATA);

  DPRINTF("[Requested %zu bytes, which becomes: %zu bytes + %zu bytes]\n", 
      size, SZ_BLOCK_METADATA, aux_size - SZ_BLOCK_METADATA);

  if (_h.size <= 0) {
    init_heap(&_h);
  }

  if (aux_size > _h.available_size) {
    assert("Not enough space" == NULL);
    return NULL;
  }

  // Search for the first block that fits the requested size.
  allocation *tmp = _h.start_of_heap;
  for (;tmp; tmp = NEXT_ADDRESS(tmp)) {
    if (!IS_INUSE(tmp)) {
      do_compact(tmp);

      if (BLOCK_SIZE(tmp) >= aux_size)
        break;
    }
  }

  assert(tmp);

  // A candidate block was found. Now decide whether it needs splitting.  The
  // block needs to be at least `aux_size + SZ_BLOCK_METADATA + word_size`.
  // Otherwise, just hand the entire block.
  if (BLOCK_SIZE(tmp) >= aux_size + SZ_BLOCK_METADATA + ALIGN(1)) {
    allocation *tmp2 = (void *)((uintptr_t) tmp + aux_size);
    tmp2->hdr.next = tmp->hdr.next;
    UNSET_INUSE(tmp2);

    tmp->hdr.next = tmp2;
  }

  SET_INUSE(tmp);

  _h.available_size -= aux_size;
  CHECK_HEAP();

  return tmp->start;
}

/*-----------------------------------------
 * Free:
 * Just need to unset the `in-use` flag.
 *---------------------------------------*/
void myfree(void *ptr)
{
  if (ptr == NULL) return;

  allocation *tmp = (void *)((uintptr_t) ptr - SZ_BLOCK_METADATA);

#ifdef DEBUG
  size_t alloc_size = (size_t) ((uintptr_t)NEXT_ADDRESS(tmp) - (uintptr_t)&(tmp->start));
  DPRINTF("Freeing block at address: %p of size: %zu\n", tmp, alloc_size);
#endif

  UNSET_INUSE(tmp);
  do_compact(tmp);
  CHECK_HEAP();
}

