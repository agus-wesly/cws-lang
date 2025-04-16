#ifndef CWS_OBJECT_H
#define CWS_OBJECT_H

#include "common.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

typedef struct Object Object;

struct Obj
{
    Object *object;
};

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjType;

struct Object
{
    ObjType type;
    Obj *next;
};

struct ObjectString
{
    Object *object;
    char *chars;
    int length;
};

struct Obj *allocate_obj(ObjType type, size_t size);

#define OBJ_TYPE(value) (AS_OBJ(value)->object->type)
#define ALLOC_OBJ(type, obj_type) ((type *)allocate_obj(obj_type, sizeof(type)))

#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define FREE_OBJECT(ptr) (reallocate(ptr, sizeof(Object), 0))
#define FREE_OBJ(ptr) (reallocate(ptr, sizeof(Obj), 0))

static inline int IsObjType(Value value, ObjType type)
{
    return ((IS_OBJ(value)) && OBJ_TYPE(value) == type);
}

ObjectString *allocate_string(char *chars, int length);
ObjectString *copy_string(const char *start, int length);
void free_obj(Obj *obj);

#endif // !CWS_OBJECT_H
