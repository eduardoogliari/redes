#ifndef DEFS_H
#define DEFS_H

#include <stdlib.h>
#include <stdio.h>


// SAFE_FREE
#define SAFE_FREE(x) if(x) { free(x); x = NULL; }

// CLS
#define CLS() printf("\e[1;1H\e[2J");

#ifdef _DEBUG

// DBG_BREAK()
#if defined(__GNUC__) && (defined(__i386) || defined(__x86_64))
#define DBG_BREAK() asm volatile ("int $3");
#else
#include <signal.h>
#define DBG_BREAK() raise(SIGTRAP);
#endif

// ENSURE
#define ENSURE(x, ...) if((!x)) { printf( __VA_ARGS__ ); puts(""); DBG_BREAK(); }

#else
#define DBG_BREAK()
#define ENSURE(x, str, ...)
#endif

#endif