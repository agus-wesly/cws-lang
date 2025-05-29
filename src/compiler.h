#ifndef CWS_COMPILER_H
#define CWS_COMPILER_H

#include "chunk.h"
#include "debug.h"
#include "scanner.h"
#include "value.h"

ObjectFunction *compile(const char *code);

typedef enum
{
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

typedef void (*ParseFn)(int can_parse_set);

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct
{
    int idx;
    int depth;
} Jump;

typedef struct
{
    int offset;
    int depth;
} Loop;


void mark_compiler();

#endif // !CWS_COMPILER_H
