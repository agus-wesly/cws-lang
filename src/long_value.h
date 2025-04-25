#ifndef CWS_LONG_VALUE_H
#define CWS_LONG_VALUE_H

#include "common.h"
#include "value.h"

// typedef double Value;

typedef struct
{
    uint8_t capacity;
    uint32_t count;

    Value *values;
} LongValues;

void init_long_values(LongValues *values);
void append_long_values(LongValues *values, Value newItem);
void free_long_values(LongValues *values);
#endif // !CWS_LONG_VALUE_H
