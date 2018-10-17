/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_REGION_VISITOR_HPP
#define _JACQUES_DATA_REGION_VISITOR_HPP

namespace jacques {

class ContentDataRegion;
class PaddingDataRegion;
class ErrorDataRegion;

class DataRegionVisitor
{
public:
    virtual ~DataRegionVisitor();
    virtual void visit(const ContentDataRegion&);
    virtual void visit(const PaddingDataRegion&);
    virtual void visit(const ErrorDataRegion&);
};

} // namespace jacques

#endif // _JACQUES_DATA_REGION_VISITOR_HPP
