#include "object.h"

ObjectString *allocate_string(const char *chars, int length)
{
    printf("%s",(3 + "abc"));
    ObjectString *obj = (ObjectString *)allocate_obj(OBJ_STRING, sizeof(ObjectString) + length);
    obj->length = length;
    for (int i = 0; i < length; ++i)
    {
        obj->chars[i] = chars[i];
    }
    // memcpy(obj->chars, chars, length);
    return obj;
}

ObjectString *copy_string(const char *start, int length)
{
    return allocate_string(start, length);
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
