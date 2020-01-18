/* anewkirk */

#include "mutex.h"
#include <stdio.h>

void acquire_lock(volatile int8_t *lock) {
  int result = -1;
  while(result != 0) {
    __asm__("movl $1, %%ebx;\n\t"
	    "lock xchg %%ebx, %0;\n\t"
	    "movl %%ebx, %1"
	    :"+m"(*lock),"=a"(result));
  }
}

void release_lock(volatile int8_t *lock) {
  __asm__("movl $0, %%ebx;\n\t"
	  "lock xchg %%ebx, %0"
	  :"+m"(*lock));
}
