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

//Noise and random functions, and procedural texture generation.
//Moved into Class form to make thread-safe, no more statics/globals.
//Also includes global Catmull-Rom Spline functions.

#ifndef BASIS_H
#define BASIS_H

#define MAXOCTAVE 6
#define MAXNTH 10
#define TABSIZE 256

struct FeaturePoint{
	float f;	//Distance, sorted by.
	unsigned long i;	//Non-unique ID, 0-255.
};

class Basis{
public:
//	double NthDist[MAXNTH];	//Array of distances for cellular return.
//	int NthAss[MAXNTH];
	FeaturePoint NthFeature[MAXNTH];
private:
	float NoiseFloatTable[TABSIZE * 2];	//0.0 - 1.0
	float NoiseNegTable[TABSIZE * 2];		//-0.5 - 0.5
	unsigned char NoisePermTable[TABSIZE];
	unsigned char NoiseCharTable[TABSIZE];
public:
	Basis(){};
	Basis(int seed){ Seed(seed); };
	~Basis(){};
	void Seed(int seed);
	//Seed must be called before Noise or Cellular!
	//One Seed call sets seed pattern for all subsequent Noise calls, until seed changed again.
	//Zero is an invalid seed and will be changed to 1.

	FeaturePoint *Cellular(float x, float y, int nth = 1, float irreg = 1.0, int xrpt = 0, int yrpt = 0, int manhattan = 0);
	//returns a pointer to an array of MAXNTH FeaturePoints, the first being the
	//first closest feature point distance, the next being the second closest
	//distance, etc.  Values in the array are x*x + y*y, un-SQRTed!  The int part
	//of feature point is a 0-255 value unique to that point, suitable for coloring
	//individual cells different shades.

	float Noise(float x, float y, int octaves = 0, int xrpt = 0, int yrpt = 0);
//	int Noise(unsigned char *table, int length, float x, float y, float xdelta, float ydelta,
//		int octaves = 0, int xrpt = 0, int yrpt = 0);
};

double Bias(double b, double v);
double Gain(double g, double v);

float Spline1d(float T, float k0, float k1, float k2, float k3);
//Simple 1 dimensional Catmull-Rom, T is 0.0 to 1.0 and interpolates between k1 and k2.
//k0 and k3 are the external control points before and behind the spline span.
float Spline1d(float T, float knot[], int nknot);
//Arbitrary knot vector version of above.  T of 0.0 is the second knot (index 1),
//again.  To make a closed loop spline of n control points, put your points in
//knots 1 to n, then put point n - 1 in knot 0, and point 0 in knot n + 1, for
//a total of n + 2 knots passed in.
inline void Spline2d(float T, float *dx, float *dy,
					  float k0x, float k0y, float k1x, float k1y,
					  float k2x, float k2y, float k3x, float k3y);
//2 dimensional version of above.  Coord is returned to dx, dy pointers.
inline void Spline2d(float T, float *dx, float *dy, float knot[], int nknot);
//Ditto.  Same.  knot is array of x, y float pairs.

inline float Hermite1d(float T, float p0, float p1, float g0, float g1);
//Interpolates point 0 and point 1, using geometry vectors g0 and g1 to
//decide how "fast" and in what direction the curve is moving at the two points.

#endif
