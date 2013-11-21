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

#include <melosic/common/common.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/optional_fwd.hpp>

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
    friend class Decoder::Manager;
    friend struct AudioFile;
    void setAudioSpecs(AudioSpecs);
    void setTimePoints(chrono::milliseconds end, chrono::milliseconds start = 0ms);
    explicit Track(boost::filesystem::path filename,
                   chrono::milliseconds end = 0ms,
                   chrono::milliseconds start = 0ms);

public:
    ~Track();

    bool operator==(const Track&) const noexcept;
    bool operator!=(const Track&) const noexcept;

    chrono::milliseconds duration() const;
    Melosic::AudioSpecs getAudioSpecs() const;
    const boost::filesystem::path& filePath() const;
    optional<std::string> getTag(const std::string& key) const;
    optional<std::string> format_string(const std::string&) const;

    boost::synchronized_value<TagMap>& getTags();
    const boost::synchronized_value<TagMap>& getTags() const;
    void setTags(TagMap);

    bool tagsReadable() const;

    Signals::Track::TagsChanged& getTagsChangedSignal() const noexcept;

private:
    class impl;
    std::shared_ptr<impl> pimpl;
    friend size_t hash_value(const Track &b);
};

size_t hash_value(const Track& b);

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_TRACK_HPP
