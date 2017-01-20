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

//#include "StdAfx.h"
#include "pi.h"
#include <math.h>
//#include <cmath>

//using namespace std;

#define NUM_TAB 360
#define FNUM_TAB 36000.0F

#ifndef FPVAL
#define FP 8
#define FPVAL 256
#endif

int TableMade = 0;
float SinTab[NUM_TAB + 1];
float CosTab[NUM_TAB + 1];

void MakeTable(){
	for(int i = 0; i < NUM_TAB + 1; i++){
		SinTab[i] = sinf((float)i * (float)DEG2RAD);
		CosTab[i] = cosf((float)i * (float)DEG2RAD);
	}
	TableMade = 1;
}
float tsin(float a){
	if(TableMade == 0) MakeTable();
//	int ia = (int)floorf(FNUM_TAB + a) % NUM_TAB;
	int ia = (int)(FNUM_TAB + a) % NUM_TAB;
	return SinTab[ia] + (SinTab[ia + 1] - SinTab[ia]) * fabsf(a - floorf(a));
}
float tcos(float a){
	if(TableMade == 0) MakeTable();
//	int ia = (int)floorf(FNUM_TAB + a) % NUM_TAB;
	int ia = (int)(FNUM_TAB + a) % NUM_TAB;
	return CosTab[ia] + (CosTab[ia + 1] - CosTab[ia]) * fabsf(a - floorf(a));
}

int fpcos(float a){
	return (int)(cos((a - 90) * DEG2RAD) * FPVAL);
}
int fpsin(float a){
	return (int)(sin((a - 90) * DEG2RAD) * FPVAL);
}
