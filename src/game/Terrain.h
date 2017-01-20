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
	Terrain Class - holds height and color map data for voxel rendering.
*/

//
// March 27th, 1999.
// With the software renderer looking like a sure fire no-go, I'm going
// to try turning the "Color Map" byte array into a Dark Map, which will
// contain the intensity of the color on that texel, _seperate_ from the
// lighting color, so by default all cmap voxels will be 255, and will
// only be decreased to add specifically dark areas such as craters.
// This will be used when doing dynamic terrain modification, so it is
// easy to re-light the terrain after morphing, and yet keep the correct
// blending of multiple crater scorches.  Possible dynamic lighting will
// be done seperately, with a re-texture-lighting pass to erase the
// dynamic light.  The downside is that completely arbitrary painting
// on terrain wouldn't work, unless there was a totally seperate paint
// color texture layer...  Or maybe if instead of a Dark Map, it was
// a color mapped detail addition map, with ranges of palette entries
// for different colors as well as darkness??  Hmm.
//

#ifndef TERRAIN_H
#define TERRAIN_H

#include "Image.h"
#include "IFF.h"
#include "Quantizer.h"

#define TFORM_FRACTAL 1
#define TFORM_SPLINE 2

#define NUM_SHADE 64
#define NUM_SHADED2 32
#define NUM_SHADE_SHIFT 6
#define NUM_SQRT 10000

#define TERRAIN_ENCODE_RAW 1
#define TERRAIN_ENCODE_QUADTREE 2

#define TEXID_ENCODE_RAW 1
#define TEXID_ENCODE_RLE 2

struct AvgRng{
	int	a, r;
};

//Arrgh.  ::sigh::  Ok, we'll have the World Editable Values, and the internal chewed on
//values best for texturing with...  All actions go through public interface, except Terrain texturing.
class EcoSystem{
friend class Terrain;
private:
	double minA, minSlope1, maxSlope1;	//Altitude is a percentage from 0 to 100.
	double maxA, minSlope2, maxSlope2;	//Slope is an angle from 0 to 90.
public:
	double minAlt, minAngle1, maxAngle1;
	double maxAlt, minAngle2, maxAngle2;
	Image *tex;
	char name[256];
	EcoSystem();
	~EcoSystem();
	void Init(double, double, double, double, double, double, Image *t = NULL, char *n = NULL);
	double MinAlt(){ return minAlt; };
	double MaxAlt(){ return maxAlt; };
	double MinAngle1(){ return minAngle1; };
	double MinAngle2(){ return minAngle2; };
	double MaxAngle1(){ return maxAngle1; };
	double MaxAngle2(){ return maxAngle2; };
	void MinAlt(double a);
	void MaxAlt(double a);
	void MinAngle1(double a);
	void MinAngle2(double a);
	void MaxAngle1(double a);
	void MaxAngle2(double a);
	EcoSystem &operator=(EcoSystem &eco);
};

//OpenGL (well, general polygon rendering) related stuff.
#define MAX_LOD_BUMP 1000
#define TEXIDSIZE 32
struct ByteRect{
	unsigned char x, y, w, h;
	ByteRect() : x(0), y(0), w(0), h(0) {};
	ByteRect(unsigned char X, unsigned char Y, unsigned char W, unsigned char H) : x(X), y(Y), w(W), h(H) {};
};
//
class Terrain{
friend class RenderEngine;
public:	//OpenGL functions.
	bool MapLod();
	bool MapLod(int x, int y, int w, int h);
		//Flags areas of high irregularity for increased LOD at distance.
	bool DownloadTextures();//int paltextures = 0);	//Set to 1 to attempt to use palettized textures.
		//Downloads texture segments for terrain to OpenGL.  Must call TextureLight32 first!
	void UndownloadTextures();
	int UpdateTextures(int x1, int y1, int w, int h);
	int MakeTexturePalette(EcoSystem *eco, int numeco);	//Makes a map-specific texture palette, for use with paletted terrain textures.
	void UsePalettedTextures(bool usepal);
protected:	//OpenGl again.
//	int LodBump[MAX_LOD_BUMP][2];	//Which LOD cells should have their detail bumped up?
//	int nLodBump;
public:	//ick...  public...
	unsigned int TexIDs[TEXIDSIZE][TEXIDSIZE];
	ByteRect TexDirty[TEXIDSIZE][TEXIDSIZE];
	int Redownload(int tx, int ty);
	//
protected:
#define INVPALBITS 6
	InversePal TerrainInv;
	Quantizer TerrainQuant;
	int PalTextures;
protected:
//	unsigned char	*hdata;		//Height data
//	unsigned char	*cdata;		//Color data
	unsigned short	*data;	//Storage for height and color data interleaved.
	unsigned char	*texid;	//TextureID number map.
	int		width;
	int		height;
	int		widthmask;
	int		heightmask;
	int		widthpow;
	int		heightpow;
	unsigned char	ShadeLookup[256][NUM_SHADE];
	unsigned char	ShadeLookup32[256][NUM_SHADE];
//	int		SqrtLookup[NUM_SQRT];
	float	LightX, LightY, LightZ;	//Vector TOWARDS global sun, in +Z = forwards coords.
	float	Ambient;
	int		ScorchTex;
public:
	unsigned long	*data32;
//	unsigned long	*ShadeLookup32;
public:
	void RotateEdges();	//Rotates edges into center or vice versa for painting wrappability.
	Terrain();
	~Terrain();
	bool Init(int x, int y);
	bool Clear();
	bool Free();
	bool Load(IFF *iff, int *numecoptr, EcoSystem *eco, int maxeco, bool mirror = false);
	//Use IFF taking versions of Load and Save to tack other data on end, such as Entities.
	bool Load(const char *name, int *numecoptr, EcoSystem *eco, int maxeco, bool mirror = false);
	bool Save(IFF *iff, EcoSystem *eco, int numeco);
	bool Save(const char *name, EcoSystem *eco, int numeco);
	//
	void SetLightVector(float lx, float ly, float lz, float amb = 0.25f);
	int ClearCMap(unsigned char val);
	void SetScorchEco(int eco = -1);	//Sets an eco system to use as the Scorch texture, instead of black.
	int GetScorchEco();
	//
	bool FractalForm(int level, int min, int max, int Form1, int Form2, EcoSystem *eco, int numeco, void (*Stat)(Terrain*const, const char*, int) );
	bool InitTextureID();
	int EcoTexture(EcoSystem *eco, int numeco, int x, int y);	//EcoSystems the TexID for a specific point.
	bool Texture(EcoSystem *eco, int numeco, bool UseIDMap = false, int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0);
	bool RemapTexID(unsigned char *remap);	//256 entry remapping table.
	bool Lightsource(int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0);
	int Blow(unsigned char *dest, int dw, int dh, int dp,
		int sx = 0, int sy = 0, int sx2 = 0, int sy2 = 0, int flag = 0, int destbpp = 8, PaletteEntry pe[256] = NULL);
		//sx, sy, sx2, sy2 offsets are used to blit a specific rectangle of the map data to
		//the dest surface.  THE TOP LEFT OF THE MAP IS STILL EQUAL TO THE TOP LEFT OF THE
		//dest POINTER REGION!  This is designed for a dest region that is 1 to 1 with the
		//map in size.  flag specifies 1 for height or 0 for color map data.
	float GetSlope(int x, int y, float lx, float ly, float lz);
	int Width(){ return width; };
	int Height(){ return height; };
	bool MakeShadeLookup(InversePal *inv);//PALETTEENTRY *pe);
	//float lx = -0.7071, float ly = 0.7071, float lz = 0,
	bool TextureLight32(EcoSystem *eco, int numeco, int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0, bool UseShadeMap = true);
	int WrapX(int x) {return x & widthmask;}
	int WrapY(int y) {return y & heightmask;}
	//
#define CALCIDX(x, y) (((x) & widthmask) + (((y) & heightmask) << widthpow))
	int GetRGB(int x, int y, unsigned char *r, unsigned char *g, unsigned char *b){
		if(data32){
			unsigned char *t = (unsigned char*)(data32 + CALCIDX(x, y));
			*r = t[0];
			*g = t[1];
			*b = t[2];
			return 1;
		}
		return 0;
	}
	void PutRGB(int x, int y, unsigned char r, unsigned char g, unsigned char b){
		if(data32){
			unsigned char *t = (unsigned char*)(data32 + CALCIDX(x, y));
			t[0] = r;
			t[1] = g;
			t[2] = b;
		}
	}
	unsigned char GetT(int x, int y){
	//	if(texid == NULL || x < 0 || y < 0 || x >= width || y >= height) return 0;
	//	return *(texid + x + y * width); };
		if(!texid) return 0;
		return *(texid + CALCIDX(x,y));
	};
	unsigned char GetTraw(int x, int y){
		return *(texid + x + (y <<widthpow)); };
	//	return *(texid + x + y * width); };
	unsigned char GetTwrap(int x, int y){
		if(!texid) return 0;
		return *(texid + CALCIDX(x,y));
	};
	unsigned char GetH(int x, int y){
	//	if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return 0;
	//	return *(data + x + y * width) >> 8; };
		if(!data) return 0;
		return *(data + CALCIDX(x,y)) >>8;
	};
		//Oops, already had a seperate wrapping GetH...  Hmm...
		//This'll need to be straightened out at some point.
	unsigned char GetHraw(int x, int y){
		return *((unsigned char*)data + ((x + (y <<widthpow)) <<1) + 1); };
	//	return *(data + x + y * width) >> 8; };
	unsigned char GetHwrap(int x, int y){
	//	return *(data + (x & widthmask) + ((y & heightmask) <<widthpow)) >> 8; };
		if(!data) return 0;
		return *((unsigned char*)data + (CALCIDX(x, y) <<1) + 1); };
	unsigned short GetBigH(int x, int y);
	unsigned char GetC(int x, int y){
		if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return 0;
		return *(data + x + y * width) & 0xff; };
	unsigned char GetCraw(int x, int y){
		return *((unsigned char*)data + ((x + (y <<widthpow)) <<1)); };
	unsigned char GetCwrap(int x, int y){
	//	return *(data + CALCIDX(x, y)) & 0xff; };
		if(!data) return 0;
		return *((unsigned char*)data + (CALCIDX(x, y) <<1)); };
//	bool SetH(int x, int y, UCHAR v){
//		if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
//		*(data + x + y * width) = GetC(x, y) | (v << 8);  return true; }
	//SetH now bounds checks the input value, and takes an int instead of a char.
	bool SetT(int x, int y, int v){
		if(texid == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
		*(texid + x + y * width) = v;
		return true; };
	void SetTraw(int x, int y, int v){
		*(texid + x + y * width) = v; };
	bool SetH(int x, int y, int v){
		if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
		*(data + x + y * width) = GetCraw(x, y) | ((v > 255 ? 255 : (v < 0 ? 0 : v)) << 8);
		return true; };
	void SetHraw(int x, int y, int v){
		*(data + x + y * width) = GetCraw(x, y) | ((v > 255 ? 255 : (v < 0 ? 0 : v)) << 8); };
	void SetHwrap(int x, int y, int v){
		if(!data) return;
	//	*(data + CALCIDX(x, y)) = GetCwrap(x, y) | ((v > 255 ? 255 : (v < 0 ? 0 : v)) << 8); };
		*((unsigned char*)data + (CALCIDX(x, y) <<1) + 1) = ((v > 255 ? 255 : (v < 0 ? 0 : v))); };
		//
	bool SetBigH(int x, int y, unsigned short v);
	bool SetC(int x, int y, unsigned char v){
		if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
		*(data + x + y * width) = (GetH(x, y) << 8) | v;  return true; };
	bool SetCraw(int x, int y, unsigned char v){
		*(data + x + y * width) = (GetHraw(x, y) << 8) | v;  return true; };
	void SetCwrap(int x, int y, unsigned char v){
		if(!data) return;
		*((unsigned char*)data + (CALCIDX(x, y) <<1)) = v; };
		//
	int GetI(int x, int y);	//Get an interpolated height, passing in FP x,y and getting back VP sized voxel height data.
	float FGetI(float x, float y);	//Get an interpolated height, passing in FP x,y and getting back VP sized voxel height data.
//	double GetSlope(int x, int y, int lx, int ly, int lz);
protected:
	AvgRng Average(int x, int y, int p);
//	AvgRng AverageBig(int x, int y, int p);
	int FastSqrt(int x, int y, int z);
};

#endif
