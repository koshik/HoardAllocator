#ifndef _SUPERBLOCK_IMPL_H
#define _SUPERBLOCK_IMPL_H

#include <pthread.h>
#include "stack.h"

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct superblock;


template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct heap;

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct superblock_impl_h;

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct bins;


template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct superblock_impl : public superblock_impl_h<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM>
{
    typedef superblock_impl_h<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> parent;
public:
    superblock_impl(size_t block_size, size_t dt_size):
        parent(block_size, dt_size, (char*)(this + 1))
    {}
private:

    char a[parent::alignment - (sizeof(parent) % parent::alignment)];
};



template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct superblock_impl_h
{

    typedef heap<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> heap_type;
    typedef superblock<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> sb_type;
    typedef superblock_impl_h<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> sb_impl_type;
    typedef bins<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> bins_type;

public:

    const size_t is_valid;

    static const size_t alignment = 8;

    superblock_impl_h(size_t block_size, size_t dt_size, char* data_start):
        is_valid(C + (size_t)this),
        start(data_start)
    {
        this->block_size = block_size;
        position = start;
        data_size = dt_size;
        all_obj_num = (size_t) (dt_size) / block_size;
        reapable_objects = all_obj_num;
        free_obj_num = all_obj_num;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &attr);
        u = 0;
        next = NULL;
        prev = NULL;
    }

    inline void* malloc()
    {
        //lock();
        void* ptr = reap_malloc();
        if (!ptr)
        {
            ptr = free_list_malloc();
        }

        if (ptr)
            u = u + block_size;

        //unlock();
        return ptr;
    }

    inline void free(void* ptr)
    {
        free_list.push(reinterpret_cast<stack::node*>(ptr));
        u = u - block_size;
        free_obj_num++;
        if (free_obj_num == all_obj_num)
        {
            free_list.clear();
            free_obj_num = all_obj_num;
            position = start;
            reapable_objects = all_obj_num;
        }
    }

    inline sb_type* get_next()
    {
        return next;
    }

    inline void set_next(sb_type* n)
    {
        next = n;
    }

    inline void lock()
    {
        pthread_mutex_lock(&mutex);
    }

    inline void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

    inline void* normalize(void* ptr)
    {
        size_t off = (size_t)ptr - (size_t)start;
        ///2
        return (void*) ((size_t)ptr - (off % block_size));
    }

    inline sb_type* get_prev()
    {
        return prev;
    }

    inline void set_prev(sb_type* p)
    {
        prev = p;
    }

    inline size_t get_block_size()
    {
        return block_size;
    }

    inline size_t get_all_obj_num()
    {
        return all_obj_num;
    }

    inline size_t get_free_obj_num()
    {
        return free_obj_num;
    }

    inline void set_owner(heap_type* hp_p)
    {
        owner = hp_p;
    }

    inline heap_type* get_owner()
    {
        return owner;
    }

    inline void set_u(size_t u1)
    {
        u = u1;
    }

    inline size_t get_u()
    {
        return u;
    }

    ~superblock_impl_h()
    {
        pthread_mutex_destroy(&mutex);
        free_list.clear();
    }


private:

    stack free_list;

    size_t block_size;
    size_t data_size;
    size_t sb_size;
    char* const start;
    char* position;
    size_t reapable_objects;
    size_t free_obj_num;
    size_t all_obj_num;

    heap_type* owner;

    sb_type* next;
    sb_type* prev;

    pthread_mutexattr_t attr;
    pthread_mutex_t mutex;


    size_t u; //amount of memory inn use

    inline void* free_list_malloc()
    {
        char* ptr = reinterpret_cast<char*>(free_list.pop());
        if (ptr) free_obj_num--;
        return ptr;
    }

    inline void* reap_malloc()
    {
        if (reapable_objects > 0)
        {
            char* ptr = position;
            position = ptr + block_size;
            reapable_objects--;
            free_obj_num--;
            return ptr;
        }
        return NULL;
    }



    enum {C = 0xcafed00d};
};

#endif
