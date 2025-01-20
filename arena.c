#include <stdlib.h>


#ifndef ARENA
#define ARENA

struct Arena {
    size_t remaining;
    size_t capacity;
    void *beg;
    void *cur;
};

/**
 * Sets up a given memory arena with size `size`.
 * 
 * @param arena pointer to arena to allocate
 * @param size size to allocate, in bytes
 * @return -1 if failed to allocate, otherwise 0
 */
int
arena_init(struct Arena *_arena, size_t _size)
{
    _arena = malloc(sizeof(struct Arena));
    _arena->remaining = _size;
    _arena->capacity = _size;
    _arena->beg = malloc(_size);
    return _arena->beg == NULL ? -1 : 0;
}

/**
 * Allocates a memory region of size `size`.
 * 
 * @param arena pointer to arena to allocate in
 * @param size size of memory to allocate
 * @return `NULL` if size is greater than the remaining arena capacity,
 *         or a `void*` to the region if allocated successfully 
 */
void*
arena_alloc(struct Arena *_arena, size_t _size)
{
    if (_size > _arena->remaining) return NULL;
    _arena->remaining -= _size;
    void *ret = _arena->cur;
    _arena->cur = (void*)((char*)(_arena->cur) + _size);
    return ret;
}

/**
 * Frees the arena at `arena`.
 * 
 * @param arena arena to free
 */
void
arena_free(struct Arena *_arena)
{
    free(_arena->beg);
    free(_arena);
}
#endif