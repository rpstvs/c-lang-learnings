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
        malloc_header->amount_of_blocks = 1;
        malloc_header->amount_of_pages = 1;
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
    my_stats *malloc_header = get_malloc_header();

    while (malloc_header->my_simple_lock)
    {
        sleep(1);
    }

    malloc_header->my_simple_lock = true;

    area *block = (area *)((char *)heap_start + sizeof(my_stats));
    area *smallest_block = NULL;
    area *last_block = block;

    while (block != NULL)
    {
        assert(block->marker == BLOCK_MARKER);
        if ((block->length + sizeof(area)) >= size && block->in_use == false)
        {
            if (smallest_block == NULL || smallest_block->length > block->length)
            {
                smallest_block = block;
            }
        }
        last_block = block;
        block = block->next;
    }

    if (smallest_block == NULL)
    {
        area *last_block = find_last_block();
        while (last_block->length < size)
        {
            sbrk(4096);
            last_block->length += 4096;
            malloc_header->amount_of_pages += 1;
        }
        smallest_block = last_block;
    }

    smallest_block->in_use = true;

    int must_have_size = smallest_block->length - size - sizeof(area) - 1;
    if (must_have_size <= 0)
    {
        sbrk(4096);
        malloc_header->amount_of_pages += 1;
        last_block->length += 4096;
        must_have_size = smallest_block->length - size - sizeof(area) - 1;
    }
    int remaining_size = must_have_size + 1;
    malloc_header->amount_of_blocks += 1;
    area *new_block = (area *)((char *)smallest_block + sizeof(area) + size);
    new_block->marker = BLOCK_MARKER;
    new_block->prev = smallest_block;
    new_block->next = smallest_block->next;

    if (new_block->next != NULL)
    {
        (new_block->next)->prev = new_block;
    }
    smallest_block->next = new_block;
    new_block->length = remaining_size;
    smallest_block->length = size;
    malloc_header->my_simple_lock = false;
    return (int *)((char *)smallest_block + sizeof(area));
}

bool an_free(void *ptr)
{
    my_stats *malloc_header = get_malloc_header();

    while (malloc_header->my_simple_lock)
    {
        sleep(1);
    }
    malloc_header->my_simple_lock = true;

    area *block = ptr - sizeof(area);
    if (block->marker != BLOCK_MARKER)
    {
        return false;
    }
    else
    {
        block->in_use = false;
        memset(ptr, 0, block->length);
        if (block->next != NULL && (block->next)->in_use == false)
        {
            area *not_used_next_block = block->next;
            if (not_used_next_block != NULL)
            {
                block->next = not_used_next_block->next;

                if (not_used_next_block->next != NULL)
                {
                    not_used_next_block->next->prev = block;
                }
            }
            else
            {
                block->next = NULL;
            }
            memset((void *)not_used_next_block, 0, sizeof(area) + not_used_next_block->length);
            malloc_header->amount_of_blocks -= 1;
        }
        if (block->prev != NULL && (block->prev)->in_use == false)
        {
            area *to_delete_block = block;
            block = block->prev;

            block->length += sizeof(area) + to_delete_block->length;

            block->next = to_delete_block->next;

            if (block->next != NULL)
            {
                block->next->prev = block;
            }
            malloc_header->amount_of_blocks -= 1;
        }
        reduce_heap_size_if_possible();
    }
    malloc_header->my_simple_lock = false;
    return true;
}