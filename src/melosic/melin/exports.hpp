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
namespace Thread {
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

extern "C" MELOSIC_EXPORT void registerTasks(Melosic::Thread::Manager*);
typedef decltype(registerTasks) registerTasks_F;
typedef std::function<registerTasks_F> registerTasks_T;

extern "C" MELOSIC_EXPORT void destroyPlugin();
typedef decltype(destroyPlugin) destroyPlugin_F;
typedef std::function<destroyPlugin_F> destroyPlugin_T;

#define MELOSIC_PLUGIN_API_VERSION 1,0,0

namespace {
constexpr long timeWhenCompiled();
}

namespace Melosic {
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
    Version() : vers(0) {}
    constexpr Version(uint8_t major, uint8_t minor, uint8_t patch) : vers((major << 16) | (minor << 8) | patch) {}
    constexpr Version(uint32_t vers) : vers(vers) {}
    constexpr bool operator==(const Version& b) const { return vers == b.vers; }
    constexpr bool operator!=(const Version& b) const { return vers != b.vers; }
private:
    uint32_t vers;
    template <typename CharT, typename TraitsT>
    friend std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>&, const Version&);
};

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& out, const Version& v) {
    return out << ((v.vers >> 16)&0xFF) << "." << ((v.vers >> 8)&0xFF) << "." << (v.vers&0xFF);
}

extern constexpr Version expectedAPIVersion() {
    return Version(MELOSIC_PLUGIN_API_VERSION);
}

struct Info {
    Info() = default;
    constexpr Info(const char* name, uint32_t type, Version vers)
        : name(name),
          type(type),
          version(vers),
          APIVersion(MELOSIC_PLUGIN_API_VERSION),
          built(timeWhenCompiled())
    {}
    const char* name;
    uint32_t type;
    Version version;
    Version APIVersion;
    time_t built;
};

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& out, const Info& info) {
    char str[40];
    std::strftime(str, sizeof str, "%x %X %Z", std::localtime(&info.built));
    return out << info.name
               << " " << info.version
               << " compiled for Melosic API " << info.APIVersion
               << " on " << str;
}

} // namespace Plugin

namespace Core {
class Kernel;
}

struct MELOSIC_EXPORT RegisterFuncsInserter {
    RegisterFuncsInserter(Core::Kernel& k, std::list<std::function<void()>>& l) : k(k), l(l) {}

    RegisterFuncsInserter& operator<<(const registerInput_T&);
    RegisterFuncsInserter& operator<<(const registerDecoder_T&);
    RegisterFuncsInserter& operator<<(const registerOutput_T&);
    RegisterFuncsInserter& operator<<(const registerEncoder_T&);
    RegisterFuncsInserter& operator<<(const registerConfig_T&);
    RegisterFuncsInserter& operator<<(const registerTasks_T&);
private:
    Core::Kernel& k;
    std::list<std::function<void()>>& l;
};

} // namespace Melosic

namespace {

constexpr bool startsWith(const char* a, const char* b) {
    return b[0] == 0 ? true : a[0] == 0 ? false : a[0] == b[0] && startsWith(a + 1, b + 1);
}

constexpr uint8_t clock_(const char* h) {
    return (h[0] - '0') * 10 + h[1] - '0';
}

constexpr uint8_t day_(const char* d) {
    return (d[0] == ' ' ? 0 : d[0] - '0') * 10 + d[1] - '0';
}

constexpr uint8_t month_(const char* m) {
    return startsWith(m, "Jan") ? 0 :
           startsWith(m, "Feb") ? 1 :
           startsWith(m, "Mar") ? 2 :
           startsWith(m, "Apr") ? 3 :
           startsWith(m, "May") ? 4 :
           startsWith(m, "Jun") ? 5 :
           startsWith(m, "Jul") ? 6 :
           startsWith(m, "Aug") ? 7 :
           startsWith(m, "Sep") ? 8 :
           startsWith(m, "Oct") ? 9 :
           startsWith(m, "Nov") ? 10 :
           startsWith(m, "Dec") ? 11 : 20;
}

constexpr uint16_t year_(const char* y) {
    return (y[0] - '0') * 1000 +  (y[1] - '0') * 100 + (y[2] - '0') * 10 + y[3] - '0';
}

constexpr bool isLeapYear(uint16_t y) {
    return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

static constexpr uint8_t ndays[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

constexpr uint16_t cyears(uint16_t y, uint16_t i) {
    return i >= y ? 0 : (isLeapYear(i) ? 366 : 365) + cyears(y, i+1);
}

constexpr uint8_t cmonths(uint8_t m, uint8_t i) {
    return i >= m ? 0 : ndays[i] + cmonths(m, i+1);
}

constexpr long totime_t(uint8_t secs, uint8_t mins, uint8_t hrs, uint8_t day, uint8_t mon, uint16_t yr) {
    return (((cyears(yr, 1970)
              + cmonths(mon, 0)
              + (mon > 1 && isLeapYear(yr) ? 1 : 0)
              + day - 1) * 24 + hrs) * 60 + mins) * 60 + secs;
}

constexpr long timeWhenCompiled() {
    return ::totime_t(clock_(__TIME__+6),
                      clock_(__TIME__+3),
                      clock_(__TIME__),
                      ::day_(__DATE__+4),
                      ::month_(__DATE__),
                      ::year_(__DATE__+7));
}

}// namespace {}

#endif // MELOSIC_EXPORTS_HPP
