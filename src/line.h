#ifndef CWS_LINE_H
#define CWS_LINE_H

#include "common.h"
#include "std.h"

typedef struct
{
    uint8_t idx;
    uint32_t number;
} Line;

void InitLine(Line *line, uint8_t idx, uint32_t number);

typedef struct
{
    uint8_t capacity;
    uint8_t count;

    Line **lines;
} Lines;

void InitLines(Lines *lines);
void WriteLines(Lines *lines, Line *newItem);
void FreeLines(Lines *lines);

#endif // !CWS_LINE_H
