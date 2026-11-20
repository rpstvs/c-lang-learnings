#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <unistd.h>

struct free_area
{
    uint8_t marker;
    struct free_area *prev;
    bool in_use;
    uint32_t length;
    struct free_area *next;
};

struct stats
{
    int magical_bytes;
    bool my_simple_lock;
    uint32_t amount_of_blocks;
    uint16_t amount_of_pages;
};

typedef struct stats my_stats;
typedef struct free_area area;

const int MAGICAL_BYES = 0x55;
const int BLOCK_MARKER = 0xDD;

char *heap_start;

int main(int argc, char **argv)
{
    return 0;
}

int *an_malloc(ssize_t size)
{
    if (heap_start == NULL)
    {
        heap_start = sbrk(0);
        sbrk(4096);
    }

    char *heap_end = sbrk(0);
    long int length = heap_end - heap_start;

    if ((*heap_start) != MAGICAL_BYES)
    {
        *(heap_start) = MAGICAL_BYES;
        my_stats *malloc_header = (my_stats *)heap_start;
        malloc_header.amount_of_blocks = 1;
        malloc_header.amount_of_pages = 1;
        area *firstblock = (area *)((char *)heap_start + sizeof(my_stats));
        firstblock->marker = BLOCK_MARKER;
        firstblock->in_use = false;
        firstblock->length = length - sizeof(my_stats) - sizeof(area);
        firstblock->prev = NULL;
        firstblock->next = NULL;
    }
    return add_used_block(size);
}

int *add_used_block(ssize_t size)
{
}