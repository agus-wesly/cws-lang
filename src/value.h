#ifndef CWS_VALUE_H
#define CWS_VALUE_H

#include "memory.h"
#include "string.h"

typedef enum
{
    TYPE_BOOLEAN,
    TYPE_NUMBER,
    TYPE_NIL,
    TYPE_OBJ,
} ValueType;

typedef struct Obj Obj;

typedef struct ObjectString ObjectString;
typedef struct ObjectFunction ObjectFunction;

typedef struct
{
    ValueType type;
    union {
        float decimal;
        uint8_t boolean;
        Obj *obj;
    } as;
} Value;

typedef struct
{
    uint8_t capacity;
    uint8_t count;

    Value *values;
} Values;

#define VALUE_NUMBER(value)                                                                                            \
    (Value)                                                                                                            \
    {                                                                                                                  \
        .type = TYPE_NUMBER, .as = {.decimal = value }                                                                 \
    }

#define VALUE_BOOL(value)                                                                                              \
    (Value)                                                                                                            \
    {                                                                                                                  \
        .type = TYPE_BOOLEAN, .as = {.boolean = value }                                                                \
    }

#define VALUE_NIL                                                                                                      \
    (Value)                                                                                                            \
    {                                                                                                                  \
        .type = TYPE_NIL, .as = {.boolean = 0 }                                                                        \
    }

#define VALUE_OBJ(object)                                                                                              \
    (Value)                                                                                                            \
    {                                                                                                                  \
        .type = TYPE_OBJ, .as = {.obj = (Obj *)object }                                                                \
    }


#define IS_NUMBER(value) (value.type == TYPE_NUMBER)
#define IS_NIL(value) (value.type == TYPE_NIL)
#define IS_OBJ(value) (value.type == TYPE_OBJ)
#define IS_OBJ_TYPE(obj, obj_type) (obj->type == obj_type)

void init_values(Values *values);
void append_values(Values *values, Value newItem);
void free_values(Values *values);
void print_value(Value value);
int compare(Value value1, Value value2);
int is_falsy(Value v);

#endif // !CWS_VALUE_H
