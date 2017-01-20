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

#include <math.h>
#include "vecmath.h"

float C2Vector::length(void) const
{
	return (float)sqrtf(val[0]*val[0] + val[1]*val[1]);
}

float C2Vector::length_squared(void) const
{
	return val[0]*val[0] + val[1]*val[1];
}

C2Vector& C2Vector::normalize(void)
{
	float len = length();
	if (len != 0.0f)
		*this /= length();
	return *this;
}

float C2Vector::dot(const C2Vector &_rhs) const
{
	return(val[0]*_rhs.val[0] + val[1]*_rhs.val[1]);
}

/* ----------------------------------------------------------------------------- */

float C3Vector::length(void) const
{
	return (float)sqrtf(val[0]*val[0] + val[1]*val[1] + val[2]*val[2]);
}

float C3Vector::length_squared(void) const
{
	return val[0]*val[0] + val[1]*val[1] + val[2]*val[2];
}

C3Vector& C3Vector::normalize(void)
{
	float len = length();
	if (len != 0.0f)
		*this /= length();
	return *this;
}

float C3Vector::dot(const C3Vector &_rhs) const
{
	return(val[0]*_rhs.val[0] + val[1]*_rhs.val[1] + val[2]*_rhs.val[2]);
}

C3Vector C3Vector::cross(const C3Vector &_rhs) const
{
	return C3Vector(val[1]*_rhs.val[2] - _rhs.val[1]*val[2],
					val[2]*_rhs.val[0] - _rhs.val[2]*val[0],
					val[0]*_rhs.val[1] - _rhs.val[0]*val[1]);
}

/* ----------------------------------------------------------------------------- */

float C4Vector::length(void) const
{
	return (float)sqrtf(val[0]*val[0] + val[1]*val[1] + val[2]*val[2] + val[3]*val[3]);
}

float C4Vector::length_squared(void) const
{
	return val[0]*val[0] + val[1]*val[1] + val[2]*val[2] + val[3]*val[3];
}

C4Vector& C4Vector::normalize(void)
{
	float len = length();
	if (len != 0.0f)
		*this /= length();
	return *this;
}

float C4Vector::dot(const C4Vector &_rhs) const
{
	return(val[0]*_rhs.val[0] + val[1]*_rhs.val[1] + val[2]*_rhs.val[2] + val[3]*_rhs.val[3]);
}
/*
C3Matrix& C3Matrix::orthogonalize(void)
{
//	V1' = V1/|V1|
//	V2'' = V2 - (V2 dot  V1')V1'
//	V2' = V2''/|V2''|
//	V3' =(+/-) V1' cross V2'
	vec[0].normalize();
	vec[1] = (vec[1] - (vec[1].dot(vec[0]) * vec[0])).normalize();
	vec[2] = vec[0].cross(vec[1]);
	return *this;
}
*/

C4Matrix& C4Matrix::transpose(void)
{
	C4Matrix	tmp(*this);

//	vec[0][0] = tmp[0][0];
	vec[0][1] = tmp[1][0];
	vec[0][2] = tmp[2][0];
	vec[0][3] = tmp[3][0];
	vec[1][0] = tmp[0][1];
//	vec[1][1] = tmp[1][1];
	vec[1][2] = tmp[2][1];
	vec[1][3] = tmp[3][1];
	vec[2][0] = tmp[0][2];
	vec[2][1] = tmp[1][2];
//	vec[2][2] = tmp[2][2];
	vec[2][3] = tmp[3][2];
	vec[3][0] = tmp[0][3];
	vec[3][1] = tmp[1][3];
	vec[3][2] = tmp[2][3];
//	vec[3][3] = tmp[3][3];

	return (*this);
}

C4Matrix& C4Matrix::transpose_and_negate(void)
{
	C4Matrix	tmp(*this);

	vec[0][0] = tmp[0][0];
	vec[0][1] = tmp[1][0];
	vec[0][2] = tmp[2][0];
	vec[0][3] = 0.0f;
	vec[1][0] = tmp[0][1];
	vec[1][1] = tmp[1][1];
	vec[1][2] = tmp[2][1];
	vec[1][3] = 0.0f;
	vec[2][0] = tmp[0][2];
	vec[2][1] = tmp[1][2];
	vec[2][2] = tmp[2][2];
	vec[2][3] = 0.0f;
	vec[3][0] = -tmp[3].dot(tmp[0]);
	vec[3][1] = -tmp[3].dot(tmp[1]);
	vec[3][2] = -tmp[3].dot(tmp[2]);
	vec[3][3] = 1.0f;

	return (*this);
}

C4Matrix& C4Matrix::make_rot(const C3Vector &axis, const float angle)
{
	C3Vector ax(axis);
	ax.normalize();

	float temp_angle = angle * DEG_TO_RAD;
	float s = (float) sin ( temp_angle );
	float c = (float) cos ( temp_angle );
	float t = 1.0f - c;

	vec[0][0] = t * ax[0] * ax[0] + c;
	vec[0][1] = t * ax[0] * ax[1] + s * ax[2];
	vec[0][2] = t * ax[0] * ax[2] - s * ax[1];
	vec[0][3] = 0.0f;

	vec[1][0] = t * ax[1] * ax[0] - s * ax[2];
	vec[1][1] = t * ax[1] * ax[1] + c;
	vec[1][2] = t * ax[1] * ax[2] + s * ax[0];
	vec[1][3] = 0.0f;

	vec[2][0] = t * ax[2] * ax[0] + s * ax[1];
	vec[2][1] = t * ax[2] * ax[1] - s * ax[0];
	vec[2][2] = t * ax[2] * ax[2] + c;
	vec[2][3] = 0.0f;

	vec[3][0] = 0.0f;
	vec[3][1] = 0.0f;
	vec[3][2] = 0.0f;
	vec[3][3] = 1.0f;

	return (*this);
}

C4Matrix& C4Matrix::make_trans(const C3Vector &offset)
{
	vec[0][1] = vec[0][2] = vec[0][3] =
	vec[1][0] = vec[1][2] = vec[1][3] =
	vec[2][0] = vec[2][1] = vec[2][3] = 0.0f;
	vec[0][0] = vec[1][1] = vec[2][2] = vec[3][3] = 1.0f;
	vec[3][0] = offset[0];
	vec[3][1] = offset[1];
	vec[3][2] = offset[2];
	return (*this);
}

const C4Matrix C4Matrix::operator*(const C4Matrix _rhs) const
{
	C4Matrix dst;

    for ( int j = 0; j < 4; j++ )
    {
      dst[0][j] = (vec[0][0] * _rhs[0][j] +
                   vec[0][1] * _rhs[1][j] +
                   vec[0][2] * _rhs[2][j] +
                   vec[0][3] * _rhs[3][j]);

      dst[1][j] = (vec[1][0] * _rhs[0][j] +
                   vec[1][1] * _rhs[1][j] +
                   vec[1][2] * _rhs[2][j] +
                   vec[1][3] * _rhs[3][j]);

      dst[2][j] = (vec[2][0] * _rhs[0][j] +
                   vec[2][1] * _rhs[1][j] +
                   vec[2][2] * _rhs[2][j] +
                   vec[2][3] * _rhs[3][j]);

      dst[3][j] = (vec[3][0] * _rhs[0][j] +
                   vec[3][1] * _rhs[1][j] +
                   vec[3][2] * _rhs[2][j] +
                   vec[3][3] * _rhs[3][j]);
    }
	return dst;
}

void CPlane::normalize(void)
{
	float fTmp = 1.0f / norm.length();

	norm *= fTmp;
	dist *= fTmp;
}
