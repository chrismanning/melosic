/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#include <lastfm/image.hpp>

namespace lastfm {

const network::uri& image::uri() const {
    return m_uri;
}

void image::uri(network::uri uri) {
    m_uri = std::move(uri);
}

image_size image::size() const {
    return m_size;
}

void image::size(image_size size) {
    m_size = size;
}

} // namespace lastfm
