#ifndef CWS_VM_H
#define CWS_VM_H

#include "compiler.h"
#include "hashmap.h"
#include "memory.h"
#include "stdarg.h"

#define FRAME_MAX 60
#define STACK_MAX FRAME_MAX * 1024

typedef struct
{
    int capacity;
    int size;

    Value *items;
} Stack;

typedef struct
{
    ObjectClosure *closure;

    int slots;
    uint8_t *ip;
} CallFrame;

void init_call_frame(CallFrame *call_frame);
void free_call_frame(CallFrame *call_frame);

typedef struct
{
    // Chunk *chunk;
    // uint8_t *ip;

    Stack *stack;

    // Value stack[STACK_MAX];
    int stack_top;

    int frame_count;
    CallFrame frame[FRAME_MAX];

    Obj *objects;

    Map strings;
    Map globals;

    ObjectUpValue *upvalues;

    int grey_cap;
    int grey_count;
    Obj **grey_stack;

    size_t current_bytes;
    size_t next_gc;

    ObjectString *init_string;
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_COMPILE_ERROR,

} InterpretResult;

extern VM vm;

void init_vm();
void free_vm();

void init_stack(Stack *stack);
void write_stack(Stack *stack, Value *value);
void free_stack(Stack *stack);
void runtime_error(char *format, ...);

void push(Value value);
Value pop();

ObjectString *stringify(Value value);

InterpretResult interpret(const char *code);

#define RUNTIME_ERROR(ip, ...)                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        runtime_error(__VA_ARGS__);                                                                                    \
        print_error_line(ip);                                                                                          \
        resetStack();                                                                                                  \
    } while (0);

#endif // !CWS_VM_H
