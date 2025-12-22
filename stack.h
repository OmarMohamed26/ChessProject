#ifndef STACK_H
#define STACK_H

#include "main.h"
#include <stddef.h>

typedef struct
{
    Move *data;
    size_t size;
    size_t capacity;

} MoveStack;

MoveStack *InitializeStack(size_t initialMaximumCapacity);

void FreeStack(MoveStack *stack);

void ClearStack(MoveStack *stack);

bool PushStack(MoveStack *stack, Move move);

bool PopStack(MoveStack *stack, Move *out);

Move PeekStack(MoveStack *stack);

bool IsStackEmpty(MoveStack *stack);

size_t StackSize(MoveStack *stack);

size_t StackCapacity(MoveStack *stack);

#endif