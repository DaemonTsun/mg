#pragma once

#include <math.h>

namespace mg
{
template<typename T>
struct number_range
{
    T offset;
    T size;
};

template<typename T>
T align_next(T x, T alignment)
{
    if (x % alignment == 0)
        return x;
    
    return ceil(x / alignment) * alignment;
}
    
template<typename T>
T end(const number_range<T> *range)
{
    return range->offset + range->size;
}

template<typename T>
bool overlaps_with(const number_range<T> *a, const number_range<T> *b)
{
    return (end(a) >= b->offset) && (end(b) >= a->offset);
}

template<typename T>
bool contains(const number_range<T> *a, const number_range<T> *b)
{
    return (a->offset <= b->offset) && (end(a) >= end(b));
}

template<typename T>
bool has_space_for(const number_range<T> *range, T size, T alignment)
{
    auto aligned = align_next(range->offset, alignment);
    auto offset_diff = aligned - range->offset;
    auto size_diff = range->size - offset_diff;
    return size_diff >= size;
}
}
