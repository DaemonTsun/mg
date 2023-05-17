
#pragma once

#include "shl/array.hpp"

template<typename T>
inline u32 find_first_free_index(array<T> *arr, u32 first_index)
{
    for (u32 i = first_index; i < arr->size; ++i)
        if (!arr->data[i].used)
            return i;

    return arr->size;
}

