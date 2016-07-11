#ifndef _HEAP_H
#define _HEAP_H

#include <pthread.h>

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct bins;

template<size_t BINS_TYPE_NUM, size_t SB_SIZE, size_t CLASSES_NUM>
struct heap
{
typedef heap<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> heap_type;
typedef superblock<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> sb_type;
typedef superblock_impl<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM> sb_impl_type;

public:

    heap(heap_type* g_heap, size_t id):
        correct(C + (size_t)this)
    {
        gl_heap = g_heap;
        sb_size = SB_SIZE;
        a = 0;
        u = 0;
        this->id = id;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &attr);
    }

    inline void* malloc(size_t size)
    {
        lock();
        size_t index = get_size_class_index(size);
        size_t rounded_up_size = size_classes[index];

        void* ptr = superblocks[index].malloc();


        if (!ptr)
        {
            sb_type* new_sb = 0;

            new_sb = gl_heap->get_superblock(size);
            if (new_sb)
            {
                new_sb->lock();
                put_superblock(index, new_sb);
                ptr = superblocks[index].malloc();
                new_sb->unlock();
            } else
            {
                new_sb = (sb_type*) mmap_align();
                if (!new_sb)
                {
                    print("Out of memory");
                    unlock();
                    return NULL;
                }
                new (new_sb) sb_type(rounded_up_size);
                put_superblock(index, new_sb);
                ptr = superblocks[index].malloc();
            }
        }
        if (ptr)
        {
            sb_type* sb = bins<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM>::get_superblock(ptr);
            size_t block_sz = sb->get_block_size();
            u = u + block_sz;
        }
        unlock();
        return ptr;
    }

    inline void free(void* ptr)
    {
        sb_type* sb = bins<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM>::get_superblock(ptr);
        sb->lock();
        heap_type* heap_p = sb->get_owner();
        if (heap_p == 0)
        {
            sb->free(ptr);
            sb->unlock();
            return;
        }

        heap_p->lock();

        size_t size = sb->get_block_size();
        size_t index = get_size_class_index(size);

        heap_p->free(index, size ,ptr);

        heap_p->unlock();
        sb->unlock();
    }

    inline sb_type* get_superblock(size_t size)
    {
        //lock();
        size_t bin_index = get_size_class_index(size);
        sb_type* sb = superblocks[bin_index].get_emptiest();
        //unlock();
        return sb;
    }

    inline size_t get_id()
    {return id;}

    inline void put_superblock(size_t bin_index, sb_type* sb)
    {
        lock();
        sb->set_owner(this);
        size_t s_u = sb->get_u();
        u = u + s_u;
        a = a + SB_SIZE;

        superblocks[bin_index].put(sb);
        unlock();
    }

    inline void lock()
    {
        pthread_mutex_lock(&mutex);
    }

    inline void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

    inline bool is_valid()
    {
        return correct == C + (size_t)this;
    }

    ~heap()
    {
        pthread_mutex_destroy(&mutex);
    }

private:

    unsigned int num_of_size_classes = BINS_TYPE_NUM;

    const size_t PG_SIZE = 4096;

    const double size_class = 1.2;

    size_t id;

    size_t u,a;

    const double f = 1.0 / 3.0;

    const unsigned int K = 4;

    heap_type* gl_heap;

    size_t sb_size;

    pthread_mutexattr_t attr;
    pthread_mutex_t mutex;

    const size_t correct;

    array<BINS_TYPE_NUM, bins<BINS_TYPE_NUM, SB_SIZE, CLASSES_NUM>> superblocks;

    inline int get_size_class_index(size_t size)
    {
        int i = 0;
        while (size > size_classes[i])
        {
            i++;
        }
        return i;
    }

    inline int crossed_emptiness_treshold()
    {
        //trace(" checking empt tr u = ", u, " a=", a, "\n");
        int c1 = (1.0 - (double)f) * (double)a;
        int c2 = a - (int)K * SB_SIZE;
        if ((int)u < c2 && (int)u < c1)
        {
            return true;
        }
        return false;
    }

    inline void free(size_t bin_index, size_t block_size ,void* ptr)
    {
        superblocks[bin_index].free(ptr);
        if (id == 0)
        {
            return;
        } else
        {
            u = u - block_size;

            if (crossed_emptiness_treshold())
            {
                //trace("crossed emptiness treshold\n");
                sb_type* sblock = superblocks[bin_index].get_emptiest();
                if (sblock)
                {
                    u = u - sblock->get_u();
                    a = a - SB_SIZE;
                    gl_heap->put_superblock(bin_index, sblock);
                }
            }
        }

    }

    inline void* mmap_align()
    {
        void* ptr =  mmap(NULL, SB_SIZE * (SB_SIZE / PG_SIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        size_t mod = (size_t)ptr & (SB_SIZE - 1);
        if (!ptr)
        {
            return NULL;
        }
        void* res = (void*)((size_t)ptr + SB_SIZE - mod);
        size_t b = SB_SIZE - mod;
        size_t a = SB_SIZE * (SB_SIZE / PG_SIZE - 1) - b;
        if (b != 0)
        {
            munmap(ptr, b);
        }
        if (a != 0)
        {
            munmap((void*)((size_t)res + SB_SIZE), a);
        }
        return res;
    }

    enum{C = 0xcafed00d};

    const size_t size_classes[BINS_TYPE_NUM] = {8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 96, 112, 128, 152, 176, 208, 248, 296, 352, 416, 496, 592, 704, 840, 1008, 1208, 1448, 1736, 2272, 2720, 3400, 4096 - sizeof(sb_impl_type)};


};

#endif
