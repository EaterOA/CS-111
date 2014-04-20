#ifndef DARRAY_H
#define DARRAY_H

#include "alloc.h"

typedef struct darray* darray_t;

struct darray
{
    void** data;
    size_t count;
    size_t max_count;
};

darray_t darray_init(void);
void darray_free(darray_t a);
void darray_push(darray_t a, void* element);
void* darray_pop(darray_t a);
void* darray_back(darray_t a);
void* darray_get(darray_t a, size_t idx);
void darray_set(darray_t a, size_t idx, void* element);
void darray_swap_back(darray_t a, size_t idx);
void** darray_extract(darray_t a);
size_t darray_count(darray_t a);

#endif