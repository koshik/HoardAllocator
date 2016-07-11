#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <unistd.h>

namespace hoard {



void* hoard_malloc(size_t size);

void hoard_free(void* ptr);

void* hoard_calloc(size_t n, size_t size);

void* hoard_realloc(void* ptr, size_t size);

int hoard_posix_memalign(void** memptr, size_t alignment, size_t size);

}

#endif // ALLOCATOR_H
