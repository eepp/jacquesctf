/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_PADDING_PKT_REGION_HPP
#define _JACQUES_DATA_PADDING_PKT_REGION_HPP

#include "pkt-region.hpp"

namespace jacques {

class PaddingPktRegion final :
    public PktRegion
{
public:
    explicit PaddingPktRegion(const PktSegment& segment, Scope::SP scope);

private:
    void _accept(PktRegionVisitor& visitor) override;
};

} // namespace jacques

#endif // _JACQUES_DATA_PADDING_PKT_REGION_HPP
