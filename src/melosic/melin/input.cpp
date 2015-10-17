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

std::unique_ptr<std::istream> Manager::open(const network::uri& uri) const {
    try {
        if(!uri.scheme())
            return nullptr;
        if(uri.scheme()->to_string() == "file") {
            return std::make_unique<fs::ifstream>(uri_to_path(uri));
        } else if(uri.scheme()->to_string() == "http") {
        }
    } catch(...) {
        ERROR_LOG(logject) << "Could not open uri " << uri << ": " << boost::current_exception_diagnostic_information();
        return nullptr;
    }

    return nullptr;
}

Manager::~Manager() {
}

boost::filesystem::path uri_to_path(const network::uri& uri) {
    boost::filesystem::path p /*{"/"}*/;
    if(uri.host())
        p /= uri.host()->to_string();
    if(uri.path()) {
        auto str_ref = *uri.path();
        str_ref = {str_ref.data() - 1, str_ref.size()};
        auto str = std::string{};
        network::uri::decode(str_ref.begin(), str_ref.end(), std::back_inserter(str));
        p /= std::move(str);
    }
    return p;
}

network::uri to_uri(const boost::filesystem::path& path) {
    network::uri_builder uri{};
    uri.scheme("file");
    uri.authority("");
    uri.path(path);

    return network::uri{uri};
}

} // namespace Input
} // namespace Melosic
