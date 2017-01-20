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

#ifndef __SIMPLESTATUS_H__
#define __SIMPLESTATUS_H__

#include "Poly.h"
#include "GenericBuffer.h"
#include "FileManager.h"

class SimpleStatus{
private:
	Mesh chars[36];
	float charw, charh;
	CStr str;
public:
	SimpleStatus();
	bool Init(FileManager *fm, float w, float h);
	int Status(const char *text);
	int Draw(GenericBuffer& glb);
};

#endif // __SIMPLESTATUS_H__
