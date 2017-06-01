/* The file is part of Snowman decompiler. */
/* See doc/licenses.asciidoc for the licensing information. */

#pragma once

#include <nc/config.h>

#include <type_traits>
#include <boost/operators.hpp>
#include <boost/predef/other/endian.h>
#include <cassert>
#include <cinttypes>
#include <climits>

namespace nc {

namespace common {

namespace bit_storage {

// is_power_of_2 would preferably be implemented using constexpr function.
template <unsigned int number>
struct is_power_of_2 {
    static const bool value = (number & (number-1)) == 0;
};

template <class T, unsigned int size>
struct ilog2_impl {
    static_assert(is_power_of_2<size>::value, "size must be a power of 2");
    static unsigned int calc(T value) {
        const bool larger_than_size = value >> size;
        int partial_result = 0;
        if (larger_than_size) {
            value >>= size;
            partial_result = size;
        }
        return partial_result | ilog2_impl<T,size/2>::calc(value);
    }
};

template <class T>
struct ilog2_impl<T,1> {
    static unsigned int calc(T value) {
        return value >> 1;
    }
};

template <class T>
unsigned int ilog2(T value) {
    return value == 0 ? -1 : ilog2_impl<T,(sizeof(T)*CHAR_BIT)/2>::calc(value);
}

template <typename T, size_t storage_size>
class BitStorage :
        public boost::shiftable<BitStorage<T, storage_size>,unsigned int>
{
public:
    typedef T bit_data_t;

protected:
    static const size_t chunk_size = sizeof(bit_data_t) * CHAR_BIT;
    static const size_t data_chunks = storage_size / chunk_size;
    static_assert(data_chunks * chunk_size == storage_size, "storage size must be a multiple of sizeof(T) * CHAR_BIT");
    std::array<bit_data_t,data_chunks> data_;

public:
    BitStorage():
        data_{0}
    {}

    BitStorage(const BitStorage& other):
        data_(other.data_)
    {}

    bool operator == (const BitStorage &rhs) const {
        return (data_ == rhs.data_);
    }

    BitStorage operator ~ () const {
        BitStorage value{*this};
        return value;
    }

    BitStorage& operator |= (const BitStorage& rhs){
        for (int i = 0; i < data_chunks; ++i) {
            data_[i] |= rhs.data_[i];
        }
        return *this;
    }

    BitStorage& operator &= (const BitStorage& rhs){
        for (int i = 0; i < data_chunks; ++i) {
            data_[i] &= rhs.data_[i];
        }
        return *this;
    }

    BitStorage& operator ^= (const BitStorage& rhs){
        for (int i = 0; i < data_chunks; ++i) {
            data_[i] ^= rhs.data_[i];
        }
        return *this;
    }

    BitStorage& operator <<= (unsigned int rhs) {
        rhs &= (storage_size-1);
        if (rhs == 0) {
            /* do nothing */
            /* necessary to avoid undefined behavior from x >> (64-rhs)*/
        } else if (rhs > 63) {
            hi_ = lo_ << (rhs-chunk_size);
            lo_ = 0;
        } else {
            hi_ <<= rhs;
            hi_ |= lo_ >> (chunk_size-rhs);
            lo_ <<= rhs;
        }
        return *this;
    }

    BitStorage& operator >>= (unsigned int rhs) {
        rhs &= (storage_size-1);
        if (rhs == 0) {
            /* do nothing */
            /* necessary to avoid undefined behavior from x << (chunk_size-rhs)*/
        } else if (rhs > 63) {
            lo_ = hi_ >> (rhs-chunk_size);
            hi_ = hi_ < 0 ? -1 : 0;
        } else {
            lo_ >>= rhs;
            lo_ |= static_cast<bit_data_t>(hi_) << (chunk_size-rhs);
            hi_ >>= rhs;
        }
        return *this;
    }

};


}} // namespace common::extended_integer

using BitStorage128 = nc::common::bit_storage::BitStorage<uint64_t, 128>;

} // namespace nc

/* vim:set et sts=4 sw=4: */
