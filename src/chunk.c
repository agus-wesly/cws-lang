// Copyright 2025 Agustinus Wesly Sitanggang <agustchannel@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "chunk.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

void init_chunk(Chunk *chunk)
{
    chunk->capacity = 0;
    chunk->count = 0;

    chunk->code = NULL;

    Values *values = malloc(sizeof(Values));
    init_values(values);
    chunk->constants = values;

    LongValues *longValues = malloc(sizeof(LongValues));
    init_long_values(longValues);
    chunk->constantsLong = longValues;

    Lines *lines = malloc(sizeof(Lines));
    InitLines(lines);
    chunk->lines = lines;
}

void free_chunk(Chunk *chunk)
{
    free_values(chunk->constants);
    free_long_values(chunk->constantsLong);

    FreeLines(chunk->lines);

    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    init_chunk(chunk);
}

uint32_t add_long_constant(Chunk *chunk, Value constant)
{
    append_long_values(chunk->constantsLong, constant);
    return chunk->constantsLong->count - 1;
}

void make_constant(Chunk *chunk, Value value, uint32_t lineNumber)
{
    push(value);
    append_long_values(chunk->constantsLong, value);
    pop();

    uint32_t constantIndex = chunk->constantsLong->count - 1;
    for (size_t i = 0; i < 4; ++i)
    {
        uint8_t chunkIdx = (constantIndex >> (8 * (3 - i)));
        write_chunk(chunk, chunkIdx, lineNumber);
    }
}

void emit_constant(Chunk *chunk, Value value, uint32_t lineNumber)
{
    push(value);
    write_chunk(chunk, OP_CONSTANT_LONG, lineNumber);
    make_constant(chunk, value, lineNumber);
    pop();
}

void writeLine(Chunk *chunk, uint32_t lineNumber)
{
    if (chunk->lines->lines == NULL)
    {
        Line *line = malloc(sizeof(Line));
        InitLine(line, chunk->count, lineNumber);
        WriteLines(chunk->lines, line);
    }
    else
    {
        Line *prevLine = chunk->lines->lines[chunk->lines->count - 1];
        if (lineNumber != prevLine->number)
        {
            Line *line = malloc(sizeof(Line));
            InitLine(line, chunk->count, lineNumber);
            WriteLines(chunk->lines, line);
        }
    }
}

void write_chunk(Chunk *chunk, uint8_t newItem, uint32_t lineNumber)
{
    if (chunk->capacity < chunk->count + 1)
    {
        uint8_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    writeLine(chunk, lineNumber);

    chunk->code[chunk->count] = newItem;
    chunk->count++;
}

int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int constant_instruction(const char *name, Chunk *chunk, int offset)
{
    printf("%-20s %d ", name, offset);

    uint8_t constant = chunk->code[offset + 1];
    print_value(chunk->constants->values[constant], true, 0);
    printf("\n");

    return offset + 2;
}

int constantLongInstruction(const char *name, Chunk *chunk, int offset)
{
    ++offset;
    printf("%-20s %d ", name, offset);
    uint32_t operand = READ4BYTE(offset);

    print_value(chunk->constantsLong->values[operand], true, 0);
    printf("\n");

    return offset;
}

int get_local_instruction(const char *name, Chunk *chunk, int offset)
{

    printf("%-20s %d ", name, offset);
    uint32_t operand = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        uint32_t byt = chunk->code[offset + 1 + i];
        operand = operand | (byt << (8 * (3 - i)));
    }
    printf("%d", operand);
    printf("\n");

    return offset + 1 + 4;
}

int jump_instruction(const char *name, int sign, Chunk *chunk, int offset)
{

    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-20s %d -> %d", name, offset, offset + 3 + sign * jump);
    printf("\n");

    return offset + 3;
}

uint32_t get_line(Chunk *chunk, uint8_t idx)
{
    for (int i = chunk->lines->count - 1; i >= 0; --i)
    {
        Line *line = chunk->lines->lines[i];
        if (idx >= line->idx)
        {
            return line->number;
        }
    }
    return -1;
}

int find_line(Chunk *chunk, int offset)
{
    for (int i = chunk->lines->count - 1; i >= 0; --i)
    {
        Line *line = chunk->lines->lines[i];
        if (offset >= line->idx)
        {
            return line->number;
        }
    }
    assert(0 && "Unreachable at find line");
}

int disassemble_instruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);
    for (int i = chunk->lines->count - 1; i >= 0; --i)
    {
        Line *line = chunk->lines->lines[i];
        if (offset > line->idx)
        {
            printf("   | ");
            break;
        }
        else if (offset == line->idx)
        {
            printf("%4d ", line->number);
            break;
        }
    }

    uint8_t current = chunk->code[offset];
    switch (current)
    {
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    case OP_CONSTANT:
        return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
        return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);

    case OP_TRUE: {
        return simple_instruction("OP_TRUE", offset);
    }
    case OP_FALSE: {
        return simple_instruction("OP_FALSE", offset);
    }

    case OP_NIL: {
        return simple_instruction("OP_NIL", offset);
    }

    case OP_ADD: {
        return simple_instruction("OP_ADD", offset);
    }
    case OP_DOT_GET: {
        return constantLongInstruction("OP_GET_FIELD", chunk, offset);
    }
    case OP_DOT_SET: {
        return constantLongInstruction("OP_SET_FIELD", chunk, offset);
    }
    case OP_SQR_BRACKET_GET: {
        return simple_instruction("OP_GET_FIELD_B", offset);
    }
    case OP_SQR_BRACKET_SET: {
        return simple_instruction("OP_SET_FIELD_B", offset);
    }
    case OP_DEL:
        return simple_instruction("OP_DEL", offset);
    case OP_SUBTRACT: {
        return simple_instruction("OP_SUBTRACT", offset);
    }
    case OP_MULTIPLY: {
        return simple_instruction("OP_MULTIPLY", offset);
    }
    case OP_DIVIDE: {
        return simple_instruction("OP_DIVIDE", offset);
    }
    case OP_GREATER: {
        return simple_instruction("OP_GREATER", offset);
    }
    case OP_LESS: {
        return simple_instruction("OP_LESS", offset);
    }
    case OP_EQUAL_EQUAL: {
        return simple_instruction("OP_EQUAL_EQUAL", offset);
    }

    case OP_NEGATE: {
        return simple_instruction("OP_NEGATE", offset);
    }
    case OP_BANG: {
        return simple_instruction("OP_BANG", offset);
    }
    case OP_TERNARY: {
        return simple_instruction("OP_TERNARY", offset);
    }
    case OP_PRINT: {
        return simple_instruction("OP_PRINT", offset);
    }
    case OP_POP: {
        return simple_instruction("OP_POP", offset);
    }
    case OP_CLOSE_UPVALUE: {
        return simple_instruction("OP_TAKE", offset);
    }
    case OP_GLOBAL_VAR: {
        return constantLongInstruction("OP_GLOBAL_VAR", chunk, offset);
    }
    case OP_GET_GLOBAL: {
        return constantLongInstruction("OP_GET_GLOBAL", chunk, offset);
    }
    case OP_SET_GLOBAL: {
        return constantLongInstruction("OP_SET_GLOBAL", chunk, offset);
    }
    case OP_GET_LOCAL: {
        return get_local_instruction("OP_GET_LOCAL", chunk, offset);
    }
    case OP_SET_LOCAL: {
        return constantLongInstruction("OP_SET_LOCAL", chunk, offset);
    }
    case OP_GET_UPVALUE: {
        return get_local_instruction("OP_GET_UPVALUE", chunk, offset);
    }
    case OP_SET_UPVALUE: {
        return constantLongInstruction("OP_SET_UPVALUE", chunk, offset);
    }
    case OP_JUMP_IF_FALSE: {
        return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    }
    case OP_JUMP_IF_TRUE: {
        return jump_instruction("OP_JUMP_IF_TRUE", 1, chunk, offset);
    }
    case OP_JUMP: {
        return jump_instruction("OP_JUMP", 1, chunk, offset);
    }
    case OP_SWITCH_JUMP: {
        printf("OP_SWITCH_JUMP\n");
        return offset + 2;
    }
    case OP_LOOP: {
        return jump_instruction("OP_LOOP", -1, chunk, offset);
    }
    case OP_CALL: {
        printf("OP_CALL\n");
        return offset + 2;
    }

    case OP_INVOKE: {
        ++offset;
        ++offset;

        printf("%-20s %d ", "OP_INVOKE", offset);

        uint32_t operand = READ4BYTE(offset);
        print_value(chunk->constantsLong->values[operand], true, 0);
        printf("\n");

        return offset;
    }
    case OP_CLOSURE: {
        ++offset;
        printf("%-20s %d ", "OP_CLOSURE", offset);
        uint32_t operand = READ4BYTE(offset);

        ObjectFunction *fn = AS_FUNCTION(chunk->constantsLong->values[operand]);
        printf("fn<%s>", fn->name->chars);
        printf("\n");

        for (int i = 0; i < fn->upvalue_count; ++i)
        {
            int is_local = chunk->code[offset++];
            uint32_t index = READ4BYTE(offset);

            printf("%04d    |                      %s %d\n", offset - 2, is_local ? "local" : "upvalue", index);
        }
        return offset;
    }

    case OP_CLASS: {
        ++offset;
        printf("%-20s %d ", "OP_CLASS", offset);
        uint32_t klass_name = READ4BYTE(offset);
        printf("%d \n", klass_name);
        return offset;
    }

    case OP_METHOD:
        return constantLongInstruction("OP_METHOD", chunk, offset);

    case OP_TABLE:
        return simple_instruction("OP_TABLE", offset);

    case OP_TABLE_ITEMS: {
        ++offset;
        printf("%-20s %d ", "OP_TABLE_ITEMS", offset);
        uint32_t operand = READ4BYTE(offset);

        printf("%d \n", operand);
        return offset;
    }
    case OP_ARRAY:
        return simple_instruction("OP_ARRAY", offset);

    case OP_ARRAY_ITEMS: {
        ++offset;
        printf("%-20s %d ", "OP_ARRAY_ITEMS", offset);
        uint32_t operand = READ4BYTE(offset);

        printf("%d \n", operand);
        return offset;
    }

    case OP_ARRAY_PUSH:
        return simple_instruction("OP_ARRAY_PUSH", offset);

    default:
        return offset + 1;
    }
}

void disassemble_chunk(Chunk *chunk, const char *title)
{
    printf("== %s ==\n", title);

    for (size_t i = 0; i < chunk->count;)
    {
        i = disassemble_instruction(chunk, i);
    }
}

uint8_t add_constant(Chunk *chunk, Value constant)
{
    assert(0 && "Small constant is deprecated");
    append_values(chunk->constants, constant);
    return chunk->constants->count - 1;
}

void print_chunk(Chunk *chunk)
{
    printf("[");
    for (size_t i = 0; i < chunk->count; ++i)
    {
        printf("%d,", chunk->code[i]);
    }
    printf("]");
    printf("\n");
}
