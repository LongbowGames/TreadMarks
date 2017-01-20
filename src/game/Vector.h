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

//Vector.h, 3D vector inline function library, by Seumas McNally, 1998.

//Matrices are just groups of vectors.  A 3x3 matrix here is:  [vector][coord]
//So &Mat3[n][0] is a Vec3 array.
//
//Vec3 [x, y, z]
//
//     [Xx, Xy, Xz]
//Mat3 [Yx, Yy, Yz]
//     [Zx, Zy, Zz]
//
//[0][0], [0][1], [0][2]
//[1][0], [1][1], [1][2]
//[2][0], [2][1], [2][2]
//
//Where X, Y, and Z are the coordinate system axis vectors,
//and x, y, and z are their vector coordinates.

#ifndef VECTOR_H
#define VECTOR_H

#ifdef WIN32
#pragma inline_depth(255)
#endif

#include <cmath>
#include <cfloat>
#include <limits>

#include "macros.h"

#define R2D  57.2957795147f
#define D2R  0.01745329251944f
#define PI 3.1415926535f
#define PI2 6.283185307f
#define PIOVER2 1.57079632675f
#define PIRECIP 0.3183098861929f

typedef float Pnt2[2];
typedef float Pnt3[3];
typedef float Vec2[2];
typedef float Vec3[3];
typedef float Mat3[3][3];	//3x3 rotation matrix.
typedef float Mat43[4][3];	//4x3 rotation plus translation matrix, non-homogeneous.
typedef float Mat4[4][4];
typedef float Rot3[3];	//A, B, C rotation angles about X, Y, and Z respectively.

#define LERP(a,b,t) ((a) + ((b) - (a)) * (t))
#define CLAMP(x, l, h) ( ((x) >= (l) ? ((x) <= (h) ? (x) : (h)) : (l)) )
//More true (faster?) ifs if no clamping needed.
// ((x) < (l) ? (l) : (x) > (h) ? (h) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define SQUARE(a) ((a) * (a))
#define DAMP(val, damp, time) ((val) * pow(MAX(0.00001f, (damp)), (time)))


//Fast, rounding-mode-don't-care float to long conversion.
inline int FastFtoL(float f){
#ifdef WIN32
		int i;
		_asm{
			fld dword ptr [f]
			fistp dword ptr [i]
		}
		return i;
#else
        return (int)f;
#endif
};

//This is NOT high enough precision for normalizing ordinary vectors or matrices!
float CDECL fsqrt_inv(float f);

//More random randoms.
void TMsrandom(int s);
int TMrandom();

class CVec3{
public:
	float x, y, z;
	inline CVec3(){x = 0; y = 0; z = 0;};
	inline CVec3(const float _x, const float _y, const float _z) {x = _x; y = _y; z = _z;};
	inline operator float*(){ return (float*)this; };
};

//Vector/Point/Euler functions.
inline void SetVec3(const float x, const float y, const float z, Vec3 dst){
	dst[0] = x; dst[1] = y; dst[2] = z;
};
inline void ClearVec3(Vec3 dst){
	dst[0] = dst[1] = dst[2] = 0.0f;
};
inline void ScaleVec3(const Vec3 src, const float scale, Vec3 dst){	//Scales a vector to a dest vector.
	dst[0] = src[0] * scale;
	dst[1] = src[1] * scale;
	dst[2] = src[2] * scale;
};
inline void ScaleVec3(Vec3 src, const float scale){	//Scales a vector in place.
	src[0] *= scale;
	src[1] *= scale;
	src[2] *= scale;
};
inline void ScaleAddVec3(const Vec3 src, const float scale, const Vec3 src2, Vec3 dst){	//dst == src is OK.
	dst[0] = src2[0] + src[0] * scale;
	dst[1] = src2[1] + src[1] * scale;
	dst[2] = src2[2] + src[2] * scale;
};
inline void ScaleAddVec3(const Vec3 src, const float scale, Vec3 dst){	//dst == src is OK.
	ScaleAddVec3(src, scale, dst, dst);
};
inline void SubVec3(const Vec3 src1, const Vec3 src2, Vec3 dst){	//dst == src is OK.
	dst[0] = src1[0] - src2[0];
	dst[1] = src1[1] - src2[1];
	dst[2] = src1[2] - src2[2];
};
inline void AddVec3(const Vec3 src1, const Vec3 src2, Vec3 dst){	//dst == src is OK.
	dst[0] = src1[0] + src2[0];
	dst[1] = src1[1] + src2[1];
	dst[2] = src1[2] + src2[2];
};
inline void AddVec3(const Vec3 src, Vec3 dst){	//dst == src is OK.
	dst[0] += src[0];
	dst[1] += src[1];
	dst[2] += src[2];
};
inline void CopyVec3(const Vec3 src, Vec3 dst){	//dst == src is silly.
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
};
inline void LerpVec3(const Vec3 src1, const Vec3 src2, const float t, Vec3 dst){	//Linear Interpolate between two vectors, dst MAY BE one of the sources.
	dst[0] = LERP(src1[0], src2[0], t);
	dst[1] = LERP(src1[1], src2[1], t);
	dst[2] = LERP(src1[2], src2[2], t);
};
inline float DotVec3(const Vec3 src1, const Vec3 src2){	//Dot product of input vectors returned.
	return src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2];
};
//For counter-clockwise winding, cross pt1-pt2 with pt3-pt2 to get face normal.
inline void CrossVec3(const Vec3 src1, const Vec3 src2, Vec3 dst){	//Cross product of source vectors.  Dest must be different!
	dst[0] = src1[1] * src2[2] - src1[2] * src2[1];
	dst[1] = src1[2] * src2[0] - src1[0] * src2[2];
	dst[2] = src1[0] * src2[1] - src1[1] * src2[0];
};
inline float LengthVec3(const Vec3 src1){
	return sqrtf(src1[0] * src1[0] + src1[1] * src1[1] + src1[2] * src1[2]);
};
/*
inline void NormVec3(Vec3 src1, Vec3 dst){	//Normalizes vector.  dst == src is OK.
//	float d = LengthVec3(src1);
//	if(d == 0.0f){
//		ClearVec3(dst);
//	}else{
//		float tlenr = 1.0f / d;//LengthVec3(src1);
	float tlenr = fsqrt_inv(src1[0] * src1[0] + src1[1] * src1[1] + src1[2] * src1[2]);
		dst[0] = src1[0] * tlenr;
		dst[1] = src1[1] * tlenr;
		dst[2] = src1[2] * tlenr;
//	}
};
*/
inline void NormVec3(const Vec3 src1, Vec3 dst){	//Normalizes vector.  dst == src is OK.
	float d = LengthVec3(src1);
	if(d == 0.0f){
		ClearVec3(dst);
	}else{
		float tlenr = 1.0f / d;//LengthVec3(src1);
//	float tlenr = fsqrt_inv(src1[0] * src1[0] + src1[1] * src1[1] + src1[2] * src1[2]);
		dst[0] = src1[0] * tlenr;
		dst[1] = src1[1] * tlenr;
		dst[2] = src1[2] * tlenr;
	}
};
inline void NormVec3(Vec3 dst){
	NormVec3(dst, dst);
};
inline float DistVec3(const Vec3 src1, const Vec3 src2){
	Vec3 t = {src1[0] - src2[0], src1[1] - src2[1], src1[2] - src2[2]};
	return LengthVec3(t);
};

//Matrix functions.
inline void IdentityMat3(Mat3 dst){
	dst[0][0] = 1.0f; dst[0][1] = 0.0f; dst[0][2] = 0.0f;
	dst[1][0] = 0.0f; dst[1][1] = 1.0f; dst[1][2] = 0.0f;
	dst[2][0] = 0.0f; dst[2][1] = 0.0f; dst[2][2] = 1.0f;
};
inline void IdentityMat43(Mat43 dst){
	dst[0][0] = 1.0f; dst[0][1] = 0.0f; dst[0][2] = 0.0f;
	dst[1][0] = 0.0f; dst[1][1] = 1.0f; dst[1][2] = 0.0f;
	dst[2][0] = 0.0f; dst[2][1] = 0.0f; dst[2][2] = 1.0f;
	dst[3][0] = 0.0f; dst[3][1] = 0.0f; dst[3][2] = 0.0f;
};
inline void NormMat3(const Mat3 src, Mat3 dst){
	NormVec3(src[0], dst[0]);
	NormVec3(src[1], dst[1]);
	NormVec3(src[2], dst[2]);
};
inline void NormMat3(Mat3 mat){
	NormMat3(mat, mat);
};
inline void LerpMat3(const Mat3 src1, const Mat3 src2, float t, Mat3 dst){
//	for(int i = 0; i < 3; i++){
	LerpVec3(src1[0], src2[0], t, dst[0]); NormVec3(dst[0]);
	LerpVec3(src1[1], src2[1], t, dst[1]); NormVec3(dst[1]);
	LerpVec3(src1[2], src2[2], t, dst[2]); NormVec3(dst[2]);
//	}
};
inline void Vec3MulMat3(const Vec3 src, const Mat3 mat, Vec3 dst){	//Grinds src vec through rotation matrix to produce output vec.
	dst[0] = mat[0][0] * src[0] + mat[1][0] * src[1] + mat[2][0] * src[2];	//Dest X coord is X of three matrix vectors multiplied by first vector coords which are the scalars.
	dst[1] = mat[0][1] * src[0] + mat[1][1] * src[1] + mat[2][1] * src[2];
	dst[2] = mat[0][2] * src[0] + mat[1][2] * src[1] + mat[2][2] * src[2];
};
inline void Vec3IMulMat3(const Vec3 src, const Mat3 mat, Vec3 dst){	//This should multiply the vector back through the matrix, so a vector in the matrix's coord space becomes one in the default coord space.
	dst[0] = mat[0][0] * src[0] + mat[0][1] * src[1] + mat[0][2] * src[2];	//Dest X coord is X of three matrix vectors multiplied by first vector coords which are the scalars.
	dst[1] = mat[1][0] * src[0] + mat[1][1] * src[1] + mat[1][2] * src[2];
	dst[2] = mat[2][0] * src[0] + mat[2][1] * src[1] + mat[2][2] * src[2];
};
inline void Mat3MulMat3(const Mat3 src1, const Mat3 src2, Mat3 dst){
	Vec3MulMat3(src1[0], src2, dst[0]);	//Grind X axis vector of first matrix through second matrix, dump into X axis vector of third.
	Vec3MulMat3(src1[1], src2, dst[1]);	//Ditto Y axis vector.
	Vec3MulMat3(src1[2], src2, dst[2]);	//Ditto Z axis vector.
};
inline void Mat3IMulMat3(const Mat3 src1, const Mat3 src2, Mat3 dst){
	Vec3IMulMat3(src1[0], src2, dst[0]);	//Grind X axis vector of first matrix through second matrix, dump into X axis vector of third.
	Vec3IMulMat3(src1[1], src2, dst[1]);	//Ditto Y axis vector.
	Vec3IMulMat3(src1[2], src2, dst[2]);	//Ditto Z axis vector.
};

inline void Vec3MulMat43(const Vec3 src, const Mat43 mat, Vec3 dst){	//Rotation + translation matrix mult.
	dst[0] = mat[0][0] * src[0] + mat[1][0] * src[1] + mat[2][0] * src[2] + mat[3][0];	//Dest X coord is X of three matrix vectors multiplied by first vector coords which are the scalars.
	dst[1] = mat[0][1] * src[0] + mat[1][1] * src[1] + mat[2][1] * src[2] + mat[3][1];
	dst[2] = mat[0][2] * src[0] + mat[1][2] * src[1] + mat[2][2] * src[2] + mat[3][2];
};
inline void Vec3IMulMat43(const Vec3 src, const Mat43 mat, Vec3 dst){
	Vec3 t = {src[0] - mat[3][0], src[1] - mat[3][1], src[2] - mat[3][2]};	//I think this should work...
	dst[0] = mat[0][0] * t[0] + mat[0][1] * t[1] + mat[0][2] * t[2];	//Dest X coord is X of three matrix vectors multiplied by first vector coords which are the scalars.
	dst[1] = mat[1][0] * t[0] + mat[1][1] * t[1] + mat[1][2] * t[2];
	dst[2] = mat[2][0] * t[0] + mat[2][1] * t[1] + mat[2][2] * t[2];
};
inline void Mat43MulMat43(const Mat43 src1, const Mat43 src2, Mat43 dst){
	Vec3MulMat3(src1[0], src2, dst[0]);	//Grind X axis vector of first matrix through second matrix, dump into X axis vector of third.
	Vec3MulMat3(src1[1], src2, dst[1]);	//Ditto Y axis vector.
	Vec3MulMat3(src1[2], src2, dst[2]);	//Ditto Z axis vector.
//	Vec3 tmp;
//	AddVec3(src1[3], src2[3], tmp);	//Ditto Translate axis vector.
//	Vec3MulMat3(tmp, src2, dst[3]);	//Ok, it needs to be rotated after adding...
	Vec3MulMat43(src1[3], src2, dst[3]);
	//Nope, actually translate should be fully 43 matrix mulled by the matrix.
	//
//	Vec3MulMat43(src1[0], src2, dst[0]);	//Grind X axis vector of first matrix through second matrix, dump into X axis vector of third.
//	Vec3MulMat43(src1[1], src2, dst[1]);	//Ditto Y axis vector.
//	Vec3MulMat43(src1[2], src2, dst[2]);	//Ditto Z axis vector.
//	Vec3MulMat43(src1[3], src2, dst[3]);	//Ditto Translate axis vector.
};

inline void Rot3ToMat3(const Rot3 rot, Mat3 dst){	//Creates a 3x3 rotation matrix based on the input angles, in Radians.
	float sina = (float)sin(rot[0]), cosa = (float)cos(rot[0]);
	float sinb = (float)sin(rot[1]), cosb = (float)cos(rot[1]);
	float sinc = (float)sin(rot[2]), cosc = (float)cos(rot[2]);
	float tYx = (sinb * sina), tYz = (cosb * sina);
	dst[0][0] = (cosb) * cosc + (tYx) * -sinc;	//Lets make sure this works before optimizing stuff out.
	dst[0][1] = (cosa) * -sinc;
	dst[0][2] = (-sinb) * cosc + (tYz) * -sinc;
	dst[1][0] = (tYx) * cosc + (cosb) * sinc;	//(y axis vector component) (x component)
	dst[1][1] = (cosa) * cosc;
	dst[1][2] = (tYz) * cosc + (-sinb) * sinc;
	dst[2][0] = sinb * cosa;	//Z axis vector is fine now, isn't affected by C rotation.
	dst[2][1] = -sina;
	dst[2][2] = cosb * cosa;
};
inline void Mat3ToRot3(const Mat3 mat, Rot3 rot){	//Extracts H, P, Bs from rotation matrix.
	rot[0] = (float)asin(-mat[2][1]);
	if(cos(rot[0]) != 0.0f){
		rot[1] = (float)atan2f(mat[2][0], mat[2][2]);
		rot[2] = (float)atan2f(-mat[0][1], mat[1][1]);
	}else{
		rot[1] = (float)atan2f(mat[1][0], mat[1][2]);
		rot[2] = 0.0f;
	}
};

//Euler functions.
inline float NormRot(float r){
	if(std::numeric_limits<float>::infinity() != r){
		if(r > PI2 * 10.0f || r < -PI2 * 10.0f) r = fmodf(r, PI2);
		while(r < PI) r += PI2;
		while(r > PI) r -= PI2;
		return r;
	}
	return 0.0f;
};
inline void NormRot3(Rot3 rot){	//Brings rotations into the range 0 to PI * 2.
	for(int i = 0; i < 3; i++){
		rot[i] = NormRot(rot[i]);
	}
};
inline void DeltaRot3(Rot3 rot, const float clamp = PI){	//Normalizes rotation deltas to good values.
	NormRot3(rot);	//First get in good range.
	for(int i = 0; i < 3; i++){
		if(rot[i] > PI) rot[i] -= PI2;	//Greater than 180 rotation, so go the other way.
		rot[i] = CLAMP(rot[i], -clamp, clamp);	//Optionally clamp to lower than PI values in each direction.
	}
};
/*	dst[0][0] = (cosb) * cosc + (sinb * sina) * -sinc;	//Three axis, A, B, C.
	dst[0][1] = (0) * cosc + (cosa) * -sinc;
	dst[0][2] = (-sinb) * cosc + (cosb * sina) * -sinc;
	dst[1][0] = (sinb * sina) * cosc + (cosb) * sinc;	//(y axis vector component) (x component)
	dst[1][1] = (cosa) * cosc + 0 * sinc;
	dst[1][2] = (cosb * sina) * cosc + (-sinb) * sinc;
	dst[2][0] = sinb * cosa;	//Z axis vector is fine now, isn't affected by C rotation.
	dst[2][1] = -sina;
	dst[2][2] = cosb * cosa;*/
/*	dst[0][0] = cosb;	//Two axis, A, B rotation.
	dst[0][1] = 0;
	dst[0][2] = -sinb;
	dst[1][0] = sinb * sina;
	dst[1][1] = cosa;
	dst[1][2] = cosb * sina;
	dst[2][0] = sinb * cosa;
	dst[2][1] = -sina;
	dst[2][2] = cosb * cosa;*/
/*	dst[0][0] = cosb;	//One axis, B rotation.
	dst[0][1] = 0;
	dst[0][2] = -sinb;
	dst[1][0] = 0;
	dst[1][1] = 1;
	dst[1][2] = 0;
	dst[2][0] = sinb;
	dst[2][1] = 0;
	dst[2][2] = cosb;*/

#endif
