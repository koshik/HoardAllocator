#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#include "superblock_impl.h"

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct heap;

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct superblock_impl;

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct superblock
{

    typedef heap<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> heap_type;
    typedef superblock<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> sb_type;
    typedef superblock_impl<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> sb_impl_type;

public:

    superblock(size_t block_size) : sb_impl(block_size, SB_SIZE - sizeof(sb_impl_type))
    {}

    inline void* malloc()
    {
        void* ptr = sb_impl.malloc();
        return ptr;
    }

    inline void free(void* ptr)
    {
        sb_impl.free(ptr);
    }

    inline bool is_valid()
    {
        return sb_impl.is_valid();
    }

    inline sb_type* get_prev()
    {
        return sb_impl.get_prev();
    }

    inline sb_type* get_next()
    {
        return sb_impl.get_next();
    }

    inline void set_prev(sb_type* sb)
    {
        sb_impl.set_prev(sb);
    }

    inline void set_next(sb_type* sb)
    {
        sb_impl.set_next(sb);
    }

    inline void lock()
    {
        sb_impl.lock();
    }

    inline void unlock()
    {
        sb_impl.unlock();
    }

    inline void* normalize(void* ptr)
    {
        return sb_impl.normalize(ptr);
    }

    inline unsigned int get_num_of_all_objs()
    {
        return sb_impl.get_all_obj_num();
    }

    inline unsigned int get_num_of_free_objs()
    {
        return sb_impl.get_free_obj_num();
    }

    inline sb_type* get_superblock(void* ptr)
    {
        return (sb_type*) (((size_t)ptr) & ~((size_t)SB_SIZE - 1));
    }

    inline static size_t get_superblock_size()
    {
        return SB_SIZE;
    }

    inline size_t get_block_size()
    {
        return sb_impl.get_block_size();

    }

    inline void set_owner(heap_type* hp)
    {
        sb_impl.set_owner(hp);
    }

    inline heap_type* get_owner()
    {
        return sb_impl.get_owner();
    }

    inline void set_u(size_t u1)
    {
        sb_impl.set_u(u1);
    }

    inline size_t get_u()
    {
        return sb_impl.get_u();
    }

private:

    sb_impl_type sb_impl;

    char data[SB_SIZE - sizeof(sb_impl_type)];


};

struct large_superblock
{

    large_superblock(size_t sz, void* start):
        size(sz),
        data(start),
        correct(C + (size_t) this)
    {
        size_t* c = (size_t*)((size_t)data - sizeof(size_t));
        *c = correct;
        large_superblock** ptr_to_this = (large_superblock**)((size_t)c - sizeof(large_superblock*));
        *ptr_to_this = this;
    }


    inline bool is_valid()
    {
        return correct == C + (size_t) this;
    }

    inline size_t get_size()
    {
        return size;
    }

    enum { C = 0xcde1230a };
private:
    size_t size;
    void* data;
    large_superblock* ptr;
    const size_t correct;
};

#endif
