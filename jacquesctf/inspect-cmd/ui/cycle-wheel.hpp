/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_CYCLE_WHEEL_HPP
#define _JACQUES_INSPECT_COMMAND_UI_CYCLE_WHEEL_HPP

#include <vector>
#include <utility>
#include <initializer_list>

namespace jacques {

/*
 * Simple wheel which cycles through a fixed list of values.
 *
 * Use next() to go to the next value (possibly wrapping to the first
 * value), and curVal() to get the current selected value.
 */
template <typename ValT>
class CycleWheel final
{
public:
    /*
     * Builds a cycle wheel with `vals` as its content.
     */
    explicit CycleWheel(std::initializer_list<ValT> vals) :
        _vals {std::move(vals)},
        _curIt {_vals.begin()}
    {
    }

    /*
     * Selects to the next value, possibly wrapping and selecting the
     * first.
     */
    void next()
    {
        ++_curIt;

        if (_curIt == _vals.end()) {
            _curIt = _vals.begin();
        }
    }

    /*
     * Current selected value.
     */
    const ValT& curVal() const noexcept
    {
        return *_curIt;
    }

private:
    std::vector<ValT> _vals;
    typename std::vector<ValT>::const_iterator _curIt;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_CYCLE_WHEEL_HPP
