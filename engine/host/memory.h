
#ifndef MEMORY_H
#define MEMORY_H

typedef struct MemoryBlock {
    int start_mb;
    int size_mb;
    int is_allocated;

    struct MemoryBlock *next;
} MemoryBlock;

/* Create one initial free memory block */
MemoryBlock *create_memory(int start_mb, int total_memory_mb);

/* Find the first free block large enough for a process */
MemoryBlock *find_first_fit(
    MemoryBlock *head,
    int required_mb
);

/* Allocate memory using the First Fit strategy */
MemoryBlock *allocate_memory(
    MemoryBlock *head,
    int required_mb
);

/* Mark a previously allocated block as free */
void free_memory(MemoryBlock *block);

/* Combine adjacent free memory blocks */
void merge_free_blocks(MemoryBlock *head);

/* Release all memory-block metadata */
void destroy_memory(MemoryBlock *head);

#endif
