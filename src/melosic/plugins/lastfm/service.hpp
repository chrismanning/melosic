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
#include <melosic/common/optional.hpp>

namespace Melosic {

namespace Core {
class Track;
class Playlist;
}

namespace Output {
enum class DeviceState;
}
namespace Slots {
class Manager;
}
namespace Thread {
class Manager;
}
}

namespace LastFM {

struct User;
struct Method;
struct Track;

class Service : public std::enable_shared_from_this<Service> {
public:
    Service(const std::string& apiKey, const std::string& sharedSecret, Melosic::Thread::Manager*&);

    ~Service();

    Melosic::Thread::Manager* getThreadManager();

//    Scrobbler scrobbler();
    User& getUser();
    User& setUser(User u);

    const std::string& apiKey();
    const std::string& sharedSecret();

    std::shared_ptr<Track> currentTrack();
    void trackChangedSlot(const Melosic::Core::Track& newTrack);
    void playlistChangeSlot(std::shared_ptr<Melosic::Core::Playlist> playlist);

    Method prepareMethodCall(const std::string& methodName);
    Method sign(Method method);
    std::string postMethod(const Method& method);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

typedef std::map<std::string, std::string> StringStringMap;
typedef std::pair<std::string, std::string> Member;
struct Parameter {
    Parameter() = default;
    Parameter(Parameter&&) = delete;
    Parameter(const Parameter&) = delete;

    Parameter& addMember(const std::string& key, Melosic::optional<std::string> value = {}) {
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
    Method& operator=(const Method&) = delete;

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
