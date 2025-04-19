#include "object.h"
#include "vm.h"

ObjectString *find_string(Map *m, const char *key, int length, int capacity)
{
    if (capacity == 0)
        return NULL;

    uint32_t hash = fnv_32a_str(key);
    int idx = hash % capacity;

    for (;;)
    {
        Entry *entry = &m->entries[idx];
        if (entry->key == NULL)
        {
            return NULL;
        }

        if (entry->key->length == length && entry->key->hash == hash && strcmp(entry->key->chars, key) == 0)
        {
            return entry->key;
        }

        idx = (idx + 1) % capacity;
    }
}

ObjectString *allocate_string(const char *chars, int length)
{
    ObjectString *allocated = find_string(vm.strings, chars, length, vm.strings->capacity);
    if (allocated != NULL)
        return allocated;

    ObjectString *obj = (ObjectString *)allocate_obj(OBJ_STRING, sizeof(ObjectString) + length + 1);
    obj->length = length;
    for (int i = 0; i < length; ++i)
    {
        obj->chars[i] = chars[i];
    }
    obj->chars[length] = '\0';

    uint32_t hash = fnv_32a_str(obj->chars);
    obj->hash = hash;

    map_set(vm.strings, obj, VALUE_NIL);

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
