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
    printf("Free : %p\n", vm.grey_stack);
    // free(NULL);
}

void free_vm()
{
    // freeObjects();
    // free(vm.stack);
    // free_map(&vm.strings);
    // free_map(&vm.globals);

    free(vm.strings.entries);
}

void runtime_error(char *format, ...)
{
    va_list args;
    va_start(args, format);
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
        // vm.stack->items = GROW_ARRAY(Value, vm.stack->items, old_capacity, new_capacity);
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

#define HANDLE_CONCAT()                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!IS_STRING(PEEK(0)) || !IS_STRING(PEEK(1)))                                                                \
        {                                                                                                              \
            runtimeError("Operand must be of type string");                                                            \
            resetStack() return INTERPRET_RUNTIME_ERROR;                                                               \
        }                                                                                                              \
        ObjectString *b = AS_STRING(pop());                                                                            \
        ObjectString *a = AS_STRING(pop());                                                                            \
        push(VALUE_OBJ(concat(a, b)));                                                                                 \
    } while (0);

#define PEEK(index) (vm.stack->items[vm.stack_top - 1 - index])

ObjectString *stringify(Value value)
{
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

static bool call(ObjectClosure *callee, int args_count)
{
    if (callee->function->arity != args_count)
    {
        runtime_error("Expected %d arguments but got %d", callee->function->arity, args_count);
        return false;
    }

    if (vm.frame_count >= FRAME_MAX)
    {
        runtime_error("Maximum call frame has been exceeded");
        return false;
    }

    CallFrame *current = &vm.frame[vm.frame_count++];
    current->slots = (vm.stack_top - args_count - 1);
    current->ip = callee->function->chunk.code;
    current->closure = callee;

    return true;
}

static bool call_value(Value callee, int args_count)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
        case OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), args_count);

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

            vm.stack_top = vm.stack_top - args_count - 1;
            push(VALUE_OBJ(inst));
            return true;
        }

        case OBJ_METHOD: {
            ObjectMethod *method = AS_METHOD(callee);
            return call(method->closure, args_count);
        }

        default:
            break;
        }
    }

    runtime_error("Attempted to call to non-function value");
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

void print_error_line(uint8_t *ip)
{
    for (int i = vm.frame_count - 1; i >= 0; --i)
    {
        CallFrame *frame = &vm.frame[i];
        ObjectFunction *function = frame->closure->function;
        uint8_t idx = ip - function->chunk.code;
        uint32_t line_number = get_line(&frame->closure->function->chunk, idx);
        fprintf(stderr, "[Line %d] in ", line_number);
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

#define HANDLE_BINARY(value, op)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!IS_NUMBER(PEEK(0)) || !IS_NUMBER(PEEK(1)))                                                                \
                                                                                                                       \
        {                                                                                                              \
            RUNTIME_ERROR("Operand must be of type number");                                                           \
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
        double false_expr = AS_NUMBER(pop());                                                                          \
        double true_expr = AS_NUMBER(pop());                                                                           \
        double condition = AS_NUMBER(pop());                                                                           \
        if (!!condition)                                                                                               \
            push(VALUE_NUMBER(true_expr));                                                                             \
        else                                                                                                           \
            push(VALUE_NUMBER(false_expr));                                                                            \
    } while (0);

#define RUNTIME_ERROR(...)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        runtime_error(__VA_ARGS__);                                                                                    \
        print_error_line(ip);                                                                                          \
        resetStack();                                                                                                  \
    } while (0);

    for (;;)
    {

#ifdef DEBUG_TRACE_EXECUTION

        printf("[");
        for (int i = 0; i < vm.stack_top; ++i)
        {
            Value cur = vm.stack->items[i];
            print_value(cur);
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
            push(VALUE_BOOL(1));
            break;
        }
        case OP_FALSE: {
            push(VALUE_BOOL(0));
            break;
        }

        case OP_NIL: {
            push(VALUE_NIL);
            break;
        }

        case OP_NEGATE: {
            if (!IS_NUMBER(PEEK(0)))
            {
                RUNTIME_ERROR("Expected number");
                return INTERPRET_RUNTIME_ERROR;
            }
            vm.stack->items[vm.stack_top - 1].as.decimal *= -1;
            break;
        }
        case OP_BANG: {
            Value a = pop();
            push(VALUE_BOOL(is_falsy(a)));
            break;
        }

        case OP_ADD: {
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
                RUNTIME_ERROR("Operands must be number or string");
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
        case OP_GET_FIELD: {
            Value val = pop();
            if (!IsObjType(val, OBJ_INSTANCE))
            {
                RUNTIME_ERROR("Only instances have fields");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjectInstance *inst = AS_INSTANCE(val);
            ObjectString *key = AS_STRING(READ_LONG_CONSTANT());

            Value get_val;
            if (map_get(&inst->table, key, &get_val))
            {
                push(get_val);
                break;
            }

            // find in the method
            if (map_get(&inst->klass->methods, key, &get_val))
            {
                assert(IS_METHOD(get_val));
                push(get_val);
                break;
            };

            RUNTIME_ERROR("Field error : '%s'", key->chars);
            return INTERPRET_RUNTIME_ERROR;
        }
        case OP_SET_FIELD: {
            Value new_val = PEEK(0);
            Value inst_val = PEEK(1);

            if (!IsObjType(inst_val, OBJ_INSTANCE))
            {
                RUNTIME_ERROR("Only instances have fields");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjectInstance *inst = AS_INSTANCE(inst_val);
            ObjectString *key = AS_STRING(READ_LONG_CONSTANT());

            map_set(&inst->table, key, new_val);
            pop();
            pop();
            push(new_val);

            break;
        }
        case OP_GET_FIELD_SQR_BRACKET: {
            Value key_val = pop();
            Value val = pop();

            if (!IsObjType(key_val, OBJ_STRING))
            {
                RUNTIME_ERROR("Expression inside key must be type of string");
                return INTERPRET_RUNTIME_ERROR;
            }

            if (!IsObjType(val, OBJ_INSTANCE))
            {
                RUNTIME_ERROR("Only instances have fields");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjectInstance *inst = AS_INSTANCE(val);
            ObjectString *key = AS_STRING(key_val);

            Value get_val;
            if (map_get(&inst->table, key, &get_val))
            {
                push(get_val);
                break;
            }

            RUNTIME_ERROR("Field error : '%s'", key->chars);
            return INTERPRET_RUNTIME_ERROR;
        }
        case OP_SET_FIELD_SQR_BRACKET: {
            Value new_val = PEEK(0);
            Value key_val = PEEK(1);
            Value inst_val = PEEK(2);

            if (!IsObjType(key_val, OBJ_STRING))
            {
                RUNTIME_ERROR("Expression inside bracket must be type of string");
                return INTERPRET_RUNTIME_ERROR;
            }

            if (!IsObjType(inst_val, OBJ_INSTANCE))
            {
                RUNTIME_ERROR("Only instances have fields");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjectInstance *inst = AS_INSTANCE(inst_val);
            ObjectString *key = AS_STRING(key_val);

            map_set(&inst->table, key, new_val);
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
            print_value(value);
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
            ObjectString *name = AS_STRING(READ_LONG_CONSTANT());
            map_set(&vm.globals, name, pop());

            break;
        }

        case OP_GET_GLOBAL: {
            ObjectString *name = AS_STRING(READ_LONG_CONSTANT());
            Value val;
            if (!map_get(&vm.globals, name, &val))
            {
                RUNTIME_ERROR("Cannot access undeclared variable : %s", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(val);
            break;
        }

        case OP_SET_GLOBAL: {
            ObjectString *name = AS_STRING(READ_LONG_CONSTANT());
            Value val = PEEK(0);

            if (map_set(&vm.globals, name, val))
            {
                map_delete(&vm.globals, name);
                RUNTIME_ERROR("Cannot set to undefined variable : '%s'", name->chars);
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

            if (!call_value(callee, args_count))
                return INTERPRET_RUNTIME_ERROR;

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
            push(VALUE_OBJ(new_class(AS_STRING(READ_LONG_CONSTANT()))));
            break;
        }

        case OP_METHOD: {
            Value val_name = READ_LONG_CONSTANT();
            assert(IS_CLOSURE(PEEK(0)));
            assert(IS_CLASS(PEEK(1)));
            assert(IS_STRING(val_name));

            ObjectMethod *method = new_method(AS_CLOSURE(PEEK(0)));
            ObjectClass *klass = AS_CLASS(PEEK(1));
            ObjectString *name = AS_STRING(val_name);

            map_set(&klass->methods, name, VALUE_OBJ(method));

            pop();

            break;
        }

        case OP_DEL: {
            Value key_val = pop();
            Value inst_val = pop();

            if (!IsObjType(key_val, OBJ_STRING))
            {
                RUNTIME_ERROR("Expression must be type of string");
                return INTERPRET_RUNTIME_ERROR;
            }

            if (!IsObjType(inst_val, OBJ_INSTANCE))
            {
                RUNTIME_ERROR("Only instances have fields");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjectInstance *inst = AS_INSTANCE(inst_val);
            ObjectString *key = AS_STRING(key_val);
            if (!map_delete(&inst->table, key))
            {
                RUNTIME_ERROR("Key Error : '%s'", key->chars);
                return INTERPRET_RUNTIME_ERROR;
            };

            break;
        }

        default:
            return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_LONG_CONSTANT
#undef HANDLE_BINARY
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

// STACK

void init_stack(Stack *stack)
{
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}
