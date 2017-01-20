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

//Definitions of OpenGL-based add-on members for Terrain and RenderEngine classes.
//By Seumas McNally, 1998.
//
//Note, this is probably NOT THREAD SAFE!  (Don't tell two objects to render at
//the very same time, but then that would be silly.)
//
//There are also hard limits, such as 2048x2048 maps (at 26 megs a piece!!!),
//and LOD patches are compile-time set to 16 voxels square.
//

#pragma inline_depth(255)

#include "GLTerrainRender3.h"
#include "GLTerrain.h"
#include "../GenericBuffer.h"

#include "../Terrain.h"
#include "../Render.h"
#include "../Poly.h"
#include "../CamRBD.h"
#include "../Image.h"
#include "../ResourceManager.h"
#include "../CfgParse.h"
#include "../Vector.h"
#include "../macros.h"

#include <GL/glew.h>

void GLRenderEngine3::GLViewplane(float w, float h, float view, float n, float f){
	float x = n * ((w / 2.0f) / view);
	float y = n * ((h / 2.0f) / view);
	glFrustum(-x, x, -y, y, n, f + 1.0f);
}

//************************************************************************************
//								Actual Terrain Rendering starts here!
//************************************************************************************

GLRenderEngine3::GLRenderEngine3()
{
	PolyCount = 0;

	nFanStack = 0;
	FanCount = 0;
	UseMT = 0;

	TerrainFrame = 0;

	CurQuality = 0.1f;
	CurBinTris = 1;

	patches = 0;
	npatches = 0;
}

inline int BinaryTriangleR3::Split2(BinTriPool *pool)
{//float propoh){	//Note, leaves, LeftChild->RightNeighbor and RightChild->LeftNeighbor hanging!
	if(pool->AvailBinTris() >= 2){
		LeftChild = pool->AllocBinTri();
		LeftChild->SplitVertHeight = 0.5f;	//Pass on T value for vert morph.
		RightChild = pool->AllocBinTri();
		RightChild->SplitVertHeight = 0.5f;
		LeftChild->LeftNeighbor = RightChild;
		RightChild->RightNeighbor = LeftChild;	//Link kids to each other.
		LeftChild->BottomNeighbor = LeftNeighbor;
		if(LeftNeighbor){
			if(LeftNeighbor->BottomNeighbor == this) LeftNeighbor->BottomNeighbor = LeftChild;
			else if(LeftNeighbor->LeftNeighbor == this) LeftNeighbor->LeftNeighbor = LeftChild;
			else LeftNeighbor->RightNeighbor = LeftChild;
		}
		RightChild->BottomNeighbor = RightNeighbor;
		if(RightNeighbor){
			if(RightNeighbor->BottomNeighbor == this) RightNeighbor->BottomNeighbor = RightChild;
			else if(RightNeighbor->RightNeighbor == this) RightNeighbor->RightNeighbor = RightChild;
			else RightNeighbor->LeftNeighbor = RightChild;
		}
		LeftChild->LeftChild = LeftChild->RightChild = 0;
		RightChild->LeftChild = RightChild->RightChild = 0;	//Null childs' child pointers.
		return 1;
	}
	return 0;
};
inline void BinaryTriangleR3::Split(BinTriPool *pool)
{
	if(BottomNeighbor == NULL){	//No bottom, so split half-diamond.
		if(Split2(pool)){
			LeftChild->RightNeighbor = RightChild->LeftNeighbor = 0;	//Null hanging pointers.
		}
	}else{
		if(BottomNeighbor->BottomNeighbor != this){	//Not diamond, so split bottom neighbor.
			BottomNeighbor->SplitVertHeight = SplitVertHeight;
			BottomNeighbor->Split(pool);
		}
		//Now BottomNeighbor->BottomNeighbor _should_ == this...
		if(BottomNeighbor->BottomNeighbor == this && pool->AvailBinTris() >= 4){
			Split2(pool);
			BottomNeighbor->Split2(pool);
			//
			LeftChild->RightNeighbor = BottomNeighbor->RightChild;
			RightChild->LeftNeighbor = BottomNeighbor->LeftChild;
			BottomNeighbor->LeftChild->RightNeighbor = RightChild;	//Link kids with each other.
			BottomNeighbor->RightChild->LeftNeighbor = LeftChild;
		}
	}
};

//Splits binary tri only if variance greater than parameter.
void BinaryTriangleR3::TestSplit(LodTree *lod, int variance, int LimitLod, int level, int index, BinTriPool *pool)
{
	if(lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index] > variance){
		if(LeftChild == 0) Split(pool);	//Don't split if already split!
		if(level < (MaxLodDepth - 1) - LimitLod){
			int index2 = index;
			if(level < LodTreeDepth - 1){
				index = index <<1;
				index2 = index + 1;
			}
			level++;
			if(LeftChild) LeftChild->TestSplit(lod, variance, LimitLod, level, index, pool);
			if(RightChild) RightChild->TestSplit(lod, variance, LimitLod, level, index2, pool);
		}
	}
};

//NOTE: This function, normally, ROUNDS TO NEAREST!!!
inline long FloatToLong(float f) { // relies on IEEE format for float
	f += -0.499999f;	//This change makes it CHOP TO LOWER!!
	f += (3 << 22);
	return ((*(long*)&f)&0x007fffff) - 0x00400000;
}

void BinaryTriangleR3::TestSplitZ(int level, int LimitLod, int index, float cz1, float cz2, float cz3, LodTree *lod, BinTriPool *pool)
{
	float avgcz = (cz1 + cz2) * 0.5f;
	//CameraZs will be pre-multiplied by variance now.
	int iv = FloatToLong(avgcz);
	int vd = lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index];// - iv;
	//Brute force method first...
	if(vd > iv){
		//
		if(vd - iv > 2) SplitVertHeight = 1.0f;
		else SplitVertHeight = (((float)vd - avgcz)) * 0.5f;
		//
		if(LeftChild == 0 && pool->ElectiveSplitSafe()) Split(pool);	//Don't split if already split!
		if(level < (MaxLodDepth - 1) - LimitLod){
			int index2 = index;
			if(level < LodTreeDepth - 1){
				index = index <<1;
				index2 = index + 1;
			}
			level++;
			if(LeftChild) LeftChild->TestSplitZ(level, LimitLod, index, cz3, cz1, avgcz, lod, pool);
			if(RightChild) RightChild->TestSplitZ(level, LimitLod, index2, cz2, cz3, avgcz, lod, pool);
			return;
		}
	}else{
		SplitVertHeight = 0.0f;
	}
	if(LeftChild) LeftChild->SplitVertHeight = SplitVertHeight;
	if(RightChild) RightChild->SplitVertHeight = SplitVertHeight;
};

void BinaryTriangleR3::TestSplitClip(int level, float variance, int LimitLod, int index, float radius, int x1, int y1, int x2, int y2, int x3, int y3, float lclip[3], float rclip[3], float zclip[3], LodTree *lod, BinTriPool *pool)
{
	int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
	float t = (float)(avgx) * lclip[0] + (float)(-avgy) * lclip[1] - lclip[2];
	float t2 = (float)(avgx) * rclip[0] + (float)(-avgy) * rclip[1] - rclip[2];
	if(t < -radius || t2 < -radius){	//Clipped totally.
		//Okay, trying something...  Set neighbor pointers that reference this to null, so it won't be
		//force split at all.  This will BREAK if a concurrent tree is used...
		LeftChild = RightChild = NULL;	//In case we have already been force split some amount.
		if(BottomNeighbor){
			if(BottomNeighbor->BottomNeighbor == this) BottomNeighbor->BottomNeighbor = NULL;
			else if(BottomNeighbor->LeftNeighbor == this) BottomNeighbor->LeftNeighbor = NULL;
			else if(BottomNeighbor->RightNeighbor == this) BottomNeighbor->RightNeighbor = NULL;
			BottomNeighbor = NULL;
		}
		if(LeftNeighbor){
			if(LeftNeighbor->BottomNeighbor == this) LeftNeighbor->BottomNeighbor = NULL;
			else if(LeftNeighbor->LeftNeighbor == this) LeftNeighbor->LeftNeighbor = NULL;
			else if(LeftNeighbor->RightNeighbor == this) LeftNeighbor->RightNeighbor = NULL;
			LeftNeighbor = NULL;
		}
		if(RightNeighbor){
			if(RightNeighbor->BottomNeighbor == this) RightNeighbor->BottomNeighbor = NULL;
			else if(RightNeighbor->LeftNeighbor == this) RightNeighbor->LeftNeighbor = NULL;
			else if(RightNeighbor->RightNeighbor == this) RightNeighbor->RightNeighbor = NULL;
			RightNeighbor = NULL;
		}
		iheight = IHEIGHT_NORENDER;
		return;
	}
	if(t > radius && t2 > radius)
	{	//Unclipped totally.
		//Since we have to pass the result of three dot products into testsplitz just to find out that
		//this tri may not need to be split, do a normal, simpler split test first (basically the good bits of old testsplitz)
		//and only do dot products if split and need to test children.
		float avgcz = ((float)(avgx) * zclip[0] + (float)(-avgy) * zclip[1] - zclip[2]) * variance;
		int iv = FloatToLong(avgcz);
		int vd = lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index];// - iv;
		//
		if(vd > iv)
		{
			//
			if(vd - iv > 2)
				SplitVertHeight = 1.0f;
			else
				SplitVertHeight = (((float)vd - avgcz)) * 0.5f;
			//
			if(LeftChild == 0 && pool->ElectiveSplitSafe())
				Split(pool);
			if(level < (MaxLodDepth - 1) - LimitLod)
			{
				int index2 = index;
				if(level < LodTreeDepth - 1)
				{
					index = index <<1;
					index2 = index + 1;
				}
				level++;
				float t1 = ((float)(x1) * zclip[0] + (float)(-y1) * zclip[1] - zclip[2]) * variance;
				float t2 = ((float)(x2) * zclip[0] + (float)(-y2) * zclip[1] - zclip[2]) * variance;
				float t3 = ((float)(x3) * zclip[0] + (float)(-y3) * zclip[1] - zclip[2]) * variance;
				if(LeftChild)
					LeftChild->TestSplitZ(level, LimitLod, index, t3, t1, avgcz, lod, pool);
				if(RightChild)
					RightChild->TestSplitZ(level, LimitLod, index2, t2, t3, avgcz, lod, pool);
				return;
			}
		}
		else
		{
			SplitVertHeight = 0.0f;
		}
		if(LeftChild)
			LeftChild->SplitVertHeight = SplitVertHeight;
		if(RightChild)
			RightChild->SplitVertHeight = SplitVertHeight;
		//
	}else{	//Halfway clipped.
		float t = (float)(avgx) * zclip[0] + (float)(-avgy) * zclip[1] - zclip[2];
		float v = ((float)variance * t);	//Pre-mulled now.
		int iv = FloatToLong(v);
		int vd = lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index];// - iv;
		if(vd > iv){
			//
			if(vd - iv > 2) SplitVertHeight = 1.0f;
			else SplitVertHeight = (((float)vd - v)) * 0.5f;
			//
			if(LeftChild == 0 && pool->ElectiveSplitSafe()) Split(pool);
			if(level < (MaxLodDepth - 1) - LimitLod){
				int index2 = index;
				if(level < LodTreeDepth - 1){
					index = index <<1;
					index2 = index + 1;
				}
				level++;
				radius *= 0.707107f;
				if(LeftChild) LeftChild->TestSplitClip(level, variance, LimitLod, index, radius, x3, y3, x1, y1, avgx, avgy, lclip, rclip, zclip, lod, pool);
				if(RightChild) RightChild->TestSplitClip(level, variance, LimitLod, index2, radius, x2, y2, x3, y3, avgx, avgy, lclip, rclip, zclip, lod, pool);
				return;
			}
		}else{
			SplitVertHeight = 0.0f;
		}
		if(LeftChild) LeftChild->SplitVertHeight = SplitVertHeight;
		if(RightChild) RightChild->SplitVertHeight = SplitVertHeight;
	}
};

//Outputs one terrain vertex at requested map x and y coords.
inline void GLRenderEngine3::BinTriVert(int x, int y){
	glTexCoord2f(((float)x - texoffx) * texscalex + texaddx, ((float)y - texoffy) * texscaley + texaddy);
	glVertex3f(float(x), ((float)curmap->GetHwrap(x, y) * 0.25f), float(y));
}

//Recursive function to render all binary triangles descending from a root triangle.
void GLRenderEngine3::RenderBinTri(BinaryTriangleR3 *btri, int x1, int y1, int x2, int y2, int x3, int y3){
	if(btri->LeftChild && btri->RightChild){
		int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
		RenderBinTri(btri->LeftChild, x3, y3, x1, y1, avgx, avgy);
		RenderBinTri(btri->RightChild, x2, y2, x3, y3, avgx, avgy);// <<1;
	}else{
		glBegin(GL_TRIANGLES);	//Really badly unoptimized triangles...
		BinTriVert(x1, y1);
		BinTriVert(x2, y2);
		BinTriVert(x3, y3);
		glEnd();
		PolyCount++;
	}
}
void CDECL FakeglTexCoord2f(float a, float b){
	a += b;
}
void CDECL FakeglVertex3f(float a, float b, float c){
	a += b - c;
}
inline void GLRenderEngine3::BinTriVert3(float x, float y, float h){
	glTexCoord2f((x - texoffx) * texscalex + texaddx, (y - texoffy) * texscaley + texaddy);
	glVertex3f(x, h, y);
}
inline void GLRenderEngine3::BinTriVert3MT(float x, float y, float h){
	glMultiTexCoord2f(GL_TEXTURE0, (x - texoffx) * texscalex + texaddx, (y - texoffy) * texscaley + texaddy);
	glMultiTexCoord2f(GL_TEXTURE1, (x - texoffx2) * texscalex2, (y - texoffy2) * texscaley2);
	glVertex3f(x, h, y);
}

inline void GLRenderEngine3::InitFanStack(){
	nFanStack = 0;
	FanCount = 0;
	PolyCount = 0;
}
inline void GLRenderEngine3::FlushFanStack(){
	if(nFanStack > 0){
		//
		glBegin(GL_TRIANGLE_FAN);
		//
		if(UseMT){
			for(int n = 0; n < nFanStack; n += 3){
				BinTriVert3MT(float(FanStack[n]), float(FanStack[n + 1]), FanStackf[n + 2]);
			}
		}else{
			for(int n = 0; n < nFanStack; n += 3){
				BinTriVert3(float(FanStack[n]), float(FanStack[n + 1]), FanStackf[n + 2]);
			}
		}
		//
		glEnd();
		//
		nFanStack = 0;
		FanCount++;
	}
}
inline void GLRenderEngine3::AddFanPoint(int x, int y, float h){
	FanStack[nFanStack] = x;
	FanStack[nFanStack + 1] = y;
	FanStackf[nFanStack + 2] = h;
	nFanStack += 3;
}
inline void GLRenderEngine3::AddFanPoint(int x1, int y1, float h1, int x2, int y2, float h2, int x3, int y3, float h3){
	FanStack[nFanStack] = x1;
	FanStack[nFanStack + 1] = y1;
	FanStackf[nFanStack + 2] = h1;
	FanStack[nFanStack + 3] = x2;
	FanStack[nFanStack + 4] = y2;
	FanStackf[nFanStack + 5] = h2;
	FanStack[nFanStack + 6] = x3;
	FanStack[nFanStack + 7] = y3;
	FanStackf[nFanStack + 8] = h3;
	nFanStack += 9;
}

void GLRenderEngine3::RenderBinTriFan(BinaryTriangleR3 *btri, int x1, int y1, int x2, int y2, int x3, int y3, int sense, float h1, float h2, float h3){
	if(btri->LeftChild && btri->RightChild){
		int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
		float th;
		if(btri->iheight){
			th = btri->height;
		}else{
			if(btri->BottomNeighbor){
				float t = (btri->SplitVertHeight + btri->BottomNeighbor->SplitVertHeight) * 0.25f;
				th = (h1 + h2) * (0.5 - t) + (float)(curmap->GetHwrap(avgx, avgy)) * 0.5f * t;
				btri->height = btri->BottomNeighbor->height = th;
			}else{
				float t = btri->SplitVertHeight * 0.5f;
				th = (h1 + h2) * (0.5 - t) + (float)(curmap->GetHwrap(avgx, avgy)) * 0.5f * t;
				btri->height = th;
			}
		}
		if(sense){
			RenderBinTriFan(btri->RightChild, x2, y2, x3, y3, avgx, avgy, 0, h2, h3, th);	//Right/Left.
			RenderBinTriFan(btri->LeftChild, x3, y3, x1, y1, avgx, avgy, 0, h3, h1, th);
		}else{
			RenderBinTriFan(btri->LeftChild, x3, y3, x1, y1, avgx, avgy, 1, h3, h1, th);	//Left/Right.
			RenderBinTriFan(btri->RightChild, x2, y2, x3, y3, avgx, avgy, 1, h2, h3, th);
		}
	}else{
		//
		if(btri->iheight == IHEIGHT_NORENDER) return;
		//
		PolyCount++;
		if(nFanStack > 0){
#define HASH(x, y) (((x) <<16) ^ (y))
			register int fanfoo = HASH(FanStack[0], FanStack[1]);//(FanStack[0] <<16) ^ FanStack[1];
			register int fanfoo2 = HASH(FanStack[nFanStack - 3], FanStack[nFanStack - 2]);//(FanStack[nFanStack - 2] <<16) ^ FanStack[nFanStack - 1];
			if(((fanfoo - HASH(x1, y1)) | (fanfoo2 - HASH(x2, y2))) == 0){
				AddFanPoint(x3, y3, h3);
				return;
			}
			if(((fanfoo - HASH(x2, y2)) | (fanfoo2 - HASH(x3, y3))) == 0){
				AddFanPoint(x1, y1, h1);
				return;
			}
			if(((fanfoo - HASH(x3, y3)) | (fanfoo2 - HASH(x1, y1))) == 0){
				AddFanPoint(x2, y2, h2);
				return;
			}
			FlushFanStack();	//Not empty, but no fan connect.
		}
		if(sense) AddFanPoint(x1, y1, h1, x2, y2, h2, x3, y3, h3);
		else AddFanPoint(x3, y3, h3, x1, y1, h1, x2, y2, h2);
	}
}

bool GLRenderEngine3::GLTerrainRender(Terrain *map, Camera *cam, int flags, float quality, int ms){
	//
	//
	if((flags & GLREND_LOCKPATCHES) && (patches == 0 || npatches <= 0)){
		//Disable lockpatches for one run here so patches can be created.
		flags = flags & (~GLREND_LOCKPATCHES);
	}
	//
	//Align bintripool on 32byte boundary
	//
	msecs += ms;
	//
	if(!map || !cam || map->Width() <= 0) return false;
	curmap = map;
	float viewdist = cam->farplane;	//This is now stored in the camera object.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLViewplane(cam->viewwidth, cam->viewheight, cam->viewplane, 2.0, cam->farplane);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	if((flags & GLREND_NOSKY)){
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
	}else{
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	//
	//Draw the sky plane.  Simple ugly 2D plane that will break with shearing as
	//currently coded...  Could be easily replaced with real SkyBox for serious
	//6-DOF displays.
	if(!(flags & GLREND_NOSKY)){// && Sky && Sky->id != 0){
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_FOG);
	//	glDepthFunc(GL_ALWAYS);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glFrontFace(GL_CW);
		if(UseSkyBox == false){
			if(Sky){
				glBindTexture(GL_TEXTURE_2D, Sky->id);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_DECAL);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				float SkyPitchBias;
			//	SkyPitchBias = 10.0f;
				SkyPitchBias = std::max(-30.0f, RAD2DEG * -atan2f(cam->y, viewdist * 2.0f));
				float boxw = cam->viewwidth;
				float boxh = cam->viewheight;
				float xscale = atan((float)(boxw / 2) / cam->viewplane) * 2.0f;	//Radians per skyplane screen.
				float yscale = atan((float)(boxh / 2) / cam->viewplane);// * 2.0f;
				//Now make x radians per skyplane an even multiple of pi2.  Remember we must use two skyplanes at a time with tiling...
				xscale = (PI2 / (float)((int)(PI2 / (xscale * 2.0f) + 0.5f))) * 0.5f;
				boxw = tan(xscale * 0.5f) * 2.0f * cam->viewplane;
				//Don't bother with Y, as we won't be pitching enough for it to matter.
				float xoff = fmodf(-cam->b * DEG2RAD, xscale) / xscale;	//Fraction into skybox.
				float xdiv = (-cam->b * DEG2RAD) / xscale;	//Which skybox are we in.
				if(xoff < 0.0f) xoff += 1.0f;	//0.0 - 1.0 only, so add to get correct positive range.
				if(xdiv < 0.0f) xdiv -= 1.0f;	//any value, and chopped, so subtract to get proper floor with chop.
				float yoff = fmodf(-(cam->a + SkyPitchBias) * DEG2RAD, yscale) / yscale;
				float ydiv = (-(cam->a + SkyPitchBias) * DEG2RAD) / yscale;
				if(yoff < 0.0f) yoff += 1.0f;
				if(ydiv < 0.0f) ydiv -= 1.0f;
				float xvw = (boxw / 2.0f) * (viewdist / cam->viewplane);	//Size of skybox in GL space.
				float yvw = (boxh / 4.0f) * (viewdist / cam->viewplane);
				float zvw = (-cam->viewplane) * (viewdist / cam->viewplane);
				float xf, yf;
				float fx, fy;
				//
				float halfpixel = (1.0f / (float)Sky->Width()) / 2.0f;
				//
				glBegin(GL_QUADS);
				glColor4f(1, 1, 1, 1);
				for(int y = 0; y < 4; y++){
					for(int x = 0; x < 3; x++){
						xf = -xvw * 3.0f + x * xvw * 2.0f + xoff * xvw * 2.0f;
						yf = yvw * 4.0f + (-y * yvw * 2.0f) + (-yoff * yvw * 2.0f);
						fx = (((int)xdiv & 1) ^ (x & 1)) ? 1.0f - halfpixel : halfpixel;
						fy = (((int)ydiv & 1) ^ (y & 1)) ? halfpixel : 1.0f - halfpixel;
						glTexCoord2f(fx, fy);
						glVertex3f(xf , yf, zvw);
						glTexCoord2f(1.0f - fx, fy);
						glVertex3f(xf + xvw * 2.0f, yf, zvw);
						glTexCoord2f(1.0f - fx, 1.0f - fy);
						glVertex3f(xf + xvw * 2.0f, yf - yvw * 2.0f, zvw);
						glTexCoord2f(fx, 1.0f - fy);
						glVertex3f(xf, yf - yvw * 2.0f, zvw);
					}
				}
				glEnd();
			}
		}else{	//Render sky using a SKY BOX!!!!!
			float vcoords[6][4][3] = {
				{ {-10, 10, -10}, {10, 10, -10}, {10, 0, -10}, {-10, 0, -10} },
				{ {10, 10, -10}, {10, 10, 10}, {10, 0, 10}, {10, 0, -10} },
				{ {10, 10, 10}, {-10, 10, 10}, {-10, 0, 10}, {10, 0, 10} },
				{ {-10, 10, 10}, {-10, 10, -10}, {-10, 0, -10}, {-10, 0, 10} },
				{ {-10, 10, 10}, {10, 10, 10}, {10, 10, -10}, {-10, 10, -10} },
				{ {-10, 10, 10}, {10, 10, 10}, {10, 10, -10}, {-10, 10, -10} } };
			//
			float camb = cam->b;
			if(skyrotatespeed != 0.0f){
				camb += (fmod(((double)msecs / 1000.0) * (double)skyrotatespeed, 360.0));
			}
			//
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(cam->a, 1.0f, 0.0f, 0.0f);	//Apply camera rotation only.
			glRotatef(camb, 0.0f, 1.0f, 0.0f);
			glColor4f(1, 1, 1, 1);
		//	float zero = 1.0f / 512.0f, one = 1.0f - zero;
			float xzero, xone, yzero, yone;
		//	if(SkyBox[0]){
		//		zero = 1.0f / (float)
			//SkyBox culling.
			float needang = (atan2f(cam->viewwidth / 2.0f, cam->viewplane) + PI / 4.0f);// / 2.0f;
			//Skybox plane extends 45' (quarter PI rads) from skybox plane center, plus half of viewangle.
			float camang = DEG2RAD * camb;
			//Fix clipping on pitch changes.
			needang += fabsf(cam->a) * 0.5f;
			//
			for(int i = 0; i < 6; i++){
				if(SkyBox[i] && SkyBox[i]->id && (i >= 4 || fabsf(NormRot((PI / 2.0f * (float)i) - camang)) < needang)){
					glBindTexture(GL_TEXTURE_2D, SkyBox[i]->id);
					//
					//Use offsets based on actual size of texture reported by driver.
					int w = 1, h = 1;
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
				//	zero = 1.0f / std::max(1.0f, (float)w * 0.999f);	//Little bigger "zero" to avoid seams.
				//	one = 1.0f - zero;
					xzero = 1.0f / std::max(1.0f, (float)w * 1.99f);	//Little bigger "zero" to avoid seams.
					yzero = 1.0f / std::max(1.0f, (float)h * 1.99f);	//Little bigger "zero" to avoid seams.
					xone = 1.0f - xzero;
					yone = 1.0f - yzero;
					//
					glBegin(GL_QUADS);
					for(int n = 0; n < (i < 4 ? 2 : 1); n++){
						float s = (n == 0 ? 1.0f : -1.0f);
						glTexCoord2f(xzero, yzero);
						glVertex3f(vcoords[i][0][0], vcoords[i][0][1] * s, vcoords[i][0][2]);
						glTexCoord2f(xone, yzero);
						glVertex3f(vcoords[i][1][0], vcoords[i][1][1] * s, vcoords[i][1][2]);
						glTexCoord2f(xone, yone);
						glVertex3f(vcoords[i][2][0], vcoords[i][2][1] * s, vcoords[i][2][2]);
						glTexCoord2f(xzero, yone);
						glVertex3f(vcoords[i][3][0], vcoords[i][3][1] * s, vcoords[i][3][2]);
					}
					glEnd();
				}
			}
		}
	//	glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	}
	//
//	GLfloat fogcol[4] = {0.7f, 0.8f, 0.9f, 0.0f};
	if(!(flags & GLREND_NOFOG)){
		GLfloat fogcol[4] = {FogR, FogG, FogB, 1.0f};//0.0f};
		glFogi(GL_FOG_MODE, GL_LINEAR);
	//	glFogi(GL_FOG_MODE, GL_EXP2);
		glFogf(GL_FOG_START, viewdist - viewdist / 3.0f);
		glFogf(GL_FOG_END, viewdist);
	//	glFogf(GL_FOG_DENSITY, 2.0f / viewdist);// / viewdist);
		glFogfv(GL_FOG_COLOR, fogcol);
		glEnable(GL_FOG);
	}else{
		glDisable(GL_FOG);
	}
	//
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	//
	glEnable(GL_CULL_FACE);
	//
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	//
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(cam->a, 1.0f, 0.0f, 0.0f);
	glRotatef(cam->b, 0.0f, 1.0f, 0.0f);
	glTranslatef(-cam->x, -cam->y, cam->z);	//Add z, coord space flip.
	//
	glEnable(GL_TEXTURE_2D);
	//
	if(flags & GLREND_WIREFRAME){
		glPolygonMode(GL_FRONT, GL_LINE);
	}else{
		glPolygonMode(GL_FRONT, GL_FILL);
	}
	//
	int TexPerMapW = map->Width() / TexSize;
	int TexPerMapH = map->Height() / TexSize;
	//
	float tv[2];
    float bv[2];
    bv[0] = sin(-cam->b * DEG2RAD);
    bv[1] = cos(-cam->b * DEG2RAD);
	float s;
	//
	float tcos = cos(cam->a * DEG2RAD);
	tv[0] = -cam->viewwidth / (2.0f * SQUARE(tcos));
	tv[1] = cam->viewplane * SQUARE(tcos);
	//Adjustment to loosen clipping when camera pitches down ur up too much.
	//
	s = sqrtf(tv[0] * tv[0] + tv[1] * tv[1]);
	lclip[0] = ((bv[0] * tv[0]) / s + (bv[1] * tv[1]) / s);	//90 deg right
	lclip[1] = -((bv[1] * tv[0]) / s + (-bv[0] * tv[1]) / s);
	lclip[2] = lclip[0] * cam->x + lclip[1] * cam->z;	//Distance of plane.
	//
	rclip[0] = -((bv[0] * -tv[0]) / s + (bv[1] * tv[1]) / s);	//90 deg left
	rclip[1] = ((bv[1] * -tv[0]) / s + (-bv[0] * tv[1]) / s);
	rclip[2] = rclip[0] * cam->x + rclip[1] * cam->z;	//Distance of plane.
	//
	zclip[0] = sin(cam->b * DEG2RAD);
	zclip[1] = cos(cam->b * DEG2RAD);
	zclip[2] = zclip[0] * cam->x + zclip[1] * cam->z;	//Distance of plane.
	if((flags & GLREND_LOCKPATCHES) == 0){
		Pool.ResetBinTriPool();
	}
	//
	int x, y, x1, y1, n = 0;
	//
	gltmr.Start();
	//
	int xs = floor(cam->x / (float)TexSize), ys = floor(-cam->z / (float)TexSize);
	int psize = viewdist / TexSize + 1;
	if((flags & GLREND_LOCKPATCHES) == 0){
		npatches = SQUARE(psize * 2 + 1);
		//
	//	MapPatch *
		if(patches) delete [] patches;
		patches = new MapPatch[npatches];//TexPerMapW * TexPerMapH * 9];
	}
	if(!patches) return false;
	//
	n = 0;
	if((flags & GLREND_LOCKPATCHES) == 0){
		float patchrad = sqrtf((float)SQUARE(TexSize / 2)) * 1.4f;
		for(y = ys - psize; y <= ys + psize; y++){	//Create and link list of patches.
			for(x = xs - psize; x <= xs + psize; x++){
				float xf = (float)(x * TexSize + (TexSize >>1));
				float yf = (float)(y * TexSize + (TexSize >>1));
				float t = (float)(xf) * lclip[0] + (float)(-yf) * lclip[1] - lclip[2];
				float t2 = (float)(xf) * rclip[0] + (float)(-yf) * rclip[1] - rclip[2];
				if(t > -patchrad && t2 > -patchrad){
					patches[n].SetCoords(x, y, map->TexIDs[x & (TexPerMapW - 1)][y & (TexPerMapH - 1)]);
					if(x > xs - psize && patches[n - 1].id != 0) patches[n].LinkLeft(&patches[n - 1]);
					if(y > ys - psize && patches[n - ((psize <<1) + 1)].id != 0) patches[n].LinkUp(&patches[n - ((psize <<1) + 1)]);
				}
				n++;
			}
		}
	}
	//
	float InputQuality = quality;
	int triwant = (float)BINTRIMAX * quality;
	float TQuality = CurQuality * (((float)triwant / (float)BINTRIMAX) / ((float)CurBinTris / (float)BINTRIMAX));
	//Trying triangle-count pegging scheme.
	float DQuality = (TQuality - CurQuality) * 0.05f;	//Filter changes a little.
	CurQuality += std::min(std::max(DQuality, -0.1f), 0.05f);
	CurQuality = std::min(std::max(0.1f, CurQuality), 1.0f);
	quality = CurQuality;
	//
	float Variance = SQUARE(1.0f - quality) * 16.0f + 1.0f;
	int LimitLod = std::max(0.0f, (1.0f - InputQuality) * 4.0f);	//Fix limit lod based on specified virtual quality, to reduce lod oscillations.
	//
	if((flags & GLREND_LOCKPATCHES) == 0){
		//Dive and create variable lod levels here.
		for(n = 0; n < npatches; n++){
			if(patches[n].id > 0){
				patches[n].Split(Variance, LimitLod, lclip, rclip, zclip, &Pool);
			}
		}
	}
	//
	int splittime = gltmr.Check(1000);
	//
	gltmr.Start();
	//
	texscalex = 1.0f / (float)(TexSize + 1);
	texscaley = 1.0f / (float)(TexSize + 1);
	texaddx = texscalex * 0.5f;	//Bit to add to bring tex coords into proper internal-values for repeat mode, so border/wrap pixels are never touched.  Texels will be 0.5 to 127.5.
	texaddy = texscaley * 0.5f;
	//
	InitFanStack();
	//
	float detailrotx = sin(((double)msecs / 1000.0) * detailrotspeed * (double)DEG2RAD) * detailrotrad;
	float detailroty = cos(((double)msecs / 1000.0) * detailrotspeed * (double)DEG2RAD) * detailrotrad;
	//
	UseMT = 0;	//Flag for final drawing func.
	if(!(flags & GLREND_NODETAIL) && GLEW_ARB_multitexture && !(flags & GLREND_NOMT)){
		//Do multi-textured rendering.
		UseMT = 1;
		texscalex2 = 1.0f / 16.0f;
		texscaley2 = 1.0f / 16.0f;
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		if(DetailMT == 0 || DetailMT->id == 0){
			OutputDebugLog("Detail texture not loaded.\n");
		}else{
			glBindTexture(GL_TEXTURE_2D, DetailMT->id);
		}
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_DECAL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_DECAL);
		glColor4f(1, 1, 1, 1);
		for(n = 0; n < npatches; n++){
			if(patches[n].id > 0){
				if(!map->Redownload(patches[n].x, patches[n].y))
					glBindTexture(GL_TEXTURE_2D, patches[n].id);
				x1 = texoffx = texoffx2 = patches[n].x * TexSize;
				y1 = texoffy = texoffy2 = patches[n].y * TexSize;
				texoffx2 += detailrotx;
				texoffy2 += detailroty;
				RenderBinTriFan(&patches[n].ul, x1, y1 + TexSize, x1 + TexSize, y1, x1, y1, 1,
					(float)map->GetHwrap(x1, y1 + TexSize) * 0.25f,
					(float)map->GetHwrap(x1 + TexSize, y1) * 0.25f,
					(float)map->GetHwrap(x1, y1) * 0.25f);
				FlushFanStack();
				RenderBinTriFan(&patches[n].dr, x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize, 1,
					(float)map->GetHwrap(x1 + TexSize, y1) * 0.25f,
					(float)map->GetHwrap(x1, y1 + TexSize) * 0.25f,
					(float)map->GetHwrap(x1 + TexSize, y1 + TexSize) * 0.25f);
				FlushFanStack();
			}
		}
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}else{
		//
	for(int mt = 0; mt < ((flags & GLREND_NODETAIL) ? 1 : 2) ; mt++){
		if(mt == 1){
			if(Detail == 0 || Detail->id == 0){
				OutputDebugLog("Detail texture not loaded.\n");
				break;
			}
			texaddx = texaddy = 0.0f;
			texscalex = 1.0f / 16.0f;
			texscaley = 1.0f / 16.0f;
			glBindTexture(GL_TEXTURE_2D, Detail->id);
			glEnable(GL_BLEND);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_DECAL);
		//	glBlendFunc(GL_SRC_ALPHA, GL_ONE);//GL_ONE_MINUS_SRC_ALPHA);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glColor4f(0.0, 0.0, 0.0, 0.25);
		//	glClear(GL_DEPTH_BUFFER_BIT);
			glDepthMask(GL_FALSE);
			glDepthFunc(GL_LEQUAL);
			//Push near plane up a smidge to bias depth values towards viewer.
		//	glMatrixMode(GL_PROJECTION);
		//	glLoadIdentity();
		//	GLViewplane(cam->viewwidth, cam->viewheight, cam->viewplane, 1.0f + 0.01f, viewdist + 2.0f);
		}else{
			glColor4f(1, 1, 1, 1);
		}
		for(n = 0; n < npatches; n++){
			//	glBindTexture(GL_TEXTURE_2D, map->TexIDs[x][y]);
			if(patches[n].id > 0){
				if(mt == 0){
					map->Redownload(patches[n].x, patches[n].y);
					glBindTexture(GL_TEXTURE_2D, patches[n].id);
				}
				x1 = texoffx = patches[n].x * TexSize;
				y1 = texoffy = patches[n].y * TexSize;
				if(mt == 1){
					texoffx += detailrotx;
					texoffy += detailroty;
				}
				if(flags & GLREND_NOSTRIPFAN){
					RenderBinTri(&patches[n].ul, x1, y1 + TexSize, x1 + TexSize, y1, x1, y1);
					RenderBinTri(&patches[n].dr, x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize);
				}else{
					RenderBinTriFan(&patches[n].ul, x1, y1 + TexSize, x1 + TexSize, y1, x1, y1, 1,
						(float)map->GetHwrap(x1, y1 + TexSize) * 0.25f,
						(float)map->GetHwrap(x1 + TexSize, y1) * 0.25f,
						(float)map->GetHwrap(x1, y1) * 0.25f);
					FlushFanStack();
					RenderBinTriFan(&patches[n].dr, x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize, 1,
						(float)map->GetHwrap(x1 + TexSize, y1) * 0.25f,
						(float)map->GetHwrap(x1, y1 + TexSize) * 0.25f,
						(float)map->GetHwrap(x1 + TexSize, y1 + TexSize) * 0.25f);
					FlushFanStack();
				}
			}
		}
	}
	//
	}
	//
	CurBinTris = Pool.GetNextBinTriPool();
	//
	glDisable(GL_FOG);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	//
	int drawtime = gltmr.Check(1000);
	//
	if(TerrainFrame++ % 8 == 0 || drawtime > 250){
		OutputDebugLog("Lod split time: " + String(splittime) +
			"  Draw time: " + String(drawtime) +
			"  Total BinTris: " + String(Pool.GetNextBinTriPool()) +
			"  Total Tris: " + String(PolyCount) +
			"  Tris Per Fan: " + String((double)PolyCount / (double)std::max(FanCount, 1)) +
			"\n", 2);
	}

	return true;
}
inline void SphericalYEnvMapCoords(float *out, Vec3 vert, Vec3 norm, Mat3 envbasis, Vec3 campos){
	//RightNeighbor = u - 2 * n * (n dot u)
	Vec3 u, tv, r;
	SubVec3(vert, campos, u);
	ScaleVec3(u, fsqrt_inv(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]));
	//
	ScaleVec3(norm, 2.0f * DotVec3(norm, u), tv);
	SubVec3(u, tv, r);
	//
	Vec3 r2;
	AddVec3(r, envbasis[1], r2);
	float im = 0.5f * fsqrt_inv(r2[0] * r2[0] + r2[1] * r2[1] + r2[2] * r2[2]);
	//
	out[0] = DotVec3(r, envbasis[0]) * im + 0.5f;
	out[1] = DotVec3(r, envbasis[2]) * (-im) + 0.5f;
}
inline void EnvWaterVertex(float x, float y, float z, Vec3 n, Mat3 eb, Vec3 cp){
	Vec3 v = {x, y, z};
	float t[2];
	SphericalYEnvMapCoords(t, v, n, eb, cp);
	glTexCoord2fv(t);
	glVertex3fv(v);
}
inline void EnvWaterTriR(int level, int x1, int z1, int x2, int z2, int x3, int z3, Vec3 n, Mat3 eb, Vec3 cp){
	if(level <= 0){
		EnvWaterVertex(x1, WATERHEIGHT, z1, n, eb, cp);
		EnvWaterVertex(x2, WATERHEIGHT, z2, n, eb, cp);
		EnvWaterVertex(x3, WATERHEIGHT, z3, n, eb, cp);
		return;
	}
	EnvWaterTriR(level - 1, x3, z3, x1, z1, (x1 + x2) >>1, (z1 + z2) >>1, n, eb, cp);
	EnvWaterTriR(level - 1, x2, z2, x3, z3, (x1 + x2) >>1, (z1 + z2) >>1, n, eb, cp);
}
bool GLRenderEngine3::GLRenderWater(Terrain *map, Camera *cam, int flags, float quality){
	if(!patches) return false;
	if(!map || !cam){
		delete [] patches;	//should duplicate cleanup code here...
		patches = NULL;
		npatches = 0;
		//
		return false;
	}

	//Assume identical projection preserved.
	//Erp, canna do dat.
	//Erm, maybe can.
//	glViewport(0, 0, cam->viewwidth, cam->viewheight);//WinWidth, WinHeight);
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	GLViewplane(cam->viewwidth, cam->viewheight, cam->viewplane, 2.0, cam->farplane);

	//
	//Draw water planes where needed.
	if(!(flags & GLREND_NOFOG)){
		glEnable(GL_FOG);
	}else{
		glDisable(GL_FOG);
	}
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	if(Water && Water->id){//Water->Data()){
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, Water->id);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_DECAL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	}else{
		glDisable(GL_TEXTURE_2D);
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	//
	float iscale = 1.0f / WaterScale;
	//
	float ReflectAmount = WaterReflect;
	//
	if(!SkyENV || !SkyENV->id || flags & GLREND_NOENV) ReflectAmount = 0.0f;
	//
	float WaterAmount = 1.0f - ((1.0f - WaterAlpha) / (1.0f - (WaterAlpha * ReflectAmount)));
	//
	if(WaterAmount > 0.05f){
		glBegin(GL_TRIANGLES);
		if(Water && Water->id){//Data()){
			glColor4f(1.0f, 1.0f, 1.0f, WaterAmount);
		}else{
			glColor4f(0, 0.3f, 0.3f, 0.5f);
		}
		for(int n = 0; n < npatches; n++){
			if(patches[n].id > 0){
				if(patches[n].lodul->waterflag){
					int x1 = patches[n].x * TexSize, y1 = patches[n].y * TexSize;
					glTexCoord2f(0.0f, iscale);
					glVertex3f(x1, WATERHEIGHT, y1 + TexSize);
					glTexCoord2f(iscale, 0.0f);
					glVertex3f(x1 + TexSize, WATERHEIGHT, y1);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(x1, WATERHEIGHT, y1);
				}
				if(patches[n].loddr->waterflag){
					int x1 = patches[n].x * TexSize, y1 = patches[n].y * TexSize;
					glTexCoord2f(0.0f, iscale);
					glVertex3f(x1, WATERHEIGHT, y1 + TexSize);
					glTexCoord2f(iscale, iscale);
					glVertex3f(x1 + TexSize, WATERHEIGHT, y1 + TexSize);
					glTexCoord2f(iscale, 0.0f);
					glVertex3f(x1 + TexSize, WATERHEIGHT, y1);
				}
			}
		}
		glEnd();
	}
	//
	glDepthMask(GL_TRUE);	//Tryin it out...  Want to see trans objects cut off.
	//
	glEnable(GL_TEXTURE_2D);
	if(SkyENV && SkyENV->id && ReflectAmount > 0.05f){
		glBindTexture(GL_TEXTURE_2D, SkyENV->id);
		Mat3 id = {{1, 0, 0}, {0, 1, 0}, {0, 0, -1}};
		float CamBackup = 15.0f;
		float turb = (float)(msecs & 4095) / 4096.0f * PI2;
        Vec3 cm;
        cm[0] = cam->x - sin(turb) * 1.5f;
        cm[1] = cam->y + CamBackup;
        cm[2] = (-cam->z) + cos(turb) * 1.5f;
		Vec3 nr = {0, 1, 0};
		glBegin(GL_TRIANGLES);
		glColor4f(1.0f, 1.0f, 1.0f, WaterAlpha * 0.5f);
		glColor4f(1.0f, 1.0f, 1.0f, WaterAlpha * ReflectAmount);
		for(int n = 0; n < npatches; n++){
			if(patches[n].id > 0){
				if(patches[n].lodul->waterflag){
					int x1 = patches[n].x * TexSize, y1 = patches[n].y * TexSize;
					EnvWaterTriR(WaterTessLevel, x1, y1 + TexSize, x1 + TexSize, y1, x1, y1, nr, id, cm);
				}
				if(patches[n].loddr->waterflag){
					int x1 = patches[n].x * TexSize, y1 = patches[n].y * TexSize;
					EnvWaterTriR(WaterTessLevel, x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize, nr, id, cm);
				}
			}
		}
		glEnd();
	}

	if((flags & GLREND_LOCKPATCHES) == 0){
		delete [] patches;	//Ack, forgot this!
		patches = NULL;
		npatches = 0;
	}

	return true;
}


#pragma optimize("", on)

