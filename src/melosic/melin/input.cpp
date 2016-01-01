/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;
#include <boost/algorithm/string/replace.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <melosic/melin/logging.hpp>

#include "input.hpp"

namespace Melosic {
namespace Input {

Logger::Logger logject{logging::keywords::channel = "Input::Manager"};

class Manager::impl {};

Manager::Manager()
//    : pimpl(new impl)
{
}

std::unique_ptr<std::istream> Manager::open(const web::uri& uri) const {
    try {
        if(uri.scheme() == "file") {
            return std::make_unique<fs::ifstream>(uri_to_path(uri));
        } else if(uri.scheme() == "http") {
        }
    } catch(...) {
        ERROR_LOG(logject) << "Could not open uri " << uri.to_string() << ": "
                           << boost::current_exception_diagnostic_information();
        return nullptr;
    }

    return nullptr;
}

Manager::~Manager() {
}

boost::filesystem::path uri_to_path(const web::uri& uri) {
    return boost::filesystem::path{} / web::uri::decode(uri.host()) / web::uri::decode(uri.path());
}

web::uri to_uri(const boost::filesystem::path& path) {
    web::uri_builder uri_builder{};
    uri_builder.set_scheme("file");
    uri_builder.set_fragment("");
    uri_builder.set_path(path.string(), true);

    return uri_builder.to_uri();
}

} // namespace Input
} // namespace Melosic
