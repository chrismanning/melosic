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

#include "artist.hpp"
#include "service.hpp"
#include "tag.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace LastFM {

Artist& Artist::operator=(const Artist& artist) {
    name = artist.name;
    url = artist.url;
    return *this;
}

Artist& Artist::operator=(const std::string& artist) {
    name = artist;
    return *this;
}

void Artist::getInfo(std::shared_ptr<Service> lastserv, bool autocorrect) {
    Method method = lastserv->prepareMethodCall("artist.getInfo");
    Parameter& p = method.addParameter()
            .addMember("artist", name)
            .addMember("api_key", lastserv->apiKey());
    if(autocorrect)
        p.addMember("autocorrect[1]");
    std::string reply = lastserv->postMethod(method);
    if(reply.empty())
        return;

    boost::property_tree::ptree ptree;
    std::stringstream ss(reply);
    boost::property_tree::xml_parser::read_xml(ss, ptree, boost::property_tree::xml_parser::trim_whitespace);

    if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
        //TODO: handle error
        return;
    }

    ptree = ptree.get_child("lfm.artist");
    if(autocorrect) {
        name = ptree.get<std::string>("name", name);
    }
    url = network::uri(ptree.get<std::string>("url", ""));
    tags.clear();
    for(const boost::property_tree::ptree::value_type& val : ptree.get_child("tags")) {
        tags.emplace_back(val.second.get<std::string>("name"), val.second.get<std::string>("url"));
    }
    biographySummary = ptree.get<std::string>("bio.summary", "");
    biography = ptree.get<std::string>("bio.content", "");
}

}//namespace LastFM
