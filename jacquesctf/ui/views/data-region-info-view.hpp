/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_REGION_INFO_VIEW_HPP
#define _JACQUES_DATA_REGION_INFO_VIEW_HPP

#include "view.hpp"

namespace jacques {

class DataRegionInfoView :
    public View
{
public:
    explicit DataRegionInfoView(const Rectangle& rect,
                                std::shared_ptr<const Stylist> stylist,
                                std::shared_ptr<State> state);

private:
    void _stateChanged(const Message& msg) override;
    void _redrawContent() override;
    void _safePrintScope(yactfr::Scope scope);

private:
    std::shared_ptr<State> _state;
    const ViewStateObserverGuard _stateObserverGuard;
};

} // namespace jacques

#endif // _JACQUES_DATA_REGION_INFO_VIEW_HPP
