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

#ifndef __VERTEXARRAY_H__
#define __VERTEXARRAY_H__

#include "vecmath.h"

class CVertexArray
{
private:
	int			iCount;
	int			iCurrentIndex;
	C3Vector	*Vertices;
	C2Vector	*UVCoords;
public:
	CVertexArray() {Vertices = NULL; UVCoords = NULL; iCount = 0; iCurrentIndex = -1;}
	~CVertexArray() {Clear();}

	void Init(const int iCount) {Vertices = new C3Vector[iCount]; UVCoords = new C2Vector[iCount];}
	void Clear() {delete []Vertices; delete []UVCoords; iCount = 0; iCurrentIndex = -1;}
	void ResetCurrent(); {iCurrentIndex = -1;}

	int NextVertex() {return CLAMP(iCurrentIndex++, 0, iCount);}

	C3Vector* GetVertex(const int i) {Vertices ? return &(Vertices[CLAMP(i, 0, iCount)]) : return NULL;}
	C2Vector* GetUVCoord(const int i) {UVCoords ? return &(UVCoords[CLAMP(i, 0, iCount)]) : return NULL;}
};

#endif // __VERTEXARRAY_H__
