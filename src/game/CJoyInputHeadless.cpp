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

#include "CJoyInput.h"

CJoyInput::CJoyInput() : idJoystick(-1)
{
}

CJoyInput::~CJoyInput()
{
}

CStr getJoystickName(int iId)
{
	return "";
}

bool CJoyInput::InitController ( int iController )
{
	return false;
}

int CJoyInput::UnInitController ( void )
{
	return 0;
}

int CJoyInput::GetFirstControllerID ( void )
{
	return 0;
}

bool CJoyInput::GetDeviceInfo(trDeviceID *pDevInfo, int iID )
{
	return false;
}

int CJoyInput::GetDevIDFromInfo ( trDeviceID *pDevInfo )
{
	return 0;
}

bool CJoyInput::GetControllerName ( int id, CStr* Name )
{
	return false;
}

int CJoyInput::GetNumberOfControllers( void )
{
	return 0;
}

void CJoyInput::Update(void)
{
}
