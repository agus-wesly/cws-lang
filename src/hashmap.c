#include "hashmap.h"

void init_map(Map *h)
{
    h->size = 0;
    h->capacity = 0;
    h->entries = NULL;
}

void free_map(Map *h)
{
    FREE_ARRAY(Map *, h->entries, h->capacity);
    /* Should we free each ObjectString ? */
    init_map(h);
}

Entry *find_entry(Entry *entries, ObjectString *key, int capacity)
{
    int idx = key->hash % capacity;
    for (;;)
    {
        Entry *entry = &entries[idx];
        if (entry->key == key || entry->key == NULL)
            return entry;

        idx = (idx + 1) % capacity;
    }
}

Entry *find_next_entry(Entry *entries, Entry *current, int capacity)
{
    int idx = 0;
    for (;;)
    {
        Entry *entry = &entries[idx];
        idx = (idx + 1) % capacity;
        if (entry == current)
        {
            return &entries[idx];
        }
    }
}

void adjust_capacity(Map *old, size_t capacity)
{
    Entry *entries = ALLOC(Entry, sizeof(Entry) * capacity);
    for (size_t i = 0; i < capacity; ++i)
    {
        Entry *entry = &entries[i];
        entry->key = NULL;
        entry->value = VALUE_NIL;
    }

    for (size_t i = 0; i < old->capacity; ++i)
    {
        Entry *old_entry = &old->entries[i];
        if (old_entry->key == NULL)
            continue;

        Entry *entry = find_entry(entries, old_entry->key, capacity);
        entry->key = old_entry->key;
        entry->value = old_entry->value;
    }

    FREE_ARRAY(Entry, old->entries, old->capacity);
    old->entries = entries;
    old->capacity = capacity;
}

void map_set(Map *h, ObjectString *key, Value value)
{
    if ((h->capacity * FACTOR_TERM) <= h->size)
    {
        size_t capacity = GROW_CAPACITY(h->capacity);
        adjust_capacity(h, capacity);
    }

    /*
     * Find entry in hashmap based on key
     * If the found key is null, increase the count
     * */

    Entry *entry = find_entry(h->entries, key, h->capacity);
    if (entry->key == NULL)
        h->size++;

    entry->key = key;
    entry->value = value;
}

int map_get(Map *h, ObjectString *key, Value *value)
{
    if (h->capacity == 0)
        return 0;

    Entry *entry = find_entry(h->entries, key, h->capacity);
    if (entry->key == NULL)
        return 0;

    *value = entry->value;
    return 1;
}

void map_add_all(Map *from, Map *to)
{

    for (size_t i = 0; i < from->capacity; ++i)
    {
        Entry *from_entry = &from->entries[i];
        if (from_entry->key == NULL)
            continue;

        map_set(to, from_entry->key, from_entry->value);
    }
}

int map_delete(Map *h, ObjectString *key)
{
    if (h->size == 0)
        return 0;

    Entry *entry = find_entry(h->entries, key, h->capacity);
    if (entry->key == NULL)
        return 0;

    for (;;)
    {
        Entry *next_entry = find_next_entry(h->entries, entry, h->capacity);
        if (next_entry->key == NULL)
            break;

        uint32_t idx = entry->key->hash % h->capacity;
        uint32_t next_idx = next_entry->key->hash % h->capacity;

        if (idx == next_idx)
        {
            entry->key = next_entry->key;
            entry->value = next_entry->value;

            entry = next_entry;
        }
        else
            break;
    }

    entry->key = NULL;
    entry->value = VALUE_NIL;

    return 1;
}
