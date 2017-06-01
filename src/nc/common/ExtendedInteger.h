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

class SignedInteger128;
class UnsignedInteger128;
template <typename T>
class IntegerBase128;
//class UnsignedDivide128Impl;

template <typename T>
struct IntegerBase128Config;

template <>
struct IntegerBase128Config<SignedInteger128>{
    typedef std::uint64_t unsigned_data_t;
    typedef std::int64_t signed_data_t;

    typedef unsigned_data_t lo_data_t;
    typedef signed_data_t hi_data_t;
    typedef signed_data_t base_data_t;
};

template <>
struct IntegerBase128Config<UnsignedInteger128>{
    typedef std::uint64_t unsigned_data_t;
    typedef std::int64_t signed_data_t;

    typedef unsigned_data_t lo_data_t;
    typedef unsigned_data_t hi_data_t;
    typedef unsigned_data_t base_data_t;
};


template <class T>
class UnsignedDivide128Impl : public T {
public:
    typedef T base_class;
    typedef typename base_class::unsigned_data_t unsigned_data_t;
    typedef typename base_class::signed_data_t signed_data_t;

    UnsignedDivide128Impl(unsigned_data_t lo = 0):
        base_class(0, lo)
    {}

    UnsignedDivide128Impl(unsigned_data_t hi, unsigned_data_t lo):
        base_class(hi, lo)
    {}

    template <typename U>
    UnsignedDivide128Impl(const  IntegerBase128<U>& data):
    base_class(data.hi(), data.lo())
    {}

private:
    static UnsignedDivide128Impl impl_multiply(unsigned_data_t lhs, unsigned_data_t rhs) {
        UnsignedDivide128Impl result;

        unsigned_data_t lhsHi = lhs >> 32;
        unsigned_data_t lhsLo = lhs & 0xFFFFFFFF;

        unsigned_data_t rhsHi = rhs >> 32;
        unsigned_data_t rhsLo = rhs & 0xFFFFFFFF;

        unsigned_data_t a = lhsHi * rhsHi;
        unsigned_data_t b = lhsHi * rhsLo;
        unsigned_data_t c = lhsLo * rhsHi;

        result.hi_ = a + (b >> 32) + (c >> 32);
        b <<= 32;
        c <<= 32;

        result.lo_ = lhsLo * rhsLo + b;
        if (result.lo_ < b) result.hi_++;
        result.lo_ += c;
        if (result.lo_ < c) result.hi_++;
        return result;
    }

    static UnsignedDivide128Impl impl_multiply_high_part(const UnsignedDivide128Impl& lhs, const UnsignedDivide128Impl& rhs) {
        UnsignedDivide128Impl result;

        result = impl_multiply(lhs.lo_, rhs.lo_);
        result.lo_ = result.hi_; result.hi_ = 0; // result >>= 64;

        result += impl_multiply(lhs.hi_, rhs.lo_);
        const UnsignedDivide128Impl a = impl_multiply(lhs.lo_, rhs.hi_);
        result += a;
        bool result_overflow = result < a;
        result.lo_ = result.hi_; result.hi_ = result_overflow ? 1: 0; // result >>= 64;

        result += impl_multiply(lhs.hi_, rhs.hi_);

        return result;
    }

    static UnsignedDivide128Impl impl_divide(const UnsignedDivide128Impl& dividend, const uint32_t& divisor) {
        UnsignedDivide128Impl result = 0;
        if (dividend.hi_ == 0) {
            if ( dividend.lo_ < divisor) {
                result.lo_ = 0;
            } else {
                result.lo_ = dividend.lo_ / divisor;
            }
        } else {
            static const unsigned_data_t base_max = UINT64_C(-1);
            /* calculate (1<<64)/divisor */
            unsigned_data_t base_quot = base_max / divisor;
            unsigned_data_t base_rem = base_max % divisor + 1;
            if (base_rem == divisor) { /* side note: divisor is a power of 2 */
                base_quot ++;
                base_rem = 0;
            }

            unsigned_data_t hi_rem = dividend.hi_;
            result.hi_ = 0;
            if (hi_rem >= divisor) {
                result.hi_ = hi_rem / divisor;
                hi_rem = hi_rem % divisor;
            }
            UnsignedDivide128Impl M{0,base_rem * hi_rem};
            // M <= (max base_rem) * (max hi_rem) ->
            // M <= 0xFFFF FFFE    * 0xFFFF FFFE  = 0xFFFF FFFC 0000 0004
            M += dividend.lo_;
            // M <=    (max dividend.lo_) + (max M_old) ->
            // M <= 0xFFFF FFFF FFFF FFFF + 0xFFFF FFFC 0000 0004 = 0x1 FFFF FFFC 0000 0003, M.hi_ <= 1
            assert(M.hi_ <= 1);

            result.lo_ = hi_rem * base_quot;
            if (M.hi_ != 0) { // if M >= 0x1 0000 0000 0000 0000
                result += base_quot;
                M.lo_ += base_rem; // M <= 0x1 FFFF FFFC 0000 0003 + 0xFFFF FFFE = 0x1 FFFF FFFD 0000 0001
            }
            result += M.lo_ / divisor;
        }
        return result;
    }

    static UnsignedDivide128Impl impl_divide(const UnsignedDivide128Impl& dividend, const unsigned_data_t& divisor) {
        UnsignedDivide128Impl result = 0;
        if (dividend.hi_ == 0) {
            if ( dividend.lo_ < divisor) {
                result.lo_ = 0;
            } else {
                result.lo_ = dividend.lo_ / divisor;
            }
        } else if ((divisor >> 32) == 0) {
            result = impl_divide(dividend, static_cast<uint32_t>(divisor));
        } else {
            unsigned int initial_shift = 1+nc::common::bit_storage::ilog2<uint32_t>(divisor>>32);

            unsigned_data_t divisor_normalized = divisor << (32-initial_shift);
            unsigned_data_t divisor_normalized_hi32 = divisor_normalized >> 32;

            assert(divisor_normalized_hi32 < UINT64_C(1)<<32);
            assert(divisor_normalized_hi32 >= UINT64_C(1)<<31);

            static const unsigned_data_t base_max = UINT64_C(-1);
            /* calculate reciprocal_divisor ~= (1<<191)/divisor_normalized */
            UnsignedDivide128Impl reciprocal_divisor{(base_max/divisor_normalized_hi32)<<31,0};

            {
                UnsignedDivide128Impl tmpa = impl_multiply(divisor_normalized, reciprocal_divisor.hi_);
                tmpa<<=1;

                if (static_cast<signed_data_t>(tmpa.hi_) < 0) {
                    reciprocal_divisor += impl_multiply_high_part(reciprocal_divisor, -tmpa);
                } else {
                    reciprocal_divisor -= impl_multiply_high_part(reciprocal_divisor, tmpa);
                }
            }

            {
                UnsignedDivide128Impl tmpa = impl_multiply(divisor_normalized, reciprocal_divisor.hi_);
                tmpa += impl_multiply(divisor_normalized, reciprocal_divisor.lo_).hi_;
                tmpa <<= 1;

                if (static_cast<signed_data_t>(tmpa.hi_) < 0) {
                    reciprocal_divisor += impl_multiply_high_part(reciprocal_divisor, -tmpa);
                } else {
                    reciprocal_divisor -= impl_multiply_high_part(reciprocal_divisor, tmpa);
                }
            }

            result = impl_multiply_high_part(dividend, reciprocal_divisor);
            result >>= (31+initial_shift);
        }
        return result;
    }
public:
    static UnsignedDivide128Impl unsigned_divide(const UnsignedDivide128Impl& dividend, const UnsignedDivide128Impl& divisor) {
        UnsignedDivide128Impl result = 0;
        if (divisor.hi_ == 0) {
            result = impl_divide(dividend, divisor.lo_);
        } else if (dividend < divisor) {
            /* result = 0, do nothing */
        } else {
            unsigned int divisor_magnitude = 1 + nc::common::bit_storage::ilog2<unsigned_data_t>(divisor.hi_);
            unsigned int dividend_magnitude = 1 + nc::common::bit_storage::ilog2<unsigned_data_t>(dividend.hi_);

            unsigned int shift = dividend_magnitude - divisor_magnitude;
            UnsignedDivide128Impl divisor_normalized = divisor << shift;
            UnsignedDivide128Impl remainder = dividend;

            unsigned_data_t quotient = 0;// Quotient less than max for unsigned_data_t;
            unsigned_data_t current_power = static_cast<unsigned_data_t>(1) << shift;
            do{
                if (divisor_normalized <= remainder) {
                    remainder -= divisor_normalized;
                    quotient += current_power;
                }
                divisor_normalized >>= 1;
            } while( (current_power >>= 1) != 0 && remainder != 0);
            result = quotient;
        }
        return result;
    }
};

template <typename T>
class IntegerBase128 :
        public boost::operators<T>,
        public boost::shiftable<IntegerBase128<T>,unsigned int>
{
public:
    typedef IntegerBase128Config<T> config_t;
    typedef typename config_t::hi_data_t hi_data_t;
    typedef typename config_t::lo_data_t lo_data_t;
    typedef typename config_t::base_data_t base_data_t;

    typedef typename config_t::unsigned_data_t unsigned_data_t;
    typedef typename config_t::signed_data_t signed_data_t;

    typedef UnsignedDivide128Impl<IntegerBase128<UnsignedInteger128>> UnsignedDivide128;

protected:
    lo_data_t lo_;
    hi_data_t hi_;

public:
    IntegerBase128(signed_data_t lo = 0):
        lo_(static_cast<lo_data_t>(lo)),
        hi_((lo < 0) ? -1 : 0)
    {}

    IntegerBase128(unsigned_data_t lo):
        lo_(static_cast<lo_data_t>(lo)),
        hi_(0)
    {}

    IntegerBase128(const T &value):
        lo_(value.lo_),
        hi_(value.hi_)
    {}

    IntegerBase128(const hi_data_t& hi, const lo_data_t& lo):
        lo_(lo),
        hi_(hi)
    {}

    lo_data_t lo() const { return lo_; }
    hi_data_t hi() const { return hi_; }

    operator T& (){ return static_cast<T&>(*this); }
    operator const T& () const { return static_cast<const T&>(*this); }

    explicit operator lo_data_t () const {
        return lo_;
    }

    IntegerBase128& operator = (const signed_data_t &value) {
        lo_ = static_cast<lo_data_t>(value);
        hi_ = (value < 0) ? -1 : 0;
        return *this;
    }

    IntegerBase128& operator = (const unsigned_data_t &value) {
        lo_ = static_cast<lo_data_t>(value);
        hi_ = 0;
        return *this;
    }

    bool operator < (const IntegerBase128 &rhs) const {
        return (hi_ == rhs.hi_) ? lo_ < rhs.lo_ : hi_ < rhs.hi_;
    }

    bool operator == (const IntegerBase128 &rhs) const {
        return (hi_ == rhs.hi_ && lo_ == rhs.lo_);
    }

    IntegerBase128& operator += (const IntegerBase128 &rhs) {
        hi_ += rhs.hi_;
        lo_ += rhs.lo_;
        if (lo_ < rhs.lo_) hi_++;
        return *this;
    }

    IntegerBase128& operator -= (const IntegerBase128 &rhs) {
        *this += -rhs;
        return *this;
    }

    IntegerBase128 operator - () const {
        return (lo_ == 0) ? IntegerBase128(-hi_, 0) : IntegerBase128(~hi_, -lo_);
    }

    IntegerBase128 operator ~ () const {
        IntegerBase128 value{~hi_,~lo_};
        return value;
    }

    IntegerBase128& operator |= (const IntegerBase128& rhs){
        lo_ |= rhs.lo_;
        hi_ |= rhs.hi_;
        return *this;
    }

    IntegerBase128& operator &= (const IntegerBase128& rhs){
        lo_ &= rhs.lo_;
        hi_ &= rhs.hi_;
        return *this;
    }

    IntegerBase128& operator ^= (const IntegerBase128& rhs){
        lo_ ^= rhs.lo_;
        hi_ ^= rhs.hi_;
        return *this;
    }

    IntegerBase128& operator ++ () {
        return *this += static_cast<unsigned_data_t>(1);
    }

    IntegerBase128& operator -- () {
        return *this -= static_cast<unsigned_data_t>(1);
    }

    IntegerBase128& operator <<= (unsigned int rhs) {
        rhs &= (128-1);
        if (rhs == 0) {
            /* do nothing */
            /* necessary to avoid undefined behavior from x >> (64-rhs)*/
        } else if (rhs > 63) {
            hi_ = lo_ << (rhs-64);
            lo_ = 0;
        } else {
            hi_ <<= rhs;
            hi_ |= lo_ >> (64-rhs);
            lo_ <<= rhs;
        }
        return *this;
    }

    IntegerBase128& operator >>= (unsigned int rhs) {
        rhs &= (128-1);
        if (rhs == 0) {
            /* do nothing */
            /* necessary to avoid undefined behavior from x << (64-rhs)*/
        } else if (rhs > 63) {
            lo_ = hi_ >> (rhs-64);
            hi_ = hi_ < 0 ? -1 : 0;
        } else {
            lo_ >>= rhs;
            lo_ |= static_cast<unsigned_data_t>(hi_) << (64-rhs);
            hi_ >>= rhs;
        }
        return *this;
    }

    IntegerBase128& operator *= (const IntegerBase128& rhs) {
        const unsigned_data_t this_lo_lo = lo_ & 0xFFFFFFFF;
        const unsigned_data_t this_lo_hi = lo_ >> 32;

        const unsigned_data_t rhs_lo_lo = rhs.lo_ & 0xFFFFFFFF;
        const unsigned_data_t rhs_lo_hi = rhs.lo_ >> 32;

        hi_ = lo_ * rhs.hi_ + hi_ * rhs.lo_;
        hi_ += this_lo_hi * rhs_lo_hi;

        const unsigned_data_t part_hi_a = this_lo_lo * rhs_lo_hi;
        const unsigned_data_t part_hi_b = this_lo_hi * rhs_lo_lo;
        hi_ += (part_hi_a >> 32) + (part_hi_b >> 32);

        lo_ = (part_hi_a & 0xFFFFFFFF) + (part_hi_b & 0xFFFFFFFF);
        if (lo_ > 0xFFFFFFFF) hi_++;
        lo_ <<= 32;

        const unsigned_data_t part_lo = this_lo_lo * rhs_lo_lo;
        lo_ += part_lo;
        if (lo_ < part_lo) hi_++;

        return *this;
    }

    IntegerBase128& negate() {
        if (lo_ == 0) {
            hi_ = -hi_;
        } else {
            hi_ = ~hi_;
            lo_ = -lo_;
        }
        return static_cast<T&>(*this);
    }

    IntegerBase128& operator /= (const IntegerBase128& rhs) {
        bool negate_result = (hi_ < 0) != (rhs.hi_ < 0);

        UnsignedDivide128 dividend{hi_ < 0 ? -*this : *this};
        UnsignedDivide128 divisor{rhs.hi_ < 0 ? -rhs : rhs};

        UnsignedDivide128 unsigned_result = UnsignedDivide128::unsigned_divide(dividend, divisor);
        if (negate_result) unsigned_result = -unsigned_result;
        lo_ = unsigned_result.lo();
        hi_ = unsigned_result.hi();

        return *this;
    }

    IntegerBase128& operator %= (const IntegerBase128& rhs) {
        IntegerBase128 division_result = *this;
        division_result /= rhs;

        *this -= division_result * rhs;
        return *this;
    }
};

class SignedInteger128 : public IntegerBase128<SignedInteger128> {
public:
    typedef IntegerBase128<SignedInteger128> base_type;

    SignedInteger128(signed_data_t lo = 0):
        base_type(lo)
    {}

    SignedInteger128(const SignedInteger128 &value):
        base_type(value)
    {}

    SignedInteger128(const hi_data_t& hi, const lo_data_t& lo):
        base_type(hi,lo)
    {}

    static SignedInteger128 extendMul (base_data_t lhs, base_data_t rhs) {
        SignedInteger128 result;

        bool negate = (lhs < 0) != (rhs < 0);

        if (lhs < 0) lhs = -lhs;
        unsigned_data_t int1Hi = static_cast<unsigned_data_t>(lhs) >> 32;
        unsigned_data_t int1Lo = static_cast<unsigned_data_t>(lhs & 0xFFFFFFFF);

        if (rhs < 0) rhs = -rhs;
        unsigned_data_t int2Hi = static_cast<unsigned_data_t>(rhs) >> 32;
        unsigned_data_t int2Lo = static_cast<unsigned_data_t>(rhs & 0xFFFFFFFF);

        unsigned_data_t a = int1Hi * int2Hi;
        unsigned_data_t b = int1Lo * int2Lo;
        unsigned_data_t c = int1Hi * int2Lo + int1Lo * int2Hi;

        result.hi_ = a + (c >> 32);
        result.lo_ = c << 32;
        result.lo_ += b;
        if (result.lo_ < b) result.hi_++;
        if (negate) result = -result;
        return result;
    }
};

class UnsignedInteger128 : public IntegerBase128<UnsignedInteger128> {
public:
    typedef IntegerBase128<UnsignedInteger128> base_type;

    UnsignedInteger128(unsigned_data_t lo = 0):
        base_type(lo)
    {}

    UnsignedInteger128(const UnsignedInteger128 &value):
        base_type(value)
    {}

    UnsignedInteger128(const unsigned_data_t& hi, const unsigned_data_t& lo):
        base_type(hi,lo)
    {}

    static UnsignedInteger128 extendMul(unsigned_data_t lhs, unsigned_data_t rhs) {
        UnsignedInteger128 result;

        unsigned_data_t lhsHi = lhs >> 32;
        unsigned_data_t lhsLo = lhs & 0xFFFFFFFF;

        unsigned_data_t rhsHi = rhs >> 32;
        unsigned_data_t rhsLo = rhs & 0xFFFFFFFF;

        unsigned_data_t a = lhsHi * rhsHi;
        unsigned_data_t b = lhsHi * rhsLo;
        unsigned_data_t c = lhsLo * rhsHi;

        result.hi_ = a + (b >> 32) + (c >> 32);
        b <<= 32;
        c <<= 32;

        result.lo_ = lhsLo * rhsLo + b;
        if (result.lo_ < b) result.hi_++;
        result.lo_ += c;
        if (result.lo_ < c) result.hi_++;
        return result;
    }
};

}} // namespace common::extended_integer

using SignedInteger128 = nc::common::bit_storage::SignedInteger128;
using UnsignedInteger128 = nc::common::bit_storage::UnsignedInteger128;

} // namespace nc

/* vim:set et sts=4 sw=4: */
