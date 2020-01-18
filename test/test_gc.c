/* anewkirk */

#include "../src/test.h"
#include "../src/lilac.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static uint8_t *test_malloc() {
  li_assert(1 == 2);
  return 0;
}

static uint8_t *all_tests() {
  li_run_test(test_malloc);
  return 0;
}

int main(void) {
  li_run_suite("GC Tests");
}
