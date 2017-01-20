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

#ifndef __VECMATH_H__
#define __VECMATH_H__

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI  3.1415926535f
#endif

#define DEG_TO_RAD  0.017453292519f
#define RAD_TO_DEG  57.29577951472f

// ---------------------------------------------------------------------------------------------
// Two component vector
// ---------------------------------------------------------------------------------------------
class C2Vector
{
private:
	float val[2];
public:
	// Constructors
	C2Vector(void) {}
	C2Vector(const float x, const float y) { val[0] = x;
											 val[1] = y;}
	// Accessors
	float& x(void) { return val[0]; }
	float& y(void) { return val[1]; }
	operator float*() const {return (float*)val;}
	// Operators
	C2Vector& operator=(const C2Vector &_rhs) { val[0] = _rhs.val[0];
												val[1] = _rhs.val[1];
												return *this; }
	const C2Vector operator+(const C2Vector &_rhs) const { return C2Vector(val[0] + _rhs.val[0],
															   val[1] + _rhs.val[1]); }
	const C2Vector operator-(const C2Vector &_rhs) const { return C2Vector(val[0] - _rhs.val[0],
															   val[1] - _rhs.val[1]); }
	const C2Vector operator*(const float _rhs) const { return C2Vector(val[0] * _rhs,
														   val[1] * _rhs); }
	const C2Vector operator/(const float _rhs) const { return C2Vector(val[0] / _rhs,
														   val[1] / _rhs); }
	C2Vector& operator+=(const C2Vector &_rhs) { val[0] += _rhs.val[0];
												 val[1] += _rhs.val[1];
												 return *this; }
	C2Vector& operator-=(const C2Vector &_rhs) { val[0] -= _rhs.val[0];
												 val[1] -= _rhs.val[1];
												 return *this; }
	const C2Vector& operator*=(const float _rhs) { val[0] *= _rhs;
											 val[1] *= _rhs;
											 return *this; }
	const C2Vector& operator/=(const float _rhs) { val[0] /= _rhs;
											 val[1] /= _rhs;
											 return *this; }
	const friend C2Vector operator*(const float _lhs, const C2Vector &_rhs) { return C2Vector(_rhs.val[0] * _lhs,
																						_rhs.val[1] * _lhs); }
	const friend C2Vector operator/(const float _lhs, const C2Vector &_rhs) { return C2Vector(_rhs.val[0] / _lhs,
																						_rhs.val[1] / _lhs); }
	// Functions
	void zero(void) { val[0] = 0.0f; val[1] = 0.0f; }
	float length(void) const;
	float length_squared(void) const;
	C2Vector& normalize(void);
	float dot(const C2Vector &_rhs) const;
	void set(float x, float y) { val[0] = x; val[1] = y; }
	bool write(FILE* fp);
};

// ---------------------------------------------------------------------------------------------
// Three component vector
// ---------------------------------------------------------------------------------------------
class C3Vector
{
private:
	float val[3];
public:
	// Constructors
	C3Vector(void) {}
	C3Vector(const float x, const float y, const float z) { val[0] = x;
															val[1] = y;
															val[2] = z; }
	// Accessors
	float& x(void) { return val[0]; }
	float& y(void) { return val[1]; }
	float& z(void) { return val[2]; }
	operator float*() const {return (float*)val;}
	// Operators
	C3Vector& operator=(const C3Vector &_rhs) { val[0] = _rhs.val[0];
												val[1] = _rhs.val[1];
												val[2] = _rhs.val[2];
												return *this; }
	const C3Vector operator+(const C3Vector &_rhs) const { C3Vector temp(*this);
											   temp += _rhs; 
											   return temp; }
	const C3Vector operator-(const C3Vector &_rhs) const { C3Vector temp(*this);
											   temp -= _rhs;
											   return temp; }
	const C3Vector operator*(const float _rhs) const { C3Vector temp(*this);
										   temp *= _rhs;
										   return temp; }
	const C3Vector operator/(const float _rhs) const { C3Vector temp(*this);
										   temp *= 1/_rhs;
										   return temp; }
	C3Vector& operator+=(const C3Vector &_rhs) { val[0] += _rhs.val[0];
												 val[1] += _rhs.val[1];
												 val[2] += _rhs.val[2];
												 return *this; }
	C3Vector& operator-=(const C3Vector &_rhs) { val[0] -= _rhs.val[0];
												 val[1] -= _rhs.val[1];
												 val[2] -= _rhs.val[2];
												 return *this; }
	C3Vector& operator*=(const float _rhs) { val[0] *= _rhs;
											 val[1] *= _rhs;
											 val[2] *= _rhs;
											 return *this; }
	C3Vector& operator/=(const float _rhs) { val[0] /= _rhs;
											 val[1] /= _rhs;
											 val[2] /= _rhs;
											 return *this; }
	const friend C3Vector operator*(const float _lhs, const C3Vector &_rhs) { return C3Vector(_rhs.val[0] * _lhs,
																						_rhs.val[1] * _lhs,
																						_rhs.val[2] * _lhs); }
	const friend C3Vector operator/(const float _lhs, const C3Vector &_rhs) { return C3Vector(_rhs.val[0] / _lhs,
																						_rhs.val[1] / _lhs,
																						_rhs.val[2] / _lhs); }
	// Functions
	void zero(void) { val[0] = 0.0f; val[1] = 0.0f; val[2] = 0.0f; }
	float length(void) const;
	float length_squared(void) const;
	C3Vector& normalize(void);
	float dot(const C3Vector &_rhs) const;
	void set(float x, float y, float z) { val[0] = x; val[1] = y; val[2] = z; }
	C3Vector cross(const C3Vector &_rhs) const;
	bool write(FILE* fp);
};

class C3Point : public C3Vector
{
};

const C3Vector XAxis(1.0f, 0.0f, 0.0f);
const C3Vector YAxis(0.0f, 1.0f, 0.0f);
const C3Vector ZAxis(0.0f, 0.0f, 1.0f);

// ---------------------------------------------------------------------------------------------
// Four componenet vector
// ---------------------------------------------------------------------------------------------
class C4Vector
{
private:
	float val[4];
public:
	// Constructors
	C4Vector(void) {}
	C4Vector(float const x, float const y, float const z, const float w) { val[0] = x;
																		   val[1] = y;
																		   val[2] = z;
																		   val[3] = w; }
	// Accessors
	float& x(void) { return val[0]; }
	float& y(void) { return val[1]; }
	float& z(void) { return val[2]; }
	float& w(void) { return val[3]; }
	operator float*() const {return (float*)val;}
	// Operators
	C4Vector& operator=(const C4Vector &_rhs) { val[0] = _rhs.val[0];
												val[1] = _rhs.val[1];
												val[2] = _rhs.val[2];
												val[3] = _rhs.val[3];
												return *this; }
	const C4Vector operator+(const C4Vector &_rhs) const { C4Vector temp(*this);
											   temp += _rhs; 
											   return temp; }
	const C4Vector operator-(const C4Vector &_rhs) const { C4Vector temp(*this);
											   temp -= _rhs;
											   return temp; }
	const C4Vector operator*(const float _rhs) const { C4Vector temp(*this);
										   temp *= _rhs;
										   return temp; }
	const C4Vector operator/(const float _rhs) const { C4Vector temp(*this);
										   temp *= 1/_rhs;
										   return temp; }
	C4Vector& operator+=(const C4Vector &_rhs) { val[0] += _rhs.val[0];
												 val[1] += _rhs.val[1];
												 val[2] += _rhs.val[2];
												 val[3] += _rhs.val[3];
												 return *this; }
	C4Vector& operator-=(const C4Vector &_rhs) { val[0] -= _rhs.val[0];
												 val[1] -= _rhs.val[1];
												 val[2] -= _rhs.val[2];
												 val[3] -= _rhs.val[3];
												 return *this; }
	C4Vector& operator*=(const float _rhs) { val[0] *= _rhs;
											 val[1] *= _rhs;
											 val[2] *= _rhs;
											 val[3] *= _rhs;
											 return *this; }
	C4Vector& operator/=(const float _rhs) { *this *= 1/_rhs;
											 return *this; }
	const friend C4Vector operator*(const float _lhs, const C4Vector &_rhs) { return C4Vector(_rhs.val[0] * _lhs,
																						_rhs.val[1] * _lhs,
																						_rhs.val[2] * _lhs,
																						_rhs.val[3] * _lhs); }
	const friend C4Vector operator/(const float _lhs, const C4Vector &_rhs) { return C4Vector(_rhs.val[0] / _lhs,
																						_rhs.val[1] / _lhs,
																						_rhs.val[2] / _lhs,
																						_rhs.val[3] / _lhs); }
	// Functions
	void zero(void) { val[0] = 0.0f;
					  val[1] = 0.0f;
					  val[2] = 0.0f;
					  val[3] = 0.0f; }
	float length(void) const;
	float length_squared(void) const;
	C4Vector& normalize(void);
	float dot(const C4Vector &_rhs) const;
	void set(float x, float y, float z, float w) { val[0] = x; val[1] = y; val[2] = z; val[3] = w; }
	bool write(FILE* fp);
};

// ---------------------------------------------------------------------------------------------
// 4x4 Matrix
// ---------------------------------------------------------------------------------------------
class C4Matrix
{
private:
	C4Vector vec[4];
public:
	// Accessors
	operator C4Vector*() const { return (C4Vector*)vec; }
	// Operators
	C4Matrix& operator=(const C4Matrix &_rhs) { vec[0] = _rhs.vec[0];
												vec[1] = _rhs.vec[1];
												vec[2] = _rhs.vec[2];
												vec[3] = _rhs.vec[3];
												return *this; }
	const C4Matrix operator+(const C4Matrix &_rhs) const { C4Matrix temp(*this);
											   temp += _rhs; 
											   return temp; }
	const C4Matrix operator-(const C4Matrix &_rhs) const { C4Matrix temp(*this);
											   temp -= _rhs;
											   return temp; }
	const C4Matrix operator*(const float _rhs) const { C4Matrix temp(*this);
										   temp *= _rhs;
										   return temp; }
	const C4Matrix operator/(const float _rhs) const { C4Matrix temp(*this);
										   temp *= 1/_rhs;
										   return temp; }
	C4Matrix& operator+=(const C4Matrix &_rhs) { vec[0] += _rhs.vec[0];
												 vec[1] += _rhs.vec[1];
												 vec[2] += _rhs.vec[2];
												 vec[3] += _rhs.vec[3];
												 return *this; }
	C4Matrix& operator-=(const C4Matrix &_rhs) { vec[0] -= _rhs.vec[0];
												 vec[1] -= _rhs.vec[1];
												 vec[2] -= _rhs.vec[2];
												 vec[3] -= _rhs.vec[3];
												 return *this; }
	C4Matrix& operator*=(const float _rhs) { vec[0] *= _rhs;
											 vec[1] *= _rhs;
											 vec[2] *= _rhs;
											 vec[3] *= _rhs;
											 return *this; }
	C4Matrix& operator/=(const float _rhs) { *this *= 1/_rhs;
											 return *this; }
	const C4Matrix operator*(const C4Matrix _rhs) const;
	friend C3Vector operator*(const C3Vector _lhs, const C4Matrix _rhs)
	{
		C3Vector	dst;
		dst[0] = _lhs[0] * _rhs[0][0] + _lhs[1] * _rhs[1][0] + _lhs[2] * _rhs[2][0];
		dst[1] = _lhs[0] * _rhs[0][1] + _lhs[1] * _rhs[1][1] + _lhs[2] * _rhs[2][1];
		dst[2] = _lhs[0] * _rhs[0][2] + _lhs[1] * _rhs[1][2] + _lhs[2] * _rhs[2][2];
		return dst;
	}
	const friend C3Point operator*(const C3Point _lhs, const C4Matrix _rhs)
	{
		C3Point	dst;
		dst[0] = _lhs[0] * _rhs[0][0] + _lhs[1] * _rhs[1][0] + _lhs[2] * _rhs[2][0] + _rhs[3][0];
		dst[1] = _lhs[0] * _rhs[0][1] + _lhs[1] * _rhs[1][1] + _lhs[2] * _rhs[2][1] + _rhs[3][1];
		dst[2] = _lhs[0] * _rhs[0][2] + _lhs[1] * _rhs[1][2] + _lhs[2] * _rhs[2][2] + _rhs[3][2];
		return dst;
	}
	const friend C4Vector operator*(const C4Vector _lhs, const C4Matrix _rhs)
	{
		C4Vector	dst;
		dst[0] = _lhs[0] * _rhs[0][0] + _lhs[1] * _rhs[1][0] + _lhs[2] * _rhs[2][0] + _lhs[3] * _rhs[3][0];
		dst[1] = _lhs[0] * _rhs[0][1] + _lhs[1] * _rhs[1][1] + _lhs[2] * _rhs[2][1] + _lhs[3] * _rhs[3][1];
		dst[2] = _lhs[0] * _rhs[0][2] + _lhs[1] * _rhs[1][2] + _lhs[2] * _rhs[2][2] + _lhs[3] * _rhs[3][2];
		dst[3] = _lhs[0] * _rhs[0][3] + _lhs[1] * _rhs[1][3] + _lhs[2] * _rhs[2][3] + _lhs[3] * _rhs[3][3];
		return dst;
	}
	// Functions
	void identity(void) { vec[0][0] = 1.0f;		vec[1][0] = 0.0f;	vec[2][0] = 0.0f;	vec[3][0] = 0.0f;
						  vec[0][1] = 0.0f;		vec[1][1] = 1.0f;	vec[2][1] = 0.0f;	vec[3][1] = 0.0f;
						  vec[0][2] = 0.0f;		vec[1][2] = 0.0f;	vec[2][2] = 1.0f;	vec[3][2] = 0.0f;
						  vec[0][3] = 0.0f;		vec[1][3] = 0.0f;	vec[2][3] = 0.0f;	vec[3][3] = 1.0f; }
	C4Matrix& make_rot(const C3Vector &axis, const float angle);
	C4Matrix& make_trans(const C3Vector &offset);
	C4Matrix& invert(void);
	C4Matrix& transpose_and_negate(void);
	C4Matrix& transpose(void);
	bool write(FILE* fp);
};

class CPlane
{
private:
	C3Vector	norm;
	float		dist;
public:
	// Accessors
	C3Vector& normal(void) { return norm; }
	float& distance(void) { return dist; }
	// Functions
	const float dist_to_point(const C3Vector &point) const {return norm.dot(point) + dist; }
	bool write(FILE* fp);
	void normalize(void);
};

class CSphere
{
private:
	C3Point	cent;
	float	rad;
public:
	// Accessors
	C3Vector& center(void) { return cent; }
	float& radius(void) { return rad; }
	const bool intersects(const CSphere &_test) const { return (_test.cent - cent).length_squared() - ((rad + _test.rad) * (rad + _test.rad)) <= 0.0f; }
	const bool intersects(const CPlane &_test) const { return fabsf(_test.dist_to_point(cent)) <= rad; }
	bool write(FILE* fp);
};

class CAABBox
{
public:
	C3Point	min, max;
};

#endif // __VECMATH_H__
