/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_TRACE_TYPE_INFO_VIEW_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_TRACE_TYPE_INFO_VIEW_HPP

#include <unordered_map>
#include <vector>
#include <memory>
#include <yactfr/metadata/fwd.hpp>

#include "scroll-view.hpp"
#include "../../state/state.hpp"
#include "data/timestamp.hpp"
#include "data/duration.hpp"

namespace jacques {

class TraceInfoView :
    public ScrollView
{
public:
    explicit TraceInfoView(const Rectangle& rect,
                           const Stylist& stylist, State& state);

private:
    void _drawRows() override;
    void _stateChanged(Message msg) override;
    void _buildTraceInfoRows(const Metadata& metadata);
    void _buildRows();

private:
    struct _Row
    {
        virtual ~_Row();
    };

    struct _SectionRow :
        public _Row
    {
        explicit _SectionRow(const std::string& title) :
            title {title}
        {
        }

        std::string title;
    };

    struct _EmptyRow :
        public _Row
    {
    };

    struct _SepRow :
        public _Row
    {
    };

    struct _PropRow :
        public _Row
    {
        _PropRow(const std::string& key) :
            key {key}
        {
        }

        std::string key;
        Index valueOffset = 0;
    };

    struct _SignedIntPropRow :
        public _PropRow
    {
        _SignedIntPropRow(const std::string& key, const long long value) :
            _PropRow {key},
            value {value}
        {
        }

        long long value;
        bool sepNumber = true;
    };

    struct _DataSizePropRow :
        public _PropRow
    {
        _DataSizePropRow(const std::string& key, const DataSize size) :
            _PropRow {key},
            size {size}
        {
        }

        DataSize size;
    };

    struct _TimestampPropRow :
        public _PropRow
    {
        _TimestampPropRow(const std::string& key, const Timestamp& ts) :
            _PropRow {key},
            ts {ts}
        {
        }

        Timestamp ts;
    };

    struct _DurationPropRow :
        public _PropRow
    {
        _DurationPropRow(const std::string& key, const Duration& duration) :
            _PropRow {key},
            duration {duration}
        {
        }

        Duration duration;
    };

    struct _StringPropRow :
        public _PropRow
    {
        _StringPropRow(const std::string& key, const std::string value) :
            _PropRow {key},
            value {utils::escapeString(value)}
        {
        }

        std::string value;
    };

    struct _NonePropRow :
        public _PropRow
    {
        _NonePropRow(const std::string& key) :
            _PropRow {key}
        {
        }
    };

private:
    using Rows = std::vector<std::unique_ptr<_Row>>;

private:
    State * const _state;
    const ViewStateObserverGuard _stateObserverGuard;
    std::unordered_map<const yactfr::TraceType *, Rows> _traceInfo;
    const Rows *_rows = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_TRACE_TYPE_INFO_VIEW_HPP
