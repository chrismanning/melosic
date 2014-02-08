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

#include <mutex>
using mutex = std::mutex;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <unordered_map>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/functional/hash.hpp>
#include <boost/range/iterator_range.hpp>

#include <melosic/core/audiofile.hpp>
#include <melosic/common/optional.hpp>
#include "filecache.hpp"

namespace Melosic {
namespace Core {

struct FileCache::impl {
    struct Equiv : std::binary_function<const fs::path&, const fs::path&, bool> {
        bool operator()(const fs::path& a, const fs::path& b) const {
            return fs::equivalent(a, b);
        }
    };

    std::unordered_map<fs::path, Core::AudioFile, boost::hash<fs::path>, Equiv> fileDB;
    mutex mu;
};

FileCache::FileCache() :
    pimpl(std::make_unique<impl>())
{}

FileCache::~FileCache() {}

optional<Core::AudioFile> FileCache::getFile(const fs::path& p_, boost::system::error_code& ec) const {
    unique_lock l(pimpl->mu);

    fs::path p{fs::canonical(p_, ec)};
    if(!fs::exists(p) || fs::is_other(p) || fs::is_directory(p)) {
        pimpl->fileDB.erase(p);
        return nullopt;
    }

    auto it = pimpl->fileDB.find(p);
    if(it != pimpl->fileDB.end())
        return it->second;
    auto r = pimpl->fileDB.emplace(std::make_pair(p, Core::AudioFile{p}));
    assert(r.second);
    return r.first->second;
}

} // namespace Core
} // namespace Melosic
