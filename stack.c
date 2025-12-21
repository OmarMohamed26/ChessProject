#include "stack.h"
#include "main.h"
#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

const Move EmptyMove = {0};

MoveStack *InitializeStack(size_t initialMaximumCapacity)
{

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

void FreeStack(MoveStack *stack)
{
    if (stack == NULL)
    {
        return;
    }

    free(stack->data);
    free(stack);
}

void ClearStack(MoveStack *stack)
{
    stack->size = 0;
}

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

bool ExpandStack(MoveStack *stack, size_t size)
{
    Move *temp = realloc(stack->data, sizeof(Move) * size);

    if (temp == NULL)
    {
        TraceLog(LOG_DEBUG, "Failed to expand the data of the stack.\n");
        return false;
    }

    stack->data = temp;
    stack->capacity = size;
    return true;
}

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

Move PeekStack(MoveStack *stack)
{
    if (stack->size == 0)
    {
        TraceLog(LOG_DEBUG, "The stack is Empty");
        return EmptyMove;
    }

    return stack->data[stack->size - 1];
}

bool IsStackEmpty(MoveStack *stack)
{
    return stack->size == 0;
}

size_t StackSize(MoveStack *stack)
{
    return stack->size;
}

size_t StackCapacity(MoveStack *stack)
{
    return stack->capacity;
}
