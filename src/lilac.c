/* anewkirk */

#include "bool.h"
#include "mutex.h"
#include "lilac.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <stdint.h>

extern uint8_t etext;
extern uint8_t end;
BlockHeader *head;
BlockHeader *tail;
uint8_t *argv_ptr;
jmp_buf segv_jmp;
volatile lilac_mutex global_allocator_mutex = 0;
uint64_t gc_threshold = 1048576 * 10;
uint64_t heap_size = 0;

void init_gc(char *argv_pointer[]) {
  //Initialize argv_ptr so Lilac can find the end of the stack
  argv_ptr = (uint8_t*)argv_pointer;

  //Set up atexit() handler to purge heap
  int8_t r = atexit(exit_handler);
  if(r) {
    exit(EXIT_FAILURE);
  }
}

void update_gc_threshold(long mb) {
  gc_threshold = 1048576 * mb;
}

static void segv_handler() {
  longjmp(segv_jmp, 1);
}

static void exit_handler() {
  purge();
}

void purge() {
  //Iterate the allocated blocks from tail to head so memory
  //is freed back to the OS
  BlockHeader *curr = tail;
  while(curr) {
    BlockHeader *prev = curr->prev;
    BlockHeader *block_start = curr + 1;
    lilac_free(block_start);
    curr = prev;
  }
  head = NULL;
  tail = NULL;
}

static void mark() {
  //Scan stack
  uint64_t *sp = NULL;
  __asm__("movq %%rsp, %0"
	  :"=r"(sp));
  mark_range((uint64_t*)argv_ptr, sp);

  //Scan globals
  mark_range((uint64_t*)&end, (uint64_t*)&etext);
}

static void sweep() {
  //Iterate the block list from tail to head and free
  //any block that isn't marked
  BlockHeader *curr = tail;
  while(curr) {
    BlockHeader *prev = curr->prev;
    if(!curr->mark) {
      lilac_free(curr + 1);
    } else {
      curr->mark = false;
    }
    curr = prev;
  }
}

void collect() {
  acquire_lock(&global_allocator_mutex);
  mark();
  sweep();
  release_lock(&global_allocator_mutex);
}

static void init_block_header(BlockHeader *bh, size_t size) {
  bh->next = NULL;
  bh->prev = NULL;
  bh->size = size;
  bh->free = false;
  bh->mark = false;
}

void *lilac_malloc(size_t size) {
  //No need to allocate for negative or zero size
  if(size <= 0) {
    return NULL;
  }

  //Take ownership of the mutex to prevent race conditions
  acquire_lock(&global_allocator_mutex);
  
  //Iterate the allocated block list to see if there is a block that is free
  //that can accommodate the requested size
  BlockHeader *curr = head;
  while(curr) {
    if(curr->size >= size && curr->free == true) {
      curr->free = false;
      return curr + 1;
    }
    curr = curr->next;
  }

  //Otherwise, make a new one
  size_t total_size = sizeof(BlockHeader) + size;

  //Keep track of how much memory has been allocated to trigger GC
  heap_size += total_size;

  //Collect if the heap size threshold is met or exceeded
  if(heap_size >= gc_threshold) {
    //Release mutex to GC
    release_lock(&global_allocator_mutex);
    collect();
    acquire_lock(&global_allocator_mutex);
  }
  
  BlockHeader *new_header = (void*)sbrk(total_size);
  
  //Terminate process on failure
  if(new_header == (void*)-1) {
    exit(EXIT_FAILURE);
  }

  //Set up the new block header
  init_block_header(new_header, size);

  //Update linked list to reflect new block
  if(head == NULL) {
    head = new_header;
  }
 
  if(tail == NULL) {
    tail = new_header;
  } else {
    tail->next = new_header;
    new_header->prev = tail;
    tail = new_header;
  }

  //Release the mutex for other threads to use
  release_lock(&global_allocator_mutex);
  
  return new_header + 1;
}

void *lilac_calloc(size_t count, size_t size) {
  size_t total_size = count * size;
  if(total_size <= 0) {
    return NULL;
  }
  
  void *block = lilac_malloc(total_size);
  if(block == NULL) {
    return NULL;
  }

  memset(block, 0, total_size);
  return block;
}

void *lilac_realloc(void* ptr, size_t size) {
  if(ptr == NULL) {
    return NULL;
  }

  //If the block is already big enough to accommodate the requested size,
  //return a pointer to the same block
  BlockHeader *header = (BlockHeader*)ptr - 1;
  if(header->size >= size) {
    return ptr;
  }

  //Otherwise, allocate a new block, copy the contents over from the old one,
  //and free the old block
  void *new_block = lilac_malloc(size);
  memcpy(new_block, ptr, header->size);
  lilac_free(ptr);
  
  return new_block;
}

static void lilac_free(void *ptr) {
  //No action required for a NULL pointer
  if(ptr == NULL) {
    return;
  }

  //Calculate pointers to start of header and end of allocated block
  BlockHeader *header = (BlockHeader*)ptr - 1;
  void* end_of_block = ptr + header->size;

  size_t total_size = end_of_block - (void*)header;

  //Keep track of how much memory is allocated to trigger GC
  heap_size -= total_size;
  
  //Release the memory back to the OS if the freed block is
  //at the end of the heap
  if(end_of_block == sbrk(0)) {
    tail = header->prev;

    if(header->prev != NULL) {
      header->prev->next = NULL;
    }

    if(head == header) {
      head = header->next;
    }

    //Terminate process on failure
    void *r = sbrk(-1 * total_size);
    if(r == (void*)-1) {
      exit(EXIT_FAILURE);
    }
  } else {
    //Otherwise, simply mark it as free for later use
    header->free = true;
  }
}

static void mark_block(BlockHeader *block) {
  //Avoid re-scanning a block that's already been scanned
  if(block->mark) {
    return;
  }
  block->mark = true;
  
  uint64_t *start = (uint64_t*)(block + 1);

  //Cast start to uint8_t* to add an offset in bytes
  uint64_t *end = (uint64_t*)((uint8_t*)start + block->size);

  //Scan contents of block and compare to each allocated block start
  while(end >= start) {
    BlockHeader *curr = head;
    while(curr) {
      uint64_t *curr_start = (uint64_t*)(curr + 1);
      if(curr_start == (uint64_t*)*end) {
	//Recurse on any block that a pointer is found for
	mark_block(curr);
      }
      curr = curr->next;
    }

    //Decrement end ptr by one byte
    end = (uint64_t*)(((uint8_t*)end) - 1);
  }
}

static void mark_range(uint64_t *end, uint64_t *start) {
  signal(SIGSEGV, segv_handler);
  
  if(!setjmp(segv_jmp)) {
    while(end >= start) {
      BlockHeader *curr = head;

      while(curr) {
	uint64_t *block_start = (uint64_t*)(curr + 1);
	if(block_start == (uint64_t*)*end) {
	  
	  //Set SIGSEGV handler to default before calling mark_block
	  //so legitimate segfaults aren't masked
	  signal(SIGSEGV, SIG_DFL);
	  mark_block(curr);
	  
	  //Set it back to segv_handler for the next iterations
	  signal(SIGSEGV, segv_handler);
	  } 
	curr = curr->next;
	
      }
      end = (uint64_t*)(((uint8_t*)end) - 1);
    }
  } else {
    signal(SIGSEGV, SIG_DFL);
  }
}

void print_block_list() {
  BlockHeader *curr = head;
  while(curr) {
    printf("%p: size = %d, free=%d, mark=%d, next=%p\n\n", curr, curr->size, curr->free, curr->mark, curr->next);
    curr = curr->next;
  }
}

