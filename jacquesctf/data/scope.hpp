/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_SCOPE_HPP
#define _JACQUES_DATA_SCOPE_HPP

#include <memory>
#include <yactfr/metadata/fwd.hpp>
#include <boost/core/noncopyable.hpp>

#include "er.hpp"
#include "pkt-segment.hpp"

namespace jacques {

class Scope final :
    boost::noncopyable
{
public:
    using SP = std::shared_ptr<Scope>;
    using SPC = std::shared_ptr<const Scope>;

public:
    explicit Scope(yactfr::Scope scope) noexcept;

    explicit Scope(Er::SP er, yactfr::Scope scope, const PktSegment& segment = PktSegment {}) noexcept;

    const Er *er() const noexcept
    {
        return _er.get();
    }

    Er::SP erPtr()
    {
        return _er;
    }

    Er::SPC erPtr() const
    {
        return _er;
    }

    yactfr::Scope scope() const noexcept
    {
        return _scope;
    }

    const yactfr::DataType *dt() const noexcept
    {
        return _dt;
    }

    void dt(const yactfr::DataType& dt) noexcept
    {
        _dt = &dt;
    }

    const PktSegment& segment() const noexcept
    {
        return _segment;
    }

    PktSegment& segment() noexcept
    {
        return _segment;
    }

    void segment(const PktSegment& segment) noexcept
    {
        _segment = segment;
    }

private:
    Er::SP _er;
    const yactfr::Scope _scope;
    const yactfr::DataType *_dt = nullptr;
    PktSegment _segment;
};

} // namespace jacques

#endif // _JACQUES_DATA_SCOPE_HPP
