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

#include "VoxelAfx.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "Terrain.h"
#include "IFF.h"
#include "Vector.h"

using namespace std;

inline int Clip(int n, int low, int high){ return (n < low ? low : (n > high ? high : n)); }
inline float Clip(float n, float low, float high){ return (n < low ? low : (n > high ? high : n)); }

EcoSystem::EcoSystem(){
	minA = minSlope1 = maxSlope1 = 0;
	maxA = minSlope2 = maxSlope2 = 0;
	minAlt = minAngle1 = maxAngle1 = 0;
	maxAlt = minAngle2 = maxAngle2 = 0;
	tex = NULL;
	name[0] = '\0';
}
EcoSystem::~EcoSystem(){
}

void EcoSystem::Init(double mina, double mins1, double maxs1, double maxa, double mins2, double maxs2, Image * t, char *n){
/*	MinAlt = (mina * 256.0) / 100.0;
//	MinSlope1 =
	MinSlope1 = cos(mins1 * DEG2RAD);
	MaxSlope1 = cos(maxs1 * DEG2RAD);
	MaxAlt = (maxa * 256.0) / 100.0;
	MinSlope2 = cos(mins2 * DEG2RAD);
	MaxSlope2 = cos(maxs2 * DEG2RAD);*/
	MinAlt(mina); MaxAlt(maxa);
	MinAngle1(mins1); MaxAngle1(maxs1);
	MinAngle2(mins2); MaxAngle2(maxs2);
	if(t) tex = t;
	if(n){
		strcpy(name, n);
	}else{
		if(name[0] == '\0') strcpy(name, "none");
	}
}
void EcoSystem::MinAlt(double a){
	minAlt = a;
	minA = 256.0 * a;
}
void EcoSystem::MaxAlt(double a){
	maxAlt = a;
	maxA = 256.0 * a;
}
void EcoSystem::MinAngle1(double a){
	minAngle1 = a;
	minSlope1 = cos(a * DEG2RAD);
}
void EcoSystem::MinAngle2(double a){
	minAngle2 = a;
	minSlope2 = cos(a * DEG2RAD);
}
void EcoSystem::MaxAngle1(double a){
	maxAngle1 = a;
	maxSlope1 = cos(a * DEG2RAD);
}
void EcoSystem::MaxAngle2(double a){
	maxAngle2 = a;
	maxSlope2 = cos(a * DEG2RAD);
}
EcoSystem &EcoSystem::operator=(EcoSystem &eco){	//Does not copy name!  Well, it does now...
	strcpy(name, eco.name);
	minA = eco.minA; maxA = eco.maxA;
	minSlope1 = eco.minSlope1; maxSlope1 = eco.maxSlope1;
	minSlope2 = eco.minSlope2; maxSlope2 = eco.maxSlope2;
	minAlt = eco.minAlt; maxAlt = eco.maxAlt;
	minAngle1 = eco.minAngle1; maxAngle1 = eco.maxAngle1;
	minAngle2 = eco.minAngle2; maxAngle2 = eco.maxAngle2;
	return *this;
}

Terrain::Terrain(){
	data = NULL;
	texid = NULL;
	width = 0;
	height = 0;
	data32 = NULL;
//	ShadeLookup32 = NULL;
//	for(int i = 0; i < NUM_SQRT; i++){
//		SqrtLookup[i] = (int)(sqrt((double)(i <<FP)) * FPVAL);
//	}
	memset(TexIDs, 0, sizeof(TexIDs));
	//
	PalTextures = 0;
	ScorchTex = -1;
}
Terrain::~Terrain(){
	Free();
}
bool Terrain::Free(){
	if(data) free(data);
	if(texid) free(texid);
	data = NULL;
	texid = NULL;
	width = 0;
	height = 0;
	if(data32) free(data32);
//	if(ShadeLookup32) free(ShadeLookup32);
	data32 = NULL;
//	ShadeLookup32 = NULL;
	return true;
}
void Terrain::SetScorchEco(int i){
	ScorchTex = i;
}
int Terrain::GetScorchEco(){
	return ScorchTex;
}
bool Terrain::Load(const char *name, int *numecoptr, EcoSystem *eco, int maxeco, bool mirror){
	IFF iff;
	if(name){
		iff.OpenIn(name);	//Stack object will close open file on return.
		return Load(&iff, numecoptr, eco, maxeco, mirror);
	}
	return false;
}
bool Terrain::Load(IFF *iff, int *numecoptr, EcoSystem *eco, int maxeco, bool mirror){
	if(iff == NULL) return false;
	//
	//Should load light vector from map...  Or maybe ents.
	SetLightVector(-1, 1, 0);
	//
//	IFF iff;	//Destructor for stack object will close file.
	int x, y, i;
	unsigned char buf[16384];
//	if(iff->OpenIn(name)){
	if(iff->IsFileOpen() && iff->IsFileRead()){
		if(iff->IsType("VOXL")){
			if(iff->FindChunk("HEAD") >= 8){
				x = iff->ReadLong();
				y = iff->ReadLong();
				Init(x, y);
			}
			if(iff->FindChunk("HMAP") >= 4 && data && width > 0 && height > 0){
				if(iff->ReadLong() == TERRAIN_ENCODE_RAW){
					if(mirror){
						for(y = height - 1; y >= 0; y--){
							iff->ReadBytes(buf, width);
							for(x = 0; x < width; x++) SetHraw(x, y, buf[x]);
						}
					}else{
						for(y = 0; y < height; y++){
							iff->ReadBytes(buf, width);
							for(x = 0; x < width; x++) SetHraw(x, y, buf[x]);
						}
					}
				}
			}
			if(iff->FindChunk("TXID") >= 4 && data && width > 0 && height > 0){
				InitTextureID();
				int Encoding = iff->ReadLong();
				if(texid){
					if(Encoding == TEXID_ENCODE_RAW){	//Old raw reader.
						if(mirror){
							for(y = height - 1; y >= 0; y--){
								iff->ReadBytes(buf, width);
								for(x = 0; x < width; x++) SetTraw(x, y, buf[x]);
							}
						}else{
							for(y = 0; y < height; y++){
								iff->ReadBytes(buf, width);
								for(x = 0; x < width; x++) SetTraw(x, y, buf[x]);
							}
						}
					}
					if(Encoding == TEXID_ENCODE_RLE){	//New RLE reader.
						x = y = 0;
						unsigned char byte = 0, run = 0;
						if(mirror){
							y = height - 1;
							do{
								run = iff->ReadByte();
								byte = iff->ReadByte();
								for(i = 0; i < run; i++){
									SetTraw(x, y, byte);
									if(++x >= width){  x = 0;  y--;  }
								}
							}while((run != 0 || byte != 0) && y >= 0);
						}else{
							do{
								run = iff->ReadByte();
								byte = iff->ReadByte();
								for(i = 0; i < run; i++){
									SetTraw(x, y, byte);
									if(++x >= width){  x = 0;  y++;  }
								}
							}while((run != 0 || byte != 0) && y < height);
						}
					}
				}
			}
			if(iff->FindChunk("ECOS") >= 8 && numecoptr && eco && maxeco > 0){
				int numeco = iff->ReadLong();
				numeco = std::min(numeco, maxeco);
				*numecoptr = numeco;
				int floats = iff->ReadLong();	//floats per eco.
				uchar slen;
				if(floats >= 6){
					for(i = 0; i < numeco; i++){
						eco[i].MinAlt((float)iff->ReadFloat());
						eco[i].MinAngle1((float)iff->ReadFloat());
						eco[i].MaxAngle1((float)iff->ReadFloat());
						eco[i].MaxAlt((float)iff->ReadFloat());
						eco[i].MinAngle2((float)iff->ReadFloat());
						eco[i].MaxAngle2((float)iff->ReadFloat());
						if(floats > 6) for(x = 6; x < floats; x++) iff->ReadFloat();
						slen = iff->ReadByte();	//Length includes ending null!
						iff->ReadBytes((uchar*)eco[i].name, slen);
					}
				}
			}
		//	iff.Close();
			return true;
		}
	}
//	iff.Close();
	return false;
}
bool Terrain::Save(const char *name, EcoSystem *eco, int numeco){
	IFF iff;
	if(name){
		iff.OpenOut(name);	//Stack object will close open file on return.
		return Save(&iff, eco, numeco);
	}
	return false;
}
bool Terrain::Save(IFF *iff, EcoSystem *eco, int numeco){
	if(iff == NULL || data == NULL || width <= 0 || height <= 0) return false;
//	IFF iff;
	int x, y, i;
	unsigned char buf[16384];
//	if(iff->OpenOut(name, "VOXL")){
	if(iff->IsFileOpen() && iff->IsFileWrite() && iff->SetType("VOXL")){
		//Write header.
		iff->StartChunk("HEAD");
		iff->WriteLong(width);
		iff->WriteLong(height);
		iff->EndChunk();
		//Write height map data.
		iff->StartChunk("HMAP");
		iff->WriteLong(TERRAIN_ENCODE_RAW);
		for(y = 0; y < height; y++){
			for(x = 0; x < width; x++){
				buf[x] = GetHraw(x, y);
			}
			iff->WriteBytes(buf, width);
		}
		iff->EndChunk();
		//If written, color map would be CMAP chunk.
		//Write Texture ID map.
		if(texid){
			iff->StartChunk("TXID");
			iff->WriteLong(TEXID_ENCODE_RLE);
			//RLE will be simple, dumb, run length byte, run data byte, repeat.
			//A run of length 0 is an escape signal, a data of 0 after means the end.
			unsigned char byte = 0, lbyte = 0;
			int run = 0;
			for(y = 0; y < height; y++){
				for(x = 0; x < width; x++){
					lbyte = byte;
					byte = GetTraw(x, y);
					if((x > 0 && lbyte != byte) || run >= 255){
						iff->WriteByte(run);
						iff->WriteByte(lbyte);
						run = 0;
					}
					run++;
				}
				iff->WriteByte(run);	//Flush end of line run.
				iff->WriteByte(byte);
				run = 0;
			}
			iff->WriteByte(0);	//Escape.
			iff->WriteByte(0);	//End of data.
		//	iff->WriteLong(TEXID_ENCODE_RAW);
		//	for(y = 0; y < height; y++){
		//		for(x = 0; x < width; x++){
		//			buf[x] = GetTraw(x, y);
		//		}
		//		iff->WriteBytes(buf, width);
		//	}
			iff->EndChunk();
		}
		//And ecosystem info.
		if(eco && numeco > 0){
			iff->StartChunk("ECOS");
			iff->WriteLong(numeco);
			iff->WriteLong(6);	//floats per eco.
			for(i = 0; i < numeco; i++){
				iff->WriteFloat((float)eco[i].MinAlt());
				iff->WriteFloat((float)eco[i].MinAngle1());
				iff->WriteFloat((float)eco[i].MaxAngle1());
				iff->WriteFloat((float)eco[i].MaxAlt());
				iff->WriteFloat((float)eco[i].MinAngle2());
				iff->WriteFloat((float)eco[i].MaxAngle2());
				iff->WriteByte(strlen(eco[i].name) + 1);	//Length includes ending null!
				iff->WriteBytes((uchar*)eco[i].name, strlen(eco[i].name) + 1);
			}
			iff->EndChunk();
		}
	//	iff->Close();
		return true;
	}
	return false;
}
int Terrain::FastSqrt(int x, int y, int z){
	return 1;//SqrtLookup[std::max(std::min((abs(x * x) + abs(y * y) + abs(z * z)) >>FP, 0), NUM_SQRT - 1)];
}
bool Terrain::Init(int x, int y){
	Free();
	if(x <= 0 || y <= 0) return false;
	x = 1 << (widthpow = HiBit(x));	//Make sure width and height are powers of two now!
	y = 1 << (heightpow = HiBit(y));
	if(NULL == (data = (unsigned short*)malloc(x * (y + 2) * 2))){
		Free();
		return false;
	}
	width = x;
	height = y;
	widthmask = width - 1;	//And coords with these masks to wrap around map.
	heightmask = height - 1;
	//
//	if(!data32){
	if(NULL == (data32 = (unsigned long*)malloc(width * height * sizeof(unsigned long)))){
		Free();
		return false;
	}
	memset(data32, 0, width * height * 4);
//	}
	return Clear();
}
bool Terrain::Clear(){
//	if(hdata) memset(hdata, 0, width * height);
	if(data) memset(data, 0, width * height * 2);
	if(data == NULL) return false;
	return true;
}
bool Terrain::InitTextureID(){
	if(width > 0 && height > 0 && texid == NULL && (texid = (unsigned char*)malloc(width * height))){
		memset(texid, 0, width * height);
		return true;
	}
	return false;
}
void Terrain::RotateEdges(){
	int sline, dline;
	if(data && width > 0 && height > 0){
		unsigned short *tdata = (unsigned short*)malloc(width * height * 2);
		if(tdata){
			memcpy(tdata, data, width * height * 2);
			for(sline = 0; sline < height; sline++){
				dline = sline + (height >>1);
				if(dline >= height) dline -= height;
				memcpy((unsigned char*)data + dline * width * 2, (unsigned char*)tdata + sline * width * 2 + width, width);
				memcpy((unsigned char*)data + dline * width * 2 + width, (unsigned char*)tdata + sline * width * 2, width);
			}
			free(tdata);
		}
		if(texid){
			unsigned char *ttexid = (unsigned char*)malloc(width * height);
			if(ttexid){
				memcpy(ttexid, texid, width * height);
				for(sline = 0; sline < height; sline++){
					dline = sline + (height >>1);
					if(dline >= height) dline -= height;
					memcpy((unsigned char*)texid + dline * width, (unsigned char*)ttexid + sline * width + width / 2, width / 2);
					memcpy((unsigned char*)texid + dline * width + width / 2, (unsigned char*)ttexid + sline * width, width / 2);
				}
				free(ttexid);
			}
		}
	}
}
int Terrain::Blow(unsigned char *dest, int dw, int dh, int dp,
		 int sx, int sy, int sx2, int sy2, int flag, int destbpp, PaletteEntry pe[256]){
	//Check for sanity.
	if(data == NULL || dest == NULL || dw <= 0 || dh <= 0 ||
		sx >= width || sy >= height || sx2 < 0 || sy2 < 0 || destbpp < 8 || destbpp > 32) return 0;
	flag = (flag == 1);
	if(sx < 0) sx = 0;
	if(sy < 0) sy = 0;
	if(sx2 == 0) sx2 = width;
	if(sy2 == 0) sy2 = height;
	sx2 = std::min(sx2, width);
	sy2 = std::min(sy2, height);
	if(destbpp == 32){
		//New code.
		if(flag && pe){
			for(int y = sy; y < sy2; y++){
				unsigned char *dst = dest + sx * 4 + y * dp;
				unsigned char *src = (unsigned char*)data + 1 + sx * 2 + y * width * 2;
				for(int x = sx; x < sx2; x++){
					dst[0] = pe[*src].peBlue;
					dst[1] = pe[*src].peGreen;
					dst[2] = pe[*src].peRed;
					dst[3] = 0;
					dst += 4;
					src += 2;
				}
			}
		}else{
			if(data32){
				for(int y = sy; y < sy2; y++){
					unsigned char *dst = dest + sx * 4 + y * dp;
					unsigned char *src = (unsigned char*)data32 + sx * 4 + y * width * 4;
					for(int x = sx; x < sx2; x++){
						dst[0] = src[2];
						dst[1] = src[1];
						dst[2] = src[0];
						dst[3] = 0;
						dst += 4;
						src += 4;
					}
				}
			}
		}
	}else{
		//Fall back to old 8-bit code.
		unsigned char *tdest = dest + sx + sy * dp;
		int tdestp = dp - (sx2 - sx);	//Bytes to add to dest each line.
		unsigned char *tsrc = (unsigned char*)data + flag + sx * 2 + sy * width * 2;
		int tsrcp = width * 2 - (sx2 - sx) * 2;
		int x, y;
		for(y = sy; y < sy2; y++){
			for(x = sx; x < sx2; x++){
				*tdest++ = *tsrc;
				tsrc += 2;
			}
			tdest += tdestp;
			tsrc += tsrcp;
		}
	}
	return 1;
}

//***************************************************
//
//	This function is where the real action takes place,
//	we fractally terraform a landscape.  Level is how
//	far apart the initial purely random height data
//	should be created, as a power of 2.  Min and max
//	constrain the terrain to a certian altitude range.
//
//***************************************************
#define STAT(a,b) if(Stat) (*Stat)(this, (a), (b))
bool Terrain::FractalForm(int level, int min, int max, int Form1, int Form2, EcoSystem *eco, int numeco, void (*Stat)(Terrain*const, const char*, int) ){

	if(data == NULL) return false;
	//|| eco == NULL || numeco <= 0) return false;
//	for(int t = 0; t < numeco; t++){
//		if(eco[t].tex == NULL || eco[t].tex->Data() == NULL) return false;
//	}
	//Now must be called by main program.
	//MakeShadeLookup(eco[0].tex->pe);

	int Normalize = 0;
	int Average = 1;
	int Perturb = 2;

	min = std::min(std::max(min, 1), 254);
	max = std::min(std::max(max, min + 1), 255);
	level = std::max(level, 0);

//	AvgRng ar;
	int first = 1, x = 0, y = 0, step = 0, i, t;//, tx, ty
	int linetotal = 0, linecurrent = 0;//, temp1, temp2;
	char buf[1000];
	int randrange;

	//New paradigm, allocate and create low resolution mini-map of heights first, using
	//one algorithm for general terrain form, then interpolate between those low res
	//heights using another algorithm of choice.
	//First set dimensions and allocate.
	step = 1 <<level;
	int MiniW = width / step + 1;
	int MiniH = height / step + 1;
	unsigned char *MiniD = (unsigned char*)malloc(MiniW * MiniH);
	if(!MiniD) return false;
	//Then fill it in.
	randrange = (step <<VOXSQUEEZE);// / 2;
	int lasty, thisy = min + rand() % (max - min);
	//Top row.
	for(x = 0; x < MiniW; x++){
		lasty = thisy;
		thisy = Clip(lasty - (randrange >>1) + rand() % randrange, min, max);
		*(MiniD + x) = thisy;
	}
	//Left column.
	for(y = 1; y < MiniH; y++){
		*(MiniD + y * MiniW) = Clip(*(MiniD + (y - 1) * MiniW) - (randrange >>1) + rand() % randrange, min, max);
	}
	//The rest.
	for(y = 1; y < MiniH; y++){
		for(x = 1; x < MiniW; x++){
			*(MiniD + x + y * MiniW) = Clip(
				(*(MiniD + x + (y - 1) * MiniW) +
				*(MiniD + x - 1 + (y - 1) * MiniW) +
				*(MiniD + x - 1 + y * MiniW)) / 3 -
				(randrange >>1) + rand() % randrange, min, max);
		}
	}

if(Form2 == TFORM_FRACTAL){
	//Fractal terraforming.
	while(level >= 0){
		step = 1 << level;
		randrange = std::max((step <<VOXSQUEEZE) / 2, 1);
		if(first){	//Seed terrain with mini-map created values.
			for(y = 0; y <= height; y += step){
				for(x = 0; x <= width; x += step){
					SetH(std::min(x, width - 1), std::min(y, height - 1), *(MiniD + x/step + y/step * MiniW));
				}
			}
		}else{
			int xflip, yflip = 0, random;	//flip-flop variables.
			for(y = 0; y < height; y += step){
				xflip = 0;	//1 for first column, then 0, then 1, etc.
				if(yflip = 1 - yflip){	//Is 1 for first line, 0 for next, etc.
					for(x = 0; x < width; x += step){	//Even lines (starting at 0).
						if(!(xflip = 1 - xflip)){
							random = rand() % randrange - (randrange >>1);
						//	SetH(x, y, random + (GetH(x - step, y) + GetH(std::min(x + step, width - 1), y)) >>1);
							SetH(x, y, random + ((GetHwrap(x - step, y) + GetHwrap(x + step, y)) >>1));
						}
					}
				}else{
					for(x = 0; x < width; x += step){	//Odd lines (starting at 0).
						random = rand() % randrange - (randrange >>1);
						if(xflip = 1 - xflip){
						//	SetH(x, y, random + (GetH(x, y - step) + GetH(x, std::min(y + step, height - 1))) >>1);
							SetH(x, y, random + ((GetHwrap(x, y - step) + GetHwrap(x, y + step)) >>1));
						}else{
							SetH(x, y, random + ((GetHwrap(x - step, y - step) + GetHwrap(x - step, y + step) +
								GetHwrap(x + step, y - step) + GetHwrap(x + step, y + step)) >>2));
						}
					}
				}
				sprintf(buf, "TForm Level %d", level);
				STAT(buf, (y * 100) / (height - (height - 1) % step));
			}
		}
		sprintf(buf, "TForm Level %d", level);
		STAT(buf, 100);
		level--;
		first = 0;
	}
}
if(Form2 == TFORM_SPLINE){
	//Zero-slope start and end, S-curve middle, spline based terrain interpolation.
	int hy1, hy1V, hy1VD;
	int hy2, hy2V, hy2VD;
	int vy, vyV, vyVD;
	int x2, y2, w2 = step / 2;
	//Outer loops go through boxes of step x step dimensions.
	for(y2 = 0; y2 < height / step; y2++){
		for(x2 = 0; x2 < width / step; x2++){
			//Uppper spline for this box.
			hy1V = 0;
			hy1 = ((int)*(MiniD + x2 + y2 * MiniW)) <<FP2;
			hy1VD = ((*(MiniD + x2 + 1 + y2 * MiniW) <<FP2) - hy1) / (w2 * w2);
			//Lower spline for this box.
			hy2V = 0;
			hy2 = ((int)*(MiniD + x2 + (y2 + 1) * MiniW)) <<FP2;
			hy2VD = ((((int)*(MiniD + x2 + 1 + (y2 + 1) * MiniW)) <<FP2) - hy2) / (w2 * w2);
			for(x = x2 * step; x < (x2 + 1) * step; x++){
				//At mid point reverse spline delta deltas.
				if((x + step / 2) % step == 0){
					hy1VD = -hy1VD;
					hy2VD = -hy2VD;
				}
				hy1 += hy1V;
				hy1V += hy1VD;
				hy2 += hy2V;
				hy2V += hy2VD;
				//Vertical spline between upper and lower splines.
				vyV = 0;
				vy = hy1;
				vyVD = (hy2 - hy1) / (w2 * w2);
				for(y = y2 * step; y < (y2 + 1) * step; y++){
					if((y + step / 2) % step == 0){
						vyVD = -vyVD;
					}
					vy += vyV;
					vyV += vyVD;
					SetH(x, y, vy >>FP2);
				}
			}
		}
		sprintf(buf, "Splining.");
		STAT(buf, y);//x + y * (width / step));
	}
	}
	//Normalize the terrain to fit fully within the min, max range.
	//First find actual min, max heights of terrain.
	if(Average){
		if(Normalize){	//Normalizing is SLOOW.
			int realmin = 255, realmax = 0;
			for(y = 0; y < height; y++){
				for(x = 0; x < width; x++){
					t = GetH(x, y);
				//	t = GetBigH(x, y);
					if(t < realmin) realmin = t;
					if(t > realmax) realmax = t;
				}
			}
			int yrange1 = max - min, yrange2 = realmax - realmin, vtot, vnum, ix, iy;
			//Now shift and scale all voxels appropriately.
			//New:  Grab values, normalize, THEN average.  Fixes jaggies.
			for(y = 0; y < height; y++){
				for(x = 0; x < width; x++){
					//Manually average so we can normalize each sample BEFORE averaging.
					vtot = vnum = 0;
					for(iy = y; iy < y + 3; iy++){	//Current voxel is top-left of 9 samples so output is never read back in.
						for(ix = x; ix < x + 3; ix++){
							if((t = GetH(ix, iy)) > 0){
								vtot += ((t - realmin) * yrange1) / yrange2;
								vnum++;
							}
						}
					}
					SetH(x, y, min + vtot / std::max(vnum, 1));
				}
				STAT("Normalizing", (y * 100) / (height - 1));
			}
		}else{	//Just average.  FAAST.
			for(y = 1; y < height - 1; y++){
				for(x = 1; x < width - 1; x++){
					//GetHs will always fall within boundaries, so use raw to avoid sanity checks.
					SetH(x, y, (GetHraw(x - 1, y - 1) + GetHraw(x, y - 1) + GetHraw(x + 1, y - 1) +
								GetHraw(x - 1, y) + /*GetH(x, y) +*/ GetHraw(x + 1, y) +
								GetHraw(x - 1, y + 1) + GetHraw(x, y + 1) + GetHraw(x + 1, y + 1)) >>3);// / 9);
				}
				STAT("Smoothing", (y * 100) / (height - 1));
			}
		}
	}

	if(Perturb > 0){
		int nump = ((width * height) / 10) * Perturb;
		for(i = 0; i < nump; i++){
			x = rand() % width;
			y = rand() % height;
			SetH(x, y, GetH(x, y) + 1);
		//	SetH(x, y - 1, GetH(x, y - 1) + 1);
		//	SetH(x + 1, y, GetH(x + 1, y) + 1);
		//	SetH(x, y + 1, GetH(x, y + 1) + 1);
		//	SetH(x - 1, y, GetH(x - 1, y) + 1);
		}
	}

	//************************************** OK, below should be all texturing...
	//Right now all texturing is skipped if no ecosystems passed in...

	Texture(eco, numeco);
	Lightsource();

	return true;
}
//Ambient must be set BEFORE MakeShadeLookup, and light vector BEFORE TextureLightEtc.
void Terrain::SetLightVector(float lx, float ly, float lz, float amb){
	float l = sqrtf(lx * lx + ly * ly + lz * lz);
	if(l > 0.0f){
		LightX = lx / l;
		LightY = ly / l;
		LightZ = lz / l;
	}
	Ambient = amb;
}
bool Terrain::RemapTexID(unsigned char *remap){
	if(width && height && texid && remap){
		for(int y = 0; y < height; y++){
			unsigned char *tt = texid + y * width;
			for(int x = width; x; x--){
				*tt = remap[*tt];
				tt++;
			}
		}
		return true;
	}
	return false;
}
int Terrain::EcoTexture(EcoSystem *eco, int numeco, int x, int y){
	if(x < width && y < height && x >= 0 && y >= 0 && data && eco){
		int T = 0, H = GetHraw(x, y);
		double S = GetSlope(x, y, 0, 1.0, 0);	//Like a light intensity calc for noon.
		for(T = 0; T < numeco; T++){
			if(H >= eco[T].minA && H <= eco[T].maxA){
				if(S <= eco[T].minSlope1 + (eco[T].minSlope2 - eco[T].minSlope1) /
					(eco[T].maxA - eco[T].minA) * (H - eco[T].minA)
					&& S >= eco[T].maxSlope1 + (eco[T].maxSlope2 - eco[T].maxSlope1) /
					(eco[T].maxA - eco[T].minA) * (H - eco[T].minA) ){
					break;
				}
			}
		}
		T = std::min(std::max(T, 0), numeco - 1);
		if(texid) SetTraw(x, y, T);
		return T;
	}
	return -1;
}
bool Terrain::Texture(EcoSystem *eco, int numeco, bool UseIDMap, int x1, int y1, int x2, int y2){
	if(data == NULL || eco == NULL || numeco <= 0 ||
		x2 < 0 || y2 < 0 || x1 > width || y1 > height ||
		(UseIDMap && texid == NULL)) return false;	//Sanity.
	for(int t = 0; t < numeco; t++){
		if(eco[t].tex == NULL) return false;// || eco[t].tex->Data() == NULL) return false;
	}
	if(x2 == 0) x2 = width; if(y2 == 0) y2 = height;	//Default to whole shebang.
	x1 = std::max(0, x1); y1 = std::max(0, y1);	//Clip.
	x2 = std::min(width, x2); y2 = std::min(height, y2);	//Combined with sanity, all should be OK.
	int x, y, T = 0, H;
	double S;
	int xmask[50], ymask[50];	//NOTE, textures must be power of two width and height now!
	char EcoOk[50];
	for(int e = 0; e < numeco; e++){
		xmask[e] = (1 << HiBit(eco[e].tex->Width())) - 1;
		ymask[e] = (1 << HiBit(eco[e].tex->Height())) - 1;
		EcoOk[e] = (eco[e].tex->Data() != NULL);
	}
	for(y = y1; y < y2; y++){
		for(x = x1; x < x2; x++){
			if(UseIDMap){	//Pick texture based on ID map.
				T = GetTraw(x, y);
				T = std::min(std::max(T, 0), numeco - 1);
			}else{	//Pick texture based on ecosystem parameters.
				H = GetHraw(x, y);
				S = GetSlope(x, y, 0, 1.0, 0);	//Like a light intensity calc for noon.
				for(T = 0; T < numeco; T++){
					if(H >= eco[T].minA && H <= eco[T].maxA){
						if(S <= eco[T].minSlope1 + (eco[T].minSlope2 - eco[T].minSlope1) /
							(eco[T].maxA - eco[T].minA) * (H - eco[T].minA)
							&& S >= eco[T].maxSlope1 + (eco[T].maxSlope2 - eco[T].maxSlope1) /
							(eco[T].maxA - eco[T].minA) * (H - eco[T].minA) ){
							break;
						}
					}
				}
				T = std::min(std::max(T, 0), numeco - 1);
				if(texid) SetTraw(x, y, T);
			}
			//Added use of Image's remap table.
			if(EcoOk[T]) SetCraw(x, y, eco[T].tex->remap[*(eco[T].tex->Data() + (x & xmask[T]) + (y & ymask[T]) * eco[T].tex->Pitch())]);
		}
	}
	return true;
}
bool Terrain::Lightsource(int x1, int y1, int x2, int y2){
	if(data == NULL || x2 < 0 || y2 < 0 || x1 > width || y1 > height) return false;	//Sanity.
	if(x2 == 0) x2 = width; if(y2 == 0) y2 = height;	//Default to whole shebang.
	x1 = std::max(0, x1); y1 = std::max(0, y1);	//Clip.
	x2 = std::min(width, x2); y2 = std::min(height, y2);	//Combined with sanity, all should be OK.
	int x, y, i;
	for(y = y1; y < y2; y++){
		for(x = x1; x < x2; x++){
			i = NUM_SHADE / 2 + (int)((double)NUM_SHADE / 2.0 * GetSlope(x, y, LightX, LightY, LightZ));	//Light intensity.
			SetCraw(x, y, ShadeLookup[GetCraw(x, y)][std::min(std::max(i, 0), NUM_SHADE - 1)]);
		}
	}
	return true;
}
int Terrain::ClearCMap(unsigned char val){
	if(data && width > 0 && height > 0){
		for(int y = 0; y < height; y++){
			for(int x = 0; x < width; x++){
				SetCraw(x, y, val);
			}
		}
		return 1;
	}
	return 0;
}

#define ECOSMOOTH

//Does one-pass texturing from TextureIDMap and lighting to 32bit color buffer.
bool Terrain::TextureLight32(EcoSystem *eco, int numeco, int x1, int y1, int x2, int y2, bool UseShadeMap){
	if(!data32){
		return false;
	//	if(NULL == (data32 = (ULONG*)malloc(width * height * sizeof(ULONG)))) return false;
	//	memset(data32, 0, width * height * 4);
	}
//	if(data == NULL || x2 < 0 || y2 < 0 || x1 > width || y1 > height ||
//		eco == NULL || numeco <= 0 || texid == NULL) return false;	//Sanity.
	//
	if(x2 == 0 && x1 == 0) x2 = width;
	if(y2 == 0 && y1 == 0) y2 = height;	//Default to whole shebang.
	//Adding support for wrapping coordinates.
	if(data == NULL || x2 <= x1 || y2 <= y1 || eco == NULL || numeco <= 0 || texid == NULL) return false;	//Sanity.
	//
//	x1 = std::max(0, x1); y1 = std::max(0, y1);	//Clip.
//	x2 = std::min(width, x2); y2 = std::min(height, y2);	//Combined with sanity, all should be OK.
	int x, y;
	//, i, T = 0;
//	float fi;
	PaletteEntry *tpe, *tpe2;
	int xmask[32], ymask[32], xshift[32];	//NOTE, textures must be power of two width and height now!
	char EcoOk[32];
	for(int e = 0; e < numeco; e++){
		xmask[e] = (1 << HiBit(eco[e].tex->Width())) - 1;
		xshift[e] = HiBit(eco[e].tex->Pitch());
		ymask[e] = (1 << HiBit(eco[e].tex->Height())) - 1;
		EcoOk[e] = (eco[e].tex->Data() != NULL);
	}
	//
	ScorchTex = std::min(ScorchTex, numeco - 1);
	//
	float invamb = 1.0f - Ambient;
	float amb = Ambient * (float)NUM_SHADE;
	//
	unsigned char *tdst;
	for(y = y1; y < y2; y++){
	//	tdst = ((unsigned char*)data32) + ((x1 & widthmask) + (y & heightmask) * width) * 4;
		tdst = ((unsigned char*)data32) + ((y & heightmask) * width) * 4;
		for(x = x1; x < x2; x++){
			int i, T;
			{
				float slope = GetSlope(x, y, LightX, LightY, LightZ);	//Lets try a "real" lighting calc, clamping negative cosine at 0.
				if(UseShadeMap){
					i = FastFtoL((amb + ((float)(NUM_SHADE - 1)) * invamb * std::min(std::max(slope, 0.0f), 1.0f)) * ((float)GetCwrap(x, y)) * 0.00390625f);	//Light intensity.
				}else{
					i = (int)((amb + ((float)(NUM_SHADE - 1)) * invamb * std::min(std::max(slope, 0.0f), 1.0f)));	//Light intensity.
				}
			}
			i = std::min(std::max(i, 0), NUM_SHADE - 1);
#ifdef ECOSMOOTH
			int r = 0, g = 0, b = 0;
			for(int n = 0; n < 4; n++){
				T = GetTwrap(x + (n & 1), y + (n >>1));
				if(EcoOk[T]){
					tpe = &eco[T].tex->pe[*(eco[T].tex->Data() + (x & xmask[T]) + ((y & ymask[T]) <<xshift[T]))];//* eco[T].tex->Pitch())];
					r += tpe->peRed;
					g += tpe->peGreen;
					b += tpe->peBlue;
				}
			}
			if(ScorchTex >= 0 && EcoOk[ScorchTex]){
				tpe2 = &eco[ScorchTex].tex->pe[*(eco[ScorchTex].tex->Data() + (x & xmask[ScorchTex]) + ((y & ymask[ScorchTex]) <<xshift[ScorchTex]))];//* eco[ScorchTex].tex->Pitch())];
				register int foo = ((x & widthmask) <<2);
				int si = ((255 - (int)GetCwrap(x, y)) * (NUM_SHADE)) >>8;	//Alpha blend to special scorch texture rather than to black.
				tdst[foo + 0] = ShadeLookup32[r >>2][i] + ShadeLookup32[tpe2->peRed][si];	//Average ecosystem pixels together.
				tdst[foo + 1] = ShadeLookup32[g >>2][i] + ShadeLookup32[tpe2->peGreen][si];
				tdst[foo + 2] = ShadeLookup32[b >>2][i] + ShadeLookup32[tpe2->peBlue][si];
			}else{
				register int foo = ((x & widthmask) <<2);
				tdst[foo + 0] = ShadeLookup32[r >>2][i];	//Average ecosystem pixels together.
				tdst[foo + 1] = ShadeLookup32[g >>2][i];
				tdst[foo + 2] = ShadeLookup32[b >>2][i];
			}
#else
			T = GetTwrap(x, y);//raw(x, y);
			T = std::min(T, numeco - 1);
			if(EcoOk[T]){
				if(ScorchTex >= 0 && EcoOk[ScorchTex]){
					tpe = &eco[T].tex->pe[*(eco[T].tex->Data() + (x & xmask[T]) + (y & ymask[T]) * eco[T].tex->Pitch())];
					tpe2 = &eco[ScorchTex].tex->pe[*(eco[ScorchTex].tex->Data() + (x & xmask[ScorchTex]) + (y & ymask[ScorchTex]) * eco[ScorchTex].tex->Pitch())];
					register int foo = ((x & widthmask) <<2);
					int si = ((255 - (int)GetCwrap(x, y)) * (NUM_SHADE)) >>8;	//Alpha blend to special scorch texture rather than to black.
					tdst[foo + 0] = ShadeLookup32[tpe->peRed][i] + ShadeLookup32[tpe2->peRed][si];
					tdst[foo + 1] = ShadeLookup32[tpe->peGreen][i] + ShadeLookup32[tpe2->peGreen][si];
					tdst[foo + 2] = ShadeLookup32[tpe->peBlue][i] + ShadeLookup32[tpe2->peBlue][si];
				}else{
					tpe = &eco[T].tex->pe[*(eco[T].tex->Data() + (x & xmask[T]) + (y & ymask[T]) * eco[T].tex->Pitch())];
					register int foo = ((x & widthmask) <<2);
					tdst[foo + 0] = ShadeLookup32[tpe->peRed][i];
					tdst[foo + 1] = ShadeLookup32[tpe->peGreen][i];
					tdst[foo + 2] = ShadeLookup32[tpe->peBlue][i];
				}
			}
#endif
		}
	}
	return true;
}

//Total range of slopes held for x and y slope.
#define SLOPETAB_SIZE 32
//Half that, IOW the absolute slope precision.
#define SLOPETAB_HALF 16
//How many voxels to deviate from target voxel in up/down/left/right directions when
//picking heights for slope differences.
#define SLOPETAB_GRAB 1
inline float Terrain::GetSlope(int x, int y, float lx, float ly, float lz){
	static float SlopeTab[SLOPETAB_SIZE][SLOPETAB_SIZE][3];
	static int TabMade = 0;
	float *tflt;
	int s1, s2;
	if(!TabMade){
		float ux, uy, uz, vx, vy, vz, nx, ny, nz, len;
		int sx, sy;
		for(sy = 0; sy < SLOPETAB_SIZE; sy++){
			for(sx = 0; sx < SLOPETAB_SIZE; sx++){
				uz = SLOPETAB_GRAB * 2.0f;
			//	uy = ((float)GetH(x, y + SLOPETAB_GRAB) - (float)GetH(x, y - SLOPETAB_GRAB)) / (float)(1 << VOXSQUEEZE);
				uy = (float)(sy - SLOPETAB_HALF) / (float)(1 << VOXSQUEEZE);
				ux = 0.0f;
				vz = 0.0f;
			//	vy = ((float)GetH(x + SLOPETAB_GRAB, y) - (float)GetH(x - SLOPETAB_GRAB, y)) / (float)(1 << VOXSQUEEZE);
				vy = (float)(sx - SLOPETAB_HALF) / (float)(1 << VOXSQUEEZE);
				vx = SLOPETAB_GRAB * 2.0f;
				nx = uy * vz - uz * vy;	//Compute normal of two vectors.
				ny = uz * vx - ux * vz;	//(Cross product.)
				nz = uy * vx - ux * vy;
				len = sqrtf(fabsf(nx * nx) + fabsf(ny * ny) + fabsf(nz * nz));	//Get length.
				SlopeTab[sx][sy][0] = nx / len;	//Normalize normal to 1.0 float.
				SlopeTab[sx][sy][1] = ny / len;
				SlopeTab[sx][sy][2] = nz / len;
			}
		}
		TabMade = 1;
	}
	s1 = GetHwrap(x + SLOPETAB_GRAB, y) - GetHwrap(x - SLOPETAB_GRAB, y) + SLOPETAB_HALF;
	s2 = GetHwrap(x, y + SLOPETAB_GRAB) - GetHwrap(x, y - SLOPETAB_GRAB) + SLOPETAB_HALF;
	tflt = &SlopeTab[Clip(s1, 0, SLOPETAB_SIZE - 1)][Clip(s2, 0, SLOPETAB_SIZE - 1)][0];
	return (tflt[0] * lx + tflt[1] * ly + tflt[2] * lz);
}
//Note: This function will only return good data if a color map (SetC) has NOT been applied to this voxel.
inline unsigned short Terrain::GetBigH(int x, int y){
	if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return 0;
	return *(data + x + y * width);
}
//Note: Using this function to set a 16-bit height value will overwrite any color map data.
inline bool Terrain::SetBigH(int x, int y, unsigned short v){
	if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
	*(data + x + y * width) = v;
	return true;
}
//Now wraps.
inline int Terrain::GetI(int x, int y){
//	if(data == NULL || x < 0 || y < 0 || (x >>FP) >= width - 1 || (y >>FP) >= height - 1) return 0;
	if(data == NULL) return 0;
	int xx = x >>FP;
	int yy = y >>FP;
	//return
	//	(
	//	(*(data + xx + yy * width) >> 8) * ((FPVAL - (x & 0xff)) + (FPVAL - (y & 0xff))) +
	//	(*(data + xx + 1 + yy * width) >> 8) * ((x & 0xff) + (FPVAL - (y & 0xff))) +
	//	(*(data + xx + (yy + 1) * width) >> 8) * ((FPVAL - (x & 0xff)) + (y & 0xff)) +
	//	(*(data + xx + 1 + (yy + 1) * width) >> 8) * ((x & 0xff) + (y & 0xff)) <<VP
	//	) >>(2 + FP);
	int h1, h2, h3, h4, top, bot;
//	h1 = *(data + xx + yy * width) >>8;
//	h2 = *(data + xx + 1 + yy * width) >>8;//) * ((x & 0xff) + (FPVAL - (y & 0xff))) +
//	h3 = *(data + xx + (yy + 1) * width) >>8;//) * ((FPVAL - (x & 0xff)) + (y & 0xff)) +
//	h4 = *(data + xx + 1 + (yy + 1) * width) >>8;//) * ((x & 0xff) + (y & 0xff)) <<VP
	h1 = GetHwrap(xx, yy);
	h2 = GetHwrap(xx + 1, yy);
	h3 = GetHwrap(xx, yy + 1);
	h4 = GetHwrap(xx + 1, yy + 1);
	top = (h1 <<8) + (x & 0xff) * (h2 - h1);
	bot = (h3 <<8) + (x & 0xff) * (h4 - h3);
	return ((top <<8) + (y & 0xff) * (bot - top)) >>(8 + 2);
}
float Terrain::FGetI(float x, float y){
	return (float)GetI((int)(x * FPVAL), (int)(y * FPVAL)) * INVFPVAL;/// (float)FPVAL;
}
inline AvgRng Terrain::Average(int x, int y, int p){
	AvgRng ar;
	if(data == NULL) return ar;
	int step = 1 << p, qnt = 1 << (p + 1);
	int num = 0, val = 0, total = 0, low = 255, high = 0;

	for(int yy = y - step; yy <= y + step; yy += step){
		if(yy >= 0 && yy < height){
			for(int xx = x - step; xx <= x + step; xx += step){
			//	if(xx % qnt == 0 && yy % qnt == 0){
				if(xx >= 0 && xx < width){
				//	if(0 != (val = *(hdata + xx + yy * width) )){
					if(0 != (val = *(data + xx + yy * width) >> 8 )){
					//if(0 != (val = GetH(xx, yy))){
					//if(0 != (val = GetH((xx / qnt) * qnt, (yy / qnt) * qnt))){
						num++;
						total += val;
						if(val < low) low = val;
						if(val > high) high = val;
					}
				}
			//	}
			}
		}
	}

	ar.a = (total <<FP) / std::max(num, 1);
	ar.r = std::max(high - low, 0);
	return ar;
}
bool Terrain::MakeShadeLookup(InversePal *inv){//PALETTEENTRY *pe){
	if(NULL == inv) return false;

	int r, g, b, c, i, col;//, k, diff, col, t;

	int floor = 0; //NUM_SHADE;
//	int floor = NUM_SHADE / 3;	//Boost "ambient" color a bit.
//	int floor = (int)((Ambient * (float)NUM_SHADE) / (1.0f - Ambient));// / 3;	//Boost "ambient" color a bit.

	for(c = 0; c < 256; c++){	//Original color register.
		for(i = 0; i < NUM_SHADE; i++){	//Intensity.
			r = (inv->pe[c].peRed * (i + 1 + floor)) / (NUM_SHADE + floor);
			g = (inv->pe[c].peGreen * (i + 1 + floor)) / (NUM_SHADE + floor);	//r,g,b, hold best-color for intensity.
			b = (inv->pe[c].peBlue * (i + 1 + floor)) / (NUM_SHADE + floor);
			/*
			col = 0;	//Last color reg with lowest difference found.
			diff = 2048;
		//	diff = 768;
		//	diff = 256;
			for(k = 0; k < 256; k++){
				t = (abs(pe[k].peRed - r) << 1) + (abs(pe[k].peGreen - g) << 2) + abs(pe[k].peBlue - b);
			//	t = abs(pe[k].peRed - r) + abs(pe[k].peGreen - g) + abs(pe[k].peBlue - b);
			//	t = abs(pe[k].peRed - r);
				if(t < diff){
					diff = t;
					col = k;
				}
			}
			*/
			col = inv->Lookup(r, g, b);
			//
			ShadeLookup[c][i] = col;
			//
			ShadeLookup32[c][i] = std::max(0, (c * (i + floor)) / (NUM_SHADE + floor));//>>NUM_SHADE_SHIFT);
		}
	}
	return true;
}
