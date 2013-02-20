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

#include <memory>

#include <boost/log/sinks.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "logging.hpp"

template class logging::sources::severity_channel_logger_mt<Melosic::Logger::Severity>;

namespace Melosic {
namespace Logger {
void init() {
    typedef sinks::synchronous_sink<sinks::text_ostream_backend > text_sink;
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

    boost::shared_ptr<std::ostream> stream(&std::clog, logging::empty_deleter());
    sink->locked_backend()->add_stream(stream);

    logging::core::get()->add_sink(sink);
    sink->set_formatter
            (
            expr::format("%1% [%2%] "/*%|32T |*/ "<%3%> %4%")
                        % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d/%m/%y %H:%M:%S")
                        % expr::attr<std::string>("Channel")
                        % expr::attr<Severity>("Severity")
                        % expr::smessage
                );
    logging::add_common_attributes();
}
}
}

namespace std {
std::ostream& operator<<(std::ostream& strm, const std::type_info& t) {
    const auto str = abi::__cxa_demangle(t.name(), nullptr, nullptr, nullptr);
    if(str) {
        strm << str;
        ::free(str);
    }
    return strm;
}
}
