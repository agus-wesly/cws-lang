#include "line.h"
#include "memory.h"

void InitLine(Line *line, uint8_t idx, uint32_t number)
{
    line->idx = idx;
    line->number = number;
}

void InitLines(Lines *lines)
{
    lines->capacity = 0;
    lines->count = 0;
    lines->lines = NULL;
}

void WriteLines(Lines *lines, Line *newItem)
{

    if (lines->capacity < lines->count + 1)
    {
        uint8_t oldCapacity = lines->capacity;
        lines->capacity = GROW_CAPACITY(lines->capacity);
        lines->lines = GROW_ARRAY(Line *, lines->lines, oldCapacity, lines->capacity);
    }
    lines->lines[lines->count] = newItem;
    lines->count++;
}

void FreeLine(Line *line)
{
    free(line);
}

void FreeLines(Lines *lines)
{
    for (size_t i = 0; i < lines->count; ++i)
    {
        FreeLine(lines->lines[i]);
    }

    FREE_ARRAY(Line *, lines->lines, lines->capacity);

    InitLines(lines);
}
