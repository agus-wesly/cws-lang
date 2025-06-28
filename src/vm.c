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

#include "vm.h"
#include "chunk.h"
#include "hashmap.h"
#include "native.h"
#include "object.h"
#include "value.h"

VM vm;

static void define_native(const char *name, NativeFn function);

void resetStack()
{
    vm.stack_top = 0;
    vm.frame_count = 0;
}

void update_stack_ptr()
{
    vm.stack_top = 0;
}

void init_vm()
{
    // vm.chunk = NULL;
    // vm.ip = NULL;
    vm.objects = NULL;
    vm.frame_count = 0;
    vm.upvalues = NULL;
    vm.stack_top = 0;
    vm.current_bytes = 0;
    vm.next_gc = 10;

    Stack *stack_ptr = malloc(sizeof(Stack));
    vm.stack = stack_ptr;
    init_stack(vm.stack);

    init_map(&vm.strings);
    init_map(&vm.globals);

    vm.grey_count = 0;
    vm.grey_cap = 0;
    vm.grey_stack = NULL;

    update_stack_ptr();

    vm.init_string = NULL;
    vm.init_string = copy_string("init", 4);

    define_native("time", time_native);
}

void freeObjects()
{
    Obj *object = vm.objects;
    while (object != NULL)
    {
        Obj *next = object->next;
        free_obj(object);
        object = next;
    }
}

void free_vm()
{
    freeObjects();
    free(vm.stack);
    free_map(&vm.strings);
    free_map(&vm.globals);

    free(vm.strings.entries);
    vm.init_string = NULL;
}

void runtime_error(char *format, ...)
{
    va_list args;
    va_start(args, format);
    fputs("Kesalahan Runtime : ", stderr);
    vfprintf(stderr, format, args);
    va_end(args);

    fputs("\n", stderr);
}

void push(Value value)
{
    if (vm.stack->size >= vm.stack->capacity)
    {
        int old_capacity = vm.stack->capacity;
        int new_capacity = GROW_CAPACITY(old_capacity);
        vm.stack->capacity = new_capacity;
        vm.stack->items = (Value *)realloc(vm.stack->items, new_capacity * sizeof(Value));
    }

    vm.stack->items[vm.stack_top++] = value;
    if (vm.stack_top > vm.stack->size)
        vm.stack->size++;
}

Value pop()
{
    if (vm.stack->size == 0)
    {
        assert(0 && "Cannot Pop if stack is empty");
    }

    vm.stack_top--;
    return vm.stack->items[vm.stack_top];
}

static void define_native(const char *name, NativeFn function)
{
    ObjectString *s = copy_string(name, strlen(name));
    Value str = VALUE_OBJ(s);
    push(str);

    ObjectNative *f = new_native(function);
    Value fn = VALUE_OBJ(f);
    push(fn);

    map_set(&vm.globals, AS_STRING(vm.stack->items[0]), vm.stack->items[1]);

    pop();
    pop();
}

void print_error_line(uint8_t *ip)
{
    for (int i = vm.frame_count - 1; i >= 0; --i)
    {
        CallFrame *frame = &vm.frame[i];

        if (i != vm.frame_count - 1)
            ip = frame->ip;

        ObjectFunction *function = frame->closure->function;
        uint8_t idx = ip - function->chunk.code;
        uint32_t line_number = get_line(&frame->closure->function->chunk, idx);
        fprintf(stderr, "[Baris %d] di ", line_number);
        if (function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s\n", function->name->chars);
        }
    }
}

#define HANDLE_CONCAT()                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!IS_STRING(PEEK(0)) || !IS_STRING(PEEK(1)))                                                                \
        {                                                                                                              \
            runtime_error("Operand harus bertipe data string");                                                        \
            resetStack() return INTERPRET_RUNTIME_ERROR;                                                               \
        }                                                                                                              \
        ObjectString *b = AS_STRING(pop());                                                                            \
        ObjectString *a = AS_STRING(pop());                                                                            \
        push(VALUE_OBJ(concat(a, b)));                                                                                 \
    } while (0);

#define PEEK(index) (vm.stack->items[vm.stack_top - 1 - (index)])

ObjectString *stringify(Value value)
{
#ifdef NAN_BOXING

    if (IS_NUMBER(value))
    {
        int len = 0;
        char *start = {0};
        if (AS_NUMBER(value) == (int)AS_NUMBER(value))
        {
            int int_num = (int)AS_NUMBER(value);
            len = snprintf(NULL, 0, "%d", int_num);
            start = malloc(len + 1);
            snprintf(start, len + 1, "%d", int_num);
        }
        else
        {
            double double_num = (double)AS_NUMBER(value);
            len = snprintf(NULL, 0, "%f", double_num);
            start = malloc(len + 1);
            snprintf(start, len + 1, "%f", double_num);
        }

        ObjectString *result = take_string(start, len);
        return result;
    }
    if (IS_NIL(value))
    {
        return copy_string("VALUE_NIL", 3);
    }
    if (IS_BOOLEAN(value))
    {
        if (AS_BOOL(value))
            return copy_string("VALUE_TRUE", 4);

        return copy_string("VALUE_FALSE", 5);
    }
    if (IS_OBJ(value))
    {
        if (IS_STRING(value))
            return AS_STRING(value);
    }
    assert(0 && "Unreachable at stringify");
#else
    switch (value.type)
    {
    case TYPE_NUMBER: {
        int len = snprintf(NULL, 0, "%f", value.as.decimal);

        char *start = malloc(len + 1);
        snprintf(start, len + 1, "%f", value.as.decimal);

        ObjectString *result = take_string(start, len);
        return result;
    }
    case TYPE_NIL: {
        return copy_string("VALUE_NIL", 3);
    }
    case TYPE_BOOLEAN: {
        if (AS_BOOL(value))
            return copy_string("VALUE_TRUE", 4);

        return copy_string("VALUE_FALSE", 5);
    }
    case TYPE_OBJ: {
        if (IS_STRING(value))
            return AS_STRING(value);

        assert(0 && "Unreachable at stringify");
    }
    default:
        assert(0 && "Unreachable at stringify");
    }
#endif
}

ObjectString *concatenate()
{
    ObjectString *b = stringify(PEEK(0));
    ObjectString *a = stringify(PEEK(1));

    int length = a->length + b->length;
    char *result = ALLOC(char, length + 1);

    memcpy(result, a->chars, a->length);
    memcpy(result + a->length, b->chars, b->length);
    result[length] = '\0';

    pop();
    pop();

    return take_string(result, length);
}

static bool call(ObjectClosure *callee, int args_count, uint8_t *ip)
{
    if (callee->function->arity != args_count)
    {
        runtime_error("Diharapkan %d arguments namun mendapat %d", callee->function->arity, args_count);
        print_error_line(ip);

        return false;
    }

    if (vm.frame_count >= FRAME_MAX)
    {
        runtime_error("Jumlah melewati jumlah call frame maksimum");
        print_error_line(ip);
        return false;
    }

    CallFrame *current = &vm.frame[vm.frame_count++];
    current->slots = (vm.stack_top - args_count - 1);
    current->ip = callee->function->chunk.code;
    current->closure = callee;

    return true;
}

static bool call_value(Value callee, int args_count, uint8_t *ip)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
        case OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), args_count, ip);

        case OBJ_NATIVE: {
            ObjectNative *native = AS_NATIVE(callee);
            NativeFn function = native->function;
            Value returned;
            if (!function(args_count, vm.stack_top - args_count, &returned))
            {
                return false;
            }

            vm.stack_top = vm.stack_top - args_count - 1;
            push(returned);
            return true;
        }

        case OBJ_CLASS: {
            ObjectClass *klass = AS_CLASS(callee);
            ObjectInstance *inst = new_instance(klass);

            vm.stack->items[vm.stack_top - args_count - 1] = VALUE_OBJ(inst);

            Value init_val;
            if (map_get(&klass->methods, vm.init_string, &init_val))
            {
                assert(IS_CLOSURE(init_val));
                return call(AS_CLOSURE(init_val), args_count, ip);
            }
            else if (args_count != 0)
            {
                runtime_error("Diharapkan 0 parameter namun mendapat %d", args_count);
                print_error_line(ip);
                resetStack();
                return false;
            }

            return true;
        }

        case OBJ_METHOD: {
            ObjectMethod *method = AS_METHOD(callee);
            vm.stack->items[vm.stack_top - args_count - 1] = method->receiver;
            return call(method->closure, args_count, ip);
        }

        default:
            break;
        }
    }

    runtime_error("Mencoba untuk mengeksekusi value non-fungsi");
    return false;
}

static ObjectUpValue *get_from_uplist(int idx)
{
    ObjectUpValue *prev_upvalue = NULL;
    ObjectUpValue *curr_upvalue = vm.upvalues;
    while (curr_upvalue != NULL && curr_upvalue->idx > idx)
    {
        prev_upvalue = curr_upvalue;
        curr_upvalue = curr_upvalue->next;
    }

    if (curr_upvalue != NULL && curr_upvalue->idx == idx)
    {
        return curr_upvalue;
    }

    ObjectUpValue *created_upvalue = new_upvalue();
    created_upvalue->idx = idx;
    created_upvalue->next = curr_upvalue;

    if (prev_upvalue == NULL)
    {
        vm.upvalues = created_upvalue;
        created_upvalue->next = curr_upvalue;
    }
    else
    {
        prev_upvalue->next = created_upvalue;
    }
    return created_upvalue;
}

static void close_up_values(int last)
{
    while (vm.upvalues != NULL && vm.upvalues->idx >= last)
    {
        ObjectUpValue *upvalue = vm.upvalues;
        upvalue->val = vm.stack->items[upvalue->idx];
        upvalue->p_val = &upvalue->val;
        vm.upvalues = vm.upvalues->next;
    }
}

static bool validate_array_key(ObjectArray *array, int *key_ptr)
{
    int key_int = *key_ptr;

    if (key_int > UINT16_MAX || key_int >= array->count)
    {
        runtime_error("Indeks %d diluar jangkauan", key_int);
        return false;
    }

    if (key_int < 0)
    {
        key_int = key_int + array->count;
        if (key_int < 0)
        {
            runtime_error("Indeks %d diluar jangkauan", *key_ptr);
            return false;
        }
        *key_ptr = key_int;
    }
    return true;
}

static bool len_expression(Value expr, Value *result)
{
    if (!IS_OBJ(expr))
    {
        runtime_error("Expression tidak valid");
        return false;
    }

    switch (OBJ_TYPE(expr))
    {
    case OBJ_STRING: {
        ObjectString *string = AS_STRING(expr);
        *result = VALUE_NUMBER(string->length);
        return true;
    }
    case OBJ_TABLE: {
        ObjectTable *table = AS_TABLE(expr);
        *result = VALUE_NUMBER(table->values.size);
        return true;
    }
    case OBJ_ARRAY: {
        ObjectArray *array = AS_ARRAY(expr);
        *result = VALUE_NUMBER(array->count);
        return true;
    }
    default: {
        runtime_error("Expression tidak valid");
        return false;
    }
    }
}

static bool get_field(Value container_val, Value key_value, Value *value)
{
    if (!IS_OBJ(container_val))
    {
        runtime_error("Hanya instance atau table yang memiliki fields");
        return false;
    }

    switch (OBJ_TYPE(container_val))
    {
    case OBJ_INSTANCE: {
        ObjectInstance *inst = AS_INSTANCE(container_val);
        if (!IS_STRING(key_value))
        {
            runtime_error("Key harus bertipe string");
            return false;
        }

        ObjectString *key = AS_STRING(key_value);
        if (map_get(&inst->table, key, value))
        {
            return true;
        }

        // find in the method
        if (!map_get(&inst->klass->methods, key, value))
        {
            runtime_error("objek tidak memiliki attribute '%s'", key->chars);
            return false;
        };

        assert(IS_CLOSURE(*value));
        ObjectMethod *method = new_method(container_val, AS_CLOSURE(*value));
        *value = (VALUE_OBJ(method));
        return true;
    }
    case OBJ_TABLE: {
        ObjectTable *table = AS_TABLE(container_val);
        if (!IS_STRING(key_value))
        {
            runtime_error("Key harus bertipe string");
            return false;
        }

        ObjectString *key = AS_STRING(key_value);
        if (map_get(&table->values, key, value))
        {
            return true;
        }
        runtime_error("Objek 'table' tidak memiliki attribute '%s'", key->chars);
        return false;
    }
    case OBJ_ARRAY: {
        ObjectArray *array = AS_ARRAY(container_val);
        if (IS_STRING(key_value))
        {
            ObjectString *key = AS_STRING(key_value);
            if (memcmp(key->chars, "push", key->length) == 0)
            {
                assert(map_get(&array->methods, key, value));
                return true;
            }
            if (memcmp(key->chars, "pop", key->length) == 0)
            {
                assert(map_get(&array->methods, key, value));
                return true;
            }
        }

        if (!IS_NUMBER(key_value))
        {
            if (IS_STRING(key_value))
                runtime_error("Objek 'List' tidak memiliki attribute: %s", AS_C_STRING(key_value));
            else
                runtime_error("Objek 'List' tidak memiliki attribute yang sesuai");
            return false;
        }

        int key_int = AS_NUMBER(key_value);

        if (!validate_array_key(array, &key_int))
            return false;

        *value = array->values[key_int];
        return true;
    }
    default:
        return false;
    }
}

static bool set_field(Value container_val, Value key_value, Value new_val)
{
    if (!IS_OBJ(container_val))
        return false;

    switch (OBJ_TYPE(container_val))
    {
    case OBJ_INSTANCE: {
        ObjectInstance *inst = AS_INSTANCE(container_val);
        if (!IS_STRING(key_value))
        {
            runtime_error("Key harus bertipe string");
            return false;
        }

        ObjectString *key_string = AS_STRING(key_value);
        map_set(&inst->table, key_string, new_val);
        break;
    }
    case OBJ_TABLE: {
        ObjectTable *table = AS_TABLE(container_val);

        if (!IS_STRING(key_value))
        {
            runtime_error("Key harus bertipe string");
            return false;
        }

        ObjectString *key_string = AS_STRING(key_value);
        map_set(&table->values, key_string, new_val);
        break;
    }
    case OBJ_ARRAY: {
        ObjectArray *array = AS_ARRAY(container_val);
        int key_int = AS_NUMBER(key_value);

        if (!validate_array_key(array, &key_int))
            return false;

        array->values[key_int] = new_val;
        break;
    }
    default:
        return false;
    }

    return true;
}

static bool del_field(Value container_val, ObjectString *key)
{
    assert(IS_OBJ(container_val));

    switch (OBJ_TYPE(container_val))
    {
    case OBJ_INSTANCE: {
        ObjectInstance *inst = AS_INSTANCE(container_val);
        return map_delete(&inst->table, key);
    }
    case OBJ_TABLE: {
        ObjectTable *table = AS_TABLE(container_val);
        return map_delete(&table->values, key);
    }
    default:
        return false;
    }
}

static InterpretResult run()
{
    CallFrame *frame = &vm.frame[vm.frame_count - 1];
    uint8_t *ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() ((ip += 2), ((uint16_t)(ip[-2] | ip[-1])))
#define READ_CONSTANT() (frame->closure->function->chunk.constants->values[READ_BYTE()])
#define STRING() (AS_STRING(READ_CONSTANT()))
#define READ_LONG_BYTE()                                                                                               \
    ({                                                                                                                 \
        uint32_t res = 0;                                                                                              \
        int i = 0;                                                                                                     \
        do                                                                                                             \
        {                                                                                                              \
            res |= ((uint32_t)READ_BYTE() << (8 * (3 - i)));                                                           \
            ++i;                                                                                                       \
        } while (i < 4);                                                                                               \
        res;                                                                                                           \
    })

#define READ_LONG_CONSTANT()                                                                                           \
    ({                                                                                                                 \
        uint32_t res = 0;                                                                                              \
        int i = 0;                                                                                                     \
        do                                                                                                             \
        {                                                                                                              \
            res |= ((uint32_t)READ_BYTE() << (8 * (3 - i)));                                                           \
            ++i;                                                                                                       \
        } while (i < 4);                                                                                               \
        frame->closure->function->chunk.constantsLong->values[res];                                                    \
    })

#define READ_STRING() AS_STRING(READ_LONG_CONSTANT())

#define HANDLE_BINARY(value, op)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!IS_NUMBER(PEEK(0)) || !IS_NUMBER(PEEK(1)))                                                                \
                                                                                                                       \
        {                                                                                                              \
            RUNTIME_ERROR(ip - 1, "Operand harus bertipe number");                                                     \
            return INTERPRET_RUNTIME_ERROR;                                                                            \
        }                                                                                                              \
        double b = AS_NUMBER(pop());                                                                                   \
        double a = AS_NUMBER(pop());                                                                                   \
        push(value(a op b));                                                                                           \
    } while (0);

#define HANDLE_EQUAL()                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        Value b = pop();                                                                                               \
        Value a = pop();                                                                                               \
        push(VALUE_BOOL(compare(a, b)));                                                                               \
    } while (0);

#define HANDLE_TERNARY()                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        /* TODO : this only supports number, change to support other */                                                \
        double false_expr = AS_NUMBER(pop());                                                                          \
        double true_expr = AS_NUMBER(pop());                                                                           \
        double condition = AS_NUMBER(pop());                                                                           \
        if (!!condition)                                                                                               \
            push(VALUE_NUMBER(true_expr));                                                                             \
        else                                                                                                           \
            push(VALUE_NUMBER(false_expr));                                                                            \
    } while (0);

    for (;;)
    {

#ifdef DEBUG_TRACE_EXECUTION

        printf("[");
        for (int i = 0; i < vm.stack_top; ++i)
        {
            Value cur = vm.stack->items[i];
            print_value(cur, true, 0);
            printf(",");
        }
        printf("]");
        printf("\n");
        disassemble_instruction(&frame->closure->function->chunk, (int)(ip - frame->closure->function->chunk.code));
        printf("\n");
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_RETURN: {
            Value return_value = pop();

            close_up_values(frame->slots);

            vm.frame_count--;
            if (vm.frame_count == 0)
            {
                pop();
                return INTERPRET_OK;
            }

            int dist = (int)(vm.stack_top - frame->slots);

            vm.stack_top = frame->slots;
            vm.stack->size -= dist;

            push(return_value);

            frame = &vm.frame[vm.frame_count - 1];
            ip = frame->ip;

            break;
        }

        case OP_CONSTANT: {
            Value constant_value = READ_CONSTANT();
            push(constant_value);
            break;
        }

        case OP_CONSTANT_LONG: {
            Value constant_value = READ_LONG_CONSTANT();
            push(constant_value);
            break;
        }

        case OP_TRUE: {
            push(VALUE_BOOL(true));
            break;
        }
        case OP_FALSE: {
            push(VALUE_BOOL(false));
            break;
        }

        case OP_NIL: {
            push(VALUE_NIL);
            break;
        }

        case OP_LEN: {
            Value val = pop();
            Value result = {0};
            if (!len_expression(val, &result))
            {
                print_error_line(ip);
                resetStack();
                return INTERPRET_RUNTIME_ERROR;
            };
            push(result);
            break;
        }

        case OP_NEGATE: {
            uint8_t *prev_ip = ip - 1;
            if (!IS_NUMBER(PEEK(0)))
            {
                RUNTIME_ERROR(prev_ip, "Diharapkan number");
                return INTERPRET_RUNTIME_ERROR;
            }
            double num = AS_NUMBER(PEEK(0)) * -1;
            pop();
            push(VALUE_NUMBER(num));
            break;
        }
        case OP_BANG: {
            Value a = pop();
            push(VALUE_BOOL(is_falsy(a)));
            break;
        }

        case OP_ADD: {
            uint8_t *prev_ip = ip - 1;
            if (IS_NUMBER(PEEK(0)) && IS_NUMBER(PEEK(1)))
            {
                HANDLE_BINARY(VALUE_NUMBER, +);
                break;
            }
            if ((IS_STRING(PEEK(0)) || (IS_NUMBER(PEEK(0)))) && ((IS_STRING(PEEK(1))) || IS_NUMBER(PEEK(1))))
            {
                push(VALUE_OBJ(concatenate()));
                break;
            }
            else
            {
                RUNTIME_ERROR(prev_ip, "Operands harus bertipe number atau string");
                return INTERPRET_RUNTIME_ERROR;
            }
        }
        case OP_SUBTRACT:
            HANDLE_BINARY(VALUE_NUMBER, -);
            break;
        case OP_MULTIPLY:
            HANDLE_BINARY(VALUE_NUMBER, *);
            break;
        case OP_DIVIDE:
            HANDLE_BINARY(VALUE_NUMBER, /);
            break;
        case OP_GREATER:
            HANDLE_BINARY(VALUE_BOOL, >);
            break;
        case OP_LESS:
            HANDLE_BINARY(VALUE_BOOL, <);
            break;
        case OP_EQUAL_EQUAL:
            HANDLE_EQUAL();
            break;
        case OP_DOT_GET: {
            Value container_val = pop();

            Value value;
            if (!get_field(container_val, READ_LONG_CONSTANT(), &value))
            {
                print_error_line(ip);
                resetStack();
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_DOT_SET: {
            Value key = READ_LONG_CONSTANT();
            Value new_val = PEEK(0);
            Value container_val = PEEK(1);

            if (!set_field(container_val, key, new_val))
            {
                print_error_line(ip);
                resetStack();
                return INTERPRET_RUNTIME_ERROR;
            }
            pop();
            pop();
            push(new_val);

            break;
        }
        case OP_SQR_BRACKET_GET: {
            Value key_val = pop();
            Value container_val = pop();

            Value value;
            if (!get_field(container_val, key_val, &value))
            {
                print_error_line(ip);
                resetStack();
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_SQR_BRACKET_SET: {
            Value new_val = PEEK(0);
            Value key_val = PEEK(1);
            Value container_val = PEEK(2);

            if (!set_field(container_val, key_val, new_val))
            {
                print_error_line(ip);
                resetStack();
                return INTERPRET_RUNTIME_ERROR;
            }

            pop();
            pop();
            pop();
            push(new_val);

            break;
        }

        case OP_TERNARY:
            HANDLE_TERNARY();
            break;

        case OP_PRINT: {
            Value value = pop();
            print_value(value, false, 1);
            printf("\n");
            break;
        }
        case OP_POP:
            pop();
            break;

        case OP_CLOSE_UPVALUE: {
            close_up_values(vm.stack_top - 1);
            pop();
            break;
        }

        case OP_COMPARE: {
            Value b = PEEK(0);
            Value a = PEEK(1);

            push(VALUE_BOOL(compare(a, b)));
            break;
        }

        case OP_GLOBAL_VAR: {
            ObjectString *name = READ_STRING();
            map_set(&vm.globals, name, pop());

            break;
        }

        case OP_GET_GLOBAL: {
            uint8_t *prev_ip = ip - 1;
            ObjectString *name = READ_STRING();
            Value val;
            if (!map_get(&vm.globals, name, &val))
            {
                RUNTIME_ERROR(prev_ip, "Tidak dapat mengakses variabel yang tidak terdeklarasi: %s", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(val);
            break;
        }

        case OP_SET_GLOBAL: {
            uint8_t *prev_ip = ip - 1;
            ObjectString *name = READ_STRING();
            Value val = PEEK(0);

            if (map_set(&vm.globals, name, val))
            {
                map_delete(&vm.globals, name);
                RUNTIME_ERROR(prev_ip, "Tidak dapat menetapkan nilai ke variabel yang tidak terdeklarasi : '%s'",
                              name->chars);
                return INTERPRET_RUNTIME_ERROR;
            };
            break;
        }

        case OP_GET_LOCAL: {
            uint32_t idx = READ_LONG_BYTE();
            push(vm.stack->items[frame->slots + idx]);
            break;
        }

        case OP_SET_LOCAL: {
            uint32_t idx = READ_LONG_BYTE();
            vm.stack->items[frame->slots + idx] = PEEK(0);
            break;
        }

        case OP_GET_UPVALUE: {
            uint32_t idx = READ_LONG_BYTE();
            ObjectUpValue *upvalue = frame->closure->upvalues[idx];
            if (upvalue->p_val == NULL)
            {
                push(vm.stack->items[upvalue->idx]);
            }
            else
            {
                push(*upvalue->p_val);
            }
            break;
        }

        case OP_SET_UPVALUE: {
            uint32_t idx = READ_LONG_BYTE();
            ObjectUpValue *upvalue = frame->closure->upvalues[idx];
            if (upvalue->p_val == NULL)
            {
                vm.stack->items[upvalue->idx] = PEEK(0);
            }
            else
            {
                *upvalue->p_val = PEEK(0);
            }
            break;
        }

        case OP_JUMP_IF_FALSE: {
            uint16_t jump = READ_SHORT();
            if (is_falsy(PEEK(0)))
            {
                ip += jump;
            }
            break;
        }

        case OP_JUMP_IF_TRUE: {
            uint16_t jump = READ_SHORT();
            if (!is_falsy(PEEK(0)))
            {
                ip += jump;
            }
            break;
        }

        case OP_JUMP: {
            uint16_t jump = READ_SHORT();
            ip += jump;
            break;
        }

        case OP_LOOP: {
            uint16_t jump = READ_SHORT();
            ip -= jump;
            break;
        }

        case OP_MARK_JUMP: {
            ip += 2;
            break;
        }

        case OP_SWITCH: {
            push(VALUE_BOOL(0));
            break;
        }

        case OP_CASE_COMPARE: {
            Value b = pop();
            Value a = PEEK(1);
            vm.stack->items[vm.stack_top - 1] = VALUE_BOOL(compare(a, b));
            break;
        }

        case OP_SWITCH_JUMP: {
            ip += 2;
            uint8_t idx = *(ip - 2);
            uint16_t jump =
                ((uint16_t)(frame->closure->function->chunk.code[idx] | frame->closure->function->chunk.code[idx + 1]));
            uint8_t dist = *(ip - 1);

            ip += (jump - dist);

            break;
        }

        case OP_CALL: {
            uint8_t args_count = READ_BYTE();
            Value callee = PEEK(args_count);

            frame->ip = ip;

            if (!call_value(callee, args_count, ip))
            {
                return INTERPRET_RUNTIME_ERROR;
            }

            frame = &vm.frame[vm.frame_count - 1];
            ip = frame->ip;

            break;
        }

        case OP_CLOSURE: {
            ObjectFunction *function = AS_FUNCTION(READ_LONG_CONSTANT());
            ObjectClosure *closure = new_closure(function);

            for (int i = 0; i < function->upvalue_count; ++i)
            {
                bool is_local = READ_BYTE();
                int index = READ_LONG_BYTE();
                if (is_local)
                {
                    closure->upvalues[i] = get_from_uplist(frame->slots + index);
                }
                else
                {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }

            push(VALUE_OBJ(closure));
            break;
        }

        case OP_CLASS: {
            push(VALUE_OBJ(new_class(READ_STRING())));
            break;
        }

        case OP_METHOD: {
            Value val_name = READ_LONG_CONSTANT();

            assert(IS_CLOSURE(PEEK(0)));
            assert(IS_CLASS(PEEK(1)));
            assert(IS_STRING(val_name));

            ObjectClosure *method = (AS_CLOSURE(PEEK(0)));
            ObjectClass *klass = AS_CLASS(PEEK(1));
            ObjectString *name = AS_STRING(val_name);

            // TODO : if this is init, then find another way
            // to store it
            map_set(&klass->methods, name, VALUE_OBJ(method));

            pop();

            break;
        }

        case OP_DEL: {
            uint8_t *prev_ip = ip - 1;
            Value key_val = pop();
            Value container_val = pop();

            if (!IsObjType(key_val, OBJ_STRING))
            {
                RUNTIME_ERROR(prev_ip, "Expression harus bertipe string");
                return INTERPRET_RUNTIME_ERROR;
            }

            if (!IS_OBJ(container_val))
            {
                RUNTIME_ERROR(prev_ip, "Hanya instances yang memiliki fields");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjectString *key = AS_STRING(key_val);
            if (!del_field(container_val, key))
            {
                RUNTIME_ERROR(prev_ip, "Kesalahan field : '%s'", key->chars);
                return INTERPRET_RUNTIME_ERROR;
            };

            break;
        }

        case OP_INVOKE: {
            uint8_t args_count = READ_BYTE();
            Value key = READ_LONG_CONSTANT();
            Value inst_val = PEEK(args_count);

            Value val;
            if (!get_field(inst_val, key, &val))
            {
                print_error_line(ip);
                resetStack();
                return INTERPRET_RUNTIME_ERROR;
            };

            frame->ip = ip;

            if (!call_value(val, args_count, ip))
            {
                return INTERPRET_RUNTIME_ERROR;
            }

            frame = &vm.frame[vm.frame_count - 1];
            ip = frame->ip;

            break;
        }

        case OP_TABLE: {
            push(VALUE_OBJ(new_table()));
            break;
        }

        case OP_TABLE_ITEMS: {
            uint32_t table_count = READ_LONG_BYTE();

            for (size_t i = 0; i < table_count; ++i)
            {
                Value key_val = PEEK(1);
                Value value_val = PEEK(0);
                Value inst = PEEK(table_count * 2 - (i * 2));

                assert(IS_TABLE(inst));

                ObjectTable *table = AS_TABLE(inst);
                map_set(&table->values, AS_STRING(key_val), value_val);

                pop();
                pop();
            }

            break;
        }

        case OP_ARRAY: {
            push(VALUE_OBJ(new_array()));
            break;
        }

        case OP_ARRAY_ITEMS: {
            int array_count = READ_LONG_BYTE();

            for (int i = 0; i < array_count; ++i)
            {
                Value inst = PEEK(array_count);

                assert(IS_ARRAY(inst));

                ObjectArray *array = AS_ARRAY(inst);
                Value val = PEEK(array_count - 1 - i);
                append_array(array, val);
            }
            vm.stack_top -= array_count;
            break;
        }

        case OP_ARRAY_PUSH: {
            Value val = PEEK(0);
            Value container_val = PEEK(1);
            assert(IS_ARRAY(container_val));
            ObjectArray *array = AS_ARRAY(container_val);
            append_array(array, val);

            break;
        }

        case OP_ARRAY_POP: {
            uint8_t *prev_ip = ip - 1;
            Value container_val = PEEK(0);
            assert(IS_ARRAY(container_val));

            ObjectArray *array = AS_ARRAY(container_val);
            if (array->count <= 0)
            {
                RUNTIME_ERROR(prev_ip, "Tidak dapat melakukan pop pada array yang kosong");
                return INTERPRET_RUNTIME_ERROR;
            }
            pop_array(array);
            break;
        }

        default:
            return INTERPRET_OK;
        }
    }

#undef READ_SHORT
#undef READ_BYTE
#undef STRING
#undef READ_CONSTANT
#undef READ_LONG_CONSTANT
#undef READ_STRING
#undef HANDLE_BINARY
#undef HANDLE_EQUAL
#undef HANDLE_TERNARY
#undef RUNTIME_ERROR
}

void init_call_frame(CallFrame *call_frame)
{
    call_frame->ip = call_frame->closure->function->chunk.code;
    call_frame->slots = 0;
}

InterpretResult interpret(const char *source)
{
    ObjectFunction *base_function = compile(source);
    if (base_function == NULL)
        return INTERPRET_COMPILE_ERROR;

    ObjectClosure *closure = new_closure(base_function);
    push(VALUE_OBJ(closure));

    CallFrame *current = &vm.frame[vm.frame_count++];
    current->slots = 0;
    current->ip = base_function->chunk.code;
    current->closure = closure;

    return run();
}

void init_stack(Stack *stack)
{
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}
