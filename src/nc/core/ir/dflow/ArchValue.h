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

#include <nc/common/Types.h>

#include "ArchAbstractValue.h"

namespace nc {
namespace core {
namespace ir {
namespace dflow {

/**
 * Dataflow information about a term.
 */
class ArchValue {
    ArchAbstractValue abstractValue_; ///< Abstract value of the term, in the host byte order.

public:
    /**
     * Class constructor.
     *
     * \param size Size of the term.
     */
    ArchValue(SmallBitSize size);

    /**
     * \return Abstract value of the term.
     */
    const ArchAbstractValue &abstractValue() const { return abstractValue_; }

    /**
     * Sets the abstract value of the term.
     *
     * \param value New abstract value.
     *
     * The value is resized to the size given to constructor of this class.
     */
    void setAbstractValue(ArchAbstractValue value) { abstractValue_ = value.resize(abstractValue_.size()); }

};

} // namespace dflow
} // namespace ir
} // namespace core
} // namespace nc

/* vim:set et sts=4 sw=4: */
