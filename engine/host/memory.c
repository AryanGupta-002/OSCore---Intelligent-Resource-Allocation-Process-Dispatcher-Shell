
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

/* Create one large free memory block */
MemoryBlock *create_memory(int start_mb, int total_memory_mb)
{
    if (total_memory_mb <= 0 || start_mb < 0) {
        return NULL;
    }

    MemoryBlock *block = malloc(sizeof(MemoryBlock));

    if (block == NULL) {
        return NULL;
    }

    block->start_mb = start_mb;
    block->size_mb = total_memory_mb;
    block->is_allocated = 0;
    block->next = NULL;

    return block;
}

/* Find the first suitable free block */
MemoryBlock *find_first_fit(MemoryBlock *head, int required_mb)
{
    if (required_mb <= 0) {
        return NULL;
    }

    MemoryBlock *current = head;

    while (current != NULL) {
        if (!current->is_allocated &&
            current->size_mb >= required_mb) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

/* Allocate memory using First Fit */
MemoryBlock *allocate_memory(MemoryBlock *head, int required_mb)
{
    MemoryBlock *block = find_first_fit(head, required_mb);

    if (block == NULL) {
        return NULL;
    }

    /* Split the block if extra memory remains */
    if (block->size_mb > required_mb) {
        MemoryBlock *remaining = malloc(sizeof(MemoryBlock));

        if (remaining == NULL) {
            return NULL;
        }

        remaining->start_mb = block->start_mb + required_mb;
        remaining->size_mb = block->size_mb - required_mb;
        remaining->is_allocated = 0;
        remaining->next = block->next;

        block->next = remaining;
        block->size_mb = required_mb;
    }

    block->is_allocated = 1;

    return block;
}

/* Mark a block as free */
void free_memory(MemoryBlock *block)
{
    if (block != NULL) {
        block->is_allocated = 0;
    }
}

/* Merge adjacent free blocks */

/* Merge adjacent free blocks */
void merge_free_blocks(MemoryBlock *head)
{
    MemoryBlock *current = head;

    while (current != NULL && current->next != NULL) {
        MemoryBlock *next_block = current->next;

        int are_contiguous =
            current->start_mb + current->size_mb
            == next_block->start_mb;

        if (!current->is_allocated &&
            !next_block->is_allocated &&
            are_contiguous) {

            current->size_mb += next_block->size_mb;
            current->next = next_block->next;

            free(next_block);

            /* Do not advance: another free block may follow. */
        } else {
            current = current->next;
        }
    }
}


/* Release all memory-block structures */
void destroy_memory(MemoryBlock *head)
{
    while (head != NULL) {
        MemoryBlock *next_block = head->next;
        free(head);
        head = next_block;
    }
}
