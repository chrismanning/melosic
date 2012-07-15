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

#ifndef INPUT_PLUGINTERFACE_H
#define INPUT_PLUGINTERFACE_H

#include <melosic/managers/common.h>

class IInputDecoder {
public:
    virtual bool writeCallback(unsigned chunkSize, unsigned short channels, const int * buffer[]) = 0;
    virtual bool canOpen(const char * extension) = 0;
    virtual void openFile(const char * filename) = 0;
};

class IKernel;

extern "C" void registerPlugin(IKernel * k);

#endif // INPUT_PLUGINTERFACE_H
