#ifndef _ARRAY_H
#define _ARRAY_H

template <unsigned long N, typename T>
struct array
{
    inline T& operator [] (unsigned long i)
    {
        return m[i];
    }


    inline const T& operator[](unsigned long i) const{
        return m[i];
    }

private:
    T m[N];
};


#endif
