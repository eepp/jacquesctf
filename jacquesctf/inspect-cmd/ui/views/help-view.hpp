/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_HELP_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_HELP_VIEW_HPP

#include <variant>

#include "scroll-view.hpp"

namespace jacques {

class HelpView final :
    public ScrollView
{
public:
    explicit HelpView(const Rect& rect, const Stylist& stylist);

private:
    void _drawRows() override;

private:
    void _buildRows();

private:
    struct _SectionRow final
    {
        std::string title;
    };

    struct _SubSectionRow final
    {
        std::string title;
    };

    struct _EmptyRow final
    {
    };

    struct _TextRow final
    {
        std::string line;
        bool bold = false;
    };

    struct _KeyRow final
    {
        std::string key;
        std::string descr;
    };

    struct _SearchSyntaxRow final
    {
        std::string descr;
        std::string fmt;
    };

private:
    std::vector<std::variant<_SectionRow, _SubSectionRow, _EmptyRow, _TextRow,
                             _KeyRow, _SearchSyntaxRow>> _rows;
    Size _longestRowWidth;
    Size _ssRowFmtPos;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_HELP_VIEW_HPP
