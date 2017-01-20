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

#pragma inline_depth(255)

#include "GLPolyRender.h"
#include "../GenericBuffer.h"

#include <GL/glew.h>

#include "../Terrain.h"
#include "../Render.h"
#include "../CamRBD.h"
#include "../Image.h"
#include "../ResourceManager.h"
#include "../Basis.h"
#include "../CfgParse.h"

using namespace std;

static Basis Perturber(12345);

extern GLenum GL_Best_Clamp;

//Everything uses it, so everything will default to GL_MODULATE env mode now,

void GLPolyRender::GLResetStates(){
	blendmode = BLEND_UNSET;
	texunbound = true;
}
void GLPolyRender::GLBlendMode(int mode){
	if(blendmode == BLEND_UNSET || mode != blendmode){
		if(mode == BLEND_NONE){
			glDisable(GL_BLEND);
		}else{
			if(blendmode == BLEND_NONE || blendmode == BLEND_UNSET){
				glEnable(GL_BLEND);
			}
			switch(mode){
			case BLEND_ADD : glBlendFunc(GL_ONE, GL_ONE); break;
			case BLEND_ALPHA : glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
			case BLEND_MOD : glBlendFunc(GL_DST_COLOR, GL_ZERO); break;
			}
		}
		blendmode = mode;
	}
}
void GLPolyRender::GLBindTexture(unsigned int id){
	if(id != boundtex || texunbound){
		glBindTexture(GL_TEXTURE_2D, id);
		boundtex = id;
		texunbound = false;
	}
}

//************************************************************************************
//							PolyMesh/Sprite Rendering starts here!
//************************************************************************************
//
#define MAXTEMPVERTS 2048
//For storing pre-calculated ENV map texcoords.
static float TempTexCoords[MAXTEMPVERTS][2];
//For storing interim real-time modified vertex coords.
static Vector TempVertCoords[MAXTEMPVERTS];
//
inline void PolarEnvMapCoords(float *out, Vec3 vert, Vec3 norm, Mat3 envbasis, Vec3 campos){
	//r = u - 2 * n * (n dot u)
	Vec3 u, tv, r;
	SubVec3(vert, campos, u);
	NormVec3(u);
	ScaleVec3(norm, 2.0f * DotVec3(norm, u), tv);
	SubVec3(u, tv, r);
	//
	out[0] = atan2f(DotVec3(r, envbasis[0]), DotVec3(r, envbasis[2])) * PIRECIP * 0.5f + 0.5f;
	out[1] = -DotVec3(envbasis[1], r) * 0.5f + 0.5f;
}
inline void SphericalYEnvMapCoords(float *out, Vec3 vert, Vec3 norm, Mat3 envbasis, Vec3 campos, float scale){
	//r = u - 2 * n * (n dot u)
	Vec3 tv, r;
//	SubVec3(vert, campos, u);
	Vec3 u = {vert[0] * scale - campos[0], vert[1] * scale - campos[1], vert[2] * scale - campos[2]};
//	NormVec3(u);
	ScaleVec3(u, fsqrt_inv(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]));
	//
	ScaleVec3(norm, 2.0f * DotVec3(norm, u), tv);
	SubVec3(u, tv, r);
	//
//	Vec3 r2;
//	AddVec3(r, envbasis[1], r2);
	Vec3 r2 = {r[0] + envbasis[0][1], r[1] + envbasis[1][1], r[2] + envbasis[2][1]};
	//Need to add inverse matrix Y axis instead...
//	Vec3MulMat3(r, envbasis, r2);
//	tv[0] = r2[0];  tv[1] = r2[1] + 1.0f;  tv[2] = r2[2];
//	float im = 1.0f / (2.0f * LengthVec3(r2));	//1 / (2 * sqrtf((rx+envyx)^2 + (ry+envyy)^2 + (rz+envyz)^2))
	float im = 0.5f * fsqrt_inv(r2[0] * r2[0] + r2[1] * r2[1] + r2[2] * r2[2]);
	//
//	out[0] = DotVec3(r, envbasis[0]) * im + 0.5f;
//	out[1] = DotVec3(r, envbasis[2]) * im + 0.5f;
	out[0] = (r[0] * envbasis[0][0] + r[1] * envbasis[1][0] + r[2] * envbasis[2][0]) * im + 0.5f;
	out[1] = (r[0] * envbasis[0][2] + r[1] * envbasis[1][2] + r[2] * envbasis[2][2]) * (-im) + 0.5f;	//Positive Z model space is forwards, and negative (up) is forwards in env map.
//	out[0] = r2[0] * im + 0.5f;
//	out[1] = r2[2] * im + 0.5f;
}
//float tex[20][2];	//Temporary texcoord storage for finding broken envmap texcoords in strips.
//int ntt = 0;
#define TEXMODE_NORMAL 0
#define TEXMODE_ENVSMOOTH 1
#define TEXMODE_ENVFLAT 2
#define TEXMODE_ENVPRECALC 3
#define COLMODE_FLAT 0
#define COLMODE_SMOOTH 1
#define COLMODE_SOLID 2
#define COLMODE_EDGETRANS 3
//
void GLPolyRender::GLRenderMeshObject(MeshObject *thismesh, PolyRender *PR){
	if(thismesh->M){// && Texture){
		//
		float tex[10][2];
		//
		int flags = thismesh->Flags;//Obj->Flags;
	//	flags |= MESH_BLEND_ENVMAP | MESH_ENVMAP1_SPHERE;// | MESH_SHADE_SMOOTH | MESH_ENVMAP_SMOOTH;
	//	Obj->Opacity = 0.5f;
	//	flags |= MESH_SHADE_SMOOTH;
		//
		Mesh *m = thismesh->M;//Obj->M;	//Temporary working mesh pointer.
		//Do LOD!
		//Calc zdist.
		float z = PR->zplane[0] * thismesh->Pos[0] + PR->zplane[1] * thismesh->Pos[2] + PR->zplane[2];
		//
		while((z > (thismesh->LodBias * PR->Cam.viewplane * (float)(m->LodLevel + 1)) ||
			m->LodLevel < PR->LODLimit) && m->NextLOD != NULL) m = m->NextLOD;
	//	while((Obj->xfpos[2] > (m->LodBias * Cam.viewplane * (float)(m->LodLevel + 1)) ||
	//		m->LodLevel < LODLimit) && m->NextLOD != NULL) m = m->NextLOD;
		//
		//Create model*view matrix.
		Mat43 modelview;
		for(int i = 0; i < 3; i++) Vec3MulMat3(thismesh->Orient[i], PR->view, modelview[i]);
		Vec3MulMat43(thismesh->Pos, PR->view, modelview[3]);
		//
	//	Mat43MulMat43(Obj->model, view, modelview);
		GLfloat gm[16];
		memset(gm, 0, sizeof(gm));
		for(int i = 0; i < 4; i++){
			CopyVec3(&modelview[i][0], &gm[i * 4]);
			gm[i * 4 + 2] = -gm[i * 4 + 2];	//Flip coord system.
		}
		gm[15] = 1.0f;
		//
		float scale = 1.0f;
		if(flags & MESH_SCALE_MATRIX){
			scale = LengthVec3(modelview[1]);
			NormMat3(modelview);	//Remove scaling component from local matrix, leave in OpenGL matrix.
		}
		//
		Vec3 light = {PR->LightX, PR->LightY, PR->LightZ};	//Temporary global light source.
		float InvAmbient = 1.0f - PR->Ambient;
		Vec3 tlv;
		Vec3MulMat3(light, PR->view, tlv);	//Light is now in camera space.
		Vec3IMulMat3(tlv, modelview, light);	//Light is now in Object Space.
		float l;
		//
	//	glShadeModel(GL_SMOOTH);//GL_FLAT);
		//
		if(flags & MESH_DISABLE_CULL) glDisable(GL_CULL_FACE);
		//
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadMatrixf(gm);
		//
		float opac = thismesh->Opacity;
		if(!(flags & MESH_BLEND_ADD) && (opac >= 1.0f || (flags & MESH_BLEND_ENVMAP))){
		//	BlendEnabled(0);
			PR->GLBlendMode(BLEND_NONE);
		}else{
		//	BlendEnabled(1);
			if(flags & MESH_BLEND_ADD) PR->GLBlendMode(BLEND_ADD);//BlendModeAdd();
			else PR->GLBlendMode(BLEND_ALPHA);//BlendModeAlpha();
		//	glDepthMask(GL_FALSE);
		}
		//
		//Environment mapping setup.
		Mat3 envbasis;
		Vec3 camoff;
	//	if(flags & MESH_ENVBASIS_MODEL){	//Model matrix to pull envmap basis with it.
	//		IdentityMat3(envbasis);
	//	}else{
		//	for(int i = 0; i < 3; i++) CopyVec3(Obj->model[i], envbasis[i]);
		Mat3 lightmat;
		IdentityMat3(lightmat);
		if(flags & MESH_ENVBASIS_LIGHT){	//TODO:  Add a way to config this flag through mesh options in cfg file.
			lightmat[1][0] = PR->LightX;  lightmat[1][1] = PR->LightY;  lightmat[1][2] = PR->LightZ;
			CrossVec3(lightmat[0], lightmat[1], lightmat[2]);
			NormVec3(lightmat[2]);	//This is needed!!  Argh.
			CrossVec3(lightmat[1], lightmat[2], lightmat[0]);	//Create matrix based on light vector.  Rotate basis so that "top" of ENV map points at light.
		}
		Mat3 tlmat;
		if(flags & MESH_ENVBASIS_MODEL){	//Model matrix to pull envmap basis with it.
			Mat3MulMat3(thismesh->Orient, lightmat, tlmat);
		}else{
			for(int i = 0; i < 3; i++) CopyVec3(lightmat[i], tlmat[i]);
		}
		Mat3IMulMat3(thismesh->Orient, tlmat, envbasis);
	//	}
	//	Mat3MulMat3(tm, Obj->model, envbasis);
		//
		if(flags & MESH_SCALE_MATRIX) NormMat3(envbasis);
		//
		Vec3 tv = {0, 0, 0};
		Vec3IMulMat43(tv, modelview, camoff);
		//
		//Pre-calc envmap texcoords if appropriate.
		int EnvPrecalced = 0;
		if((flags & MESH_ENVMAP_SMOOTH) && (flags & MESH_ENVMAP_MASK) && m->nVertex < MAXTEMPVERTS){
			for(int v = 0; v < m->nVertex; v++){
				SphericalYEnvMapCoords(&TempTexCoords[v][0], m->Vertex[v], m->VertNorm[v], envbasis, camoff, scale);
			}
			EnvPrecalced = 1;
		}
		//
		//
		Vector *VertA = m->Vertex;	//Current vertex array pointer.
		//
		//Experiment with Noise based distortion!
		if((thismesh->Flags & MESH_PERTURB_NOISE || thismesh->Flags & MESH_PERTURB_NOISE_Z) && m->nVertex < MAXTEMPVERTS){
		//	float cscale = 1.0f / 5.0f;
			float cscale = thismesh->PerturbScale;
			if(thismesh->Flags & MESH_PERTURB_NOISE_Z){	//Special mode for YZ plane noise scaled by Z for insignia flags.
				for(int v = 0; v < m->nVertex; v++){
					CVec3 tv(m->Vertex[v].y + thismesh->PerturbX, m->Vertex[v].x, m->Vertex[v].z + thismesh->PerturbZ);
					Vec3 pt = {Perturber.Noise((tv.x + tv.y) * cscale, (tv.z + tv.y) * cscale, 2) - 0.5f,
						Perturber.Noise((tv.x + tv.y) * cscale + 55.0f, (tv.z - tv.y) * cscale + 55.0f, 2) - 0.5f,
						Perturber.Noise((tv.x - tv.y) * cscale + 99.0f, (tv.z + tv.y) * cscale + 99.0f, 2) - 0.5f};
					ScaleAddVec3(pt, thismesh->Perturb * m->Vertex[v].z, m->Vertex[v], TempVertCoords[v]);
				}
			}else{
				for(int v = 0; v < m->nVertex; v++){
				//	CVec3 tv;
				//	Vec3MulMat3(m->Vertex[v], Orient, tv);
				//	AddVec3(Pos, tv);
					CVec3 tv(m->Vertex[v].x + thismesh->PerturbX, m->Vertex[v].y, m->Vertex[v].z + thismesh->PerturbZ);
					Vec3 pt = {Perturber.Noise((tv.x + tv.y) * cscale, (tv.z + tv.y) * cscale, 2) - 0.5f,
						Perturber.Noise((tv.x + tv.y) * cscale + 55.0f, (tv.z - tv.y) * cscale + 55.0f, 2) - 0.5f,
						Perturber.Noise((tv.x - tv.y) * cscale + 99.0f, (tv.z + tv.y) * cscale + 99.0f, 2) - 0.5f};
					ScaleAddVec3(pt, thismesh->Perturb, m->Vertex[v], TempVertCoords[v]);
				}
			}
			VertA = TempVertCoords;
		}
		//
		//Create object-space camera heading vector, for trans edges.
		Vec3 CameraZ, tcz = {0, 0, 1};
		Vec3IMulMat3(tcz, modelview, CameraZ);
		//
		//
		for(int iter = 0; iter < (flags & MESH_BLEND_ENVMAP ? 2 : 1); iter++){
			int ColMode, TexMode;	//Coordinate and color generation modes.
			float LightScale = 1.0f;	//Scales post-ambient light and solid light, to darken in total for additive mixing.
			//
			//Set rendering flags for this pass as appropriate.
			if((flags & MESH_ENVMAP_MASK) && (!(flags & MESH_BLEND_ENVMAP) || iter == 1)){
				TexMode = (flags & MESH_ENVMAP_SMOOTH ? TEXMODE_ENVSMOOTH : TEXMODE_ENVFLAT);
				if(EnvPrecalced) TexMode = TEXMODE_ENVPRECALC;
				ColMode = COLMODE_SOLID;
				if(flags & MESH_SHADE_EDGETRANS){
					ColMode = COLMODE_EDGETRANS;
				}
				if(flags & MESH_BLEND_ENVMAP){
				//	BlendEnabled(1);
				//	BlendModeAdd();
					PR->GLBlendMode(BLEND_ADD);
					glDepthFunc(GL_LEQUAL);
				//	glEnable(GL_POLYGON_OFFSET_FILL);
				//	glPolygonOffset(-0.2f, -0.5f);
					opac = 1.0f;
					LightScale = 1.0f - thismesh->Opacity;
				}
				if(flags & MESH_BLEND_ADD){
					opac = 1.0f;
					LightScale = thismesh->Opacity;
				//	PR->GLBlendMode(BLEND_ADD);
				}//else{
				//	PR->GLBlendMode(BLEND_NONE);
				//}
				if(flags & MESH_ENVMAP1_SPHERE && PR->EnvMap1) PR->GLBindTexture(PR->EnvMap1->id);
				if(flags & MESH_ENVMAP2_SPHERE && PR->EnvMap2) PR->GLBindTexture(PR->EnvMap2->id);
			//	if(flags & MESH_ENVMAPT_SPHERE && m->Poly[0].Tex) PR->GLBindTexture(m->Poly[0].Tex->id);
				if(flags & MESH_ENVMAPT_SPHERE && thismesh->Texture) PR->GLBindTexture(thismesh->Texture->id);
			}else{
				TexMode = TEXMODE_NORMAL;
			//	ColMode = (flags & MESH_SHADE_SMOOTH ? COLMODE_SMOOTH : (flags & MESH_SHADE_SOLID ? COLMODE_SOLID : COLMODE_FLAT));
				ColMode = COLMODE_FLAT;
				if(flags & MESH_SHADE_SMOOTH) ColMode = COLMODE_SMOOTH;
				if(flags & MESH_SHADE_SOLID) ColMode = COLMODE_SOLID;
				if(flags & MESH_SHADE_EDGETRANS) ColMode = COLMODE_EDGETRANS;
				//
				if(flags & MESH_BLEND_ENVMAP || flags & MESH_BLEND_ADD){
					opac = 1.0f;
					LightScale = thismesh->Opacity;
				//	if(flags & MESH_BLEND_ADD) PR->GLBlendMode(BLEND_ADD);
				//	else PR->GLBlendMode(BLEND_NONE);
				}
			//	if(m->Poly[0].Tex) PR->GLBindTexture(m->Poly[0].Tex->id);
				if(thismesh->Texture) PR->GLBindTexture(thismesh->Texture->id);
			}
			//
		//	if(m->Poly[0].Tex) BindTexture(m->Poly[0].Tex->id);
			Poly3D *pp = m->Poly;
			for(int p = 0; p < m->nPoly; p++){
				if(pp->Flags == POLY_STRIP){
					if(TexMode == TEXMODE_NORMAL){
						tex[0][0] = pp->UV.U[0];  tex[0][1] = pp->UV.V[0];
					}else if(TexMode == TEXMODE_ENVSMOOTH){
						SphericalYEnvMapCoords(&tex[0][0], VertA[pp->Points[0]], m->VertNorm[pp->Points[0]], envbasis, camoff, scale);
					}else if(TexMode == TEXMODE_ENVPRECALC){
						tex[0][0] = TempTexCoords[pp->Points[0]][0];
						tex[0][1] = TempTexCoords[pp->Points[0]][1];
					}else{	//TEXMODE_ENVFLAT
						SphericalYEnvMapCoords(&tex[0][0], VertA[pp->Points[0]], pp->Normal, envbasis, camoff, scale);
					}
					if(ColMode == COLMODE_FLAT){
						l = DotVec3(light, pp->Normal);  l = (std::max(0.0f, l) * InvAmbient + PR->Ambient) * LightScale;	//Ok, now direct lighting plus ambient.
						glColor4f(l, l, l, opac);
						glTexCoord2fv(&tex[0][0]);
						glVertex3fv(VertA[pp->Points[0]]);
					}else if(ColMode == COLMODE_SMOOTH){
						l = DotVec3(light, m->VertNorm[pp->Points[0]]);  l = (std::max(0.0f, l) * InvAmbient + PR->Ambient) * LightScale;	//Ok, now direct lighting plus ambient.
						glColor4f(l, l, l, opac);
						glTexCoord2fv(&tex[0][0]);
						glVertex3fv(VertA[pp->Points[0]]);
					}else if(ColMode == COLMODE_EDGETRANS){
						float edge = fabsf(DotVec3(m->VertNorm[pp->Points[0]], CameraZ));
						glColor4f(LightScale * edge, LightScale * edge, LightScale * edge, opac * edge);
						glTexCoord2fv(&tex[0][0]);
						glVertex3fv(VertA[pp->Points[0]]);
					}else{	//COLMODE_SOLID
						glTexCoord2fv(&tex[0][0]);
						glVertex3fv(VertA[pp->Points[0]]);
					}
				}else{
					if(p > 0) glEnd();
					glBegin(GL_TRIANGLE_STRIP);
					if(TexMode == TEXMODE_NORMAL){
						for(int i = 0; i < 3; i++){ tex[i][0] = pp->UV.U[i];  tex[i][1] = pp->UV.V[i]; }
					}else if(TexMode == TEXMODE_ENVSMOOTH){
						for(int i = 0; i < 3; i++)
							SphericalYEnvMapCoords(&tex[i][0], VertA[pp->Points[i]], m->VertNorm[pp->Points[i]], envbasis, camoff, scale);
					}else if(TexMode == TEXMODE_ENVPRECALC){
						for(int i = 0; i < 3; i++){
							tex[i][0] = TempTexCoords[pp->Points[i]][0];
							tex[i][1] = TempTexCoords[pp->Points[i]][1];
						}
					}else{	//TEXMODE_ENVFLAT
						for(int i = 0; i < 3; i++)
							SphericalYEnvMapCoords(&tex[i][0], VertA[pp->Points[i]], pp->Normal, envbasis, camoff, scale);
					}
					if(ColMode == COLMODE_FLAT){
						l = DotVec3(light, pp->Normal);  l = (std::max(0.0f, l) * InvAmbient + PR->Ambient) * LightScale;	//Ok, now direct lighting plus ambient.
						glColor4f(l, l, l, opac);
						for(int i = 0; i < 3; i++){
							glTexCoord2fv(&tex[i][0]);
							glVertex3fv(VertA[pp->Points[i]]);
						}
					}else if(ColMode == COLMODE_SMOOTH){
						for(int i = 0; i < 3; i++){
							l = DotVec3(light, m->VertNorm[pp->Points[i]]);  l = (std::max(0.0f, l) * InvAmbient + PR->Ambient) * LightScale;	//Ok, now direct lighting plus ambient.
							glColor4f(l, l, l, opac);
							glTexCoord2fv(&tex[i][0]);
							glVertex3fv(VertA[pp->Points[i]]);
						}
					}else if(ColMode == COLMODE_EDGETRANS){
						for(int i = 0; i < 3; i++){
							float edge = fabsf(DotVec3(m->VertNorm[pp->Points[i]], CameraZ));
							glColor4f(LightScale * edge, LightScale * edge, LightScale * edge, opac * edge);
							glTexCoord2fv(&tex[i][0]);
							glVertex3fv(VertA[pp->Points[i]]);
						}
					}else{	//COLMODE_SOLID
						glColor4f(LightScale, LightScale, LightScale, opac);
						for(int i = 0; i < 3; i++){
							glTexCoord2fv(&tex[i][0]);
							glVertex3fv(VertA[pp->Points[i]]);
						}
					}
				}
				pp++;
			}
			glEnd();
		}
		//
	//	glDisable(GL_POLYGON_OFFSET_FILL);
	//	glPolygonOffset(0.0f, 0.0f);
		glDepthFunc(GL_LESS);
		//
		//Display vertex normals.
		if(PR->ShowPolyNormals){
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINES);
			glColor4f(1, 0, 0, 1);
			for(int v = 0; v < m->nVertNorm; v++){
				Vec3 tv;
				glVertex3fv(m->Vertex[v]);
				AddVec3(m->Vertex[v], m->VertNorm[v], tv);
				glVertex3fv(tv);
			}
			//Do poly norms too.
			glColor4f(0, 1, 0, 1);
			for(int p = 0; p < m->nPoly; p++){
				Vec3 tv = {0, 0, 0};
				for(int j = 0; j < 3; j++) AddVec3(m->Vertex[m->Poly[p].Points[j]], tv);
				ScaleVec3(tv, 0.33333333f);
				glVertex3fv(tv);
				AddVec3(m->Poly[p].Normal, tv);
				glVertex3fv(tv);
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}

		if(flags & MESH_DISABLE_CULL) glEnable(GL_CULL_FACE);	//Turn it back on.
		//
		glPopMatrix();
		return;
	}
	return;
}

#define NUMRANDFLOAT 256
static float randfloat[NUMRANDFLOAT * 2];
static int randfloatmade = 0;

#define NUMROTTABLE 32
#define QTRNUMROTTABLE 8
#define NUMROTMASK 31
static float rottable[NUMROTTABLE][2];

void GLPolyRender::GLRenderSpriteObject(SpriteObject *thissprite, PolyRender *PR) {
	if(thissprite->Texture){
		PR->GLBindTexture(thissprite->Texture->id);
		GLfloat c[4];
		if(thissprite->Flags & SPRITE_BLEND_ADD){
		//	BlendModeAdd();
			PR->GLBlendMode(BLEND_ADD);
			c[0] = c[1] = c[2] = c[3] = std::max(thissprite->Opacity, 0.01f);//1.0f;
		}else if(thissprite->Flags & SPRITE_BLEND_SUB){
		//	BlendModeAlpha();
			PR->GLBlendMode(BLEND_ALPHA);
			c[0] = c[1] = c[2] = 0.0f; c[3] = std::max(thissprite->Opacity, 0.01f);
		}else if(thissprite->Flags & SPRITE_BLEND_ALPHA2){
		//	BlendModeAlpha();
			PR->GLBlendMode(BLEND_ALPHA);
			c[0] = c[1] = c[2] = c[3] = std::max(thissprite->Opacity, 0.01f);
		}else{
		//	BlendModeAlpha();
			PR->GLBlendMode(BLEND_ALPHA);
			c[0] = c[1] = c[2] = 1.0f; c[3] = std::max(thissprite->Opacity, 0.01f);//1.0f;
		}
		//
		Vec3 xfpos;
		Vec3MulMat43(thissprite->Pos, PR->view, xfpos);
		//
		int ww = 1;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &ww);
		//
	//	float zero = (1.0f / (float)MAX(1, Texture->Width())) * 1.0f;
		float zero = (1.0f / (float)MAX(1, ww)) * 1.0f;
		float one = 1.0f - zero;
		//
		float wx = rottable[(thissprite->IntRot + QTRNUMROTTABLE) & NUMROTMASK][0] * thissprite->Width;
		float wy = rottable[(thissprite->IntRot + QTRNUMROTTABLE) & NUMROTMASK][1] * thissprite->Width;
		float hx = rottable[(thissprite->IntRot) & NUMROTMASK][0] * thissprite->Height;
		float hy = rottable[(thissprite->IntRot) & NUMROTMASK][1] * thissprite->Height;
		float x1 = (xfpos[0] - wx * 0.5f) - hx * 0.5f;
		float y1 = (xfpos[1] - wy * 0.5f) - hy * 0.5f;
		//
		glBegin(GL_QUADS);
	//	if(Obj->Flags != 0){
//		if(Texture->flags & BFLAG_ROTATED){
//			glColor4fv(c);
//			glTexCoord2f(zero, one);
//			glVertex3f(xfpos[0] - Width * 0.5f, xfpos[1] + Height * 0.5f, -xfpos[2]);
//		//	glVertex3f(x1, y1, z);
//			glTexCoord2f(zero, zero);
//			glVertex3f(xfpos[0] + Width * 0.5f, xfpos[1] + Height * 0.5f, -xfpos[2]);
//		//	glVertex3f(x2, y1, z);
//			glTexCoord2f(one, zero);
//			glVertex3f(xfpos[0] + Width * 0.5f, xfpos[1] - Height * 0.5f, -xfpos[2]);
//		//	glVertex3f(x2, y2, z);
//			glTexCoord2f(one, one);
//			glVertex3f(xfpos[0] - Width * 0.5f, xfpos[1] - Height * 0.5f, -xfpos[2]);
//		//	glVertex3f(x1, y2, z);
//		}else{
			glColor4fv(c);
			glTexCoord2f(zero, zero);
		//	glVertex3f(xfpos[0] - Width * 0.5f, xfpos[1] + Height * 0.5f, -xfpos[2]);
			glVertex3f(x1 + hx, y1 + hy, -xfpos[2]);
			glTexCoord2f(one, zero);
		//	glVertex3f(xfpos[0] + Width * 0.5f, xfpos[1] + Height * 0.5f, -xfpos[2]);
			glVertex3f(x1 + wx + hx, y1 + wy + hy, -xfpos[2]);
			glTexCoord2f(one, one);
		//	glVertex3f(xfpos[0] + Width * 0.5f, xfpos[1] - Height * 0.5f, -xfpos[2]);
			glVertex3f(x1 + wx, y1 + wy, -xfpos[2]);
			glTexCoord2f(zero, one);
		//	glVertex3f(xfpos[0] - Width * 0.5f, xfpos[1] - Height * 0.5f, -xfpos[2]);
			glVertex3f(x1, y1, -xfpos[2]);
//		}
		glEnd();
		//
	//	glPopMatrix();
		//
		return;// true;
	}
	return;// false;
}

void GLPolyRender::GLRenderParticleCloudObject(ParticleCloudObject *thispc, PolyRender *PR) {
	if(PR->ParticleAtten > 0.0f && thispc->Texture){
		PR->GLBindTexture(thispc->Texture->id);
		if(thispc->Flags & SPRITE_BLEND_ADD){
			PR->GLBlendMode(BLEND_ADD);
			glColor4f((float)thispc->Red / 255.0f * thispc->Opacity, (float)thispc->Green / 255.0f * thispc->Opacity, (float)thispc->Blue / 255.0f * thispc->Opacity, std::max(thispc->Opacity, 0.01f));
		}else{
			PR->GLBlendMode(BLEND_ALPHA);
			glColor4f((float)thispc->Red / 255.0f, (float)thispc->Green / 255.0f, (float)thispc->Blue / 255.0f, std::max(thispc->Opacity, 0.01f));
		}
		//
		Vec3 xfpos;
		Vec3MulMat43(thispc->Pos, PR->view, xfpos);
		//
	//	glPointSize(PARTICLE_SIZE / xfpos[2] * PR->Cam.viewplane);
		int r = thispc->Seed & (NUMRANDFLOAT - 1);
	//	glBegin(GL_POINTS);
		glBegin(GL_TRIANGLES);
		float h2 = thispc->Height * 2.0f, w2 = thispc->Width * 2.0f;
	//	float x = xfpos[0] - Width * 0.5f, y = xfpos[1] - Height * 0.5f;
		float z = -xfpos[2], rad = thispc->Radius;
		int numpart = FastFtoL((float)thispc->Particles * PR->ParticleAtten);
		//
		float wx = rottable[(thispc->IntRot + QTRNUMROTTABLE) & NUMROTMASK][0] * w2;
		float wy = rottable[(thispc->IntRot + QTRNUMROTTABLE) & NUMROTMASK][1] * w2;
		float hx = rottable[(thispc->IntRot) & NUMROTMASK][0] * h2;
		float hy = rottable[(thispc->IntRot) & NUMROTMASK][1] * h2;
		float x = (xfpos[0] - wx * 0.25f) - hx * 0.25f;
		float y = (xfpos[1] - wy * 0.25f) - hy * 0.25f;
		//
		for(int i = 0; i < numpart; i += 2){
			{	//Draw two at once with different particle texture halves.
				float xx = x + randfloat[r] * rad, yy = y + randfloat[r + 1] * rad, zz = z + randfloat[r + 2] * rad;
				glTexCoord2f(0, 1);
				glVertex3f(xx, yy, zz);
				glTexCoord2f(0, 0);
				glVertex3f(xx + hx, yy + hy, zz);	//	glVertex3f(xx, yy + h2, zz);
				glTexCoord2f(1, 1);
				glVertex3f(xx + wx, yy + wy, zz);	//	glVertex3f(xx + w2, yy, zz);
				r = (r + 3) & (NUMRANDFLOAT - 1);
			}
			{
				float xx = x + randfloat[r] * rad, yy = y + randfloat[r + 1] * rad, zz = z + randfloat[r + 2] * rad;
				glTexCoord2f(1, 0);
				glVertex3f(xx, yy, zz);
				glTexCoord2f(0, 0);
				glVertex3f(xx + hx, yy + hy, zz);
				glTexCoord2f(1, 1);
				glVertex3f(xx + wx, yy + wy, zz);
				r = (r + 3) & (NUMRANDFLOAT - 1);
			}
		//	glVertex3f(x + randfloat[r] * rad, y + randfloat[r + 1] * rad, z + randfloat[r + 2] * rad);
		}
		glEnd();
	//	glEnable(GL_TEXTURE_2D);
		return;// true;
	}
	return;// false;
}

void GLPolyRender::GLRenderStringObject(StringObject *thisstring, PolyRender *PR) {
	if(thisstring->Texture){
		//
		PR->GLBindTexture(thisstring->Texture->id);
		//
		GLfloat c[4];
		float x = thisstring->X, y = thisstring->Y, w = thisstring->W, h = thisstring->H;
		//
		float gw = 1.0f / (float)thisstring->CharsX, gh = 1.0f / (float)thisstring->CharsY;
		//
		float iterscale = 1.0f / std::max(1.0f, (float)thisstring->AddIters);
		//
		for(int iter = 0; iter < 1 + std::max(1, thisstring->AddIters); iter++){
			float gsx, gsy;
			x = thisstring->X;
			float t = iterscale * (float)iter;
			if(iter == 0){	//Alpha blend pass.
				if(thisstring->Opacity <= 0.0f) continue;
				PR->GLBlendMode(BLEND_ALPHA);
				c[0] = thisstring->Red; c[1] = thisstring->Green; c[2] = thisstring->Blue;
				c[3] = std::max(thisstring->Opacity, 0.01f);
				gsx = ((w * thisstring->GlyphScale) - w) * 0.5f;
				gsy = ((h * thisstring->GlyphScale) - h) * 0.5f;
			}else{	//Additive blend pass.
				if(thisstring->AddRed <= 0.01f && thisstring->AddGreen <= 0.01f && thisstring->AddBlue <= 0.01f) continue;
				PR->GLBlendMode(BLEND_ADD);
				c[0] = thisstring->AddRed * t; c[1] = thisstring->AddGreen * t; c[2] = thisstring->AddBlue * t;
				c[3] = std::max(thisstring->Opacity * t, 0.01f);
				gsx = ((w * thisstring->AddGlyphScale) - w) * 0.5f * t;
				gsy = ((h * thisstring->AddGlyphScale) - h) * 0.5f * t;
			}
			for (int i=0;i<4;i++) {
				if (c[i] < 0.0f)	c[i] = 0.0f;
				if (c[i] > 1.0f)	c[i] = 1.0f;
			}

			glBegin(GL_QUADS);
			glColor4fv(c);
			for(int n = 0; n < thisstring->nGlyph; n++){
				unsigned char g = thisstring->Glyph[n];
				float gx = (float)(g & (thisstring->CharsX - 1)) * gw;
				float gy = (float)(g / thisstring->CharsX) * gh;
				glTexCoord2f(gx, gy);
				glVertex2f(x - gsx, y - gsy);
				glTexCoord2f(gx + gw, gy);
				glVertex2f(x + w + gsx, y - gsy);
				glTexCoord2f(gx + gw, gy + gh);
				glVertex2f(x + w + gsx, y + h + gsy);
				glTexCoord2f(gx, gy + gh);
				glVertex2f(x - gsx, y + h + gsy);
				x += w;
			}
			glEnd();
		}
	}
}

void GLPolyRender::GLRenderLineMapObject(LineMapObject *thislinemap, PolyRender *PR){
//	OutputDebugString("*** Rendering Ortho\n");
	PR->GLBlendMode(BLEND_ALPHA);
//	PR->GLBindTexture(0);
	glDisable(GL_TEXTURE_2D);
	float mat[2][2] = { {sin(-thislinemap->Rot + PI * 0.5f), cos(-thislinemap->Rot + PI * 0.5f)}, {sin(-thislinemap->Rot), cos(-thislinemap->Rot)} };
//	OutputDebugString("*** nLines is " + String(nLines) + "\n");
	float yscale = (float)PR->Cam.viewwidth / (float)PR->Cam.viewheight;	//Must remove distortion, screen is 0.0 to 1.0, which would be fine for square, but not for normal 4/3.
	float yoff = (yscale - 1.0f) * -0.5f;
	glBegin(GL_QUADS);
	for(int l = 0; l < thislinemap->nLines; l++){
		float X1 = thislinemap->Lines[l].x1 - thislinemap->CenterX, Y1 = thislinemap->Lines[l].y1 - thislinemap->CenterY;
		float X2 = thislinemap->Lines[l].x2 - thislinemap->CenterX, Y2 = thislinemap->Lines[l].y2 - thislinemap->CenterY;
		//
	//	float cx = (X1 + X2) * 0.5f, cy = (Y1 + Y2) * 0.5f;
		float clipped1 = sqrtf(X1 * X1 + Y1 * Y1) / thislinemap->ClipRad;
		clipped1 = Bias(thislinemap->ClipBias, 1.0f - std::min(std::max(clipped1, 0.0f), 1.0f));
		float clipped2 = sqrtf(X2 * X2 + Y2 * Y2) / thislinemap->ClipRad;
		clipped2 = Bias(thislinemap->ClipBias, 1.0f - std::min(std::max(clipped2, 0.0f), 1.0f));
		//
		float x1 = (X1 * mat[0][0] + Y1 * mat[1][0]) * thislinemap->Scale + thislinemap->X;
		float x2 = (X2 * mat[0][0] + Y2 * mat[1][0]) * thislinemap->Scale + thislinemap->X;
		float y1 = (Y1 * mat[1][1] + X1 * mat[0][1]) * thislinemap->Scale + thislinemap->Y;
		float y2 = (Y2 * mat[1][1] + X2 * mat[0][1]) * thislinemap->Scale + thislinemap->Y;
		float dy = x2 - x1, dx = -(y2 - y1);	//Rot right 90.
		float s = thislinemap->LineWidth / sqrtf(dx * dx + dy * dy) * 0.5f;
		dx *= s;
		dy *= s;
		glColor4f(thislinemap->Lines[l].r, thislinemap->Lines[l].g, thislinemap->Lines[l].b, thislinemap->Opacity * clipped1);
		glVertex3f(x1 + dx, yoff + (y1 + dy) * yscale, thislinemap->Z);
		glVertex3f(x1 - dx, yoff + (y1 - dy) * yscale, thislinemap->Z);
		glColor4f(thislinemap->Lines[l].r, thislinemap->Lines[l].g, thislinemap->Lines[l].b, thislinemap->Opacity * clipped2);
		glVertex3f(x2 - dx, yoff + (y2 - dy) * yscale, thislinemap->Z);
		glVertex3f(x2 + dx, yoff + (y2 + dy) * yscale, thislinemap->Z);
	}
	glEnd();
	for(int p = 0; p < thislinemap->nPoints; p++){
		float X1 = thislinemap->Points[p].x - thislinemap->CenterX, Y1 = thislinemap->Points[p].y - thislinemap->CenterY;
		//Clipping (fading out).
		float clipped = sqrtf(X1 * X1 + Y1 * Y1) / thislinemap->ClipRad;
		clipped = Bias(thislinemap->ClipBias, 1.0f - std::min(std::max(clipped, 0.0f), 1.0f));
		float edge = std::max(0.0f, clipped - thislinemap->Points[p].ramp);
		//Rotation.
		float x1 = (X1 * mat[0][0] + Y1 * mat[1][0]) * thislinemap->Scale + thislinemap->X;
		float y1 = ((Y1 * mat[1][1] + X1 * mat[0][1]) * thislinemap->Scale + thislinemap->Y) * yscale + yoff;
		float boff = thislinemap->Points[p].size;
		float loff = boff * 0.4141f;	//Octagonal offset (magic number).
		glBegin(GL_TRIANGLE_FAN);
		glColor4f(thislinemap->Points[p].r, thislinemap->Points[p].g, thislinemap->Points[p].b, thislinemap->Opacity * clipped);
		glVertex3f(x1, y1, thislinemap->Z);	//Center point.
		glColor4f(thislinemap->Points[p].r, thislinemap->Points[p].g, thislinemap->Points[p].b, thislinemap->Opacity * edge);
		glVertex3f(x1 - loff, y1 - boff * yscale, thislinemap->Z);	//Upper-left top corner.
		glVertex3f(x1 + loff, y1 - boff * yscale, thislinemap->Z);	//Upper-right top.
		glVertex3f(x1 + boff, y1 - loff * yscale, thislinemap->Z);
		glVertex3f(x1 + boff, y1 + loff * yscale, thislinemap->Z);
		glVertex3f(x1 + loff, y1 + boff * yscale, thislinemap->Z);
		glVertex3f(x1 - loff, y1 + boff * yscale, thislinemap->Z);
		glVertex3f(x1 - boff, y1 + loff * yscale, thislinemap->Z);
		glVertex3f(x1 - boff, y1 - loff * yscale, thislinemap->Z);
		glVertex3f(x1 - loff, y1 - boff * yscale, thislinemap->Z);
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);
	return;
}

void GLPolyRender::GLRenderChamfered2DBoxObject(Chamfered2DBoxObject *thisbox, PolyRender *PR)
{
	if(!thisbox || !PR)
		return;

	PR->GLBlendMode(BLEND_ALPHA);

	GLfloat v[8][2];

	thisbox->Z;
	v[0][0] = thisbox->x;
	v[0][1] = thisbox->y + (thisbox->chamferwidth * 1.333);

	v[1][0] = thisbox->x + thisbox->chamferwidth;
	v[1][1] = thisbox->y;

	v[2][0] = thisbox->x + thisbox->w - thisbox->chamferwidth;
	v[2][1] = thisbox->y;

	v[3][0] = thisbox->x + thisbox->w;
	v[3][1] = thisbox->y + (thisbox->chamferwidth * 1.333);

	v[4][0] = thisbox->x + thisbox->w;
	v[4][1] = thisbox->y + thisbox->h - (thisbox->chamferwidth * 1.333);

	v[5][0] = thisbox->x + thisbox->w - thisbox->chamferwidth;
	v[5][1] = thisbox->y + thisbox->h;

	v[6][0] = thisbox->x + thisbox->chamferwidth;
	v[6][1] = thisbox->y + thisbox->h;

	v[7][0] = thisbox->x;
	v[7][1] = thisbox->y + thisbox->h - (thisbox->chamferwidth * 1.333);

	GLboolean TexEnabled;
	glGetBooleanv(GL_TEXTURE_2D, &TexEnabled);
	if(TexEnabled)
		glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(thisbox->red, thisbox->green, thisbox->blue, thisbox->alpha);
	for(int i = 0 ; i < 8; i++)
		glVertex2fv(v[i]);
	glVertex2fv(v[0]);
	glEnd();

	if(TexEnabled)
		glEnable(GL_TEXTURE_2D);
}

void GLPolyRender::GLRenderTilingTextureObject(TilingTextureObject *thistile, PolyRender *PR) {
	if(thistile->Texture || thistile->UseEnvTexture){//Obj && Obj->Bmp){
		//
		if(thistile->UseEnvTexture == 1 && PR->EnvMap1) PR->GLBindTexture(PR->EnvMap1->id);
		if(thistile->UseEnvTexture == 2 && PR->EnvMap2) PR->GLBindTexture(PR->EnvMap2->id);
		if(thistile->UseEnvTexture == 0 && thistile->Texture) PR->GLBindTexture(thistile->Texture->id);
		//
		if(thistile->Flags & SPRITE_BLEND_ADD) PR->GLBlendMode(BLEND_ADD);
		else PR->GLBlendMode(BLEND_ALPHA);
	//	PR->GLBlendMode(BLEND_ALPHA);
		//
		GLfloat c[4] = {thistile->red, thistile->green, thistile->blue, thistile->alpha};
		//
		for (int i=0;i<4;i++) {
			// FIXME: bit of a bodge to prevent values going out of bounds
			// Some come in at <0.95, 1.05, 1.05, 1.00>
			if (c[i] < 0.0f)	c[i] = 0.0f;
			if (c[i] > 1.0f)	c[i] = 1.0f;
		}
		if(thistile->VertsX > 1 && thistile->VertsY > 1 && thistile->VertsX * thistile->VertsY < MAXTEMPVERTS){
			float ivx = 1.0f / (float)(thistile->VertsX - 1);
			float ivy = 1.0f / (float)(thistile->VertsY - 1);
			int totalv = thistile->VertsX * thistile->VertsY;
			for(int v = 0; v < totalv; v++){	//Make vertex and texture coords.
				SetVec3((float)(v % thistile->VertsX) * ivx, (float)(v / thistile->VertsX) * ivy, 0, TempVertCoords[v]);
				//
				CVec3 tv(TempVertCoords[v].x + thistile->PerturbX, TempVertCoords[v].y + thistile->PerturbY, 0);
				Vec3 pt = {Perturber.Noise(tv.x * thistile->PerturbScale, tv.y * thistile->PerturbScale, 2) - 0.5f,
					Perturber.Noise(tv.x * thistile->PerturbScale + 55.0f, tv.y * thistile->PerturbScale + 55.0f, 2) - 0.5f, 0.0f};
				TempTexCoords[v][0] = TempVertCoords[v][0] + pt[0] * thistile->Perturb;
				TempTexCoords[v][1] = TempVertCoords[v][1] + pt[1] * thistile->Perturb;
			//	ScaleAddVec3(pt, Perturb, m->Vertex[v], TempVertCoords[v]);
			}
			glColor4fv(c);
			for(int y = 0; y < thistile->VertsY - 1; y++){
				glBegin(GL_TRIANGLE_STRIP);
				int v1 = y * thistile->VertsX;
				for(int x = 0; x < thistile->VertsX * 2; x++){
					int v = v1 + (x >>1) + (1 - (x & 1)) * thistile->VertsX;
					glTexCoord2fv(&TempTexCoords[v][0]);
					glVertex2fv(&TempVertCoords[v][0]);
				}
				glEnd();
			}
		}else{
			glBegin(GL_QUADS);
			glColor4fv(c);
			glTexCoord2f(thistile->tu1, thistile->tv1);
			glVertex2f(thistile->x1, thistile->y1);
			glTexCoord2f(thistile->tu2, thistile->tv2);
			glVertex2f(thistile->x2, thistile->y2);
			glTexCoord2f(thistile->tu3, thistile->tv3);
			glVertex2f(thistile->x3, thistile->y3);
			glTexCoord2f(thistile->tu4, thistile->tv4);
			glVertex2f(thistile->x4, thistile->y4);
			glEnd();
		}
		return;// true;
	}
	return;// false;
}

void GLViewplane(float w, float h, float view, float n, float f);

bool GLPolyRender::GLDoRender(){
	//
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLViewplane(Cam.viewwidth, Cam.viewheight, Cam.viewplane, 2.0, Cam.farplane);
	//Watch out...  If the near plane distance is different between here and GLTerrainRender, we're screwed.
	//
	if(!randfloatmade){
		for(int n = 0; n < NUMRANDFLOAT; n++){
			randfloat[n] = ((float)TMrandom() - (float)(RAND_MAX / 2)) / (float)(RAND_MAX / 2);
			randfloat[n + NUMRANDFLOAT] = randfloat[n];
		}
		randfloatmade = 1;
		//
		for(int a = 0; a < NUMROTTABLE; a++){
			float an = ((float)a / (float)(NUMROTTABLE)) * PI2;
			rottable[a][0] = sin(an);
			rottable[a][1] = cos(an);
		}
		//
	}
	//
	//To keep it simple, for now we ASSUME that the OpenGL modelview matrix has been
	//left in the correct state for the current camera position by the previous code,
	//such as GLTerrainRender.  This will need changing to be able to use this as a
	//standalone model viewer.
	//Actually, what it assumes now is that the view matrix in the PolyRender object
	//is correct, vis a vis real pitch vs faked pitch etc, as it'll be thrown directly
	//to OpenGL.
	//
	glFrontFace(GL_CW);
	//
	if(!DisableFog){
		glEnable(GL_FOG);	//Turn on fog for everything, with previously set settings.
	}else{
		glDisable(GL_FOG);
	}
	//
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	if(AlphaTestOn){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
	}else{
		glDisable(GL_ALPHA_TEST);
	}
	glEnable(GL_TEXTURE_2D);
	//
	GLResetStates();
	//
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_REPLACE);//GL_DECAL);//GL_DECAL);
	//
	for(int i = 0; i < nSolidObject; i++){
		SolidObjectP[i]->Render(this);
	}
	//
	return true;
}

bool GLPolyRender::GLDoRenderSecondary(){	//Renders using OpenGL.
	//
	if(nSecondaryObject <= 0) return false;
	//
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLViewplane(Cam.viewwidth, Cam.viewheight, Cam.viewplane, 2.0, Cam.farplane);
	//
	glClear(GL_DEPTH_BUFFER_BIT);	//Clear the Z buffer.
	//
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	if(AlphaTestOn){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
	}else{
		glDisable(GL_ALPHA_TEST);
	}
	glEnable(GL_TEXTURE_2D);
	//
	GLResetStates();
	//
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_REPLACE);//GL_DECAL);//GL_DECAL);
	//
	for(int i = 0; i < nSecondaryObject; i++){
		if(SecondaryObjectP[i]->Transparent()){
			glDepthMask(GL_FALSE);
			SecondaryObjectP[i]->Render(this);
			glDepthMask(GL_TRUE);
		}else{
			SecondaryObjectP[i]->Render(this);
		}
	}
	//
	return true;
}

bool GLPolyRender::GLDoRenderTrans(){
	glFrontFace(GL_CW);
	//
//	if(!DisableFog){
//		glEnable(GL_FOG);	//Turn on fog for everything, with previously set settings.
//	}else{
		glDisable(GL_FOG);
//	}
	//Now disable fog no matter what for transparent objects.
	//
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	//
	if(AlphaTestOn){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
	}else{
		glDisable(GL_ALPHA_TEST);
	}
	//
	GLResetStates();
	//
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//GL_REPLACE);//GL_DECAL);//GL_DECAL);
	//
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glDepthMask(GL_FALSE);
	//
	if(nTransObject > 0){
		qsort(TransObjectP, nTransObject, sizeof(Object3D*), CompareObject3DKeys);
	}
	//
	for(int i = 0; i < nTransObject; i++){
		TransObjectP[i]->Render(this);
	}
	glDepthMask(GL_TRUE);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	return true;
}
bool GLPolyRender::GLDoRenderOrtho(){
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, 1.0f, 1.0f, 0.0f, -100.0f, 100.0f);	//Set up top-down ortho matrix.
//	glOrtho(0.0f, 1.0f, 1.0f, 0.0f, 0.1f, 100.0f);	//Set up top-down ortho matrix.
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
//	glDisable(GL_CULL_FACE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_FOG);
	//
	if(AlphaTestOn){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
	}else{
		glDisable(GL_ALPHA_TEST);
	}
	//
	GLResetStates();
	if(nOrthoObject > 0){	//Orthos are now sorted by Z order (0 is viewer, increasing away).
		qsort(OrthoObjectP, nOrthoObject, sizeof(ObjectOrtho*), CompareObject2DKeys);
	}
	//
	IdentityMat43(view);
	//
	for(int i = 0; i < nOrthoObject; i++){
		OrthoObjectP[i]->Render(this);
	}
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	return true;
}


//Downloads loaded images to OpenGL driver as textures.
//Indexed Images are only used to paint terrain, so aren't downloaded.
//Download sliced up terrain textures in Terrain class.
bool ResourceManager::DownloadTextures(bool UpdateOnly){
	if(DisableLoading) return true;	//Just skip if we have resource loading disabled.
	int oldTexels = Texels;	//Only debug log texel count if changed.
	Bitmap tbmp;
	GLenum format;
	ARGB rgba[256];
	if(!UpdateOnly) OutputDebugLog("Downloading Textures.\n");
	ImageNode *node = ImageNodeHead.NextLink();
	while(node){	//
	//	if(node->id == 0){	//Generate new id.
	//	}
//		node->ScaleToPow2();	//OpenGL only likes pow2 textures.
		int m;
		for(m = 0; m < node->Images(); m++){	//Now handle all images in set.
			//
			bool UpdateTex = false;
			//
			if((*node)[m].flags & BFLAG_DONTDOWNLOAD){
				continue;	//Bitmap does not need to be downloaded.
			}
			//
			if((*node)[m].id){
				if(UpdateOnly){
					if((*node)[m].flags & BFLAG_WANTDOWNLOAD){
						UpdateTex = true;
						(*node)[m].flags &= (~BFLAG_WANTDOWNLOAD);
					}else{
						continue;	//No forcing, so break out of this for loop and continue while loop.
					}
				}else{	//Want redownload, so clean out.
					OutputDebugLog("Calling DeleteTextures " + String((*node)[m].id) + ".\n");
					GLDeleteTextures(1, (GLuint*)&(*node)[m].id);	//Delete old object before rebinding.
				}
			}
			if(UpdateTex == false){
				GLGenTextures(1, (GLuint*)&(*node)[m].id);
				OutputDebugLog("Calling GenTextures " + String((*node)[m].id) + ".\n");
			}
			//
			//Reload if image has been flushed.
			if((*node)[m].Data() == 0 && m == 0){
				LoadImage(node);	//Try re-loading from disk if image has been flushed.
			}
			if((*node)[m].Data() == 0){
				continue;	//Big error, skip out.
			}
			//
			if(tbmp.Init((*node)[m].Width(), (*node)[m].Height(), 32)){
				RGBAfromPE(rgba, (*node)[m].pe, 255);
				if(node->Trans){
				//	rgba[0].argb = 0;	//Give 0 alpha to black/trans pixels.
					for(int i = 0; i < 256; i++)
						if((*node)[m].pe[i].peRed == 0 &&
						(*node)[m].pe[i].peGreen == 0 &&
						(*node)[m].pe[i].peBlue == 0) rgba[i].argb = 0;
					//
					format = GL_RGB5_A1;
					if(node->AlphaGrad){	//Gradiate alpha from 0 to 255.  This will probably break if image has been remapped to a global palette!  e.g. switching from software without reload.
						int i, avmax = 0, av;
						for(i = 0; i < 256; i++){
							av = ((((int)(*node)[m].pe[i].peRed) <<1) + (((int)(*node)[m].pe[i].peGreen) <<2) +
								(((int)(*node)[m].pe[i].peBlue))) / (7);
							if(av > avmax) avmax = av;
						}
						for(i = 0; i < 256; i++){
							av = ((((((int)(*node)[m].pe[i].peRed) <<1) + (((int)(*node)[m].pe[i].peGreen) <<2) +
								((int)(*node)[m].pe[i].peBlue)) / (7)) * 255) / avmax;
							if(node->AlphaGrad == ALPHAGRAD_ALPHAONLY2){
								rgba[i].argb = (0xffffff) | (av <<24);
							}else if(node->AlphaGrad == ALPHAGRAD_ALPHAONLY1){
								for(int c = 0; c < 3; c++) rgba[i].byte[c] = (rgba[i].byte[c] >>2) + 191;
								rgba[i].byte[3] = av;
							}else{
							//	rgba[i].argb = (rgba[i].argb & 0xffffff) | (av <<24);
								//
								av = (int)(Bias(node->AlphaGradBias, (float)av / 255.0f) * 255.0f);
								//
								//Limit alpha on low end so inverse scale never clips colors high.
								for(int b = 0; b < 3; b++) if(av < rgba[i].byte[b]) av = rgba[i].byte[b];
								//
								int rav = (256 * 256) / std::max(1, av);
								rgba[i].byte[3] = av;
								for(int b = 0; b < 3; b++){
									int t = (((int)rgba[i].byte[b] * rav) >>8);
									rgba[i].byte[b] = (unsigned char)std::min(255, t);
								}
								//Okay, NOW we're doing it properly, inversly scaling up the color channel
								//by 1 / alpha so that when we put the image over a color on the screen, we
								//get the same end result as when it's over black (in editing).
							}
						}
						format = GL_RGBA;//4;
					}
				}else{
					if(CompressTex && node->Dynamic == false && TextureCompressionType != 0)
						format = TextureCompressionType;
					else
						format = GL_RGB;
				}
			//	format = 4;
				if((*node)[m].BPP() > 8){	//Handle 24/32bit bitmap in BGR or BGRA format.
					unsigned char *src, *dst, alpha = ((*node)[m].BPP() == 32 ? 1 : 0);
					for(int y = 0; y < tbmp.Height(); y++){
						src = (*node)[m].Data() + y * (*node)[m].Pitch();
						dst = tbmp.Data() + y * tbmp.Pitch();
						for(int x = 0; x < tbmp.Width(); x++){
							dst[0] = src[2];
							dst[1] = src[1];
							dst[2] = src[0];
							if(alpha){
								dst[3] = src[3];
								src += 4;
							}else{
								dst[3] = 255;
								src += 3;
							}
							dst += 4;
						}
					}
				}else{	//Handle 8-bit bitmap.
					(*node)[m].BlitRaw8to32((uint32_t*)tbmp.Data(), tbmp.Pitch(), 0, 0, tbmp.Width(), tbmp.Height(), false, rgba);
					//
					if(node->Trans){	//Special hack for binary transparent images, to smush color into trans pixels to help bilerping.
						for(int y = 0; y < tbmp.Height() - 1; y++){
							unsigned char *src = (unsigned char*)(*node)[m].Data() + (*node)[m].Pitch() * y;
							unsigned char *dst = (unsigned char*)tbmp.Data() + tbmp.Pitch() * y;
							int spitch = (*node)[m].Pitch();
							int dpitch = tbmp.Pitch();
							for(int x = tbmp.Width() - 1; x; x--){	//Skip right and bottom rows!!!
								if(rgba[*src].argb == 0){	//Pixel is trans, suck from down/right.
									if(rgba[*(src + 1)].argb != 0){	//Right.
										dst[0] = dst[4 + 0]; dst[1] = dst[4 + 1]; dst[2] = dst[4 + 2];
									}else if(rgba[*(src + spitch)].argb != 0){	//Down.
										dst[0] = dst[dpitch]; dst[1] = dst[dpitch + 1]; dst[2] = dst[dpitch + 2];
									}else if(rgba[*(src + spitch + 1)].argb != 0){	//Down-right.
										dst[0] = dst[dpitch + 4]; dst[1] = dst[dpitch + 5]; dst[2] = dst[dpitch + 6];
									}
								}else{	//Pixel is color, smush to down/right.
									if(rgba[*(src + 1)].argb == 0){	//Right.
										dst[4] = dst[0]; dst[5] = dst[1]; dst[6] = dst[2];
									}
									if(rgba[*(src + spitch)].argb == 0){	//Down.
										dst[dpitch] = dst[0]; dst[dpitch + 1] = dst[1]; dst[dpitch + 2] = dst[2];
									}
									if(rgba[*(src + spitch + 1)].argb == 0){	//Down-right.
										dst[dpitch + 4] = dst[0]; dst[dpitch + 5] = dst[1]; dst[dpitch + 6] = dst[2];
									}
								}
								src++;
								dst += 4;
							}
						}
					}
				}
			//	if((*node)[m].BlitRaw8to32((unsigned long*)tbmp.Data(), tbmp.Pitch(), 0, 0, tbmp.Width(), tbmp.Height(), false, rgba)){
//				tbmp.GammaCorrect(1.8f);
				glBindTexture(GL_TEXTURE_2D, (*node)[m].id);
				//
			//	OutputDebugString("Called BindTexture.\n");
				//
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_Best_Clamp);//CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_Best_Clamp);//CLAMP);
			//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
				//
				//
				//TODO:  Use glTexSubImage2D when texture is to be updated in place.
				//
				//
				int MaxRes = MAX(node->ForceMaxTexRes, MaxTexRes >> (abs(node->SizeBias)));
				//MaxTexRes is global, SizeBias pulls it down n levels, ForceMTR brings it up to a minimum.
				//
				if(node->MipMap && UsePalTex == false){// && 0){
					int t = GLMipMap(&tbmp, format, node->Trans, MaxRes,//MaxTexRes >> (abs(node->SizeBias)),
						node->BlackOutX, node->BlackOutY, false, UsePalTex, UpdateTex);
					if(!UpdateTex) Texels += t;
					//Trilinear!!
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//_MIPMAP_NEAREST);
					if(UseTrilinear){
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);//NEAREST);
					}else{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
					}
				//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}else{
				//	glTexImage2D(GL_TEXTURE_2D, 0, format, tbmp.Width(), tbmp.Height(), 0,
				//		GL_RGBA, GL_UNSIGNED_BYTE, tbmp.Data());
					int t = GLMipMap(&tbmp, format, node->Trans, MaxRes,//MaxTexRes >> (abs(node->SizeBias)),
						node->BlackOutX, node->BlackOutY, true, UsePalTex, UpdateTex);
					if(!UpdateTex) Texels += t;
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				//
				//Flush downloaded textures if configured to do so.
				if(TextureFlush && node->Flushable && node->Dynamic == false && UpdateTex == false){
					int id = (*node)[m].id, flags = (*node)[m].flags;
					(*node)[m].Free();
					(*node)[m].id = id;
					(*node)[m].flags = flags;	//Preserve GenTexed ID and flags over free.
					OutputDebugLog("Flushing RAM copy of \"" + node->name + "\", #" + String(m) + ".\n");
				}
				//
			}
		//	}
		}
		node = node->NextLink();
	}
	if(oldTexels != Texels){
		OutputDebugLog("Resource Managed Texels on card: " + String(Texels) + ".\n");
	}
	return true;
}

int GLMipMap(Bitmap *bmp, int fmt, int trans, int maxres, int blackoutx, int blackouty, bool nomipmap, bool usepaltex, bool update){
	int texels = 0;
	if(bmp && bmp->Data() && bmp->Width() > 0 && bmp->Height() > 0){
		int w = bmp->Width(), h = bmp->Height();
		Bitmap tbmp1, tbmp2;
		int level = 0;
		tbmp1 = *bmp;
		while(true){
			//DL.
			if(tbmp1.Width() <= maxres && tbmp1.Height() <= maxres){
				//Skip download and level increment but still filter down if above max res.
				if(tbmp1.Data() && tbmp1.Width() > 0 && tbmp1.Height() > 0){
					if(nomipmap && usepaltex){	//Do 8-bit textures.
						Image tbmp8;
						tbmp1.flags |= BFLAG_BYTEORDER_RGBA;
						OutputDebugLog("Quantizing texture to 8 bits...\n\n");
						if(tbmp1.Quantize32to8(&tbmp8, 0, 256, 3)){
							//Determine palette alpha.
							int alphaaccum[256], alphacount[256];
							memset(alphaaccum, 0, sizeof(alphaaccum));
							memset(alphacount, 0, sizeof(alphacount));
							for(int y = 0; y < tbmp1.Height(); y++){
								unsigned char *p1 = (unsigned char*)tbmp1.Data() + y * tbmp1.Pitch();
								unsigned char *p2 = (unsigned char*)tbmp8.Data() + y * tbmp8.Pitch();
								for(int x = tbmp1.Width(); x; x--){
									alphaaccum[*p2] += p1[3];
									alphacount[*p2] += 1;
									p1 += 4;
									p2 += 1;
								}
							}
							unsigned char pal[256][4];
							for(int i = 0; i < 256; i++){
								pal[i][0] = tbmp8.pe[i].peRed;
								pal[i][1] = tbmp8.pe[i].peGreen;
								pal[i][2] = tbmp8.pe[i].peBlue;
								pal[i][3] = (unsigned char)(alphaaccum[i] / std::max(1, alphacount[i]));
							}
							glTexImage2D(GL_TEXTURE_2D, level, GL_R3_G3_B2, tbmp1.Width(), tbmp1.Height(), 0,
								GL_RGBA, GL_UNSIGNED_BYTE, tbmp1.Data());
							int ii = 0;
							if(ii = glGetError())
								OutputDebugLog("Error uploading texture: " + String(ii) + ".\n");

							texels += tbmp1.Width() * tbmp1.Height();
							return texels;
						}
					}else{	//Normal.
						if(update){	//Use TexSubImage to update existing texture object only.
							glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, tbmp1.Width(), tbmp1.Height(),
								GL_RGBA, GL_UNSIGNED_BYTE, tbmp1.Data());
							int ii = 0;
							if(ii = glGetError())
								OutputDebugLog("Error uploading texture: " + String(ii) + ".\n");
						}else{
							glTexImage2D(GL_TEXTURE_2D, level, fmt, tbmp1.Width(), tbmp1.Height(), 0,
								GL_RGBA, GL_UNSIGNED_BYTE, tbmp1.Data());
							int ii = 0;
							if(ii = glGetError())
								OutputDebugLog("Error uploading texture: " + String(ii) + ".\n");
						}
						texels += tbmp1.Width() * tbmp1.Height();
					}
					if(nomipmap) return texels;
				}
				level++;
			}
			if(w <= 1 && h <= 1) break;
			//Mip.
			w = std::max(1, w / 2);
			h = std::max(1, h / 2);
			if(tbmp2.Init(w, h, 32) && tbmp2.Data() && tbmp2.Width() == tbmp1.Width() / 2 && tbmp2.Height() == tbmp1.Height() / 2){
				//
				//Make a Gain table, trying something out...
			//	unsigned char gaintab[256];
			//	for(int i = 0; i < 256; i++){
			//		gaintab[i] = (int)(Gain(0.8f, (float)i / 256.0f) * 255.0f);
			//	}
				//
				int x, y;
				for(y = 0; y < h; y++){
					unsigned char *ts = tbmp1.Data() + (y * 2) * tbmp1.Pitch();
					unsigned char *td = tbmp2.Data() + y * tbmp2.Pitch();
					for(x = w; x; x--){
						//Maximum filter.
					//	int pp = tbmp1.Pitch();
					//	const int p = 0;
					//	int p1 = ((int)ts[p] + (int)ts[p + 1] + (int)ts[p + 2]) * (int)ts[p + 3];
					//	int p2 = ((int)ts[p + 4] + (int)ts[p + 5] + (int)ts[p + 6]) * (int)ts[p + 7];
					//	int p3 = ((int)ts[p + pp] + (int)ts[p + 1 + pp] + (int)ts[p + 2 + pp]) * (int)ts[p + 3 + pp];
					//	int p4 = ((int)ts[p + 4 + pp] + (int)ts[p + 5 + pp] + (int)ts[p + 6 + pp]) * (int)ts[p + 7 + pp];
					//	if(p1 > p2 && p1 > p3 && p1 > p4){
					//		*((int*)&td[p]) = *((int*)&ts[p]);
					//	}else if(p2 > p1 && p2 > p3 && p2 > p4){
					//		*((int*)&td[p]) = *((int*)&ts[p + 4]);
					//	}else if(p3 > p2 && p3 > p1 && p3 > p4){
					//		*((int*)&td[p]) = *((int*)&ts[p + pp]);
					//	}else{
					//		*((int*)&td[p]) = *((int*)&ts[p + pp + 4]);
					//	}
						for(int p = 0; p < 4; p++){
							//Gain filter.
						//	td[p] = gaintab[(unsigned char)((((int)ts[p]) + (int)ts[p + 4] +
						//		(int)ts[p + tbmp1.Pitch()] + (int)ts[p + 4 + tbmp1.Pitch()]) >>2)];
							//Uneven box filter 2.
							td[p] = (((int)ts[p] <<2) + (int)ts[p + 4] +
								(int)ts[p + tbmp1.Pitch()] + ((int)ts[p + 4 + tbmp1.Pitch()] <<1)) >>3;
							//Uneven box filter.
						//	td[p] = (((int)ts[p] * 5) + (int)ts[p + 4] +
						//		(int)ts[p + tbmp1.Pitch()] + (int)ts[p + 4 + tbmp1.Pitch()]) >>3;
							//Even box filter.
						//	td[p] = (((int)ts[p]) + (int)ts[p + 4] +
						//		(int)ts[p + tbmp1.Pitch()] + (int)ts[p + 4 + tbmp1.Pitch()]) >>2;
						}
						ts += 8;
						td += 4;
					}
				}
				//Black out multi-cell edges if need be.
				if(blackoutx > 0 && blackouty > 0){
					int xmask = (1 << HiBit(w / blackoutx)) - 1;
					int ymask = (1 << HiBit(h / blackouty)) - 1;
					for(y = 0; y < h; y++){	//Horizontal lines.
						if((y & ymask) == 0 || (y & ymask) == ymask){
							unsigned char *td = tbmp2.Data() + y * tbmp2.Pitch();
							for(x = 0; x < (w <<2); x++) *td++ = 0;
						}
					}
					for(x = 0; x < w; x++){	//Vertical lines.
						if((x & xmask) == 0 || (x & xmask) == xmask){
							unsigned char *td = tbmp2.Data() + x * 4;
							for(y = 0; y < h; y++){
								for(int p = 0; p < 4; p++) td[p] = 0;
								td += tbmp2.Pitch();
							}
						}
					}
				}
			}
			//Copy.
			tbmp1 = tbmp2;
		}
	}
	return texels;
}

//These are the good pass-through functions.
int GLGenTextures(int n, unsigned int *a){
	glGenTextures(n, a);
	return n;
}
int GLDeleteTextures(int n, unsigned int *a){
	glDeleteTextures(n, a);
	return n;
}

/*
//Manual Gen and Delete textures for possible RagePro workaround.
#define MAX_TEX_NAMES 10000
char TextureNames[MAX_TEX_NAMES];
int TextureNamesInitialized = 0;
int TextureNameSearchFrom = 0;
//
int GLGenTextures(int n, unsigned int *a){
	//
	static foo = 1;
	for(int t = 0; t < n; t++) a[t] = foo++;
	return n;
	//
//	glGenTextures(n, a);
	if(TextureNamesInitialized == 0){
		memset(TextureNames, 0, MAX_TEX_NAMES);
		TextureNamesInitialized = 1;
		TextureNameSearchFrom = 0;
	}
	for(int i = 0; i < n; i++){
		while(TextureNameSearchFrom < MAX_TEX_NAMES && TextureNames[TextureNameSearchFrom] != 0){
			TextureNameSearchFrom++;
		}
		if(TextureNameSearchFrom >= MAX_TEX_NAMES){
			OutputDebugLog("\n***\n***Texture name leak, broke bank!\n***\n\n");
			break;
		}else{
			a[i] = TextureNameSearchFrom + 1;
			TextureNames[TextureNameSearchFrom] = 1;	//Mark it as alloced.
			TextureNameSearchFrom++;
		}
	}
	return n;
}
int GLDeleteTextures(int n, unsigned int *a){
	//
	return n;
	//
//	glDeleteTextures(n, a);
	for(int i = 0; i < n; i++){
		int j = a[i];
		if(j > 0 && j <= MAX_TEX_NAMES){
			TextureNames[j - 1] = 0;	//Mark as free.
			if((j - 1) < TextureNameSearchFrom) TextureNameSearchFrom = j - 1;	//Reset search start to earliest free.
		}else{
			OutputDebugLog("\n***\n***Bad delete textures ID!\n***\n\n");
		}
	}
	return n;
}
*/

