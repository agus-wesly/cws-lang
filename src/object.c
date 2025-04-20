#include "object.h"
#include "vm.h"

ObjectString *find_string(Map *m, const char *key, int length)
{
    if (m->capacity == 0)
        return NULL;

    uint32_t hash = fnv_32a_str(key, length);
    int idx = hash % m->capacity;

    for (;;)
    {
        Entry *entry = &m->entries[idx];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value))
                return NULL;
        }
        else if (entry->key->length == length && entry->key->hash == hash &&
                 memcmp(entry->key->chars, key, length) == 0)
        {
            return entry->key;
        }

        idx = (idx + 1) % m->capacity;
    }
}

ObjectString *take_string(char *chars, int length)
{
    ObjectString *allocated = find_string(&vm.strings, chars, length);
    if (allocated != NULL)
    {
        FREE_ARRAY(char, chars, length + 1);
        return allocated;
    }

    return allocate_string(chars, length);
}

ObjectString *allocate_string(const char *chars, int length)
{
    ObjectString *string = (ObjectString *)allocate_obj(OBJ_STRING, sizeof(ObjectString) + length + 1);
    string->length = length;
    for (int i = 0; i < length; ++i)
    {
        string->chars[i] = chars[i];
    }
    string->chars[length] = '\0';

    uint32_t hash = fnv_32a_str(string->chars, length);
    string->hash = hash;

    map_set(&vm.strings, string, VALUE_NIL);

    return string;
}

ObjectString *copy_string(const char *chars, int length)
{
    ObjectString *allocated = find_string(&vm.strings, chars, length);
    if (allocated != NULL)
        return allocated;

    return allocate_string(chars, length);
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
