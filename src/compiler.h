#include "chunk.h"

#ifndef CWS_COMPILER_H

int compile(const char *code, Chunk *chunk);

typedef enum {
    PRESENCE_NONE,
    PRESENCE_ASSIGNMENT,
    PRESENCE_EQUALITY,
    PRESENCE_COMPARISON,
    PRESENCE_TERM,
    PRESENCE_FACTOR,
    PRESENCE_UNARY,
    PRESENCE_PRIMARY,
} Presedence;

typedef void (*Function)();

typedef struct {
    Function prefix;
    Function infix;
    Presedence presedence;
} Table;

void parsePresedence(Presedence presedence);

#endif // !CWS_COMPILER_H
