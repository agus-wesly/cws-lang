#include "value.h"
#include "memory.h"

void InitValues(Values *values)
{
    values->capacity = 0;
    values->values = NULL;
    values->count = 0;
}

void AppendValues(Values *values, Value newItem)
{
    if (values->capacity < values->count + 1)
    {
        uint8_t oldCapacity = values->capacity;
        values->capacity = GROW_CAPACITY(values->capacity);
        values->values = GROW_ARRAY(Value, values->values, oldCapacity, values->capacity);
    }

    values->values[values->count++] = newItem;
}

void PrintValue(Value *value)
{
    printf("'%g'", *value);
}

void FreeValues(Values *values)
{
    FREE_ARRAY(Value, values->values, values->capacity);
    InitValues(values);
}
