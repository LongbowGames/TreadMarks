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

#include "Poly.h"

PolyRender::PolyRender(){
//	for(int i = 0; i < MAX_YSHEAR; i++){
//		YShear[i] = 0;
//	}
//	nObject = nTPoly = nTVertex = 0;
//	YBufUsed = 0;
	LODLimit = 0;
//	DisableParticles = 0;
	ParticleAtten = 1.0f;
	DisableFog = false;
	ShowPolyNormals = false;
	EnvMap1 = EnvMap2 = NULL;
	//
	AlphaTestOn = false;
}
PolyRender::~PolyRender(){
}
bool PolyRender::SetEnvMap1(Image *env1){
	return false;
}
bool PolyRender::SetEnvMap2(Image *env2){
	return false;
}
void PolyRender::UnlinkTextures(){
}
bool PolyRender::LimitLOD(int level){
	return false;
}
void PolyRender::Particles(float atten){//int yesno){
}
void PolyRender::Fog(bool yesno){
}
void PolyRender::AlphaTest(bool yesno){
}
void PolyRender::PolyNormals(bool yesno){
}
void PolyRender::SetLightVector(float lx, float ly, float lz, float amb){
}

bool PolyRender::InitRender(){
	return true;
}

bool PolyRender::SetupCamera(Camera *Cm){
	return true;
}

bool PolyRender::AddSolidObject(Object3D *obj){
	return false;
}
bool PolyRender::AddSecondaryObject(Object3D *obj){
	return false;
}
bool PolyRender::AddTransObject(Object3D *obj){
	return false;
}
bool PolyRender::AddOrthoObject(ObjectOrtho *obj){
	return false;
}

bool LineMapObject::AddLine(float X1, float Y1, float X2, float Y2, float R, float G, float B){
	return false;
}
bool LineMapObject::AddPoint(float X, float Y, float Size, float R, float G, float B, float Ramp){
	return false;
}


void MeshObject::Render(PolyRender *PR){
}
void SpriteObject::Render(PolyRender *PR){
}
void ParticleCloudObject::Render(PolyRender *PR){
}
void StringObject::Render(PolyRender *PR){
}
void LineMapObject::Render(PolyRender *PR){
}
void TilingTextureObject::Render(PolyRender *PR){
}
void Chamfered2DBoxObject::Render(PolyRender *PR){
}
