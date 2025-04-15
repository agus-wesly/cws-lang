#ifndef CWS_COMPILER_H
#define CWS_COMPILER_H

#include "chunk.h"
#include "scanner.h"
#include "debug.h"
#include "value.h"

int compile(const char *code, Chunk *chunk);

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_TERNARY,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;



#endif // !CWS_COMPILER_H
