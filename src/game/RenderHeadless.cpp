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

// Stub for headless

#include "Render.h"

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
}
bool RenderEngine::SetSkyRotate(float rotspeed){
	return true;
}
bool RenderEngine::SetDetailRot(float rotspeed, float rotrad){
	return true;
}
bool RenderEngine::SetSkyTexture(Image *sky, Image *skyenv){
	return false;
}
bool RenderEngine::SetSkyBoxTexture(int face, Image *tex){
	return false;
}

bool RenderEngine::SetDetailTexture(Image *detail, Image *detailmt){
	return false;
}
bool RenderEngine::SetWaterTexture(Image *water, float wateralpha, float waterscale, float waterenv){
	return false;
}
bool RenderEngine::SetFogColor(float r, float g, float b){
	return true;
}
