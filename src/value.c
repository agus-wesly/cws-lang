#include "value.h"
#include "object.h"
#include "vm.h"

void init_values(Values *values)
{
    values->capacity = 0;
    values->values = NULL;
    values->count = 0;
}

void append_values(Values *values, Value newItem)
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

void print_string(ObjectString *obj)
{
    if (obj->length == 0)
        printf("<empty string>");
    else
        printf("%s", obj->chars);
}

void print_function(ObjectFunction *obj)
{
    if (obj->name == NULL)
    {
        printf("<script>");
    }
    else
    {
        printf("fn<%s>", obj->name->chars);
    }
}

void print_obj(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING: {
        print_string(AS_STRING(value));
        break;
    }
    case OBJ_FUNCTION: {
        print_function(AS_FUNCTION(value));
        break;
    }

    case OBJ_NATIVE: {
        printf("<nativefn>");
        break;
    }

    case OBJ_CLOSURE: {
        print_function(AS_CLOSURE(value)->function);
        break;
    }

    case OBJ_UPVALUE: {
        print_value((*AS_UPVALUE(value)->p_val));
        break;
    }

    case OBJ_CLASS: {
        printf("<class> ");
        print_string(AS_CLASS(value)->name);
        break;
    }

    case OBJ_INSTANCE: {
        printf("<instanceof> ");
        print_string(AS_INSTANCE(value)->klass->name);
        break;
    }

    case OBJ_METHOD: {
        print_function(AS_METHOD(value)->closure->function);
        break;
    }

    default:
        assert(0 && "Unreachable");
        return;
    }
}

void print_value(Value value)
{
    switch (value.type)
    {
    case TYPE_NUMBER:
        printf("%f", value.as.decimal);
        break;

    case TYPE_BOOLEAN:
        if (!!value.as.boolean)
            printf("true");
        else
            printf("false");
        break;

    case TYPE_NIL:

        printf("<nil>");
        break;

    case TYPE_OBJ:
        print_obj(value);
        break;

    default:
        assert(0 && "Unreachable at print value");
        break;
    }
}

void free_values(Values *values)
{
    FREE_ARRAY(Value, values->values, values->capacity);
    init_values(values);
}

int compare_string(Value a, Value b)
{
    ObjectString *string_a = AS_STRING(a);
    ObjectString *string_b = AS_STRING(b);
    return (string_a->length == string_b->length) && (memcmp(string_a->chars, string_b->chars, string_a->length) == 0);
}

bool compare(Value a, Value b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
    case TYPE_NIL:
        return true;

    case TYPE_BOOLEAN:
        return AS_BOOL(a) == AS_BOOL(b);

    case TYPE_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);

    case TYPE_OBJ: {

        if (OBJ_TYPE(a) != OBJ_TYPE(b))
            return false;

        switch (OBJ_TYPE(a))
        {
        case OBJ_STRING: {
            // return compare_string(a, b);
            return AS_STRING(a) == AS_STRING(b);
        }
        default:
            return false;
        }
    }

    default:
        assert(0 && "Unreachable at compare");
    }
}

bool is_falsy(Value v)
{
    return (v.type == TYPE_NIL || (v.type == TYPE_BOOLEAN && !v.as.boolean) ||
            (v.type == TYPE_NUMBER && !v.as.decimal));
}
