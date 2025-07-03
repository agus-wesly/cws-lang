#ifndef ws_common_h
#define ws_common_h
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ENABLE_GC
#define NAN_BOXING

#ifndef __EMSCRIPTEN__
// #define DEBUG_TRACE_EXECUTION
// #define TEST_STRESS_GC
// #define DEBUG_GC
// #define DEBUG_PRINT

#endif // __EMSCRIPTEN__

#endif // !ws_common_h
