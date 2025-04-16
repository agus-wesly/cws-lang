#include "object.h"

ObjectString *allocate_string(char *chars, int length)
{
    ObjectString *obj = ALLOC_OBJ(ObjectString, OBJ_STRING);
    obj->chars = chars;
    obj->length = length;
    return obj;
}

ObjectString *copy_string(const char *start, int length)
{
    char *copy = ALLOC(char, length + 1);
    memcpy(copy, start, length);
    copy[length] = '\0';

    return allocate_string(copy, length);
}

Obj *allocate_obj(ObjType type, size_t size)
{
    Obj *ptr = (Obj *)reallocate(NULL, 0, size);
    ptr->object->type = type;
    return ptr;
}
