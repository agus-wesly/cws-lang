#include "hashmap.h"
#include "value.h"
#include "vm.h"

void init_map(Map *h)
{
    h->size = 0;
    h->capacity = 0;
    h->entries = NULL;
}

void free_map(Map *h)
{
    free(h->entries);
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
    // Entry *entries = ALLOC(Entry, (sizeof(Entry) * capacity));
    Entry *entries = (Entry *)calloc(capacity, sizeof(Entry));
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

    // FREE_ARRAY(Entry, old->entries, old->capacity);
    old->entries = entries;
    old->capacity = capacity;
}

bool map_set_value(Map *h, Value key, Value value)
{
    ObjectString *str_key = stringify(key);
    return map_set(h, str_key, value);
}

bool map_set(Map *h, ObjectString *key, Value value)
{
    push(VALUE_OBJ(key));

    if ((h->capacity * FACTOR_TERM) <= h->size)
    {
        size_t capacity = GROW_CAPACITY(h->capacity);
        adjust_capacity(h, capacity);
    }

    pop();

    Entry *entry = find_entry(h->entries, key, h->capacity);
    bool is_new = entry->key == NULL;
    if (is_new && IS_NIL(entry->value))
        h->size++;

    entry->key = key;
    entry->value = value;

    return is_new;
}

bool map_get_value(Map *h, Value key, Value *value)
{
    ObjectString *str_key = stringify(key);
    return map_get(h, str_key, value);
}

bool map_get(Map *h, ObjectString *key, Value *value)
{
    if (h->capacity == 0)
        return false;

    Entry *entry = find_entry(h->entries, key, h->capacity);
    if (entry->key == NULL)
        return false;

    *value = entry->value;
    return true;
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

bool map_delete(Map *h, ObjectString *key)
{
    if (h->size == 0)
        return false;

    Entry *entry = find_entry(h->entries, key, h->capacity);
    if (entry->key == NULL)
        return false;

    entry->key = NULL;
    entry->value = VALUE_BOOL(0);

    return true;
}
