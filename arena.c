/**
 * This is a simple implementation of a linear allocator, built solely for PeasantFisher.
 * It makes use of log functions available only with the library cogmasters/concord, which can be removed.
 */

#include <stdlib.h>


#ifndef ARENA
#define ARENA

struct Arena {
    size_t remaining;
    void *beg;
    void *cur;
};

/**
 * Sets up a given memory arena with size `size`.
 * 
 * @param arena pointer to arena pointer to allocate
 * @param size size to allocate, in bytes
 * @return -1 if failed to allocate, otherwise 0
 */
int
arena_init(struct Arena **_arena, size_t _size)
{
    *_arena = malloc(sizeof(struct Arena));
    (*_arena)->remaining = _size;
    (*_arena)->beg = malloc(_size);
    (*_arena)->cur = (*_arena)->beg;
    log_debug("arena_init: Allocated beg @ %p", (*_arena)->beg);
    return (*_arena)->beg == NULL ? -1 : 0;
}

/**
 * Allocates a memory region of size `size`.
 * 
 * @param arena pointer to arena to allocate in
 * @param size size of memory to allocate, in bytes
 * @return `NULL` if size is greater than the remaining arena capacity,
 *         or a `void*` to the region if allocated successfully 
 */
void*
arena_alloc(struct Arena *_arena, size_t _size)
{
    if (_size > _arena->remaining) return NULL;

    _arena->remaining -= _size;

    void *ret = _arena->cur;
    log_debug("arena_alloc: Allocated ret @ %p", ret);
    _size += 8 - (_size % 8);
    _arena->cur = (void*)((char*)(_arena->cur) + _size);
    log_debug("arena_alloc: Arena->cur is now %p", _arena->cur);

    return ret;
}

/**
 * Allocates a memory region with `size` bytes, all zeroed (as bytes).
 * 
 * @param arena pointer to arena to allocate in
 * @param size size of memory to allocate, in bytes
 */
void*
arena_calloc(struct Arena *_arena, size_t _size)
{
    void *ret = arena_alloc(_arena, _size);
    memset(ret, 0, _size);
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
    log_debug("arena_free: freeing beg @ %p\n\t\tfreeing arena @ %p", _arena->beg, _arena);
    free(_arena->beg);
    free(_arena);
}
#endif