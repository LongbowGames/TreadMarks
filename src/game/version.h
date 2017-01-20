// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 7
#define VERSION_PATCH 0
#define VERSION_REVISION 0

#define _VERSION_STRING_STR(s) #s
#define _VERSION_STRING_XSTR(s) _VERSION_STRING_STR(s)
#define VERSION_STRING _VERSION_STRING_XSTR(VERSION_MAJOR) "." _VERSION_STRING_XSTR(VERSION_MINOR) "." _VERSION_STRING_XSTR(VERSION_PATCH) "." _VERSION_STRING_XSTR(VERSION_REVISION)

#endif
