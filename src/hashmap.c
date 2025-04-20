#include "hashmap.h"
#include "value.h"

void init_map(Map *h)
{
    h->size = 0;
    h->capacity = 0;
    h->entries = NULL;
}

void free_map(Map *h)
{
    FREE_ARRAY(Map *, h->entries, h->capacity);
    init_map(h);
}

Entry *find_entry(Entry *entries, ObjectString *key, int capacity)
{
    Entry *grave = NULL;
    int idx = key->hash % capacity;
    for (;;)
    {
        Entry *entry = &entries[idx];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value))
            {
                if (grave == NULL)
                    return entry;
                return grave;
            }
            else
            {
                grave = entry;
            }
        }
        else if (entry->key == key)
            return entry;

        idx = (idx + 1) % capacity;
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

    old->size = 0;

    for (size_t i = 0; i < old->capacity; ++i)
    {
        Entry *old_entry = &old->entries[i];
        if (old_entry->key == NULL)
            continue;

        Entry *entry = find_entry(entries, old_entry->key, capacity);

        entry->key = old_entry->key;
        entry->value = old_entry->value;
        old->size++;
    }

    FREE_ARRAY(Entry, old->entries, old->capacity);
    old->entries = entries;
    old->capacity = capacity;
}

/* TODO : add support for another type of key
 * Maybe we can make the map_set accept `Value` and try to stringify that ??
 * We can use `stringify` function we created before
 *
 * */
void map_set(Map *h, ObjectString *key, Value value)
{
    if ((h->capacity * FACTOR_TERM) <= h->size)
    {
        size_t capacity = GROW_CAPACITY(h->capacity);
        adjust_capacity(h, capacity);
    }

    Entry *entry = find_entry(h->entries, key, h->capacity);
    if (entry->key == NULL && IS_NIL(entry->value))
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

    entry->key = NULL;
    entry->value = VALUE_BOOL(0);

    return 1;
}
