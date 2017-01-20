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
	Image & Bitmap - Tool classes to load 256 color bitmaps (Windows BMPs right now)
	into raw data and palette buffers.
	Extension of RawBMP class.
	Adding multiple-image support and mip-map generation.
*/
//Ok, figured out a way that might work to simplify the whole bitmap with palette
//and bitmaps sharing palette thing.  Each Bitmap will have a POINTER to a palette
//and a remap table, that it will use if the passed in palette/remap pointers are
//null, for functions requiring them.  They will be externally controlled arrays,
//guaranteed by the user to exist for the life of the Bitmap, if used.  Also the
//ImageSet class will go away, and the Image class will have an operator[] added
//to access multiple Bitmaps.  Op[] will return a REFERENCE to a Bitmap, with [0]
//returning the Bitmap that the Image object is derived from.

//So, the ppe and premap pointers in the Bitmap are private except to descendants,
//like the Image class.  At the moment the Image class is the ONLY place where
//those pointers will be set!  And they WILL NOT CHANGE for the life of the
//Bitmap and parent Image class, they will always point to the Image's fixed pe
//and remap arrays.

#ifndef IMAGE_H
#define IMAGE_H

#include <new>
#include <iostream>
#include <cstring>
#include <algorithm>

#include <cstdlib>
#include "Quantizer.h"
#include "CStr.h"

struct ARGB{
	union{
		unsigned long argb;
		unsigned short argb16;	//Can NOT be set through use of b, g, r, and a.
		struct{
			unsigned char b, g, r, a;
		};	// FIXME: anonymous structs are BAD!
		unsigned char byte[4];
	};
};

bool BGRAfromPE(ARGB *argb, PaletteEntry *pe, unsigned char alpha = 0);	//Fills an array of 256 longs with 32bit ARGB intel format color entries matching 256 entry PE array, for quick 8bit to 32bit conversion.
bool RGBAfromPE(ARGB *argb, PaletteEntry *pe, unsigned char alpha = 0);	//Same in OpenGL style format.

inline int HiBit(int num){
	int bit = 31;
	while(bit){
		if((unsigned long)num & (unsigned long)((unsigned long)1 << bit)) return bit;
		bit--;
	}
	return 0;
};	//Returns the number of the highest bit set, starting from 0.

bool MakeRemapTable(unsigned char *Remap, const PaletteEntry *oldpe, const PaletteEntry *pe);
	//Makes a remapping table to remap from oldpe to pe.
bool MakeRemapTable(unsigned char *Remap, const PaletteEntry *oldpe, const InversePal *inv);
	//Makes a remapping table to remap from oldpe to pe.

class Bitmap;

bool LoadBMP(const char *name, Bitmap *bmp, PaletteEntry *pe);
	//Opens file and hands off to file stream based load function.
bool LoadBMP(FILE *f, Bitmap *bmp, PaletteEntry *pe);
	//Loads a .BMP file from the current seek position in the file
	//stream.  Suitable for loading from uncompressed "pack" archive
	//files.
bool LoadPackedBMP(void *bmi, Bitmap *bmp, PaletteEntry *pe);
	//Loads a packed format BMP from RAM, with a pointer to the
	//BITMAPINFOHEADER structure starting the packed bitmap.
bool SaveBMP(const char *name, Bitmap *bmp, PaletteEntry *pe, bool noflip = false);
	//Ditto.
bool SaveBMP(FILE *f, Bitmap *bmp, PaletteEntry *pe, bool noflip = false);
	//Ditto for saving a .BMP to any location in a larger file.
	//Will overwrite data after seek position.

//*******************************************************************
//                                Bitmap
//*******************************************************************
//Bare 8-bit image bits, plus line-based registration info.
//No palette or palette handling.
//Now supports 16, 24 and 32bit bitmap storage, but not all members work in high color.
//Bitmap flags:
#define BFLAG_ROTATED	0x01
//Bitmap has been rotated.
#define BFLAG_REMAPPED	0x02
//Bitmap has been remapped (not just had remap tables set).
#define BFLAG_MIPMAP	0x04
//Bitmap is a mip-map of another bitmap.
#define BFLAG_QUANTIZED	0x08
//Bitmap has been quantized from high color to paletted.
#define BFLAG_CLEARED	0x10
//Bitmap has been cleared since init.
#define BFLAG_SCALED	0x20
//Bitmap has been scaled.
#define BFLAG_COLORCORRECTED	0x40
//Bitmap has been color corrected.
#define BFLAG_WANTDOWNLOAD	0x80
//Bitmap wants redownload to friendly neighborhood graphics card.
#define BFLAG_BYTEORDER_RGBA	0x100
//Bitmap is in OpenGL-style RGBA byte order, rather than DIB style BGRA.
#define BFLAG_DONTDOWNLOAD	0x200
//Bitmap does NOT want to be downloaded to graphics card.
#define BFLAG_HFLIPPED		0x400
//Indicate the bitmap has been flipped horizontally.
//
class Bitmap{
friend class Image;
private:
	void SetBPP(int b){
		bpp = b;
		pixelpitch = bpp / 8;
		alphaskip = (pixelpitch > 3 ? pixelpitch - 3 : 0);
		colorbytes = (pixelpitch > 3 ? 3 : pixelpitch);
	};
	int nLines;
	int *LineLeft;
	int *LineRight;
	int xhot, yhot;
	int CalcPitch(int w, int b){ return ((w * (b >>3)) + 3) & (~3); };
	int CalcLength(int w, int h, int b){ return CalcPitch(w, b) * std::abs(h); };
protected:
	unsigned char *data;
	int width, height, pitch, bpp;	//bpp currently must be 8 or 24/32!
	int pixelpitch, colorbytes, alphaskip;
	PaletteEntry *ppe;
	unsigned char *premap;
public:
	int id;	//User-specifiable ID number, for e.g. OpenGL texture caching.  Set to 0 on init.
	int flags;	//Flags that record what has been done to the bitmap.  Set to 0 on init.
//	unsigned int xmask, xshift;	//ONLY VALID FOR POW2-WIDTH IMAGES!
//	unsigned int ymask, yshift;	//Andable-masks for dimensions, and shift-values (in bytes).
//	void MakeMasks();
public:
	Bitmap();
	Bitmap(const Bitmap &bmp);
//	Bitmap(int x, int y, int bpp = 8){ Bitmap(); Init(x, y, bpp); };
	virtual ~Bitmap();	//Make destructors virtual!
	Bitmap& operator=(const Bitmap &bmp);
	void Free();	//Free not virtual!!!
//	void Palette(PALETTEENTRY *pe){ ppe = pe; };	//Set and get palette pointer.  POINTED TO ARRAY MUST EXIST THROUGH LIFE OF BITMAP!
	PaletteEntry *Palette(){ return ppe; };
//	void RemapTable(unsigned char *remap){ premap = remap; };	//Set and get 256 entry remap table pointer.  DITTO!
	unsigned char *RemapTable(){ return premap; };
	bool Init(int w, int h, int bpp = 8);
		//Frees and/or allocates new image data for w and h dimensions.
	bool Init(Bitmap *bmp){	//Clones geometry of bitmap.
		if(bmp) return Init(bmp->Width(), bmp->Height(), bmp->BPP());
		return false;
	};
	bool Clear(unsigned char color = 0);
		//Clears bitmap to supplied value.
	bool To32Bit(PaletteEntry *pe = NULL);
		//Converts a 24bit or 8bit image to 32bit.
	bool RotateRight90();
		//Rotates image.
	bool RotateLeft90();
		//Ditto.
	bool ScaleToPow2();
		//If need be, scales width and height to nearest (lower) power of 2 size.
	bool Scale(int wnew, int hnew, bool lerp = false);
		//Scales image to new arbitrary dimensions.
	bool FlipH();
		//Flips bitmap horizontally
	bool InitAnalyze();
		//Allocates line registration data tables for height of bitmap.
	bool AnalyzeLines();
		//Analyzes amount of color-0 space at beginning and end of each line.
	void FreeAnalyze();
		//Frees data tables.
	unsigned int GetPixel(int x, int y);
	bool PutPixel(int x, int y, unsigned int pixel);
		//Sanity-checked high-level pixel plotting funcs.
	bool Suck(void *src, int w, int h, int pitch, int sbpp = 0);
		//Sucks bytes from source data into bitmap data.  sbpp is Source BPP, source and bitmap must both be 8bit or both some flavour of true color.
		//New, bitmap can be 8 bit and source true color, and blit will average to grayscale.
	bool Suck(Bitmap *bmp){
		if(bmp) return Suck(bmp->Data(), bmp->Width(), bmp->Height(), bmp->Pitch(), bmp->BPP());
		return false;
	};
	bool SuckARGB(Bitmap *bmp, ARGB *argb);
		//Sucks from an 8-bit bitmap to high/true-color self using ARGB array for pixel conversion.
	bool Blit(void *dest, int dw, int dh, int dp, int dx = 0, int dy = 0, bool trans = false);
		//Clipped and optionally transparent blit.  Provide top-left pointer of the
		//entire destination graphics area, and full width and height values.
	bool Blit(void *dest, int dw, int dh, int dp, int dx, int dy, int sx, int sy, int sw, int sh, bool trans = false);
		//Version of same that accepts a rectangle in the source bitmap too.
//	bool Blit(Bitmap *bmp){
//		if(bmp) return Blit(bmp->Data(), bmp->Width(), bmp->Height(), bmp->Pitch());
//		return false;
//	};
	bool Blit(Bitmap *bmp, int dx = 0, int dy = 0, bool trans = false){
		if(bmp) return Blit(bmp->Data(), bmp->Width(), bmp->Height(), bmp->Pitch(), dx, dy, trans);
		return false;
	};
	bool Blit(Bitmap *bmp, int dx, int dy, int sx, int sy, int sw, int sh, bool trans = false){
		if(bmp) return Blit(bmp->Data(), bmp->Width(), bmp->Height(), bmp->Pitch(), dx, dy, sx, sy, sw, sh, trans);
		return false;
	};
	bool BlitRaw(void *dest, int pitch, int insetx, int insety, int w, int h, bool trans = false);
		//Unclipped blit.  Provide top-left pointer of where bitmap should land.
	bool BlitRaw8to32(uint32_t *dest, int dpitch, int insetx, int insety, int w, int h, bool trans = false, ARGB *argb = NULL);
		//Unclipped blit from 8bit source to 32bit dest using supplied ARGB table.
	bool Quantize32to8(Bitmap *destb, PaletteEntry *pe = NULL, int cols = 256, int prequantize = 0);	//Destination palette.  prequantize is number of low bits to throw out in each color to speed quantization.
	bool Quantize32to8HighQuality(Bitmap *destb, PaletteEntry *pe = NULL);
		//High quality (slow) color quantization.
	bool MakeMipMap(Bitmap *Source, MixTable *Mix, int mixmode = MIX50, bool trans = false);
		//Creates half-sized (with mixing) image in current bitmap of source bitmap.
		//Set mixmode to the define for the type of mixing you would like.  Higher
		//values (e.g. MIX75) will bias towards the top-left pixel of each box sampled
		//quartet from the source.  Set trans to true to zero all dest pixels when
		//the top-left of the source box sample is zero, instead of mixing a really
		//dark output color if say only one of the other 3 pixels is non-trans.
	bool Remap(unsigned char *RemapTable = NULL);
		//Remaps bitmap using 256 byte remap table provided.
	bool ColorCorrect(float rgain, float ggain, float bgain, float again,
					float rbias, float gbias, float bbias, float abias,
					float rscale, float gscale, float bscale, float ascale,
					float rshift, float gshift, float bshift, float ashift);
	bool GammaCorrect(const float fGamma);
public:
	// TODO: Make platform independent
	bool LoadBMP(const char *bmp){ return ::LoadBMP(bmp, this, ppe); };
	bool SaveBMP(const char *bmp, bool noflip = false){ return ::SaveBMP(bmp, this, ppe, noflip); };
	bool LoadBMP(FILE *f){ return ::LoadBMP(f, this, ppe); };
	bool SaveBMP(FILE *f, bool noflip = false){ return ::SaveBMP(f, this, ppe, noflip); };
	bool LoadBMP8(const char *bmp){	//Forces the loaded bitmap down to 8 bit, if true color.
		FILE *f;
		if(bmp && (f = fopen(bmp, "rb"))){
			if(LoadBMP8(f)){
				fclose(f);
				return true;
			}
			fclose(f);
		}
		return false;
	};
	bool LoadBMP8(FILE *f){
		if(LoadBMP(f)){
			if(bpp >= 24){
				Bitmap tbmp;
				if(Quantize32to8(&tbmp, ppe)){
					*this = tbmp;
					return true;
				}else{
					return false;
				}
			}
			return true;
		}
		return false;
	};
	bool LoadPackedBMP(void *bmi){ return false; }//::LoadPackedBMP(bmi, this, ppe); };
public:
	int Width(){ return width;};
	int Height(){ return height;};
	int Pitch(){ return pitch;};
	int BPP(){ return bpp;};
	unsigned char *Data(){ return data;};
	int XHot(){ return xhot; };
	int YHot(){ return yhot; };
	void Hotspot(int x, int y){ xhot = x; yhot = y; };
	int LineL(int n){ if(n >= 0 && n < nLines) return LineLeft[n]; return 0; };
		//Number of linear transparent pixels on left of specified line.
	int LineR(int n){ if(n >= 0 && n < nLines) return LineRight[n]; return 0; };
		//Ditto the right side of line, moving left.
};

//*******************************************************************
//                                Image
//*******************************************************************
//Subclass of Bitmap with palette and loading added, suitable as drop in
//replacement for single-image RawBMP.
class Image : public Bitmap{
friend class ImageSet;
private:
	Bitmap *bmp;	//Array/pointer for extra bitmaps.
	int nBitmaps;	//Starts at 1, and includes parent bitmap object.
public:
	//Note, overloaded copy constructors and assignment operators don't automatically
	//copy the base class!  Be sure to manually copy it, if copy/assign ops are added
	//for a derived class like this one.
	PaletteEntry pe[256];
	unsigned char remap[256];
	ARGB tcremap[256];	//High or True-Color reverse indexing table for 8bit images.
	Image(int n = 0) : bmp(NULL), nBitmaps(1) {
		ppe = pe;
		premap = remap;
		InitRemapTable();
		if(n > 1) InitSet(n);
	};
	Image(const Image &img) : bmp(NULL), nBitmaps(1) {
		ppe = pe;
		premap = remap;
		InitRemapTable();
		*this = img;
	};
	virtual ~Image(){
		Free();
	};
	Image &operator=(const Image &img){
		if(&img == this) return *this;	//Identity.
		InitSet(img.nBitmaps);
	//	if(img.nBitmaps > 1){
		for(int i = 0; i < (nBitmaps - 0); i++) (*this)[i] = img[i];// (*this)[i] = img[i];
	//	}
		SetPalette(img.pe);
		SetRemapTable(img.remap);
	//	*((Bitmap*)this) = *((Bitmap*)&img);	//Copy underlying bitmap.
		return *this;
	};
	Bitmap &operator[](int n) const {
		if(n > 0 && n < nBitmaps && bmp) return bmp[n - 1];
		return *((Bitmap*)this);
	};
	void FreeSet(){
		if(bmp) delete [] bmp;
		bmp = NULL;
		nBitmaps = 1;
	};
	void Free(){	//Argh, virtual is bad here!
		FreeSet();
		InitPalette();
		Bitmap::Free();
	};
	bool InitSet(int n){	//Ok, now InitSet does NOT harm the original bitmap!!
		FreeSet();
		if(n == 1) return true;
		if(n > 1){
			if((bmp = new Bitmap[n - 1])){
				nBitmaps = n;
				for(int i = 0; i < (n - 1); i++){
					bmp[i].ppe = pe;
					bmp[i].premap = remap;
				}
				return true;
			}
		}
		return false;
	};
	bool InitBitmaps(int x, int y, int bpp){
		if(nBitmaps > 0){
			for(int i = 0; i < nBitmaps; i++){
				(*this)[i].ppe = pe;
				(*this)[i].premap = remap;
				if(false == (*this)[i].Init(x, y, bpp)) return false;
			}
			return true;
		}
		return false;
	};
	bool InitSet(int n, int x, int y, int bpp){
		if(InitSet(n) && InitBitmaps(x, y, bpp)) return true;
		else return false;
	};
	int Bitmaps(){ return nBitmaps; };
	void InitPalette(){
		std::memset(pe, 0, sizeof(pe));
		InitRemapTable();
	};
	bool Init(int x, int y, int bpp = 8){
		InitPalette();
		return Bitmap::Init(x, y, bpp);
	};
	bool SetPalette(const PaletteEntry *newpe){
		if(newpe){
			for(int i = 0; i < 256; i++) pe[i] = newpe[i];
			return true;
		}
		return false;
	};
	bool GetPalette(PaletteEntry *newpe){
		if(newpe){
			for(int i = 0; i < 256; i++) newpe[i] = pe[i];
			return true;
		}
		return false;
	};
	bool InitRemapTable(){
		for(int i = 0; i < 256; i++) remap[i] = i;	//1-1 remap table.
		return true;
	};
	bool InitRemapTable(const PaletteEntry *newpe){
		if(newpe) return MakeRemapTable(remap, pe, newpe);	//Remap table based on newpe.
		return false;
	};
	bool InitRemapTable(const InversePal *inv){
		if(inv) return MakeRemapTable(remap, pe, inv);	//Remap table based on inverse palette.
		return false;
	};
	bool SetRemapTable(const unsigned char *rmp){	//rmp is 256 element array.
		if(rmp){
			memcpy(remap, rmp, 256);
			return true;
		}
		return false;
	};
	bool Remap(const PaletteEntry *newpe = NULL){	//Remaps actual image bits of all contained Bitmaps.
		bool ret = true;
		if(newpe) InitRemapTable(newpe);
		for(int i = 0; i < nBitmaps; i++){
			if(false == (*this)[i].Remap()) ret = false;
		}	//Leave setting the palette to the one remapped to up to the app.
		if(newpe){	//Ok, I lied, set the palette and init the remap table, if a palette is provided.
			InitRemapTable();
			SetPalette(newpe);
		}
		return ret;
	};
	bool Remap(const InversePal *inv){	//Remaps actual image bits of all contained Bitmaps.
		bool ret = true;
		if(inv) InitRemapTable(inv);
		for(int i = 0; i < nBitmaps; i++){ if(false == (*this)[i].Remap()) ret = false; }
		if(inv){
			InitRemapTable();
			SetPalette(inv->pe);
		}
		return ret;
	};
	bool RotateRight90(){
	//	if(bmp && nBitmaps){
		for(int n = 0; n < nBitmaps; n++) if(false == (*this)[n].RotateRight90()) return false;
		return true;
	//	}
	//	return false;
	};
	bool RotateLeft90(){
	//	if(bmp && nBitmaps){
		for(int n = 0; n < nBitmaps; n++) if(false == (*this)[n].RotateLeft90()) return false;
		return true;
	//	}
	//	return false;
	};
	bool ScaleToPow2(){
	//	if(bmp && nBitmaps){
		for(int n = 0; n < nBitmaps; n++) if(false == (*this)[n].ScaleToPow2()) return false;
		return true;
	//	}
	//	return false;
	};
	bool AnalyzeLines(){
	//	if(bmp && nBitmaps){
		for(int n = 0; n < nBitmaps; n++) if(false == (*this)[n].AnalyzeLines()) return false;
		return true;
	//	}
	//	return false;
	};
	bool MakeMipMap(MixTable *Mix, int mixmode = MIX50, bool trans = false){
		if(InitSet(HiBit(std::max(Width(), Height())))){
	//	if(bmp && nBitmaps){
			for(int n = 1; n < nBitmaps; n++) if(false == (*this)[n].MakeMipMap(&(*this)[n - 1], Mix, mixmode, trans)) return false;
			return true;
		}
		return false;
	};
};

//*******************************************************************
//                                ImageSet
//*******************************************************************
class ImageSet : public Image {
private:
	Image *img;
	CStr *names;
	int nImages;
public:
	ImageSet(int n = 0) : img(NULL), names(NULL), nImages(1) {
		if(n > 1) InitSet(n);
	};
	~ImageSet(){
		Free();
	};
	ImageSet(const ImageSet &img) : img(NULL), names(NULL), nImages(1) {
		*this = img;
	};
	ImageSet &operator=(const ImageSet &img){
		if(&img == this) return *this;
		InitSet(img.Images());
		for(int n = 0; n < img.Images(); n++) (*this)[n] = img[n];
		if(names && img.names && nImages == img.Images()){
			for(int n = 0; n < img.Images(); n++) names[n] = img.names[n];
		}
		return *this;
	};
	Image &operator[](int n) const {
		if(n > 0 && n < nImages && img) return img[n - 1];
		return *((Image*)this);
	};
	int Images() const { return nImages; };
	void FreeSet(){
		if(img) delete [] img;
		img = NULL;
		if(names) delete [] names;
		names = NULL;
		nImages = 1;
	};
	void Free(){	//Argh, virtual is bad here!
		FreeSet();
		Image::Free();
	};
	bool InitSet(int n){	//Ok, now InitSet does NOT harm the original bitmap!!
		FreeSet();
		if(n > 0){
			if(names = new CStr[n]){
				if(n == 1) return true;
				if(n > 1){
					if((img = new Image[n - 1])){
						nImages = n;
						return true;
					}
				}
			}
		}
		return false;
	};
	CStr GetName(int n){
		if(names && n >= 0 && n < nImages){
			return names[n];
		}
		return CStr();
	};
	bool SetName(int n, const char *s){
		if(names && s && n >= 0 && n < nImages){
			names[n] = s;
			return true;
		}
		return false;
	};
	Image *FindImage(const char *s){
		if(names){
			for(int n = 0; n < nImages; n++){
				if(CmpLower(s, names[n])) return &(*this)[n];
			}
		}
		return NULL;
	};
	int FindImageIndex(const char *s){
		if(names){
			for(int n = 0; n < nImages; n++){
				if(CmpLower(s, names[n])) return n;
			}
		}
		return -1;
	};
	bool InitRemapTable(){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].InitRemapTable()) return false;
		return true;
	};
	bool InitRemapTable(const PaletteEntry *newpe){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].InitRemapTable(newpe)) return false;
		return true;
	};
	bool InitRemapTable(const InversePal *inv){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].InitRemapTable(inv)) return false;
		return true;
	};
	bool SetRemapTable(const unsigned char *rmp){	//rmp is 256 element array.
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].SetRemapTable(rmp)) return false;
		return true;
	};
	bool Remap(const PaletteEntry *newpe = NULL){	//Remaps actual image bits of all contained Bitmaps.
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].Remap(newpe)) return false;
		return true;
	};
	bool Remap(const InversePal *inv){	//Remaps actual image bits of all contained Bitmaps.
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].Remap(inv)) return false;
		return true;
	};
	bool RotateRight90(){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].RotateRight90()) return false;
		return true;
	};
	bool RotateLeft90(){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].RotateLeft90()) return false;
		return true;
	};
	bool ScaleToPow2(){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].ScaleToPow2()) return false;
		return true;
	};
	bool AnalyzeLines(){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].AnalyzeLines()) return false;
		return true;
	};
	bool MakeMipMap(MixTable *Mix, int mixmode = MIX50, bool trans = false){
		for(int n = 0; n < nImages; n++) if(false == (*this)[n].MakeMipMap(Mix, mixmode, trans)) return false;
		return true;
	};
	bool LoadSet(FILE *f);
	bool LoadSet(const char *n);
};

#endif
