#ifndef CWS_VALUE_H
#define CWS_VALUE_H

#include "common.h"

typedef double Value;

typedef struct
{
    uint8_t capacity;
    uint8_t count;

    Value *values;
} Values;

void InitValues(Values *values);
void AppendValues(Values *values, Value newItem);
void FreeValues(Values *values);
void PrintValue(Value *value);
#endif // !CWS_VALUE_H
