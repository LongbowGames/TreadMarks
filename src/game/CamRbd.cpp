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

#include "CamRbd.h"

void Camera::SetAll(int X, int Y, int Z, int A, int B, int C, float P, float R, int V){
	SetAll((float)X, (float)Y, (float)Z, (float)A, (float)B, (float)C, P, R, V);
}
void Camera::SetAll(float X, float Y, float Z, float A, float B, float C, float P, float R, int V){
	x = X;
	y = Y;
	z = Z;
	a = A;
	b = B;
	c = C;
	pitch = P;
	roll = R;
	viewplane = V;
}

