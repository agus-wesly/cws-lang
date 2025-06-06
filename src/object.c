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

    ObjectString *string = allocate_string(chars, length);
    FREE_ARRAY(char, chars, length + 1);
    return string;
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

ObjectFunction *new_function()
{
    ObjectFunction *function = ALLOC_OBJ(ObjectFunction, OBJ_FUNCTION);

    function->name = NULL;
    function->arity = 0;
    init_chunk(&function->chunk);

    return function;
}

ObjectNative *new_native(NativeFn function)
{
    ObjectNative *native = ALLOC_OBJ(ObjectNative, OBJ_NATIVE);
    native->function = function;

    return native;
}

ObjectClosure *new_closure(ObjectFunction *function)
{
    push(VALUE_OBJ(function));
    ObjectUpValue **upvalues = ALLOC(ObjectUpValue *, function->upvalue_count * sizeof(ObjectUpValue *));
    for (int i = 0; i < function->upvalue_count; ++i)
    {
        upvalues[i] = NULL;
    }

    // Why it is not marking <script> in here ?
    ObjectClosure *closure = ALLOC_OBJ(ObjectClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;

    pop();

    return closure;
}

ObjectUpValue *new_upvalue()
{
    ObjectUpValue *upvalue = ALLOC_OBJ(ObjectUpValue, OBJ_UPVALUE);
    upvalue->p_val = NULL;
    upvalue->next = NULL;
    return upvalue;
}

ObjectClass *new_class(ObjectString *name)
{
    ObjectClass *klass = ALLOC_OBJ(ObjectClass, OBJ_CLASS);
    klass->name = name;
    return klass;
}

ObjectInstance *new_instance(ObjectClass *klass)
{
    ObjectInstance *instance = ALLOC_OBJ(ObjectInstance, OBJ_INSTANCE);
    instance->klass = klass;
    init_map(&instance->table);
    return instance;
}

Obj *allocate_obj(ObjType type, size_t size)
{
    Obj *obj = (Obj *)reallocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    obj->is_marked = false;
    vm.objects = obj;

#ifdef DEBUG_GC
    printf("Object %p allocate %zu of type %d\n", obj, size, obj->type);
#endif

    return obj;
}

void free_obj(Obj *obj)
{

#ifdef DEBUG_GC
    printf("%p free type %d\n", obj, obj->type);
    // assert(0);
#endif

    switch (obj->type)
    {
    case OBJ_STRING: {
        FREE(ObjectString, obj);
        break;
    }
    case OBJ_FUNCTION: {
        ObjectFunction *function = (ObjectFunction *)(obj);
        free_chunk(&function->chunk);
        FREE(ObjectFunction, obj);
        break;
    }
    case OBJ_NATIVE: {
        FREE(ObjectNative, obj);
        break;
    }
    case OBJ_CLOSURE: {
        FREE(ObjectClosure, obj);
        break;
    }
    case OBJ_UPVALUE: {
        FREE(ObjectUpValue, obj);
        break;
    }
    default:
        assert(0 && "TODO : implement free for another type");
        break;
    }
}
