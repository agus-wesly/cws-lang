#ifndef CWS_CHUNK_H
#define CWS_CHUNK_H

#include "common.h"
#include "line.h"
#include "long_value.h"
#include "std.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_RETURN,
    OP_NEGATE,
    OP_BANG,
    OP_TERNARY,
    OP_GREATER,
    OP_LESS,
    OP_EQUAL_EQUAL,
    OP_ADD,
    OP_SUBTRACT,
    OP_DIVIDE,
    OP_MULTIPLY,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_PRINT
} OpCode;

typedef struct
{
    uint8_t capacity;
    uint8_t count;

    uint8_t *code;

    Values *constants;
    LongValues *constantsLong;
    Lines *lines;

} Chunk;

void InitChunk(Chunk *chunk);
void WriteChunk(Chunk *chunk, uint8_t newItem, uint32_t line);
void PrintChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);
int FindLine(Chunk *chunk, int offset);
uint8_t AddConstant(Chunk *chunk, Value newConstant);


void WriteConstant(Chunk *chunk, Value value, uint32_t lineNumber);

#endif // !CWS_CHUNK_H
