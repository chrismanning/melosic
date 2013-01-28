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

#ifndef LASTFM_SERVICE_HPP
#define LASTFM_SERVICE_HPP

#include <string>
#include <memory>
#include <list>
#include <map>

#include <boost/range/iterator_range.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace Melosic {
class Track;
class Playlist;
namespace Output {
enum class DeviceState;
}
}

namespace LastFM {

struct Method;
struct Track;
typedef std::map<std::string, std::string> StringStringMap;

class Service : public std::enable_shared_from_this<Service> {
public:
    Service(const std::string& apiKey, const std::string& sharedSecret);

    ~Service();

//    Scrobbler scrobbler();
//    User user();

    const std::string& apiKey();
    const std::string& sharedSecret();
    const std::string& signature();

    std::shared_ptr<Track> currentTrack();
    void trackChangedSlot(const Melosic::Track& newTrack, bool alive);
    void playlistChangeSlot(std::shared_ptr<Melosic::Playlist> playlist);

    Method prepareMethodCall(const std::string& methodName);
    std::string postMethod(const Method& method);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

struct Parameter {
    Parameter() = default;
    Parameter(Parameter&&) = delete;
    Parameter(const Parameter&) = delete;

    Parameter& addMember(const std::string& key, boost::optional<std::string> value = boost::optional<std::string>()) {
        members.emplace(key, value ? *value : "");
        return *this;
    }

    boost::iterator_range<StringStringMap::iterator> getMembers() {
        return boost::make_iterator_range(members);
    }
    boost::iterator_range<StringStringMap::const_iterator> getMembers() const {
        return boost::make_iterator_range(members);
    }

private:
    StringStringMap members;
};

typedef std::list<Parameter> ParameterList;

struct Method {
    Method(const std::string& methodName) : methodName(methodName) {}
    Method(Method&&) = default;
    Method(const Method&) = delete;

    Parameter& addParameter() {
        params.emplace_back();
        return params.back();
    }

    boost::iterator_range<ParameterList::iterator> getParameters() {
        return boost::make_iterator_range(params);
    }
    boost::iterator_range<ParameterList::const_iterator> getParameters() const {
        return boost::make_iterator_range(params);
    }

    const std::string& renderXML();

private:
    friend class Service;
    const std::string methodName;
    ParameterList params;
};

}

#endif // LASTFM_SERVICE_HPP
