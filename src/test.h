/* anewkirk */

#pragma once

#include "lilac.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ERRBUFLEN 1000
#define STRINGIFY(X) #X

uint32_t tests_run;
uint32_t asserts_run;

#define li_assert(condition) do {					\
    asserts_run++;							\
    printf("    [+] assert %d: %s\n",					\
	   asserts_run, STRINGIFY(condition));				\
    if (!(condition)) {							\
      uint8_t *m = lilac_calloc(ERRBUFLEN, 1);				\
      snprintf(m, ERRBUFLEN,						\
	       "    Error occurred in test #%d, assert #%d\n\n",	\
	       tests_run, asserts_run);					\
      return m;								\
    }									\
  } while (0)

#define li_run_test(test) do {						\
    asserts_run = 0;							\
    tests_run++;							\
    printf("  [!] Running test %d: %s\n", tests_run, STRINGIFY(test));	\
    uint8_t *message = test();						\
    if (message) {							\
      printf("[!] Failure! %s did not complete successfully.\n%s\n",	\
	     STRINGIFY(test), message);					\
      return message;							\
    }									\
  } while (0)

#define li_run_suite(suite_name) do {		\
    puts("[+] Testing "suite_name"...");	\
    uint8_t *result = all_tests();		\
    if(!result) {				\
      puts("[+] " suite_name " tests OK");	\
    }						\
    printf("Tests run: %d\n\n", tests_run);	\
    purge();					\
    return result != 0;				\
  } while (0)
