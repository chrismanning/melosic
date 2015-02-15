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

#include "wiki.hpp"

namespace lastfm {

wiki::wiki(std::string_view summary, std::string_view content, date_t published)
    : m_summary(summary), m_content(content.to_string()), m_published(published) {}

std::string_view wiki::summary() const { return m_summary; }

void wiki::summary(std::string_view summary) { m_summary = summary.to_string(); }

std::string_view wiki::content() const { return m_content; }

void wiki::content(std::string_view content) { m_content = content.to_string(); }

date_t wiki::published() const { return m_published; }

void wiki::published(date_t published) { m_published = published; }

} // namespace lastfm
