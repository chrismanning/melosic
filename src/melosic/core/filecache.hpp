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

#ifndef MELOSIC_FILECACHE_HPP
#define MELOSIC_FILECACHE_HPP

#include <memory>

#include <boost/filesystem/path.hpp>

#include <melosic/common/optional_fwd.hpp>

namespace Melosic {
namespace Core {

struct AudioFile;

class FileCache {
public:
    FileCache();
    ~FileCache();

    optional<Core::AudioFile> getFile(const boost::filesystem::path&, boost::system::error_code&) const;

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_FILECACHE_HPP
