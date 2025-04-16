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
    Obj *obj = (Obj *)reallocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    return obj;
}

void free_obj(Obj *obj)
{
    switch (obj->type)
    {
    case OBJ_STRING: {
        ObjectString *string = (ObjectString *)(obj);
        FREE_ARRAY(char, string->chars, string->length);
        break;
    }
    default:
        assert(0 && "TODO : implement free for another type");
        break;
    }

    FREE_OBJ(obj);
}
