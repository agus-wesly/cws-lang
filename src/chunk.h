#include "common.h"
#include "line.h"
#include "long_value.h"
#include "std.h"
#include "value.h"

#ifndef CWS_CHUNK_H
#define CWS_CHUNK_H

typedef enum
{
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_RETURN,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_DIVIDE,
    OP_MULTIPLY,
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
uint8_t AddConstant(Chunk *chunk, Value newConstant);

void DisassembleChunk(Chunk *chunk, const char *title);
int DisassembleInstruction(Chunk *chunk, int offset);

void WriteConstant(Chunk *chunk, Value value, uint32_t lineNumber);

#endif // !CWS_CHUNK_H
