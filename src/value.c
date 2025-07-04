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
    values->values[values->count++] = newItem;
}

void print_string(ObjectString *obj)
{
    if (obj->length == 0)
        printf("<empty string>");
    else
        printf("\"%s\"", obj->chars);
}

void print_function(ObjectFunction *obj)
{
    if (obj->name == NULL)
    {
        printf("<script>");
    }
    else
    {
        printf("fungsi<%s>", obj->name->chars);
    }
}

void print_instance(ObjectInstance *instance, bool debug, int level)
{
    if (debug)
    {
        printf("<instansidari ");
        print_string(instance->klass->name);
        printf(">");
        return;
    }
    print_map(&instance->table, level);
}

void print_table(ObjectTable *obj, bool debug, int level)
{
    if (debug)
    {
        printf("<table>");
        return;
    }

    print_map(&obj->values, level);
}

void print_array(ObjectArray *array, bool debug)
{
    if (debug)
    {
        printf("<array>");
        return;
    }
    printf("[");
    for (int i = 0; i < array->count; ++i)
    {
        print_value(array->values[i], debug, 1);
        if (i != array->count - 1)
            printf(",");
    }
    printf("]");
}

void print_obj(Value value, bool debug, int level)
{
#ifdef NAN_BOXING
    if (IS_STRING(value))
    {
        print_string(AS_STRING(value));
        return;
    }

    if (IS_FUNCTION(value))
    {
        print_function(AS_FUNCTION(value));
        return;
    }

    if (IS_NATIVE(value))
    {
        printf("<nativefn>");
        return;
    }

    if (IS_CLOSURE(value))
    {
        print_function(AS_CLOSURE(value)->function);
        return;
    }

    if (IS_UPVALUE(value))
    {
        print_value((*AS_UPVALUE(value)->p_val), debug, level);
        return;
    }

    if (IS_CLASS(value))
    {
        printf("<kelas");
        print_string(AS_CLASS(value)->name);
        printf(">");
        return;
    }

    if (IS_INSTANCE(value))
    {
        print_instance(AS_INSTANCE(value), debug, level);
        return;
    }

    if (IS_METHOD(value))
    {
        print_function(AS_METHOD(value)->closure->function);
        return;
    }

    if (IS_TABLE(value))
    {
        print_table(AS_TABLE(value), debug, level);
        return;
    }

    if (IS_ARRAY(value))
    {
        print_array(AS_ARRAY(value), debug);
        return;
    }

    assert(0 && "Unreachable");

#else
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
        printf("<class");
        print_string(AS_CLASS(value)->name);
        printf(">");
        break;
    }

    case OBJ_INSTANCE: {
        printf("<instanceof ");
        print_string(AS_INSTANCE(value)->klass->name);
        printf(">");
        break;
    }

    case OBJ_METHOD: {
        print_function(AS_METHOD(value)->closure->function);
        break;
    }

    case OBJ_TABLE: {
        print_table(AS_TABLE(value));
        break;
    }

    default:
        assert(0 && "Unreachable");
        return;
    }

#endif
}

void print_value(Value value, bool debug, int level)
{
#ifdef NAN_BOXING
    if (IS_NUMBER(value))
    {
        if (AS_NUMBER(value) == (int)AS_NUMBER(value))
            printf("%i", (int)AS_NUMBER(value));
        else
            printf("%f", AS_NUMBER(value));
        return;
    }
    if (IS_NIL(value))
    {
        printf("nihil");
        return;
    }
    if (IS_BOOLEAN(value))
    {
        if (AS_BOOL(value))
            printf("sah");
        else
            printf("sesat");

        return;
    }
    if (IS_OBJ(value))
    {
        print_obj(value, debug, level);
        return;
    }
    assert(0 && "Unreachable at print value");
#else

    switch (value.type)
    {
    case TYPE_NUMBER:
        if (value.as.decimal == (int)value.as.decimal)
            printf("%i", (int)value.as.decimal);
        else
            printf("%f", value.as.decimal);
        break;

    case TYPE_BOOLEAN:
        if (!!value.as.boolean)
            printf("sah");
        else
            printf("sesat");
        break;

    case TYPE_NIL:
        printf("<nihil>");
        break;

    case TYPE_OBJ:
        print_obj(value, debug, level);
        break;

    default:
        assert(0 && "Unreachable at print value");
        break;
    }

#endif
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
#ifdef NAN_BOXING
    return a == b;
#else

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
#endif
}

bool is_falsy(Value v)
{
#ifdef NAN_BOXING
    return (IS_NIL(v) || (IS_BOOLEAN(v) && IS_FALSE(v)) || (IS_NUMBER(v) && !AS_NUMBER(v)));
#else
    return (v.type == TYPE_NIL || (v.type == TYPE_BOOLEAN && !v.as.boolean) ||
            (v.type == TYPE_NUMBER && !v.as.decimal));
#endif
}

Value value_number(double number)
{
    Value value;
    memcpy(&value, &number, sizeof(number));
    return value;
}
