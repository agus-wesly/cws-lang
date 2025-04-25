#include "long_value.h"
#include "memory.h"

void init_long_values(LongValues *values)
{
    values->capacity = 0;
    values->values = NULL;
    values->count = 0;
}

void append_long_values(LongValues *values, Value newItem)
{
    if (values->capacity < values->count + 1)
    {
        uint8_t oldCapacity = values->capacity;
        values->capacity = GROW_CAPACITY(values->capacity);
        values->values = GROW_ARRAY(Value, values->values, oldCapacity, values->capacity);
    }

    values->values[values->count++] = newItem;
}

void free_long_values(LongValues *values)
{
    FREE_ARRAY(Value, values->values, values->capacity);
    init_long_values(values);
}
