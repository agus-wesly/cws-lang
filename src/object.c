#include "object.h"
#include "vm.h"

ObjectString *find_string(Map *m, const char *key, int length)
{
    if (m->capacity == 0)
        return NULL;

    uint32_t hash = fnv_32a_str(key, length);
    int idx = hash & (m->capacity - 1);

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

        idx = (idx + 1) & (m->capacity - 1);
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
    function->upvalue_count = 0;
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
    init_map(&klass->methods);
    return klass;
}

ObjectInstance *new_instance(ObjectClass *klass)
{
    ObjectInstance *instance = ALLOC_OBJ(ObjectInstance, OBJ_INSTANCE);
    instance->klass = klass;
    init_map(&instance->table);
    return instance;
}

ObjectMethod *new_method(Value receiver, ObjectClosure *closure)
{
    ObjectMethod *method = ALLOC_OBJ(ObjectMethod, OBJ_METHOD);
    method->receiver = receiver;
    method->closure = closure;
    return method;
}

ObjectTable *new_table()
{
    ObjectTable *table = ALLOC_OBJ(ObjectTable, OBJ_TABLE);
    init_map(&table->values);
    return table;
}

/* ===========================================
 * ARRAY METHODS
 * TODO : make this only called once
 * ===========================================
 * */

static ObjectMethod *arr_push_method(ObjectArray *array)
{
    ObjectClosure *closure = new_closure(new_function());

    push(VALUE_OBJ(closure));
    closure->function->name = copy_string("<push>", 6);
    closure->function->arity = 1;

    write_chunk(&closure->function->chunk, OP_ARRAY_PUSH, 0);
    write_chunk(&closure->function->chunk, OP_NIL, 0);
    write_chunk(&closure->function->chunk, OP_RETURN, 0);
    pop();

    ObjectMethod *method = new_method(VALUE_OBJ(array), closure);
    return method;
}

static ObjectMethod *arr_pop_method(ObjectArray *array)
{
    ObjectClosure *closure = new_closure(new_function());
    closure->function->name = copy_string("<pop>", 5);
    closure->function->arity = 0;

    push(VALUE_OBJ(closure));
    write_chunk(&closure->function->chunk, OP_ARRAY_POP, 0);
    write_chunk(&closure->function->chunk, OP_NIL, 0);
    write_chunk(&closure->function->chunk, OP_RETURN, 0);
    pop();

    ObjectMethod *method = new_method(VALUE_OBJ(array), closure);
    return method;
}

/* ===========================================
 */

ObjectArray *new_array()
{
    ObjectArray *array = ALLOC_OBJ(ObjectArray, OBJ_ARRAY);
    array->values = NULL;
    array->cap = 0;
    array->count = 0;

    push(VALUE_OBJ(array));

    init_map(&array->methods);

    {
        // PUSH(x)
        ObjectMethod *method = arr_push_method(array);
        push(VALUE_OBJ(method));
        map_set(&array->methods, copy_string("push", 4), VALUE_OBJ(method));
        pop();
    }

    {
        // POP()
        ObjectMethod *method = arr_pop_method(array);
        push(VALUE_OBJ(method));
        map_set(&array->methods, copy_string("pop", 3), VALUE_OBJ(method));
        pop();
    }

    pop();
    return array;
}

void append_array(ObjectArray *array, Value newItem)
{
    if (array->cap < array->count + 1)
    {
        uint16_t oldCapacity = array->cap;
        array->cap = GROW_CAPACITY(array->cap);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->cap);
    }
    array->values[array->count++] = newItem;
}

void pop_array(ObjectArray *array)
{
    assert(array->count > 0 && "Cannot pop empty array");
    array->count--;
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

double number_value(Value value)
{
    double number;
    memcpy(&number, &value, sizeof(Value));
    return number;
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

    case OBJ_CLASS: {
        ObjectClass *klass = (ObjectClass *)obj;
        free_map(&klass->methods);

        FREE(ObjectClass, obj);
        break;
    }

    case OBJ_INSTANCE: {
        ObjectInstance *instance = (ObjectInstance *)obj;
        FREE(ObjectInstance, obj);
        free_map(&instance->table);
        break;
    }

    case OBJ_METHOD: {
        FREE(ObjectMethod, obj);
        break;
    }

    case OBJ_TABLE: {
        ObjectTable *table = (ObjectTable *)obj;
        free_map(&table->values);
        FREE(ObjectTable, obj);
        break;
    }

    case OBJ_ARRAY: {
        ObjectArray *array = (ObjectArray *)obj;
        free(array->values);
        FREE(ObjectArray, obj);
        break;
    }

    default:
        assert(0 && "TODO : implement free for another type");
        break;
    }
}
