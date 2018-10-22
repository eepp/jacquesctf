/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_CYCLE_WHEEL_HPP
#define _JACQUES_CYCLE_WHEEL_HPP

#include <vector>
#include <initializer_list>

namespace jacques {

/*
 * Simple wheel which cycles through a fixed list of values.
 *
 * Use CycleWheel::next() to go to the next value (possibly wrapping to
 * the first value), and CycleWheel::currentValue() to get the current
 * selected value.
 */
template <typename ValueT>
class CycleWheel
{
public:
    /*
     * Builds a cycle wheel with `values` as its content.
     */
    explicit CycleWheel(std::initializer_list<ValueT> values) :
        _values {values},
        _curIt {std::begin(_values)}
    {
    }

    /*
     * Selects to the next value, possibly wrapping and selecting the
     * first.
     */
    void next()
    {
        ++_curIt;

        if (_curIt == std::end(_values)) {
            _curIt = std::begin(_values);
        }
    }

    /*
     * Current selected value.
     */
    const ValueT& currentValue() const noexcept
    {
        return *_curIt;
    }

private:
    std::vector<ValueT> _values;
    typename std::vector<ValueT>::const_iterator _curIt;
};

} // namespace jacques

#endif // _JACQUES_CYCLE_WHEEL_HPP
