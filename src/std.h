#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#ifndef CWS_STD_H

#define CWS_STD_H

typedef struct block_meta *block_ptr;
struct block_meta
{
    size_t size;
    int free;
    int magic;

    block_ptr next;
    block_ptr prev;

    void *ptr;
    char data[1];
};


#define BLOCK_SIZE 40
#define allign4(x) (((x - 1) >> 2) << 2) + 4

int is_valid(void *addr);
block_ptr get_free_block(block_ptr *last, size_t size);
block_ptr get_block_addr(void *addr);
block_ptr request_new_block(block_ptr prev, size_t size);
block_ptr split_block(block_ptr block, size_t size);
block_ptr merge_block(block_ptr block);

void *ws_malloc(size_t size);
void *ws_calloc(size_t nmemb, size_t size);
void ws_free(void *addrs);
void *ws_realloc(void *ptr, size_t size);

void Debug(void *addr);

#endif // !CWS_STD_H
