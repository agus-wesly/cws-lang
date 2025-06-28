// Copyright 2025 Agustinus Wesly Sitanggang <agustchannel@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "memory.h"
#include "object.h"
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
    }

    if (vm.grey_stack == NULL)
    {
        exit(1);
    }

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
    for (int i = 0; i < count; ++i)
    {
        mark_value(val[i]);
    }
}

static void mark_references()
{
    while (vm.grey_count > 0)
    {
        Obj *obj = vm.grey_stack[--vm.grey_count];

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

        case OBJ_CLASS: {
            ObjectClass *klass = (ObjectClass *)obj;
            mark_obj((Obj *)klass->name);
            mark_table(&klass->methods);
            break;
        }
        case OBJ_INSTANCE: {
            ObjectInstance *inst = (ObjectInstance *)obj;
            mark_obj((Obj *)inst->klass);
            mark_table(&inst->table);
            break;
        }

        case OBJ_METHOD: {
            ObjectMethod *method = (ObjectMethod *)obj;
            mark_obj((Obj *)method->closure);
            mark_value(method->receiver);
            break;
        }

        case OBJ_TABLE: {
            ObjectTable *table = (ObjectTable *)obj;
            mark_table(&table->values);
            break;
        }

        case OBJ_ARRAY: {
            ObjectArray *array = (ObjectArray *)obj;
            for (size_t i = 0; i < array->count; ++i)
            {
                mark_value(array->values[i]);
            }
            mark_table(&array->methods);
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
#ifdef DEBUG_GC
            printf("Removing : %s\n", entry.key->chars);
#endif
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
#ifdef DEBUG_GC
    printf("--gc begin\n");
    size_t before = vm.current_bytes;
#endif

    mark_roots();
    mark_references();
    mark_obj((Obj *)vm.init_string);

    sweep_strings(&vm.strings);
    sweep();

    vm.next_gc = vm.current_bytes * GC_GROW_FACTOR;

#ifdef DEBUG_GC
    printf("--gc end\n");
    printf("Collected : %zu, before : %zu, after : %zu\n", before - vm.current_bytes, before, vm.current_bytes);
#endif
}

void *reallocate(void *array, int oldSize, int newSize)
{
    vm.current_bytes += newSize - oldSize;

    if (newSize == 0)
    {
        free(array);
        return NULL;
    }

#ifdef TEST_STRESS_GC
    collect_garbage();
#else
    if (vm.current_bytes > vm.next_gc)
    {
        collect_garbage();
    }
#endif

    void *result = realloc(array, newSize);
    if (result == NULL)
        exit(69);

    return result;
}
