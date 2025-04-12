#ifndef CWS_VALUE_H
#define CWS_VALUE_H

#include "common.h"

typedef enum
{
    TYPE_BOOLEAN,
    TYPE_NUMBER,
    TYPE_NIL
} ValueType;

typedef struct
{
    ValueType type;
    union {
        float decimal;
        uint8_t boolean;
    } as;
} Value;

typedef struct
{
    uint8_t capacity;
    uint8_t count;

    Value *values;
} Values;

#define VALUE_NUMBER(value) \
    (Value){.type = TYPE_NUMBER, .as = {.decimal = value}}
    
#define VALUE_BOOL(value) \
    (Value){.type = TYPE_BOOLEAN, .as = {.boolean = value}}

#define VALUE_NIL() \
    (Value){.type = TYPE_NIL, .as = {.boolean = 0}}

#define AS_BOOL(value) ((Value)value).as.boolean)
#define AS_NUMBER(value) (value.as.decimal)

void InitValues(Values *values);
void AppendValues(Values *values, Value newItem);
void FreeValues(Values *values);
void PrintValue(Value *value);
#endif // !CWS_VALUE_H
