#include "vm.h"
#include "chunk.h"
#include "hashmap.h"
#include "object.h"
#include "value.h"

VM vm;

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

    Stack *stack_ptr = malloc(sizeof(Stack));
    vm.stack = stack_ptr;
    init_stack(vm.stack);

    init_map(&vm.strings);
    init_map(&vm.globals);

    update_stack_ptr();
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
}

void runtime_error(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; --i)
    {
        CallFrame *frame = &vm.frame[i];
        ObjectFunction *function = frame->function;
        uint8_t idx = frame->ip - function->chunk.code;

        uint32_t line_number = get_line(&frame->function->chunk, idx);
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

    resetStack();
}

void push(Value value)
{
    if (vm.stack->size >= vm.stack->capacity)
    {
        uint8_t old_capacity = vm.stack->capacity;
        uint8_t new_capacity = GROW_CAPACITY(old_capacity);
        vm.stack->capacity = new_capacity;
        vm.stack->items = GROW_ARRAY(Value, vm.stack->items, old_capacity, new_capacity);
    }

    vm.stack->items[vm.stack_top++] = value;
    vm.stack->size++;
}

Value pop()
{
    if (vm.stack->size == 0)
    {
        assert(0 && "Cannot Pop if stack is empty");
    }

    vm.stack_top--;
    vm.stack->size--;
    return vm.stack->items[vm.stack_top];
}

#define HANDLE_CONCAT()                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!IS_STRING(PEEK(0)) || !IS_STRING(PEEK(1)))                                                                \
        {                                                                                                              \
            runtimeError("Operand must be of type string");                                                            \
            return INTERPRET_RUNTIME_ERROR;                                                                            \
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
    ObjectString *b = stringify(pop());
    ObjectString *a = stringify(pop());

    int length = a->length + b->length;
    char *result = ALLOC(char, length + 1);

    memcpy(result, a->chars, a->length);
    memcpy(result + a->length, b->chars, b->length);
    result[length] = '\0';

    return take_string(result, length);
}

static bool call(ObjectFunction *callee, int args_count)
{
    if (callee->arity != args_count)
    {
        runtime_error("Expected %d arguments but got %d", callee->arity, args_count);
        return false;
    }

    CallFrame *current = &vm.frame[vm.frame_count++];
    current->slots = (vm.stack_top - args_count - 1);
    current->ip = callee->chunk.code;
    current->function = callee;

    return true;
}

static bool call_value(Value callee, int args_count)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
        case OBJ_FUNCTION:
            return call(AS_FUNCTION(callee), args_count);

        default:
            break;
        }
    }

    runtime_error("Attempted to call to non-function value");
    return false;
}

static InterpretResult run()
{
    CallFrame *frame = &vm.frame[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() ((frame->ip += 2), ((uint16_t)(frame->ip[-2] | frame->ip[-1])))
#define READ_CONSTANT() (frame->function->chunk.constants->values[READ_BYTE()])
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
        frame->function->chunk.constantsLong->values[res];                                                             \
    })

#define HANDLE_BINARY(value, op)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!IS_NUMBER(PEEK(0)) || !IS_NUMBER(PEEK(1)))                                                                \
                                                                                                                       \
        {                                                                                                              \
            runtime_error("Operand must be of type number");                                                           \
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

    for (;;)
    {

#ifdef DEBUG_TRACE_EXECUTION

        printf("[");
        for (Value *cur = vm.stack->items; cur < vm.stack_top; ++cur)
        {
            print_value(*cur);
            printf(",");
        }
        printf("]");
        printf("\n");
        disassemble_instruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
        printf("\n");
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_RETURN: {
            Value return_value = pop();

            vm.frame_count--;
            if (vm.frame_count == 0)
            {
                pop();
                return INTERPRET_OK;
            }

            int dist = (int)(vm.stack_top - frame->slots);
            for (int i = 0; i < dist; ++i)
            {
                pop();
            }

            push(return_value);

            frame = &vm.frame[vm.frame_count - 1];

            break;
        }
            // pop();

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
            if (IS_NUMBER(PEEK(0)))
            {
                runtime_error("Expected number");
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
                runtime_error("Operands must be number or string");
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
                runtime_error("Cannot access undeclared variable : %s", name->chars);
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
                runtime_error("Cannot set to undefined variable : '%s'", name->chars);
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

        case OP_JUMP_IF_FALSE: {
            uint16_t jump = READ_SHORT();
            if (is_falsy(PEEK(0)))
            {
                frame->ip += jump;
            }
            break;
        }

        case OP_JUMP_IF_TRUE: {
            uint16_t jump = READ_SHORT();
            if (!is_falsy(PEEK(0)))
            {
                frame->ip += jump;
            }
            break;
        }

        case OP_JUMP: {
            uint16_t jump = READ_SHORT();
            frame->ip += jump;
            break;
        }

        case OP_LOOP: {
            uint16_t jump = READ_SHORT();
            frame->ip -= jump;
            break;
        }

        case OP_MARK_JUMP: {
            frame->ip += 2;
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
            frame->ip += 2;
            uint8_t idx = *(frame->ip - 2);
            uint16_t jump = ((uint16_t)(frame->function->chunk.code[idx] | frame->function->chunk.code[idx + 1]));
            uint8_t dist = *(frame->ip - 1);

            frame->ip += (jump - dist);

            break;
        }

        case OP_CALL: {
            uint8_t args_count = READ_BYTE();
            Value callee = PEEK(args_count);
            if (!call_value(callee, args_count))
            {
                return INTERPRET_RUNTIME_ERROR;
            };
            frame = &vm.frame[vm.frame_count - 1];

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
    call_frame->ip = call_frame->function->chunk.code;
    call_frame->slots = 0;
}

InterpretResult interpret(const char *source)
{
    ObjectFunction *base_function = compile(source);
    if (base_function == NULL)
        return INTERPRET_COMPILE_ERROR;

    push(VALUE_OBJ(base_function));

    CallFrame *current = &vm.frame[vm.frame_count++];
    current->slots = 0;
    current->ip = base_function->chunk.code;
    current->function = base_function;

    return run();
}

// STACK

void init_stack(Stack *stack)
{
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}
