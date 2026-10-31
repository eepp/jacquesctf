/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
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
