/*
 * Kacper Chmielewski 332606
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  Blocks are never coalesced or reused.  The size of
 * a block is found at the first aligned word before the block (we need
 * it for realloc).
 *
 * This code is correct and blazingly fast, but very bad usage-wise since
 * it never frees anything.
 *
 *
 * Opis działania programu znajduje się w pliku Opis.md w tym repozytorium
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

typedef struct {
  int32_t header;
  /*
   * We don't know what the size of the payload will be, so we will
   * declare it as a zero-length array.  This allow us to obtain a
   * pointer to the start of the payload.
   */
  uint8_t payload[];
} block_t;
///////////////////////////////////
// funkcje z mm-implicit
typedef int32_t word_t; /* Heap is bascially an array of 4-byte words. */

typedef enum {
  FREE = 0,     /* Block is free */
  USED = 1,     /* Block is used */
  PREVFREE = 2, /* Previous block is free (optimized boundary tags) */
} bt_flags;

static word_t *heap_start;    /* Address of the first block */
static word_t *heap_end;      /* Address past last byte of last block */
static word_t *last;          /* Points at last block */
static word_t *lastAllocated; /* Points to last allocated block*/

/* --=[ boundary tag handling ]=-------------------------------------------- */

static inline word_t bt_size(word_t *bt) {
  return *bt & ~(USED | PREVFREE);
}

static inline word_t bt_used_flags(word_t *bt) {
  return *bt & (USED | PREVFREE);
}

static inline int bt_used(word_t *bt) {
  return *bt & USED;
}

static inline int bt_free(word_t *bt) {
  return !(*bt & USED);
}

/* Returns address of next block or NULL. */
static inline word_t *bt_next(word_t *bt) {
  return (void *)bt + bt_size(bt);
}

/* Returns address of previous block or NULL. */
static inline word_t *bt_prev(word_t *bt) {
  return (void *)bt - bt_size(bt - 1);
}

static inline word_t *bt_footer(word_t *bt) {
  return (void *)bt + bt_size(bt) - sizeof(word_t);
}

static inline int bt_get_prevfree(word_t *bt) {
  return *bt & PREVFREE;
}

static inline void bt_clr_prevfree(word_t *bt) {
  if (bt)
    *bt &= ~PREVFREE;
}

static inline void bt_set_prevfree(word_t *bt) {
  *bt |= PREVFREE;
} // bt_set_prevfree(bt_next(block));

/* Given payload pointer returns an address of boundary tag. */
static inline word_t *bt_fromptr(void *ptr) {
  return (word_t *)ptr - 1;
}

static void set_size_and_flags(word_t *block, size_t size, int is_allocated) {
  *block = size | is_allocated;
}

/* Creates boundary tag(s) for given block. */
static inline void bt_make(word_t *bt, size_t size, int flags) {
  set_size_and_flags(bt, size, flags);
  set_size_and_flags(bt_footer(bt), size, flags);
}

/* Returns address of payload. */
static inline void *bt_payload(word_t *bt) {
  return bt + 1;
}

static void *morecore(size_t size) {
  void *ptr = mem_sbrk(size);
  if (ptr == (void *)-1)
    return NULL;
  return ptr;
}

static size_t round_up(size_t size) {
  return (size + ALIGNMENT - 1) & -ALIGNMENT;
}

/*
 * mm_init - Called when a new trace starts.

 */

int mm_init(void) {
  /* Pad heap start so first payload is at ALIGNMENT. */
  if ((long)mem_sbrk(ALIGNMENT - offsetof(block_t, payload)) < 0)
    return -1;

  heap_start = morecore(0);
  last = heap_start;
  heap_end = last;
  lastAllocated = heap_start;
  return 0;
}

/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */

static inline word_t *findfit(size_t size) {
  word_t *block = lastAllocated;

  while (block != heap_end) {
    if (bt_free(block) && bt_size(block) >= size) {
      return block;
    }
    block = bt_next(block);
  }

  block = heap_start;

  while (block != lastAllocated) {
    if (bt_free(block) && bt_size(block) >= size) {
      return block;
    }
    block = bt_next(block);
  }
  return NULL;
}

static inline void split_block(word_t *act_heap, size_t size) {
  size_t newSize = bt_size(act_heap) - size;
  set_size_and_flags(act_heap, size, USED);
  bt_make(bt_next(act_heap), newSize, FREE);
  if (act_heap == last)
    last = bt_next(act_heap);
}

void *malloc(size_t size) {
  size = round_up(4 + size);
  word_t *block = findfit(size);
  if (block == NULL) {
    block = morecore(size);
    // bt_make(block, size, USED);
    set_size_and_flags(block, size, USED);

    if (bt_free(last))
      bt_set_prevfree(block);

    last = block;
    heap_end = bt_next(last);
  }

  else {
    if (size == bt_size(block)) {
      set_size_and_flags(block, size, USED);
      if (bt_next(block) != heap_end) {
        bt_clr_prevfree(bt_next(block));
      }
    } else {
      split_block(block, size);
    }
  }
  mm_checkheap(1);
  lastAllocated = block;
  return bt_payload(block);
}

/*
 * free - We don't know how to free a block.  So we ignore this call.
 *      Computers have big memories; surely it won't be a problem.
 */
static inline void merge_free_blocks(word_t *bt) {
  word_t *next = bt_next(bt);
  if (next != heap_end) {
    if (bt_free(next)) {
      bt_make(bt, bt_size(bt) + bt_size(next), FREE | bt_get_prevfree(bt));
      if (next == last)
        last = bt;
      if (next == lastAllocated)
        lastAllocated = bt;
    }
  }

  if (bt != heap_start) {
    if (bt_get_prevfree(bt)) {
      word_t *prev = bt_prev(bt);
      if (bt == last)
        last = prev;
      if (bt == lastAllocated)
        lastAllocated = prev;
      bt_make(prev, bt_size(bt) + bt_size(prev), FREE);
    }
  }
  if (bt_next(bt) != heap_end)
    bt_set_prevfree(bt_next(bt));
}

void free(void *ptr) {
  if (ptr == NULL)
    return;
  word_t *block = bt_fromptr(ptr);
  bt_make(block, bt_size(block), FREE | bt_get_prevfree(block));
  merge_free_blocks(block);
  mm_checkheap(0);
}

/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.
 **/
void *realloc(void *old_ptr, size_t size) {
  /* If size == 0 then this is just free, and we return NULL. */
  if (size == 0) {
    free(old_ptr);
    return NULL;
  }

  /* If old_ptr is NULL, then this is just malloc. */
  if (!old_ptr)
    return malloc(size);

  word_t *block = bt_fromptr(old_ptr);
  size_t old_size = bt_size(block);
  void *new_ptr = malloc(size);

  /* If malloc() fails, the original block is left untouched. */
  if (!new_ptr)
    return NULL;

  /* Copy the old data. */
  if (size < old_size)
    old_size = size;
  memcpy(new_ptr, old_ptr, old_size);

  /* Free the old block. */
  free(old_ptr);

  return new_ptr;
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc(size_t nmemb, size_t size) {
  size_t bytes = nmemb * size;
  void *new_ptr = malloc(bytes);

  /* If malloc() fails, skip zeroing out the memory. */
  if (new_ptr)
    memset(new_ptr, 0, bytes);

  return new_ptr;
}

/*
 * mm_checkheap - So simple, it doesn't need a checker!
 */
void mm_checkheap(int verbose) {
  int i = 0;
  return;
  printf("typ operacji = %d: heap_start = %p, heap_end = %p, last = %p \n",
         verbose, heap_start, heap_end, last);

  word_t *ind = heap_start;
  while (ind != heap_end) {
    printf("Numer bloku = %d, flagi = %d, size = %d, pointer = %p \n", i,
           bt_used_flags(ind), bt_size(ind), ind);
    i++;
    ind = bt_next(ind);
  }
  putchar('\n');
}