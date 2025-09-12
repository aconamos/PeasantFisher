/*
    Simple C99 hashmap implementation.

    MIT License

    https://github.com/aconamos/chmap
 */

#ifndef CHMAP
#define CHMAP
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BACKING_ARRAY_LENGTH 20
#define ARRAY_GROW_FACTOR 2.0f
#define MAX_LOAD_FACTOR 0.9f

// siphash is a cryptographic hash; it doesn't matter much for our use case, so we can use a bad key.
static const char * SIPHASH_KEY = "abcdef9876543210";


/*
   SipHash reference C implementation

   Copyright (c) 2012-2022 Jean-Philippe Aumasson
   <jeanphilippe.aumasson@gmail.com>
   Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along
   with
   this software. If not, see
   <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

/* default: SipHash-2-4 */
#ifndef cROUNDS
#define cROUNDS 2
#endif
#ifndef dROUNDS
#define dROUNDS 4
#endif

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)                                                        \
    (p)[0] = (uint8_t)((v));                                                   \
    (p)[1] = (uint8_t)((v) >> 8);                                              \
    (p)[2] = (uint8_t)((v) >> 16);                                             \
    (p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)                                                        \
    U32TO8_LE((p), (uint32_t)((v)));                                           \
    U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#define U8TO64_LE(p)                                                           \
    (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |                        \
     ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) |                 \
     ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) |                 \
     ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))

#define SIPROUND                                                               \
    do {                                                                       \
        v0 += v1;                                                              \
        v1 = ROTL(v1, 13);                                                     \
        v1 ^= v0;                                                              \
        v0 = ROTL(v0, 32);                                                     \
        v2 += v3;                                                              \
        v3 = ROTL(v3, 16);                                                     \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = ROTL(v3, 21);                                                     \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = ROTL(v1, 17);                                                     \
        v1 ^= v2;                                                              \
        v2 = ROTL(v2, 32);                                                     \
    } while (0)

#ifdef DEBUG_SIPHASH
#include <stdio.h>

#define TRACE                                                                  \
    do {                                                                       \
        printf("(%3zu) v0 %016" PRIx64 "\n", inlen, v0);                       \
        printf("(%3zu) v1 %016" PRIx64 "\n", inlen, v1);                       \
        printf("(%3zu) v2 %016" PRIx64 "\n", inlen, v2);                       \
        printf("(%3zu) v3 %016" PRIx64 "\n", inlen, v3);                       \
    } while (0)
#else
#define TRACE
#endif

/*
    Computes a SipHash value
    *in: pointer to input data (read-only)
    inlen: input data length in bytes (any size_t value)
    *k: pointer to the key data (read-only), must be 16 bytes 
    *out: pointer to output data (write-only), outlen bytes must be allocated
    outlen: length of the output in bytes, must be 8 or 16
*/
int siphash(const void *in, const size_t inlen, const void *k, uint8_t *out,
            const size_t outlen) {

    const unsigned char *ni = (const unsigned char *)in;
    const unsigned char *kk = (const unsigned char *)k;

    assert((outlen == 8) || (outlen == 16));
    uint64_t v0 = UINT64_C(0x736f6d6570736575);
    uint64_t v1 = UINT64_C(0x646f72616e646f6d);
    uint64_t v2 = UINT64_C(0x6c7967656e657261);
    uint64_t v3 = UINT64_C(0x7465646279746573);
    uint64_t k0 = U8TO64_LE(kk);
    uint64_t k1 = U8TO64_LE(kk + 8);
    uint64_t m;
    int i;
    const unsigned char *end = ni + inlen - (inlen % sizeof(uint64_t));
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    if (outlen == 16)
        v1 ^= 0xee;

    for (; ni != end; ni += 8) {
        m = U8TO64_LE(ni);
        v3 ^= m;

        TRACE;
        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }

    switch (left) {
    case 7:
        b |= ((uint64_t)ni[6]) << 48;
        /* FALLTHRU */
    case 6:
        b |= ((uint64_t)ni[5]) << 40;
        /* FALLTHRU */
    case 5:
        b |= ((uint64_t)ni[4]) << 32;
        /* FALLTHRU */
    case 4:
        b |= ((uint64_t)ni[3]) << 24;
        /* FALLTHRU */
    case 3:
        b |= ((uint64_t)ni[2]) << 16;
        /* FALLTHRU */
    case 2:
        b |= ((uint64_t)ni[1]) << 8;
        /* FALLTHRU */
    case 1:
        b |= ((uint64_t)ni[0]);
        break;
    case 0:
        break;
    }

    v3 ^= b;

    TRACE;
    for (i = 0; i < cROUNDS; ++i)
        SIPROUND;

    v0 ^= b;

    if (outlen == 16)
        v2 ^= 0xee;
    else
        v2 ^= 0xff;

    TRACE;
    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out, b);

    if (outlen == 8)
        return 0;

    v1 ^= 0xdd;

    TRACE;
    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out + 8, b);

    return 0;
}

/*
 * End siphash REFERENCE IMPLEMENTATION
 */


/* --- struct definitions --- */
struct entry {
    int has_entry;
    size_t psl;
    uint64_t keyword;
    size_t backing_array_key;
};

/**
 * The main struct for a map. Contains the backing array, data sizes, and the array length.
 */
struct chmap {
    // The size of any given item. This is needed for backing array allocation and indexing.
    size_t isize;

    // The size of the key type for this array.
    size_t ksize;

    // The number of elements stored in this map. Used to compute load factor, 
    // and for keeping track of the next index in the backing array
    size_t used_size;

    // The number of elements in the translation array and backing array.
    size_t array_size;

    // In more common vernacular, this is the array of buckets.
    // It is where hashes key into and holds indices that reference
    // a spot in the backing array to store data.
    struct entry * translation_array;

    // This is the array that holds actual data.
    void * backing_array;

    // This is the array that holds a stack of indices to use in the backing array.
    // Also, required so that no "holes" are left in the backing array.
    size_t * bais;

    // Top index of the backing array index stack.
    size_t bais_idx;
};

/**
 * Struct that describes a position in a translation array and a PSL to get to it.
 */
struct probe_sequence { 
    /**
     * Index referencing a translation array.
     */
    uint64_t index; 

    /**
     * The distance from the original location.
     */
    size_t psl;
};


/* --- public interface functions */

/**
 * Given an item_size, creates a new hashmap that can store items of item_size.
 * 
 * @param item_size size of data that will be stored in this hashmap
 * @param key_size  size of keys that will be used in this hashmap
 */
struct chmap * chmap_new(const size_t item_size, const size_t key_size);

/**
 * Puts an item into the given map at the given key. Returns 1 if an item was overwritten,
 * or a 0 if the slot was empty.
 */
int chmap_put(
    struct chmap * map, 
    const void * key, 
    const void * item 
);

/**
 * Gets a pointer to the item associated with `key`, or `NULL` if not found.
 */
void * chmap_get(struct chmap * map, const void * key);

/**
 * Deletes the item at `key`.
 */
void chmap_del(struct chmap * map, const void * key);

/**
 * Frees and totally deallocates the given map.
 */
void chmap_free(struct chmap * map);


/* --- debug functions --- */

#ifdef DEBUG
void debug_map(struct chmap * map) {
    for (size_t i = 0; i < map->array_size; i++) {
        struct entry entry = map->translation_array[i];

        if (entry.has_entry)
        printf("bak %3lu; psl: %3lu; tind: %3lu; val: %lu;\n",
            entry.backing_array_key, 
            entry.psl,
            i,
            *((size_t*)map->backing_array + entry.backing_array_key * map->isize)
        );
    }
}

void debug_map_params(struct chmap * map) {
    printf("item_size: %lu\nused_size: %lu\narray_size: %lu\ntarray_addr: %p\nbarray_addr: %p\n",
        map->isize,
        map->used_size,
        map->array_size,
        map->translation_array,
        map->backing_array
    );
}
#endif


/* --- internal helper functions --- */

/**
 * This puts in an item using a hash (instead of the key).
 */
static int chmap_put_hash(
    struct chmap * map,
    const uint64_t hash,
    const void * item
);

static size_t * init_bais_stack(
    size_t numentries
);

static inline void * get_ba_ptr(
    struct chmap * map,
    size_t index
);

static inline void * get_ba_ptr_arr(
    void * ba,
    size_t isize,
    size_t index
);

/**
 * Takes an entry and a location and tries to insert it at the location, performing
 * robinhood shuffling if necessary to maintain low PSL or whatever.
 */
static void bubble_up(struct chmap * map, const struct entry inentry, size_t ind) {
    struct entry grabbed_entry = inentry;

    do {
        struct entry working_entry = map->translation_array[ind];

        if (working_entry.has_entry == 0) {
            // We've encountered an empty spot, and can insert and jump ship.
            map->translation_array[ind] = grabbed_entry;
            return;
        }

        if (working_entry.psl < grabbed_entry.psl) {
            // Take from the rich, and give to the poor - this means,
            // if an entry has a lower PSL than our current one, swap.
            map->translation_array[ind] = grabbed_entry;
            grabbed_entry = working_entry;
        }

        grabbed_entry.psl++;
        ind = (ind + 1) % map->array_size;
    } while (grabbed_entry.has_entry == 1);
}

/**
 * Given a chmap and a key, returns the index where that key should be inserted and the PSL.
 */
static struct probe_sequence probe_array(struct chmap *map, const uint64_t key) {
    uint64_t working_index = key % map->array_size;
    size_t psl = 0;

    struct entry working_entry = map->translation_array[working_index];

    while (working_entry.has_entry == 1 && working_entry.keyword != key && working_entry.psl >= psl) {
        working_entry = map->translation_array[working_index];
        working_index = (working_index + 1) % map->array_size;
        psl++;
    }

    return (struct probe_sequence){ working_index, psl};
}

/**
 * Initializes an array of empty entries, with size `numentries`.
 */
static struct entry * init_translation_array(const size_t numentries) {
    struct entry * entries = calloc(numentries, sizeof(struct entry));
    struct entry empty = {
        .has_entry = 0,
    };

    for (size_t i = 0; i < numentries; i++) {
        entries[i] = empty;
    }

    return entries;
}

/**
 * Given a map, increases its size by ARRAY_GROW_FACTOR and copies the entries and data from the backing array over.
 */
static void grow_map(struct chmap * map) {
    size_t new_size = map->array_size * ARRAY_GROW_FACTOR;
    size_t old_size = map->array_size;

    void * old_backing_array = map->backing_array;
    size_t * old_bais = map->bais;
    struct entry * old_translation_array = map->translation_array;

    void * new_backing_array = malloc(map->isize * new_size);
    size_t * new_bais = init_bais_stack(new_size);
    struct entry * new_translation_array = init_translation_array(new_size);
    
    // Instead of writing some jank code, we'll just reuse the put item operation.
    // This requires us to act like there's no items in the array.
    map->backing_array = new_backing_array;
    map->translation_array = new_translation_array;
    map->array_size = new_size;
    // This is so we can reset backing array indices.
    map->used_size = 0;
    map->bais_idx = new_size - 1;
    map->bais = new_bais;

    for (size_t i = 0; i < old_size; i++) {
        struct entry entry = old_translation_array[i];
        if (entry.has_entry) {
            void * ba_ptr = get_ba_ptr_arr(old_backing_array, map->isize, entry.backing_array_key);
            chmap_put_hash(map, entry.keyword, ba_ptr);
        }
    }

    free(old_backing_array);
    free(old_translation_array);
    free(old_bais);
}

/**
 * Initializes a stack of backing arrays, where the top entry is 0, and the bottom entry is `numentries - 1`.
 */
static size_t * init_bais_stack(size_t numentries) {
    size_t * stack = calloc(numentries, sizeof(size_t));

    for (size_t i = 0; i < numentries; i++) {
        stack[i] = numentries - i;
    }

    return stack;
}

/**
 * Pops an entry off of a backing array index stack from a given map.
 */
static size_t pop_bais_idx(struct chmap * map) {
    size_t bais_idx = map->bais_idx;

    #ifdef DEBUG
    assert(bais_idx >= 0);
    #endif

    size_t ret = map->bais[bais_idx];
    map->bais_idx--;
    return ret;
}

/**
 * Pushes an entry onto a backing array index stack from a given array.
 */
static void push_bais_idx(struct chmap * map, size_t val) {
    map->bais_idx++;

    map->bais[map->bais_idx] = val;
}

/**
 * Given a pointer to a backing array, the item size, and an index, gets the pointer to the item at `index`.
 */
static inline void * get_ba_ptr_arr(
    void * ba,
    size_t isize,
    size_t index
) {
    return ((char*)ba) + index * isize; 
}

/**
 * Given a map and an index, gets the pointer to the item at `index`.
 */
static inline void * get_ba_ptr(struct chmap * map, size_t index) {
    return ((char*)map->backing_array) + index * map->isize;
}


/* --- definitions of public functions --- */

/**
 * Creates a new, empty hashmap with the given item size and key size.
 */
struct chmap * chmap_new(const size_t item_size, const size_t key_size) {
    void * backing_array = calloc(DEFAULT_BACKING_ARRAY_LENGTH, sizeof(item_size));
    struct chmap * map = malloc(sizeof(struct chmap));

    map->bais_idx = DEFAULT_BACKING_ARRAY_LENGTH - 1;
    map->ksize = key_size;
    map->isize = item_size;
    map->used_size = 0;
    map->array_size = DEFAULT_BACKING_ARRAY_LENGTH;
    map->translation_array = init_translation_array(DEFAULT_BACKING_ARRAY_LENGTH);
    map->bais = init_bais_stack(DEFAULT_BACKING_ARRAY_LENGTH);
    map->backing_array = backing_array;

    return map;
}

/**
 * Given a map, a hash, and a pointer to an item, puts the item in the map with key `hash`.
 */
static int chmap_put_hash(
    struct chmap * map,
    const uint64_t hash,
    const void * item
) {
    struct probe_sequence probe = probe_array(map, hash);
    const struct entry looking_at = map->translation_array[probe.index];
    const size_t itemsize = map->isize;

    if (looking_at.has_entry == 0) {
        // We found an empty spot - put it in, no fuss
        size_t bak = pop_bais_idx(map);
        map->used_size++;
        struct entry new_entry = {
            1,
            probe.psl,
            .backing_array_key = bak,
            .keyword = hash,
        };

        
        map->translation_array[probe.index] = new_entry;

        void * ba_ptr = get_ba_ptr(map, bak);

        memcpy(ba_ptr, item, itemsize);
    } else if (looking_at.keyword == hash) {
        // This key already is associated - overwrite it
        void * ba_ptr = get_ba_ptr(map, looking_at.backing_array_key);

        memcpy(ba_ptr, item, itemsize);

        return 1;
    } else {
        // We need to now swap the two out, and put the next one somewhere else down in the array.
        size_t bak = pop_bais_idx(map);
        map->used_size++;
        struct entry new_entry = {
            1,
            probe.psl,
            .backing_array_key = bak,
            .keyword = hash,
        };

        void * ba_ptr = get_ba_ptr(map, bak);

        memcpy(ba_ptr, item, itemsize);

        bubble_up(map, new_entry, probe.index);
    }

    return 0;
}

int chmap_put(
    struct chmap * map,
    const void * key,
    const void * item
) {
    // This is used in place of a uint8_t[8] to provide the same 8 bytes
    // but in a format easier to use as a key.
    uint64_t outword;

    if (map->used_size >= map->array_size * MAX_LOAD_FACTOR) {
        grow_map(map);
    }

    siphash(key, map->ksize, SIPHASH_KEY, (uint8_t*)&outword, 8);

    return chmap_put_hash(map, outword, item);
}

void * chmap_get(struct chmap * map, const void * key) {
    uint64_t outword;

    siphash(key, map->ksize, SIPHASH_KEY, (uint8_t*)&outword, 8);

    size_t working_index = outword % map->array_size;

    struct entry maybe = map->translation_array[working_index];
    while (maybe.has_entry && maybe.keyword != outword) {
        working_index = (working_index + 1) % map->array_size;
        maybe = map->translation_array[working_index];
    };

    if (maybe.has_entry) {
        return (get_ba_ptr(map, maybe.backing_array_key));
    } else {
        return NULL;
    }
}


void chmap_del(struct chmap * map, const void * key) {
    uint64_t outword;

    siphash(key, map->ksize, SIPHASH_KEY, (uint8_t*)&outword, 8);

    size_t working_index = outword % map->array_size;
    struct entry removing_entry = map->translation_array[working_index];
    struct entry next;

    push_bais_idx(map, removing_entry.backing_array_key);

    do {
        size_t cur = working_index;
        working_index = (working_index + 1) % map->array_size;
        next = map->translation_array[working_index];

        next.psl--;

        map->translation_array[cur] = next;
    } while (next.has_entry);

    map->used_size--;
}

void chmap_free(struct chmap * map) {
    free(map->bais);
    free(map->translation_array);
    free(map->backing_array);
    free(map);
}
#endif
