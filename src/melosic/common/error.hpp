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

#ifndef MELOSIC_ERROR_HPP
#define MELOSIC_ERROR_HPP

#include <exception>
#include <utility>
#include <string>

#include <boost/exception/all.hpp>
#include <boost/filesystem/path.hpp>

#define MELOSIC_THROW(a, str, logger) ERROR_LOG(logger) << str; BOOST_THROW_EXCEPTION(a)

namespace Melosic {

template <class Exception, typename ... Args>
void enforceEx(bool expression, Args&& ... arguments) {
    if(!expression) {
        throw Exception(std::forward<Args>(arguments)...);
    }
}

//base exception type
struct Exception : virtual boost::exception, virtual std::exception {};
//IO related exceptions
struct IOException : virtual Exception {};
struct ReadException : virtual IOException {};
struct WriteException : virtual IOException {};
struct ReadOnlyException : virtual WriteException {};
struct SeekException : virtual IOException {};
//file exceptions
struct FileException : virtual IOException {};
struct FileOpenException : virtual FileException {};
struct FileNotFoundException : virtual FileOpenException {};
struct FileSeekException : virtual SeekException, virtual FileException {};
struct FileReadException : virtual ReadException, virtual FileException {};
struct FileWriteException : virtual WriteException, virtual FileException {};
struct FileReadOnlyException : virtual ReadOnlyException, virtual FileException {};
namespace ErrorTag {
typedef boost::error_info<struct tagFilePath, boost::filesystem::path> FilePath;
}
//device exceptions
struct DeviceException : virtual IOException {};
struct DeviceOpenException : virtual DeviceException {};
struct DeviceNotFoundException : virtual DeviceOpenException {};
struct DeviceParamException : virtual DeviceException {};
struct DeviceBusyException : virtual DeviceOpenException {};
struct DeviceReadException : virtual DeviceException, virtual ReadException {};
struct DeviceWriteException : virtual DeviceException, virtual WriteException {};
//decoder exceptions
struct DecoderException : virtual Exception {};
struct DecoderInitException : virtual DecoderException {};
struct UnsupportedTypeException : virtual DecoderException {};
struct UnsupportedFileTypeException : virtual UnsupportedTypeException, virtual FileException {};
struct AudioDataInvalidException : virtual DecoderException {};
struct AudioDataUnsupported : virtual DecoderException {};
//metadata exceptions
struct MetadataException : virtual Exception, virtual FileException {};
struct MetadataNotFoundException : virtual MetadataException {};
struct MetadataUnsupportedException : virtual MetadataException {};
struct MetadataInvalidException : virtual MetadataException {};
//player exceptions

//plugin exceptions
struct PluginException : virtual Exception {};
struct PluginInvalidException : virtual PluginException {};
struct PluginSymbolNotFoundException : virtual PluginException {};
struct PluginVersionMismatchException : virtual PluginException {};
//playlist exceptions

//config exceptions
struct ConfigException : virtual Exception {};
struct KeyNotFoundException : virtual ConfigException {};
struct ChildNotFoundException : virtual ConfigException {};
namespace ErrorTag {
typedef boost::error_info<struct tagConfigKey, std::string> ConfigKey;
typedef boost::error_info<struct tagConfigChild, std::string> ConfigChild;
}

//network exceptions
struct NetworkException : virtual Exception {};
struct HttpException : virtual NetworkException {};
struct ServiceNotAvailableException : virtual NetworkException {};
namespace ErrorTag {
struct _HttpStatus {
    uint16_t status_code;
    std::string status_str;
};
typedef boost::error_info<struct tagHttpStatus, _HttpStatus> HttpStatus;
}

}

#endif // MELOSIC_ERROR_HPP
