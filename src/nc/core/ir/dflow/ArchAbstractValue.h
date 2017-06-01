/* The file is part of Snowman decompiler. */
/* See doc/licenses.asciidoc for the licensing information. */

#pragma once

#include <nc/config.h>

#include <algorithm> /* std::max */
#include <cassert>

#include <nc/common/BitTwiddling.h>
#include <nc/common/SizedValue.h>
#include <nc/common/Types.h>
#include <nc/common/Unused.h>

namespace nc {
namespace core {
namespace ir {
namespace dflow {

/**
 * An integer value of a variable size with bits taking values from the power set of {0, 1}.
 */
class ArchAbstractValue {
    SmallBitSize size_;         ///< Size of the abstract value.
    ArchExtendedConstantValue zeroBits_;    ///< Bit mask of positions that can be zero.
    ArchExtendedConstantValue oneBits_;     ///< Bit mask of positions that can be one.

public:
    /**
     * Constructs a value of zero size.
     */
    ArchAbstractValue(): size_(0), zeroBits_(0), oneBits_(0) {}

    /**
     * Constructor.
     *
     * \param size      Size of the abstract value.
     * \param zeroBits  Bit mask of positions that can be zero.
     * \param oneBits   Bit mask of positions that can be one.
     *
     * Given bit masks are truncated to the given size.
     */
    ArchAbstractValue(SmallBitSize size, ArchExtendedConstantValue zeroBits, ArchExtendedConstantValue oneBits):
        size_(size), zeroBits_(bitTruncate(zeroBits, size_)), oneBits_(bitTruncate(oneBits, size_))
    {
        assert(size >= 0);
    }

private:
    /**
     * Helper enum to construct values without truncation.
     */
    enum Exact {};

public:
    /**
     * Helper member to construct values without truncation.
     */
    static const Exact exact = static_cast<Exact>(0);

    /**
     * Constructor.
     *
     * \param size      Size of the abstract value.
     * \param zeroBits  Bit mask of positions that can be zero.
     * \param oneBits   Bit mask of positions that can be one.
     * \param exact     Exact construction flag.
     *
     * Given bit masks are truncated to the given size.
     */
    ArchAbstractValue(SmallBitSize size, ArchExtendedConstantValue zeroBits, ArchExtendedConstantValue oneBits, Exact exact):
        size_(size), zeroBits_(zeroBits), oneBits_(oneBits)
    {
        NC_UNUSED(exact);
        assert(size >= 0);
        assert(bitTruncate(zeroBits, size) == zeroBits);
        assert(bitTruncate(oneBits, size) == oneBits);
    }

    /**
     * Constructs a concrete abstract value from a sized value.
     *
     * \param x Sized value.
     */
    ArchAbstractValue(const SizedValue &x):
        size_(x.size()), zeroBits_(x.value() ^ bitMask<ArchExtendedConstantValue>(size_)), oneBits_(x.value())
    {}

    /**
     * \return Size of the abstract value.
     */
    SmallBitSize size() const { return size_; }

    /**
     * Resizes the abstract value to the given size.
     *
     * \param size New size.
     *
     * \return *this.
     */
    ArchAbstractValue &resize(SmallBitSize size) {
        if (size < size_) {
            zeroBits_ = bitTruncate(zeroBits_, size);
            oneBits_  = bitTruncate(oneBits_,  size);
        }
        size_ = size;
        return *this;
    }

    /**
     * \return Bit mask of bits that can be zero.
     */
    ArchExtendedConstantValue zeroBits() const { return zeroBits_; }

    /**
     * \return Bit mask of bits that can be one.
     */
    ArchExtendedConstantValue oneBits() const { return oneBits_; }

    /**
     * \return True if all (at least one) bits of the value are known to be either one or zero, false otherwise.
     */
    bool isConcrete() const { return size_ > 0 && (zeroBits_ ^ oneBits_) == bitMask<ArchExtendedConstantValue>(size_); }

    /**
     * \return True if the value has a bit that can be both zero and one, false otherwise.
     */
    bool isNondeterministic() const { return (zeroBits_ & oneBits_) != 0; }

    /**
     * \return Concrete value of this abstract value, if this value is concrete.
     *         Otherwise, the behavior is undefined.
     */
    SizedValue asConcrete() const { assert(isConcrete()); return SizedValue(size_, oneBits_, SizedValue::exact); }

    /**
     * Shifts the value by the given number of bits.
     * If the number of bits is positive, the shift is to the left.
     * If the number of bits is negative, the shift is to the right.
     * Increases (decreases) the value size by the same amount of bits.
     * If the resulting size is less than zero, it is set to zero.
     *
     * \param nbits Number of bits.
     *
     * \return *this.
     */
    ArchAbstractValue &shift(int nbits) {
        size_ = std::max(size_ + nbits, 0);
        zeroBits_ = bitShift(zeroBits_, nbits);
        oneBits_  = bitShift(oneBits_,  nbits);
        return *this;
    }

    /**
     * Ands each component of the abstract value with the mask.
     *
     * \param mask Bit mask.
     *
     * \return *this.
     */
    ArchAbstractValue &project(ArchExtendedConstantValue mask) {
        zeroBits_ &= mask;
        oneBits_  &= mask;
        return *this;
    }

    /**
     * Zero-extends *this to the given size.
     *
     * \param size New size.
     *
     * \return *this.
     */
    ArchAbstractValue &zeroExtend(SmallBitSize size) {
        assert(size > size_);

        zeroBits_ |= shiftLeft(bitMask<ArchExtendedConstantValue>(size - size_), size_);
        size_ = size;

        return *this;
    }

    /**
     * Sign-extends *this to given size.
     *
     * \param size New size.
     *
     * \return *this.
     */
    ArchAbstractValue &signExtend(SmallBitSize size) {
        assert(size > size_);

        auto signBitMask = shiftLeft<ArchExtendedConstantValue>(1, size_ - 1);

        if (zeroBits_ & signBitMask) {
            zeroBits_ |= shiftLeft(bitMask<ArchExtendedConstantValue>(size - size_), size_);
        }
        if (oneBits_ & signBitMask) {
            oneBits_ |= shiftLeft(bitMask<ArchExtendedConstantValue>(size - size_), size_);
        }

        size_ = size;

        return *this;
    }

};

inline
ArchAbstractValue operator&(const ArchAbstractValue &a, const ArchAbstractValue &b) {
    assert(a.size() == b.size());
    return ArchAbstractValue(a.size(), a.zeroBits() | b.zeroBits(), a.oneBits() & b.oneBits(), ArchAbstractValue::exact);
}

inline
ArchAbstractValue operator|(const ArchAbstractValue &a, const ArchAbstractValue &b) {
    assert(a.size() == b.size());
    return ArchAbstractValue(a.size(), a.zeroBits() & b.zeroBits(), a.oneBits() | b.oneBits(), ArchAbstractValue::exact);
}

inline
ArchAbstractValue operator^(const ArchAbstractValue &a, const ArchAbstractValue &b) {
    assert(a.size() == b.size());
    return ArchAbstractValue(a.size(),
        (a.zeroBits() & b.zeroBits()) | (a.oneBits() & b.oneBits()),
        (a.oneBits() & b.zeroBits()) | (a.zeroBits() & b.oneBits()),
        ArchAbstractValue::exact);
}

inline
ArchAbstractValue operator~(const ArchAbstractValue &a) {
    return ArchAbstractValue(a.size(), a.oneBits(), a.zeroBits());
}

inline
ArchAbstractValue operator<<(const ArchAbstractValue &a, const ArchAbstractValue &b) {
    if (b.isConcrete()) {
        auto nbits = b.asConcrete().value();
        return ArchAbstractValue(a.size(),
            shiftLeft(a.zeroBits(), nbits) | bitMask<ArchExtendedConstantValue>(nbits),
            shiftLeft(a.oneBits(), nbits));
    } else {
        return ArchAbstractValue(a.size(), -1, -1);
    }
}

inline
ArchAbstractValue operator>>(const ArchAbstractValue &a, const ArchAbstractValue &b) {
    if (b.isConcrete()) {
        auto nbits = b.asConcrete().value();

        return ArchAbstractValue(a.size(),
            shiftRight(a.zeroBits(), nbits) | shiftLeft(bitMask<ArchExtendedConstantValue>(nbits), a.size() - nbits),
            shiftRight(a.oneBits(), nbits));
    } else {
        return ArchAbstractValue(a.size(), -1, -1);
    }
}

inline
ArchAbstractValue operator==(const ArchAbstractValue &a, const ArchAbstractValue &b) {
    assert(a.size() == b.size());

    return ArchAbstractValue(1,
        (a.zeroBits() & b.oneBits()) || (a.oneBits() & b.zeroBits()),
        ((a.zeroBits() & b.zeroBits()) | (a.oneBits() & b.oneBits())) == bitMask<ArchExtendedConstantValue>(a.size()),
        ArchAbstractValue::exact);
}
} // namespace dflow
} // namespace ir
} // namespace core
} // namespace nc

/* vim:set et sts=4 sw=4: */
