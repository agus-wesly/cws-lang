#include "chunk.h"
#include "memory.h"

void InitChunk(Chunk *chunk)
{
    chunk->capacity = 0;
    chunk->count = 0;

    chunk->code = NULL;

    // TODO
    Values *values = malloc(sizeof(Values));
    InitValues(values);
    chunk->constants = values;

    LongValues *longValues = malloc(sizeof(LongValues));
    InitLongValues(longValues);
    chunk->constantsLong = longValues;

    Lines *lines = malloc(sizeof(Lines));
    InitLines(lines);
    chunk->lines = lines;
}

void FreeChunk(Chunk *chunk)
{
    FreeValues(chunk->constants);
    FreeLongValues(chunk->constantsLong);

    FreeLines(chunk->lines);

    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    InitChunk(chunk);
}

void WriteConstant(Chunk *chunk, Value value, uint32_t lineNumber)
{
    AppendLongValues(chunk->constantsLong, value);

    WriteChunk(chunk, OP_CONSTANT_LONG, lineNumber);

    uint32_t constantIndex = chunk->constantsLong->count - 1;
    for (size_t i = 0; i < 4; ++i)
    {
        uint8_t chunkIdx = (constantIndex >> (8 * (3 - i)));
        WriteChunk(chunk, chunkIdx, lineNumber);
    }
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

void WriteChunk(Chunk *chunk, uint8_t newItem, uint32_t lineNumber)
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

int simpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int constantInstruction(const char *name, Chunk *chunk, int offset)
{
    printf("%-20s %d ", name, offset);

    uint8_t constant = chunk->code[offset + 1];
    PrintValue(&chunk->constants->values[constant]);
    printf("\n");

    return offset + 2;
}

int constantLongInstruction(const char *name, Chunk *chunk, int offset)
{

    printf("%-20s %d ", name, offset);
    uint32_t operand = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        uint32_t byt = chunk->code[offset + 1 + i];
        operand = operand | (byt << (8 * (3 - i)));
    }
    PrintValue(&chunk->constantsLong->values[operand]);
    printf("\n");

    return offset + 1 + 4;
}

uint32_t getLine(Chunk *chunk, uint8_t idx)
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

int DisassembleInstruction(Chunk *chunk, int offset)
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
        return simpleInstruction("OP_RETURN", offset);
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
        return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);

    case OP_ADD: {
        return simpleInstruction("OP_ADD", offset);
    }
    case OP_SUBTRACT: {
        return simpleInstruction("OP_SUBTRACT", offset);
    }
    case OP_MULTIPLY: {
        return simpleInstruction("OP_MULTIPLY", offset);
    }
    case OP_DIVIDE: {
        return simpleInstruction("OP_DIVIDE", offset);
    }
    case OP_NEGATE: {
        return simpleInstruction("OP_NEGATE", offset);
    }
    default:
        // printf("Unknown instruction\n");
        return offset + 1;
    }
}

void DisassembleChunk(Chunk *chunk, const char *title)
{
    printf("== %s ==\n", title);

    for (size_t i = 0; i < chunk->count;)
    {
        i = DisassembleInstruction(chunk, i);
    }
}

uint8_t AddConstant(Chunk *chunk, Value constant)
{
    AppendValues(chunk->constants, constant);
    return chunk->constants->count - 1;
}

void PrintChunk(Chunk *chunk)
{
    printf("[");
    for (size_t i = 0; i < chunk->count; ++i)
    {
        printf("%d,", chunk->code[i]);
    }
    printf("]");
    printf("\n");
}
