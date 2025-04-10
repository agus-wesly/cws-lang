#ifndef CWS_LONG_VALUE_H
#define CWS_LONG_VALUE_H

#include "common.h"

typedef double Value;

typedef struct
{
    uint8_t capacity;
    uint32_t count;

    Value *values;
} LongValues;

void InitLongValues(LongValues *values);
void AppendLongValues(LongValues *values, Value newItem);
void FreeLongValues(LongValues *values);
#endif // !CWS_LONG_VALUE_H
