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

#ifndef MELOSIC_TAGLIBFILE_HPP
#define MELOSIC_TAGLIBFILE_HPP

#include <taglib/tiostream.h>
#include <taglib/fileref.h>

namespace Melosic {
namespace IO {

class File;

class TagLibFile : public TagLib::IOStream {
public:
    explicit TagLibFile(File& file) : file(file) {
        seek(0);
    }

    TagLib::FileName name() const;
    TagLib::ByteVector readBlock(ulong length);
    void writeBlock(const TagLib::ByteVector& data);
    void insert(const TagLib::ByteVector &data, ulong start = 0, ulong replace = 0);
    void removeBlock(ulong start = 0, ulong length = 0);
    bool readOnly() const;
    bool isOpen() const;
    void seek(long offset, Position p = Beginning);
    void clear();
    long tell() const;
    long length();
    void truncate(long length);

private:
    File& file;
};

} //end namespace IO
} //end namespace Melosic

#endif // MELOSIC_TAGLIBFILE_HPP
