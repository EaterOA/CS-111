#ifndef STACK_H
#define STACK_H

#include "alloc.h"

typedef struct stack* stack_t;

struct stack
{
    void** data;
    size_t count;
    size_t max_count;
};

stack_t stack_init();
void stack_push(stack_t s, void* element);
void* stack_pop(stack_t s);
void* stack_top(stack_t s);
size_t stack_count(stack_t s);

#endif