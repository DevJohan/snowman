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

#include <nc/common/SizedFloatValue.h>
#include <nc/common/Types.h>

#include "Expression.h"
#include "Types.h"

namespace nc {
namespace core {
namespace likec {

/**
 * Float constant.
 */
class FloatConstant: public Expression {
    SizedFloatValue value_; ///< Value of the constant.
    const FloatType *type_; ///< Type of the constant.

public:
    /**
     * Class constructor.
     *
     * \param[in] value Value of the constant.
     * \param[in] type Type of the constant. The type size must be equal to the value size.
     */
    FloatConstant(const SizedFloatValue &value, const FloatType *type);

    /**
     * Class constructor.
     *
     * \param[in] value Value.
     * \param[in] type Type of the constant.
     */
    FloatConstant(ConstantFloatValue value, const FloatType *type);

    /**
     * \return The value of the constant.
     */
    const SizedFloatValue &value() const { return value_; }

    /**
     * Sets the value of the constant.
     *
     * \param[in] value The new value.
     */
    void setValue(const SizedFloatValue &value);

    /**
     * \return Type of the constant.
     */
    const FloatType *type() const { return type_; }
};

} // namespace likec
} // namespace core
} // namespace nc

NC_SUBCLASS(nc::core::likec::Expression, nc::core::likec::FloatConstant, nc::core::likec::Expression::FLOAT_CONSTANT)

/* vim:set et sts=4 sw=4: */
