#include "memory.h"

void *reallocate(void *array, int oldSize, int newSize)
{
    if (newSize == 0)
    {
        free(array);
        return NULL;
    }

    oldSize++;

    void* result =  realloc(array, newSize);
    if (result == NULL) exit(69);
    return result;
}
