#ifndef CWS_COMPILER_H
#define CWS_COMPILER_H

#include "chunk.h"
#include "scanner.h"

int compile(const char *code, Chunk *chunk);

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} Presedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Presedence presedence;
} ParseRule;

static void expression();
static void parsePresedence(Presedence presedence);
static ParseRule *get_rule(TokenType token_type);


#endif // !CWS_COMPILER_H
