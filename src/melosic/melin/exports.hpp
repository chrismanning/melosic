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
}

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
inline std::chrono::system_clock::time_point timeWhenCompiled();
}

namespace Plugin {

enum class Type {
    decoder         = 0b0000001,
    encoder         = 0b0000010,
    outputDevice    = 0b0000100,
    inputDevice     = 0b0001000,
    utility         = 0b0010000,
    service         = 0b0100000,
    gui             = 0b1000000
};

constexpr Type operator|(Type a, Type b) noexcept {
    return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(a) |
                             static_cast<std::underlying_type_t<Type>>(b));
}

constexpr bool operator&(Type a, Type b) noexcept {
    return static_cast<bool>(static_cast<std::underlying_type_t<Type>>(a) &
                             static_cast<std::underlying_type_t<Type>>(b));
}

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
    Info(std::experimental::string_view name, Type type, Version vers)
        : name(name.to_string()), type(type), version(vers) {
    }
    std::string name;
    Type type;
    Version version;
    Version APIVersion{MELOSIC_PLUGIN_API_VERSION};
    std::chrono::system_clock::time_point built{timeWhenCompiled()};
};

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& out, const Info& info) {
    char str[40];
    auto time = std::chrono::system_clock::to_time_t(info.built);
    std::strftime(str, sizeof str, "%x %X %Z", std::localtime(&time));
    return out << info.name << " " << info.version << " compiled for Melosic API " << info.APIVersion << " on " << str;
}

} // namespace Plugin

namespace {

inline std::chrono::system_clock::time_point timeWhenCompiled() {
    auto date_time = std::string(__DATE__) + " " + __TIME__;

    std::tm tm;
    strptime(date_time.c_str(), "%b %d %Y %H:%M:%S", &tm);

    std::time_t time = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time);
}

} // namespace {}

} // namespace Melosic

#endif // MELOSIC_EXPORTS_HPP
