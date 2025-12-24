#ifndef HASH_H
#define HASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint32_t data[4];
} Hash;

typedef struct
{
    Hash *hashArray;
    size_t size;
    size_t capacity;
} DynamicHashArray;

DynamicHashArray *InitializeDHA(size_t capacity);

void FreeDHA(DynamicHashArray *DHA);

void ClearDHA(DynamicHashArray *DHA);

bool IsRepeated3times(DynamicHashArray *DHA, Hash currentHash);

bool PushDHA(DynamicHashArray *DHA, Hash hash);

Hash PopDHA(DynamicHashArray *DHA);

Hash CurrentGameStateHash(void);

#endif