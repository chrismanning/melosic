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

#ifdef WIN32
#ifdef MELOSIC_PLUGIN_EXPORTS
#define MELOSIC_EXPORT __declspec(dllexport)
#else
#define MELOSIC_EXPORT __declspec(dllimport)
#endif
#else
#define MELOSIC_EXPORT
#endif

#include <functional>
#include <ctime>

#define MELOSIC_PLUGIN_API_VERSION 1,0,0

namespace Melosic {
namespace Plugin {

constexpr uint32_t generateVersion(uint8_t major, uint8_t mid, uint8_t minor) {
    return (major << 16) | (mid << 8) | minor;
}

enum class Type {
    decode,
    encode,
    outputDevice,
    inputDevice,
    utility,
    service
};

struct Version {
    Version() : vers(0) {}
    constexpr Version(uint32_t vers) : vers(vers) {}
    bool operator==(const Version& b) const { return vers == b.vers; }
    bool operator!=(const Version& b) const { return vers != b.vers; }
    uint32_t vers;
};
inline std::ostream& operator<<(std::ostream& out, const Version& v) {
    return out << ((v.vers >> 16)&0xFF) << "." << ((v.vers >> 8)&0xFF) << "." << (v.vers&0xFF);
}

extern constexpr Version expectedAPIVersion() {
    return Version(generateVersion(MELOSIC_PLUGIN_API_VERSION));
}

struct Info {
    Info() = default;
    std::string name;
    Type type;
    Version version;
    Version APIVersion;
    std::time_t built;
};
inline std::ostream& operator<<(std::ostream& out, const Info& info) {
    return out << info.name
               << " " << info.version
               << " compiled for Melosic API " << info.APIVersion
               << " on " << std::asctime(std::localtime(&info.built));;
}

} // end namespace Plugin
} // end namespace Melosic

extern "C" void registerPlugin(Melosic::Plugin::Info*);
typedef decltype(registerPlugin) registerPlugin_F;
typedef std::function<registerPlugin_F> registerPlugin_T;
extern "C" void destroyPlugin();
typedef decltype(destroyPlugin) destroyPlugin_F;
typedef std::function<destroyPlugin_F> destroyPlugin_T;

extern inline time_t time_when_compiled()
{
    std::string datestr = __DATE__;
    std::string timestr = __TIME__;

    std::istringstream iss_date( datestr );
    std::string str_month;
    int day;
    int year;
    iss_date >> str_month >> day >> year;

    int month;
    if     (str_month == "Jan") month = 1;
    else if(str_month == "Feb") month = 2;
    else if(str_month == "Mar") month = 3;
    else if(str_month == "Apr") month = 4;
    else if(str_month == "May") month = 5;
    else if(str_month == "Jun") month = 6;
    else if(str_month == "Jul") month = 7;
    else if(str_month == "Aug") month = 8;
    else if(str_month == "Sep") month = 9;
    else if(str_month == "Oct") month = 10;
    else if(str_month == "Nov") month = 11;
    else if(str_month == "Dec") month = 12;
    else exit(-1);

    for(std::string::size_type pos = timestr.find(':'); pos != std::string::npos; pos = timestr.find(':', pos))
        timestr[ pos ] = ' ';
    std::istringstream iss_time(timestr);
    int hour, min, sec;
    iss_time >> hour >> min >> sec;

    tm t = {0,0,0,0,0,0,0,0,0,0,0};
    t.tm_mon = month-1;
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    return mktime(&t);
}

#endif // MELOSIC_EXPORTS_HPP
