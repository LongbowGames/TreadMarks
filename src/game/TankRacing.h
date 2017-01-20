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

//TankRacing include file.

#ifndef TANKRACING_H
#define TANKRACING_H

#include <stdio.h>
#include "CStr.h"
#include "Reg.h"
#include "Vector.h"
#include "GenericBuffer.h"

#ifdef QT_CORE_LIB
#include "Qt/Dedicated.h"
#endif

//Change this whenever the net protocol changes, and old clients/servers shouldn't be able to connect.
#define TREADMARKS_CHALLENGE_KEY	0x7a2b0004

struct TreadMarksVersion{
	unsigned char v[4];
};

extern TreadMarksVersion GameVersion;

extern GenericBuffer GenBuf;

#ifdef QT_CORE_LIB
Dedicated* GetDedicatedWin();
#endif

extern const char szAppName[];

bool RegistrySave();	//Loads/Saves values to registry which are editable in options menus.
bool RegistryLoad();

#endif

