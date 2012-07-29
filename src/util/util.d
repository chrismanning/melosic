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

module util.util;

import
std.array
,std.format
,std.string
,std.stdio
;
version(Posix) import core.stdc.config;
version(Windows) import core.sys.windows.windows;

struct Dimensions {
    ushort w;
    ushort h;
}

version(Posix) {
    extern(C) {
        struct winsize {
            ushort ws_row;
            ushort ws_col;
            ushort ws_xpixel;
            ushort ws_ypixel;
        };
        int ioctl(int __fd, c_ulong __request, ...);
        enum TIOCGWINSZ = 0x5413;
    }
    auto getTermSize() {
        winsize w;
        ioctl(0, TIOCGWINSZ, &w);
        return Dimensions(w.ws_col, w.ws_row);
    }
}
version(Windows) {
    auto getTermSize() {
        CONSOLE_SCREEN_BUFFER_INFO bi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bi);
        return cast(Dimensions) bi.dwSize;
    }
}

void progressBar(ulong val, ulong total) {
    ushort columns = getTermSize().w;
    auto width = columns - 9;
    auto app = appender!string();
    app.reserve(columns);
    app.put('[');
    auto ratio = val / cast(double)total;
    auto size = cast(uint) (ratio * width);

    foreach(i; 0..size) {
        app.put('=');
    }
    foreach(j; size..width) {
        app.put(' ');
    }
    formattedWrite(app, "] %d%%", cast(uint) (ratio * 100));
    if(app.capacity) {
        auto padding = new char[app.capacity-columns-1];
        padding[] = ' ';
        app.put(padding);
    }

    write(strip(app.data));

    version(Posix) {
        write("\n\33[1A\33[2K");
    }
    version(Windows) {
        write("\r");
        stdout.flush();
    }
}
