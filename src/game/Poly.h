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

//Polygon renderer for Voxel engine integration.

#ifndef POLY_H
#define POLY_H

#include "Image.h"
#include "CamRbd.h"
//#include <math.h>
#include <cmath>
#include "pi.h"
#include "Quantizer.h"

#include "Vector.h"

#include "macros.h"

typedef float Float;

#define MAX_LOD 3
//#define MAX_YBUF 10000
//#define MAX_YBUF 16
//#define YBUF_EXTRA 1024
//#define YBUF_EXTRA 16
//#define MAX_YSHEAR 2048
//#define MAX_CHILDREN 16
#define MAX_RENDEROBJ 1024
/*
#define MAX_FARMOBJ 2048
#define MAX_XFORMVERT 1024
#define MAX_XFORMPOLY 1024
#define MAX_TEXTOBJECT 64
*/
/*
#define OBJECT_NONE 0
#define OBJECT_POLY 1
#define OBJECT_SPRITE 2
#define OBJECT_PARTICLE 4
*/
//Flags for AddMesh/AddSprite.
#define SPRITE_BLEND_ALPHA		0x000
#define SPRITE_BLEND_ADD		0x100
#define SPRITE_BLEND_SUB		0x200
#define SPRITE_BLEND_ALPHA2		0x400
//
#define MESH_BLEND_ALPHA		0x000
#define MESH_BLEND_ADD			0x100
//#define MESH_BLEND_SUB		0x200
#define MESH_BLEND_ENVADD		0x200
//Makes the second ENV pass additive.
#define MESH_BLEND_ALPHA2		0x400
#define MESH_BLEND_ENVMAP		0x800
//Blends between primary texture and ENV map.  Opacity is that of ENV map.
//
#define MESH_ENVMAP1_SPHERE		0x1000
//Global envmap 1.
#define MESH_ENVMAP2_SPHERE		0x2000
//Global envmap 2.
#define MESH_ENVMAPT_SPHERE		0x4000
//Use own primary texture as envmap.
//
#define MESH_ENVMAP_FLAT		0x00000
#define MESH_ENVMAP_SMOOTH		0x10000
//
#define MESH_SHADE_FLAT			0x00000
#define MESH_SHADE_SMOOTH		0x20000
#define MESH_SHADE_SOLID		0x40000
#define MESH_SHADE_EDGETRANS	0x80000
//
#define MESH_ENVBASIS_WORLD		0x000000
#define MESH_ENVBASIS_MODEL		0x200000
#define MESH_ENVBASIS_LIGHT		0x100000
//
#define MESH_SCALE_MATRIX		0x10
//Says we're using a scaling matrix.
#define MESH_DISABLE_CULL		0x20
//Disables backface culling.
#define MESH_PERTURB_NOISE		0x40
#define MESH_PERTURB_NOISE_Z	0x80
//Turns on Noise Perturbation.

//MESH_BLEND_ENVMAP will mix envmapping with normal texturing when envmapping is
//enabled.  Opacity will be the amount of normal texture to blend in.  Envmapping
//with another MESH_BLEND mode will create a transparent envmap only.

#define MESH_ENVMAP_MASK (MESH_ENVMAP1_SPHERE | MESH_ENVMAP2_SPHERE | MESH_ENVMAPT_SPHERE)

#define UVFILEVERSION 1

struct PolyUV{
	Float U[3];
	Float V[3];
};

typedef CVec3 Vector;

//Classification for each polygon.  Normal means all 3 points are valid and/or is a strip start, strip means only first point is valid as strip continuation.
#define POLY_NORMAL 0
#define POLY_STRIP 1

class Poly3D{
public:
	int Points[3];	//Pointers to points that make up poly.  Now array indices.
	Vector Normal;	//Polygon face normal vector.
	PolyUV UV;
//	Image *Tex;	//Pointer to RawBMP texture for this poly.
	char Flags;
	char Group;
	char pad1;
	char pad2;
public:
	Poly3D(){ Points[0] = Points[1] = Points[2] = NULL; Flags = Group = 0; return;};//Tex = NULL; return;};
	~Poly3D(){ return;};
	void SetPoints(int p1, int p2, int p3){ Points[0] = p1; Points[1] = p2; Points[2] = p3;};
	void SetUVs(Float u1, Float v1, Float u2, Float v2, Float u3, Float v3){
		UV.U[0] = u1; UV.V[0] = v1;
		UV.U[1] = u2; UV.V[1] = v2;
		UV.U[2] = u3; UV.V[2] = v3;
	};
};

struct Poly2D{
	Vector *Points[3];
	Poly3D *Poly;
	Float SortZ;
	Float Lighting;
};

class Mesh{
public:
	Vector *Vertex;
	Poly3D *Poly;
	Vector *VertNorm;
	int nVertex;
	int nPoly;
	int nVertNorm;
	Vector BndMin, BndMax;
	Vector Size, Offset;	//Records the size of the bounding box, and the accumulated offsets applied to the mesh.
	Float BndRad;
	Mesh *NextLOD;
	float LodBias;
	int LodLevel;
	Image *texture;
public:
	Mesh(){
		Vertex = NULL; Poly = NULL; VertNorm = NULL;
		nVertex = nPoly = nVertNorm = 0;
		NextLOD = NULL;
		texture = NULL;
	};
	~Mesh(){ Free();};
	void Free();
	bool Init(int V, int P);
	bool Center();	//Centers the mesh, setting offset needed to center mesh in Offset.
	bool Shift(Float x, Float y, Float z);	//Offsets mesh points by amounts, ditto Offset.
	bool SetShift(Vec3 shft);	//Sets shift offset to specific amount.
	bool LoadLWO(const char *name, float Scale = 1.0f);
	bool LoadLWO(FILE *f, float Scale = 1.0f);
	bool BasicCube(float meters = 1.0f);	//Creates a basic cube with basic UV mappings (use when a load fails).
	bool UVMapCylindricalZ(Image *bmp);
	bool LoadUV(const char *name);
	bool LoadUV(FILE *f);
	bool SaveUV(const char *name);
	bool MakeBounding();	//Called automatically by LoadLWO.
	bool MakePolyNormals();
	bool MakeVertexNormals();
	bool MakePolyStrips(bool megastrip = false);	//Set this to 1 to ignore different normals when making strips; FOR SMOOTH SHADED OBJECTS ONLY!
	bool SetTexture(Image *img);
	bool SetLod(int level, float bias, Mesh *next);
};
/*
class Object{
friend class PolyRender;
friend class RenderEngine;
private:
	int Type;
	int Flags;
	float Opacity;	//HW-accel for sprites only, 0.0 to 1.0, fade out/in.
	//Lod will now be handled by having links to next-lod-mesh in mesh structures.
	Mesh *M;	//Easy pointer.
	int *YBufPtr;
	int YBufLeft, YBufRight;	//YBuf space is now dynamically allocated off of YBuf heap.
	Image *Sprite;
	Float SpriteWidth, SpriteHeight;
	Object *Link, *Parent;	//Pointer to next linked object, if any.
	unsigned char Red, Green, Blue, Seed;	//Particle cloud.
	float Radius;	//Cloud size.
public:
	Mat43 model;	//model matrix for object.
	Vector xfpos;	//transformed viewspace position.
	Float SortZ;
};
*/
/*
#define MAX_TEXTLEN 1024
class TextObject{
friend class PolyRender;
private:
	Float X, Y, W, H;
	Float Opacity;
//	CStr Text;
	unsigned char Glyph[MAX_TEXTLEN];	//Index into texture, NOT ASCII!
	int nGlyph;
	Image *Bmp;
	int Flags;
	int CharsX, CharsY;	//Character grid in texture bitmap.
	unsigned char Red, Green, Blue;
};
*/

class PolyRender;	//Forward decl.

class Object3D{
public:
	Object3D(){ BndRad = 0.0f; SortKey = 0; };
	~Object3D(){ };
public:
	Vector Pos;
	Float BndRad;
	int SortKey;
public:
	virtual void Render(PolyRender *PR) = 0;
	virtual bool Transparent() = 0;
};

class SpriteObject : public Object3D{
public:
	Image *Texture;
	int Flags;
	Float Width, Height, Opacity;
	int IntRot;
public:
	void Configure(Image *img, Vec3 p, Float w, Float h, int flags, Float opac = 1.0f){//, int introt = 0){
		CopyVec3(p, Pos);  Texture = img;  Width = w;  Height = h;  Opacity = opac;  Flags = flags;
		BndRad = w * 0.5f;
	//	IntRot = introt;
	};
	SpriteObject() : IntRot(0) { };
	void Render(PolyRender *PR);
	bool Transparent(){ return true; };
};

class MeshObject : public Object3D{
public:
	Image *Texture;
	Mesh *M;
	float LodBias;
	int Flags;
	Mat3 Orient;
	Float Opacity;
	float PerturbX, PerturbZ, PerturbScale, Perturb;
public:
	void SetMat43(Mat43 m){
		for(int i = 0; i < 3; i++) CopyVec3(m[i], Orient[i]);
		CopyVec3(m[3], Pos);
	};
	//This function does no validity checking!  Only pass in good pointers!
	void Configure(Image *img, Mesh *m, Mat3 mat, Vec3 pos, int flags, Float opac, Float rad, Float lod){
	//	SetMat43(mat);
		for(int i = 0; i < 3; i++) CopyVec3(mat[i], Orient[i]);
		CopyVec3(pos, Pos);
		Texture = img;  Flags = flags;  Opacity = opac;
		M = m;  BndRad = rad;  LodBias = lod;
	};
	MeshObject() : Perturb(0), PerturbX(0), PerturbZ(0), PerturbScale(1) { };
	void Render(PolyRender *PR);
	bool Transparent(){
		if(!(Flags & MESH_BLEND_ADD) && (Opacity >= 1.0f || (Flags & MESH_BLEND_ENVMAP))) return false;
		else return true;
	};

};

class ParticleCloudObject : public Object3D{
public:
	Image *Texture;
	int Flags, Particles;
	Float Width, Height, Radius, Opacity;	//Width and height are of the particles, Radius is of the cloud.
	unsigned char Red, Green, Blue, Seed;
	int IntRot;
public:
	void Configure(unsigned char r, unsigned char g, unsigned char b, unsigned char seed,
		Vec3 pos, float w, float h, float size, int particles, int flags = 0, float opacity = 1.0f, Image *img = NULL){
		Texture = img;  Flags = flags;  Radius = size;  Opacity = opacity;
		Red = r;  Green = g;  Blue = b;  Seed = seed;  Particles = particles;
		Width = w;  Height = h;  CopyVec3(pos, Pos);
	};
	ParticleCloudObject() : IntRot(0) { };
	void Render(PolyRender *PR);
	bool Transparent(){ return true; };
};

class ObjectOrtho{
public:
	ObjectOrtho() : Z(0.0f) { };
	~ObjectOrtho(){ };
public:
	float Z;	//Sort order.
public:
	virtual void Render(PolyRender *PR) = 0;
};

#define MAX_TEXTLEN 256
class StringObject : public ObjectOrtho{
public:
	Image *Texture;
	int Flags;
	Float X, Y, W, H;
	unsigned char Glyph[MAX_TEXTLEN];	//Index into texture, NOT ASCII!
	int nGlyph;
	int CharsX, CharsY;	//Character grid in texture bitmap.
//	unsigned char Red, Green, Blue, Dummy;
	Float Red, Green, Blue, Opacity;
	Float AddRed, AddGreen, AddBlue, GlyphScale, AddGlyphScale;
	int AddIters;
public:
	StringObject() : AddRed(0), AddGreen(0), AddBlue(0), GlyphScale(1.0f), AddGlyphScale(1.0f), AddIters(1) { };
	void Configure(Image *img, int charsx, int charsy, Float x, Float y, Float w, Float h,
		unsigned const char *glyphs, int count, float opac = 1.0f,
		float r = 1.0f, float g = 1.0f, float b = 1.0f){
		//
		Texture = img;  CharsX = charsx;  CharsY = charsy;  X = x;  Y = y;  W = w;  H = h;
		memcpy(Glyph, glyphs, std::min(count, MAX_TEXTLEN));  nGlyph = count;
		Opacity = opac;  Red = r;  Green = g;  Blue = b;
	};
	void Render(PolyRender *PR);
};

#define MAX_LINEMAP 128
class LineMapObject : public ObjectOrtho{
	friend class GLPolyRender;
private:
	struct Line{ float x1, y1, x2, y2, r, g, b; };
	struct Point{ float x, y, size, r, g, b, ramp; };
	Line Lines[MAX_LINEMAP];
	Point Points[MAX_LINEMAP];
	int nLines, nPoints;
public:
	void ResetLines(){ nLines = 0; };
	void ResetPoints(){ nPoints = 0; };
	bool AddLine(float X1, float Y1, float X2, float Y2, float R, float G, float B);
	bool AddPoint(float X, float Y, float Size, float R, float G, float B, float Ramp);
public:
	Float X, Y, CenterX, CenterY, ClipRad, ClipBias, Rot, Opacity, Scale, LineWidth;
public:
	LineMapObject(){ };
	~LineMapObject(){ };
	void Render(PolyRender *PR);
};

class Chamfered2DBoxObject : public ObjectOrtho {
public:
	float x, y, w, h, chamferwidth;
	float red, green, blue, alpha;
public:
	Chamfered2DBoxObject(){ x = y = w = h = chamferwidth = red = green = blue = alpha = 0.0f; }
	void Configure(float _x, float _y, float _w, float _h, float _chamferwidth, float r, float g, float b, float a){
		x = _x; y = _y; w = _w; h = _h; chamferwidth = _chamferwidth; red = r; green = g; blue = b; alpha = a;}
	void Render(PolyRender *PR);
};

class TilingTextureObject : public ObjectOrtho {
public:
	float tu1, tv1, tu2, tv2, tu3, tv3, tu4, tv4;
	float x1, y1, x2, y2, x3, y3, x4, y4;
	float red, green, blue, alpha;
	Image *Texture;
	int UseEnvTexture;
	int Flags;
	int VertsX, VertsY;
	float PerturbX, PerturbY, PerturbScale, Perturb;
public:
	void Configure(Image *tex, int useenv, int flags, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4,
		float r, float g, float b, float a){
		Texture = tex;
		UseEnvTexture = useenv;  Flags = flags;
		tu1 = u1; tv1 = v1; tu2 = u2; tv2 = v2; tu3 = u3; tv3 = v3; tu4 = u4; tv4 = v4;
		red = r;  green = g;  blue = b;  alpha = a;
	};
	void ConfigurePoly(float X1, float Y1, float X2, float Y2, float X3, float Y3, float X4, float Y4){
		x1 = X1; y1 = Y1; x2 = X2; y2 = Y2; x3 = X3; y3 = Y3; x4 = X4; y4 = Y4;
	};
	TilingTextureObject(){
		x1 = y1 = y2 = x4 = 0;
		x2 = y3 = x3 = y4 = 1;
		VertsX = VertsY = 0;
		PerturbX = PerturbY = PerturbScale = Perturb = 0.0f;
	};
	~TilingTextureObject(){ };
	void Render(PolyRender *PR);
};

class OrthoMeshObject : public ObjectOrtho, public MeshObject {
public:
	void Render(PolyRender *PR){
		MeshObject::Render(PR);
	};
};

int CDECL CompareObject3DKeys(const void *obj1, const void *obj2);

int CDECL CompareObject2DKeys(const void *obj1, const void *obj2);

class PolyRender{
friend class RenderEngine;
friend class VoxelWorld;
	friend class GLPolyRender;
public:
	//
	Vec3 lplane, rplane, zplane;	//Plane equations for clipping against left, right, and z distance planes.  Set up in InitRender.
	//
	Camera Cam;
	Mat43 view;	//This is going to be accessed from VoxelWorld.
	//
	int LODLimit;
//	int DisableParticles;
	float ParticleAtten;
	bool DisableFog;
	float LightX, LightY, LightZ;	//Global light source vector.  +Z forwards, coord pointing at light.
	float Ambient;
	bool ShowPolyNormals;
	bool AlphaTestOn;
	//
	Image *EnvMap1, *EnvMap2;
	//
private:
//	RendBufDesc Rbd;
//	MixTable *Mix;
//	Float CenterX, CenterY, Viewplane;
//	Vector UVVectR;
//	Vector UVVectL;
//	Vector UVVectU;
//	Vector UVVectD;
//	int YShear[MAX_YSHEAR];
//	Object *ObjectP[MAX_RENDEROBJ];
	//Now we have three lists for objects to be drawn, Solid, Trans, and Ortho.
	Object3D *SolidObjectP[MAX_RENDEROBJ];
	int nSolidObject;
	Object3D *TransObjectP[MAX_RENDEROBJ];
	int nTransObject;
	ObjectOrtho *OrthoObjectP[MAX_RENDEROBJ];
	int nOrthoObject;
	//
	Object3D *SecondaryObjectP[MAX_RENDEROBJ];
	int nSecondaryObject;
	//
//	int nObject;
//	Vector TVertex[MAX_XFORMVERT];	//2D screen coords.
//	int nTVertex;
//	Poly2D TPoly[MAX_XFORMPOLY];
//	Poly2D *TPolyP[MAX_XFORMPOLY];
//	int nTPoly;
//	int YBuf[MAX_YBUF + YBUF_EXTRA * 2];
//	int YBufUsed;
//	Object ObjectFarm[MAX_FARMOBJ];
//	int ObjectFarmUsed;
////	Float CamBsin, CamBcos;	//Sin and Cos for transforming object centers from world to cam coords.
//	TextObject Text[MAX_TEXTOBJECT];
//	int nTextObject;
public:
	PolyRender();
	~PolyRender();
	bool InitRender();//, RendBufDesc *Rd, MixTable *mix);	//OpenGL friendly, use (cam, NULL, NULL).
	//
	bool SetupCamera(Camera *Cm);
	//
	void SetLightVector(float lx, float ly, float lz, float amb = 0.25f);
//	int AddObject(Object *Obj);
	//
	//Placeholders to not break code...
	bool AddMesh(Mesh *mesh, Vec3 pos, Rot3 rot, bool link = false, int flags = 0, float opacity = 1.0f){
		return true;
	};
	bool AddMeshMat(Mesh *mesh, Mat43 mat, bool link = false, int flags = 0, float opacity = 1.0f){
		return false;
	};
	bool AddSprite(Image *img, Float w, Float h, Vec3 pos, bool link = false, int flags = 0, float opacity = 1.0f){
		return true;
	};
	bool AddParticle(unsigned char r, unsigned char g, unsigned char b, unsigned char seed,
		Vec3 pos, float size, int flags = 0, float opacity = 1.0f){
		return true;
	};
	bool AddText(Image *img, int charsx, int charsy, Float x, Float y, Float w, Float h,
		unsigned const char *glyphs, int count, float opac = 1.0f,
		unsigned char r = 255, unsigned char g = 255, unsigned char b = 255){
		return true;
	};
	//
	bool AddSolidObject(Object3D *obj);
	bool AddTransObject(Object3D *obj);
	bool AddOrthoObject(ObjectOrtho *obj);
	//
	bool AddSecondaryObject(Object3D *obj);
	//
//	int ClipObjects();	//openGl friendly.
//	void ResetYBuffers();	//Only call this if TerrainRendering is NOT used!
//	int DoRender();	//Renders using software rasterizer.
	//
	virtual bool GLDoRender() = 0;	//Renders using OpenGL.
	virtual bool GLDoRenderTrans() = 0;	//Renders transparents, MUST BE CALLED AFTER ABOVE CALL!
	virtual bool GLDoRenderOrtho() = 0;
	virtual bool GLDoRenderSecondary() = 0;	//Renders using OpenGL.
	//
	virtual void GLRenderMeshObject(MeshObject *thismesh, PolyRender *PR) = 0;
	virtual void GLRenderSpriteObject(SpriteObject *thissprite, PolyRender *PR) = 0;
	virtual void GLRenderParticleCloudObject(ParticleCloudObject *thispc, PolyRender *PR) = 0;
	virtual void GLRenderStringObject(StringObject *thisstring, PolyRender *PR) = 0;
	virtual void GLRenderLineMapObject(LineMapObject *thislinemap, PolyRender *PR) = 0;
	virtual void GLRenderTilingTextureObject(TilingTextureObject *thistile, PolyRender *PR) = 0;
	virtual void GLRenderChamfered2DBoxObject(Chamfered2DBoxObject *thisbox, PolyRender *PR) = 0;
	//
	bool LimitLOD(int level);	//Limits poly model LOD to no higher than this.  0 is max lod.
	void Particles(float percent);	//Attenuates number of Particles.
	void Fog(bool yesno);
	void PolyNormals(bool yesno);	//Debugging, poly normals.
	void AlphaTest(bool yesno);
	//
	bool SetEnvMap1(Image *env1);
	bool SetEnvMap2(Image *env2);
	void UnlinkTextures();	//Unlinks pointers to envmap textures.
public:
	//Functions for objects to call back into.
#define BLEND_UNSET	0x0
#define BLEND_ADD	0x1
#define BLEND_ALPHA	0x2
#define BLEND_NONE	0x3
#define BLEND_MOD	0x4
	virtual void GLResetStates() = 0;
	virtual void GLBlendMode(int mode) = 0;	//Will not re-set already set modes.
	virtual void GLBindTexture(unsigned int name) = 0;	//Will not re-bind already bound textures.
	//
private:
	int blendmode;
	unsigned int boundtex;
	bool texunbound;
//	Object *ParentPtr;
//	int *AllocYBuf(int entries);
//	Object *AddFarmObj(int type, bool link);
//	bool XFormToTemp(Object *Obj);
//	void RenderPolyList(int *ybufp, int flags);
//	void RenderSprite(Image *img, Vector *xfp, Float w, Float h, int *ybufp, int flags);
//	int GLRenderPoly(Object *Obj);
//	int GLRenderSprite(Object *Obj);
//	int GLRenderParticle(Object *Obj);
//	int GLRenderText(TextObject *Obj);
};

#endif
