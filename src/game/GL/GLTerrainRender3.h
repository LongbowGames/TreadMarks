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

#ifndef GLTERRAINRENDER3_H
#define GLTERRAINRENDER3_H

#include "../Render.h"
#include "GLTerrain.h"

#define IHEIGHT_NORENDER 0xffffffff

#define BINTRIPOOL 25000
#define BINTRISAFE 24000
#define BINTRIMAX  20000
//Should be even multiple of 4.

class BinTriPool;

class BinaryTriangleR3
{
public:
	BinaryTriangleR3 *LeftNeighbor, *RightNeighbor, *BottomNeighbor, *LeftChild, *RightChild;
	float SplitVertHeight;	//Split vertex height.
	union{
		float height;
		int iheight;
	};
	unsigned int iIndex; // index into vertex array
	void Null(){
		LeftNeighbor = 0;
		RightNeighbor = 0;
		BottomNeighbor = 0;
		LeftChild = 0;
		RightChild = 0;
		SplitVertHeight = 0.5f;
		iheight = 0;
	};
private:
	int Split2(BinTriPool *pool);	//Note, leaves, LeftChild->RightNeighbor and RightChild->LeftNeighbor hanging!
public:
	void Split(BinTriPool *pool);
	void TestSplit(LodTree *lod, int variance, int LimitLod, int level, int index, BinTriPool *pool);
	void TestSplitZ(int level, int LimitLod, int index, float cz1, float cz2, float cz3, LodTree *lod, BinTriPool *pool);
	void TestSplitClip(int level, float variance, int LimitLod, int index, float radius, int x1, int y1, int x2, int y2, int x3, int y3, float lclip[3], float rclip[3], float zclip[3], LodTree *lod, BinTriPool *pool);
};

class BinTriPool
{
private:
	BinaryTriangleR3	RealBinTriPool[BINTRIPOOL + 1];
	BinaryTriangleR3	*AlignedBinTriPool;
	int					NextBinTriPool;

public:
	BinTriPool() {NextBinTriPool = 0; 	AlignedBinTriPool = (BinaryTriangleR3*)((((unsigned long)RealBinTriPool) + 31) & (~31));}
	void ResetBinTriPool(){ NextBinTriPool = 0;}
	int AvailBinTris(){ return BINTRIPOOL - NextBinTriPool;}
	int ElectiveSplitSafe(){ return NextBinTriPool < BINTRISAFE;}
	BinaryTriangleR3 *AllocBinTri()
	{
		if(NextBinTriPool < BINTRIPOOL)
		{
			BinaryTriangleR3 *t = &AlignedBinTriPool[NextBinTriPool++];
			t->iheight = 0;
			return t;
		}
		return 0;
	}
	void FreeBinTri(BinaryTriangleR3 *tri){return;}
	int GetNextBinTriPool() {return NextBinTriPool;}
};

//A section of world for rendering, may point to wrapped terrain off-map.
struct MapPatch{
	int x, y;	//Coordinates in patch grid.
	unsigned int id;
	BinaryTriangleR3 ul, dr;
	LodTree *lodul, *loddr;
	MapPatch() : x(0), y(0), id(0) {
		ul.Null(); ul.BottomNeighbor = &dr;	//Links component root bintris together at bottoms.
		dr.Null(); dr.BottomNeighbor = &ul;
	};
	MapPatch(int X, int Y, unsigned int ID) : x(X), y(Y), id(ID) {
		ul.Null(); ul.BottomNeighbor = &dr;	//Links component root bintris together at bottoms.
		dr.Null(); dr.BottomNeighbor = &ul;
	};
	void SetCoords(int X, int Y, unsigned int ID){
		x = X;
		y = Y;
		id = ID;
		lodul = LodMap.Tree(x, y, 0);
		loddr = LodMap.Tree(x, y, 1);
	};
	void LinkUp(MapPatch *p){	//Links with a map patch above.
		if(p){
			ul.RightNeighbor = &p->dr;
			p->dr.RightNeighbor = &ul;
		}
	};
	void LinkLeft(MapPatch *p){	//Ditto left.
		if(p){
			ul.LeftNeighbor = &p->dr;
			p->dr.LeftNeighbor = &ul;
		}
	};
	void Split(float variance, int LimitLod, float lclip[3], float rclip[3], float zclip[3], BinTriPool *pool){
		int x1 = x * TexSize, y1 = y * TexSize;
		int x2 = x1 + TexSize, y2 = y1 + TexSize;
		float rad = sqrtf((float)(SQUARE(TexSize >>1) + SQUARE(TexSize >>1)));
		//
		ul.TestSplitClip(0, variance * 0.01f, LimitLod, 0, rad, x1, y2, x2, y1, x1, y1, lclip, rclip, zclip, lodul, pool);
		dr.TestSplitClip(0, variance * 0.01f, LimitLod, 0, rad, x2, y1, x1, y2, x2, y2, lclip, rclip, zclip, loddr, pool);
	};
};

class GLRenderEngine3 : public RenderEngine
{
private:
	BinTriPool	Pool;
	Terrain		*curmap;

	float		lclip[3];
	float		rclip[3];
	float		zclip[3];

	float		texoffx, texoffy;
	float		texscalex, texscaley, texaddx, texaddy;
	float		texoffx2, texoffy2, texscalex2, texscaley2;

	int			PolyCount;

	int			nFanStack;
	int			FanCount;
	int			UseMT;
	union
	{	int FanStack[200];
		float FanStackf[200];
	};

	Timer		gltmr;
	int			TerrainFrame;

	float		CurQuality;
	int			CurBinTris;

	MapPatch	*patches;
	int			npatches;

	void GLViewplane(float w, float h, float view, float n, float f);

	void BinTriVert(int x, int y);
	void RenderBinTri(BinaryTriangleR3 *btri, int x1, int y1, int x2, int y2, int x3, int y3);
	void BinTriVert3(float x, float y, float h);
	void BinTriVert3MT(float x, float y, float h);
	void InitFanStack();
	void FlushFanStack();
	void AddFanPoint(int x, int y, float h);
	void AddFanPoint(int x1, int y1, float h1, int x2, int y2, float h2, int x3, int y3, float h3);
	void RenderBinTriFan(BinaryTriangleR3 *btri, int x1, int y1, int x2, int y2, int x3, int y3, int sense, float h1, float h2, float h3);
public:
	GLRenderEngine3();

	bool GLTerrainRender(Terrain *map, Camera *cam, int flags, float quality, int ms = 0);	//Optional number of msecs for frame parameter.
	bool GLRenderWater(Terrain *map, Camera *cam, int flags, float quality);
	const char *GLTerrainDriverName() {return "Tom's Experimental Terrain Driver";}
};

#endif

