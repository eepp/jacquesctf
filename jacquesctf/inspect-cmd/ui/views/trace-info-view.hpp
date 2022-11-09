/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_TRACE_TYPE_INFO_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_TRACE_TYPE_INFO_VIEW_HPP

#include <unordered_map>
#include <vector>
#include <memory>
#include <yactfr/metadata/fwd.hpp>

#include "scroll-view.hpp"
#include "data/ts.hpp"
#include "data/duration.hpp"

namespace jacques {

class TraceInfoView final :
    public ScrollView
{
public:
    explicit TraceInfoView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);

private:
    void _drawRows() override;
    void _appStateChanged(Message msg) override;
    void _buildTraceInfoRows(const Trace& metadata);
    void _buildRows();

private:
    struct _Row
    {
        virtual ~_Row() = default;
    };

    struct _SectionRow final :
        public _Row
    {
        explicit _SectionRow(std::string title) :
            title {std::move(title)}
        {
        }

        std::string title;
    };

    struct _EmptyRow final :
        public _Row
    {
    };

    struct _SepRow final :
        public _Row
    {
    };

    struct _PropRow :
        public _Row
    {
        _PropRow(std::string key) :
            key {std::move(key)}
        {
        }

        std::string key;
        Index valOffset = 0;
    };

    struct _SIntPropRow final :
        public _PropRow
    {
        _SIntPropRow(const std::string& key, const long long val) :
            _PropRow {key},
            val {val}
        {
        }

        long long val;
        bool sepNumber = true;
    };

    struct _DataLenPropRow final :
        public _PropRow
    {
        _DataLenPropRow(std::string key, const DataLen len) :
            _PropRow {std::move(key)},
            len {len}
        {
        }

        DataLen len;
    };

    struct _TsPropRow final :
        public _PropRow
    {
        _TsPropRow(std::string key, const Ts& ts) :
            _PropRow {std::move(key)},
            ts {ts}
        {
        }

        Ts ts;
    };

    struct _DurationPropRow final :
        public _PropRow
    {
        _DurationPropRow(std::string key, const Duration& duration) :
            _PropRow {std::move(key)},
            duration {duration}
        {
        }

        Duration duration;
    };

    struct _StrPropRow :
        public _PropRow
    {
        _StrPropRow(std::string key, const std::string& val) :
            _PropRow {std::move(key)},
            val {utils::escapeStr(val)}
        {
        }

        std::string val;
    };

    struct _ErrorPropRow final :
        public _StrPropRow
    {
        using _StrPropRow::_StrPropRow;
    };

    struct _NonePropRow final :
        public _PropRow
    {
        _NonePropRow(std::string key) :
            _PropRow {std::move(key)}
        {
        }
    };

private:
    using _Rows = std::vector<std::unique_ptr<_Row>>;

private:
    InspectCmdState *_appState;
    ViewInspectCmdStateObserverGuard _appStateObserverGuard;
    std::unordered_map<const Trace *, _Rows> _traceInfo;
    const _Rows *_rows = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_TRACE_TYPE_INFO_VIEW_HPP
