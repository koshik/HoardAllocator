#ifndef TRACING_H
#define TRACING_H

#include <stdlib.h>

namespace hoard {

void print_object(char const*);

void print_object(void* px);

void print_object(size_t n);

void print();


template <typename T>
void print(T t)
{
    print_object(t);
}

template <typename T, typename... Args>
void print(T t, Args ... args)
{
    print_object(t);

    print(args...);
}

bool trace_enabled();

template <typename ... Args>
void trace(Args ... args)
{
    if (!trace_enabled())
        return;

    print(args...);
}

}

#endif // TRACING_H
