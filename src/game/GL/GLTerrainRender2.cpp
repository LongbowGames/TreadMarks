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

#include "GLTerrainRender2.h"
#include "GLTerrain.h"
#include "../GenericBuffer.h"

#include "../Terrain.h"
#include "../Render.h"
#include "../Poly.h"
#include "../CamRBD.h"
#include "../Image.h"
#include "../ResourceManager.h"
#include "../CfgParse.h"
#include "../macros.h"

#include <GL/glew.h>

const char *GLRenderEngine2::GLTerrainDriverName(){
	static const char DriverName[] = "Experimental Terrain Driver";
	return DriverName;
}


/*
int FastFtoL(float f){
	int i;
	_asm{
		fld dword ptr [f]
		fistp dword ptr [i]
	}
	return i;
}
*/

//#pragma optimize("aw", on)

#define SQUARE(x) ((x) * (x))


static void GLViewplane(float w, float h, float view, float n, float f){
	float x = n * ((w / 2.0f) / view);
	float y = n * ((h / 2.0f) / view);
	glFrustum(-x, x, -y, y, n, f + 1.0f);
}

//#define TexSize 128
//#define LodTreeDepth 12
//#define MaxLodDepth 14
//#define WaterTessLevel 4
//This should make for 4096 byte trees representing 8192 hexels, with 2048 lowest level lod metrics.
//
//#define TexSize 64
//#define LodTreeDepth 10
//#define MaxLodDepth 12
//#define WaterTessLevel 3


//************************************************************************************
//								Actual Terrain Rendering starts here!
//************************************************************************************

/*
struct Tri2D{
	int fanpoly;
	int p[6];
	inline void Set(int x1, int y1, int x2, int y2, int x3, int y3){
		p[0] = x1; p[1] = y1;
		p[2] = x2; p[3] = y2;
		p[4] = x3; p[5] = y3;
	};
};
#define MAXTRISTACK 100
Tri2D TriStack[MAXTRISTACK];
*/

#define IHEIGHT_NORENDER 0xffffffff

struct LodTree;

struct BinaryTriangle2{
//	BinaryTriangle2 *l, *r, *b, *cl, *cr;
	unsigned short l, r, b, cl, cr;
//	float h;	//Split vertex height.
	unsigned char ih, dummy;
	union{
		float height;
		int iheight;
	};
//	int dumm1;//, dummy2;	//Bring total size up to 32 bytes.
	void Null(){
		l = 0;
		r = 0;
		b = 0;
		cl = 0;
		cr = 0;
	//	memset(this, 0, sizeof(BinaryTriangle2));
	//	h = 0.5f;
		ih = 128;
		iheight = 0;
	};
private:
	int Split2();//float propoh = 0.5f);	//Note, leaves, cl->r and cr->l hanging!
public:
	void Split();//float propoh = 0.5f);
	void TestSplit(LodTree *lod, int variance, int level, int index);
//	void TestSplitZ(LodTree *lod, float variance, int level, int index,
//					int x1, int y1, int x2, int y2, int x3, int y3);
//	void TestSplitClip(LodTree *lod, float variance, int level, int index, float radius,
//						int x1, int y1, int x2, int y2, int x3, int y3);
	void TestSplitZ(int level, int index,
					float cz1, float cz2, float cz3);
	void TestSplitClip(int level, int index, float radius,
						int x1, int y1, int x2, int y2, int x3, int y3);
};

#define BINTRIPOOL 25000
#define BINTRISAFE 24000
#define BINTRIMAX  20000
//Should be even multiple of 4.

static BinaryTriangle2 RealBinTriPool[BINTRIPOOL + 1];
static BinaryTriangle2 *BinTriPool;// = &RealBinTriPool[0];
//BinaryTriangle2 BinTriPool[BINTRIPOOL + 1];
static int NextBinTriPool = 1;

#define BTP(a) (&BinTriPool[(a)])

static Terrain *curmap;

//These are seperate so they might be turned into pool alloc/freeing functions later.
//Ok, sooner than later.  :)
inline void ResetBinTriPool(){
	NextBinTriPool = 1;
}
inline int AvailBinTris(){
	return BINTRIPOOL - NextBinTriPool;
}
inline int ElectiveSplitSafe(){
	return NextBinTriPool < BINTRISAFE;
}
//BinaryTriangle2 *AllocBinTri(){
unsigned short AllocBinTri(){
	if(NextBinTriPool < BINTRIPOOL){
		BinaryTriangle2 *t = &BinTriPool[NextBinTriPool++];
	//	t->Null();
		t->iheight = 0;
		return NextBinTriPool - 1;//t;
	//	return &BinTriPool[NextBinTriPool++];
	}
	return 0;
}
void FreeBinTri(BinaryTriangle2 *tri){
	return;
}

int BinaryTriangle2::Split2(){//float propoh){	//Note, leaves, cl->r and cr->l hanging!
	if(AvailBinTris() >= 2){
		cl = AllocBinTri();
		BTP(cl)->ih = 128;//0.5f;	//Pass on T value for vert morph.
	//	cl->h = propoh;	//Pass on T value for vert morph.
		cr = AllocBinTri();
		BTP(cr)->ih = 128;//0.5f;
	//	cr->h = propoh;
		BTP(cl)->l = cr;
		BTP(cr)->r = cl;	//Link kids to each other.
		BTP(cl)->b = l;
		if(l){
			if(BTP(BTP(l)->b) == this) BTP(l)->b = cl;
			else if(BTP(BTP(l)->l) == this) BTP(l)->l = cl;
			else BTP(l)->r = cl;
		}
		BTP(cr)->b = r;
		if(r){
			if(BTP(BTP(r)->b) == this) BTP(r)->b = cr;
			else if(BTP(BTP(r)->r) == this) BTP(r)->r = cr;
			else BTP(r)->l = cr;
		}
		BTP(cl)->cl = BTP(cl)->cr = 0;
		BTP(cr)->cl = BTP(cr)->cr = 0;	//Null childs' child pointers.
		return 1;
	}
	return 0;
};
void BinaryTriangle2::Split(){//float propoh){
//	h = propoh;
	if(b == NULL){	//No bottom, so split half-diamond.
		if(Split2()){
			BTP(cl)->r = BTP(cr)->l = 0;	//Null hanging pointers.
		}
	}else{
		if(BTP(BTP(b)->b) != this){	//Not diamond, so split bottom neighbor.
			BTP(b)->ih = ih;
			BTP(b)->Split();
		}
		//Now b->b _should_ == this...
		if(BTP(BTP(b)->b) == this && AvailBinTris() >= 4){
	//		b->h = h;
			Split2();
			BTP(b)->Split2();
			//
			BTP(cl)->r = BTP(b)->cr;
			BTP(cr)->l = BTP(b)->cl;
			BTP(BTP(b)->cl)->r = cr;	//Link kids with each other.
			BTP(BTP(b)->cr)->l = cl;
	//	}else{
	//		if(b->b != this) OutputDebugString("Bottom neighbor bottom not equal this after split!\n");
		}
	}
};

//Divines the index of the first node at a certian level in an implicit tree.
//#define LODTREEOFF(level) ((1 << (level)) - 1)





static float lclip[3];
static float rclip[3];
static float zclip[3];

static int LimitLod = 0;

//Splits binary tri only if variance greater than parameter.
//__forceinline
void BinaryTriangle2::TestSplit(LodTree *lod, int variance, int level, int index){
	if(lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index] > variance){
		if(cl == 0) Split();	//Don't split if already split!
		if(level < (MaxLodDepth - 1) - LimitLod){
			int index2 = index;
			if(level < LodTreeDepth - 1){
				index = index <<1;
				index2 = index + 1;
			}
			level++;
			if(cl) BTP(cl)->TestSplit(lod, variance, level, index);
			if(cr) BTP(cr)->TestSplit(lod, variance, level, index2);
		}
	}
};
/*
inline void BinaryTriangle2::SetTValue(LodTree *lod, float variance, int level, int index,
										  int x1, int y1, int x2, int y2, int x3, int y3){
	int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
	float t = (float)(avgx) * zclip[0] + (float)(-avgy) * zclip[1] - zclip[2];
	float v = ((float)variance * t * 0.01f);	//Variance at 100 meters.
	int iv = (int)v;
	int vd = lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index];// - iv;
};
*/
//__forceinline
//void BinaryTriangle2::TestSplitZ(LodTree *lod, float variance, int level, int index,
//										  int x1, int y1, int x2, int y2, int x3, int y3){

//Now use globals for these rather than polluting the stack.
static LodTree *lod = 0;
static float variance = 0.0f;

//NOTE: This function, normally, ROUNDS TO NEAREST!!!
inline long FloatToLong(float f) { // relies on IEEE format for float
	f += -0.499999f;	//This change makes it CHOP TO LOWER!!
	f += (3 << 22);
	return ((*(long*)&f)&0x007fffff) - 0x00400000;
//	return (long)f;
}

//#define MORPH_DIST 2
//#define MORPH_INV 0.5f
#define MORPH_DIST 1
#define MORPH_INV 1.0f

void BinaryTriangle2::TestSplitZ(int level, int index, float cz1, float cz2, float cz3){
										 // int x1, int y1, int x2, int y2, int x3, int y3){
	//	int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
	//	float t = (float)(avgx) * zclip[0] + (float)(-avgy) * zclip[1] - zclip[2];
	float avgcz = (cz1 + cz2) * 0.5f;
//	float v = ((float)variance * t * 0.01f);	//Variance at 100 meters.
	//	float v = ((float)variance * avgcz);	//Done ahead of time now.
	//CameraZs will be pre-multiplied by variance now.
//	int iv = (int)v;
	int iv = FloatToLong(avgcz);//FastFtoL(v);
	int vd = lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index];// - iv;
	//Brute force method first...
//	if(vd > 1) h = 0.0f;
//	else h = v - (float)iv;
	//
	if(vd > iv){
		//
		if(vd - iv > MORPH_DIST) ih = 255;//1.0f;
		else ih = FloatToLong((((float)vd - avgcz)) * MORPH_INV * 254.0f);
		//
		if(cl == 0 && ElectiveSplitSafe()) Split();	//Don't split if already split!
		if(level < (MaxLodDepth - 1) - LimitLod){
			//
			//Brute force method first...
		//	if(vd > 1){	//All-split.
		//	//	h = (float)(curmap->GetHwrap(avgx, avgy)) * 0.25f;
		//		h = 0.0f;
		//	}else{	//Morph.
		//		h = v - (float)iv;
		//	//	float t = v - (float)iv;
		//	//	t = 0.0f;
		//	//	h = (float)(curmap->GetHwrap(x1, y1) + curmap->GetHwrap(x2, y2)) * 0.125f * t
		//	//		+ (float)(curmap->GetHwrap(avgx, avgy)) * 0.25f * (1.0f - t);
		//	}
			//
			int index2 = index;
			if(level < LodTreeDepth - 1){
				index = index <<1;
				index2 = index + 1;
			}
			level++;
		//	if(cl) cl->TestSplitZ(lod, variance, level, index, x3, y3, x1, y1, avgx, avgy);
		//	if(cr) cr->TestSplitZ(lod, variance, level, index2, x2, y2, x3, y3, avgx, avgy);
			if(cl) BTP(cl)->TestSplitZ(level, index, cz3, cz1, avgcz);
			if(cr) BTP(cr)->TestSplitZ(level, index2, cz2, cz3, avgcz);
			return;
		}
	}else{
		ih = 0;//0.0f;
	}
	if(cl) BTP(cl)->ih = ih;
	if(cr) BTP(cr)->ih = ih;
};
//__forceinline
//void BinaryTriangle2::TestSplitClip(LodTree *lod, float variance, int level, int index,
void BinaryTriangle2::TestSplitClip(int level, int index,
										  float radius,
										  int x1, int y1, int x2, int y2, int x3, int y3){
	int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
	float t = (float)(avgx) * lclip[0] + (float)(-avgy) * lclip[1] - lclip[2];
	float t2 = (float)(avgx) * rclip[0] + (float)(-avgy) * rclip[1] - rclip[2];
//	if(level < 8){	//Arbitrary.
	if(t < -radius || t2 < -radius){	//Clipped totally.
		//Okay, trying something...  Set neighbor pointers that reference this to null, so it won't be
		//force split at all.  This will BREAK if a concurrent tree is used...
		cl = cr = NULL;	//In case we have already been force split some amount.
		if(b){
			if(BTP(BTP(b)->b) == this) BTP(b)->b = NULL;
			else if(BTP(BTP(b)->l) == this) BTP(b)->l = NULL;
			else if(BTP(BTP(b)->r) == this) BTP(b)->r = NULL;
			b = NULL;
		}
		if(l){
			if(BTP(BTP(l)->b) == this) BTP(l)->b = NULL;
			else if(BTP(BTP(l)->l) == this) BTP(l)->l = NULL;
			else if(BTP(BTP(l)->r) == this) BTP(l)->r = NULL;
			l = NULL;
		}
		if(r){
			if(BTP(BTP(r)->b) == this) BTP(r)->b = NULL;
			else if(BTP(BTP(r)->l) == this) BTP(r)->l = NULL;
			else if(BTP(BTP(r)->r) == this) BTP(r)->r = NULL;
			r = NULL;
		}
		iheight = IHEIGHT_NORENDER;
		return;
	}
	//Possible different clipping version...
//	}else{
//		float nhr = radius * -0.75f;
//		if(t < nhr || t2 < nhr)	//Clipped totally. (Sorta)
//			return;
//	}
//	}else
	if(t > radius && t2 > radius){	//Unclipped totally.
		//	TestSplitZ(lod, variance, level, index, x1, y1, x2, y2, x3, y3);
		//Pass z-distances for vertices pre-multiplied with variance into lowest level test func.
	//	TestSplitZ(level, index,
	//		((float)(x1) * zclip[0] + (float)(-y1) * zclip[1] - zclip[2]) * variance,
	//		((float)(x2) * zclip[0] + (float)(-y2) * zclip[1] - zclip[2]) * variance,
	//		((float)(x3) * zclip[0] + (float)(-y3) * zclip[1] - zclip[2]) * variance);
	//	return;	//Just hand off to other procedure.

		//Since we have to pass the result of three dot products into testsplitz just to find out that
		//this tri may not need to be split, do a normal, simpler split test first (basically the good bits of old testsplitz)
		//and only do dot products if split and need to test children.
		float avgcz = ((float)(avgx) * zclip[0] + (float)(-avgy) * zclip[1] - zclip[2]) * variance;
		int iv = FloatToLong(avgcz);//FastFtoL(v);
		int vd = lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index];// - iv;
		//
		if(vd > iv){
			//
			if(vd - iv > MORPH_DIST) ih = 255;//1.0f;
			else ih = FloatToLong((((float)vd - avgcz)) * MORPH_INV * 254.0f);
			//
			if(cl == 0 && ElectiveSplitSafe()) Split();
			if(level < (MaxLodDepth - 1) - LimitLod){
				int index2 = index;
				if(level < LodTreeDepth - 1){
					index = index <<1;
					index2 = index + 1;
				}
				level++;
				float t1 = ((float)(x1) * zclip[0] + (float)(-y1) * zclip[1] - zclip[2]) * variance;
				float t2 = ((float)(x2) * zclip[0] + (float)(-y2) * zclip[1] - zclip[2]) * variance;
				float t3 = ((float)(x3) * zclip[0] + (float)(-y3) * zclip[1] - zclip[2]) * variance;
				if(cl) BTP(cl)->TestSplitZ(level, index, t3, t1, avgcz);
				if(cr) BTP(cr)->TestSplitZ(level, index2, t2, t3, avgcz);
				return;
			}
		}else{
			ih = 0;//0.0f;
		}
		if(cl) BTP(cl)->ih = ih;
		if(cr) BTP(cr)->ih = ih;
		//
	}else{	//Halfway clipped.
		float t = (float)(avgx) * zclip[0] + (float)(-avgy) * zclip[1] - zclip[2];
	//	float v = ((float)variance * t * 0.01f);	//Variance at 100 meters.
		float v = ((float)variance * t);	//Pre-mulled now.
//		int iv = (int)v;
		int iv = FloatToLong(v);//FastFtoL(v);
		int vd = lod->lodtree[LODTREEOFF(std::min(level, LodTreeDepth - 1)) + index];// - iv;
		if(vd > iv){
			//
			if(vd - iv > MORPH_DIST) ih = 255;//1.0f;
			else ih = FloatToLong((((float)vd - v)) * MORPH_INV * 254.0f);
			//
			if(cl == 0 && ElectiveSplitSafe()) Split();
			if(level < (MaxLodDepth - 1) - LimitLod){
				int index2 = index;
				if(level < LodTreeDepth - 1){
					index = index <<1;
					index2 = index + 1;
				}
				level++;
				radius *= 0.707f;
			//	if(cl) cl->TestSplitClip(lod, variance, level, index, radius, x3, y3, x1, y1, avgx, avgy);
			//	if(cr) cr->TestSplitClip(lod, variance, level, index2, radius, x2, y2, x3, y3, avgx, avgy);
				if(cl) BTP(cl)->TestSplitClip(level, index, radius, x3, y3, x1, y1, avgx, avgy);
				if(cr) BTP(cr)->TestSplitClip(level, index2, radius, x2, y2, x3, y3, avgx, avgy);
				return;
			}
		}else{
			ih = 0;//0.0f;
		}
		if(cl) BTP(cl)->ih = ih;
		if(cr) BTP(cr)->ih = ih;
		//	return;	//Test.
	}
};

//A section of world for rendering, may point to wrapped terrain off-map.
struct MapPatch2{
	int x, y;	//Coordinates in patch grid.
	unsigned int id;
//	BinaryTriangle2 ul, dr;
	unsigned short ul, dr;
	LodTree *lodul, *loddr;
	MapPatch2() : x(0), y(0), id(0) {
	//	ul.Null(); ul.b = &dr;	//Links component root bintris together at bottoms.
	//	dr.Null(); dr.b = &ul;
		ul = AllocBinTri();
		dr = AllocBinTri();
		BTP(ul)->Null(); BTP(ul)->b = dr;	//Links component root bintris together at bottoms.
		BTP(dr)->Null(); BTP(dr)->b = ul;
	};
	MapPatch2(int X, int Y, unsigned int ID) : x(X), y(Y), id(ID) {
	//	ul.Null(); ul.b = &dr;	//Links component root bintris together at bottoms.
	//	dr.Null(); dr.b = &ul;
		ul = AllocBinTri();
		dr = AllocBinTri();
		BTP(ul)->Null(); BTP(ul)->b = dr;	//Links component root bintris together at bottoms.
		BTP(dr)->Null(); BTP(dr)->b = ul;
	};
	void SetCoords(int X, int Y, unsigned int ID){
		x = X;
		y = Y;
		id = ID;
		lodul = LodMap.Tree(x, y, 0);
		loddr = LodMap.Tree(x, y, 1);
	};
	void LinkUp(MapPatch2 *p){	//Links with a map patch above.
		if(p){
			BTP(ul)->r = p->dr;
			BTP(p->dr)->r = ul;
		}
	};
	void LinkLeft(MapPatch2 *p){	//Ditto left.
		if(p){
			BTP(ul)->l = p->dr;
			BTP(p->dr)->l = ul;
		}
	};
	void Split(float variance){//, int scale
	//	ul.TestSplit(LodMap.Tree(x, y, 0), variance, 0, 0);
	//	dr.TestSplit(LodMap.Tree(x, y, 1), variance, 0, 0);
		int x1 = x * TexSize, y1 = y * TexSize;
		int x2 = x1 + TexSize, y2 = y1 + TexSize;
	//	float rad = sqrtf((float)SQUARE(TexSize));
		float rad = sqrtf((float)(SQUARE(TexSize >>1) + SQUARE(TexSize >>1)));
		//
		::variance = variance * 0.01f;
		//
		lod = lodul;
		BTP(ul)->TestSplitClip(0, 0, rad, x1, y2, x2, y1, x1, y1);
		lod = loddr;
		BTP(dr)->TestSplitClip(0, 0, rad, x2, y1, x1, y2, x2, y2);
		//
	//	ul.TestSplitClip(lodul, variance * 0.01f, 0, 0, rad, x1, y2, x2, y1, x1, y1);
	//	dr.TestSplitClip(loddr, variance * 0.01f, 0, 0, rad, x2, y1, x1, y2, x2, y2);
		//
	//	ul.TestSplitClip(LodMap.Tree(x, y, 0), variance, 0, 0, rad, x1, y2, x2, y1, x1, y1);
	//	dr.TestSplitClip(LodMap.Tree(x, y, 1), variance, 0, 0, rad, x2, y1, x1, y2, x2, y2);
	//	dr.TestSplit(LodMap.Tree(x, y, 1), variance, 0, 0);
	};
};

static float texoffx, texoffy;

//#define texscalex (1.0f / (float)(TexSize + 1))
//#define texscaley (1.0f / (float)(TexSize + 1))
//#define texaddx (texscalex * 0.5f)
//Bit to add to bring tex coords into proper internal-values for repeat mode, so border/wrap pixels are never touched.  Texels will be 0.5 to 127.5.
//#define texaddy (texscaley * 0.5f)
static float texscalex, texscaley, texaddx, texaddy;
static float texoffx2, texoffy2, texscalex2, texscaley2;


//Outputs one terrain vertex at requested map x and y coords.
inline void BinTriVert(int x, int y){
	glTexCoord2f(((float)x - texoffx) * texscalex + texaddx, ((float)y - texoffy) * texscaley + texaddy);
	glVertex3f(x, ((float)curmap->GetHwrap(x, y) * 0.25f), y);
}
static int PolyCount = 0;

//Recursive function to render all binary triangles descending from a root triangle.
//static void RenderBinTri(BinaryTriangle2 *btri, int x1, int y1, int x2, int y2, int x3, int y3){
//	if(btri->cl && btri->cr){
//		int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
//	//	int t =
//		RenderBinTri(btri->cl, x3, y3, x1, y1, avgx, avgy);
//	//	if(btri->cr){
//	//	t |=
//		RenderBinTri(btri->cr, x2, y2, x3, y3, avgx, avgy);// <<1;
//	}else{
//		glBegin(GL_TRIANGLES);	//Really badly unoptimized triangles...
//		BinTriVert(x1, y1);
//		BinTriVert(x2, y2);
//		BinTriVert(x3, y3);
//		glEnd();
//		PolyCount++;
//	//	return 1;
//	}
//}
static void CDECL FakeglTexCoord2f(float a, float b){
	a += b;
}
static void CDECL FakeglVertex3f(float a, float b, float c){
	a += b - c;
}
inline void BinTriVert3(float x, float y, float h){
	glTexCoord2f((x - texoffx) * texscalex + texaddx, (y - texoffy) * texscaley + texaddy);
	glVertex3f(x, h, y);
	//Testing without OpenGL overhead.
//	FakeglTexCoord2f((x - texoffx) * texscalex + texaddx, (y - texoffy) * texscaley + texaddy);
//	FakeglVertex3f(x, h, y);
}
inline void BinTriVert3MT(float x, float y, float h){
	glMultiTexCoord2f(GL_TEXTURE0, (x - texoffx) * texscalex + texaddx, (y - texoffy) * texscaley + texaddy);
	glMultiTexCoord2f(GL_TEXTURE1, (x - texoffx2) * texscalex2, (y - texoffy2) * texscaley2);
	glVertex3f(x, h, y);
}
static union{
int FanStack[200];
float FanStackf[200];
};

static int nFanStack = 0;
static int FanCount = 0;
static int UseMT = 0;

inline void InitFanStack(){
	nFanStack = 0;
	FanCount = 0;
	PolyCount = 0;
}
inline void FlushFanStack(){
	if(nFanStack > 0){
		//
		glBegin(GL_TRIANGLE_FAN);
		//
		if(UseMT){
			for(int n = 0; n < nFanStack; n += 3){
				BinTriVert3MT(FanStack[n], FanStack[n + 1], FanStackf[n + 2]);
			}
		}else{
			for(int n = 0; n < nFanStack; n += 3){
			//	BinTriVert(FanStack[n], FanStack[n + 1]);
				BinTriVert3(FanStack[n], FanStack[n + 1], FanStackf[n + 2]);
			}
		}
		//
		glEnd();
		//
		nFanStack = 0;
		FanCount++;
	}
}
inline void AddFanPoint(int x, int y, float h){
	FanStack[nFanStack] = x;
	FanStack[nFanStack + 1] = y;
	FanStackf[nFanStack + 2] = h;
	nFanStack += 3;
}
inline void AddFanPoint(int x1, int y1, float h1, int x2, int y2, float h2, int x3, int y3, float h3){
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
//	AddFanPoint(x1, y1);
//	AddFanPoint(x2, y2);
//	AddFanPoint(x3, y3);
}
//__forceinline
static void RenderBinTriFan(BinaryTriangle2 *btri, int x1, int y1, int x2, int y2, int x3, int y3, int sense, float h1, float h2, float h3){
	if(btri->cl && btri->cr){
		int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
		float th;
		if(btri->iheight){
		//	if(btri->iheight == IHEIGHT_NORENDER) return;
			th = btri->height;
		}else{
		//	float t;
		//	if(btri->b) t = (btri->h + btri->b->h) * 0.25f;
		//	else t = btri->h;
		//	th = (h1 + h2) * (0.5 - t) + (float)(curmap->GetHwrap(avgx, avgy)) * 0.5f * t;
			if(btri->b){
				float t = (float)((int)btri->ih + (int)BTP(btri->b)->ih) * 0.0009803f;//(1.0 / 1020.0f);//(btri->h + btri->b->h) * 0.25f;
				th = (h1 + h2) * (0.5 - t) + (float)(curmap->GetHwrap(avgx, avgy)) * 0.5f * t;
				btri->height = BTP(btri->b)->height = th;
			}else{
				float t = (float)(btri->ih) * 0.0019607;//(1.0f / 510.0f);//btri->h * 0.5f;
				th = (h1 + h2) * (0.5 - t) + (float)(curmap->GetHwrap(avgx, avgy)) * 0.5f * t;
				btri->height = th;
			}
		}
		if(sense){
			RenderBinTriFan(BTP(btri->cr), x2, y2, x3, y3, avgx, avgy, 0, h2, h3, th);	//Right/Left.
			RenderBinTriFan(BTP(btri->cl), x3, y3, x1, y1, avgx, avgy, 0, h3, h1, th);
		//	RenderBinTriFan(btri->cr, x2, y2, x3, y3, avgx, avgy, 0);	//Right/Left.
		//	RenderBinTriFan(btri->cl, x3, y3, x1, y1, avgx, avgy, 0);
		}else{
			RenderBinTriFan(BTP(btri->cl), x3, y3, x1, y1, avgx, avgy, 1, h3, h1, th);	//Left/Right.
			RenderBinTriFan(BTP(btri->cr), x2, y2, x3, y3, avgx, avgy, 1, h2, h3, th);
		//	RenderBinTriFan(btri->cl, x3, y3, x1, y1, avgx, avgy, 1);	//Left/Right.
		//	RenderBinTriFan(btri->cr, x2, y2, x3, y3, avgx, avgy, 1);
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
		//	if(x1 == FanStack[0] && y1 == FanStack[1]){
			//	if(x2 == FanStack[nFanStack - 2] && y2 == FanStack[nFanStack - 1]){
				AddFanPoint(x3, y3, h3);
				return;
			//	}
			}
			if(((fanfoo - HASH(x2, y2)) | (fanfoo2 - HASH(x3, y3))) == 0){
		//	if(x2 == FanStack[0] && y2 == FanStack[1]){
			//	if(x3 == FanStack[nFanStack - 2] && y3 == FanStack[nFanStack - 1]){
				AddFanPoint(x1, y1, h1);
				return;
			//	}
			}
			if(((fanfoo - HASH(x3, y3)) | (fanfoo2 - HASH(x1, y1))) == 0){
		//	if(x3 == FanStack[0] && y3 == FanStack[1]){
			//	if(x1 == FanStack[nFanStack - 2] && y1 == FanStack[nFanStack - 1]){
				AddFanPoint(x2, y2, h2);
				return;
			//	}
			}
			FlushFanStack();	//Not empty, but no fan connect.
		}
		if(sense) AddFanPoint(x1, y1, h1, x2, y2, h2, x3, y3, h3);
		else AddFanPoint(x3, y3, h3, x1, y1, h1, x2, y2, h2);
	}
}

static Timer gltmr;
static int TerrainFrame = 0;

static float CurQuality = 0.1f;
static int CurBinTris = 1;

static MapPatch2 *patches = 0;
static int npatches = 0;

bool GLRenderEngine2::GLTerrainRender(Terrain *map, Camera *cam, int flags, float quality, int ms){
	//
	//
	//Align bintripool on 32byte boundary
	BinTriPool = (BinaryTriangle2*)((((unsigned long)RealBinTriPool) + 31) & (~31));
//	BinTriPool = RealBinTriPool;
	//
	msecs += ms;
	//
	if(!map || !cam || map->Width() <= 0) return false;
	curmap = map;
	float viewdist = cam->farplane;	//This is now stored in the camera object.
//	glViewport(0, 0, cam->viewwidth, cam->viewheight);//WinWidth, WinHeight);
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
	//
//	if(flags & GLREND_NOTREA
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
//	glShadeModel(GL_SMOOTH);	//Testing.
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
//	glEnable(GL_BLEND);	//See if transparent fog works...
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//
	glEnable(GL_CULL_FACE);
//	glDisable(GL_CULL_FACE);
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
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);//GL_DECAL);
	//
	if(flags & GLREND_WIREFRAME){
		glPolygonMode(GL_FRONT, GL_LINE);
	}else{
		glPolygonMode(GL_FRONT, GL_FILL);
	}
	//
	int TexPerMapW = map->Width() / TexSize;
	int TexPerMapH = map->Height() / TexSize;
//	BinaryTriangle2 bt[9];
//	memset(bt, 0, sizeof(bt));
//	bt[0].cl = bt[0].cr = &bt[1];
//	bt[1].cr = bt[1].cl = &bt[2];
//	bt[2].cr = bt[2].cl = &bt[3];
//	bt[3].cr = bt[3].cl = &bt[4];
//	bt[4].cr = bt[4].cl = &bt[5];
//	bt[5].cr = bt[5].cl = &bt[6];
//	bt[6].cr = bt[6].cl = &bt[7];
//	bt[7].cr = bt[7].cl = &bt[8];
	//
	float tv[2];
    float bv[2];
    bv[0] = sin(-cam->b * DEG2RAD);
    bv[1] = cos(-cam->b * DEG2RAD);
	float s;
//	tv[0] = -cam->viewwidth / 2.0f; tv[1] = cam->viewplane;
	//
	float tcos = cos(cam->a * DEG2RAD);
	tv[0] = -cam->viewwidth / (2.0f * SQUARE(tcos));
	tv[1] = cam->viewplane * SQUARE(tcos);
	//Adjustment to loosen clipping when camera pitches down ur up too much.
	//
//	tv[0] = -cam->viewwidth / 4.0f; tv[1] = cam->viewplane;
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
	//Right-handed world coordinate clipping planes.
	//
	//
//	rclip[0] = cam->viewwidth / 2.0f; rclip[1] = -cam->viewplane;
//	zclip[0] = 1.0f; zclip[0] = 0;
//	RotVec2(
	//
	ResetBinTriPool();
	//
//	glBindTexture(GL_TEXTURE_2D, map->TexIDs[0][0]);
	int x, y, x1, y1, n = 0;
	//
	gltmr.Start();
	//
	int xs = floor(cam->x / (float)TexSize), ys = floor(-cam->z / (float)TexSize);
	int psize = viewdist / TexSize + 1;
//	int
	npatches = SQUARE(psize * 2 + 1);
	//
//	MapPatch2 *
	if(patches) delete [] patches;
	patches = new MapPatch2[npatches];//TexPerMapW * TexPerMapH * 9];
	if(!patches) return false;
	//
	n = 0;
//	MapPatch2 *tpatch = &patches[0];
//	for(y = -TexPerMapH; y < TexPerMapH * 2; y++){	//Create and link list of patches.
//		for(x = -TexPerMapW; x < TexPerMapW * 2; x++){
	float patchrad = sqrtf((float)SQUARE(TexSize / 2)) * 1.4f;
	for(y = ys - psize; y <= ys + psize; y++){	//Create and link list of patches.
		for(x = xs - psize; x <= xs + psize; x++){
			float xf = (float)(x * TexSize + (TexSize >>1));
			float yf = (float)(y * TexSize + (TexSize >>1));
			float t = (float)(xf) * lclip[0] + (float)(-yf) * lclip[1] - lclip[2];
			float t2 = (float)(xf) * rclip[0] + (float)(-yf) * rclip[1] - rclip[2];
			if(t > -patchrad && t2 > -patchrad){
		//	if(t > 0 && t2 > 0){
			//	patches[n].x = x;// * TexSize;
			//	patches[n].y = y;// * TexSize;
			//	patches[n].id = map->TexIDs[x & (TexPerMapW - 1)][y & (TexPerMapH - 1)];
				patches[n].SetCoords(x, y, map->TexIDs[x & (TexPerMapW - 1)][y & (TexPerMapH - 1)]);
				if(x > xs - psize && patches[n - 1].id != 0) patches[n].LinkLeft(&patches[n - 1]);
				if(y > ys - psize && patches[n - ((psize <<1) + 1)].id != 0) patches[n].LinkUp(&patches[n - ((psize <<1) + 1)]);
			//	if(x > -TexPerMapW) patches[n].LinkLeft(&patches[n - 1]);
			//	if(y > -TexPerMapH) patches[n].LinkUp(&patches[n - TexPerMapW * 3]);
			}
			n++;
		}
	}
//	int npatches = n;
	//
	float InputQuality = quality;
	int triwant = (float)BINTRIMAX * quality;
	float TQuality = CurQuality * (((float)triwant / (float)BINTRIMAX) / ((float)CurBinTris / (float)BINTRIMAX));
	//Trying triangle-count pegging scheme.
//	float DQuality = (TQuality - CurQuality) * 0.1f;	//Filter changes a little.
	float DQuality = (TQuality - CurQuality) * 0.05f;	//Filter changes a little.
	CurQuality += std::min(std::max(DQuality, -0.1f), 0.05f);
//	CurQuality += std::min(std::max(DQuality, -0.01f), 0.005f);
	CurQuality = std::min(std::max(0.1f, CurQuality), 1.0f);
	quality = CurQuality;
	//
	float Variance = SQUARE(1.0f - quality) * 16.0f + 1.0f;
//	LimitLod = std::max(0, (1.0f - quality) * 6.0f);
	LimitLod = std::max(0.0f, (1.0f - InputQuality) * 4.0f);	//Fix limit lod based on specified virtual quality, to reduce lod oscillations.
	//
	//
//	_controlfp(_RC_CHOP, _MCW_RC);	//Set FPU to CHOP mode for use with FastFtoL in following routines.
	//
	//Dive and create variable lod levels here.
	for(n = 0; n < npatches; n++){
		if(patches[n].id > 0){
			patches[n].Split(Variance);
		}
	}
	//
	int splittime = gltmr.Check(1000);
	//
	gltmr.Start();
	//
//	for(y = 0; y < TexPerMapH; y++){
//		for(x = 0; x < TexPerMapW; x++){
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
				RenderBinTriFan(BTP(patches[n].ul), x1, y1 + TexSize, x1 + TexSize, y1, x1, y1, 1,
					(float)map->GetHwrap(x1, y1 + TexSize) * 0.25f,
					(float)map->GetHwrap(x1 + TexSize, y1) * 0.25f,
					(float)map->GetHwrap(x1, y1) * 0.25f);
				FlushFanStack();
				RenderBinTriFan(BTP(patches[n].dr), x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize, 1,
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
				//	RenderBinTri(&patches[n].ul, x1, y1 + TexSize, x1 + TexSize, y1, x1, y1);
				//	RenderBinTri(&patches[n].dr, x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize);
				}else{
					RenderBinTriFan(BTP(patches[n].ul), x1, y1 + TexSize, x1 + TexSize, y1, x1, y1, 1,
						(float)map->GetHwrap(x1, y1 + TexSize) * 0.25f,
						(float)map->GetHwrap(x1 + TexSize, y1) * 0.25f,
						(float)map->GetHwrap(x1, y1) * 0.25f);
					FlushFanStack();
					RenderBinTriFan(BTP(patches[n].dr), x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize, 1,
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
	//
//	FlushFanStack();
	//
	CurBinTris = NextBinTriPool;
	//
	glDisable(GL_FOG);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	//
	//
	//
	int drawtime = gltmr.Check(1000);
	//
	if(TerrainFrame++ % 8 == 0 || drawtime > 250){
		OutputDebugLog("Lod split time: " + String(splittime) +
			"  Draw time: " + String(drawtime) +
			"  Total BinTris: " + String(NextBinTriPool) +
			"  Total Tris: " + String(PolyCount) +
			"  Tris Per Fan: " + String((double)PolyCount / (double)std::max(FanCount, 1)) +
			"  BTS: " + String((int)sizeof(BinaryTriangle2)) +
			"\n", 2);
	}

//	delete [] patches;	//Ack, forgot this!

	return true;
}
inline void SphericalYEnvMapCoords(float *out, Vec3 vert, Vec3 norm, Mat3 envbasis, Vec3 campos){
	//r = u - 2 * n * (n dot u)
	Vec3 u, tv, r;
	SubVec3(vert, campos, u);
//	NormVec3(u);
	ScaleVec3(u, fsqrt_inv(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]));
	//
	ScaleVec3(norm, 2.0f * DotVec3(norm, u), tv);
	SubVec3(u, tv, r);
	//
	Vec3 r2;
	AddVec3(r, envbasis[1], r2);
//	float im = 1.0f / (2.0f * LengthVec3(r2));	//1 / (2 * sqrtf((rx+envyx)^2 + (ry+envyy)^2 + (rz+envyz)^2))
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
bool GLRenderEngine2::GLRenderWater(Terrain *map, Camera *cam, int flags, float quality){
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
		//	glColor4f(1.0f, 1.0f, 1.0f, 1.0f - ((1.0f - WaterAlpha) / (1.0f - (WaterAlpha * 0.5f))) );
		//	glColor4f(1.0f, 1.0f, 1.0f, 1.0f - ((1.0f - WaterAlpha) / (1.0f - (WaterAlpha * ReflectAmount))) );
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
	//	float t[2];
	//	Vec3 v;
		Mat3 id = {{1, 0, 0}, {0, 1, 0}, {0, 0, -1}};
	//	float CamBackup = 20.0f;
		float CamBackup = 15.0f;
		float turb = (float)(msecs & 4095) / 4096.0f * PI2;
	//	Vec3 cm = {cam->x - sin(cam->b * DEG2RAD) * CamBackup, cam->y + CamBackup, (-cam->z) + cos(cam->b * DEG2RAD) * CamBackup};//, 0, 0};
        Vec3 cm;
        cm[0] = cam->x - sin(turb) * 1.5f;
        cm[1] = cam->y + CamBackup;
        cm[2] = (-cam->z) + cos(turb) * 1.5f;
	//	Vec3 nr = {sin(turb) * 0.2f, 1, cos(turb) * 0.2f};
		Vec3 nr = {0, 1, 0};
		glBegin(GL_TRIANGLES);
		glColor4f(1.0f, 1.0f, 1.0f, WaterAlpha * 0.5f);
		glColor4f(1.0f, 1.0f, 1.0f, WaterAlpha * ReflectAmount);
		for(int n = 0; n < npatches; n++){
			if(patches[n].id > 0){
				if(patches[n].lodul->waterflag){
					int x1 = patches[n].x * TexSize, y1 = patches[n].y * TexSize;
					EnvWaterTriR(WaterTessLevel, x1, y1 + TexSize, x1 + TexSize, y1, x1, y1, nr, id, cm);
				//	EnvWaterVertex(x1, WATERHEIGHT, y1 + TexSize, nr, id, cm);
				//	EnvWaterVertex(x1 + (TexSize >>1), WATERHEIGHT, y1 + (TexSize >>1), nr, id, cm);
				//	EnvWaterVertex(x1, WATERHEIGHT, y1, nr, id, cm);
				//	EnvWaterVertex(x1 + (TexSize >>1), WATERHEIGHT, y1 + (TexSize >>1), nr, id, cm);
				//	EnvWaterVertex(x1 + TexSize, WATERHEIGHT, y1, nr, id, cm);
				//	EnvWaterVertex(x1, WATERHEIGHT, y1, nr, id, cm);
				}
				if(patches[n].loddr->waterflag){
					int x1 = patches[n].x * TexSize, y1 = patches[n].y * TexSize;
					EnvWaterTriR(WaterTessLevel, x1 + TexSize, y1, x1, y1 + TexSize, x1 + TexSize, y1 + TexSize, nr, id, cm);
				//	EnvWaterVertex(x1, WATERHEIGHT, y1 + TexSize, nr, id, cm);
				//	EnvWaterVertex(x1 + TexSize, WATERHEIGHT, y1 + TexSize, nr, id, cm);
				//	EnvWaterVertex(x1 + (TexSize >>1), WATERHEIGHT, y1 + (TexSize >>1), nr, id, cm);
				//	EnvWaterVertex(x1 + (TexSize >>1), WATERHEIGHT, y1 + (TexSize >>1), nr, id, cm);
				//	EnvWaterVertex(x1 + TexSize, WATERHEIGHT, y1 + TexSize, nr, id, cm);
				//	EnvWaterVertex(x1 + TexSize, WATERHEIGHT, y1, nr, id, cm);
				}
			}
		}
		glEnd();
	}

	delete [] patches;	//Ack, forgot this!
	patches = NULL;
	npatches = 0;

	return true;
}


#pragma optimize("", on)

