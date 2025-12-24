/**
 * hash.h
 *
 * Responsibilities:
 * - Define the Hash structure used to uniquely identify game states.
 * - Define the DynamicHashArray structure for storing history (for 3-fold repetition).
 * - Export functions to manage the hash history and compute current state hashes.
 */

#ifndef HASH_H
#define HASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Hash
 *
 * Represents a 128-bit hash value (MD5 result).
 * Used to compare board states efficiently.
 */
typedef struct
{
    uint32_t data[4];
} Hash;

/**
 * DynamicHashArray
 *
 * A resizable array container for Hash objects.
 * Used to store the history of all board positions in the current game.
 */
typedef struct
{
    Hash *hashArray; // Pointer to the heap-allocated array
    size_t size;     // Current number of elements
    size_t capacity; // Total allocated slots
} DynamicHashArray;

/* Allocates and initializes a new DynamicHashArray */
DynamicHashArray *InitializeDHA(size_t capacity);

/* Frees the array and the structure itself */
void FreeDHA(DynamicHashArray *DHA);

/* Resets the count to 0 (does not free memory) */
void ClearDHA(DynamicHashArray *DHA);

/* Checks if 'currentHash' exists at least twice in the history */
bool IsRepeated3times(DynamicHashArray *DHA, Hash currentHash);

/* Adds a hash to the history, expanding if necessary */
bool PushDHA(DynamicHashArray *DHA, Hash hash);

/* Removes the last hash (for undo operations) */
Hash PopDHA(DynamicHashArray *DHA);

/* Computes the hash of the current game state */
Hash CurrentGameStateHash(void);

#endif