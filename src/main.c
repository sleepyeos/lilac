#include "mutex.h"
#include "lilac.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void *a;

int main(int argc, char *argv[]) {
  init_gc(argv);
  update_gc_threshold(1);

  a = lilac_malloc(200000);
  lilac_malloc(32);
  
  uint64_t **b = lilac_malloc(sizeof(uint64_t *));
  *b = lilac_malloc(sizeof(uint64_t));
  **b = 23424234;
  lilac_malloc(23443);
  print_block_list();

  printf("Collecting...\n");
  collect();
  print_block_list();
  
}
