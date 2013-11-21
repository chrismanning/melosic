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

#include <tcejdb/ejdb.h>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/core/track.hpp>
#include "library.hpp"

namespace Melosic {
namespace Library {

static const fs::path DataDir{Directories::dataHome()/"melosic"};

struct Manager::impl {
    impl() : db(ejdbnew()) {
        if(!fs::exists(DataDir))
            fs::create_directory(DataDir);
        assert(fs::exists(DataDir) && fs::is_directory(DataDir));
        ejdbopen(db.get(), (DataDir/"medialibrary").c_str(), JBOREADER | JBOWRITER | JBOCREAT | JBOTSYNC);
    }

    struct EJDBDeleter {
        void operator()(EJDB* ptr) const {
            ejdbdel(ptr);
        }
    };
    std::unique_ptr<EJDB, EJDBDeleter> db;
};

Manager::Manager(Config::Manager&) : pimpl(new impl) {
}

Manager::~Manager() {}

void Manager::addDirectory(boost::filesystem::path) {

}

} // namespace Library
} // namespace Melosic
