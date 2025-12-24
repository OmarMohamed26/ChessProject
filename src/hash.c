#include "hash.h"
#include "raylib.h"
#include "save.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static bool ExpandDHA(DynamicHashArray *DHA, size_t expandFactor);

DynamicHashArray *InitializeDHA(size_t capacity)
{
    if (capacity < 1)
    {
        capacity = 1;
    }

    DynamicHashArray *DHA = malloc(sizeof(DynamicHashArray));

    if (DHA == NULL)
    {
        TraceLog(LOG_DEBUG, "Failed to create space for DynamicHashArray.");
        return NULL;
    }

    DHA->hashArray = malloc(sizeof(Hash) * capacity);

    if (DHA->hashArray == NULL)
    {
        TraceLog(LOG_DEBUG, "Failed to create space for hashArray.");
        free(DHA);
        return NULL;
    }

    DHA->size = 0;
    DHA->capacity = capacity;

    return DHA;
}

void FreeDHA(DynamicHashArray *DHA)
{
    if (DHA == NULL)
    {
        return;
    }

    free(DHA->hashArray);
    free(DHA);
}

void ClearDHA(DynamicHashArray *DHA)
{
    DHA->size = 0;
}

bool PushDHA(DynamicHashArray *DHA, Hash hash)
{
    if (DHA->size >= DHA->capacity)
    {
        if (!ExpandDHA(DHA, 2))
        {
            return false;
        }
    }

    DHA->hashArray[DHA->size++] = hash;
    return true;
}

Hash PopDHA(DynamicHashArray *DHA)
{
    if (DHA->size == 0)
    {
        return (Hash){0};
    }

    return DHA->hashArray[DHA->size--];
}

static bool ExpandDHA(DynamicHashArray *DHA, size_t expandFactor)
{
    Hash *temp = realloc(DHA->hashArray, sizeof(Hash) * DHA->capacity * expandFactor);

    if (temp == NULL)
    {
        TraceLog(LOG_DEBUG, "Failed to expand the DHA.");
        return false;
    }

    DHA->hashArray = temp;
    DHA->capacity *= expandFactor;
    TraceLog(LOG_DEBUG, "Expanded the DHA to new capacity:%zu", DHA->capacity);
    return true;
}

// CHANGED: Now takes 'currentHash' as argument. NO SIDE EFFECTS.
bool IsRepeated3times(DynamicHashArray *DHA, Hash currentHash)
{
    int count = 1; // 1 for the current position we are holding

    // Optimization: If history is tiny, it can't be a repetition yet
    if (DHA->size < 2) // Need at least 2 previous occurrences to make 3
    {
        return false;
    }

    for (size_t i = 0; i < DHA->size; i++)
    {
        if (memcmp(&DHA->hashArray[i], &currentHash, sizeof(Hash)) == 0)
        {
            if (++count >= 3)
            {
                return true;
            }
        }
    }

    // DO NOT PUSH HERE
    return false;
}

Hash CurrentGameStateHash(void)
{
    unsigned char *currentState = SaveFEN();
    int spaceCount = 0;

    int counter;
    for (counter = 0; spaceCount < 4; spaceCount += currentState[counter++] == ' ')
    {
        ;
    }

    currentState[counter - 1] = '\0';

    int dataSize = (int)strlen((const char *)currentState);
    unsigned int *array = ComputeMD5(currentState, dataSize);

    free(currentState);
    Hash hash = {0};

    if (array)
    {
        memcpy(hash.data, array, sizeof(Hash));
    }

    return hash;
}
