/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PKT_REGION_VISITOR_HPP
#define _JACQUES_DATA_PKT_REGION_VISITOR_HPP

namespace jacques {

class ContentPktRegion;
class PaddingPktRegion;
class ErrorPktRegion;

class PktRegionVisitor
{
protected:
    explicit PktRegionVisitor() noexcept = default;

public:
    virtual ~PktRegionVisitor() = default;
    virtual void visit(const ContentPktRegion&);
    virtual void visit(const PaddingPktRegion&);
    virtual void visit(const ErrorPktRegion&);
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_REGION_VISITOR_HPP
