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

//Polygon renderer for voxel terrain integration.

/*
ToDo:
	? Load textures from LWO file.
		...Maybe not...
	+ Add cylindrical mapping function, and sub-project to output a bitmap with the unfolded
		polygons outlined on it for easy texture painting.
	- Optimize texture mapper, perhaps span based.
	- Try span rendering on voxels too, use two ybufs and two cbufs, and just swap
		pointers after each line of Z.  Render to new ybuf/cbuf, then fill in vertical lines
		between new ybuf/cbuf and old ybuf/cbuf.  Use tight ASM for both seperate loops.
	- Shadows!  Project models onto ground using temporary shadow buffer, then transparent/
		darken map that onto the terrain using a flat polygon rotated to the terrain's
		contours.
	- Either cylindrical projector or polygon transformation/rendering is flipped through the
		X axis...  Have a closer look with a specially created object later.
*/

//#define ASM_SPRITE

#include "Poly.h"
//#include <math.h>
//#include <stdio.h>
#include "IFF.h"
#include "CfgParse.h"
#include <cmath>
#include <cstdio>

using namespace std;

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
	if(env1){// && env1->id){//Data()){
		EnvMap1 = env1;
		return true;
	}
	return false;
}
bool PolyRender::SetEnvMap2(Image *env2){
	if(env2){// && env2->id){//Data()){
		EnvMap2 = env2;
		return true;
	}
	return false;
}
void PolyRender::UnlinkTextures(){
	EnvMap1 = EnvMap2 = 0;
}
bool PolyRender::LimitLOD(int level){
	if(level >= 0){
		LODLimit = level;
		return true;
	}else{
		return false;
	}
}
void PolyRender::Particles(float atten){//int yesno){
//	DisableParticles = !yesno;
	ParticleAtten = atten;
}
void PolyRender::Fog(bool yesno){
	DisableFog = !yesno;
}
void PolyRender::AlphaTest(bool yesno){
	AlphaTestOn = yesno;
}
void PolyRender::PolyNormals(bool yesno){
	ShowPolyNormals = yesno;
}
void PolyRender::SetLightVector(float lx, float ly, float lz, float amb){
	float l = sqrtf(lx * lx + ly * ly + lz * lz);
	if(l > 0.0f){
		LightX = lx / l;
		LightY = ly / l;
		LightZ = lz / l;
	}
	Ambient = amb;
}

bool PolyRender::InitRender(){
	//
	nSolidObject = 0;
	nTransObject = 0;
	nOrthoObject = 0;
	//
	nSecondaryObject = 0;
	//
	//Force state functions to re-set state on first call.
	GLResetStates();
	//
	return true;
}

bool PolyRender::SetupCamera(Camera *Cm){
	if(Cm) Cam = *Cm;
	//
	float bv[2] = {sin(Cam.b * DEG2RAD), cos(Cam.b * DEG2RAD)};
    float tv[2];
    tv[0] = -Cam.viewwidth / 2.0f;
    tv[1] = Cam.viewplane;
	float s = sqrtf(SQUARE(tv[0]) + SQUARE(tv[1]));
	s = std::max(0.001f, s);
	//
	//Plane equations (signed dist = x * a + y * b + c) for left, right, and z clipping planes.
	//
	lplane[0] = (tv[0] * (-bv[0]) + tv[1] * (bv[1])) / s;	//X coord of X basis vect, X coord of Y basis vect.
	lplane[1] = (tv[0] * (-bv[1]) + tv[1] * (-bv[0])) / s;	//Y coord of X basis vect, Y coord of Y basis vect.
	lplane[2] = -(lplane[0] * Cam.x + lplane[1] * Cam.z);
	//
	rplane[0] = ((-tv[0]) * (bv[0]) + tv[1] * (-bv[1])) / s;	//X coord of X basis vect, X coord of Y basis vect.
	rplane[1] = ((-tv[0]) * (bv[1]) + tv[1] * (bv[0])) / s;	//Y coord of X basis vect, Y coord of Y basis vect.
	rplane[2] = -(rplane[0] * Cam.x + rplane[1] * Cam.z);
	//
	zplane[0] = bv[0];
	zplane[1] = bv[1];
	zplane[2] = -(zplane[0] * Cam.x + zplane[1] * Cam.z);
	//
	//Create View matrix.
	Rot3 rot = {Cam.a * DEG2RAD, Cam.b * DEG2RAD, Cam.c * DEG2RAD};
	Rot3ToMat3(rot, view);
#define SWAP(a, b) (t = (a), (a) = (b), (b) = t)
	float t;
	SWAP(view[0][1], view[1][0]);
	SWAP(view[0][2], view[2][0]);	//Invert matrix after making with non-negative rots.
	SWAP(view[1][2], view[2][1]);	//Hah, it works!
	//
	Vec3 p = {-Cam.x, -Cam.y, -Cam.z};
	Vec3MulMat3(p, view, view[3]);	//To do the translate then rotate the view transform requires, must rotate translation amount by rotation amount.
	//
	return true;
}

bool PolyRender::AddSolidObject(Object3D *obj){
	if(nSolidObject < MAX_RENDEROBJ && obj){
		//
		float l = (obj->Pos[0] * lplane[0] + obj->Pos[2] * lplane[1] + lplane[2]) + obj->BndRad;
		float r = (obj->Pos[0] * rplane[0] + obj->Pos[2] * rplane[1] + rplane[2]) + obj->BndRad;
		float z = (obj->Pos[0] * zplane[0] + obj->Pos[2] * zplane[1] + zplane[2]) - obj->BndRad;
		//
		if(l > 0.0f && r > 0.0f && z < Cam.farplane){
			SolidObjectP[nSolidObject++] = obj;
			return true;
		}
		//
	}
	return false;
}
bool PolyRender::AddSecondaryObject(Object3D *obj){
	if(nSecondaryObject < MAX_RENDEROBJ && obj){
		//
	//	float l = (obj->Pos[0] * lplane[0] + obj->Pos[2] * lplane[1] + lplane[2]) + obj->BndRad;
	//	float r = (obj->Pos[0] * rplane[0] + obj->Pos[2] * rplane[1] + rplane[2]) + obj->BndRad;
	//	float z = (obj->Pos[0] * zplane[0] + obj->Pos[2] * zplane[1] + zplane[2]) - obj->BndRad;
		//
	//	if(l > 0.0f && r > 0.0f && z < Cam.farplane){
		SecondaryObjectP[nSecondaryObject++] = obj;
		return true;
	//	}
		//
	}
	return false;
}
bool PolyRender::AddTransObject(Object3D *obj){
	if(nTransObject < MAX_RENDEROBJ && obj){
		//
		//Perform coarse culling here.
		float l = (obj->Pos[0] * lplane[0] + obj->Pos[2] * lplane[1] + lplane[2]) + obj->BndRad;
		float r = (obj->Pos[0] * rplane[0] + obj->Pos[2] * rplane[1] + rplane[2]) + obj->BndRad;
		float z = (obj->Pos[0] * zplane[0] + obj->Pos[2] * zplane[1] + zplane[2]) - obj->BndRad;
		//
		if(l > 0.0f && r > 0.0f && z < Cam.farplane){
			obj->SortKey = FastFtoL(z);
			TransObjectP[nTransObject++] = obj;
			return true;
		}
		//
	}
	return false;
}
bool PolyRender::AddOrthoObject(ObjectOrtho *obj){
	if(nOrthoObject < MAX_RENDEROBJ && obj){
		OrthoObjectP[nOrthoObject++] = obj;
		return true;
	}
	return false;
}

//Ok, now ALL objects allocated off the farm will have pointers added to the ObjectP
//list, child/linked objects will be connected back to the most recent top level object
//added, as well as having the most previous farm object (whatever it is) linked to
//it.  So any child can reference back to the parent, and from there the child links
//can be walked to descend all children.  Thus all objects can be clipped and ybuffered
//independantly, yet as soon as any children come up for rendering the whole set will
//be rendered.
/*
inline Object *PolyRender::AddFarmObj(int type, bool link){
	if(ObjectFarmUsed < MAX_FARMOBJ){
		Object *obj = &ObjectFarm[ObjectFarmUsed];
		obj->Type = type;
	//	obj->M = mesh;
		if(link && ObjectFarmUsed > 0){
			ObjectFarm[ObjectFarmUsed - 1].Link = obj;
			obj->Parent = ParentPtr;
			obj->Link = NULL;
		}else{
			obj->Link = obj->Parent = NULL;
			ParentPtr = obj;
		}
		ObjectFarmUsed++;
		if(nObject < MAX_RENDEROBJ){	//Add to pointer list.
			ObjectP[nObject++] = obj;
			return obj;
		}
	}
	return NULL;
}
*/
/*
bool PolyRender::AddMesh(Mesh *mesh, Vec3 pos, Rot3 rot, bool link, int flags, float opacity){
	Object *obj;
	if(mesh && pos && rot && (obj = AddFarmObj(OBJECT_POLY, link))){
		obj->Flags = flags;
		obj->M = mesh;
		obj->Opacity = opacity;
		Rot3ToMat3(rot, obj->model);
		CopyVec3(pos, obj->model[3]);
		return true;
	}
	return false;
}
bool PolyRender::AddMeshMat(Mesh *mesh, Mat43 mat, bool link, int flags, float opacity){
	Object *obj;
	if(mesh && mat && (obj = AddFarmObj(OBJECT_POLY, link))){
		obj->Flags = flags;
		obj->M = mesh;
		obj->Opacity = opacity;
		memcpy(obj->model, mat, sizeof(Mat43));
		return true;
	}
	return false;
}
bool PolyRender::AddSprite(Image *img, Float w, Float h, Vec3 pos, bool link, int flags, float opacity){
	Object *obj;
	if(img && pos && (obj = AddFarmObj(OBJECT_SPRITE, link))){
		obj->Flags = flags;
		obj->Sprite = img;
		obj->SpriteWidth = w;
		obj->SpriteHeight = h;
		obj->Opacity = opacity;
		IdentityMat43(obj->model);
		CopyVec3(pos, obj->model[3]);
		return true;
	}
	return false;
}
bool PolyRender::AddParticle(unsigned char r, unsigned char g, unsigned char b, unsigned char seed,
		Vec3 pos, float size, int flags, float opacity){
	Object *obj;
	if(pos && (obj = AddFarmObj(OBJECT_PARTICLE, false))){
		obj->Flags = flags;
		obj->Radius = size;
		obj->Red = r;
		obj->Green = g;
		obj->Blue = b;
		obj->Seed = seed;
		obj->Opacity = opacity;
		IdentityMat43(obj->model);
		CopyVec3(pos, obj->model[3]);
		return true;
	}
	return false;
}
bool PolyRender::AddText(Image *img, int charsx, int charsy, Float x, Float y, Float w, Float h,
		unsigned const char *glyphs, int count, float opac,
		unsigned char r, unsigned char g, unsigned char b){
	if(img && glyphs && nTextObject < MAX_TEXTOBJECT && count > 0 && count < MAX_TEXTLEN){
		memcpy(Text[nTextObject].Glyph, glyphs, count);
		Text[nTextObject].nGlyph = count;
		Text[nTextObject].Bmp = img;
		Text[nTextObject].W = w;
		Text[nTextObject].H = h;
		Text[nTextObject].x = x;
		Text[nTextObject].y = y;
		Text[nTextObject].Red = r;
		Text[nTextObject].Green = g;
		Text[nTextObject].Blue = b;
		Text[nTextObject].CharsX = charsx;
		Text[nTextObject].CharsY = charsy;
		Text[nTextObject].Opacity = opac;
		nTextObject++;
		return true;
	}
	return false;
}
*/

int CDECL CompareObject3DKeys(const void *obj1, const void *obj2){
	if((*(Object3D**)obj1)->SortKey < (*(Object3D**)obj2)->SortKey) return 1;
	if((*(Object3D**)obj1)->SortKey > (*(Object3D**)obj2)->SortKey) return -1;
	return 0;
}
int CDECL CompareObject2DKeys(const void *obj1, const void *obj2){
	if((*(ObjectOrtho**)obj1)->Z < (*(ObjectOrtho**)obj2)->Z) return 1;
	if((*(ObjectOrtho**)obj1)->Z > (*(ObjectOrtho**)obj2)->Z) return -1;
	return 0;
}
/*
int CDECL CompareObjectsZ(const void *obj1, const void *obj2){
	if((*(Object**)obj1)->SortZ < (*(Object**)obj2)->SortZ) return -1;
	if((*(Object**)obj1)->SortZ > (*(Object**)obj2)->SortZ) return 1;
	return 0;
}
int CDECL ComparePolyZ(const void *poly1, const void *poly2){
	if((*(Poly2D**)poly1)->SortZ < (*(Poly2D**)poly2)->SortZ) return -1;
	if((*(Poly2D**)poly1)->SortZ > (*(Poly2D**)poly2)->SortZ) return 1;
	return 0;
}

int *PolyRender::AllocYBuf(int amt){	//Lop off requested amount of YBuffer from cache farm.
	if(amt > 0 && YBufUsed + amt < MAX_YBUF){
		int *buf = &YBuf[YBufUsed];
		YBufUsed += amt;
		return buf;
	}
	return &YBuf[YBUF_EXTRA];
}
*/

#if 0
int PolyRender::ClipObjects(){
	//Set up view matrix for camera rot/pos.
	//Oops, screwed this, Rot3ToMat3 is fine for object matrices, but not for cameras!
//	Rot3 rot = {-Cam.a * DEG2RAD, -Cam.b * DEG2RAD, -Cam.c * DEG2RAD};
//	Rot3ToMat3(rot, view);
	//
	Rot3 rot = {Cam.a * DEG2RAD, Cam.b * DEG2RAD, Cam.c * DEG2RAD};
	Rot3ToMat3(rot, view);
#define SWAP(a, b) (t = (a), (a) = (b), (b) = t)
	float t;
	SWAP(view[0][1], view[1][0]);
	SWAP(view[0][2], view[2][0]);	//Invert matrix after making with non-negative rots.
	SWAP(view[1][2], view[2][1]);	//Hah, it works!
	//
	Vec3 p = {-Cam.x, -Cam.y, -Cam.z};
	Vec3MulMat3(p, view, view[3]);	//To do the translate then rotate the view transform requires, must rotate translation amount by rotation amount.
	//
	Vector pos;
	Float rad, sortrad;
	int ybl, ybr;
	//
	if(nObject > 0){
		for(int o = 0; o < nObject; o++){
		//	ObjectP[o]->XFormCenterBOnly(CamBsin, CamBcos, Cam.x, Cam.y, Cam.z);
			Vec3MulMat43(ObjectP[o]->model[3], view, pos);
			switch(ObjectP[o]->Type){
			case OBJECT_POLY : rad = sortrad = ObjectP[o]->M->BndRad; break;
			case OBJECT_SPRITE : rad = ObjectP[o]->SpriteWidth * 0.5f; sortrad = 0.0f; break;
			case OBJECT_PARTICLE : rad = ObjectP[o]->Radius; sortrad = 0.0f;
			}
			ObjectP[o]->SortZ = pos.z - sortrad;
			CopyVec3(pos, ObjectP[o]->xfpos);
			//Children aren't frustom clipped; if parent is in ir out, children are same.
			//Dot position with normal (90' left) of right frustum plane,
			//and normal (90' right) of left frustum plane.
			if(	pos.x * (-UVVectR.z) + pos.z * UVVectR.x + rad < 0.0f ||
				pos.x * UVVectL.z + pos.z * (-UVVectL.x) + rad < 0.0f ||
				pos.z > Cam.farplane
				){	//Object is outside viewing frustum on B axis.
//			if(tvl.x + ObjectP[o]->BoundingRadius < 0 || tvr.x - ObjectP[o]->BoundingRadius > 0){
				ObjectP[o] = ObjectP[nObject - 1];	//Copy last object over top of current object to 'delete' it from pointer list.
				nObject--;
				if(o < nObject) o--;	//If we haven't just deleted the last object, dec o so it
				//gets inced back to the current object so the one copied from the end gets processed.
			}else{
				//Since software isn't a serious prospect anymore...
				/*
				//Gee, it was silly to do this BEFORE frustom clipping!  Eeek.
				if(pos.z > 0){	//Avoid div0.  Find min and max screen column that have to be filled with ybuf data by voxel renderer.
					float iz = Viewplane / pos.z;
					ybl = (int)(CenterX + floorf(((pos.x - rad) * iz)));//Viewplane) / pos.z));
					ybr = (int)(CenterX + ceilf(((pos.x + rad) * iz)));//Viewplane) / pos.z));
					ybl = CLAMP(ybl, 0, Rbd.width);
					ybr = CLAMP(ybr, 0, Rbd.width);
					ObjectP[o]->YBufRight = ybr;
					ObjectP[o]->YBufLeft = ybl;
					ObjectP[o]->YBufPtr = AllocYBuf(ybr - ybl);
				}else{
					ObjectP[o]->YBufRight = 0;
					ObjectP[o]->YBufLeft = 0;
					ObjectP[o]->YBufPtr = AllocYBuf(0);
				}
				*/
			}
		}
		//Now sort objects in list by ascending Z.
		if(nObject > 0){
			qsort(ObjectP, nObject, sizeof(Object*), CompareObjectsZ);
		}
		return 1;
	}
	return 0;
}
#endif

/*
bool PolyRender::XFormToTemp(Object *Obj){
	Mesh *m = Obj->M;	//Temporary working mesh pointer.
	if(!m) return false;
	if(m->nVertex + nTVertex > MAX_XFORMVERT || m->nPoly + nTPoly > MAX_XFORMPOLY){
		OutputDebugLog("Temp Vertex or Poly overrun.\n");
		return 0;
	}
	int OriVertex = nTVertex;	//This is where polygon's vertex indices start indexing.
	//
	//Create model*view matrix.
	Mat43 modelview;
	Mat43MulMat43(Obj->model, view, modelview);
	//
	//Setup for lighting.  Rotate global light into object space.
	Vec3 light = {-0.71f, 0.71f, 0.0f};	//Temporary global light source.
	Vec3 tlv;
	Vec3MulMat3(light, view, tlv);	//Light is now in camera space.
	Vec3IMulMat3(tlv, modelview, light);	//Light is now in Object Space.
	//
	Vector tv;
	Float w;
	if(m->nVertex > 0){
		for(int v = 0; v < m->nVertex; v++){
			Vec3MulMat43(m->Vertex[v], modelview, tv);//TVertex[nTVertex]);
			//Project to screen.
			if(tv.z == 0.0f) TVertex[nTVertex].z = 0.01f;
			else TVertex[nTVertex].z = tv.z;
			w = Viewplane / TVertex[nTVertex].z;
			TVertex[nTVertex].x = CenterX + (tv.x * w);//Viewplane) * w;
			TVertex[nTVertex].y = CenterY - (tv.y * w);//Viewplane) * w;
			nTVertex++;
		}
	}
	if(m->nPoly > 0){
		Vector *v[3];
		for(int p = 0; p < m->nPoly; p++){
			TPoly[nTPoly].Poly = &m->Poly[p];
			TPoly[nTPoly].Points[0] = v[0] = &TVertex[OriVertex + m->Poly[p].Points[0]];
			TPoly[nTPoly].Points[1] = v[1] = &TVertex[OriVertex + m->Poly[p].Points[1]];
			TPoly[nTPoly].Points[2] = v[2] = &TVertex[OriVertex + m->Poly[p].Points[2]];
			//Backface removal.
			//Only process forwards facing polygons.
			//Also skip polies with any point at negative Z (behind the viewer).
			//Better to use real 3D clipping and a Near Clip Plane!
			if(	v[0]->Z > 0.0f && v[1]->Z > 0.0f && v[2]->Z > 0.0f &&
				(v[2]->X - v[0]->X) * (v[1]->Y - v[0]->Y) +
				(v[2]->Y - v[0]->Y) * -(v[1]->X - v[0]->X) < 0.0f){
				//Backwards of normal backface check since screen coords are flipped from
				//world coords in Y direction.
				TPoly[nTPoly].SortZ =
					(TPoly[nTPoly].Points[0]->Z +
					TPoly[nTPoly].Points[1]->Z +
					TPoly[nTPoly].Points[3]->Z) * 0.333333f;/// 3.0f;
				//
				//Calculate lighting.
				TPoly[nTPoly].Lighting = (DotVec3(light, m->Poly[p].Normal) + 2.0f) * 0.3333f;
				//
				TPolyP[nTPoly] = &TPoly[nTPoly];
				nTPoly++;
			}
		}
	}
	return false;
}
*/


/*
void PolyRender::ResetYBuffers(){
	for(int i = 0; i < YBufUsed; i++){
		YBuf[i] = Rbd.height;
	}
}
*/


/*
bool PolyRender::DoRender(){
	if(nObject > 0){
		int o, o2, polies;
		Object *OP, *OC;
	//	int *ybuf;
		for(o = nObject - 1; o >= 0; o--){	//Go from furthest to nearest object.
			OP = ObjectP[o];
			nTVertex = nTPoly = 0;	//Reset temp vertex/poly buffer counters.
			//Grrrr...  This sucks.  Ok, for now linked children will be displayed or
			//not displayed based solely on the parent.  Linked children should only
			//be used for really close linking such as tank->turret.
			if(OP->Parent) continue;	//Skip if a child.
		//	ybuf = OP->YBufPtr;
			polies = 0;
			while(OP){
				if(OP->Type == OBJECT_POLY){
					XFormToTemp(OP);	//Transform object and children's points/polies into temp arrays.
					polies++;
				}
				if(OP->Type == OBJECT_SPRITE){
					RenderSprite(OP->Sprite, &OP->xfpos, OP->SpriteWidth, OP->SpriteHeight,
						OP->YBufPtr - OP->YBufLeft, OP->Flags);
				}
				if(OP->Type == OBJECT_PARTICLE){
					//No particle rendering in software yet...
				}
				OP = OP->Link;
			}
			//This is a little weird...  YBufPtrs will now start about 1024 ints in
			//from the start of the farm array, so we can subtract the left start from
			//the pointer so that the lowest level functions will see a ybuf array
			//starting at the left side of the screen, even if the contents is bogus
			//except in the area occupied by the object.  Argh, this hacky stuff sucks!
			if(polies) RenderPolyList(ObjectP[o]->YBufPtr - ObjectP[o]->YBufLeft, ObjectP[o]->Flags);
		}
		return true;
	}
	return false;
}
*/


/*
void PolyRender::RenderSprite(Image *imgset, Vector *xfp, Float w, Float h, int *ybufp, int flags){
	//Ok, we're hooked up for requiring source textures rotated 90 degrees to the left.
	//So the "top" of the sprite is on the left of the bitmap.
	Float ZRecip = Viewplane * (1.0f / xfp->Z);
	int Left = (int)(CenterX + (xfp->X + -w * 0.5f) * ZRecip);
	int Top = (int)(CenterY - (xfp->Y + h * 0.5f) * ZRecip);
	int Right = (int)(CenterX + (xfp->X + w * 0.5f) * ZRecip);
	int Bottom = (int)(CenterY - (xfp->Y + -h * 0.5f) * ZRecip);
	int Width = Right - Left;
	int Height = Bottom - Top;
	if(Width <= 0 || Height <= 0) return;
	//Mip mapping added!
	int mip = 0;
	Bitmap *img = &(*imgset)[mip];
	while(mip < imgset->Bitmaps() - 1 && Width < ((img->Width() >> 1) + (img->Width() >> 2))){
		img = &(*imgset)[++mip];
	}
	if(img->Width() <= 0) return;
//	int XDelta = (img->Width() <<16) / Width;
//	int YDelta = (img->Height() <<16) / Height;
	int XDelta = (img->Height() <<16) / Width;
	int YDelta = (img->Width() <<16) / Height;
	int RevYDelta = (Height <<16) / img->Width();
//	int RevXDelta = (Width <<16) / img->Width();
	int SourceX = 0, SourceY = 0;
	int DestX = 0, DestY = 0, DestH = 0, DestSkip, SourceLine;
//	int SXStart, SXEnd, SYStart, SYEnd;
	int t;
	unsigned char *tdata, *tsrc, *tsrc2, *src = img->Data();
	int pitch = Rbd.pitch, srcpitch = img->Pitch();
	if(Left < 0){
		t = -Left;
		Left += t;
		SourceX += t * XDelta;
		Width -= t;
	}
	if(Left + Width > Rbd.width){
		Width -= (Left + Width) - Rbd.width;
	}
	if(Width > 0){
		Right = Left + Width;
		for(DestX = Left; DestX < Right; DestX++){
			SourceY = 0;
			DestY = Top + YShear[DestX];
			DestH = Height;
			//Note that "SourceX" and "Y" are currently reversed...
			SourceLine = img->Height() - 1 - (SourceX >>16);
			//Skip over empty space in sprite.
		//	if(img->LineLeft){
			DestSkip = (img->LineL(SourceLine) * RevYDelta) >>16;
			DestY += DestSkip;
			SourceY += YDelta * DestSkip;
			DestH -= DestSkip + ((img->LineR(SourceLine) * RevYDelta) >>16);
		//	}
			if(DestY < 0){
				t = -DestY;
				DestY += t;
				SourceY += t * YDelta;
				DestH -= t;
			}
			if(DestY + DestH > *(ybufp + DestX)){
				DestH -= (DestY + DestH) - *(ybufp + DestX);
			}
			tdata = (unsigned char*)Rbd.data + DestX + DestY * Rbd.pitch;
		//	tsrc2 = src + srcpitch * (img->Height() - 1) - srcpitch * (SourceX >>16);
			tsrc2 = src + srcpitch * SourceLine;
			if(DestH > 0){
#ifdef ASM_SPRITE
		__asm{
			mov eDx, SourceY;
			mov eBx, YDelta;
			mov eCx, DestH;
			xor eAx, eAx;
			mov eDI, tdata;
		//	mov eSI, tsrc2;
			looptop:
				mov eSI, eDx;	//1
				add eDx, eBx;	//2
				shr eSI, 16;	//1
				add eSI, tsrc2;	//1
				mov Al, [eSI];	//1
				test Al, Al;	//1
				jz trans;
					mov [eDI], Al;
				trans:
				add eDI, pitch;	//2
				dec eCx;
				jnz looptop;
			//That's it.
		}
#else
			if(flags & SPRITE_BLEND_ADD){	//Saturation Addition Table Blend.
				do{
				//	unsigned char pixel;
				//	if(pixel = ){
					*tdata = Mix->Add(*(tsrc2 + (SourceY >>16)), *tdata);
				//	}
					tdata += pitch;
					SourceY += YDelta;
				}while(--DestH > 0);
			}else if(flags & SPRITE_BLEND_SUB){	//Saturation Subtraction Table Blend.
				do{
				//	unsigned char pixel;
				//	if(pixel = ){
					*tdata = Mix->Sub(*(tsrc2 + (SourceY >>16)), *tdata);
				//	}
					tdata += pitch;
					SourceY += YDelta;
				}while(--DestH > 0);
		//	}else if(flags & SPRITE_BLEND_HALF){
		//		do{
		//			unsigned char pixel;
		//			if(pixel = *(tsrc2 + (SourceY >>16))){
		//				*tdata = Mix->Mix50(pixel, *tdata);
		//			}
		//			tdata += pitch;
		//			SourceY += YDelta;
		//		}while(--DestH > 0);
			}else{	//Normal copy.
				do{
					unsigned char pixel;
					if(pixel = *(tsrc2 + (SourceY >>16))){
						*tdata = pixel;
					}
					tdata += pitch;
					SourceY += YDelta;
				}while(--DestH > 0);
			}
#endif
			}
			SourceX += XDelta;
		}
	}
}
*/

/*
void PolyRender::RenderPolyList(int *ybufp, int flags){
	int poly;
	//Add poly sorting here.
	if(nTPoly > 0){
		qsort(TPolyP, nTPoly, sizeof(Poly2D*), ComparePolyZ);
	}
	//Render polies.
	for(poly = nTPoly - 1; poly >= 0; poly--){	//0; poly < nTPoly; poly++){
		//
		//Calculate lighting lookup table entry for this poly.
		unsigned char *lighttab = &Mix->Light1[
			std::min(std::max((int)(TPolyP[poly]->Lighting * (float)LIGHT_SHADES), 0), LIGHT_SHADES - 1)][0];
		//
		Vector *v[3] = {TPolyP[poly]->Points[0], TPolyP[poly]->Points[1], TPolyP[poly]->Points[2]};
	//	//Skip backwards facing polygons.
	//	if(	(v[2]->X - v[0]->X) * (v[1]->Y - v[0]->Y) +
	//		(v[2]->Y - v[0]->Y) * -(v[1]->X - v[0]->X) >= 0.0f) continue;
	//	//Backwards of normal backface check since screen coords are flipped from
	//	//world coords in Y direction.
		unsigned char *tdata, *TexD = TPolyP[poly]->Poly->Tex->Data();
//		int TexP = TPolyP[poly]->Poly->Tex->Pitch();
		int TexP = HiBit(TPolyP[poly]->Poly->Tex->Pitch());
		PolyUV *uv = &TPolyP[poly]->Poly->UV;
		//
		int i, twidth = TPolyP[poly]->Poly->Tex->Width(), theight = TPolyP[poly]->Poly->Tex->Height();
		PolyUV UV;
		for(i = 0; i < 3; i++){
			UV.U[i] = (float)((int)(uv->U[i] * (float)twidth) & (twidth - 1));
			UV.V[i] = (float)((int)(uv->V[i] * (float)theight) & (theight - 1));
		}
		uv = &UV;
		//
		int Top, Mid, Bot, Left, Right, CurX, XD1, XD2, X, Height, Y, T, XHalf, FixEdge;
		Float BotY, BotYD, BotU, BotUD, BotV, BotVD;
		Float TopY, TopYD, TopU, TopUD, TopV, TopVD;
		Float CurU, CurUD, CurV, CurVD, XRecipB, XRecipT, HRecip;
		int CurUfp, CurVfp, CurUDfp, CurVDfp;
		int Pitch = Rbd.pitch;
		//Switching over to column scanning.
		//Top is now the same as Right used to be, Bot is Left.
		//Start by finding top point and middle point, and bottom point.
		if(v[1]->X < v[0]->X && v[1]->X < v[2]->X){
			Left = 1;  Bot = 0;  Top = 2;
		}else{
			if(v[2]->X < v[0]->X && v[2]->X < v[1]->X){
				Left = 2;  Bot = 1;  Top = 0;
			}else{
				Left = 0;  Bot = 2;  Top = 1;
			}
		}
		if(v[Bot]->X < v[Top]->X){
			Mid = Bot;  Right = Top;
		}else{
			Mid = Top;  Right = Bot;							//<
		}
	//	CurX = (int)floorf(v[Left]->X);
	//	XD1 = (int)floorf(v[Mid]->X) - CurX;
	//	XD2 = (int)floorf(v[Right]->X) - CurX;
		CurX = (int)(v[Left]->X);
		XD1 = (int)(v[Mid]->X) - CurX;
		XD2 = (int)(v[Right]->X) - CurX;
		BotY = TopY = v[Left]->Y;
		BotU = TopU = uv->U[Left];
		BotV = TopV = uv->V[Left];
		if(Mid == Bot){
			XRecipB = 1.0f / (Float)std::max(XD1, 1);
			XRecipT = (XRecipB * (Float)std::max(XD1, 1)) / (Float)std::max(XD2, 1);
			FixEdge = 1;	//Set which edge is to be fixed at half way point.
		}else{
			XRecipT = 1.0f / (Float)std::max(XD1, 1);					//<Different lines.
			XRecipB = (XRecipT * (Float)std::max(XD1, 1)) / (Float)std::max(XD2, 1);	//<
			FixEdge = 2;
		}
		BotYD = (v[Bot]->Y - v[Left]->Y) * XRecipB;
		BotUD = (uv->U[Bot] - uv->U[Left]) * XRecipB;
		BotVD = (uv->V[Bot] - uv->V[Left]) * XRecipB;
		TopYD = (v[Top]->Y - v[Left]->Y) * XRecipT;
		TopUD = (uv->U[Top] - uv->U[Left]) * XRecipT;
		TopVD = (uv->V[Top] - uv->V[Left]) * XRecipT;
		X = XD1;
		for(XHalf = 0; XHalf < 2; XHalf++){
			while(X > 0){
				if(CurX >= 0 && CurX < Rbd.width){
				//	Y = (int)floorf(TopY) + YShear[CurX];
					Y = (int)(TopY) + YShear[CurX];
				//	Height = 1 + (int)floorf(BotY) - (int)floorf(TopY);
					Height = 1 + (int)(BotY) - (int)(TopY);
					HRecip = 1.0f / (Float)Height;//(TopX - BotX + 1.0f);
				//	CurU = BotU;  CurV = BotV;
				//	CurUD = (TopU - BotU) * WRecip;
				//	CurVD = (TopV - BotV) * WRecip;
					CurUfp = (int)(TopU * 65536.0f);
					CurUDfp = (int)(((BotU - TopU) * HRecip) * 65536.0f);
					CurVfp = (int)(TopV * 65536.0f);
					CurVDfp = (int)(((BotV - TopV) * HRecip) * 65536.0f);
					if(Y < 0){
						T = -Y;  Y += T;
					//	CurU += CurUD * (Float)T;
					//	CurV += CurVD * (Float)T;
						CurUfp += CurUDfp * T;
						CurVfp += CurVDfp * T;
						Height -= T;
					}
					if(Y + Height > *(ybufp + CurX)){
						Height -= (Y + Height) - *(ybufp + CurX);
					}
					tdata = (unsigned char*)Rbd.data + CurX + Y * Pitch;
					while(Height > 0){
					//	*tdata = *(TexD + (int)CurU + (int)CurV * TexP);
					//	if(CurX < *(ybufp + X)){
//						*tdata = *(TexD + (CurUfp >>16) + (CurVfp >>16) * TexP);
						//
					//	*tdata = *(TexD + (CurUfp >>16) + ((CurVfp >>16) <<TexP));
						//Now we're doing lighting!
						*tdata = lighttab[*(TexD + (CurUfp >>16) + ((CurVfp >>16) <<TexP))];
						//
					//	}
						Height--;
					//	CurU += CurUD;  CurV += CurVD;
						CurUfp += CurUDfp;  CurVfp += CurVDfp;
						tdata += Pitch;
					}
				}
				BotY += BotYD;  BotU += BotUD;  BotV += BotVD;
				TopY += TopYD;  TopU += TopUD;  TopV += TopVD;
				CurX++;  X--;
			}
			X = XD2 - XD1;
			if(FixEdge == 1){	//Bot edge has 'crook', fix at halfway.
				XRecipB = 1.0f / (Float)std::max((XD2 - XD1), 1);
				BotY = v[Mid]->Y;
				BotU = uv->U[Mid];
				BotV = uv->V[Mid];
				BotYD = (v[Right]->Y - v[Mid]->Y) * XRecipB;
				BotUD = (uv->U[Right] - uv->U[Mid]) * XRecipB;
				BotVD = (uv->V[Right] - uv->V[Mid]) * XRecipB;
			}
			if(FixEdge == 2){	//Top edge needs fixing.
				XRecipT = 1.0f / (Float)std::max((XD2 - XD1), 1);	//<
				TopY = v[Mid]->Y;								//<
				TopU = uv->U[Mid];							//<
				TopV = uv->V[Mid];							//<
				TopYD = (v[Right]->Y - v[Mid]->Y) * XRecipT;	//<
				TopUD = (uv->U[Right] - uv->U[Mid]) * XRecipT;	//<
				TopVD = (uv->V[Right] - uv->V[Mid]) * XRecipT;	//<
			}
			FixEdge = 0;
		}
	}
}
*/

/*
				//Start by finding top point and middle point, and bottom point.
				if(v[1]->Y < v[0]->Y && v[1]->Y < v[2]->Y){
					Top = 1;  Left = 0;  Right = 2;
				}else{
					if(v[2]->Y < v[0]->Y && v[2]->Y < v[1]->Y){
						Top = 2;  Left = 1;  Right = 0;
					}else{
						Top = 0;  Left = 2;  Right = 1;
					}
				}
				if(v[Left]->Y < v[Right]->Y){
					Mid = Left;  Bot = Right;
				}else{
					Mid = Right;  Bot = Left;							//<
				}
				CurY = (int)floorf(v[Top]->Y);
				YD1 = (int)floorf(v[Mid]->Y) - CurY;
				YD2 = (int)floorf(v[Bot]->Y) - CurY;
				LeftX = RightX = v[Top]->X;
				LeftU = RightU = uv->U[Top];
				LeftV = RightV = uv->V[Top];
				if(Mid == Left){
					YRecipL = 1.0f / (Float)std::max(YD1, 1);
					YRecipR = (YRecipL * (Float)std::max(YD1, 1)) / (Float)std::max(YD2, 1);
					FixEdge = 1;	//Set which edge is to be fixed at half way point.
				}else{
					YRecipR = 1.0f / (Float)std::max(YD1, 1);					//<Different lines.
					YRecipL = (YRecipR * (Float)std::max(YD1, 1)) / (Float)std::max(YD2, 1);	//<
					FixEdge = 2;
				}
				LeftXD = (v[Left]->X - v[Top]->X) * YRecipL;
				LeftUD = (uv->U[Left] - uv->U[Top]) * YRecipL;
				LeftVD = (uv->V[Left] - uv->V[Top]) * YRecipL;
				RightXD = (v[Right]->X - v[Top]->X) * YRecipR;
				RightUD = (uv->U[Right] - uv->U[Top]) * YRecipR;
				RightVD = (uv->V[Right] - uv->V[Top]) * YRecipR;
				Y = YD1;
				for(YHalf = 0; YHalf < 2; YHalf++){
					while(Y > 0){
						if(CurY >= 0 && CurY < Rbd.height){
							X = (int)floorf(LeftX);
							Width = 1 + (int)floorf(RightX) - (int)floorf(LeftX);
							WRecip = 1.0f / (Float)Width;//(RightX - LeftX + 1.0f);
						//	CurU = LeftU;  CurV = LeftV;
						//	CurUD = (RightU - LeftU) * WRecip;
						//	CurVD = (RightV - LeftV) * WRecip;
							CurUfp = (int)(LeftU * 65536.0f);
							CurUDfp = (int)(((RightU - LeftU) * WRecip) * 65536.0f);
							CurVfp = (int)(LeftV * 65536.0f);
							CurVDfp = (int)(((RightV - LeftV) * WRecip) * 65536.0f);
							if(X < 0){
								T = -X;  X += T;
							//	CurU += CurUD * (Float)T;
							//	CurV += CurVD * (Float)T;
								CurUfp += CurUDfp * T;
								CurVfp += CurVDfp * T;
								Width -= T;
							}
							if(X + Width > Rbd.width){
								Width -= (X + Width) - Rbd.width;
							}
							tdata = (unsigned char*)Rbd.data + X + CurY * Rbd.pitch;
							while(Width > 0){
							//	*tdata = *(TexD + (int)CurU + (int)CurV * TexP);
								if(CurY < *(ybufp + X)){
									*tdata = *(TexD + (CurUfp >>16) + (CurVfp >>16) * TexP);
								}
								Width--;
							//	CurU += CurUD;  CurV += CurVD;
								CurUfp += CurUDfp;  CurVfp += CurVDfp;
								tdata++;
							}
						}
						LeftX += LeftXD;  LeftU += LeftUD;  LeftV += LeftVD;
						RightX += RightXD;  RightU += RightUD;  RightV += RightVD;
						CurY++;  Y--;
					}
					Y = YD2 - YD1;
					if(FixEdge == 1){	//Left edge has 'crook', fix at halfway.
						YRecipL = 1.0f / (Float)std::max((YD2 - YD1), 1);
						LeftX = v[Mid]->X;
						LeftU = uv->U[Mid];
						LeftV = uv->V[Mid];
						LeftXD = (v[Bot]->X - v[Mid]->X) * YRecipL;
						LeftUD = (uv->U[Bot] - uv->U[Mid]) * YRecipL;
						LeftVD = (uv->V[Bot] - uv->V[Mid]) * YRecipL;
					}
					if(FixEdge == 2){	//Right edge needs fixing.
						YRecipR = 1.0f / (Float)std::max((YD2 - YD1), 1);	//<
						RightX = v[Mid]->X;								//<
						RightU = uv->U[Mid];							//<
						RightV = uv->V[Mid];							//<
						RightXD = (v[Bot]->X - v[Mid]->X) * YRecipR;	//<
						RightUD = (uv->U[Bot] - uv->U[Mid]) * YRecipR;	//<
						RightVD = (uv->V[Bot] - uv->V[Mid]) * YRecipR;	//<
					}
					FixEdge = 0;
				}
				*/



bool LineMapObject::AddLine(float X1, float Y1, float X2, float Y2, float R, float G, float B){
	if(nLines < MAX_LINEMAP){
		Lines[nLines].x1 = X1; Lines[nLines].x2 = X2; Lines[nLines].y1 = Y1; Lines[nLines].y2 = Y2;
		Lines[nLines].r = R; Lines[nLines].g = G; Lines[nLines].b = B;
		nLines++;
		return true;
	}
	return false;
}
bool LineMapObject::AddPoint(float X, float Y, float Size, float R, float G, float B, float Ramp){
	if(nPoints < MAX_LINEMAP){
		Points[nPoints].x = X; Points[nPoints].y = Y; Points[nPoints].size = Size;
		Points[nPoints].r = R; Points[nPoints].g = G; Points[nPoints].b = B; Points[nPoints].ramp = Ramp;
		nPoints++;
		return true;
	}
	return false;
}


void MeshObject::Render(PolyRender *PR){
	PR->GLRenderMeshObject(this, PR);
}
void SpriteObject::Render(PolyRender *PR){
	PR->GLRenderSpriteObject(this, PR);
}
void ParticleCloudObject::Render(PolyRender *PR){
	PR->GLRenderParticleCloudObject(this, PR);
}
void StringObject::Render(PolyRender *PR){
	PR->GLRenderStringObject(this, PR);
}
void LineMapObject::Render(PolyRender *PR){
	PR->GLRenderLineMapObject(this, PR);
}
void TilingTextureObject::Render(PolyRender *PR){
	PR->GLRenderTilingTextureObject(this, PR);
}
void Chamfered2DBoxObject::Render(PolyRender *PR){
	PR->GLRenderChamfered2DBoxObject(this, PR);
}
