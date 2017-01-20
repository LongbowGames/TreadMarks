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

/*
	RenderEngine - voxel terrain rendering engine class.
	By Seumas McNally, Copyright 1998.
*/

#ifndef RENDER_H
#define RENDER_H

#include "Timer.h"
#include "Terrain.h"
#include "Poly.h"
//#include "CamRBD.h"
//#include <Image.h>

class Image;
class Camera;
class RendBufDesc;

//#define MARKMIXTABLE
//#define MARKMIX(a,b) MarkMix[(a)][(b)] = 20
#define MARKMIX(a,b)

//In-Voxel color mixing at render phase.
#define REND_SMOOTH 0x01
//Fractional voxel sampling coords produce bilinear filtered height/color output.
#define REND_FILTER 0x02
//Multiple virtual voxels are sampled and displayed per physical voxel.
#define REND_OVERSAMPLE 0x04
//Pulls color index values from height map instead of color map.  Editor only.  Define REND_HEIGHTCOLOR_ON to activate.
#define REND_HEIGHTCOLOR 0x08
//
#define REND_16BIT 0x100

#define GLREND_LIMITLOD 0x01
//Caps lod at 1, with no 0 lod patches.
#define GLREND_NOSKY 0x02
//Skips drawing the sky and clearing the screen, good for wrap drawing.
#define GLREND_WIREFRAME 0x04
//Draws terrain in wire-frame.
#define GLREND_NOSTRIPFAN 0x08
//Turns off dynamic strip/fan generation.
#define GLREND_NODETAIL 0x10
//Turns off detail textures.
#define GLREND_NOMT 0x20
//Disables multi-texture support.
#define GLREND_NOFOG 0x40
//Disables fogging.
#define GLREND_NOENV 0x80
//Disables environment mapped water.
#define GLREND_LOCKPATCHES 0x100
//Locks the last allocated LOD patches in place.

#define MAX_Z 0x7fff
//16000
#define ZFP 4
#define ZFPVAL 16

#define WATERHEIGHT 2.0f
#define WATERHEIGHTRAW 8

/*
class MixTable{
friend class RenderEngine;
private:
	UCHAR Mix4[257][256][4];
public:
	unsigned char Mix25(unsigned char a, unsigned char b){ return Mix4[a][b][1]; };
	unsigned char Mix50(unsigned char a, unsigned char b){ return Mix4[a][b][2]; };
	unsigned char Mix75(unsigned char a, unsigned char b){ return Mix4[a][b][3]; };
	unsigned char Mix(unsigned char a, unsigned char b, unsigned char c){
		return Mix4[a][b][c]; };
	bool MakeLookup(PALETTEENTRY *pe, bool disk);
		//Set disk to true to load in old table (if palette matches) and save out table.
};*/

class RenderEngine{
protected:
	Camera lastcam;
//	int Zoffset[MAX_Z];	//Note, not every entry of this will be filled in, only those that exactly match z-step values.
	Timer timer;
	unsigned int msecs;
#ifdef MARKMIXTABLE
//	UCHAR MarkMix[256][256];
#endif
	Image *Sky, *Detail, *DetailMT, *Water, *SkyENV;
	float WaterAlpha, WaterScale, WaterReflect;
	float FogR, FogG, FogB;
	float detailrotspeed, detailrotrad;
	float skyrotatespeed;
	//
	Image *SkyBox[6];
	bool UseSkyBox;
	//
	//Lookup tables and such.
//	int Zlookup[MAX_Z];
//	int LastZlVp;
//	int Clookup[1024];
//	unsigned char Mlookup[1024];
//	unsigned char SMlookup[1024];
//	unsigned char Mix33[257][256];
//	unsigned char Mix4[257][256][4];
	unsigned short Color8to16[256];	//Put in a palette reg number, get a true color.
	unsigned int Color8to16x[256];	//Pre-precision-doubled color long word.
	unsigned int RedMask, GreenMask, BlueMask;	//Bit mask for channel.
	unsigned int RedMaskLen, GreenMaskLen, BlueMaskLen;	//Length of channel in bits.
	unsigned int RedMaskOff, GreenMaskOff, BlueMaskOff;	//Offset from start (right) of USHORT.
	unsigned int RedMask2, GreenMask2, BlueMask2;	//Bit mask for channel.
	unsigned int RedMaskLen2, GreenMaskLen2, BlueMaskLen2;	//Length of channel in bits.
	unsigned int RedMaskOff2, GreenMaskOff2, BlueMaskOff2;	//Offset from start (right) of USHORT.
public:
	RenderEngine();
	~RenderEngine();
	void UnlinkTextures();	//Call this if any of the below textures get deleted!
	bool SetSkyTexture(Image *sky, Image *skyenv = NULL);
	bool SetSkyBoxTexture(int face, Image *tex);
	bool SetDetailTexture(Image *detail, Image *detailmt = 0);
	bool SetWaterTexture(Image *water, float wateralpha, float waterscale, float waterenv);
	bool SetFogColor(float r, float g, float b);
	bool Init16BitTables(int rmask, int gmask, int bmask, PaletteEntry *pe, unsigned short *C816);
	//bool TerrainRender(Terrain *map, Camera *cam, PolyRender *Poly, MixTable *Mix, RendBufDesc *rbd, int flags, int viewdist, int perfectdist);
	virtual bool GLTerrainRender(Terrain *map, Camera *cam, int flags, float quality, int ms = 0) = 0;	//Optional number of msecs for frame parameter.
	virtual bool GLRenderWater(Terrain *map, Camera *cam, int flags, float quality) = 0;
	virtual const char *GLTerrainDriverName() = 0;
	bool SetDetailRot(float rotspeed, float rotrad);
	bool SetSkyRotate(float rotspeed);
};

#endif

