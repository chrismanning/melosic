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

#ifndef MELOSIC_TRACK_HPP
#define MELOSIC_TRACK_HPP

#include <memory>
#include <map>

#include <boost/filesystem/path.hpp>
#include <boost/locale/collator.hpp>
#include <boost/thread/synchronized_value.hpp>

#include <network/uri.hpp>

#include <jbson/document_fwd.hpp>

#include <melosic/common/common.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/optional.hpp>

namespace Melosic {

struct AudioSpecs;

namespace Core {
using CaseInsensitiveComparator = boost::locale::comparator<char, boost::locale::collator_base::primary>;
using TagMap = std::multimap<std::string, std::string, CaseInsensitiveComparator>;
}

namespace Signals {
namespace Track {

using TagsChanged = SignalCore<void(const boost::synchronized_value<Core::TagMap>&)>;
using DurationChanged = SignalCore<void(std::chrono::milliseconds)>;
using AudioSpecsChanged = SignalCore<void(AudioSpecs)>;

}// Track
}// Signals

namespace Decoder {
class Manager;
}

namespace Core {

class MELOSIC_EXPORT Track final {
private:
    friend class Decoder::Manager;
    void audioSpecs(AudioSpecs);
    void start(chrono::milliseconds start);
    void end(chrono::milliseconds end);

    explicit Track(const network::uri& location,
                   chrono::milliseconds end = 0ms,
                   chrono::milliseconds start = 0ms);

public:
    explicit Track(const jbson::document&);
    ~Track();

    bool operator==(const Track&) const noexcept;
    bool operator!=(const Track&) const noexcept;

    chrono::milliseconds start() const;
    chrono::milliseconds end() const;
    chrono::milliseconds duration() const;
    Melosic::AudioSpecs audioSpecs() const;

    const network::uri& uri() const;
    optional<std::string> tag(const std::string& key) const;
    optional<std::string> format_string(const std::string&) const;

    boost::synchronized_value<TagMap>& tags();
    const boost::synchronized_value<TagMap>& tags() const;
    void tags(TagMap);

    bool tagsReadable() const;

    Signals::Track::TagsChanged& getTagsChangedSignal() const noexcept;
    jbson::document bson() const;

private:
    class impl;
    std::shared_ptr<impl> pimpl;
    friend size_t hash_value(const Track &b);
};

bool operator<(const Track&, const Track&);

size_t hash_value(const Track& b);

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_TRACK_HPP
