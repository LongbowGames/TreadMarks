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

#include "Vector.h"
#include <cmath>
#include <cstdlib>

/*
//Quietly handle math errors sanely.
int CDECL _matherr( struct _exception *except ){
	except->retval = 0.0001;
	return 1;
}
*/

int seed1 = 0, seed2 = 0;

void TMsrandom(int s){
	srand(s);
	seed1 = s + 3;
	seed2 = s + 7;
}

int TMrandom(){
	int r1 = (seed1 = (seed1 * 1664525) + 1013904223);
	int r2 = ((seed2 = seed2 * 1103515245 + 12345) >> 16) & 0x7fff;
	return (r1 ^ r2 ^ rand()) & RAND_MAX;	//relies on RAND_MAX being contiguous 1s.
}

CVec3 tv;

void foo(){
	tv.y = 0.0f;
	tv[1] = 1.0f;
	SetVec3(1.0f, 2.0f, 3.0f, tv);
	CopyVec3(tv, tv);
}

// Gary Tarolli's clever inverse square root technique
const float ONE_HALF = 0.5f;
const float THREE_HALVES = 1.5f;
/*
float fsqrt_inv(float f)
{
 long i;
 float x2, y;
 x2 = 0.5f*f;
 i = *(long *)&f;
 i = 0x5f3759df - (i>>1);
 y = *(float *)&i;
 // repeat this iteration for more accuracy
 y = 1.5f*y - (x2*y * y*y);
 return y;
}
*/
// same as above but in assembly
#ifdef WIN32
	__declspec(naked) float CDECL fsqrt_inv(float f)
	{
	 __asm // 18 cycles
	 {
	  fld   dword ptr [esp + 4]
	  fmul  dword ptr [ONE_HALF]
	  mov   eax, [esp + 4]
	  mov   ecx, 0x5f3759df
	  shr   eax, 1
	  sub   ecx, eax
	  mov   [esp + 4], ecx
	  fmul  dword ptr [esp + 4]
	  fld   dword ptr [esp + 4]
	  fmul  dword ptr [esp + 4]
	  fld   dword ptr [THREE_HALVES]
	  fmul  dword ptr [esp + 4]
	  fxch  st(2)
	  fmulp  st(1), st
	  fsubp  st(1), st
	  ret
	 }
	};
#else
	float fsqrt_inv(float f)
	{
	 long i;
	 float x2, y;
	 x2 = 0.5f*f;
	 i = *(long *)&f;
	 i = 0x5f3759df - (i>>1);
	 y = *(float *)&i;
	 // repeat this iteration for more accuracy
	 y = 1.5f*y - (x2*y * y*y);
	 return y;
	}
#endif
