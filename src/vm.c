#include "vm.h"
#include "chunk.h"
#include "hashmap.h"
#include "object.h"
#include "value.h"

VM vm;

void resetStack()
{
    // vm.stackPointer = vm.stack;
    vm.stackPointer = vm.stack->items;
}

void update_stack_ptr()
{
    vm.stackPointer = vm.stack->items;
}

void init_vm()
{
    vm.chunk = NULL;
    vm.ip = NULL;
    vm.objects = NULL;

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
        Obj *next = vm.objects->next;
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
    int token_idx = (int)(vm.ip - vm.chunk->code) - 1;
    fprintf(stderr, "[Line : %d] ", find_line(vm.chunk, token_idx));

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputs("\n", stderr);

    resetStack();
}

void push(Value value)
{
    if (vm.stack->size >= vm.stack->capacity)
    {
        int dist = (int)(vm.stackPointer - vm.stack->items);

        uint8_t old_capacity = vm.stack->capacity;
        uint8_t new_capacity = GROW_CAPACITY(old_capacity);
        vm.stack->capacity = new_capacity;
        vm.stack->items = GROW_ARRAY(Value, vm.stack->items, old_capacity, new_capacity);
        vm.stackPointer = vm.stack->items;

        vm.stackPointer += dist;
    }

    *vm.stackPointer = value;
    vm.stackPointer++;
    vm.stack->size++;
}

Value pop()
{
    if (vm.stack->size == 0)
    {
        assert(0 && "Cannot Pop if stack is empty");
    }

    vm.stackPointer--;
    vm.stack->size--;
    return *vm.stackPointer;
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

#define PEEK(index) (vm.stackPointer[(-1 - index)])

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

static InterpretResult run()
{
#define IS_NUMBER(value) (value.type == TYPE_NUMBER)
#define READ_BYTE() (*vm.ip++)
#define READ_SHORT() ((vm.ip += 2), ((uint16_t)(vm.ip[-2] | vm.ip[-1])))
#define READ_CONSTANT() (vm.chunk->constants->values[READ_BYTE()])
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
        vm.chunk->constantsLong->values[res];                                                                          \
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
        for (Value *cur = vm.stack->items; cur < vm.stackPointer; ++cur)
        {
            // Value foo = *cur;
            print_value(*cur);
            printf(",");
        }
        printf("]");
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        printf("\n");
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_RETURN:
            // pop();
            return INTERPRET_OK;

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
                runtime_error("Expected number");
                return INTERPRET_RUNTIME_ERROR;
            }
            (vm.stackPointer - 1)->as.decimal *= -1;
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
                runtime_error("Cannot access undeclared variable : %s\n", name->chars);
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
            push(vm.stack->items[idx]);
            break;
        }

        case OP_SET_LOCAL: {
            uint32_t idx = READ_LONG_BYTE();
            vm.stack->items[idx] = PEEK(0);
            break;
        }

        case OP_JUMP_IF_FALSE: {
            uint16_t jump = READ_SHORT();
            if (is_falsy(PEEK(0)))
            {
                vm.ip += jump;
            }
            break;
        }

        case OP_JUMP_IF_TRUE: {
            uint16_t jump = READ_SHORT();
            if (!is_falsy(PEEK(0)))
            {
                vm.ip += jump;
            }
            break;
        }

        case OP_JUMP: {
            uint16_t jump = READ_SHORT();
            vm.ip += jump;
            break;
        }

        case OP_LOOP: {
            uint16_t jump = READ_SHORT();
            vm.ip -= jump;
            break;
        }

        case OP_MARK_JUMP: {
            vm.ip += 2;
            break;
        }

        case OP_SWITCH: {
            push(VALUE_BOOL(0));
            break;
        }

        case OP_CASE_COMPARE: {
            // [e, false, e]
            // [e, true]
            Value b = pop();
            Value a = PEEK(1);
            *(vm.stackPointer - 1) = VALUE_BOOL(compare(a, b));
            break;
        }

        case OP_SWITCH_JUMP: {
            vm.ip += 2;
            uint8_t idx = *(vm.ip - 2);
            uint16_t jump = ((uint16_t)(vm.chunk->code[idx] | vm.chunk->code[idx + 1]));
            uint8_t dist = *(vm.ip - 1);
            // TODO : debug here
            vm.ip += (jump - dist);

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

InterpretResult interpret(const char *source)
{
    Chunk chunk;
    init_chunk(&chunk);

    int is_error = compile(source, &chunk);
    if (is_error)
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    free_chunk(&chunk);
    return result;
}

// STACK

void init_stack(Stack *stack)
{
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}
