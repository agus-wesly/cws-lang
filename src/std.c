/*
 * This is not needed, we can remove it in the future
 * */
#include "std.h"
#include <string.h>

block_ptr BASE_HEAD = NULL;

block_ptr get_free_block(block_ptr *last, size_t size)
{
    block_ptr current = BASE_HEAD;

    while (current && (current->size <= size || !current->free))
    {
        *last = current;
        current = current->next;
    }

    return current;
}

block_ptr request_new_block(block_ptr prev, size_t size)
{
    block_ptr block;
    block = sbrk(0);
    void *currBlock = sbrk(BLOCK_SIZE + size);
    if (currBlock == (void *)-1)
    {
        return NULL;
    }

    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->free = 0;
    block->magic = 0x7777;
    block->ptr = block->data;

    if (prev)
    {
        prev->next = block;
        block->prev = prev;
    }

    return block;
}

block_ptr split_block(block_ptr block, size_t size)
{
    block_ptr new_block;
    new_block = (block_ptr)block->data + size;
    new_block->size = (block->size - size - BLOCK_SIZE);
    new_block->free = 1;
    new_block->prev = block;
    new_block->next = block->next;
    new_block->ptr = new_block->data;
    new_block->magic = 0x69;

    block->size = size;
    block->next = new_block;

    if (new_block->next)
    {
        new_block->next->prev = new_block;
    }

    return block;
}

int is_valid(void *addr)
{
    if (addr)
    {
        if (BASE_HEAD != NULL && addr < sbrk(0))
        {
            block_ptr p = get_block_addr(addr);
            return (p->ptr == addr);
        }
    };
    return 0;
}

block_ptr merge_block(block_ptr block)
{
    if (block->next && block->next->free)
    {
        block->size += block->next->size + BLOCK_SIZE;
        block->next = block->next->next;

        if (block->next)
        {
            block->next->prev = block;
        }
    }
    return block;
}

block_ptr get_block_addr(void *addr)
{
    char *temp = addr;
    temp = temp - BLOCK_SIZE;
    addr = temp;
    return addr;
}

void Debug(void *addr)
{
    switch (get_block_addr(addr)->magic)
    {
    case 0x12345:
        printf("Status: Reused \n");
        break;
    case 0x7777:
        printf("Status: New block \n");
        break;
    case 0x6969:
        printf("Status: Free \n");
        break;
    case 0x69:
        printf("Status: Splitted \n");
        break;
    default:
        printf("Status: Unknown \n");
        break;
    }
}

void *ws_malloc(size_t size)
{

    if (size <= 0)
    {
        return NULL;
    }

    size_t s = allign4(size);
    block_ptr block;

    if (BASE_HEAD == NULL)
    {
        block = request_new_block(NULL, s);

        if (!block)
            return NULL;
        else
        {
            BASE_HEAD = block;
        }
    }
    else
    {
        block_ptr last = BASE_HEAD;
        block = get_free_block(&last, s);
        if (!block)
        {
            block = request_new_block(last, s);
            if (!block)
                return NULL;
        }
        else
        {
            if (block->size - s >= BLOCK_SIZE + 4)
            {
                block = split_block(block, s);
            };
            block->free = 0;
            block->magic = 0x12345;
        }
    }

    return block->data;
}

void ws_free(void *addrs)
{
    if (is_valid(addrs))
    {
        block_ptr block = get_block_addr(addrs);
        block->free = 1;
        block->magic = 0x6969;

        if (block->prev && block->prev->free)
        {
            block = merge_block(block->prev);
        }

        if (block->next)
        {
            block = merge_block(block);
        }
        else
        {
            if (block->prev)
            {
                block->prev->next = NULL;
            }
            else
            {
                BASE_HEAD = NULL;
                return;
            }
            brk(block);
        }
    }
}

void *ws_realloc(void *addr, size_t size)
{
    if (!addr)
    {
        return ws_malloc(size);
    }

    if (!is_valid(addr))
    {
        return NULL;
    }

    if (size == 0)
    {
        ws_free(addr);
        return NULL;
    }

    size_t s = allign4(size);
    block_ptr block = get_block_addr(addr);

    if (block->free)
    {
        return NULL;
    }

    if (s > block->size)
    {

        if (block->next && block->next->free && (block->size + block->next->size + BLOCK_SIZE) >= s)
        {
            block = merge_block(block);
            return block->data;
        }
        else
        {
            void *new_block = ws_malloc(s);
            memcpy(new_block, block->data, block->size);

            ws_free(block->data);

            return new_block;
        }
    }
    else if (block->size - s >= BLOCK_SIZE + 4)
    {
        block = split_block(block, s);
        return block->data;
    }

    block->size = s;
    return block->data;
}

void *ws_calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *ptr = ws_malloc(total);
    if (ptr)
    {
        memset(ptr, 0, allign4(total));
    }

    return ptr;
}
