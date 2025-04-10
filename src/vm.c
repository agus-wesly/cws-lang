#include "vm.h"

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

void initVm()
{
    vm.chunk = NULL;
    vm.ip = NULL;

    Stack *stack_ptr = malloc(sizeof(Stack));
    vm.stack = stack_ptr;
    InitStack(vm.stack);

    update_stack_ptr();
}

void freeVm()
{
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

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants->values[READ_BYTE()])
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
    });

#define HANDLE_BINARY(op)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        Value b = pop();                                                                                               \
        Value a = pop();                                                                                               \
        push(a op b);                                                                                                  \
    } while (0);

    for (;;)
    {

#ifdef DEBUG_TRACE_EXECUTION

        printf("[");
        for (Value *cur = vm.stack->items; cur < vm.stackPointer; ++cur)
        {
            PrintValue(cur);
            printf(",");
        }
        printf("]");
        printf("\n");
        DisassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        printf("\n");
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_RETURN:
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
        case OP_NEGATE: {
            *(vm.stackPointer - 1) = (*(vm.stackPointer-1) * -1);
            break;
        }
        case OP_ADD:
            HANDLE_BINARY(+);
            break;
        case OP_SUBTRACT:
            HANDLE_BINARY(-);
            break;
        case OP_MULTIPLY:
            HANDLE_BINARY(*);
            break;
        case OP_DIVIDE:
            HANDLE_BINARY(/);
            break;

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
    InitChunk(&chunk);

    int is_error = compile(source, &chunk);
    if (is_error)
    {
        FreeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    FreeChunk(&chunk);
    return result;
}

// STACK

void InitStack(Stack *stack)
{
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}
