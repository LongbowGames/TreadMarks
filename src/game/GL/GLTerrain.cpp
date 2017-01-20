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

#include "GLTerrain.h"
#include "../GenericBuffer.h"

#include "../Terrain.h"
#include "../ResourceManager.h"
#include "../CfgParse.h"

#include <GL/glew.h>


void Terrain::UndownloadTextures(){
	if(TexIDs[0][0] != 0){
		GLDeleteTextures(TEXIDSIZE * TEXIDSIZE, (unsigned int*)TexIDs);
		memset(TexIDs, 0, TEXIDSIZE * TEXIDSIZE * 4);
	}
}
int Terrain::MakeTexturePalette(EcoSystem *eco, int numeco){	//Makes a map-specific texture palette, for use with paletted terrain textures.
	if(!eco) return 0;
	TerrainQuant.ClearPalette();
	for(int i = 0; i < numeco; i++){
		if(eco[i].tex) TerrainQuant.AddPalette(eco[i].tex->pe, 256, 6);
	}
	PaletteEntry pe[256];
	TerrainQuant.GetCompressedPalette(pe, 256);
	TerrainInv.Init(INVPALBITS, INVPALBITS, INVPALBITS);
	TerrainInv.Make(pe);
	return 1;
}
void Terrain::UsePalettedTextures(bool usepal){
	PalTextures = usepal;
}

bool Terrain::DownloadTextures(){
	Bitmap tbmp;	//Temporary storage, ASSUMES ROWS WILL BE PACKED TIGHTLY TO DWORDS AND WITH SANE PITCH!  Currently this is true of Bitmap objects.
	int w = Width(), h = Height(), x, y;
	//
	glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
	glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	//
	if(data32 && tbmp.Init(TexSize, TexSize, 32)){
		//
		OutputDebugLog("Downloading Terrain Textures.\n");
		//
		//Generate texture IDs.
		if(TexIDs[0][0] != 0){
			GLDeleteTextures(TEXIDSIZE * TEXIDSIZE, (GLuint*)TexIDs);
		}
		GLGenTextures(TEXIDSIZE * TEXIDSIZE, (GLuint*)TexIDs);
		//First patch seams by copying neighboring data.
		unsigned long *tdd, *tds;
		for(y = 0; y < h; y += TexSize){	//Horizontal lines.
			tdd = data32 + y * w;
			tds = data32 + ((y - 1) & (h - 1)) * w;
			for(x = 0; x < w; x++) tdd[x] = tds[x];
		}
		for(x = 0; x < w; x += TexSize){	//Vertical lines.
			tdd = data32 + x;
			tds = data32 + ((x - 1) & (w - 1));
			for(y = 0; y < h; y++){
				*tdd = *tds;
				tdd += w;
				tds += w;
			}
		}
		//Then download textures.
		for(y = 0; y < h; y += TexSize){
			for(x = 0; x < w; x += TexSize){
				//Break color map into textures of TexSize width and height.
				glBindTexture(GL_TEXTURE_2D, TexIDs[x / TexSize][y / TexSize]);
			//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);//GL_MODULATE);//GL_DECAL);
				//
				//Stupid 3DFX opengl driver doesn't like row_length...
				for(int y2 = y; y2 < y + TexSize; y2++){
					memcpy(tbmp.Data() + (y2 - y) * TexSize * 4, data32 + x + y2 * w, TexSize * 4);
				}
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,//GL_RGB,
					TexSize, TexSize, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, tbmp.Data());
			}
		}
		OutputDebugLog("Finished Downloading Terrain Textures.\n");
		memset(TexDirty, 0, sizeof(TexDirty));	//Clear dirty terrain rect list.
		return true;
	}
	return false;
}
int Terrain::UpdateTextures(int x1, int y1, int w, int h){
//	if(!data32) return 0;
	w++;
	h++;	//Add an extra, so patch that should suck from left or top always does so.
	int px, py, dx, dy, dw, dh;
	int startpx, startpy, endpx, endpy;
	if(x1 < 0) startpx = (x1 - (TexSize - 1)) / TexSize;
	else startpx = x1 / TexSize;
	if(y1 < 0) startpy = (y1 - (TexSize - 1)) / TexSize;
	else startpy = y1 / TexSize;
	if(x1 + w - 1 < 0) endpx = (x1 + w - 1 - (TexSize - 1)) / TexSize;
	else endpx = (x1 + w - 1) / TexSize;
	if(y1 + h - 1 < 0) endpy = (y1 + h - 1 - (TexSize - 1)) / TexSize;
	else endpy = (y1 + h - 1) / TexSize;
	//
	int tpw = Width() / TexSize, tph = Height() / TexSize;
	//
	//Loop over all texture patches covered by rectangle.
	for(py = startpy; py <= endpy; py++){
		for(px = startpx; px <= endpx; px++){
			dx = std::max(0, x1 - (px * TexSize));	//Coords inside current tex patch.
			dy = std::max(0, y1 - (py * TexSize));
			dw = std::min(TexSize - dx, x1 + w - (px * TexSize + dx));
			dh = std::min(TexSize - dy, y1 + h - (py * TexSize + dy));
			//
			ByteRect br(dx, dy, dw, dh);
			ByteRect &br2 = TexDirty[px & (tpw - 1)][py & (tph - 1)];
			if(br2.w <= 0 || br2.h <= 0){	//Just set rect.
				br2 = br;
			}else{	//Take maximum of two rects.
				if(br.x < br2.x){
					br2.w += br2.x - br.x;
					br2.x = br.x;
				}
				if(br.y < br2.y){
					br2.h += br2.y - br.y;
					br2.y = br.y;
				}
				if(br.w + br.x > br2.w + br2.x) br2.w = (br.x + br.w) - br2.x;
				if(br.h + br.y > br2.h + br2.y) br2.h = (br.y + br.h) - br2.y;
			}
			//
		}
	}
	return 1;
}

int Terrain::Redownload(int px, int py){
	if(!data32) return 0;
	static unsigned long tbuf[TexSize * TexSize];
	int tpw = Width() / TexSize, tph = Height() / TexSize;
	//
	GLuint id = TexIDs[px & (tpw - 1)][py & (tph - 1)];
	ByteRect &br = TexDirty[px & (tpw - 1)][py & (tph - 1)];
	if(id > 0 && br.w > 0 && br.h > 0 &&
		br.x < TexSize && br.y < TexSize && br.x + br.w <= TexSize && br.y + br.h <= TexSize){
		if(br.x & 1){ br.x--; br.w++; }
		if(br.y & 1){ br.y--; br.h++; }
		if(br.w & 1) br.w++;	//Always have even widths and heights.
		if(br.h & 1) br.h++;
		//
		glBindTexture(GL_TEXTURE_2D, id);
		unsigned char *ts, *td = (unsigned char*)tbuf;
		int xs = (px & (tpw - 1)) * TexSize + br.x;
		int ys = (py & (tph - 1)) * TexSize + br.y;
		//
	//	if(ys < 0 || ys + br.h > Height() || xs < 0 || xs + br.w > Width()){
	//		OutputDebugLog("! ! ! ! Bad download!  xs" + String(xs) + " ys" + String(ys) + " w" +
	//			String((int)br.w) + " h" + String((int)br.h) +
	//			" x" + String((int)br.x) + " y" + String((int)br.y) + "\n");
	//	}else{
			//
		if(br.x == 0){	//Suck from left.
			for(int y = ys; y < ys + br.h; y++){
				*(data32 + xs + y * Width()) = *(data32 + ((xs - 1) & (Width() - 1)) + y * Width());
			}
		}
		if(br.y == 0){	//Suck from top.
			for(int x = xs; x < xs + br.w; x++){
				*(data32 + x + ys * Width()) = *(data32 + x + ((ys - 1) & (Height() - 1)) * Width());
			}
		}
		//
		ts = (unsigned char*)(data32 + ys * Width() + xs);
		for(int y = ys; y < ys + br.h; y++){
		//	memcpy(td, ts, ((unsigned int)br.w) <<2);
			for(int t = 0; t < ((unsigned int)br.w); t++) ((unsigned long*)td)[t] = ((unsigned long*)ts)[t];
			ts += Width() <<2;
			td += ((unsigned int)br.w) <<2;
		}
		glTexSubImage2D(GL_TEXTURE_2D, 0, br.x, br.y, br.w, br.h,
			GL_RGBA, GL_UNSIGNED_BYTE, tbuf);
		br.x = br.y = br.w = br.h = 0;
		return 1;
	}
	br.x = br.y = br.w = br.h = 0;
	return 0;
}



LodTree LodTreeMap::dummy;

LodTreeMap LodMap;	//Single global variable.

bool Terrain::MapLod(){
	//
//	OutputDebugLog("sizeof(BinaryTriangle) = " + String((int)sizeof(BinaryTriangle)) + "\n");
	//
	int pw = Width() / TexSize, ph = Height() / TexSize;
	LodMap.Init(pw, ph);
	for(int y = 0; y < ph; y++){
		for(int x = 0; x < pw; x++){	//Build lod variance trees for entire map.
			int x1 = x * TexSize, y1 = y * TexSize;
			int x2 = x1 + TexSize, y2 = y1 + TexSize;
			LodMap.Tree(x, y, 0)->BuildLodTree(this, x1, y2, x2, y1, x1, y1);
			LodMap.Tree(x, y, 1)->BuildLodTree(this, x2, y1, x1, y2, x2, y2);
		}
	}
	return true;
}
bool Terrain::MapLod(int x, int y, int w, int h){
	//This is simple and hacky and very non-optimal.  It should check whether it should
	//continue diving at each bintri.  It'll still get a 32x+ savings right now though.
//	int xc = x + w / 2, yc = y + h / 2;
//	int xt = xc / TexSize, yt = yc / TexSize;
	int xfirst = x / TexSize, yfirst = y / TexSize;
	int xlast = (x + w) / TexSize, ylast = (y + h) / TexSize;
	//Now handle huge changes, such as nuke, by iterating over every square affected.
	for(int yt = yfirst; yt <= ylast; yt++){
		for(int xt = xfirst; xt <= xlast; xt++){
			int x1 = xt * TexSize, y1 = yt * TexSize;
			int x2 = x1 + TexSize, y2 = y1 + TexSize;
		//	int xv = xc - x2, yv = yc - y1;
		//	if(abs(xv) > abs(yv)){
			LodMap.Tree(xt, yt, 0)->waterflag = 0;
			LodMap.Tree(xt, yt, 0)->BuildLodTree(this, x1, y2, x2, y1, x1, y1);
		//	}else{
			LodMap.Tree(xt, yt, 1)->waterflag = 0;
			LodMap.Tree(xt, yt, 1)->BuildLodTree(this, x2, y1, x1, y2, x2, y2);
		//	}
		}
	}
	return true;
}

