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

void PRINT_OBJ(Obj *obj)
{
    switch (OBJ_TYPE(obj))
    {
    case OBJ_STRING: {
        printf("%s", AS_STRING(obj)->string);
    }
    default:
        return;
    }
}

void PrintValue(Value *value)
{
    if (value->type == TYPE_NUMBER)
        printf("'%f'", value->as.decimal);
    else if (value->type == TYPE_BOOLEAN)
        printf("'%d'", value->as.boolean);
    else if (value->type == TYPE_OBJ)
    {
        PRINT_OBJ(AS_OBJ(*value));
    }
}

void FreeValues(Values *values)
{
    FREE_ARRAY(Value, values->values, values->capacity);
    InitValues(values);
}

int compare_string(Obj *a, Obj *b)
{
    ObjectString *string_a = AS_STRING(a);
    ObjectString *string_b = AS_STRING(b);
    return (string_a->length == string_b->length) &&
           (memcmp(string_a->string, string_b->string, string_a->length) == 0);
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
        Obj *obj_a = AS_OBJ(a);
        Obj *obj_b = AS_OBJ(b);

        if (OBJ_TYPE(obj_a) != OBJ_TYPE(obj_b))
            return 0;

        switch (OBJ_TYPE(obj_a))
        {
        case OBJ_STRING: {
            return compare_string(obj_a, obj_b);
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
