/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_SUB_DATA_TYPE_EXPLORER_VIEW_HPP
#define _JACQUES_SUB_DATA_TYPE_EXPLORER_VIEW_HPP

#include "data-type-explorer-view.hpp"

namespace jacques {

class SubDataTypeExplorerView :
    public DataTypeExplorerView
{
public:
    explicit SubDataTypeExplorerView(const Rectangle& rect,
                                     const Stylist& stylist, State& state);

private:
    void _stateChanged(Message msg) override;

private:
    State * const _state;
    const ViewStateObserverGuard _stateObserverGuard;
};

} // namespace jacques

#endif // _JACQUES_SUB_DATA_TYPE_EXPLORER_VIEW_HPP
