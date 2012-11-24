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

#include "taglibfile.hpp"
#include <melosic/common/file.hpp>

namespace Melosic {
namespace IO {

TagLib::FileName TagLibFile::name() const {
    return file.filename().c_str();
}

TagLib::ByteVector TagLibFile::readBlock(ulong length) {
    TagLib::ByteVector buf(length, 0);
    auto r = file.read(buf.data(), length);
    buf.resize(r);
    return buf;
}

void TagLibFile::writeBlock(const TagLib::ByteVector& data) {
    file.seekp(file.tellg(), std::ios_base::beg);
    file.write(data.data(), data.size());
}

void TagLibFile::insert(const TagLib::ByteVector& data, ulong start, ulong replace) {
    //TODO: insert
}

void TagLibFile::removeBlock(ulong start, ulong length) {
    //TODO: removeBlock
}

bool TagLibFile::readOnly() const {
    return false;
}

bool TagLibFile::isOpen() const {
    return file.isOpen();
}

void TagLibFile::seek(long offset, TagLib::IOStream::Position p) {
    std::ios_base::seekdir d;
    switch(p) {
        case Beginning:
            d = std::ios_base::beg;
            break;
        case Current:
            d = std::ios_base::cur;
            break;
        case End:
            d = std::ios_base::end;
            break;
    }

    file.seekg(offset, d);
}

void TagLibFile::clear() {}

long TagLibFile::tell() const {
    return file.tellg();
}

long TagLibFile::length() {
    auto res = file.tellp();
    static auto size = file.seekp(0, std::ios_base::end);
    file.seekg(res, std::ios_base::beg);

    return size;
}

void TagLibFile::truncate(long length) {
}

} //end namespace IO
} //end namespace Melosic
