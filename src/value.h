#ifndef CWS_VALUE_H
#define CWS_VALUE_H

#include "memory.h"
#include "string.h"

#ifdef NAN_BOXING
#define TYPE_NIL 1
#define TYPE_FALSE 2
#define TYPE_TRUE 3
#else
typedef enum
{
    TYPE_BOOLEAN,
    TYPE_NUMBER,
    TYPE_NIL,
    TYPE_OBJ,
} ValueType;
#endif

typedef struct Obj Obj;

typedef struct ObjectString ObjectString;
typedef struct ObjectFunction ObjectFunction;
typedef struct ObjectClosure ObjectClosure;
typedef struct ObjectUpValue ObjectUpValue;
typedef struct ObjectClass ObjectClass;
typedef struct ObjectInstance ObjectInstance;
typedef struct ObjectMethod ObjectMethod;
typedef struct ObjectTable ObjectTable;
typedef struct ObjectArray ObjectArray;

#ifdef NAN_BOXING
typedef uint64_t Value;
#else
typedef struct
{
    ValueType type;
    union {
        float decimal;
        uint8_t boolean;
        Obj *obj;
    } as;
} Value;
#endif

typedef struct
{
    uint8_t capacity;
    uint8_t count;

    Value *values;
} Values;

#ifdef NAN_BOXING
#define QNAN 0x7ffc000000000000
#define SIGNED_BIT 0x8000000000000000

#define VALUE_NIL (QNAN | TYPE_NIL)
#define VALUE_TRUE (QNAN | TYPE_TRUE)
#define VALUE_FALSE (QNAN | TYPE_FALSE)

#define VALUE_NUMBER(number) (value_number((number)))
#define VALUE_BOOL(boolean) (boolean ? VALUE_TRUE : VALUE_FALSE)
#define VALUE_OBJ(object) ((Value)(SIGNED_BIT | QNAN | (Value)(uintptr_t)object))

#define IS_NUMBER(value) (((Value)(value) & QNAN) != QNAN)
#define IS_TRUE(value) ((Value)(value) == VALUE_TRUE)
#define IS_FALSE(value) ((Value)(value) == VALUE_FALSE)
#define IS_BOOLEAN(value) (((Value)(value) | 1) == VALUE_TRUE)
#define IS_NIL(value) ((Value)(value) == VALUE_NIL)
#define IS_OBJ(value) (((Value)value & (SIGNED_BIT | QNAN)) == (SIGNED_BIT | QNAN))

#else
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
        .type = TYPE_OBJ, .as = {.obj = (Obj *)(object) }                                                              \
    }

#define IS_NUMBER(value) (value.type == TYPE_NUMBER)
#define IS_BOOLEAN(value) (value.type == TYPE_BOOLEAN)
#define IS_NIL(value) (value.type == TYPE_NIL)
#define IS_OBJ(value) (value.type == TYPE_OBJ)
#endif

#define IS_OBJ_TYPE(obj, obj_type) (obj->type == obj_type)

void init_values(Values *values);
void append_values(Values *values, Value newItem);
void free_values(Values *values);
bool compare(Value value1, Value value2);
bool is_falsy(Value v);

void mark_obj(Obj *obj);
void mark_value(Value val);
void print_value(Value value, bool debug, int level);
void print_obj(Value value, bool debug, int level);

Value value_number(double number);

#endif // !CWS_VALUE_H
