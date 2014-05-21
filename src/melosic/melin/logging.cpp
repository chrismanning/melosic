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
#include <boost/utility/empty_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include <melosic/common/directories.hpp>

#include "logging.hpp"

namespace Melosic {
namespace Logger {

void init() {
    auto con_sink = boost::make_shared<sinks::synchronous_sink<sinks::text_ostream_backend>>();

    boost::shared_ptr<std::ostream> stream(&std::clog, boost::empty_deleter());
    con_sink->locked_backend()->add_stream(stream);

    logging::core::get()->add_sink(con_sink);

    auto date_facet = new boost::posix_time::time_facet;

    date_facet->format("%d/%m/%y %H:%M:%S");
    auto loc = std::locale(std::locale(), date_facet);
    auto formatter = [loc](logging::record_view const& rec, logging::formatting_ostream& strm) {
        strm.imbue(loc);
        strm << logging::extract<boost::posix_time::ptime>("TimeStamp", rec) << " ";

        strm << "[" << logging::extract<std::string>("Channel", rec) << "] ";
        strm << "<" << logging::extract_or_default<Severity>("Severity", rec, Severity::info) << "> ";

        // Finally, put the record message to the stream
        strm << rec[expr::smessage];
    };

    con_sink->set_formatter(formatter);

    auto file_backend = boost::make_shared<sinks::text_file_backend>(
                keywords::file_name = Directories::dataHome()/"melosic"/"melosic.log");
    auto file_sink = boost::make_shared<sinks::synchronous_sink<sinks::text_file_backend>>(file_backend);
    file_backend->auto_flush(true);

    logging::core::get()->add_sink(file_sink);
    file_sink->set_formatter(formatter);

    logging::add_common_attributes();
}

} // namespace Logger
} // namespace Melosic
