/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
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
