/**
 * stack.h
 *
 * Responsibilities:
 * - Define the MoveStack structure for Undo/Redo history.
 * - Export functions for stack management (Push, Pop, Peek, Resize).
 */

#ifndef STACK_H
#define STACK_H

#include "main.h"
#include <stddef.h>

typedef struct MoveStack
{
    Move *data;
    size_t size;
    size_t capacity;

} MoveStack;

/* Allocates and initializes a new stack */
MoveStack *InitializeStack(size_t initialMaximumCapacity);

/* Frees the stack and its data */
void FreeStack(MoveStack *stack);

/* Resets the stack size to 0 without freeing memory */
void ClearStack(MoveStack *stack);

/* Pushes a move onto the stack, resizing if necessary */
bool PushStack(MoveStack *stack, Move move);

/* Pops the top move from the stack into 'out' */
bool PopStack(MoveStack *stack, Move *out);

/* Returns the top move without removing it */
Move PeekStack(MoveStack *stack);

/* Checks if the stack is empty */
bool IsStackEmpty(MoveStack *stack);

/* Returns the current number of items in the stack */
size_t StackSize(MoveStack *stack);

/* Returns the current capacity of the stack */
size_t StackCapacity(MoveStack *stack);

#endif