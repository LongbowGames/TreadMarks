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
	Color quantization class.  Compresses multiple 256 color palettes into one.
*/

//Optimization possible for MakeRemapTable over in Image class.
//If all original colors were guaranteed fed into octree, use
//octree's inverse palette lookup to fill remap tables instead
//of doing 256^2 tests.  Since raw ColorOctree palettes are unsorted,
//would need a remap table in Quantizer class to map between sorted
//"official" palette and unsorted ColorOctree inverse lookup result.
//Pass a function to qsort that sorts by indecise et viola, remap
//table is by-product of sort.  Would also let Image::Quantize32to8
//produce sorted palettes...  Do this soon!

//Also, improve the damn color quantization for High Quality stuff,
//it SUCKS compared to Photoshop.  Agh.  And move Inverse Palette
//stuff in here, and use an octree to make a generic 5x5x5 bit inverse
//palette perhaps.

//NOTE, do not use .h on end of Standard C++ Header files!

//Disable exceptions not enabled warnings.
#pragma warning( disable : 4530 )

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "Quantizer.h"

#include "macros.h"

#define SQR(a) ((a) * (a))

//#define TEST_CUBE_LERP
//Test code for RGB 233 color space sorting of 8bit palette for lerp mixing.
typedef PaletteEntry PE;
int CDECL PeRedCompare(const void *c1, const void *c2){
	return ((PE*)c1)->peRed - ((PE*)c2)->peRed; }
int CDECL PeGreenCompare(const void *c1, const void *c2){
	return ((PE*)c1)->peGreen - ((PE*)c2)->peGreen; }
int CDECL PeBlueCompare(const void *c1, const void *c2){
	return ((PE*)c1)->peBlue - ((PE*)c2)->peBlue; }
int CDECL PeRedOverBlueCompare(const void *c1, const void *c2){
	return (((PE*)c1)->peRed - ((PE*)c1)->peBlue) - (((PE*)c2)->peRed - ((PE*)c2)->peBlue); }
int CDECL PeGreenOverBlueCompare(const void *c1, const void *c2){
	return (((PE*)c1)->peGreen - ((PE*)c1)->peBlue) - (((PE*)c2)->peGreen - ((PE*)c2)->peBlue); }
int CDECL PeBrightnessCompare(const void *c1, const void *c2){
	return ( (((PE*)c1)->peRed <<1) + (((PE*)c1)->peGreen <<2) + (((PE*)c1)->peBlue)
		) - ( (((PE*)c2)->peRed <<1) + (((PE*)c2)->peGreen <<2) + (((PE*)c2)->peBlue) );
}
#define RMASK 0x07
#define RSHIFT 0
#define GMASK 0x38
#define GSHIFT 3
#define BMASK 0xc0
#define BSHIFT 6
unsigned char LerpRGB332(unsigned char rgb1, unsigned char rgb2, float t){
	float r1 = (float)((rgb1 & RMASK) >>RSHIFT) + 0.5f;
	float g1 = (float)((rgb1 & GMASK) >>GSHIFT) + 0.5f;
	float b1 = (float)((rgb1 & BMASK) >>BSHIFT) + 0.5f;
	float r2 = (float)(((rgb2 & RMASK) >>RSHIFT) + 0.5f) * t + r1 * (1.0f - t);
	float g2 = (float)(((rgb2 & GMASK) >>GSHIFT) + 0.5f) * t + g1 * (1.0f - t);
	float b2 = (float)(((rgb2 & BMASK) >>BSHIFT) + 0.5f) * t + b1 * (1.0f - t);
	return (unsigned char)(((int)r2 <<RSHIFT) | ((int)g2 <<GSHIFT) | ((int)b2 <<BSHIFT));
}
void SortPaletteCube(PaletteEntry *pe){
	int i;
#if 1
	qsort(&pe[0], 256, sizeof(PaletteEntry), PeBlueCompare);
	for(i = 0; i < 256; i += 64){
		qsort(&pe[i], 64, sizeof(PaletteEntry), PeGreenCompare);
	}
	for(i = 0; i < 256; i += 8){
		qsort(&pe[i], 8, sizeof(PaletteEntry), PeRedCompare);
	}
#else
	qsort(&pe[0], 256, sizeof(PaletteEntry), PeBrightnessCompare);
	for(i = 0; i < 256; i += 64){
		qsort(&pe[i], 64, sizeof(PaletteEntry), PeRedOverBlueCompare);
	}
	for(i = 0; i < 256; i += 8){
		qsort(&pe[i], 8, sizeof(PaletteEntry), PeGreenOverBlueCompare);
	}
#endif
}


//Ok, now we're trying out making a full inverse palette every time, but at
//a very low resolution (16, 16, 16, or 4096 total, ala Amiga).
void MixTable::BestColorInit(PaletteEntry *pe){
	DifferentColors = 0;
	CachedColors = 0;
	/*
	int x, y, z;
	for(x = 0; x < 64; x++){
		for(y = 0; y < 64; y++){
			for(z = 0; z < 64; z++){
				InvPal[x][y][z] = 0;
			}
		}
	}
	*/
	/*
	int r, g, b, x, y, z;
	unsigned int diff, k, t;
	UCHAR col;
	for(x = 0; x < INVPALSIZE; x++){
		for(y = 0; y < INVPALSIZE; y++){
			for(z = 0; z < INVPALSIZE; z++){
				diff = 2048;
				col = 127;	//Last color reg with lowest difference found.
				r = (x <<INVPALSHIFT) | (x >>INVPALSHIFT2);
				g = (y <<INVPALSHIFT) | (y >>INVPALSHIFT2);
				b = (z <<INVPALSHIFT) | (z >>INVPALSHIFT2);
				for(k = 0; k < 256; k++){
					t = (abs(pe[k].peRed - r) <<1) + (abs(pe[k].peGreen - g) <<2) + abs(pe[k].peBlue - b);
					if(t < diff){ diff = t; col = k; }
				}
				InvPal[x][y][z] = col;
			}
		}
	}
	*/
	InvPal.Init(6, 6, 6);
	InvPal.Make(pe);
}
inline unsigned char MixTable::BestColor(PaletteEntry *pe, unsigned char r, unsigned char g, unsigned char b){
	return InvPal.Lookup(r, g, b);
//	return InvPal[r >>INVPALSHIFT][g >>INVPALSHIFT][b >>INVPALSHIFT];
	/*
	UCHAR *ucp = &InvPal[r >>INVPALSHIFT][g >>INVPALSHIFT][b >>INVPALSHIFT];
	if(*ucp){
		CachedColors++;
		return *ucp;	//Color reg already in Inverse Palette Table.
	}
	unsigned int diff = 2048;
	UCHAR col = 0;	//Last color reg with lowest difference found.
	unsigned int k, t;
	for(k = 0; k < 256; k++){
		t = (abs(pe[k].peRed - r) <<1) + (abs(pe[k].peGreen - g) <<2) + abs(pe[k].peBlue - b);
		if(t < diff){ diff = t; col = k; }
	}
	DifferentColors++;
	*ucp = col;
	return col;
	*/
}
bool MixTable::MakeLookup(PaletteEntry *pe, bool disk){
	if(NULL == pe) return false;

	//*** WARNING!  TEMPORARY!
	disk = false;
	//*** New Inverse Palette should make things fast enough to not need this...

	int r, g, b, c, i;//, k, diff, col, t,
	bool OK;
//	unsigned char tempfooey;
	const char fn[] = "MixLookup.dat";
	const int tablesize = 256 * 256 * 4;
	PaletteEntry tpe[256];
	FILE *f;

	//Now the lookup is saved, and if the saved lookup palette is the same as the palette
	//that a lookup has to be made for, the old lookup is loaded in.
#ifndef TEST_CUBE_LERP
	if(disk){
		if((f = fopen(fn, "rb")) != NULL){
			fread(tpe, sizeof(PaletteEntry), 256, f);
			OK = true;
			for(i = 0; i < 256; i++){
				if(	pe[i].peRed != tpe[i].peRed ||
					pe[i].peGreen != tpe[i].peGreen ||
					pe[i].peBlue != tpe[i].peBlue){
					OK = false;
				}
			}
			if(OK == true && fread(Mix4, 1, tablesize, f) == tablesize){
				fclose(f);
				return true;
			}
			fclose(f);
		}
	}
#endif

	BestColorInit(pe);

#ifdef TEST_CUBE_LERP
	for(c = 0; c < 256; c++){
		for(i = 0; i < 256; i++){
			for(k = 0; k < 4; k++){
				Mix4[c][i][k] = LerpRGB332(c, i, (float)k * 0.25f);
			}
		}
	}
#else
	for(c = 0; c < 256; c++){  for(i = 0; i < 256; i++){
	//	if(i < 256 - 8) tempfooey = Mix4[c][i + 8][0];
			Mix4[c][i][0] = c;
			r = ((pe[c].peRed <<7) + (pe[c].peRed <<6) + (pe[i].peRed <<6)) >>8;
			g = ((pe[c].peGreen <<7) + (pe[c].peGreen <<6) + (pe[i].peGreen <<6)) >>8;	//r,g,b, hold best-color.
			b = ((pe[c].peBlue <<7) + (pe[c].peBlue <<6) + (pe[i].peBlue <<6)) >>8;
			Mix4[c][i][1] = BestColor(pe, r, g, b);
	}  }
	for(c = 0; c < 256; c++){  for(i = 0; i < 256; i++){
			r = (pe[c].peRed + pe[i].peRed) >>1;
			g = (pe[c].peGreen + pe[i].peGreen) >>1;	//r,g,b, hold best-color.
			b = (pe[c].peBlue + pe[i].peBlue) >>1;
			Mix4[c][i][2] = BestColor(pe, r, g, b);
	}  }
	for(c = 0; c < 256; c++){  for(i = 0; i < 256; i++){
			Mix4[c][i][3] = Mix4[i][c][1];
	}  }
	//Now fill out new Add and Subtract tables.
	for(c = 0; c < 256; c++){
		for(i = c; i < 256; i++){	//We can compute only half the table.
			r = pe[c].peRed + pe[i].peRed; if(r > 255) r = 255;
			g = pe[c].peGreen + pe[i].peGreen; if(g > 255) g = 255;
			b = pe[c].peBlue + pe[i].peBlue; if(b > 255) b = 255;
			Add1[c][i] = Add1[i][c] = BestColor(pe, r, g, b);
		}
	}
	for(c = 0; c < 256; c++){
		for(i = 0; i < 256; i++){	//Full table must be computed.
			r = pe[c].peRed - pe[i].peRed; if(r < 0) r = 0;
			g = pe[c].peGreen - pe[i].peGreen; if(g < 0) g = 0;
			b = pe[c].peBlue - pe[i].peBlue; if(b < 0) b = 0;
			Sub1[i][c] = BestColor(pe, r, g, b);
		}
	}
	//And the Lighting table.
	for(c = 0; c < 256; c++){
		for(i = 0; i < LIGHT_SHADES; i++){
			r = (pe[c].peRed * i) / (LIGHT_SHADES - 1);
			g = (pe[c].peGreen * i) / (LIGHT_SHADES - 1);
			b = (pe[c].peBlue * i) / (LIGHT_SHADES - 1);
			Light1[i][c] = BestColor(pe, r, g, b);
		}
	}

#endif
	if(disk){
		if((f = fopen(fn, "wb")) != NULL){
			fwrite(pe, sizeof(PaletteEntry), 256, f);
			fwrite(Mix4, 1, tablesize, f);
			fclose(f);
		}
	}
	return true;
}


int ColorOctree::ColorCount = 0;
int ColorOctree::ColorIndex = 0;
PaletteEntry *ColorOctree::pep = NULL;

//Returns numeric octant that col is in relation to split.
inline int ColorOctree::Octant(PaletteEntry col){
	if(col.peGreen < Split.peGreen){	//Lower in Green.
		if(col.peRed < Split.peRed){	//Lower in Red.
			if(col.peBlue < Split.peBlue){	//Lower in Blue.
				return 0;
			}else{	//Greater Equal in Blue.
				return 1;
			}
		}else{	//Greater Equal in Red.
			if(col.peBlue < Split.peBlue){	//Lower in Blue.
				return 2;
			}else{	//Greater Equal in Blue.
				return 3;
			}
		}
	}else{	//Higher Equal in Green.
		if(col.peRed < Split.peRed){	//Lower in Red.
			if(col.peBlue < Split.peBlue){	//Lower in Blue.
				return 4;
			}else{	//Greater Equal in Blue.
				return 5;
			}
		}else{	//Greater Equal in Red.
			if(col.peBlue < Split.peBlue){	//Lower in Blue.
				return 6;
			}else{	//Greater Equal in Blue.
				return 7;
			}
		}
	}
}
//Returns the splitting plane for the child in the specified octant and with the specified size based on the current splitting plane.
inline PaletteEntry ColorOctree::SplitPlane(int octant){
	PaletteEntry pe;
	int size = Size >>1;	//Size is assumed to be full size of child node, se we add half to find splitting plane.
	if(Size == 1){	//Must special case this!  Emulate a div by 2 result of 0.5!  We still subtract, but we do not add, because of >= rule!
		if(octant >>2) pe.peGreen = Split.peGreen;
		else pe.peGreen = Split.peGreen - 1;
		if((octant & 3) > 1) pe.peRed = Split.peRed;
		else pe.peRed = Split.peRed - 1;
		if(octant & 1) pe.peBlue = Split.peBlue;
		else pe.peBlue = Split.peBlue - 1;
	}else{
		if(octant >>2) pe.peGreen = Split.peGreen + size;
		else pe.peGreen = Split.peGreen - size;
		if((octant & 3) > 1) pe.peRed = Split.peRed + size;
		else pe.peRed = Split.peRed - size;
		if(octant & 1) pe.peBlue = Split.peBlue + size;
		else pe.peBlue = Split.peBlue - size;
	}
	return pe;
}
void ColorOctree::Clear(){
	Size = 128;
	Rt = Gt = Bt = Count = 0;
	Split.peRed = Split.peGreen = Split.peBlue = 128;
	Index = -1;	//An index of -1 means no palette index set.
}
ColorOctree::ColorOctree(){
	Prev = NULL;
	for(int o = 0; o < 8; o++) Next[o] = NULL;
	Clear();
}
//Creates a child node with splitting plane split, and child size of size.
ColorOctree::ColorOctree(ColorOctree *prev, PaletteEntry split, int size){
	Prev = prev;
	for(int o = 0; o < 8; o++) Next[o] = NULL;
	Size = size;
	Rt = Gt = Bt = Count = 0;
	Split = split;
	Index = -1;
}
ColorOctree::~ColorOctree(){
	DeleteTree();
}
void ColorOctree::DeleteTree(){	//Deletes all children recursively, DOES NOT TOUCH this node.
	for(int o = 0; o < 8; o++){
		if(Next[o]){
			delete Next[o];
			Next[o] = NULL;
		}
	}
	Clear();
}

//Adds color col to tree.  If split and col match, becomes leaf node, otherwise passes on to children or makes new kids.
ColorOctree *ColorOctree::Add(PaletteEntry col){
	if(Count > 0 || (Size <= 0 && col.peRed == Split.peRed && col.peGreen == Split.peGreen && col.peBlue == Split.peBlue)){
		//We are either a leaf already, or we are at the last tree level and we match the color, so make into leaf.
		Rt += col.peRed;
		Gt += col.peGreen;
		Bt += col.peBlue;
		Count++;
		return this;
	}else{	//Nope, gotta keep diving.
		int octant = Octant(col);	//Find octant we should dive into.
		if(Next[octant] == NULL){	//No node, so make one.
			Next[octant] = new ColorOctree(this, SplitPlane(octant), Size >>1);
		}
		if(Next[octant]) return Next[octant]->Add(col);
		return NULL;
	}
}
int ColorOctree::CountLeaves(){
	int n = 0;
	if(Count) n += 1;	//We are a leaf.
	for(int o = 0; o < 8; o++) if(Next[o]) n += Next[o]->CountLeaves();	//Might be leaves below too.
	return n;
}
//Reduces current node and all children into perent node, if present.
void ColorOctree::Reduce(int size, int cols){
	for(int o = 0; o < 8; o++) if(Next[o]) Next[o]->Reduce(size, cols);
	//
//	if(Next[1]) Next[1]->Reduce(size, cols);	//Lr, Lg, Hb
//	if(Next[6]) Next[6]->Reduce(size, cols);	//Hr, Hg, Lb
//	if(Next[4]) Next[4]->Reduce(size, cols);	//Lr, Hg, Lb
//	if(Next[7]) Next[7]->Reduce(size, cols);	//Hr, Hg, Hb
//	if(Next[0]) Next[0]->Reduce(size, cols);	//Lr, Lg, Lb
//	if(Next[3]) Next[3]->Reduce(size, cols);	//Hr, Lg, Hb
//	if(Next[5]) Next[5]->Reduce(size, cols);	//Lr, Hg, Hb
//	if(Next[2]) Next[2]->Reduce(size, cols);	//Hr, Lg, Lb
	//
	if(Size < size && ColorCount > cols && Prev && Count){
		if(Prev->Count > 0) ColorCount--;	//If we're reducing into an existing leaf, reduce the total leaf count.
		Prev->Count += Count;
		Prev->Rt += Rt;
		Prev->Gt += Gt;	//Add our totals to parent.
		Prev->Bt += Bt;
		Count = 0;
		Rt = Gt = Bt = 0;	//Null our totals.
	}
}
//void ColorOctree::SetColorCount(){
//	ColorCount = Count();
//}
int ColorOctree::Reduce(int cols){
	if(cols <= 0) return 0;
	ColorCount = CountLeaves();	//Count total leaves.
	int size = 1;	//Start out only reducing size 0 nodes.
	while(ColorCount > cols){
		Reduce(size, cols);	//Recursive reduce.
		size = size <<1;	//Double size of reducible nodes.
	}
	return ColorCount;
}
void ColorOctree::GetPalette(int index){	//Privy recursive.
	if(Count && ColorCount > 0){	//Ve izt a nude!
		Rt /= Count;
		Gt /= Count;	//Get average color.
		Bt /= Count;
		pep->peRed = Rt;
		pep->peGreen = Gt;
		pep->peBlue = Bt;
		pep++;
		ColorCount--;
		index = ColorIndex++;
	}
	Index = index;
	for(int o = 0; o < 8; o++) if(Next[o]) Next[o]->GetPalette(index);
}
bool ColorOctree::GetPalette(PaletteEntry *pe, int cols){	//Public, root only.
	if(pe && cols > 0){
		pep = pe;
		ColorCount = cols;	//Now ColorCount holds how many pe entries we are to dig out.
		ColorIndex = 0;
		GetPalette(-1);	//Start diving with an index of -1, so all nodes that were never leaves will get -1, and be skipped on palette lookup.
		return true;
	}
	return false;
}
int ColorOctree::LookupIndex(PaletteEntry pe){
	if(Index >= 0 && pe.peRed == Split.peRed && pe.peGreen == Split.peGreen && pe.peBlue == Split.peBlue){	//We have a weiner!
		return Index;	//Only hit if Index is 0 or more, if it's -1 this is a real parent node and we should continue.
	}else{
		int octant = Octant(pe);
		if(Next[octant]) return Next[octant]->LookupIndex(pe);
		return 0;
	}
}
//***************************************************************************
//                               Below we have the Quantizer class.
//***************************************************************************
#define PEP (PaletteEntry*)
int CDECL PeCompare(const void *c1, const void *c2){
	return
		(
		(((PaletteEntry*)c1)->peRed <<1) +
		(((PaletteEntry*)c1)->peGreen <<2) +
		((PaletteEntry*)c1)->peBlue
		) - (
		(((PaletteEntry*)c2)->peRed <<1) +
		(((PaletteEntry*)c2)->peGreen <<2) +
		((PaletteEntry*)c2)->peBlue
		);
}

Quantizer::Quantizer(){
//	pal1 = realpal1;	//Points pointers at real palette arrays, so pointers can be swapped.
//	pal2 = realpal2;
	ClearPalette();
}
Quantizer::~Quantizer(){
}
bool Quantizer::ClearPalette(){
//	for(int c = 0; c < MAXCOLS; c++){
//		pal1[c].peRed = pal1[c].peGreen = pal1[c].peBlue = 0;
//		pal2[c].peRed = pal2[c].peGreen = pal2[c].peBlue = 0;
//	}
//	npal1 = npal2 = 0;
	octree.DeleteTree();
	return true;
}
bool Quantizer::AddPalette(PaletteEntry *pe, int NumCols, int NumShades){
	if(NumShades < 0 || pe == NULL) return false;
//	int Top = NumShades * 2;
//	int Bottom = NumShades;
	int Top = NumShades + 1;	//Not half intensity anymore.
	int Bottom = 1;
	int Shade = 0;
	int c = 0;//, found = 0, c2 = 0, r, g, b;
	if(NumShades == 0){
		Top = 1;
		Bottom = 1;
	}
	PaletteEntry col;
	//Generate NumShades extra levels of darkening (only going as low as half-intensity)
	//for input palette, plus the full-bright input palette.
	for(Shade = Top; Shade >= Bottom; Shade--){
		for(c = 0; c < NumCols; c++){
//			if(npal1 >= MAXCOLS) return false;
			//Get shaded version of color in input palette.
		//	r = (pe[c].peRed * Shade) / Top;
		//	g = (pe[c].peGreen * Shade) / Top;
		//	b = (pe[c].peBlue * Shade) / Top;

			//Use new ColorOctree.
			col.peRed = (pe[c].peRed * Shade) / Top;
			col.peGreen = (pe[c].peGreen * Shade) / Top;
			col.peBlue = (pe[c].peBlue * Shade) / Top;
			octree.Add(col);
			//Check for color already existing in palette.
/*			found = 0;
			for(c2 = 0; c2 < npal1; c2++){
				if(r == pal1[c2].peRed && g == pal1[c2].peGreen && b == pal1[c2].peBlue){
					found = 1;
					break;
				}
			}
			//Add color to global palette.
			if(found == 0 || npal1 <= 0){
				pal1[npal1].peRed = r;
				pal1[npal1].peGreen = g;
				pal1[npal1].peBlue = b;
				npal1++;
			}
*/		//	pal1[npal1++] = pe[c];
		}
	}
	return true;
}
bool Quantizer::GetCompressedPalette(PaletteEntry *pe, int NumCols){//, int level){
//	if(NumCols <= 0 || npal1 <= 0 || pe == 0) return false;
	if(NumCols <= 0 || pe == NULL) return false;

	octree.Reduce(NumCols);
	octree.GetPalette(pe, NumCols);
#ifdef TEST_CUBE_LERP
	SortPaletteCube(pe);
#else
	qsort(pe, NumCols, sizeof(PaletteEntry), PeCompare);
#endif

	return true;
}


//Inverse Palette Array class.  Creates inverse palette with variation of
//algorithm in Graphics Gems II, page 116, using a distance buffer to trace
//the convex hulls (worst-case cube hulls in my simpler implementation) of
//the colors entered into the inverse palette.
//
InversePal::InversePal(){
	inv_pal = NULL;
}
InversePal::InversePal(int rbits, int gbits, int bbits){
	inv_pal = NULL;
	Init(rbits, gbits, bbits);
}
InversePal::~InversePal(){
	Free();
}
void InversePal::Free(){
	if(inv_pal) free(inv_pal);
	inv_pal = NULL;
}
bool InversePal::Init(int rbits, int gbits, int bbits){
	Free();
	if(rbits < 0 || rbits > 8 || gbits < 0 || gbits > 8 || bbits < 0 || bbits > 8) return false;
	red_max = 1 <<rbits;
	green_max = 1 <<gbits;
	blue_max = 1 <<bbits;
	red_pow2 = rbits;
	green_pow2 = gbits;
	blue_pow2 = bbits;
	gb_pow2 = green_pow2 + blue_pow2;
//	gb_size = green_max * blue_max;	//Green+Blue entries in each Red slice.
	r_quant = 8 - rbits;
	g_quant = 8 - gbits;	//Amount to shift 888 color down to get inv pal color.
	b_quant = 8 - bbits;
	if(NULL == (inv_pal = (unsigned char*)malloc(red_max * green_max * blue_max))) return false;
	return true;
}
bool InversePal::Make(PaletteEntry *npe, int ncols){
	if(!inv_pal || npe == NULL || ncols <= 0 || ncols > 256) return false;
	int *inv_pal_dist;
	if(NULL == (inv_pal_dist = (int*)malloc(red_max * green_max * blue_max * sizeof(int)))) return false;
	int i, j;
	for(i = red_max * green_max * blue_max - 1; i >= 0; i--){
		inv_pal_dist[i] = 0x10000000;
	}
	memset(inv_pal, 0, red_max * green_max * blue_max);
	//
	unsigned char shuffle[256], ts;	//Randomize order that colors are entered.
	for(i = 0; i < 256; i++) shuffle[i] = i;
	for(i = 0; i < ncols; i++){
		j = rand() % ncols;
		ts = shuffle[i];
		shuffle[i] = shuffle[j];
		shuffle[j] = ts;
	}
	//
	for(i = 0; i < ncols; i++){
		pe[i] = npe[i];
		cur_color = shuffle[i];
		r_center = (npe[cur_color].peRed >>r_quant);// + (1 <<r_quant) / 2;
		g_center = (npe[cur_color].peGreen >>g_quant);// + (1 <<g_quant) / 2;
		b_center = (npe[cur_color].peBlue >>b_quant);// + (1 <<b_quant) / 2;
		red_init();
		red_loop(inv_pal_dist, inv_pal, r_center, 0);
	}
	free(inv_pal_dist);
	return true;
}
//Distances aren't being computed quite as correctly as they should be.  Dist
//is currently in inverse palette space, but it should be in 888 space.

inline void InversePal::blue_init(){
	b_skip1 = b_skip2 = 0;
}
inline bool InversePal::blue_loop(int *dbuf, unsigned char *cbuf, int center, int dist){
	int n, d, diter;
	bool b_found1, b_found2;
	b_found1 = false;
	d = dist + (diter = 0);
	if(!b_skip1){
		for(n = center; n < blue_max; n++){
			if(dbuf[n] > d){
				cbuf[n] = cur_color;
				dbuf[n] = d;
				b_found1 = true;
			}else if(b_found1) break;
			d += diter + (++diter);
		}
		if(!b_found1) b_skip1 = true;
	}
	b_found2 = false;
	d = dist + (diter = 1);
	if(!b_skip2){
		for(n = center - 1; n >= 0; n--){
			if(dbuf[n] > d){
				cbuf[n] = cur_color;
				dbuf[n] = d;
				b_found2 = true;
			}else if(b_found2) break;
			d += diter + (++diter);
		}
		if(!b_found2) b_skip2 = true;
	}
	return b_found1 | b_found2;
}
inline void InversePal::green_init(){
	g_skip1 = g_skip2 = 0;
}
inline bool InversePal::green_loop(int *dbuf, unsigned char *cbuf, int center, int dist){
	int n, d, diter;
	bool g_found1, g_found2;
	blue_init();
	g_found1 = false;
	d = dist + (diter = 0);
	if(!g_skip1){
		for(n = center; n < green_max; n++){
			if(blue_loop(dbuf + (n <<blue_pow2), cbuf + (n <<blue_pow2), b_center, d)){
				g_found1 = true;
			}else if(g_found1) break;
			d += diter + (++diter);
		}
		if(!g_found1) g_skip1 = true;
	}
	blue_init();
	g_found2 = false;
	d = dist + (diter = 1);
	if(!g_skip2){
		for(n = center - 1; n >= 0; n--){
			if(blue_loop(dbuf + (n <<blue_pow2), cbuf + (n <<blue_pow2), b_center, d)){
				g_found2 = true;
			}else if(g_found2) break;
			d += diter + (++diter);
		}
		if(!g_found2) g_skip2 = true;
	}
	return g_found1 | g_found2;
}
inline void InversePal::red_init(){
}
inline bool InversePal::red_loop(int *dbuf, unsigned char *cbuf, int center, int dist){
	int n, d, diter;
	bool r_found1, r_found2;
	green_init();
	r_found1 = false;
	d = dist + (diter = 0);
//	if(!r_skip1){
		for(n = center; n < red_max; n++){
			if(green_loop(dbuf + (n <<gb_pow2), cbuf + (n <<gb_pow2), g_center, d)){
				r_found1 = true;
			}else if(r_found1) break;
			d += diter + (++diter);
		}
//		if(!r_found1) r_skip1 = true;
//	}
	green_init();
	r_found2 = false;
	d = dist + (diter = 1);
//	if(!r_skip2){
		for(n = center - 1; n >= 0; n--){
			if(green_loop(dbuf + (n <<gb_pow2), cbuf + (n <<gb_pow2), g_center, d)){
				r_found2 = 1;
			}else if(r_found2) break;
			d += diter + (++diter);
		}
//		if(!r_found2) r_skip2 = true;
//	}
	return r_found1 | r_found2;
}

