#include "stack.h"
#include <error.h>

stack_t stack_init()
{
    stack_t s = (stack_t)checked_malloc(sizeof(struct stack));
    s->max_count = 16;
    s->count = 0;
    s->data = (void**)checked_malloc(sizeof(void*));
    return s;
}

void stack_push(stack_t s, void* element)
{
    if (!s) error(1, 0, "Null stack point given to stack push");
    if (s->count == s->max_count)
        s->data = (void**)checked_grow_alloc(s->data, &s->max_count);
    s->data[s->count++] = element;
}

void* stack_pop(stack_t s)
{
    if (!s) error(1, 0, "Null stack point given to stack pop");
    if (s->count == 0) error(1, 0, "Trying to pop from empty stack");
    return s->data[--s->count];
}

void* stack_top(stack_t s)
{
    if (!s) error(1, 0, "Null stack point given to stack top");
    if (s->count == 0) error(1, 0, "Trying to get top from empty stack");
    return s->data[s->count - 1];
}

size_t stack_count(stack_t s)
{
    if (!s) error(1, 0, "Null stack point given to stack count");
    return s->count;
}