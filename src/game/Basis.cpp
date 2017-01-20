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

//Perlin based Noise function with built in turbulence octaves and
//random initial values generated based on integer coordinates, so
//no pre-computed lattice is required.
//
//By Seumas McNally, May 1998.
//
//May 12th, added integer-only fixed point version of noise, and it's fast!
//Also optimized double precision version to not use recursion, and simply
//loop instead.  I don't know why I didn't see how that would work before.
//Floating point version is much faster now too.
//
//Yeesh, I forgot the guys name and had "Pellner based Noise" written in
//the earlier versions.  :)
//
//"Texturing and Modeling: A Procedural Approach" is an awesome book!!
//
//Switched over to using tables of random numbers instead of hashing on
//the fly from x and y coords after seeing a good example in above book.
//
//Woo, got rid of modf(), boosted double precision noise speed by a ton...
//
//Negative X/Y values passed to Noise should be OK now.
//

#include "Basis.h"
#include <math.h>

//using namespace std;

#define ONEDIV256 0.00390625

//This is used to scale down the current turbulence addition.
const float OctaveMult[MAXOCTAVE + 1] = {
	1.0f,
	0.5f,
	0.25f,
	0.125f,
	0.0625f,
	0.03125f,
	0.015625f};
//And this to rescale the resultant noise with all turbulences to 0.0 - 1.0.
const float OctaveScale[MAXOCTAVE + 1] = {
	1.0f,
	1.0f / 1.5f,
	1.0f / (1.5f + 0.25f),
	1.0f / (1.5f + 0.25f + 0.125f),
	1.0f / (1.5f + 0.25f + 0.125f + 0.0625f),
	1.0f / (1.5f + 0.25f + 0.125f + 0.0625f + 0.03125f),
	1.0f / (1.5f + 0.25f + 0.125f + 0.0625f + 0.03125f + 0.015625f)};
//This is used to scale down the current turbulence addition.
const int OctaveMult8[MAXOCTAVE + 1] = {
	(const int)(256.0 * 1.0),
	(const int)(256.0 * 0.5),
	(const int)(256.0 * 0.25),
	(const int)(256.0 * 0.125),
	(const int)(256.0 * 0.0625),
	(const int)(256.0 * 0.03125),
	(const int)(256.0 * 0.015625)};
//And this to rescale the resultant noise with all turbulences to 0 - 255.
const int OctaveScale8[MAXOCTAVE + 1] = {
	(const int)(256.0 * (1.0)),
	(const int)(256.0 * (1.0 / 1.5)),
	(const int)(256.0 * (1.0 / (1.5 + 0.25))),
	(const int)(256.0 * (1.0 / (1.5 + 0.25 + 0.125))),
	(const int)(256.0 * (1.0 / (1.5 + 0.25 + 0.125 + 0.0625))),
	(const int)(256.0 * (1.0 / (1.5 + 0.25 + 0.125 + 0.0625 + 0.03125))),
	(const int)(256.0 * (1.0 / (1.5 + 0.25 + 0.125 + 0.0625 + 0.03125 + 0.015625)))};

#define TABMASK (TABSIZE - 1)
#define PERM(x) (NoisePermTable[(x) & TABMASK])
#define INDEX(x, y) (PERM((x) + PERM(y)))
/*
#define MAXNTH 10
#define TABSIZE 256
double NoiseDoubleTable[TABSIZE * 2];
unsigned char NoisePermTable[TABSIZE];
unsigned char NoiseCharTable[TABSIZE];
static double NthDist[MAXNTH];
*/
//#define RANDXY(x, y) (seed[x & (NSEED - 1)][y & (NSEED - 1)] & 255)
//#define SIMPLERANDXY(a, b) STDRAND(SIMPLERAND(STDRAND(SIMPLERAND(a * 12) ^ STDRAND(b * 21))))

#define RANDXY(x, y) (NoiseCharTable[INDEX(x, y)])
#define DRANDXY(x, y) (NoiseFloatTable[INDEX(x, y)])
//#define RANDXY(x, y) ((SIMPLERANDXY(x + seed, y - seed) >> 4) & 255)
#define SIMPLERANDXY(a, b) STDRAND(SIMPLERAND(a) ^ STDRAND(b))

#define SIMPLERAND(a) ((a) * 1664525 + 1013904223)
#define STDRAND(a) ((((a) * 1103515245 + 12345) >> 16) & 0x7fff)
//Goofed this before, you set the seed PRIOR to down shifting and masking...
#define STDSEEDRAND(seedvar) (((seedvar = (seedvar * 1103515245 + 12345)) >> 16) & 0x7fff)

void Basis::Seed(int seed){
	int i;
	for(i = 0; i < TABSIZE; i++){
		NoisePermTable[i] = STDSEEDRAND(seed);
	}
	for(i = 0; i < TABSIZE; i++){
		NoiseFloatTable[i] = NoiseFloatTable[i + TABSIZE] = (float)STDSEEDRAND(seed) / (float)0x7fff;
		NoiseNegTable[i] = NoiseNegTable[i + TABSIZE] = ((float)STDSEEDRAND(seed) / (float)0x7fff) - 0.5f;
		NoiseCharTable[i] = (unsigned char)(NoiseFloatTable[i] * 255.0f);
	}
}

//Implementation of basic idea behind Steve Worley's Cellular Texture Basis Function.
//Uses same seed as Noise.
FeaturePoint *Basis::Cellular(float x, float y, int nth, float irreg,
							  int xrpt, int yrpt, int manhattan){
	int i = 0, j = 0, x1, y1, ix, iy, ix2, iy2;
	float xd, yd;
	FeaturePoint dt, t;
	float *pd;

	if(nth < 1 || nth > MAXNTH) return NthFeature;	//Only handling 1st and 2nd closest feature points.

	for(i = 0; i < nth; i++) NthFeature[i].f = 10.0f;

	if(xrpt){
		if(x < 0.0f) x = (float)xrpt + fmodf(x, (float)xrpt);
		else x = fmodf(x, (float)xrpt);
	}
	if(yrpt){
		if(y < 0.0f) y = (float)yrpt + fmodf(y, (float)yrpt);
		else y = fmodf(y, (float)yrpt);
	}
	x1 = (int)x;  if(x < 0.0f) x1--;
	y1 = (int)y;  if(y < 0.0f) y1--;
	x -= 0.5f;
	y -= 0.5f;	//Added to allow irregularity scaling.
	if(nth > 2){
		//Bigger nth, scan 25 squares around point and use sort loop.
		for(iy = y1 - 2; iy < y1 + 3; iy++){
			for(ix = x1 - 2; ix < x1 + 3; ix++){
				//Calculate one index in double table for the square, then simply
				//increment it for each double we need pulled out.
				ix2 = ix;
				iy2 = iy;
				if(xrpt){
					if(ix2 >= xrpt) ix2 = 0;
					if(ix2 < 0) ix2 = xrpt - 1;
				}
				if(yrpt){
					if(iy2 >= yrpt) iy2 = 0;
					if(iy2 < 0) iy2 = yrpt - 1;
				}
				pd = &NoiseNegTable[dt.i = INDEX(ix2, iy2)];
				xd = (float)ix + pd[0] * irreg - x;
				yd = (float)iy + pd[1] * irreg - y;
				if(manhattan) dt.f = (float)(fabsf(xd) + fabsf(yd));
				else dt.f = (float)(xd * xd + yd * yd);
				for(j = 0; j < nth; j++){
					if(dt.f < NthFeature[j].f){
						t = NthFeature[j]; NthFeature[j] = dt; dt = t;
					}
				}
			}
		}
	}else{
		//Only looking for first or second closest, scan 9 squares and use quick lowest-2 check.
		for(iy = y1 - 1; iy < y1 + 2; iy++){
			for(ix = x1 - 1; ix < x1 + 2; ix++){
				//Calculate one index in double table for the square, then simply
				//increment it for each double we need pulled out.
				ix2 = ix;
				iy2 = iy;
				if(xrpt){
					if(ix2 >= xrpt) ix2 = 0;
					if(ix2 < 0) ix2 = xrpt - 1;
				}
				if(yrpt){
					if(iy2 >= yrpt) iy2 = 0;
					if(iy2 < 0) iy2 = yrpt - 1;
				}
				pd = &NoiseNegTable[dt.i = INDEX(ix2, iy2)];
				xd = (float)ix + pd[0] * irreg - x;
				yd = (float)iy + pd[1] * irreg - y;
				if(manhattan) dt.f = (float)(fabsf(xd) + fabsf(yd));
				else dt.f = (float)(xd * xd + yd * yd);
				if(dt.f < NthFeature[0].f){
					NthFeature[1] = NthFeature[0];
					NthFeature[0] = dt;
				}else{
					if(dt.f < NthFeature[1].f) NthFeature[1] = dt;
				}
			}
		}
	}
	return NthFeature;
}

float Basis::Noise(float x, float y, int octaves, int xrpt, int yrpt){
	if(octaves < 0 || octaves > MAXOCTAVE) return 0.5f;
	int x1, y1, x1p1, y1p1, i;
	float xfrac, yfrac, d;
	d = 0.0f;
	for(i = 0; i <= octaves; i++){
		x1 = (int)x;
		if(x < 0.0f) x1--;	//This little check should allow negative x,y values passed in to Noise to work perfectly.
		xfrac = x - (float)x1;
		xfrac = (xfrac * xfrac) * (3.0f - (xfrac * 2.0f));	//Add slope to values, rather than linearly interpolating.
		if(xrpt){	//Internal repetition added as an option.
			if(x1 < 0)  x1 = (xrpt - 1) + ((x1 + 1) % xrpt);
			else  x1 = x1 % xrpt;
			x1p1 = x1 + 1;
			if(x1p1 >= xrpt) x1p1 -= xrpt;
			xrpt *= 2;
		}else x1p1 = x1 + 1;
		y1 = (int)y;
		if(y < 0.0f) y1--;
		yfrac = y - (float)y1;
		yfrac = (yfrac * yfrac) * (3.0f - (yfrac * 2.0f));
		if(yrpt){
			if(y1 < 0)  y1 = (yrpt - 1) + ((y1 + 1) % yrpt);
			else  y1 = y1 % yrpt;
			y1p1 = y1 + 1;
			if(y1p1 >= yrpt) y1p1 -= yrpt;
			yrpt *= 2;
		}else y1p1 = y1 + 1;
		d += (((DRANDXY(x1, y1) * (1.0f - xfrac) +
			DRANDXY((x1p1), y1) * xfrac) * (1.0f - yfrac) +
			(DRANDXY(x1, (y1p1)) * (1.0f - xfrac) +
			DRANDXY((x1p1), (y1p1)) * xfrac) * yfrac)/* * ONEDIV256*/) * OctaveMult[i];
		x = (x + 7.0f) * 2.0f;
		y = (y + 7.0f) * 2.0f;
	}
	return d * OctaveScale[octaves];
}

/*
//Speed increase doesn't seem too significant...
int Basis::Noise(unsigned char *table, int length, double x, double y, double xdelta, double ydelta,
		int octaves, int xrpt, int yrpt){
	if(octaves < 0 || octaves > MAXOCTAVE || table == 0 || length <= 0) return 0;
	
	int x1[MAXOCTAVE + 1];
	int y1[MAXOCTAVE + 1];
	int xd[MAXOCTAVE + 1];
	int yd[MAXOCTAVE + 1];
	int xr[MAXOCTAVE + 1];
	int yr[MAXOCTAVE + 1];
	int x1p1, y1p1, i, j;
//	unsigned int xfrac[MAXOCTAVE + 1];
//	unsigned int yfrac[MAXOCTAVE + 1];
//	double xd[MAXOCTAVE + 1];
//	double yd[MAXOCTAVE + 1];
	int d, xfrac, yfrac;
	d = 0;
	for(i = 0; i <= octaves; i++){
		//Setup.
		//Set and wrap x1, y1, which'll be 16.16.
		//Set xd, yd.
		x1[i] = (int)(x * (double)0x10000);
		xr[i] = xrpt * 0x10000;
		xd[i] = (int)(xdelta * (double)0x10000);
		if(xrpt){
			if(x1[i] < 0)  x1[i] = ((xrpt) * 0x10000 - 1) + (x1[i] % (xrpt * 0x10000));
			else  x1[i] = x1[i] % (xrpt * 0x10000);
		}
		y1[i] = (int)(y * (double)0x10000);
		yr[i] = yrpt * 0x10000;
		yd[i] = (int)(ydelta * (double)0x10000);
		if(yrpt){
			if(y1[i] < 0)  y1[i] = ((yrpt) * 0x10000 - 1) + (y1[i] % (yrpt * 0x10000));
			else  y1[i] = y1[i] % (yrpt * 0x10000);
		}
		xrpt *= 2;
		yrpt *= 2;
		x = (x + 7.0) * 2.0;
		y = (y + 7.0) * 2.0;
		xdelta *= 2.0;
		ydelta *= 2.0;
	}

	for(j = 0; j < length; j++){
		d = 0;
		for(i = 0; i <= octaves; i++){
			//Add xd, yd.
			//Slope fraction of x1, y1 into xfrac, yfrac for lerping.
			x1[i] += xd[i];
			if(xrpt){
				if(x1[i] >= xr[i]) x1[i] -= xr[i];
				if(x1[i] < 0) x1[i] += xr[i];
				x1p1 = x1[i] + 0x10000;
				if(x1p1 >= xr[i]) x1p1 -= xr[i];
			}else x1p1 = x1[i] + 0x10000;
			xfrac = (x1[i] >>8) & 0xff;
			xfrac = (((xfrac * xfrac) >>8) * ((0x300) - (xfrac <<1))) >>8;
			y1[i] += yd[i];
			if(yrpt){
				if(y1[i] >= yr[i]) y1[i] -= yr[i];
				if(y1[i] < 0) y1[i] += yr[i];
				y1p1 = y1[i] + 0x10000;
				if(y1p1 >= yr[i]) y1p1 -= yr[i];
			}else y1p1 = y1[i] + 0x10000;
			yfrac = (y1[i] >>8) & 0xff;
			yfrac = (((yfrac * yfrac) >>8) * ((0x300) - (yfrac <<1))) >>8;
			d += (((((RANDXY(x1[i] >>16, y1[i] >>16) * (256 - xfrac) +
				RANDXY((x1p1 >>16), y1[i] >>16) * xfrac) >>8) * (256 - yfrac) +
				((RANDXY(x1[i] >>16, (y1p1 >>16)) * (256 - xfrac) +
				RANDXY((x1p1 >>16), (y1p1 >>16)) * xfrac) >>8) * yfrac) * OctaveMult8[i]) >>8);
				//We now have a 16.16 value for the current octave, so add that to d.
		}
		table[j] = (d * OctaveScale8[octaves]) >>16;
	}
	return 1;
}
*/
//	for(i = 0; i <= octaves; i++){
//		x1[i] = (int)x;
//		if(x < 0.0) x1[i]--;	//This little check should allow negative x,y values passed in to Noise to work perfectly.
//		xfrac[i] = (int)((x - (double)x1) * (double)0x10000);
//		xfrac[i] = ((((xfrac[i] >>8) * (xfrac[i] >>8)) >>8) * ((0x30000) - (xfrac[i] * 2))) >>16;	//Add slope to values, rather than linearly interpolating.
//		xr[i] = xrpt;
//		if(xrpt){	//Internal repetition added as an option.
//			if(x1[i] < 0)  x1[i] = (xrpt - 1) + ((x1[i] + 1) % xrpt);
//			else  x1[i] = x1[i] % xrpt;
//			x1p1[i] = x1[i] + 1;
//			if(x1p1 >= xrpt) x1p1 -= xrpt;
//			xrpt *= 2;
//		}else x1p1 = x1 + 1;
//		y1 = (int)y;
//		if(y < 0.0) y1--;
//		yfrac = y - (double)y1;
//		yfrac = (yfrac * yfrac) * (3.0 - (yfrac * 2.0));
//		if(yrpt){
//			if(y1 < 0)  y1 = (yrpt - 1) + ((y1 + 1) % yrpt);
//			else  y1 = y1 % yrpt;
//			y1p1 = y1 + 1;
//			if(y1p1 >= yrpt) y1p1 -= yrpt;
//			yrpt *= 2;
//		}else y1p1 = y1 + 1;
//		d += (((DRANDXY(x1, y1) * (1.0 - xfrac) +
//			DRANDXY((x1p1), y1) * xfrac) * (1.0 - yfrac) +
//			(DRANDXY(x1, (y1p1)) * (1.0 - xfrac) +
//			DRANDXY((x1p1), (y1p1)) * xfrac) * yfrac)/* * ONEDIV256*/) * OctaveMult[i];
//		x = (x + 7.0) * 2.0;
//		y = (y + 7.0) * 2.0;
//	}
//	return d * OctaveScale[octaves];
//}

//Gain and Bias functions.
double Bias(double b, double v){
	//return pow(v, log(b) / log(0.5));
	//That was Perlin's, now let's try Schlick's.
	return v / ((1 / b - 2.0) * (1.0 - v) + 1.0);
}
double Gain(double g, double v){
	if(v < 0.5){
		return Bias(1.0 - g, 2.0 * v) * 0.5;
	}else{
		return 1.0 - Bias(1.0 - g, 2.0 - 2.0 * v) * 0.5;
	}
}


//Spline functions.

//Catmull-Rom spline matrix constants.
#define CR00	-0.5f
#define CR01	1.5f
#define CR02	-1.5f
#define CR03	0.5f
#define CR10	1.0f
#define CR11	-2.5f
#define CR12	2.0f
#define CR13	-0.5f
#define CR20	-0.5f
#define CR21	0.0f
#define CR22	0.5f
#define CR23	0.0f
#define CR30	0.0f
#define CR31	1.0f
#define CR32	0.0f
#define CR33	0.0f

//Based on Catmull-Rom source code in _Texturing and Modeling: A Procedural Approach_.
//T is float from 0.0 to 1.0 specifying position between k1 and k2.
inline float Spline1d(float T, float k0, float k1, float k2, float k3){
	float c0, c1, c2, c3;
	c3 = CR00 * k0 + CR01 * k1 + CR02 * k2 + CR03 * k3;
	c2 = CR10 * k0 + CR11 * k1 + CR12 * k2 + CR13 * k3;
	c1 = CR20 * k0 /*+ CR21 * k1*/ + CR22 * k2 /*+ CR23 * k3*/;	//Zero multiplies hand optimized out.
	c0 = /*CR30 * k0 +*/ CR31 * k1 /*+ CR32 * k2 + CR33 * k3*/;
	return ((c3 * T + c2) * T + c1) * T + c0;
}

inline float Spline1d(float T, float knot[], int nknot){
	if(nknot >= 4 && knot){
		int span = (int)T;
		int nspans = nknot - 3;
		if(span > nspans) span = nspans;
		if(span < 0) span = 0;
		T = T - (float)span;
		return Spline1d(T, knot[span], knot[span + 1], knot[span + 2], knot[span + 3]);
	}
	return 0.0f;
}

inline void Spline2d(float T, float *dx, float *dy,
					  float k0x, float k0y, float k1x, float k1y,
					  float k2x, float k2y, float k3x, float k3y){
	if(dx && dy){
		*dx = Spline1d(T, k0x, k1x, k2x, k3x);
		*dy = Spline1d(T, k0y, k1y, k2y, k3y);
	}
}

inline void Spline2d(float T, float *dx, float *dy, float knot[], int nknot){
	if(nknot >= 4 && knot){
		int span = (int)T;
		int nspans = nknot - 3;
		if(span > nspans) span = nspans;
		if(span < 0) span = 0;
		T = T - (float)span;
		span = span <<1;
		*dx = Spline1d(T, knot[span], knot[span + 2], knot[span + 4], knot[span + 6]);
		*dy = Spline1d(T, knot[span + 1], knot[span + 3], knot[span + 5], knot[span + 7]);
	}
}

//Hermite curve matrix constants.
#define HM00	2.0f
#define HM01	-2.0f
#define HM02	1.0f
#define HM03	1.0f
#define HM10	-3.0f
#define HM11	3.0f
#define HM12	-2.0f
#define HM13	-1.0f
#define HM20	0.0f
#define HM21	0.0f
#define HM22	1.0f
#define HM23	0.0f
#define HM30	1.0f
#define HM31	0.0f
#define HM32	0.0f
#define HM33	0.0f

//Based on code in _Computer Graphics: Principles and Practice_.
inline float Hermite1d(float T, float k0, float k1, float k2, float k3){
	float c0, c1, c2, c3;
	c3 = HM00 * k0 + HM01 * k1 + HM02 * k2 + HM03 * k3;
	c2 = HM10 * k0 + HM11 * k1 + HM12 * k2 + HM13 * k3;
	c1 = /*HM20 * k0 + HM21 * k1 +*/ HM22 * k2 /*+ HM23 * k3*/;	//Zero multiplies hand optimized out.
	c0 = HM30 * k0 /*+ HM31 * k1 + HM32 * k2 + HM33 * k3*/;
	return ((c3 * T + c2) * T + c1) * T + c0;
}
