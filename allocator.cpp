#include <unistd.h>

#include <thread>
#include <sys/mman.h>
#include <string.h>
#include <iostream>


#include "tracing.h"

namespace hoard {


#include "array.h"
#include "stack.h"
#include "superblock_impl.h"
#include "superblock.h"
#include "bins.h"
#include "heap.h"


static size_t HEAPS_NUM;
static size_t PAGE_SIZE;


static const size_t SB_SIZE = 8192;
static const size_t EMPTYNESS_CLASSES_NUM = 1000;
static const size_t BLOCK_TYPES_NUM = 32;
static const size_t BIG_OBJECT = 4096 - sizeof(superblock_impl<BLOCK_TYPES_NUM, SB_SIZE, EMPTYNESS_CLASSES_NUM>);

typedef heap<BLOCK_TYPES_NUM, SB_SIZE, EMPTYNESS_CLASSES_NUM> heap_type;
typedef superblock<BLOCK_TYPES_NUM, SB_SIZE, EMPTYNESS_CLASSES_NUM> sb_type;
typedef superblock_impl<BLOCK_TYPES_NUM, SB_SIZE, EMPTYNESS_CLASSES_NUM> sb_impl_type;


heap_type* heaps;

enum { L_SB_C = 0xcde1230a,
       SB_IMPL_C = 0xcafed00d
     };


static void init()
{
    static int initialized = 0;

    if (!initialized)
    {
        initialized = 1;
        size_t numCPU = sysconf( _SC_NPROCESSORS_ONLN );
        HEAPS_NUM = 2 * numCPU;
       // SB_SIZE = 16 * sysconf(_SC_PAGE_SIZE);
        PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
        heaps = (heap_type*) mmap(NULL, sizeof(heap_type) * (HEAPS_NUM), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        new (heaps) heap_type(0, 0);
        for (size_t i = 1; i < HEAPS_NUM; i++) {
            new (heaps + i) heap_type(&heaps[0], i);
        }
    }
}

static void abort(char const* msg)
{
    print(msg);
    std::abort();
}

static void* align(void* ptr, size_t alignment) ///TODO
{
    size_t mod = (size_t)ptr & (alignment - 1);
    if (mod == 0)
        return ptr;
    return (void*) ((size_t)ptr + alignment - mod);
}

static size_t thread_hash()
{
    return std::hash<std::thread::id>()(std::this_thread::get_id()) % (HEAPS_NUM - 1) + 1;
}

void* hoard_malloc(size_t size, size_t alignment)
{
    init();

    void* ptr;
    if (size == 0)
    {
        return NULL;
    }
    size_t data_size = size;
    size = size + alignment - 1;
    if (size > BIG_OBJECT)
    {
        size_t sz = PAGE_SIZE * ((size  + sizeof(large_superblock)+ PAGE_SIZE - 1) / PAGE_SIZE);
        large_superblock* sb = (large_superblock*)mmap(NULL, sz , PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (!sb)
            return NULL;
        void* data = align((void*)((size_t)sb + sizeof(large_superblock)), alignment);
        new (sb) large_superblock(data_size, data);
        return data;
    } else
    {
        size_t heap_index = thread_hash();
        //heap<BLOCK_TYPES_NUM, SB_SIZE, EMPTYNESS_CLASSES_NUM> hp = heaps[heap_index];
        heap_type* hp = &heaps[heap_index];

        ptr = align(hp->malloc(size), alignment);

    }
    return ptr;
}

void* hoard_malloc(size_t size)
{
    return hoard_malloc(size, 1);
}

void hoard_free(void* ptr)
{
    init();
    if (ptr == NULL) return;

    size_t* c = (size_t*)((size_t)ptr - sizeof(size_t));
    large_superblock* l_sb = *(large_superblock**)((size_t)c - sizeof(large_superblock*));
    if (*c == L_SB_C + (size_t)l_sb)
    {
        munmap(l_sb, l_sb->get_size());
    } else {
        sb_type* sb = (sb_type*) (((size_t)ptr) & ~((size_t)SB_SIZE - 1));
        sb_impl_type* sb_impl = reinterpret_cast<sb_impl_type*>(sb);
        if (sb_impl->is_valid == SB_IMPL_C + (size_t)sb_impl)
        {
            ptr = sb->normalize(ptr);
            size_t heap_index = thread_hash();
            heap_type* hp = &heaps[heap_index];

            hp->free(ptr);
        } else
        {
            print("free: given pointer doesn't point to an object ");
            return;
        }
    }
}

void* hoard_calloc(size_t n, size_t size)
{
    init();
    void* ptr;
    if (size == 0)
    {
        ptr = NULL;
    } else
    {
        ptr = hoard_malloc(n * size);
        if (ptr)
        {
            memset(ptr, 0, n * size);
        }
    }
    return ptr;
}

void* hoard_realloc(void* ptr, size_t size)
{
    init();

    void* res = hoard_malloc(size);
    if (ptr != NULL && res != NULL)
    {
        size_t* c = (size_t*)((size_t)ptr - sizeof(size_t));
        large_superblock* l_sb = *(large_superblock**)((size_t)c - sizeof(large_superblock*));
        if (*c == L_SB_C + (size_t)l_sb)
        {
            size_t sz = l_sb->get_size();
            if (size < sz)
                sz = size;
            memcpy(res, ptr, sz);
        } else
        {
            sb_type* sb = (sb_type*) (((size_t)ptr) & ~((size_t)SB_SIZE - 1));
            sb_impl_type* sb_impl = reinterpret_cast<sb_impl_type*>(sb);
            if (sb_impl->is_valid != SB_IMPL_C + (size_t)sb_impl)
            {
                print("realloc: given pointer doesn't point to an object ");
                return NULL;
            }
            /*if (sb->normalize(ptr) != ptr)
            */
            size_t sz = sb->get_block_size();
            if (size < sz)
                sz = size;
            memcpy(res, ptr, sz);
        }

    hoard_free(ptr);
    }
    return res;
}

static bool is_valid_alignment(size_t alignment)
{
    if ((alignment % sizeof(void*)) != 0)
        return false;

    if (!(!(alignment & (alignment - 1)) && alignment))
        return false;

    return true;
}

int hoard_posix_memalign(void** memptr, size_t alignment, size_t size)
{
    init();

    if (!is_valid_alignment(alignment))
        return EINVAL;

    void* ptr = hoard_malloc(size, alignment);

    if (ptr == 0)
        return ENOMEM;

    *memptr = ptr;

    return 0;
}

}


