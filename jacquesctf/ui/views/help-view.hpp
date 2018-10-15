/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_HELP_VIEW_HPP
#define _JACQUES_HELP_VIEW_HPP

#include <boost/variant.hpp>

#include "scroll-view.hpp"

namespace jacques {

class HelpView :
    public ScrollView
{
public:
    explicit HelpView(const Rectangle& rect,
                      std::shared_ptr<const Stylist> stylist);

private:
    void _drawRows() override;

private:
    void _buildRows();

private:
    struct _SectionRow
    {
        std::string title;
    };

    struct _EmptyRow
    {
    };

    struct _TextRow
    {
        std::string line;
        bool bold = false;
    };

    struct _KeyRow
    {
        std::string key;
        std::string descr;
    };

    struct _SearchSyntaxRow
    {
        std::string descr;
        std::string format;
    };

private:
    std::vector<boost::variant<_SectionRow, _EmptyRow, _TextRow,
                               _KeyRow, _SearchSyntaxRow>> _rows;
    Size _longestRowWidth;
    Size _ssRowFmtPos;
};

} // namespace jacques

#endif // _JACQUES_HELP_VIEW_HPP
