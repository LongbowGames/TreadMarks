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

#ifndef GLTERRAIN_H
#define GLTERRAIN_H

#include "../Terrain.h"
#include "../Render.h"


#define TexSize 64
#define LodTreeDepth 10
#define MaxLodDepth 12
#define WaterTessLevel 3

//Divines the index of the first node at a certian level in an implicit tree.
#define LODTREEOFF(level) ((1 << (level)) - 1)

//Holds variance data for a single parent triangle on down.
struct LodTree{
	unsigned char lodtree[1 << LodTreeDepth];
	//
	int waterflag;
	//
	void ClearLodTree(){
		memset(lodtree, 0, sizeof(lodtree));
		waterflag = 0;
	};
	LodTree(){
		ClearLodTree();
	};
	inline int BuildLodTree(Terrain *curmap, int x1, int y1, int x2, int y2, int x3, int y3, int level = 0, int index = 0){
		//Sets the variance parameter for that tri to the maximum of all variances below.
		int variance = 0, t;
		int avgx = (x1 + x2) >>1, avgy = (y1 + y2) >>1;
		if(level + 1 < MaxLodDepth){
			t = BuildLodTree(curmap, x3, y3, x1, y1, avgx, avgy, level + 1, (index <<1) + 0);
			if(t > variance) variance = t;
			t = BuildLodTree(curmap, x2, y2, x3, y3, avgx, avgy, level + 1, (index <<1) + 1);
			if(t > variance) variance = t;
		}
		int tt = curmap->GetHwrap(avgx, avgy);
		t = abs(((curmap->GetHwrap(x1, y1) + curmap->GetHwrap(x2, y2)) >>1) - tt);
		if(tt <= WATERHEIGHTRAW){
			waterflag = 1;	//Detection for below water line now.
			t <<= 1;	//Fake higher variance on water tris to avoid water walkies.
		}
	//	t = abs(((curmap->GetHwrap(x1, y1) + curmap->GetHwrap(x2, y2)) >>1) - curmap->GetHwrap(avgx, avgy));
	//	t = (hdif <<8) / std::max(abs(x1 - x2), abs(y1 - y2));
		//Nonono...  Variance should be Absolute, not Relative!
		if(t > variance) variance = t;
		if(level < LodTreeDepth){	//If not, too deep, just return variance, don't store.
			lodtree[LODTREEOFF(level) + index] = (variance > 255 ? 255 : variance);
		}
		return variance;
	};
	//
};

//Holds an array of LodTree structures, usually to cover the entire physical map.
struct LodTreeMap{
	static LodTree dummy;
	int w, h;
	LodTree *trees;
	LodTreeMap() : trees(0), w(0), h(0) {};
	bool Init(int W, int H){
		if(W > 0 && H > 0){
			Free();
			if(trees = new LodTree[W * H * 2]){
				w = W;
				h = H;
				return true;
			}
		}
		return false;
	};
	void Free(){
		if(trees) delete [] trees;
		trees = 0;
	};
	~LodTreeMap(){ Free(); };
	LodTree *Tree(int x, int y, int n){	//GUARANTEED valid pointer.  Oops, doesn't do wrapping coords yet...
		if(trees && w > 0 && h > 0){// && x >= 0 && y >= 0 && x < w && y < h){
			return &trees[(x & (w - 1)) * 2 + (y & (h - 1)) * w * 2 + (n & 1)];
		}else{
			return &dummy;
		}
	};
};

extern LodTreeMap LodMap;	//Single global variable.


#endif

