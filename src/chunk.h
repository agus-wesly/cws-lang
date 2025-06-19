#ifndef CWS_CHUNK_H
#define CWS_CHUNK_H

#include "common.h"
#include "line.h"
#include "long_value.h"
#include "std.h"

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
    OP_DOT_GET,
    OP_DOT_SET,
    OP_SQR_BRACKET_GET,
    OP_SQR_BRACKET_SET,
    OP_MULTIPLY,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,

    OP_PRINT,
    OP_COMPARE,
    OP_POP,
    OP_GLOBAL_VAR,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,

    OP_MARK_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_SWITCH_JUMP,
    OP_JUMP,
    OP_LOOP,
    OP_SWITCH,
    OP_CASE_COMPARE,

    OP_CALL,
    OP_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,

    OP_CLASS,
    OP_METHOD,
    OP_DEL,

    OP_INIT_TABLE,
    OP_INIT_ARRAY,
} OpCode;

typedef struct
{
    uint16_t capacity;
    uint16_t count;

    uint8_t *code;

    Values *constants;
    LongValues *constantsLong;
    Lines *lines;

} Chunk;

#define READ4BYTE(offset)                                                                                              \
    ({                                                                                                                 \
        uint32_t operand = 0;                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            for (size_t i = 0; i < 4; ++i)                                                                             \
            {                                                                                                          \
                uint32_t byt = chunk->code[offset++];                                                                  \
                operand = operand | (byt << (8 * (3 - i)));                                                            \
            }                                                                                                          \
        } while (false);                                                                                               \
        operand;                                                                                                       \
    })

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t newItem, uint32_t line);
void print_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
int find_line(Chunk *chunk, int offset);
uint32_t get_line(Chunk *chunk, uint8_t idx);
uint8_t add_constant(Chunk *chunk, Value newConstant);
uint32_t add_long_constant(Chunk *chunk, Value constant);

void emit_constant(Chunk *chunk, Value value, uint32_t lineNumber);
void make_constant(Chunk *chunk, Value value, uint32_t lineNumber);

#endif // !CWS_CHUNK_H
