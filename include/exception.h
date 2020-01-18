/* anewkirk */

#pragma once
#include <setjmp.h>

#define try jmp_buf __jmp; if(!setjmp(__jmp))
#define catch else
#define finally
#define throw() longjmp(__jmp, 1)
