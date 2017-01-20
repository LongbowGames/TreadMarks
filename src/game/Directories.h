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

#ifndef __DIRECTORIES_H__
#define __DIRECTORIES_H__

#include "CStr.h"

// These are a couple functions I've added to find the AppData and CommonAppData
// directories on your Windows install.  You'll find different versions of these
// functions peppered throughout our products from when I Vista-proofed them.
// -Rick-

CStr GetAppDataDir();
CStr GetCommonAppDataDir();

#endif
