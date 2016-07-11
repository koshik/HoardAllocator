// compile: g++ --shared -std=c++11 -fPIC -g -o libhoard.so malloc-intercept.cpp allocator.cpp tracing.cpp
// run with no trace: LD_PRELOAD=./libhoard.so HOARD_NO_TRACE=1 krusader
// run with trace: LD_PRELOAD=./libhoard.so HOARD_NO_TRACE=0 krusader
// run with trace: LD_PRELOAD=./libhoard.so krusader

#include <cerrno>
#include <stdlib.h>

#include "allocator.h"
#include "tracing.h"

using namespace hoard;

namespace
{
    __thread bool inside_malloc = false;

    struct recuirsion_guard
    {
        recuirsion_guard()
        {
            if (inside_malloc)
            {
                print("recursive call\n");
                abort();
            }

            inside_malloc = true;
        }

        ~recuirsion_guard()
        {
            inside_malloc = false;
        }

    private:
        recuirsion_guard(recuirsion_guard const&);
        recuirsion_guard& operator=(recuirsion_guard const&);
    };
}


extern "C"
void* malloc(size_t size)
{
    recuirsion_guard rg;

    void *p = hoard_malloc(size);

    trace("malloc: size = ", size, " ", p, "\n");

    return p;
}

extern "C"
void* calloc(size_t n, size_t size)
{
    recuirsion_guard rg;



    void* p = hoard_calloc(n, size);

    trace("calloc: nmemb=", n, ", size=", size, " ", p, "\n");

    return p;
}

extern "C"
void free(void *ptr)
{
    recuirsion_guard rg;

    trace("free: ", ptr, "\n");

    hoard_free(ptr);


}

extern "C"
void* realloc(void *ptr, size_t size)
{
    recuirsion_guard rg;



    void* p = hoard_realloc(ptr, size);

    trace("realloc: size=", size, ", ptr=", ptr, ". ", p, "\n");

    return p;
}

extern "C"
int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    recuirsion_guard rg;

    int result = hoard_posix_memalign(memptr, alignment, size);

    trace("posix_memalign: alignment=", alignment, ", size=",size, ", memptr=",*memptr, "\n");

    return result;
}

extern "C"
void *valloc(size_t size)
{
    recuirsion_guard rg;

    print("deprecated function valloc is not supported\n");
    abort();
}

extern "C"
void *memalign(size_t boundary, size_t size)
{
    recuirsion_guard rg;

    print("deprecated function memalign is not supported\n");
    abort();
}
