// code by agrima
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

/* Create first memory block */
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

/* searches for a free block that is large enough */
MemoryBlock *find_first_fit(MemoryBlock *head, int required_mb)
{
    /* A process cannot require zero or negative memory */
    if (required_mb <= 0) {
        return NULL;
    }
    
    /* current pointer is holding the address of the first block*/
    MemoryBlock *current = head;

    while (current != NULL) {
        if (current->is_allocated == 0 &&
            current->size_mb >= required_mb) {
            /* Returns the first matching block*/
            return current;
        }

        current = current->next;    /* Moves to the next memory block.*/
    }

    return NULL;    /* If no suitable block was found, it returns NULL. */
}

/* Allocate memory to a process using First Fit */
MemoryBlock *allocate_memory(MemoryBlock *head, int required_mb)
{
    MemoryBlock *block = find_first_fit(head, required_mb);

    if (block == NULL) {
        return NULL;
    }

    /* Split the block if extra memory remains */
    if (block->size_mb > required_mb) {
        /*A new block is created to represent the remaining free memory.*/
        MemoryBlock *remaining = malloc(sizeof(MemoryBlock));

        if (remaining == NULL) {
            return NULL;
        }

        remaining->start_mb = block->start_mb + required_mb; /* The remaining memory starts immediately after the process’s memory.*/
        remaining->size_mb = block->size_mb - required_mb;
        remaining->is_allocated = 0;
        remaining->next = block->next;

        block->next = remaining;
        block->size_mb = required_mb;
    }

    block->is_allocated = 1;

    return block;
}

/* releases a block */
void free_memory(MemoryBlock *block)
{
    if (block != NULL) {
        block->is_allocated = 0;
    }
}

/* Merge adjacent free blocks, prevents memory from becoming many small unusable blocks */
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

            current->size_mb += next_block->size_mb;  /*Adds the next block’s size to the current block*/
            current->next = next_block->next;

            free(next_block);

            /* Do not advance: another free block may follow. */
        } else {
            current = current->next;
        }
    }
}


/* Release all memory-block structures after simulation*/
void destroy_memory(MemoryBlock *head)
{
    while (head != NULL) {
        MemoryBlock *next_block = head->next;
        free(head);
        head = next_block;
    }
}
