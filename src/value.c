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
    if (value->type == TYPE_NUMBER)
        printf("'%f'", value->as.decimal);
    else if (value->type == TYPE_BOOLEAN)
        printf("'%d'", value->as.boolean);
}

void FreeValues(Values *values)
{
    FREE_ARRAY(Value, values->values, values->capacity);
    InitValues(values);
}

int Compare(Value a, Value b)
{
    if (a.type != b.type)
        return 0;

    switch (a.type)
    {
    case TYPE_NIL:
        return 1;

    case TYPE_BOOLEAN:
        return AS_BOOL(a) == AS_BOOL(b);

    case TYPE_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);

    default:
        assert(0 && "Unreachable at compare");
    }
}

int IsFalsy(Value v)
{
    return (v.type == TYPE_NIL || (v.type == TYPE_BOOLEAN && !v.as.boolean) ||
            (v.type == TYPE_NUMBER && !v.as.decimal));
}
