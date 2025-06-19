#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "common.h"
#include "hash.h"
#include "memory.h"
#include "value.h"

#define FACTOR_TERM 0.75

typedef struct
{
    ObjectString *key;
    Value value;
} Entry;

typedef struct
{
    size_t size;
    size_t capacity;

    Entry *entries;
} Map;

void init_map(Map *h);
void free_map(Map *h);
bool map_set_value(Map *h, Value key, Value value);
bool map_set(Map *h, ObjectString *key, Value value);
bool map_get(Map *h, ObjectString *key, Value *value);
bool map_get_value(Map *h, Value key, Value *value);
bool map_delete(Map *h, ObjectString *key);
void print_map(Map *h, int level);

#endif // !HASH_MAP_H
