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

#include "tag.hpp"
#include "service.hpp"

#include <opqit/opaque_iterator.hpp>

#include <list>

namespace LastFM {

Tag& Tag::operator=(const std::string& tag) {
    name = tag;
    return *this;
}

boost::iterator_range<opqit::opaque_iterator<Tag, opqit::forward>> Tag::getSimilar(std::shared_ptr<Service> lastserv) {
    static std::list<Tag> similar;
    if(!similar.empty())
        return boost::make_iterator_range(similar);

    return boost::make_iterator_range(similar);
}

}
