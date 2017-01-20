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

// Stub for headless builds.

#include "EntityTank.h"
#include "TankGUI.h"

CStr LastControl;

MenuPage CurrentMenu = MP_Initialize;
MenuPage ReturnToMenu = MP_Main;
MenuPage LastMenu = MP_None;
MenuPage ReturnFromText = MP_Main;

bool bTrainingOnly = false;

void DoMenus() {}

void CacheMenus() {}

void JumpToMenu(MenuPage menu) {}