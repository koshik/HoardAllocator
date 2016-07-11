#include <cstring>
#include <unistd.h>
#include <limits>
#include <stdlib.h>
#include <pthread.h>
#include "tracing.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void hoard::print_object(char const* message)
{
    pthread_mutex_lock(&mutex);
    ::write(2, message, strlen(message));
    pthread_mutex_unlock(&mutex);
}

void hoard::print_object(void* px)
{

    char const* hexdigits = "0123456789abcdef";

    char buffer[32];

    size_t n = (size_t)px;

    char* p = buffer + 31;
    *p-- = '\0';

    //*p++ = '0';
    //*p++ = 'x';

    do
    {
        size_t d = n & 0xf;
        n = n>>4;
        *p-- = hexdigits[d];
    }
    while (n != 0);

    *p-- = 'x';
    *p = '0';

    print_object(p);
}

void hoard::print_object(size_t n)
{
    char buffer[32];

    size_t divisor = 1;

    while (divisor <= (n / 10))
        divisor *= 10;

    char* p = buffer;
    do
    {
        *p++ = ((n / divisor) % 10) + '0';
        divisor /= 10;
    }
    while (divisor != 0);

    *p = '\0';

    print_object(buffer);
}

void hoard::print()
{}

bool hoard::trace_enabled()
{
    static bool enabled = (getenv("HOARD_NO_TRACE") == NULL);
    return enabled;
}


