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


#ifndef CAMRBD_H
#define CAMRBD_H

#include "Image.h"
//#include "Poly.h"
#include "pi.h"
#include "Vector.h"

class Camera{	//This class describes the camera to the render engine.
public:
	float x, y, z;
	float a, b, c;
	float pitch, roll;	//These are slope values, to fake pitch and roll in a voxel engine.
	int viewplane;
	int viewwidth, viewheight;
	int farplane;
	void SetAll(float X, float Y, float Z, float A, float B, float C, float P, float R, int V);
	void SetAll(int X, int Y, int Z, int A, int B, int C, float P, float R, int V);
	void SetView(int w, int h, int f){
		viewwidth = w;
		viewheight = h;
		farplane = f;
	};
	int operator==(Camera &cam){
		if(x == cam.x && y == cam.y && z == cam.z){
			if(a == cam.a && b == cam.b && c == cam.c){
				if(pitch == cam.pitch && roll == cam.roll && viewplane == cam.viewplane){
					if(viewwidth == cam.viewwidth && viewheight == cam.viewheight){
						if(farplane == cam.farplane){
							return 1;
						}
					}
				}
			}
		}
		return 0;
	};
	int operator!=(Camera &cam){
		return !operator==(cam);
	};
	void LerpTo(Camera *cam, float t){
		if(cam){
			x = LERP(x, cam->x, t);
			y = LERP(y, cam->y, t);
			z = LERP(z, cam->z, t);
			pitch = LERP(pitch, cam->pitch, t);
			roll = LERP(roll, cam->roll, t);
			float d;
			d = NormRot((cam->a - a) * DEG2RAD) * RAD2DEG;
			a += d * t;
			a = NormRot(a * DEG2RAD) * RAD2DEG;
			//
			d = NormRot((cam->b - b) * DEG2RAD) * RAD2DEG;
			b += d * t;
			b = NormRot(b * DEG2RAD) * RAD2DEG;
			//
			d = NormRot((cam->c - c) * DEG2RAD) * RAD2DEG;
			c += d * t;
			c = NormRot(c * DEG2RAD) * RAD2DEG;
		}
	}
};

#endif
