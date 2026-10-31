/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_ERROR_PKT_REGION_HPP
#define _JACQUES_DATA_ERROR_PKT_REGION_HPP

#include <memory>

#include "pkt-region.hpp"

namespace jacques {

class ErrorPktRegion final :
    public PktRegion
{
public:
    explicit ErrorPktRegion(const PktSegment& segment) noexcept;

private:
    void _accept(PktRegionVisitor& visitor) override;
};

} // namespace jacques

#endif // _JACQUES_DATA_ERROR_PKT_REGION_HPP
