#ifndef CWS_VM_H
#define CWS_VM_H

#include "compiler.h"
#include "memory.h"
#include "stdarg.h"
#include "hashmap.h"

#define STACK_MAX 1024

typedef struct
{
    uint8_t capacity;
    uint8_t size;

    Value *items;
} Stack;

typedef struct
{
    Chunk *chunk;
    uint8_t *ip;

    Stack *stack;

    // Value stack[STACK_MAX];
    Value *stackPointer;

    Obj *objects;

    Map *strings;

} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_COMPILE_ERROR,

} InterpretResult;

extern VM vm;

void initVm();
void freeVm();

void InitStack(Stack *stack);
void WriteStack(Stack *stack, Value *value);
void FreeStack(Stack *stack);

InterpretResult interpret(const char *code);

#endif // !CWS_VM_H
