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
