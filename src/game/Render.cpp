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
	By Seumas McNally
*/

//#define ASM_SMOOTH
//#define ASM_FLAT
//#define ASM_INTERPOLATE

#include "VoxelAfx.h"
#include <cmath>
#include <cstdio>
#include "Image.h"
#include "Terrain.h"
#include "Render.h"
#include "CamRbd.h"

using namespace std;

RenderEngine::RenderEngine(){
	Sky = NULL;
	Detail = NULL;
	DetailMT = NULL;
	Water = NULL;
	WaterAlpha = 0.75f;
	WaterReflect = 0.5f;
	FogR = FogG = FogB = 0.6f;
	msecs = 0;
	UseSkyBox = false;
	for(int i = 0; i < 6; i++) SkyBox[i] = NULL;
	skyrotatespeed = 0.0f;
	detailrotrad = 0.0f;
	detailrotspeed = 0.0f;

//	LastZlVp = 0;
//	Clookup[0] = FPVAL;
//	for(int c = 1; c < 1024; c++){	//Voxel column-width-scalar lookup table.
//		Clookup[c] = FPVAL / c;
//	}
//	for(int m = 2; m < 1024; m++){
//		Mlookup[m] = 256 / m;
//		SMlookup[m] = std::min((256 * 4) / m, 255);
//	}
//	Mlookup[0] = Mlookup[1] = 255;
//	SMlookup[0] = SMlookup[1] = 255;

//	for(int z = 0; z < MAX_Z; z++){
//		Zoffset[z] = 0;
//	}

	RedMask = RedMaskLen = RedMaskOff = 0;
	GreenMask = GreenMaskLen = GreenMaskOff = 0;
	BlueMask = BlueMaskLen = BlueMaskOff = 0;
#ifdef MARKMIXTABLE
	for(int a = 0; a < 256; a++){
		for(int b = 0; b < 256; b++){
			MarkMix[a][b] = 0;
		}
	}
#endif
}
RenderEngine::~RenderEngine(){
}
void RenderEngine::UnlinkTextures(){
	SkyENV = NULL;
	Sky = NULL;
	for(int i = 0; i < 6; i++) SkyBox[i] = NULL;
	DetailMT = NULL;
	Detail = NULL;
	Water = NULL;
	UseSkyBox = false;
}
bool RenderEngine::SetSkyRotate(float rotspeed){
	skyrotatespeed = rotspeed;
	return true;
}
bool RenderEngine::SetDetailRot(float rotspeed, float rotrad){
	detailrotspeed = rotspeed;
	detailrotrad = rotrad;
	return true;
}
bool RenderEngine::SetSkyTexture(Image *sky, Image *skyenv){
	if(skyenv){// && skyenv->Data()){
		SkyENV = skyenv;
	}else{
		SkyENV = NULL;
	}
	if(sky){// && sky->Data()){
		Sky = sky;
		UseSkyBox = false;
		return true;
	}else{
		Sky = NULL;
		return false;
	}
}
bool RenderEngine::SetSkyBoxTexture(int face, Image *tex){
	if(face >= 0 && face < 6 && tex){
		SkyBox[face] = tex;
		UseSkyBox = true;
		return true;
	}
	return false;
}

bool RenderEngine::SetDetailTexture(Image *detail, Image *detailmt){
	if(detailmt){// && detailmt->Data()){
		DetailMT = detailmt;
	}else{
		DetailMT = NULL;
	}
	if(detail){//->Data()){
		Detail = detail;
		return true;
	}else{
		Detail = NULL;
		return false;
	}
}
bool RenderEngine::SetWaterTexture(Image *water, float wateralpha, float waterscale, float waterenv){
	WaterAlpha = wateralpha;
	WaterScale = waterscale;
	WaterReflect = waterenv;
	if(water){// && water->Data()){
		Water = water;
		return true;
	}else{
		Water = NULL;
		return false;
	}
}
bool RenderEngine::SetFogColor(float r, float g, float b){
	FogR = r;
	FogG = g;
	FogB = b;
	return true;
}

/*
bool MixTable::MakeLookup(PALETTEENTRY *pe, bool disk){
	if(NULL == pe) return false;

	int r, g, b, c, i, k, diff, col, t, OK;
	unsigned char tempfooey;
	const char fn[] = "MixLookup.dat";
	const int tablesize = 256 * 256 * 4;
	PALETTEENTRY tpe[256];
	FILE *f;

	//Now the lookup is saved, and if the saved lookup palette is the same as the palette
	//that a lookup has to be made for, the old lookup is loaded in.
	if(disk){
		if((f = fopen(fn, "rb")) != NULL){
			fread(tpe, sizeof(PALETTEENTRY), 256, f);
			OK = 1;
			for(i = 0; i < 256; i++){
				if(	pe[i].peRed != tpe[i].peRed ||
					pe[i].peGreen != tpe[i].peGreen ||
					pe[i].peBlue != tpe[i].peBlue){
					OK = 0;
				}
			}
			if(OK == 1 && fread(Mix4, 1, tablesize, f) == tablesize){
				fclose(f);
				return true;
			}
			fclose(f);
		}
	}

	for(c = 0; c < 256; c++){  for(i = 0; i < 256; i++){
		if(i < 256 - 8) tempfooey = Mix4[c][i + 8][0];
			Mix4[c][i][0] = c;
			r = ((pe[c].peRed <<7) + (pe[c].peRed <<6) + (pe[i].peRed <<6)) >>8;
			g = ((pe[c].peGreen <<7) + (pe[c].peGreen <<6) + (pe[i].peGreen <<6)) >>8;	//r,g,b, hold best-color.
			b = ((pe[c].peBlue <<7) + (pe[c].peBlue <<6) + (pe[i].peBlue <<6)) >>8;
			diff = 2048; col = 0;	//Last color reg with lowest difference found.
			for(k = 0; k < 256; k++){
				t = (abs(pe[k].peRed - r) <<1) + (abs(pe[k].peGreen - g) <<2) + abs(pe[k].peBlue - b);
				if(t < diff){ diff = t; col = k; }
			}
			Mix4[c][i][1] = col;
	}  }
//	for(c = 0; c < 256; c++){  for(i = 0; i < 256; i++){
//	}  }
	for(c = 0; c < 256; c++){  for(i = 0; i < 256; i++){
			r = (pe[c].peRed + pe[i].peRed) >>1;
			g = (pe[c].peGreen + pe[i].peGreen) >>1;	//r,g,b, hold best-color.
			b = (pe[c].peBlue + pe[i].peBlue) >>1;
			diff = 2048; col = 0;	//Last color reg with lowest difference found.
			for(k = 0; k < 256; k++){
				t = (abs(pe[k].peRed - r) <<1) + (abs(pe[k].peGreen - g) <<2) + abs(pe[k].peBlue - b);
				if(t < diff){ diff = t; col = k; }
			}
			Mix4[c][i][2] = col;
	}  }
	for(c = 0; c < 256; c++){  for(i = 0; i < 256; i++){
			Mix4[c][i][3] = Mix4[i][c][1];
	}  }

	if(disk){
		if((f = fopen(fn, "wb")) != NULL){
			fwrite(pe, sizeof(PALETTEENTRY), 256, f);
			fwrite(Mix4, 1, tablesize, f);
			fclose(f);
		}
	}

/*	for(c = 0; c < 256; c++){	//Color register 1, 66% weight.
		for(i = 0; i < 256; i++){	//Register 2, 33% weight.
			r = (pe[c].peRed * 170 + pe[i].peRed * 86) >>8;
			g = (pe[c].peGreen * 170 + pe[i].peGreen * 86) >>8;	//r,g,b, hold best-color.
			b = (pe[c].peBlue * 170 + pe[i].peBlue * 86) >>8;
			col = 0;	//Last color reg with lowest difference found.
			diff = 2048;
			for(k = 0; k < 256; k++){
				t = (abs(pe[k].peRed - r) << 1) + (abs(pe[k].peGreen - g) << 2) + abs(pe[k].peBlue - b);
				if(t < diff){
					diff = t;
					col = k;
				}
			}
			Mix33[c][i] = col;
		}
	}*/
/*	return true;
}
*/
bool RenderEngine::Init16BitTables(int rmask, int gmask, int bmask, PaletteEntry *pe, unsigned short *C816){
	RedMask = (rmask &= 0xffff);
	GreenMask = (gmask &= 0xffff);
	BlueMask = (bmask &= 0xffff);
	do{
		if(rmask & 1){ RedMaskLen++; }else{ RedMaskOff++; }
		rmask = rmask >>1;
	}while(rmask > 0);
	do{
		if(gmask & 1){ GreenMaskLen++; }else{ GreenMaskOff++; }
		gmask = gmask >>1;
	}while(gmask > 0);
	do{
		if(bmask & 1){ BlueMaskLen++; }else{ BlueMaskOff++; }
		bmask = bmask >>1;
	}while(bmask > 0);
	RedMaskLen2 = RedMaskLen * 2;
	GreenMaskLen2 = GreenMaskLen * 2;
	BlueMaskLen2 = BlueMaskLen * 2;
	RedMaskOff2 = RedMaskOff + RedMaskLen;
	GreenMaskOff2 = GreenMaskOff + GreenMaskLen;
	BlueMaskOff2 = BlueMaskOff + BlueMaskLen;
	RedMask2 = (RedMask << RedMaskOff2) | (RedMask << RedMaskOff);
	GreenMask2 = (GreenMask << GreenMaskOff2) | (GreenMask << GreenMaskOff);
	BlueMask2 = (BlueMask << BlueMaskOff2) | (BlueMask << BlueMaskOff);
	int col;
	for(int c = 0; c < 256; c++){
		Color8to16[c] = col = ((pe[c].peRed >> (8 - RedMaskLen)) << RedMaskOff) |
								((pe[c].peGreen >> (8 - GreenMaskLen)) << GreenMaskOff) |
								((pe[c].peBlue >> (8 - BlueMaskLen)) << BlueMaskOff);
		Color8to16x[c] = ((col & RedMask) << RedMaskOff2) |
							((col & GreenMask) << GreenMaskOff2) |
							((col & BlueMask) << BlueMaskOff2);
		//This lets the caller get a copy of the lookup table in their own array.
		if(C816) C816[c] = col;
	}
	return true;
}


#define plot(x,y,v) (*((UCHAR*)rbd->data + (x) + (y) * lPitch) = v)

//		x2 = (x * cos(a)) - (y * sin(a));
//		y2 = (x * sin(a)) + (y * cos(a));
void RotateIntVect(int x, int y, int *xd, int *yd, float a){
	int sn = (int)(sin(a * (float)DEG2RAD) * (float)FPVAL);
	int cs = (int)(cos(a * (float)DEG2RAD) * (float)FPVAL);
	*xd = (x * cs - y * sn) >>FP;
	*yd = (y * cs + x * sn) >>FP;
}

//***********************************************************
//
//	Renders a voxel landscape to a DirectX surface.
//	*map - pointer to Terrain object with height and color data
//	*ddsd - pointer to a DDSURFACEDESC structure returned from a
//		successful Lock() on a DirectDraw surface.
//	*cam - pointer to a Camera object with view information.
//	flags - rendering options.
//	viewdist - the total distance from the camera that terrain
//		will be visible.
//	perfectdist - the distance that terrain will be rendered
//		perfectly, after that more voxels are skipped, speeding
//		up rendering.
//
//***********************************************************

#if 0

bool RenderEngine::TerrainRender(Terrain *map, Camera *cam, PolyRender *poly,
								MixTable *mix, RendBufDesc *rbd, int flags,
								int viewdist, int perfectdist){

	if(map->data == NULL || map->width <= 0 || map->height <= 0){
		return false;
	}

	//If the last time the table was made the viewplane was different, re-make it.
	int viewplane = cam->viewplane >> VOXSQUEEZE;
	if(viewplane != LastZlVp){
		LastZlVp = viewplane;
		for(int z = 1; z < MAX_Z; z++){
			Zlookup[z] = (int)((double)(LastZlVp <<(FP + 2)) / ((double)z / ZFPVAL));// >>VP;
		}
	}

	viewdist = std::min(viewdist, MAX_Z - 1);
	perfectdist = std::min(perfectdist, MAX_Z - 1);
	int mwidth = map->width, mheight = map->height;
	int mwidth8 = (mwidth - 1) << FP, mheight8 = (mheight - 1) << FP;	//One voxel smaller in width and height so interpolation doesn't read out of bounds.
	int mwidth16 = (mwidth - 1) << FP2, mheight16 = (mheight - 1) << FP2;	//One voxel smaller in width and height so interpolation doesn't read out of bounds.
//	UCHAR *hdata = map->hdata;
//	UCHAR *cdata = map->cdata;
	//USHORT *data = map->data;	//Copies made in local variables for possible speed up.
	UCHAR *data = (UCHAR*)map->data;
	int mwidth2 = mwidth * 2;	//Nummber of BYTES in map's width.

	UCHAR *surf;
	USHORT *surf16;
	//USHORT voxel, voxel2, voxel3, voxel4;
	//USHORT *tdata;
	//USHORT *ltdata;
	UCHAR *tdata;
	//UCHAR *ltdata;
	int lPitch = rbd->pitch;
	int width = rbd->width, height = rbd->height;
	int width2 = width / 2, height2 = height / 2;
	int column;
	//, colstep = 1, halfres = 0,
	int z, dx, dy;
	//, odx, ody,
	int tx, ty;
	int cx, cy, cxend, cyend;
	int alt = (int)(cam->y * (float)FPVAL);// >> VP;
//	UCHAR maph, mapc;
	int lsy, sy, syd, line;	//Y of last voxel strip on screen, and y of current one.
	//int viewangle = (int)(atan2f((double)width2, (double)cam->viewplane) * RAD2DEG * 2);
	//float anglestart = cam->b - viewangle / 2;
	float ViewVect = (float)width2 / (float)cam->viewplane;
	//For every 1.0 forwards, this many to the side.
//	float angle = (float)-(viewangle / 2);
//	float angledelta = (float)viewangle / (float)width;
	//double dxyfix;
//	int ;
	int i, j, k, t, kstart, kend;
	int mainx, mainy, maindx, maindy, maindxend, maindyend;
//	int across, acrossd,
	int acrossz, acrossx, zx;
	int columnd, col, lastcol, c, zscale, cend, linet;
	int shearstart, shear, sheard, rolldelta;

	bool tflag = false;
	int totalvoxels = 0, visvoxels = 0;
	//, voxtop, voxbot;
	//UINT mapc, lmapc, mapcy;
	UCHAR mapc, lmapc, mapcy;
//	int mapcd, mapcyd;
	int clookupt, xfrac, yfrac;

	int xpow = map->widthpow + 1;	//Map is power of two sized now.
									//Add one to multiply by 2 to get BYTES shift amount.
	//, ixfrac, iyfrac;
//	int mapcrd, mapcgd, mapcbd, mapcyrd, mapcygd, mapcybd, mapcr, mapcg, mapcb;
//	UINT mapcad, mapcsd, mapcyad, mapcysd;
	int voxfloor, interpolate;
	int smooth = flags & REND_SMOOTH, rend16bit = flags & REND_16BIT;

	int ybuf1[1024];
	int ybuf2[1024];
	int *ybuf = ybuf1, *oybuf = ybuf2, *ybufp, *tempybufp;
	UCHAR cbuf1[1024];
	UCHAR cbuf2[1024];
	UCHAR *cbuf = cbuf1, *ocbuf = cbuf2, *cbufp, *tempcbufp;
	for(i = 0; i < 1024; i++){
		ybuf1[i] = height;
		ybuf2[i] = height;
		cbuf1[i] = 128;
		cbuf2[i] = 128;
	}

	UCHAR *datayaddr[1000];
	for(i = 0; i < rbd->height; i++) datayaddr[i] = (UCHAR*)rbd->data + rbd->pitch * i;

	UCHAR HMix = 0, HMixD = 0, VMix = 0, VMixD = 0;
	unsigned long int HMixV = 0, VMixV = 0;
	UCHAR *HMixP = NULL;
	UCHAR *VMixP = NULL;

	UCHAR *Mix4 = &mix->Mix4[0][0][0];

	//ASM assisting variables.
	UCHAR *rbddata = (UCHAR*)rbd->data;
	UCHAR *MlookupP = Mlookup;
	UCHAR *SMlookupP = SMlookup;
	int *ClookupP = Clookup;
//	UCHAR *Mix4P = &Mix4[0][0][0];
	UCHAR *Mix4P = Mix4;
	ULONG *datayaddrP = (ULONG*)&datayaddr[0];
	ULONG HMixReg = 0;

	int CurPolyObj = 0;

	timer.Start();

	//Set up vectors.
	//dxyfix is the amount to multiply by so that each maindx/y addition moves exactly 1 forward in the relative Z axis.
//	dxyfix = 1.0 / cos(fabsf(viewangle / 2 * DEG2RAD));
	//May have to change these cos/sins to work as left-handed coord system.  Done.  Not.
//	maindx = (int)(cos((anglestart - 90) * DEG2RAD) * FP2VAL * dxyfix);	//Note, all voxel coords are now at FP2 precision.
//	maindy = (int)(sin((anglestart - 90) * DEG2RAD) * FP2VAL * dxyfix);
//	maindxend = (int)(cos((anglestart - 90 + viewangle) * DEG2RAD) * FP2VAL * dxyfix);	//Note, all voxel coords are now at FP2 precision.
//	maindyend = (int)(sin((anglestart - 90 + viewangle) * DEG2RAD) * FP2VAL * dxyfix);

	RotateIntVect((int)(-ViewVect * (float)FP2VAL), -FP2VAL, &maindx, &maindy, cam->b);
	RotateIntVect((int)(ViewVect * (float)FP2VAL), -FP2VAL, &maindxend, &maindyend, cam->b);

	//Slight change here, we're going to use maindxy as the vector for how far to move
	//out, and maindxyend as the vector for how much to move right/left from there, so
	//each can be controlled by a different Z.
	maindx = (maindx + maindxend) / 2;	//Average left and right end vects to get forward vect.
	maindy = (maindy + maindyend) / 2;
	maindxend = maindxend - maindx;
	maindyend = maindyend - maindy;

	//	maindx = (int)(sin((anglestart) * DEG2RAD) * FP2VAL * dxyfix);	//Note, all voxel coords are now at FP2 precision.
	//	maindy = (int)(cos((anglestart) * DEG2RAD) * FP2VAL * dxyfix);
	//	maindxend = (int)(sin((anglestart + viewangle) * DEG2RAD) * FP2VAL * dxyfix);	//Note, all voxel coords are now at FP2 precision.
	//	maindyend = (int)(cos((anglestart + viewangle) * DEG2RAD) * FP2VAL * dxyfix);
//	acrossd = (int)(cos((viewangle / 2 - 90) * DEG2RAD) * 1 * FP2VAL);// * dxyfix);
	//	t = 180 - (viewangle / 2 + 90);
//	t = viewangle / 2 + 90;
	//left->right deltas now difference between left and right screen edge projections.
	rolldelta = (int)(width * cam->roll * FPVAL);
	shearstart = (int)(cam->pitch * (float)cam->viewplane * FPVAL) - rolldelta / 2;

	//New testing, z offsets for each voxel sample line, to tack lines down to terrain
	//at least when travelling in straight line, to especially make really low res long
	//distance sampling look better.
	float XMoved = cam->x - lastcam.x, YMoved = cam->z - lastcam.z;
	//Correct for moving backwards (should be negative DistanceMoved) later.
	int DistanceMoved = (int)(sqrtf(fabsf(XMoved * XMoved) + fabsf(YMoved * YMoved)) * (float)FPVAL);
	//Note, using new property of maindxy being straight line heading vectors.
	if((XMoved * maindx) + (YMoved * maindy) < 0) DistanceMoved = -DistanceMoved;
	lastcam = *cam;
	int LastZStep = 0;
	int RealZ = FPVAL * 2;	//Start Z at a sane distance from camera.
	int MaxAcross = std::min(width, 200);

#ifdef REND_HEIGHTCOLOR_ON
	int mapcadd;
	if(flags & REND_HEIGHTCOLOR) mapcadd = 1;
	else mapcadd = 0;
#endif

	//Render landscape.
	mainx = (int)(cam->x * (float)FP2VAL);//<<FP;	//Boosts it up to FP2 precision.
	//Negate z to switch from left handed to right handed (voxel array) coords.
	mainy = (int)(-cam->z * (float)FP2VAL);//<<FP;
//	z = 0;
	while(RealZ < viewdist <<FP){
		interpolate = 0;
		acrossx = 1;
		zx = 1;
		if(flags & REND_FILTER && RealZ < (perfectdist  - (perfectdist >>2)) <<FP){
			interpolate = 2;
//			if(z < (perfectdist >>1) <<FP) interpolate = 2;
		}
		if(flags & REND_OVERSAMPLE && RealZ < (perfectdist - (perfectdist >>2)) <<FP){
			acrossx = 2;
		//	zx = 1;
		//	zx = 2;
			if(RealZ < (perfectdist >>2) <<FP){
			////	acrossx = 3;
				zx = 2;
			//	zx = 4;  acrossx = 4;
	//			acrossx = 2;
//				interpolate = 2;
		/*		if(z < (perfectdist >>2) <<FP){
					acrossx = 4;
					zx = 3;
					if(z < (perfectdist >>3) <<FP){
						acrossx = 8;
						zx = 4;
					}
				}
		*/	}
		}
		//interpolate = 0;
	//	j = (FPVAL + FPVAL * ((z / FPVAL) / perfectdist)) / zx;
	//	j = FPVAL / zx;
		j = ((RealZ >>FP) / perfectdist);
		if(j <= 1){
			j = (FPVAL + (FPVAL * j)) / zx;
		}else{
			j = (FPVAL * j * j) / zx;
		//	j = (FPVAL * j * 1) / zx;
		}	//1, 2, 4, 9, 16, 25, 36, 49, 64, 81, 100, etc.
		//Old version, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, etc.
	//	j = (FPVAL << ((z / FPVAL) / perfectdist)) / zx;
		RealZ += j;

		//Recompute proper Zoffset for this z step.
		if(LastZStep != j){
			Zoffset[j] = (Zoffset[j] + DistanceMoved) % j;
			LastZStep = j;
		}
		//Apply to z, set from RealZ.

		z = RealZ - Zoffset[j];
	//	z = RealZ;

//		cx = mainx + ((maindx >>FP) * z);
//		cy = mainy + ((maindy >>FP) * z);
//		cxend = mainx + ((maindxend >>FP) * z);
//		cyend = mainy + ((maindyend >>FP) * z);
//		acrossz = std::max((int)(ViewVect * (float)acrossx * 2.0 * (float)z) >>FP, 1);
		cx = mainx + ((maindx >>FP) * z) + ((-maindxend >>FP) * z);
		cy = mainy + ((maindy >>FP) * z) + ((-maindyend >>FP) * z);
		cxend = mainx + ((maindx >>FP) * z) + ((maindxend >>FP) * z);
		cyend = mainy + ((maindy >>FP) * z) + ((maindyend >>FP) * z);
//		acrossz = std::max((across += (acrossd * j) >>FP) >>FP2, 1) * acrossx;
		acrossz = std::max((int)(ViewVect * (float)acrossx * 2.0 * (float)z) >>FP, 1);

		//Stop voxel samples getting any finer after a voxel is sampled every two columns.
		if(acrossz >= MaxAcross){
//			dx = (odx * acrossz) / width2 / acrossx;
//			dy = (ody * acrossz) / width2 / acrossx;
			acrossz = MaxAcross;
		}else{
//			dx = odx / acrossx;
//			dy = ody / acrossx;
		}
		dx = (cxend - cx) / acrossz;
		dy = (cyend - cy) / acrossz;
		zscale = Zlookup[z >>ZFP];
		column = 0;

	//	column = (rand() * 40);

		columnd = ((width <<FP2) + FP2VAL - 1) / acrossz;	//Double normal precision for columns.
		col = 0;
		lsy = 0;
		shear = shearstart;
		sheard = rolldelta / acrossz;
	//	voxfloor = (alt - (height2 + height / 5 - (shear - abs(rolldelta) >>FP) <<(FP + VP + 2)) / zscale) <<2;
		voxfloor = (alt - (height2 + height / 5 - (shear - abs(rolldelta) >>FP) <<(FP + VP + 2)) / zscale) >>6;

		//Bounds checking for this line.
		if( (dx == 0 && (cx < 0 || cx >= mwidth16))
			|| (dy == 0 && (cy < 0 || cy >= mheight16)) ) continue;	//Skips over whole k loop and prevents divide by zeroes.
		kstart = 0;
		if(cx < 0 || cy < 0 || cx >= mwidth16 || cy >= mheight16){
			if(cx < 0) kstart = std::max(abs(cx / dx) + 1, kstart);
			if(cy < 0) kstart = std::max(abs(cy / dy) + 1, kstart);
			if(cx >= mwidth16) kstart = std::max(abs((cx - mwidth16) / dx) + 1, kstart);
			if(cy >= mheight16) kstart = std::max(abs((cy - mheight16) / dy) + 1, kstart);
			cx += dx * kstart;
			cy += dy * kstart;
			col = (column += columnd * kstart) >> FP2;
			shear += sheard * kstart;
		}
//		tx = mainx + dx * acrossz;
//		ty = mainy + dy * acrossz;
		tx = cxend;
		ty = cyend;
		kend = 0;
		if(tx < 0 || ty < 0 || tx >= mwidth16 || ty >= mheight16){
			if(tx < 0) kend = std::max(abs(tx / dx) + 1, kend);
			if(ty < 0) kend = std::max(abs(ty / dy) + 1, kend);
			if(tx >= mwidth16) kend = std::max(abs((tx - mwidth16) / dx) + 1, kend);
			if(ty >= mheight16) kend = std::max(abs((ty - mheight16) / dy) + 1, kend);
		}
		kend = acrossz - kend;
		if(kstart >= kend) continue;	//Skip k loop, line out of bounds.
		ybufp = ybuf + col;
		cbufp = cbuf + col;

		//New trial procedure:  Initial voxel sample at cx/cy, and cx/cy incremented before
		//each subsequent sample instead of after.  Should be covered by clipping still.
	//	if(flags & REND_FILTER){
		//if(cx - dx > 0 && cx - dx < mwidth16 && cy - dy > 0 && cy - dy < mheight16){
//		//	voxel = *(data + (cx - dx >>FP2) + (cy - dy >>FP2) * mwidth);	//Both in one var.
		//	tdata = data + ((cx - dx >>FP2) <<1) + (cy - dy >>FP2) * mwidth2;
		//}else{
//		//	voxel = *(data + (cx >> FP2) + (cy >> FP2) * mwidth);	//Both in one var.
		tdata = data + ((cx >>FP2) <<1) + (cy >>FP2) * mwidth2;
		//}
	//	lsy = std::max(0, height2 + (shear >>FP) + (((alt - (*(tdata + 1) <<VP) ) * zscale) >>(FP + VP + 2))) <<FP;
		sy = std::max(0, height2 + (shear >>FP) + (((alt - (*(tdata + 1) <<VP) ) * zscale) >>(FP + VP + 2)));
		//if(rend16bit == 0){
//		//	mapc = (UCHAR)voxel <<FP;
#ifdef REND_HEIGHTCOLOR_ON
		mapc = *(tdata + mapcadd);
#else
		mapc = *tdata;
#endif
		//}else{
//		//	mapc = Color8to16x[(UCHAR)voxel];
//		////	mapc = Color8to16[(UCHAR)voxel];
		//}
		//lmapc = mapc;
		totalvoxels += kend - kstart;
		for(k = kstart; k < kend; k++){
			//totalvoxels++;
			lmapc = mapc;
			lsy = sy <<FP;
			//voxel = *(tdata = data + (cx >> FP2) + (cy >> FP2) * mwidth);	//Both in one var.
			cx += dx;  cy += dy;
		//	tdata = data + ((cx >>FP2) <<1) + (cy >>FP2) * mwidth2;
			tdata = data + ((cx >>FP2) <<1) + ((cy >>FP2) <<xpow);
			if(*(tdata + 1) /*<<8*/ < voxfloor){
				lsy = (sy = height + 1) <<FP;
#ifdef REND_HEIGHTCOLOR_ON
				mapc = *(tdata + mapcadd);
#else
				mapc = *tdata;
#endif
				lastcol = col;  col = (column += columnd) >> FP2;
				ybufp += col - lastcol;
				cbufp += col - lastcol;
				shear += sheard;
			//	cx += dx;  cy += dy;
				continue;
			}else{
				if(interpolate == 0){
					sy = height2 + ((shear += sheard) >>FP) + (((alt - (*(tdata + 1) <<VP) ) * zscale) >>(FP + VP + 2));	//Extract height.
#ifdef REND_HEIGHTCOLOR_ON
					mapc = *(tdata + mapcadd);
#else
					mapc = *tdata;
#endif
				}else{
					xfrac = (cx >>FP) & 0xff;
					yfrac = (cy >>FP) & 0xff;
					//Erm, always lerping color, interpolate of 2 required to lerp height?
					//Seems wrong to me, changing so height lerp is 1, color lerp is 2.
				//	if(interpolate == 2){
#ifdef ASM_INTERPOLATE
						//This is actually slower than the C version!
					__asm{
						mov esi, tdata;	//ESI permanently holds tdata.
						mov edx, yfrac;	//EDX is yfrac for next two pieces.
						inc esi;
						mov edi, mwidth2;	//EDI now holds mwidth2 for next sections.

						xor ebx, ebx;
						xor eax, eax;
						mov bl, [esi];	//EBX, upper left.
						mov al, [esi+edi];	//EAX, lower left.
						sub eax, ebx;
						shl ebx, FP;
						imul eax, edx;
						add eax, ebx;	//EAX is now original sy, left hand side lerp.

						add esi, 2;
						xor ecx, ecx;
						xor ebx, ebx;
						mov cl, [esi];	//ECX, upper right.
						mov bl, [esi+edi];	//EBX, lower right.
						sub ebx, ecx;
						shl ecx, FP;
						imul ebx, edx;
						add ebx, ecx;	//EBX is now right hand side lerp.

						sub ebx, eax;
						shl eax, FP;
						imul ebx, xfrac;
						add eax, ebx;	//EAX is now lerped height in 16.16 fp.
						mov ebx, alt;
						shr eax, (FP + VOXSQUEEZE);
						sub ebx, eax;	//EBX is now relative altitude of voxel to player.
						imul ebx, zscale;
						sar ebx, (FP + VP + 2);	//EBX is now relative pixel height of voxel.

						mov eax, sheard;
						mov ecx, shear;
						add ecx, eax;
						mov edx, height2;
						mov shear, ecx;
						sar ecx, FP;
						add ebx, ecx;
						add ebx, edx;
						mov sy, ebx;
					}
#else
					sy = (*(tdata + 1) <<FP) + yfrac * (*(tdata + 1 + mwidth2) - *(tdata + 1));	//Temporary calc.
					sy = height2 + ((shear += sheard) >>FP) +
						(((alt - (((sy <<FP) + xfrac *
						(((*(tdata + 3) <<FP) + yfrac * (*(tdata + 3 + mwidth2) - *(tdata + 3))) - sy)
						) >>(FP + VOXSQUEEZE)
						)) * zscale) >>(FP + VP + 2));	//Extract height.
#endif
				//	}else{
				//		sy = height2 + ((shear += sheard) >>FP) + (((alt - (*(tdata + 1) <<VP) ) * zscale) >>(FP + VP + 2));	//Extract height.
				//	}
#ifdef REND_HEIGHTCOLOR_ON
					if(interpolate == 2 && !mapcadd){	//Special version for rendering heights as colors.
						mapc = mix->Mix4[ mix->Mix4[*(tdata)][*(tdata + mwidth2)][yfrac >>6]
							][ mix->Mix4[*(tdata + 2)][*(tdata + 2 + mwidth2)][yfrac >>6]
							][xfrac >>6];
					}else{
						mapc = *(tdata + mapcadd);
					}
#else
					if(interpolate == 2){
//						mapc = Mix4[ Mix4[*(tdata)][*(tdata + mwidth2)][yfrac >>6]
//							][ Mix4[*(tdata + 2)][*(tdata + 2 + mwidth2)][yfrac >>6]
//							][xfrac >>6];
						mapc = mix->Mix4[ mix->Mix4[*(tdata)][*(tdata + mwidth2)][yfrac >>6]
							][ mix->Mix4[*(tdata + 2)][*(tdata + 2 + mwidth2)][yfrac >>6]
							][xfrac >>6];
					//	MARKMIX(*(tdata), *(tdata + mwidth2));
					//	MARKMIX(*(tdata + 2), *(tdata + 2 + mwidth2));
					}else{
						mapc = *tdata;
					}
#endif
				}
			}
			if(sy <= 0) sy = 0;
			lastcol = col;
			col = (column += columnd) >> FP2;

			if(smooth == 0){
#ifdef ASM_FLAT
			__asm{
				//Store lsy.
				mov eax, lsy;	//EAX now holds lsy for both loops.
			//	syd = ((sy << FP) - lsy) * Clookup[col - lastcol] >>FP;  if(syd < 0) syd++;
				mov ebx, col;
				mov esi, ClookupP;
				sub ebx, lastcol;
				mov edx, [esi+ebx*4];
				mov ecx, sy;
				sal ecx, FP;
				sub ecx, eax;
				imul edx, ecx;
				sar edx, FP;
				jns nsnoinc;
				inc edx;
				nsnoinc:
				mov syd, edx;
			//	Store mapc and lsy.
				mov dl, mapc;	//DL now holds mapc.
			//	for(c = lastcol; c < col; c++){
				mov ebx, lastcol;	//EBX now holds c loop counter.
				nscloop:
			//		if((t = (lsy += syd) >> FP) < *ybufp){
					mov esi, eax;
					mov edi, ybufp;	//Preserve EDI as ybufp till next section.
					add esi, syd;
					mov ecx, [edi];
					mov eax, esi;
					sar esi, FP;
					cmp esi, ecx;	//ESI is now being saved for later use as t.
					jns nsifend;
						//Set counter using t.
						mov ecx, [edi];	//ESI saved as t from earlier.
						sub ecx, esi;	//ECX is now inner loop counter.
			//			surf = (UCHAR*)rbd->data + c + t * lPitch;
					//	mov edi, lPitch;
					//	imul esi, edi;	//ESI is t from earlier.
					//	add esi, rbddata;
					//	add esi, ebx;
						//imul esi, edi;	//ESI is t from earlier.
						mov edi, datayaddrP;
						mov esi, [edi+esi*4];	//Replacing imul with table lookup.
						//add esi, rbddata;
						mov edi, lPitch;
						add esi, ebx;
			//			for(line = t; line < *ybufp; line++){
						nsinnerloop:
			//				*surf = mapc;  surf += lPitch;
							mov [esi], dl;
							add esi, edi;
							dec ecx;
							jnz nsinnerloop;
			//			}
			//			*ybufp = lsy >> FP;
						mov ecx, eax;
						mov edi, ybufp;
						sar ecx, FP;
						mov [edi], ecx;
			//		}
					nsifend:
			//		ybufp++;
					add edi, 4;
					mov ybufp, edi;
			//	}
					inc ebx;
					mov ecx, col;
					cmp ebx, ecx;
					js nscloop;
				mov lsy, eax;	//Store lsy after loops.
			}
#else
				syd = ((sy << FP) - lsy) * Clookup[col - lastcol] >>FP;  if(syd < 0) syd++;
					for(c = lastcol; c < col; c++){
						if((t = (lsy += syd) >> FP) < *ybufp){
						//	surf = (UCHAR*)rbd->data + c + t * lPitch;
							surf = (UCHAR*)datayaddr[t] + c;
							for(line = t; line < *ybufp; line++){
								*surf = mapc;  surf += lPitch;
							}
							*ybufp = lsy >> FP;
						}
						ybufp++;
					}
#endif
			}else{
				clookupt = Clookup[cend = col - lastcol];
				syd = ((sy << FP) - lsy) * clookupt >>FP;  if(syd < 0) syd++;
				if(rend16bit == 0){
					//cend = col - lastcol;
#ifdef ASM_SMOOTH
				__asm{
				//Combine H and V "MixReg" into one persistent register, and use rol reg, 16
				//to swap halves instead of loading/storing to ram.
				//Instead of storing pointer to mix table,
				//store 4 byte mix table entry itself.
				//Argh!  It's slower!  Argh!  ADC rules, and sucks because of that.
				//	HMixD = Mlookup[cend];
				//	HMix = 0;
					mov dh, 4;
					mov edi, cend;
					mov esi, SMlookupP;
					mov dl, [esi+edi];
					mov HMixReg, edx;
				//	HMixP = &Mix4[lmapc][mapc][0];
					xor eax, eax;
					xor ebx, ebx;
					mov al, lmapc;
					mov bl, mapc;
					shl eax, 10;
					//shl ebx, 2;
					//add eax, ebx;
					lea esi, [eax+ebx*4];
					add esi, Mix4P;	//eax now holds the HMixP.
					mov ebx, lastcol;
					mov HMixP, esi;
					//for(c = 0; c < cend; c++){
					mov c, ebx;
					cloop:
					//	if((t = (lsy += syd) >> FP) < *ybufp){
						mov esi, lsy;
						mov edi, ybufp;	//Preserve EDI as ybufp till next section.
						add esi, syd;
						mov eax, [edi];
						mov lsy, esi;
						shr esi, FP;
						cmp esi, eax;	//ESI is now being saved for later use as t.
						jns ifend;
						//After this point, all regs are set up and saved for inner loop use.
						//No registers will be preserved after this point.
						//	mapcy = *(HMixP + (HMix >>6));
						//	HMix += HMixD;
							mov ebx, HMixP;
							xor eax, eax;
							mov edx, HMixReg;
							mov al, [ebx];	//EAX will be preserved for later as mapcy.
							add dh, dl;
							adc ebx, 0;
							mov HMixReg, edx;
							mov HMixP, ebx;
						//	linet = *ybufp - t;
							//mov ebx, ybufp;
							mov ecx, dword ptr [edi];	//EDI is ybufp from before.
							sub ecx, esi;	//ECX is loop counter, ESI saved from before.
						//Don't touch ECX below.
						//	VMixD = Mlookup[linet];
							//xor edx, edx;
							mov dh, 4;	//Initial counter value.
							mov ebx, SMlookupP;
							//mov ebx, linet;
							mov dl, [ebx+ecx];	//dl holds mixing delta, dh is counter.
							//mov dx, [eax+ecx*2];	//dx is now delta, bx will be counter.
						//	VMixP = &Mix4[mapcy][*cbufp][0];
							mov edi, cbufp;
							shl eax, 10;
							xor ebx, ebx;
							//mov al, mapcy;	//EAX is mapcy from before.
	//						add eAx, Mix4P;
							mov bl, byte ptr [edi];
	//						mov eAx, [eAx + eBx*4];	//Now eAx holds VMixV, all 4 bytes of data.
							lea eax, [eax+ebx*4];
							//mov esi, Mix4P;
							add eax, Mix4P;	//eax now holds the VMixP.
						//	VMix = 0;
						//	surf = (UCHAR*)rbd->data + lastcol + c + t * lPitch;
						//Compute surface pointer, only EDI/ESI used.
						//	mov edi, lPitch;
						//	//mov esi, t;
						//	imul esi, edi;	//ESI is t from earlier.
						//	add esi, rbddata;
						//	add esi, lastcol;
						//	add esi, c;
							mov edi, datayaddrP;
							mov ebx, [edi+esi*4];	//Replacing imul with table lookup.
							//add esi, rbddata;
							mov esi, c;
							mov edi, lPitch;
							add esi, ebx;
							//mov eax, t;
							//imul eax, edi;	//edi holds pitch.
							//add esi, eax;	//esi holds surface pointer.
						//	for(line = linet; line; line--){
							//xor ebx, ebx;
							vloop:
						//		*surf = *(VMixP + (VMix >>6));
								mov bl, byte ptr [eax];
								add dh, dl;
								mov byte ptr [esi], bl;
	//							mov byte ptr [esi], al;
								adc eax, 0;
	//							jnc vloopnc;
	//							shr eAx, 8;
	//							vloopnc:
								add esi, edi;
								dec ecx;
								jnz vloop;
						//	*ybufp = lsy >> FP;
							mov ecx, lsy;
							mov ebx, ybufp;
							sar ecx, FP;
							mov dword ptr [ebx], ecx;
						//}
						ifend:
					//	ybufp++;
					//	*cbufp++ = lmapc;
						mov ecx, cbufp;	//2
						mov ebx, ybufp;	//1
						mov al, lmapc;	//2
						add ebx, 4;		//1
						mov [ecx], al;	//2
						inc ecx;		//2
						mov ybufp, ebx;	//1
						mov cbufp, ecx;	//2
					//}
						mov ecx, c;
					//	mov eax, cend;
						mov eax, col;
						inc ecx;
						cmp ecx, eax;
						mov c, ecx;
						js cloop;
				}
#else
					HMixD = Mlookup[cend];
					HMix = 0;
	//				HMixV = *((unsigned long int*)Mix4 + (lmapc <<8) + mapc);
				//	HMixP = &Mix4[lmapc][mapc][0];
					HMixP = (Mix4 + (lmapc <<10) + (mapc <<2));
				//	for(c = 0; c < cend; c++){
					for(c = lastcol; c < col; c++){
					//	mapcy = Mix4[lmapc][mapc][HMix >>6];
						mapcy = *(HMixP + (HMix >>6)
							//Testing temporary dither.
						//	& (c))// & (HMix >>5))
							);
						HMix += HMixD;
	//					if(HMix >= 64){
	//						HMix -= 64;
	//						HMixV >>= 8;
	//					}
						//MARKMIX(lmapc, mapc);
						if((t = (lsy += syd) >> FP) < *ybufp){
						//	surf = (UCHAR*)rbd->data /*+ lastcol*/ + c + t * lPitch;
							surf = datayaddr[t] + c;// + t * lPitch;
							linet = *ybufp - t;
							VMixD = Mlookup[linet];
							VMix = 0;
	//						VMixV = *((unsigned long int*)Mix4 + ((HMixV & 0xff) <<8) + *cbufp);
						//	VMixP = &Mix4[mapcy][*cbufp][0];
							VMixP = (Mix4 + (mapcy <<10) + (*cbufp <<2));
						//	for(line = 0; line < linet; line++){
							for(line = linet; line; line--){
							//	*surf = Mix4[mapcy][*cbufp][VMix >>6];
								*surf = *(VMixP + (VMix >>6)
									//Testing temporary dither.
								//	& (line))// & (VMix >>5))
									);
	//							*surf = (unsigned char)VMixV;
								VMix += VMixD;
								//MARKMIX(mapcy, *cbufp);
								surf += lPitch;
	//							if(VMix >= 64){
	//								VMix -= 64;
	//								VMixV >>= 8;
	//							}
							}
							*ybufp = lsy >> FP;
						}
						ybufp++;
					//	*cbufp++ = lmapc;
						*cbufp++ = mapcy;
					}
#endif
				//		ybufp++;
				//		*cbufp++ = lmapc;
				//	}
				}else{	//16bit rendering code below.
				}
			}
		//	cx += dx;  cy += dy;
		}//Next k

		//Fill YBufs of Poly objects.
		while(CurPolyObj < poly->nObject && (float)z / 256.0f > poly->ObjectP[CurPolyObj]->SortZ){
			int ybl = std::max(0, poly->ObjectP[CurPolyObj]->YBufLeft);
			int ybr = std::min(rbd->width, poly->ObjectP[CurPolyObj]->YBufRight);// + 1);
			if(ybl < ybr && poly->ObjectP[CurPolyObj]->YBufPtr){
				memcpy(poly->ObjectP[CurPolyObj]->YBufPtr, &ybuf[0] + ybl, (ybr - ybl) * sizeof(int));
			//	memcpy(&poly->ObjectP[CurPolyObj]->YBuf[0] + ybl, &ybuf[0] + ybl, (ybr - ybl) * sizeof(int));
			}
			CurPolyObj++;
		}
	}

//	sprintf(DbgBuf, "Rend MS %d\n", timer.Check(1000));
//	OutputDebugString(DbgBuf);

	DbgBufLen = sprintf(DbgBuf, "Scanned: %d  Visible: %d", totalvoxels, visvoxels);

	//Render sky.
	if(Sky && Sky->Data()){
		//Sky texture must now be rotated 90 degrees to the left!!
		int SkyW = Sky->Height(), SkyH = Sky->Width();
		int scale = (SkyW <<FP2) / width;
		int sx = 0, sy = 0, sxd = scale, syd = scale;
		int swidth = SkyW <<FP2, sheight = SkyH <<FP2;
		int rotoffset = (int)(sin(1 * DEG2RAD) * cam->b * cam->viewplane);
		ybufp = ybuf;
		cbufp = cbuf;
		for(i = 0; i < width; i++){
			sx = (i + rotoffset) * scale;//) % (Sky->width <<FP2);
			if(sx < 0) sx = abs(sx);
			if((sx / swidth) & 1){
				sx = (swidth - 1) - sx % swidth;
			}else{
				sx = sx % swidth;
			}
			sy = (int)(height2 - (shearstart + (rolldelta / (double)width) * (double)i) / FPVAL) * scale;
			//	* scale) % (Sky->height - 1 <<FP2);
		//	sy = abs((int)((Sky->height - height2) + shearstart / FPVAL)) % (Sky->height - 1);
			syd = scale;
			if(sy < 0){
				sy = abs(sy);
				syd = -syd;
			}
			if((sy / sheight) & 1){
				sy = (sheight - 1) - sy % sheight;
				syd = -syd;
			}else{
				sy = sy % sheight;
			}
			t = *ybufp;
			int iter;
		//	int, skypow = Sky->HiBit(Sky->Width()),
		//	int skyh = Sky->Width();
			unsigned char *skydata = Sky->Data() + (Sky->Height() - 1 - (sx >>FP2)) * Sky->Pitch();
			if(t > 0){
				if(rend16bit == 0){
					surf = (UCHAR*)rbd->data + i;
					line = 0;
					while(line < t - 1){
				//	for(line = 0; line < t; line++){
						iter = (t - 1) - line;
						if(syd > 0){
							iter = std::min(iter, ((sheight - 1) - sy) / syd + 1);
						}else{
							iter = std::min(iter, (sy - 1) / abs(syd) + 1);
						}
						line += iter;
						for( ; iter--; ){
						//	if(line == t - 1 && (ybuf[std::max(0, i - 1)] < t || ybuf[std::min(width - 1, i + 1)] < t) ){	//Anti-aliasing for the horizon.  :)
							//	*surf = Mix33[*(Sky->data + (sx >>FP2) + (Sky->height - 1 - (sy >>FP2)) * Sky->width)][*cbufp >>FP];
						//		*surf = Mix4[*(skydata + (sx >>FP2) + (skyh - 1 - (sy >>FP2)) <<skypow)][*cbufp >>FP][2];
						//	}else{
							//	*surf = 255;
					//		*surf = *(skydata + (sx >>FP2) + ((skyh - 1 - (sy >>FP2)) <<skypow));
							*surf = *(skydata + (SkyH - 1 - (sy >>FP2)));
						//	}
							surf += lPitch;  sy += syd;
						}
						if(sy >= sheight - 1){  sy = sheight - 1;  syd = -abs(syd);  }
						if(sy <= 0){  sy = 0;  syd = abs(syd);  }
				//	}
					}
					if(ybuf[std::max(0, i - 1)] < t || ybuf[std::min(width - 1, i + 1)] < t){	//Anti-aliasing for the horizon.  :)
				//		*surf = Mix4[*(skydata + (sx >>FP2) + ((skyh - 1 - (sy >>FP2)) <<skypow))][*cbufp >>FP][2];
						*surf = mix->Mix4[*(skydata + (SkyH - 1 - (sy >>FP2)))][*cbufp >>FP][2];
					}
				}else{	//16bit rendering code below.
		//			surf16 = (USHORT*)rbd->data + i;
		//			for(line = 0; line < t; line++){
		//			//	if(line == t - 1 && (ybuf[std::max(0, i - 1)] < t || ybuf[std::min(width - 1, i + 1)] < t) ){	//Anti-aliasing for the horizon.  :)
		//			//		*surf16 = Color8to16[Mix33[*(Sky->data + (sx >>FP2) + (Sky->height - 1 - (sy >>FP2)) * Sky->width)][*cbufp >>FP]];
		//			//	}else{
		//					*surf16 = Color8to16[*(Sky->Data() + (sx >>FP2) + (Sky->Height() - 1 - (sy >>FP2)) * Sky->Pitch())];
		//			//	}
		//				surf16 += (lPitch >>1);  sy += syd;
		//				if(sy >= sheight - 1){  sy = sheight - 1;  syd = -abs(syd);  }
		//				if(sy <= 0){  sy = 0;  syd = abs(syd);  }
		//			}
				}
			}
//			if(t < 0) plot(i, std::min(abs(t), height - 1), 254);
			ybufp++;
			cbufp++;
		//	sx += sxd;
		//	if(sx >= Sky->width - 1) sxd = -1;
		//	if(sx <= 0) sxd = 1;
		}
	}

#ifdef MARKMIXTABLE
	for(int y = 0; y < 240; y++){
		for(int x = 0; x < 256; x++){
			if(MarkMix[x][y]){
				plot(x, y, 255 - (MarkMix[x][y]-- <<3));
		//	}else{
		//		plot(x, y, 0);
			}
		}
	}
#endif
	for(t = 0; t < 256; t++){
		plot(t, 0, t);
		plot(t, 1, t);
	}
//#endif

	if(tflag) plot(1, 1, 20);

//	plot(width2, height2, 200);
//	plot(width2 + ((maindx * 50) >> FP), height2 + ((maindy * 50) >> FP), 200);
//	plot(width2 + ((maindx * 50) >> FP) + ((odx * ((acrossd * 50) >> FP)) >> FP),
//		height2 + ((maindy * 50) >> FP) + ((ody * ((acrossd * 50) >> FP)) >> FP), 200);

	return true;
}

#endif
