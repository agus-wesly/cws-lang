#include "memory.h"
#include "vm.h"

extern VM vm;

void mark_obj(Obj *obj)
{
    if (obj == NULL)
        return;

    if (obj->is_marked)
        return;

#ifdef DEBUG_GC
    printf("%p mark \n", obj);
    print_obj(VALUE_OBJ(obj));
    printf("\n");
#endif

    obj->is_marked = true;

    if (vm.grey_cap < vm.grey_count + 1)
    {
        int old_cap = vm.grey_cap;
        vm.grey_cap = GROW_CAPACITY(old_cap);
        size_t s = vm.grey_cap * sizeof(Obj *);
        vm.grey_stack = (Obj **)(realloc(vm.grey_stack, s));
        printf("Aloccate : %p \n", vm.grey_stack);
    }

    if (vm.grey_stack == NULL)
    {
        exit(1);
    }

    printf("Add idx : %d\n", vm.grey_count);

    vm.grey_stack[vm.grey_count++] = obj;
}

void mark_value(Value val)
{
    if (IS_OBJ(val))
    {
        mark_obj(AS_OBJ(val));
    }
}

static void mark_table(Map *table)
{
    for (size_t i = 0; i < table->capacity; ++i)
    {
        Entry entry = table->entries[i];

        mark_obj((Obj *)(entry.key));
        mark_value(entry.value);
    }
}

static void mark_roots()
{
    for (int i = 0; i < vm.stack_top; ++i)
    {
        mark_value(vm.stack->items[i]);
    }

    mark_table(&vm.globals);

    ObjectUpValue *upvalue = vm.upvalues;
    while (upvalue != NULL)
    {
        mark_obj((Obj *)upvalue);
        upvalue = upvalue->next;
    }

    for (int i = 0; i < vm.frame_count; ++i)
    {
        CallFrame frame = vm.frame[i];
        mark_obj((Obj *)frame.closure);
    }

    mark_compiler();
}

static void mark_array(Value *val, int count)
{
    printf("Count is %d\n", count);
    for (int i = 0; i < count; ++i)
    {
        print_value(val[i]);
        printf(" \n");
        mark_value(val[i]);
    }
}

static void mark_references()
{
    while (vm.grey_count > 0)
    {
        Obj *obj = vm.grey_stack[--vm.grey_count];
        printf("Pop idx : %d\n", vm.grey_count);

#ifdef DEBUG_GC
        printf("%p blacken :  %d ", obj, obj->is_marked);
        print_value(VALUE_OBJ(obj));
        printf("\n");
#endif

        switch (obj->type)
        {
        case OBJ_FUNCTION: {
            ObjectFunction *function = (ObjectFunction *)obj;
            mark_obj((Obj *)function->name);
            mark_array(function->chunk.constantsLong->values, function->chunk.constantsLong->count);
            break;
        }

        case OBJ_STRING: {
            break;
        }

        case OBJ_CLOSURE: {
            ObjectClosure *closure = (ObjectClosure *)obj;
            mark_obj((Obj *)closure->function);
            for (int i = 0; i < closure->upvalue_count; ++i)
            {
                mark_obj((Obj *)closure->upvalues[i]);
            }
            break;
        }

        case OBJ_UPVALUE: {
            mark_value(((ObjectUpValue *)obj)->val);
            break;
        }

        case OBJ_NATIVE: {
            break;
        }
        default: {
            assert(0 && "Unreachable");
            break;
        }
        }
    }
}

void sweep_strings(Map *strings)
{
    for (size_t i = 0; i < strings->capacity; ++i)
    {
        Entry entry = strings->entries[i];
        if (entry.key != NULL && !entry.key->object.is_marked)
        {
            printf("Removing : %s\n", entry.key->chars);
            map_delete(strings, entry.key);
            entry.key->object.is_marked = false;
        }
    }
}

static void sweep()
{
    Obj *prev = NULL;
    Obj *curr = vm.objects;

    while (curr != NULL)
    {
        if (!curr->is_marked)
        {
            Obj *freed = curr;
            curr = curr->next;
            if (prev == NULL)
            {
                vm.objects = curr;
            }
            else
            {
                prev->next = curr;
            }
            free_obj(freed);
        }
        else
        {
            curr->is_marked = false;
            prev = curr;
            curr = curr->next;
        }
    }
}

void collect_garbage()
{
    printf("--gc begin\n");
#ifdef DEBUG_GC
#endif

    mark_roots();
    mark_references();

    sweep_strings(&vm.strings);
    // sweep();

#ifdef DEBUG_GC
#endif
    printf("--gc end\n");
}

void *reallocate(void *array, int oldSize, int newSize)
{
    if (newSize == 0)
    {
        printf("Free-obj : %p\n", array);
        free(array);
        return NULL;
    }

    if (newSize > oldSize)
    {
#ifdef TEST_STRESS_GC
        collect_garbage();
#endif
    }

    void *result = realloc(array, newSize);
    if (result == NULL)
        exit(69);

    return result;
}
