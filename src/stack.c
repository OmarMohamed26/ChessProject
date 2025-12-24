/**
 * stack.c
 *
 * Responsibilities:
 * - Implement a dynamic stack data structure for storing Move objects.
 * - Handle memory allocation, resizing, and deallocation for the stack.
 * - Provide standard stack operations (Push, Pop, Peek) with safety checks.
 *
 * Notes:
 * - The stack grows automatically (doubles in capacity) when full.
 * - Uses Raylib's TraceLog for error reporting.
 */

#include "stack.h"
#include "main.h"
#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

const Move EmptyMove = {0};

static bool ExpandStack(MoveStack *stack, size_t size);

/**
 * InitializeStack
 *
 * Creates a new MoveStack with a specified initial capacity.
 *
 * Parameters:
 *  - initialMaximumCapacity: The starting size of the internal array.
 *
 * Returns:
 *  - Pointer to the allocated MoveStack, or NULL if allocation fails.
 */
MoveStack *InitializeStack(size_t initialMaximumCapacity)
{

    if (initialMaximumCapacity < 1)
    {
        initialMaximumCapacity = 1;
    }

    MoveStack *stack = malloc(sizeof(MoveStack));

    if (stack == NULL)
    {
        TraceLog(LOG_DEBUG, "Failed to create space for the stack it self.\n");
        return NULL;
    }

    stack->data = malloc(initialMaximumCapacity * sizeof(Move));

    if (stack->data == NULL)
    {
        TraceLog(LOG_DEBUG, "Failed to create space for the data in the stack.\n");
        free(stack);
        return NULL;
    }

    stack->size = 0;
    stack->capacity = initialMaximumCapacity;

    return stack;
}

/**
 * FreeStack
 *
 * Releases all memory associated with the stack.
 *
 * Parameters:
 *  - stack: Pointer to the stack to free. Safe to pass NULL.
 */
void FreeStack(MoveStack *stack)
{
    if (stack == NULL)
    {
        return;
    }

    free(stack->data);
    free(stack);
}

/**
 * ClearStack
 *
 * Logically clears the stack by resetting the size to 0.
 * Does not free the internal memory buffer.
 */
void ClearStack(MoveStack *stack)
{
    stack->size = 0;
}

/**
 * PushStack
 *
 * Adds a Move to the top of the stack.
 * Automatically resizes the stack if it is full.
 *
 * Returns:
 *  - true if successful.
 *  - false if memory reallocation failed.
 */
bool PushStack(MoveStack *stack, Move move)
{
    if (stack->size >= stack->capacity)
    {
        if (!ExpandStack(stack, 2 * stack->capacity))
        {
            return false;
        }
    }

    stack->data[stack->size++] = move;
    return true;
}

/**
 * ExpandStack
 *
 * Internal helper to resize the stack's data array.
 *
 * Returns:
 *  - true if realloc succeeded.
 *  - false if realloc failed.
 */
static bool ExpandStack(MoveStack *stack, size_t size)
{
    Move *temp = realloc(stack->data, sizeof(Move) * size);

    if (temp == NULL)
    {
        TraceLog(LOG_DEBUG, "Failed to expand the data of the stack.\n");
        return false;
    }

    stack->data = temp;
    stack->capacity = size;
    TraceLog(LOG_DEBUG, "Expanded the the stack to new capacity:%zu", stack->capacity);
    return true;
}

/**
 * PopStack
 *
 * Removes the top Move from the stack and stores it in 'out'.
 *
 * Returns:
 *  - true if a move was popped.
 *  - false if the stack was empty.
 */
bool PopStack(MoveStack *stack, Move *out)
{
    if (stack->size == 0)
    {
        TraceLog(LOG_DEBUG, "The stack is Empty");
        return false;
    }

    Move move = stack->data[stack->size - 1];
    stack->size--;
    *out = move;
    return true;
}

/**
 * PeekStack
 *
 * Returns the top Move without removing it.
 *
 * Returns:
 *  - The top Move.
 *  - EmptyMove (zeroed struct) if the stack is empty.
 */
Move PeekStack(MoveStack *stack)
{
    if (stack->size == 0)
    {
        TraceLog(LOG_DEBUG, "The stack is Empty");
        return EmptyMove;
    }

    return stack->data[stack->size - 1];
}

/**
 * IsStackEmpty
 *
 * Returns true if the stack contains no elements.
 */
bool IsStackEmpty(MoveStack *stack)
{
    return stack->size == 0;
}

/**
 * StackSize
 *
 * Returns the number of elements currently in the stack.
 */
size_t StackSize(MoveStack *stack)
{
    return stack->size;
}

/**
 * StackCapacity
 *
 * Returns the current allocated capacity of the stack.
 */
size_t StackCapacity(MoveStack *stack)
{
    return stack->capacity;
}
