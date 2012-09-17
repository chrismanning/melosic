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
#include <boost/log/filters.hpp>
#include <boost/log/utility/init/to_file.hpp>
#include <boost/log/utility/init/to_console.hpp>
#include <boost/log/utility/init/common_attributes.hpp>
#include <boost/log/formatters.hpp>
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace fmt = boost::log::formatters;
namespace flt = boost::log::filters;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

namespace Melosic {

namespace Logger {

enum class Severity {
    info,
    warning,
    error,
    debug,
    trace
};

typedef boost::log::sources::severity_channel_logger_mt<Severity> Logger;

template< typename CharT, typename TraitsT >
inline std::basic_ostream<CharT, TraitsT>& operator<< (
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

inline void init() {
    auto formatter =
            fmt::format("%1% [%2%] <%3%> %4%")
                        % fmt::date_time("TimeStamp", keywords::format = "%d/%m/%y %H:%M:%S", std::nothrow)
                        % fmt::attr<std::string>("Channel", std::nothrow)
                        % fmt::attr<Severity>("Severity", std::nothrow)
                        % fmt::message();

    logging::init_log_to_console(std::clog, keywords::format = formatter);

    logging::add_common_attributes();
}

}//end namespace Logger
}//end namespace Melosic

#define LOG(lg) BOOST_LOG(lg)
#define ERROR_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::error)
#define WARN_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::warning)
#define DEBUG_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::debug)
#define TRACE_LOG(lg) BOOST_LOG_SEV(lg, Melosic::Logger::Severity::trace)
#define CHAN_LOG(lg, chan) BOOST_LOG_CHANNEL(lg, chan)
#define CHAN_ERROR_LOG(lg) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::error)
#define CHAN_WARN_LOG(lg) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::warning)
#define CHAN_DEBUG_LOG(lg) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::debug)
#define CHAN_TRACE_LOG(lg) BOOST_LOG_CHANNEL_SEV(lg, chan, Melosic::Logger::Severity::trace)

#endif // MELOSIC_LOGGING_HPP
