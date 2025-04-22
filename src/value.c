#include "value.h"
#include "object.h"

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

    // 514
    // 664

    values->values[values->count++] = newItem;
}

void PRINT_OBJ(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING: {
        if (AS_STRING(value)->length == 0)
            printf("<empty string>");
        else
            printf("%s", AS_STRING(value)->chars);
    }

    default:
        return;
    }
}

void PrintValue(Value value)
{
    if (value.type == TYPE_NUMBER)
        printf("%f", value.as.decimal);
    else if (value.type == TYPE_BOOLEAN)
        if (!!value.as.boolean)
            printf("true");
        else
            printf("false");
    else if (value.type == TYPE_OBJ)
    {
        PRINT_OBJ(value);
    }
}

void FreeValues(Values *values)
{
    FREE_ARRAY(Value, values->values, values->capacity);
    InitValues(values);
}

int compare_string(Value a, Value b)
{
    ObjectString *string_a = AS_STRING(a);
    ObjectString *string_b = AS_STRING(b);
    return (string_a->length == string_b->length) && (memcmp(string_a->chars, string_b->chars, string_a->length) == 0);
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

    case TYPE_OBJ: {

        if (OBJ_TYPE(a) != OBJ_TYPE(b))
            return 0;

        switch (OBJ_TYPE(a))
        {
        case OBJ_STRING: {
            // return compare_string(a, b);
            return AS_STRING(a) == AS_STRING(b);
        }
        default:
            return 0;
        }
    }

    default:
        assert(0 && "Unreachable at compare");
    }
}

int IsFalsy(Value v)
{
    return (v.type == TYPE_NIL || (v.type == TYPE_BOOLEAN && !v.as.boolean) ||
            (v.type == TYPE_NUMBER && !v.as.decimal));
}
