#include "native.h"
#include <time.h>

/*
 * NATIVE FUNCTION IMPLEMENTATION
 *
 * First we check if the arguments the user provided is the same as we expect
 * Then we can get the args by shifting the index manually
 *
 * Example : 
 * Value first_arg = vm.stack->items[stack_ptr + 0];
 * Value second_arg = vm.stack->items[stack_ptr + 1];
 * Value third_arg = vm.stack->items[stack_ptr + 2];
 * ...etc
 *
 * */

bool check_arity(int expected, int retrieved)
{
    if (expected != retrieved)
    {
        runtime_error("Expected %d arguments but got %d", expected, retrieved);
        return false;
    }
    return true;
}

/* TODO : Add more native function */
bool time_native(int args_count, int stack_ptr, Value *returned)
{
    if (!check_arity(1, args_count))
        return false;

    Value x = vm.stack->items[stack_ptr + 0];

    if(!IS_NUMBER(x)) {
        runtime_error("Expected first argument to be a number");
        return false;
    }

    *returned = VALUE_NUMBER((double)clock() / CLOCKS_PER_SEC + AS_NUMBER(x));
    return true;
}
