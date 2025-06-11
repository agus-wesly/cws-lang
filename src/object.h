#ifndef CWS_OBJECT_H
#define CWS_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "hash.h"
#include "hashmap.h"
#include "memory.h"

#define UPVALUE_MAX 2056

typedef struct Object Object;

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_METHOD,
} ObjType;

struct Obj
{
    ObjType type;
    Obj *next;
    bool is_marked;
};

struct ObjectString
{
    Obj object;
    int length;
    uint32_t hash;
    char chars[];
};

typedef struct
{
    bool is_local;
    int index;
} UpValue;

struct ObjectFunction
{
    Obj object;
    int arity;
    ObjectString *name;
    Chunk chunk;

    int upvalue_count;
};

struct ObjectClosure
{
    Obj object;
    ObjectFunction *function;
    int upvalue_count;
    ObjectUpValue **upvalues;
};

struct ObjectUpValue
{
    Obj object;
    Value val;
    Value *p_val;
    int idx;

    ObjectUpValue *next;
};

struct ObjectClass
{
    Obj object;
    ObjectString *name;
    Map methods;
};

struct ObjectInstance
{
    Obj object;
    ObjectClass *klass;
    Map table;
};

struct ObjectMethod
{
    Obj object;
    Value receiver;
    ObjectClosure *closure;
};

typedef bool (*NativeFn)(int args_count, int stack_ptr, Value *returned);
typedef struct
{
    Obj object;
    NativeFn function;

} ObjectNative;

typedef enum
{
    TYPE_SCRIPT,
    TYPE_FUNCTION,
    TYPE_METHOD,
} FunctionType;

struct Obj *allocate_obj(ObjType type, size_t size);

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.decimal)
#define AS_OBJ(value) ((Obj *)(value).as.obj)
#define AS_STRING(value) ((ObjectString *)AS_OBJ(value))
#define AS_C_STRING(value) (((ObjectString *)AS_OBJ(value))->chars)
#define STRINGIFY(value) ((ObjectString *)stringify(AS_OBJ(value)))
#define AS_FUNCTION(value) ((ObjectFunction *)AS_OBJ(value))
#define AS_NATIVE(value) ((ObjectNative *)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjectClosure *)AS_OBJ(value))
#define AS_UPVALUE(value) ((ObjectUpValue *)AS_OBJ(value))
#define AS_CLASS(value) ((ObjectClass *)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjectInstance *)AS_OBJ(value))
#define AS_METHOD(value) ((ObjectMethod *)AS_OBJ(value))

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define ALLOC_OBJ(type, obj_type) ((type *)allocate_obj(obj_type, sizeof(type)))

#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) IsObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) IsObjType(value, OBJ_NATIVE)
#define IS_CLOSURE(value) IsObjType(value, OBJ_CLOSURE)
#define IS_CLASS(value) IsObjType(value, OBJ_CLASS)
#define IS_METHOD(value) IsObjType(value, OBJ_METHOD)

#define FREE_OBJ(ptr) (reallocate(ptr, sizeof(Obj), 0))
#define FREE(type, ptr) (reallocate(ptr, sizeof(type), 0))

static inline int IsObjType(Value value, ObjType type)
{
    return ((IS_OBJ(value)) && OBJ_TYPE(value) == type);
}

ObjectString *allocate_string(const char *chars, int length);
ObjectString *copy_string(const char *start, int length);
ObjectString *take_string(char *chars, int length);
ObjectFunction *new_function();
ObjectNative *new_native(NativeFn function);
ObjectClosure *new_closure(ObjectFunction *function);
ObjectUpValue *new_upvalue();
ObjectClass *new_class(ObjectString *name);
ObjectInstance *new_instance(ObjectClass *klass);
ObjectMethod *new_method(Value receiver, ObjectClosure *closure);

void free_obj(Obj *obj);

#endif // !CWS_OBJECT_H
