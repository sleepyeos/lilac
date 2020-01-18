/* anewkirk */

#pragma once

#include <stdint.h>

typedef int8_t lilac_mutex;

// Takes ownership of the mutex; returns 0 on success, 1 on failure
void acquire_lock(volatile int8_t *lock);

// Releases ownership of the mutex
void release_lock(volatile int8_t *lock);


