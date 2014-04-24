#include "darray.h"
#include <stdlib.h>
#include <error.h>

darray_t darray_init(void)
{
    darray_t a = (darray_t)checked_malloc(sizeof(struct darray));
    a->max_count = 16;
    a->count = 0;
    a->data = (void**)checked_malloc(a->max_count * sizeof(void*));
    return a;
}

void darray_free(darray_t a)
{
    if (!a) error(1, 0, "Null darray pointer given to darray free");
    free(a->data);
    free(a);
}

void darray_push(darray_t a, void* element)
{
    if (!a) error(1, 0, "Null darray pointer given to darray push");
    if (a->count == a->max_count) {
        a->max_count *= 2;
        a->data = (void**)realloc(a->data, a->max_count * sizeof(void*));
        if (!a->data) error(1, 0, "Unable to allocate more memory for darray");
    }
    a->data[a->count++] = element;
}

void* darray_pop(darray_t a)
{
    if (!a) error(1, 0, "Null darray pointer given to darray pop");
    if (a->count == 0) error(1, 0, "Trying to pop from empty darray");
    return a->data[--a->count];
}

void* darray_back(darray_t a)
{
    if (!a) error(1, 0, "Null darray pointer given to darray back");
    if (a->count == 0) error(1, 0, "Trying to get back from empty darray");
    return a->data[a->count - 1];
}

void* darray_get(darray_t a, size_t idx)
{
    if (!a) error(1, 0, "Null darray pointer given to darray get");
    if (a->count <= idx) error(1,0,"Get idx out of bounds");
    return a->data[idx];
}

void darray_set(darray_t a, size_t idx, void* element)
{
    if (!a) error(1, 0, "Null darray pointer given to darray set");
    if (a->count < idx) error(1,0,"Set idx out of bounds");
    if (a->count == idx)
        darray_push(a, element);
    else
        a->data[idx] = element;
}

void darray_swap_back(darray_t a, size_t idx)
{
    if (!a) error(1, 0, "Null darray pointer given to darray swap back");
    if (a->count <= idx) error(1,0,"Swap back idx out of bounds");
    if (a->count == idx+1) return;
    void* temp = darray_pop(a);
    darray_push(a, darray_get(a, idx));
    darray_set(a, idx, temp);
}

void* darray_remove_unordered(darray_t a, size_t idx)
{
    if (!a) error(1, 0, "Null darray pointer given to darray remove unordered");
    if (a->count <= idx) error(1,0,"Remove unordered idx out of bounds");
    darray_swap_back(a, idx);
    return darray_pop(a);
}

void* darray_remove(darray_t a, size_t idx)
{
    if (!a) error(1, 0, "Null darray pointer given to darray remove");
    if (a->count <= idx) error(1,0,"Remove idx out of bounds");
    void* element = darray_get(a, idx);
    a->count--;
    size_t i = idx;
    for (; i < a->count; i++)
        a->data[i] = a->data[i+1];
    return element;
}

void darray_insert(darray_t a, size_t idx, void* element)
{
    if (!a) error(1, 0, "Null darray pointer given to darray remove");
    if (a->count < idx) error(1,0,"Remove idx out of bounds");
    if (a->count == a->max_count) {
        a->max_count *= 2;
        a->data = (void**)realloc(a->data, a->max_count * sizeof(void*));
        if (!a->data) error(1, 0, "Unable to allocate more memory for darray");
    }
    size_t i = idx;
    for (; i < a->count; i++)
        a->data[i+1] = a->data[i];
    a->count++;
    a->data[idx] = element;
}

void** darray_extract(darray_t a)
{
    if (!a) error(1, 0, "Null darray pointer given to darray extract");
    void** data = a->data;
    free(a);
    return data;
}

size_t darray_count(darray_t a)
{
    if (!a) error(1, 0, "Null darray pointer given to darray count");
    return a->count;
}