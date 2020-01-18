/* anewkirk */

#pragma once

#include "bool.h"
#include <stdlib.h>

typedef struct _block_header {
  size_t size;
  bool free;
  bool mark;
  struct _block_header *next;
  struct _block_header *prev;
} BlockHeader;

extern BlockHeader *head;
extern BlockHeader *tail;

/*
 * Allocates a block of memory of the size specified. Returns NULL on failure.
 */
void *lilac_malloc(size_t size);

/*
 * Re-allocates a block of memory to be of the specified size.
 */
void *lilac_realloc(void *ptr, size_t size);

/*
 * Allocates a block of memory for an array of length 'count' with each item occupying 'size' bytes.
 */
void *lilac_calloc(size_t count, size_t size);

/*
 * Prints to stdout a list of blocks of allocated memory and their properties 
 */
void print_block_list();

void collect();

void init_gc(char *argv_pointer[]);

void update_gc_threshold(long mb);

void purge();

/*
 * Frees a block of memory previously allocated by lilac_malloc(), lilac_calloc(), or
 * lilac_realloc(). No operation performed if passed pointer is null. UB if pointer
 * does not point to a block previously allocated by the listed functions.
 */
static void lilac_free(void *ptr);

static void mark();

static void sweep();

static void mark_range(uint64_t *start, uint64_t *end);

static void mark_block(BlockHeader *block);

static void segv_handler();

static void exit_handler();

static void init_block_header(BlockHeader *bh, size_t size);
