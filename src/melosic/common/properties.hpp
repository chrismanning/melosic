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

#ifndef MELOSIC_PROPERTIES_HPP
#define MELOSIC_PROPERTIES_HPP

#define getter(Type, Name, CName) public: Type get##CName() { return Name; }
#define setter(Type, Name, CName) public: void set##CName(Type Name) { this.##Name = Name; }

#define readWriteProperty(Type, Name, CName) \
    private: Type Name; \
    getter(Type, Name, CName) \
    setter(Type, Name, Cname)

#define readProperty(Type, Name, CName) \
    private: Type Name; \
    getter(Type, Name, CName)

#define writeProperty(Type, Name, CName) \
    private: Type Name; \
    setter(Type, Name, CName)

#endif // MELOSIC_PROPERTIES_HPP
