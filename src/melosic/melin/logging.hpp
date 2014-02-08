/**************************************************************************
**  Copyright (C) 2012 Christian Manning
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef MELOSIC_LOGGING_HPP
#define MELOSIC_LOGGING_HPP

#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
namespace logging = boost::log;
namespace expr = logging::expressions;
namespace sinks = logging::sinks;
namespace src = logging::sources;
namespace attrs = logging::attributes;
namespace keywords = logging::keywords;

#include <melosic/common/common.hpp>
#include <melosic/common/typeid.hpp>

namespace Melosic {

namespace Logger {

enum class Severity {
    info,
    warning,
    error,
    debug,
    trace
};

typedef logging::sources::severity_channel_logger_mt<Severity> Logger;

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<< (
    std::basic_ostream<CharT, TraitsT>& strm, Severity lvl)
{
    switch(lvl) {
        case Severity::info:
            strm << "info";
            break;
        case Severity::warning:
            strm << "warning";
            break;
        case Severity::error:
            strm << "error";
            break;
        case Severity::debug:
            strm << "debug";
            break;
        case Severity::trace:
            strm << "trace";
            break;
    }
    return strm;
}
extern template std::ostream& operator<<(std::ostream&, Severity);

MELOSIC_EXPORT void init();

struct nullstream : std::ostream {
    nullstream() : std::ios(0), std::ostream(0) {}
};

}//end namespace Logger
}//end namespace Melosic

extern template class logging::sources::severity_channel_logger_mt<Melosic::Logger::Severity>;

#ifndef MELOSIC_DISABLE_LOGGING
#define LOG(lg) BOOST_LOG(lg)
#define ERROR_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::error)
#define WARN_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::warning)
#define DEBUG_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::debug)
#define TRACE_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::trace)
#define CHAN_LOG(lg, chan) BOOST_LOG_CHANNEL(lg, chan)
#define CHAN_ERROR_LOG(lg, chan) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::error)
#define CHAN_WARN_LOG(lg, chan) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::warning)
#define CHAN_DEBUG_LOG(lg, chan) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::debug)
#define CHAN_TRACE_LOG(lg, chan) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::trace)
#else
#define LOG(lg) Melosic::Logger::nullstream()
#define ERROR_LOG(lg) Melosic::Logger::nullstream()
#define WARN_LOG(lg) Melosic::Logger::nullstream()
#define DEBUG_LOG(lg) Melosic::Logger::nullstream()
#define TRACE_LOG(lg) Melosic::Logger::nullstream()
#define CHAN_LOG(lg, chan) Melosic::Logger::nullstream()
#define CHAN_ERROR_LOG(lg) Melosic::Logger::nullstream()
#define CHAN_WARN_LOG(lg) Melosic::Logger::nullstream()
#define CHAN_DEBUG_LOG(lg) Melosic::Logger::nullstream()
#define CHAN_TRACE_LOG(lg) Melosic::Logger::nullstream()
#endif //MELOSIC_DISABLE_LOGGING

#endif // MELOSIC_LOGGING_HPP
