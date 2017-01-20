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

#include "Poly.h"
//#include <math.h>
//#include <stdio.h>
#include "IFF.h"
#include "CfgParse.h"
#include <cmath>
#include <cstdio>

using namespace std;

void Mesh::Free(){
	if(Vertex) delete [] Vertex;  Vertex = NULL;  nVertex = 0;
	if(Poly) delete [] Poly;  Poly = NULL;  nPoly = 0;
	if(VertNorm) delete [] VertNorm;  VertNorm = NULL;  nVertNorm = 0;
	NextLOD = NULL;
}
bool Mesh::Init(int V, int P){
	if(V > 0 && P > 0){
		Free();
		Poly = new Poly3D[P];
		Vertex = new Vector[V];
		if(Vertex && Poly){
			nVertex = V;
			nPoly = P;
			return true;
		}
	}
	Free();
	return false;
}

bool Mesh::LoadLWO(const char *name, float Scale){
	bool ret = false;
	FILE *f = fopen(name, "rb");
	if(f){
		ret = LoadLWO(f, Scale);
		fclose(f);
	}
	return ret;
}
bool Mesh::LoadLWO(FILE *f, float Scale){
	IFF iff;
	int v, p, i, npnts;
	if(iff.OpenIn(f)){	//name, 0)){
		if(!iff.IsType("LWOB")){
			OutputDebugLog("IFF file not LightWaveObject!\n");
			return false;
		}
		int NumPols = iff.FindChunk("POLS") / 10;
		int NumPnts = iff.FindChunk("PNTS") / 12;
		if(NumPnts > 0 && NumPols > 0 && Init(NumPnts, NumPols)){
			for(v = 0; v < nVertex; v++){
				Vertex[v].x = iff.ReadFloat() * Scale;
				Vertex[v].y = iff.ReadFloat() * Scale;
				Vertex[v].z = iff.ReadFloat() * Scale;
			}
			iff.FindChunk("POLS");
			for(p = 0; p < nPoly; p++){
				if(npnts = iff.ReadShort() == 3){
					Poly[p].Points[0] = iff.ReadShort();
					Poly[p].Points[1] = iff.ReadShort();
					Poly[p].Points[2] = iff.ReadShort();
					iff.ReadShort();	//Surface type.
				//	OutputDebugString("Three Point Polygon read.\n");
				}else{
					OutputDebugLog("Model contains a poly with less or more than 3 points!\n");
					for(i = 0; i < npnts + 1; i++) iff.ReadShort();
				}
			}
			ClearVec3(Offset);
			MakePolyNormals();
			MakeVertexNormals();
		//	MakePolyStrips();	//NO!  Can NOT change order of polies when simply loading!  Breaks UVs!
			return MakeBounding();
		}
	}
	OutputDebugLog("LoadLWO failed.\n");
	return false;
}
bool Mesh::BasicCube(float meters){
	float hm = meters / 2.0f;
	if(Init(8, 12)){
		for(int v = 0; v < 8; v++){
			Vertex[v].x = -hm + meters * (float)(v & 1);
			Vertex[v].y = hm - meters * (float)(v / 4);
			Vertex[v].z = hm - meters * (float)((v / 2) & 1);
		}
		Poly[0].SetPoints(0, 1, 2);
		Poly[0].SetUVs(0, 0, 1, 0, 0, 1);
		Poly[1].SetPoints(1, 3, 2);
		Poly[1].SetUVs(1, 0, 1, 1, 0, 1);
		Poly[2].SetPoints(2, 3, 6);
		Poly[2].SetUVs(0, 0, 1, 0, 0, 1);
		Poly[3].SetPoints(3, 7, 6);
		Poly[3].SetUVs(1, 0, 1, 1, 0, 1);
		Poly[4].SetPoints(3, 1, 7);
		Poly[4].SetUVs(0, 0, 1, 0, 0, 1);
		Poly[5].SetPoints(1, 5, 7);
		Poly[5].SetUVs(1, 0, 1, 1, 0, 1);
		Poly[6].SetPoints(1, 0, 5);
		Poly[6].SetUVs(0, 0, 1, 0, 0, 1);
		Poly[7].SetPoints(0, 4, 5);
		Poly[7].SetUVs(1, 0, 1, 1, 0, 1);
		Poly[8].SetPoints(0, 2, 4);
		Poly[8].SetUVs(0, 0, 1, 0, 0, 1);
		Poly[9].SetPoints(2, 6, 4);
		Poly[9].SetUVs(1, 0, 1, 1, 0, 1);
		Poly[10].SetPoints(6, 7, 4);
		Poly[10].SetUVs(0, 0, 1, 0, 0, 1);
		Poly[11].SetPoints(7, 5, 4);
		Poly[11].SetUVs(1, 0, 1, 1, 0, 1);
		ClearVec3(Offset);
		MakePolyNormals();
		MakeVertexNormals();
		return MakeBounding();
	}
	return false;
}

bool Mesh::UVMapCylindricalZ(Image *bmp){
	//U is the position up and down the cylinder, V is the angle around the cylinder.
	if(bmp && bmp->Width() > 0 && bmp->Height() > 0 && Vertex && nVertex > 0 && Poly && nPoly > 0){
		int pol, p, np;
		PolyUV *uv;
		Float MinU, MaxU, MinV, MaxV;
		MinU = MaxU = MinV = MaxV = 0.0f;	//UV bounding rectangle.
		for(pol = 0; pol < nPoly; pol++){
			//First translate the polygon's vertex coords into an angle and a height, without
			//any scaling or point repositioning to keep polies whole.
			uv = &Poly[pol].UV;
			for(p = 0; p < 3; p++){
				//U becomes -180 to +180.
				uv->U[p] = (RAD2DEG * atan2f(Vertex[Poly[pol].Points[p]].x, Vertex[Poly[pol].Points[p]].y));
				uv->V[p] = -Vertex[Poly[pol].Points[p]].z;
				//Negate so +Z on object is "up" in texture.
			}
			//Now check to see if one of the polygon's points is on the 'other side' of the
			//cylinder's seam from the other two, and if so flip it to the other 'side'.
			for(p = 0; p < 3; p++){
				np = (p + 1) % 3;	//NextPoint.
				if(fabsf(uv->U[p] - uv->U[np]) > 180.0f){
					if(uv->U[p] < uv->U[np]){	//Current point is lower, so wrap it around.
						uv->U[p] += 360.0f;
					}else{
						uv->U[np] += 360.0f;
					}
				}
			}
			//Now update UV bounding rectangle.
			for(p = 0; p < 3; p++){
				if(uv->U[p] < MinU) MinU = uv->U[p];
				if(uv->U[p] > MaxU) MaxU = uv->U[p];
				if(uv->V[p] < MinV) MinV = uv->V[p];
				if(uv->V[p] > MaxV) MaxV = uv->V[p];
			}
		}
		for(pol = 0; pol < nPoly; pol++){	//Tex coords are now standard 0.0 - 1.0.
			uv = &Poly[pol].UV;
			for(p = 0; p < 3; p++){
				uv->U[p] = (uv->U[p] - MinU) / (MaxU - MinU);
				uv->V[p] = (uv->V[p] - MinV) / (MaxV - MinV);
			}
			//Set polygon's texture to the provided bitmap, as well.
//			Poly[pol].Tex = bmp;
		}
		//Done!
		return true;
	}
	return false;
}
bool Mesh::SetTexture(Image *img){
	if(img){// && Vertex && nVertex > 0 && Poly && nPoly > 0){
//		for(int pol = 0; pol < nPoly; pol++){
//			Poly[pol].Tex = img;
//		}
		texture = img;
		return true;
	}
	return false;
}

bool Mesh::LoadUV(const char *name){
	FILE *f;
	bool ret = false;
	if((f = fopen(name, "rb"))){
		ret = LoadUV(f);
		fclose(f);
	}
	return ret;
}
bool Mesh::LoadUV(FILE *f){
	if(Poly && Vertex && nPoly > 0 && nVertex > 0){
		IFF iff;
		int UVs, i, j, Pnts;
		if(iff.OpenIn(f) && iff.IsType("UVMP")){
			if(iff.FindChunk("UVTX")){
				int Version = iff.ReadLong();
				if(Version > 10){	//Temp hack!  Will die with less than 10 polies in object or real version more than 10.
					UVs = Version;
					Version = 0;
				}else{
					UVs = iff.ReadLong();
				}
				for(i = 0; i < std::min(UVs, nPoly); i++){
					if(Version == 0){
						Poly[i].Group = 0;	//Read...
						Poly[i].Flags = 0;	//Read...
					}else{
						Poly[i].Group = iff.ReadLong();	//Read...
						Poly[i].Flags = iff.ReadLong();	//Read...
					}
					Pnts = iff.ReadLong();
					for(j = 0; j < Pnts; j++){
						if(j < 3){
							Poly[i].UV.U[j] = iff.ReadFloat();
							Poly[i].UV.V[j] = iff.ReadFloat();
						}else{
							iff.ReadFloat();
							iff.ReadFloat();
						}
					}
				}
				return true;
			}
		}
	}
	return false;
}
bool Mesh::SaveUV(const char *name){
	if(Poly && Vertex && nPoly > 0 && nVertex > 0){
		IFF iff;
		int i, j;//, Pnts;UVs,
		if(iff.OpenOut(name, "UVMP")){
			if(iff.StartChunk("UVTX")){
				iff.WriteLong(UVFILEVERSION);
				iff.WriteLong(nPoly);
				for(i = 0; i < nPoly; i++){
					iff.WriteLong(Poly[i].Group);
					iff.WriteLong(Poly[i].Flags);
					iff.WriteLong(3);
					for(j = 0; j < 3; j++){
						iff.WriteFloat(Poly[i].UV.U[j]);
						iff.WriteFloat(Poly[i].UV.V[j]);
					}
				}
				iff.EndChunk();
			}
			iff.Close();
			return true;
		}
	}
	return false;
}
bool Mesh::MakeBounding(){
	int i, v;
	if(nVertex > 0 && Vertex){
		Float B = 0.0f, Temp = 0.0f;
		ClearVec3(BndMax);
		ClearVec3(BndMin);
		for(v = 0; v < nVertex; v++){
			Temp = Vertex[v].x * Vertex[v].x +
				Vertex[v].y * Vertex[v].y +
				Vertex[v].z * Vertex[v].z;
			if(Temp > B) B = Temp;
			if(Vertex[v].x < BndMin.x) BndMin.x = Vertex[v].x;
			if(Vertex[v].x > BndMax.x) BndMax.x = Vertex[v].x;
			if(Vertex[v].y < BndMin.y) BndMin.y = Vertex[v].y;
			if(Vertex[v].y > BndMax.y) BndMax.y = Vertex[v].y;
			if(Vertex[v].z < BndMin.z) BndMin.z = Vertex[v].z;
			if(Vertex[v].z > BndMax.z) BndMax.z = Vertex[v].z;
		}
		BndRad = fabsf(sqrtf(B));
		for(i = 0; i < 3; i++) Size[i] = BndMax[i] - BndMin[i];
		return true;
	}
	return false;
}
bool Mesh::Center(){	//Centers the mesh, setting offset needed to center mesh in Offset.
	return Shift(Size[0] / 2.0f - BndMax[0], Size[1] / 2.0f - BndMax[1], Size[2] / 2.0f - BndMax[2]);
}
bool Mesh::Shift(Float x, Float y, Float z){	//Offsets mesh points by amounts, ditto Offset.
	int v;
	if(nVertex > 0 && Vertex){
		for(v = 0; v < nVertex; v++){
			Vertex[v].x += x;
			Vertex[v].y += y;
			Vertex[v].z += z;
		}
		Offset.x += x;
		Offset.y += y;
		Offset.z += z;
		return MakeBounding();
	}
	return false;
}
bool Mesh::SetShift(Vec3 shft){
	if(shft) return Shift(shft[0] - Offset[0], shft[1] - Offset[1], shft[2] - Offset[2]);
	return false;
}
bool Mesh::MakePolyNormals(){
	Vector tv, tv1, tv2;
	if(nPoly > 0 && Poly && nVertex > 0 && Vertex){
		for(int p = 0; p < nPoly; p++){
			CopyVec3(Vertex[Poly[p].Points[0]], tv);
			CopyVec3(Vertex[Poly[p].Points[1]], tv1);
			SubVec3(tv1, tv, tv1);
			CopyVec3(Vertex[Poly[p].Points[2]], tv2);
			SubVec3(tv2, tv, tv2);
			CrossVec3(tv1, tv2, tv);
			NormVec3(tv, Poly[p].Normal);
		}
		return true;
	}
	return false;
}
bool Mesh::MakeVertexNormals(){	//Must be called AFTER poly normals!
	if(nPoly > 0 && nVertex > 0 && Poly && Vertex){
		//Setup vertex normal array.
		if(VertNorm) delete [] VertNorm;
		nVertNorm = 0;
		VertNorm = new Vector[nVertex];
		if(!VertNorm) return false;
		nVertNorm = nVertex;
		memset(VertNorm, 0, nVertNorm * sizeof(Vector));
		//Calc norms.  Add norms of all polygons using vertex, then normalize.
		for(int p = 0; p < nPoly; p++){
			for(int t = 0; t < 3; t++){
				AddVec3(Poly[p].Normal, VertNorm[Poly[p].Points[t]]);
			}
		}
		for(int v = 0; v < nVertNorm; v++){
			float l = LengthVec3(VertNorm[v]);
			if(l > 0.0f) ScaleVec3(VertNorm[v], 1.0f / l);
		}
		return true;
	}
	return false;
}
inline void RotPtsDown(Poly3D &p){
	int t = p.Points[0];
	p.Points[0] = p.Points[1];
	p.Points[1] = p.Points[2];
	p.Points[2] = t;
	float tu = p.UV.U[0], tv = p.UV.V[0];
	p.UV.U[0] = p.UV.U[1]; p.UV.V[0] = p.UV.V[1];
	p.UV.U[1] = p.UV.U[2]; p.UV.V[1] = p.UV.V[2];
	p.UV.U[2] = tu; p.UV.V[2] = tv;
}
inline void RotPtsUp(Poly3D &p){
	int t = p.Points[2];
	p.Points[2] = p.Points[1];
	p.Points[1] = p.Points[0];
	p.Points[0] = t;
	float tu = p.UV.U[2], tv = p.UV.V[2];
	p.UV.U[2] = p.UV.U[1]; p.UV.V[2] = p.UV.V[1];
	p.UV.U[1] = p.UV.U[0]; p.UV.V[1] = p.UV.V[0];
	p.UV.U[0] = tu; p.UV.V[0] = tv;
}
inline void PolySwap(Poly3D &p1, Poly3D &p2){
	Poly3D tp3 = p1; p1 = p2; p2 = tp3;
}
bool Mesh::MakePolyStrips(bool megastrip){
//	return false;
//#define POLSWAP(a, b) (tp3 = (a), (a) = (b), (b) = tp3)
//	Poly3D tp3;
	int strips = 0, p, pp, sp1, sp2, rot, striplen, sp1p, sp2p;
	if(nPoly > 0 && Poly && nVertex > 0 && Vertex){
		for(p = 0; p < nPoly; p++) Poly[p].Flags = POLY_NORMAL;
	//	strips++;
		for(p = 0; p < nPoly; p++){
			for(rot = 0; rot < 3; rot++){	//Try all three rotations of first-in-strip's points until connecting strip poly is found.
				if(Poly[p].Flags == POLY_NORMAL){
					sp1 = Poly[p].Points[1];	//StripPoint.
					sp2 = Poly[p].Points[2];
					striplen = 1;
				}else{
					rot = 3;	//If we are looking at a strip-continuation, do NOT do point rotation.
					striplen++;
				}
				if(striplen & 1){	//Tells us which of the two points of strip continuation polygon should be the anchor points for the next poly in strip.
					sp1p = 0; sp2p = 1;
				}else{
					sp1p = 2; sp2p = 0;
				}
				for(pp = p + 1; pp < nPoly; pp++){
					if(Poly[pp].Group != Poly[p].Group || (megastrip == false && DotVec3(Poly[pp].Normal, Poly[p].Normal) < 0.99f))
						continue;	//Skip if not in same texture-coord group.  Or if normals materially different, now.
					if(sp1 == Poly[pp].Points[0] && sp2 == Poly[pp].Points[2]){
						RotPtsDown(Poly[pp]);	//Rotate points so point 0 is strip-continuation point.
						Poly[pp].Flags = POLY_STRIP;
						sp1 = Poly[pp].Points[sp1p]; sp2 = Poly[pp].Points[sp2p];	//Set points that next-in-strip needs to share.
						PolySwap(Poly[p + 1], Poly[pp]);	//Swap poly to be after previous-in-strip.
						break;
					}
					if(sp1 == Poly[pp].Points[1] && sp2 == Poly[pp].Points[0]){
						RotPtsUp(Poly[pp]);
						Poly[pp].Flags = POLY_STRIP;
						sp1 = Poly[pp].Points[sp1p]; sp2 = Poly[pp].Points[sp2p];
						PolySwap(Poly[p + 1], Poly[pp]);
						break;
					}
					if(sp1 == Poly[pp].Points[2] && sp2 == Poly[pp].Points[1]){
						Poly[pp].Flags = POLY_STRIP;
						sp1 = Poly[pp].Points[sp1p]; sp2 = Poly[pp].Points[sp2p];
						PolySwap(Poly[p + 1], Poly[pp]);
						break;
					}
				}
				if(pp >= nPoly && rot < 2){	//No strip match found.  Don't rotate on final miss.
					RotPtsDown(Poly[p]);
				}else{	//Strip match found!
					break;	//Break out of loop that's rotating points of
				}
			}
			if(pp >= nPoly) strips++;	//No continuation, meaning next poly will start new strip.  Erroneously misses start poly, but also counts end poly even if continuation.
		}
		OutputDebugLog(String(nPoly) + " polygons converted to " + String(strips) + " strips.\n");
		return true;
	}
	return false;
}
bool Mesh::SetLod(int level, float bias, Mesh *next){
	LodLevel = level;
	LodBias = bias;
	if(next != this) NextLOD = next;
	return true;
}
