/* The file is part of Snowman decompiler. */
/* See doc/licenses.asciidoc for the licensing information. */

/* * SmartDec decompiler - SmartDec is a native code to C/C++ decompiler
 * Copyright (C) 2015 Alexander Chernov, Katerina Troshina, Yegor Derevenets,
 * Alexander Fokin, Sergey Levin, Leonid Tsvetkov
 *
 * This file is part of SmartDec decompiler.
 *
 * SmartDec decompiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SmartDec decompiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SmartDec decompiler.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <nc/config.h>

#include <cassert>
#include <cmath>

#include <nc/common/BitTwiddling.h>
#include <nc/common/Types.h>
#include <nc/common/Unused.h>

namespace nc {

/**
 * Floating point value with a size.
 */
class SizedFloatValue {
    /** Size of the value. */
    SmallBitSize size_;

    /** The value represented as ConstantFloatValue. */
    ConstantFloatValue value_;

public:
    /**
     * Constructs a value of zero size.
     */
    SizedFloatValue(): size_(0), value_(0.) {}

    /**
     * Constructor.
     *
     * \param[in] size Size of the value.
     * \param[in] value Value to represent.
     */
    SizedFloatValue(SmallBitSize size, ConstantFloatValue value):
        size_(size),
        value_(value)
    {
        assert(size >= 0);
    }

    /**
     * \return Size of the value.
     */
    SmallBitSize size() const { return size_; }

    /**
     * \return Value as represented.
     */
    ConstantFloatValue value() const { return value_; }

    /**
     * \return Absolute value.
     */
    ConstantFloatValue absoluteValue() const { return std::fabs(value_); }
};

} // namespace nc

/* vim:set et sts=4 sw=4: */
