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

//ResourceManager class, handling texture and mesh resources for voxel
//world entities.  Purpose is to provide a central storage location for
//all game graphics to facilitate remapping to a common palette.

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "Image.h"
#include "Terrain.h"
#include "CStr.h"
#include "Trees.h"
#include "Poly.h"
#include "FileManager.h"
#include "Quantizer.h"

#include "Sound.h"

#include "Directories.h"

// Stupid, stupid Windows defines.
#ifdef LoadImage
#undef LoadImage
#endif

#define MAXTERTEX 20
#define MAXECO 20
#define TERRAINSHADES 3

enum AlphaGradType
{
	ALPHAGRAD_NONE = 0x0,
	ALPHAGRAD_NORMAL = 0x1,
	ALPHAGRAD_ALPHAONLY1 = 0x2,
	ALPHAGRAD_ALPHAONLY2 = 0x3
};

//Forward declaration.
class ResourceManager;

class ImageNode : public LinklistBase<ImageNode>, public ImageSet {
public:
	CStr name;
	bool MipMap, Trans;
	AlphaGradType AlphaGrad;
	int SizeBias;	//Set SizeBias to the number of mip-levels below the max that this texture should be forced down to (for less important textures).
	int ForceMaxTexRes;	//Set this to minimum value that MaxTexRes should be when downloading this texture, to e.g. force a texture to be 256 even if global maxtexres is 128.
	float AlphaGradBias;	//Set this to other than 0.5f to bias the gradiated alpha up or down.
	int BlackOutX, BlackOutY;	//How many horizontal and vertical cells in texture, to blacken/dealpha two lines between each cell.
	bool Dynamic;	//Set for any texture that will be regularly redownloaded.
	bool Flushable;	//True if image can be freed after download, and is auto-reloadable.
	ResourceManager *RM;
	ImageNode(ResourceManager *rm, const char *n, bool mip = false, bool trans = false, AlphaGradType agrad = ALPHAGRAD_NONE) : name(n), MipMap(mip), Trans(trans), AlphaGrad(agrad), SizeBias(0), AlphaGradBias(0.5f), BlackOutX(0), BlackOutY(0), RM(rm), Dynamic(false), Flushable(false), ForceMaxTexRes(0) { };
	ImageNode *Find(const char *n, bool mip, bool trans, AlphaGradType agrad);
	//ImageNodes will now be differentiated by above parameters as well, as they affect final modifications to image.
};
class MeshNode : public LinklistBase<MeshNode>, public Mesh {
public:
	CStr name;
	ResourceManager *RM;
	MeshNode(ResourceManager *rm, const char *n) : name(n), RM(rm) { };
	MeshNode *Find(const char *n);
};
//typedef unsigned int SoundID;
class SoundNode : public LinklistBase<SoundNode>, public SoundBuffer {
public:
	CStr name;
	ResourceManager *RM;
	SoundNode(ResourceManager *rm, const char *n, bool loop = false) : name(n), RM(rm), id(-1) { bLoop = loop; };
	SoundNode *Find(const char *n, bool loop);
public:
	int id;
	operator int() const{ return id; };
	SoundNode &operator=(int d){ id = d; return *this; };
};

class ResourceManager{
public:
	Image Tex[MAXTERTEX];	//Indexed array of terrain texture bitmaps.
	EcoSystem Eco[MAXTERTEX];
	int nTex;
//	CStr TexName[MAXTERTEX];
	int Texels;
public:
	PaletteEntry pe[256];
	Quantizer Quant;
	MixTable Mix;
	InversePal InvPal;
	FileManager *pFM;
	LinklistBase<ImageNode> ImageNodeHead;
	LinklistBase<MeshNode> MeshNodeHead;
	LinklistBase<SoundNode> SoundNodeHead;

	Sound snd;

public:
	ResourceManager(FileManager *fm);
	int LoadIndexedImage(const char *name, int index);	//Attempts to load texture into slot index.
	int FreeIndexedImage(int index);
	int RemoveIndexedImage(int index);	//Removes and reshuffles to close gap.
	int FreeIndexedImages();
	int LoadIndexedFromEco();	//Attempts to load textures named in corresponding ecosystems.
//	int MakePalette(Quantizer *quant =  NULL);	//Optionally pass in a Quantizer with some colors already loaded in.
//	bool MakeMixTable();	//Computes mix table.
//	int RemapIndexedImages();	//Sets remap tables on indexed images.
//	int RemapNamedImages();	//Actually remaps currently loaded named images.
//	int MipMapNamedImages();	//Creates mipmaps for named images that need them.
	bool DownloadTextures(bool UpdateOnly = false);	//Downloads Named Images to OpenGL.
	void UndownloadTextures();	//Frees copies of Named images downloaded to OpenGL.
	void FreeTextures();	//Releases all named image nodes, so MAKE SURE THERE ARE NO REFERENCES LEFT!
	Image *GetImage(int index);	//Gets an indexed image by number.
	ImageNode *GetCreateImage(const char *n, int w, int h, int bpp, bool mipmap = false, bool trans = false, AlphaGradType agrad = ALPHAGRAD_NONE);	//Find bitmap by name.  Create if not already loaded.  Could be a Set of Images, but that's transparent unless needed.
	ImageNode *GetImage(const char *n, bool mipmap = false, bool trans = false, AlphaGradType agrad = ALPHAGRAD_NONE);	//Find bitmap by name.  Load if not already loaded.  Could be a Set of Images, but that's transparent unless needed.
	Mesh *GetMesh(const char *n, double scale, bool smooth);	//Find mesh by name.  Load if not already loaded.
	SoundNode *GetSound(const char *n, bool loop = false, float volumebias = 0.5f);
	int GetFileCRC(const char *n, unsigned int *crc, int *size);
	void FreeSound();
	void InitSound();	//Loaded sounds will NOT be lost over sound system free and init!
//	int FlushSounds();	//Flushes sound data from RAM so a DownloadSounds() will re-load from disk and re-resample/bitchange if need be.
	bool DownloadSounds(bool reload = false);	//Downloads and/or updates sounds to audio system (call every frame is ok).
	//Setting reload will re-load sounds from disk and re-apply bit and freq limits.
	void SetMaxTexRes(int size){ MaxTexRes = size; };
	void SetTrilinear(int b){ UseTrilinear = b; };
	void SetTextureFlush(bool flush);	//Set to 1 to free local copies of disk based textures after download.
	void SetMaxSoundBits(int bits);
	void SetMaxSoundFreq(int hertz);
	int GetMaxSoundBits();
	int GetMaxSoundFreq();
	void UsePalettedTextures(bool usepal){ UsePalTex = usepal; };
	void DisableResourceLoading(bool disable = true){ DisableLoading = disable; };	//Set to true to disable loading images and sounds, and only create tiny blank placeholders.  Use for Dedicated Servers.
	void UseCompressedTextures(bool compress){ CompressTex = compress; };
protected:
	int LoadImage(ImageNode *node);	//Loads or re-loads an image using stored file name.
	int LoadSound(SoundNode *node);	//Ditto a sound, re-does max bits and max rate clamping.
	int MaxTexRes;
	int UseTrilinear;
	bool UsePalTex;
	bool DisableLoading;
	bool CompressTex;
	int maxsoundbits, maxsoundfreq;
	bool TextureFlush;
};

#ifndef HEADLESS
int GLMipMap(Bitmap *bmp, int fmt, int trans, int maxres, int blackoutx, int blackouty, bool nomipmap = false, bool usepaltex = false, bool update = false);
int GLGenTextures(int n, unsigned int *a);
int GLDeleteTextures(int n, unsigned int *a);
#else
inline int GLMipMap(Bitmap *bmp, int fmt, int trans, int maxres, int blackoutx, int blackouty, bool nomipmap = false, bool usepaltex = false, bool update = false) { return 0; }
inline int GLGenTextures(int n, unsigned int *a) { return 0; }
inline int GLDeleteTextures(int n, unsigned int *a) { return 0; }
#endif

//Hmm, what to do about remapping...  Not remapping on GetImage loads makes it a little
//useless for automatically loading in images that didn't happen to be precached, unless
//you know they won't be precached and call a manual remap on the image once loaded
//anyway.  However loading images in real-time sucks performance balls so shouldn't be
//done anyway.  However again with True Color no remapping would be needed, so the
//whole GetImage() methodology would be perfect...  Oh well.

#endif
