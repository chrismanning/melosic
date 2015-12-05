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

#ifndef MELOSIC_EXPORTS_HPP
#define MELOSIC_EXPORTS_HPP

#include <functional>
#include <ctime>
#include <cstdint>
#include <ostream>
#include <memory>
#include <list>

#include <date.h>

#include <melosic/common/common.hpp>

namespace Melosic {
namespace Plugin {
struct Info;
}
namespace Input {
class Manager;
}
namespace Decoder {
class Manager;
}
namespace Output {
class Manager;
}
namespace Encoder {
class Manager;
}
namespace Config {
class Manager;
}
namespace Slots {
class Manager;
}
struct RegisterFuncsInserter;
}

extern "C" MELOSIC_EXPORT void registerPlugin(Melosic::Plugin::Info*, Melosic::RegisterFuncsInserter);
typedef decltype(registerPlugin) registerPlugin_F;
typedef std::function<registerPlugin_F> registerPlugin_T;

extern "C" MELOSIC_EXPORT void registerInput(Melosic::Input::Manager*);
typedef decltype(registerInput) registerInput_F;
typedef std::function<registerInput_F> registerInput_T;

extern "C" MELOSIC_EXPORT void registerDecoder(Melosic::Decoder::Manager*);
typedef decltype(registerDecoder) registerDecoder_F;
typedef std::function<registerDecoder_F> registerDecoder_T;

extern "C" MELOSIC_EXPORT void registerOutput(Melosic::Output::Manager*);
typedef decltype(registerOutput) registerOutput_F;
typedef std::function<registerOutput_F> registerOutput_T;

extern "C" MELOSIC_EXPORT void registerEncoder(Melosic::Encoder::Manager*);
typedef decltype(registerEncoder) registerEncoder_F;
typedef std::function<registerEncoder_F> registerEncoder_T;

extern "C" MELOSIC_EXPORT void registerConfig(Melosic::Config::Manager*);
typedef decltype(registerConfig) registerConfig_F;
typedef std::function<registerConfig_F> registerConfig_T;

extern "C" MELOSIC_EXPORT void destroyPlugin();
typedef decltype(destroyPlugin) destroyPlugin_F;
typedef std::function<destroyPlugin_F> destroyPlugin_T;

#define MELOSIC_PLUGIN_API_VERSION 1, 0, 0

namespace Melosic {

namespace {
constexpr std::chrono::system_clock::time_point timeWhenCompiled();
}

namespace Plugin {

enum Type {
    decode = 0x1,
    encode = 0x2,
    outputDevice = 0x4,
    inputDevice = 0x8,
    utility = 0x10,
    service = 0x20,
    gui = 0x40
};

struct Version {
    Version() : vers(0) {
    }
    constexpr Version(uint8_t major, uint8_t minor, uint8_t patch) : vers((major << 16) | (minor << 8) | patch) {
    }
    constexpr Version(uint32_t vers) : vers(vers) {
    }
    constexpr bool operator==(const Version& b) const {
        return vers == b.vers;
    }
    constexpr bool operator!=(const Version& b) const {
        return vers != b.vers;
    }

  private:
    uint32_t vers;
    template <typename CharT, typename TraitsT>
    friend std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>&, const Version&);
};

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& out, const Version& v) {
    return out << ((v.vers >> 16) & 0xFF) << "." << ((v.vers >> 8) & 0xFF) << "." << (v.vers & 0xFF);
}

extern constexpr Version expectedAPIVersion() {
    return Version(MELOSIC_PLUGIN_API_VERSION);
}

struct Info {
    Info() = default;
    constexpr Info(std::experimental::string_view name, Type type, Version vers)
        : name(name), type(type), version(vers), APIVersion(MELOSIC_PLUGIN_API_VERSION), built(timeWhenCompiled()) {
    }
    std::experimental::string_view name;
    uint32_t type;
    Version version;
    Version APIVersion;
    std::chrono::system_clock::time_point built;
};

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& out, const Info& info) {
    char str[40];
    auto time = std::chrono::system_clock::to_time_t(info.built);
    std::strftime(str, sizeof str, "%x %X %Z", std::localtime(&time));
    return out << info.name << " " << info.version << " compiled for Melosic API " << info.APIVersion << " on " << str;
}

} // namespace Plugin

namespace Core {
class Kernel;
}

struct MELOSIC_EXPORT RegisterFuncsInserter {
    RegisterFuncsInserter(Core::Kernel& k, std::list<std::function<void()>>& l) : k(k), l(l) {
    }

    RegisterFuncsInserter& operator<<(const registerInput_T&);
    RegisterFuncsInserter& operator<<(const registerDecoder_T&);
    RegisterFuncsInserter& operator<<(const registerOutput_T&);
    RegisterFuncsInserter& operator<<(const registerEncoder_T&);
    RegisterFuncsInserter& operator<<(const registerConfig_T&);

  private:
    Core::Kernel& k;
    std::list<std::function<void()>>& l;
};

inline namespace literals {

constexpr std::string_view operator""_sv(const char* str, size_t len) {
    return {str, len};
}

} // inline namespace literals

namespace {

constexpr bool str_equals(std::experimental::string_view a, std::experimental::string_view b) {
    return a.size() == b.size() && a.empty() ? true : a[0] == b[0] && str_equals(a.substr(1), b.substr(1));
}

constexpr date::day parse_day(std::experimental::string_view d) {
    return date::day{static_cast<unsigned>((d[0] == ' ' ? 0 : d[0] - '0') * 10 + d[1] - '0')};
}

constexpr date::month parse_month(std::experimental::string_view m) {
    auto prefix = m.substr(0, 3);

    if(str_equals(prefix, "Jan"_sv))
        return date::jan;
    if(str_equals(prefix, "Feb"_sv))
        return date::feb;
    if(str_equals(prefix, "Mar"_sv))
        return date::mar;
    if(str_equals(prefix, "Apr"_sv))
        return date::apr;
    if(str_equals(prefix, "May"_sv))
        return date::may;
    if(str_equals(prefix, "Jun"_sv))
        return date::jun;
    if(str_equals(prefix, "Jul"_sv))
        return date::jul;
    if(str_equals(prefix, "Aug"_sv))
        return date::aug;
    if(str_equals(prefix, "Sep"_sv))
        return date::sep;
    if(str_equals(prefix, "Oct"_sv))
        return date::jan;
    if(str_equals(prefix, "Nov"_sv))
        return date::nov;
    if(str_equals(prefix, "Dec"_sv))
        return date::dec;
    return date::month(0);
}

constexpr date::year parse_year(std::experimental::string_view y) {
    return date::year{(y[0] - '0') * 1000 + (y[1] - '0') * 100 + (y[2] - '0') * 10 + y[3] - '0'};
}

template <typename DurationT>
constexpr DurationT parse_time_value(std::experimental::string_view time) {
    return DurationT{(time[0] - '0') * 10 + time[1] - '0'};
}

constexpr std::chrono::system_clock::time_point timeWhenCompiled() {
    auto date_str = std::experimental::string_view{__DATE__, sizeof(__DATE__)};
    auto time_str = std::experimental::string_view{__TIME__, sizeof(__TIME__)};
    date::year_month_day ymd = {parse_year(date_str.substr(7)), parse_month(date_str), parse_day(date_str.substr(4))};

    auto time_point = std::chrono::time_point_cast<std::chrono::seconds>(static_cast<date::day_point>(ymd));

    return time_point + parse_time_value<std::chrono::hours>(time_str)
            + parse_time_value<std::chrono::minutes>(time_str.substr(3))
            + parse_time_value<std::chrono::seconds>(time_str.substr(6));
}

} // namespace {}

} // namespace Melosic

#endif // MELOSIC_EXPORTS_HPP
