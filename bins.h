#ifndef _BINS_H
#define _BINS_H

#include "superblock.h"

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct bins
{
    typedef superblock<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> sb_type;
    typedef heap<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> heap_type;


    bins()
    {
        pthread_mutex_init(&mutex, NULL);
        for (size_t i = 0 ; i < num_of_classes; i++)
        {
            bins_of_sblocks[i] = 0;
        }
        sb_size = SB_SIZE;
    }


    inline void* malloc()
    {
        for (int i = CLASSES_NUM - 1; i >= 0; i--)
        {
            sb_type* sb = bins_of_sblocks[i];
            if (sb)
            {
                int old_class = get_fullness(sb);
                void* ptr = sb->malloc();
                int new_class = get_fullness(sb);
                if (ptr)
                {
                    if (old_class != new_class)
                    {
                        move_to_class(sb, old_class, new_class);
                    }
                    return ptr;
                }
            }
        }
        return NULL;
    }

    static inline sb_type* get_superblock(void* ptr)
    {
        return (sb_type*) (((size_t)ptr) & ~((size_t)SB_SIZE - 1));
    }

    inline void free(void* ptr)
    {


        sb_type* sb = get_superblock(ptr);
        int old_class = get_fullness(sb);
        sb->free(ptr);
        int new_class = get_fullness(sb);
        if (new_class != old_class)
        {
            move_to_class(sb, old_class, new_class);
        }
    }

    //return emptiest or almost emptiest superblock
    inline sb_type* get_emptiest()
    {

        for (size_t i = 0; i < num_of_classes; i++)
        {

            sb_type* sb = bins_of_sblocks[i];

            while (sb)
            { 
                sb->lock();
                heap_type* owner = sb->get_owner();
                owner->lock(); //wait for the end of another operations
                if (sb == bins_of_sblocks[i]) //check if superblock have been changed
                {
                    remove(sb, i);
                    sb->set_owner(NULL);
                    sb->unlock();
                    owner->unlock();
                    return sb;
                }
                sb->unlock();
                owner->unlock();
                sb = bins_of_sblocks[i];
            }
        }
        return NULL;
    }

    inline void put(sb_type* sb)
    {
        unlocked_put(sb);
    }

    inline void lock()
    {
        pthread_mutex_lock(&mutex);
    }

    inline void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }


    ~bins()
    {
        pthread_mutex_destroy(&mutex);
    }


private:

    //number of emptiness classes. zero class corresponds to completly
    //empty superblocks, (num_of_classes - 1) class - to completly full.
    unsigned int num_of_classes = CLASSES_NUM;

    size_t sb_size;

    array<CLASSES_NUM, sb_type*> bins_of_sblocks;

    pthread_mutex_t mutex;

    inline int get_fullness(sb_type* sb)
    {
        int total = sb->get_num_of_all_objs();
        int free = sb->get_num_of_free_objs();
        size_t res = num_of_classes * (total - free)/total;
        if (res == num_of_classes) return res - 1;
        return res;
    }

    inline void move_to_class(sb_type* sb, int old_class, int new_class)
    {

        remove(sb, old_class);

        unlocked_put(sb);
    }

    inline void unlocked_put(sb_type* sb)
    {

        size_t index = get_fullness(sb);
        sb_type* prev = bins_of_sblocks[index];
        sb->set_next(prev);

        sb->set_prev(0);
        if (prev)
        {
            prev->set_prev(sb);
        }
        bins_of_sblocks[index] = sb;
    }

    inline void remove(sb_type* sb, size_t index)
    {

        sb_type* prev = sb->get_prev();
        sb_type* next = sb->get_next();

        sb->set_next(0);
        sb->set_prev(0);

        if (prev)
        {
            prev->set_next(next);
        }
        if (next)
            next->set_prev(prev);

        if (bins_of_sblocks[index] == sb)
        {
            bins_of_sblocks[index] = next;
        }

    }

};

#endif
