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

#ifndef MELOSIC_XDG_HPP
#define MELOSIC_XDG_HPP

#include <cstdlib>
#include <wordexp.h>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Melosic {
namespace Directories {

inline namespace XDG {

namespace {
namespace fs = boost::filesystem;
inline fs::path readEnvDir(const char* name, const char* defaultValue) {
    const char* const env = getenv(name);
    wordexp_t exp_result;
    ::wordexp(env ? env : defaultValue, &exp_result, 0);
    fs::path returnValue(exp_result.we_wordv[0]);
    ::wordfree(&exp_result);
    return std::move(returnValue);
}

inline std::list<fs::path> readEnvDirList(const char* name, const char* defaultValue) {
    static constexpr char delim = ':';

    const char* const env = getenv(name);

    std::string str = env ? env : defaultValue;
    // if the environment variable ends with a ':', append the
    // defaultValue
    if(str.back() == delim)
        str += defaultValue;

    std::list<fs::path> returnValue;
    boost::split(returnValue, str, [] (char c) { return c == delim; });

    return std::move(returnValue);
}
}

inline const auto& dataHome() {
    static const auto tmp(readEnvDir("XDG_DATA_HOME", "~/.local/share/"));
    return tmp;
}

inline const auto& configHome() {
    static const auto tmp(readEnvDir("XDG_CONFIG_HOME", "~/.config/"));
    return tmp;
}

inline const auto& dataDirs() {
    static const auto tmp(readEnvDirList("XDG_DATA_DIRS", "/usr/local/share/:/usr/share/"));
    return tmp;
}

inline const auto& configDirs() {
    static const auto tmp(readEnvDirList("XDG_CONFIG_DIRS", "/etc/xdg/"));
    return tmp;
}

inline const auto& cacheHome() {
    static const auto tmp(readEnvDir("XDG_CACHE_HOME", "~/.cache/"));
    return tmp;
}

} // namespace XDG

} // namespace Melosic
} // namespace Melosic

#endif // MELOSIC_XDG_HPP
