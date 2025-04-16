#ifndef CWS_MEMORY_H
#define CWS_MEMORY_H

#include "common.h"

#define GROW_CAPACITY(capacity) capacity < 8 ? 8 : capacity * 2;

#define GROW_ARRAY(type, pointer, oldCapacity, newCapacity)                                                            \
    (type *)reallocate(pointer, oldCapacity * sizeof(type), newCapacity * sizeof(type));

#define FREE_ARRAY(type, pointer, oldCapacity) reallocate(pointer, oldCapacity * sizeof(type), 0);

#define ALLOC(type, size) ((type *)reallocate(NULL, 0, size));

void *reallocate(void *array, int oldCapacity, int newCapacity);

#endif // CWS_LONG_MEMORY_H
