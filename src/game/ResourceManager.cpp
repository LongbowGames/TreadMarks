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

//ResourceManager implementation.

#include "ResourceManager.h"

#include "CfgParse.h"
#include "Basis.h"

#include "Crypt.h"

#define INVPAL_BITS 6

ImageNode *ImageNode::Find(const char *n, bool mip, bool trans, AlphaGradType agrad){
	if(CmpLower(name, n) && (!mip) == (!MipMap) && (!trans) == (!Trans) && agrad == AlphaGrad) return this;
	if(NextLink()) return NextLink()->Find(n, mip, trans, agrad);
	return 0;
}
MeshNode *MeshNode::Find(const char *n){
	if(CmpLower(name, n)) return this;
	if(NextLink()) return NextLink()->Find(n);
	return 0;
}
SoundNode *SoundNode::Find(const char *n, bool loop){
	if(CmpLower(name, n) && (!loop) == (!bLoop)) return this;
	if(NextLink()) return NextLink()->Find(n, loop);
	return 0;
}

ResourceManager::ResourceManager(FileManager *fm) : pFM(fm) {
	for(int i = 0; i < MAXTERTEX; i++) Eco[i].Init(0, 0, 0, 0, 0, 0, &Tex[i]);
	nTex = 0;
	InvPal.Init(INVPAL_BITS, INVPAL_BITS, INVPAL_BITS);
	MaxTexRes = 256;
	UseTrilinear = 0;
	UsePalTex = false;
	DisableLoading = false;
	CompressTex = false;
	maxsoundbits = 0;
	maxsoundfreq = 0;
	TextureFlush = false;
	//
	int Texels = 0;
}

void ResourceManager::SetTextureFlush(bool flush){	//Set to 1 to free local copies of disk based textures after download.
	TextureFlush = flush;
}
void ResourceManager::SetMaxSoundBits(int bits){
	maxsoundbits = bits;
}
void ResourceManager::SetMaxSoundFreq(int hertz){
	maxsoundfreq = hertz;
}
int ResourceManager::GetMaxSoundBits(){
	return maxsoundbits;
}
int ResourceManager::GetMaxSoundFreq(){
	return maxsoundfreq;
}
void ResourceManager::InitSound(){
    snd.Init(64);
}
void ResourceManager::FreeSound(){
	snd.StopAll();
	snd.StopMusic();
	//Sleep(100);
	snd.Free();
	for(SoundNode *sn = SoundNodeHead.NextLink(); sn; sn = sn->NextLink()) sn->id = -1;
}
bool ResourceManager::DownloadSounds(bool reload){
	bool ret = false;
//	FILE *fp = fopen("c:\\sounds.txt", "a");
	for(SoundNode *sn = SoundNodeHead.NextLink(); sn; sn = sn->NextLink()){
		if(sn->id < 0){
			if(reload) LoadSound(sn);	//Reload from disk.
			snd.AddBuffer(sn);
			sn->id = snd.GetBufferCount() - 1;
//			if(fp)
//				fprintf(fp, "%d\t%s\n", sn->id, sn->name);
			ret = true;
		}
	}
//	if(fp)
//		fclose(fp);
	return ret;
}

ImageNode *ResourceManager::GetCreateImage(const char *n, int w, int h, int bpp, bool mipmap, bool trans, AlphaGradType agrad){
	//Find bitmap by name.  Create if not already loaded/created.
	ImageNode *node = ImageNodeHead.NextLink();
	if(pFM){
		if(node && (node = node->Find(n, mipmap, trans, agrad))) return node;	//Already loaded.
		if(node = ImageNodeHead.AddObject(new ImageNode(this, n, mipmap, trans, agrad))){
			if(DisableLoading){	//Make tiny placeholder instead.
				node->Init(8, 8, bpp);
			}else{
				node->Init(w, h, bpp);
			}
			return node;
		}
	}
	return 0;
}
ImageNode *ResourceManager::GetImage(const char *n, bool mipmap, bool trans, AlphaGradType agrad){	//Find bitmap by name.  Load if not already loaded.
	ImageNode *node = ImageNodeHead.NextLink();
	if(pFM){
		if(node && (node = node->Find(n, mipmap, trans, agrad))) return node;	//Already loaded.
		pFM->PushFile();
		if(pFM->Open(n) || DisableLoading){	//Found on disk, or loading disabled.
			if(node = ImageNodeHead.AddObject(new ImageNode(this, n, mipmap, trans, agrad))){
				OutputDebugLog(CStr("Loading Image: ") + n + " M:" + String(mipmap) + " T:" + String(trans) + " A:" + String(agrad) + "\n");
			//	if(DisableLoading){	//Make tiny placeholder instead.
			//		node->Init(8, 8, 8);
			//	}else{
			//		if(CmpLower(FileExtension(n), "img")){
			//			node->LoadSet(pFM->GetFile());
			//		}else{
			//			if(trans || agrad){
			//				node->LoadBMP8(pFM->GetFile());
			//			}else{
			//				node->LoadBMP(pFM->GetFile());
			//			}
			//			if(node->BPP() > 8){
			//				node->To32Bit();	//Shimmy 24bit images to 32bit.
			//			}
			//		}
			//	}
				node->Flushable = true;
				LoadImage(node);	//Get node to load itself.
				//
				pFM->PopFile();
				return node;
			}
		}
		pFM->PopFile();
	}
	return 0;
}
int ResourceManager::LoadImage(ImageNode *node){	//Loads or re-loads an image using stored file name.
	int idlist[256], flaglist[256];
	if(pFM && node && node->name.len() > 0){
		pFM->PushFile();
		if(pFM->Open(node->name) || DisableLoading){	//Found on disk, or loading disabled.
			OutputDebugLog(CStr("Loading Image: ") + node->name + "\n");
			if(DisableLoading){	//Make tiny placeholder instead.
				node->Init(8, 8, 8);
			}else{
				int saveids = node->Images();
				for(int i = 0; i < std::min(256, saveids); i++){
					idlist[i] = (*node)[i].id;
					flaglist[i] = (*node)[i].flags;
				}
				if(CmpLower(FileExtension(node->name), "img")){
					node->LoadSet(pFM->GetFile());
				}else{
					if(node->Trans || node->AlphaGrad){
						node->LoadBMP8(pFM->GetFile());
					}else{
						node->LoadBMP(pFM->GetFile());
					}
					if(node->BPP() > 8){
						node->To32Bit();	//Shimmy 24bit images to 32bit.
					}
				}
				for(int j = 0; j < std::min(256, saveids); j++){	//Bug fix, otherwise we'll write garbage into ids and flags!
					(*node)[j].id = idlist[j];
					(*node)[j].flags = flaglist[j];
				}
			}
			if(node->Data() == 0){
				node->Init(8, 8, 8);	//Make tiny black texture if load fails.
				node->pe[0].peRed = 255;
				node->pe[0].peGreen = 0;
				node->pe[0].peBlue = 255;
			}
			pFM->PopFile();
			return 1;
		}
		pFM->PopFile();
	}
	if(node && node->Data() == 0){
		node->Init(8, 8, 8);	//Make tiny black texture if load fails.
		node->pe[0].peRed = 255;
		node->pe[0].peGreen = 0;
		node->pe[0].peBlue = 255;
	}
	return 0;
}
Image *ResourceManager::GetImage(int index){	//Gets an indexed image by number.
	if(index >= 0 && index < MAXTERTEX) return &Tex[index];
	return 0;
}
Mesh *ResourceManager::GetMesh(const char *n, double scale, bool smooth){	//Find mesh by name.  Load if not already loaded.
	MeshNode *node = MeshNodeHead.NextLink();
	if(pFM && n && strlen(n) > 2){
		if(node && (node = node->Find(n))) return node;	//Already loaded.
		pFM->PushFile();
		if(node = MeshNodeHead.AddObject(new MeshNode(this, n))){
			if(pFM->Open(n)){	//Found on disk.  NOTE:  For collisions and such, MESH LOADING CAN NOT BE DISABLED!  Meshes must be present on dedicated server!
				OutputDebugLog(CStr("Loading Mesh: ") + n + "\n");
				node->LoadLWO(pFM->GetFile(), scale);
				node->LoadUV(pFM->Open(FileNoExtension(n) + ".luv"));
				node->MakePolyStrips(smooth);
			//	node->BasicCube();
				pFM->PopFile();
				return node;
			}else{
				OutputDebugLog(CStr("Mesh load failed, using placeholder for: ") + n + "\n");
				node->BasicCube();
				pFM->PopFile();
				return node;
			}
		}
		pFM->PopFile();
	}
	return NULL;
}
SoundNode *ResourceManager::GetSound(const char *n, bool loop, float volumebias){	//Find mesh by name.  Load if not already loaded.
	SoundNode *node = SoundNodeHead.NextLink();
	if(pFM){
		if(node && (node = node->Find(n, loop))) return node;	//Already loaded.
		pFM->PushFile();
		if(pFM->Open(n) || DisableLoading){	//Found on disk.
			if(node = SoundNodeHead.AddObject(new SoundNode(this, n, loop))){
				node->fVolumeBias = volumebias;
			//	OutputDebugLog(CStr("Loading Sound: ") + n + "\n");
			//	if(DisableLoading){
			//		node->Init(22050, 16, 0, 1024);	//Tiny blip of dead air.
			//	}else{
			//		node->LoadWAV(pFM->GetFile());
			//		if(maxsoundbits == 8) node->To8Bit();
			//		if(maxsoundfreq && maxsoundfreq < node->SampleRate()) node->Resample(maxsoundfreq);
			//	}
			//	node->id = -1;
				//
				LoadSound(node);
				//
				pFM->PopFile();
				return node;
			}
		}
		pFM->PopFile();
	}
	return 0;
}
int ResourceManager::GetFileCRC(const char *n, unsigned int *crc, int *size)
{
	int len;
	unsigned int hash = 0;
	unsigned char *mem;
	FILE *fp;

	if(pFM){
		pFM->PushFile();
		if(pFM->Open(n) || DisableLoading){
			// Find CRC

			len = pFM->length();
			mem = (unsigned char *)malloc(len);
			fp = pFM->GetFile();
			if (fp && mem) {
				fread(mem,len,1,fp);
				hash = Checksum(mem,len);
			}
			if (mem) free(mem);


			if (crc) *crc = hash;
			if (size) *size = len;

			return 1;
		}
		pFM->PopFile();
	}
	return 0;
}

int ResourceManager::LoadSound(SoundNode *node){	//Ditto a sound, re-does max bits and max rate clamping.
#ifndef HEADLESS
	if(pFM && node && node->name.len() > 0){
		pFM->PushFile();
		if(pFM->Open(node->name) || DisableLoading){	//Found on disk.
			OutputDebugLog(CStr("Loading Sound: ") + node->name + "\n");
			if(!DisableLoading)
				node->buffer.loadFromFile(pFM->GetFileName().get());
			node->id = -1;
			pFM->PopFile();
			return 1;
		}
		node->id = -1;
		pFM->PopFile();
	}
#endif
	return 0;
}
int ResourceManager::LoadIndexedImage(const char *name, int index){	//Attempts to load texture into slot index.
	if(pFM && name && index >= 0 && index < MAXTERTEX){
		if(strcmp(name, "none") != 0 && strlen(name) > 0){	//Avoid missing file errors.
			pFM->PushFile();
			if(pFM->Open(name) || DisableLoading){	//Found, or loading disabled.
				OutputDebugLog(CStr("Loading Image: ") + name + "\n");
				if(DisableLoading || !Tex[index].LoadBMP8(pFM->GetFile())){
					Tex[index].Init(8, 8, 8);	//Placeholder image if loading disabled or if not a valid bitmap.
				}
				//else{
				//	if(Tex[index].LoadBMP8(pFM->GetFile())){
				//		//
				//	}else{
				//	}
				if(Eco[index].name != name) strcpy(Eco[index].name, name);
				if(index >= nTex) nTex = index + 1;
				pFM->PopFile();
				return 1;
			}else{
				Tex[index].Init(8, 8, 8);
				Tex[index].pe[0].peRed = 255;
				Tex[index].pe[0].peGreen = 0;	//Ugly placeholder if load fails.
				Tex[index].pe[0].peBlue = 255;
				if(Eco[index].name != name) strcpy(Eco[index].name, name);
				if(index >= nTex) nTex = index + 1;
				pFM->PopFile();
				return 0;
			}
			pFM->PopFile();
		}
	}
	return 0;
}
int ResourceManager::LoadIndexedFromEco(){	//Attempts to load textures named in corresponding ecosystems.
	int ret = 0;
	nTex = 0;
	for(int i = 0; i < MAXTERTEX; i++){
		if(LoadIndexedImage(Eco[i].name, i)) ret = 1;
	}
	return ret;	//True if at least one eco-named tex was loaded.
}
int ResourceManager::FreeIndexedImages(){
	for(int i = 0; i < MAXTERTEX; i++) Tex[i].Free();
	nTex = 0;
	return 1;
}
int ResourceManager::FreeIndexedImage(int index){
	if(index >= 0 && index < MAXTERTEX){
		Tex[index].Free();
		strcpy(Eco[index].name, "none");
		if(index == nTex - 1) nTex--;
		return 1;
	}
	return 0;
}
int ResourceManager::RemoveIndexedImage(int index){	//Removes image and reshuffles rest down.
	if(index >= 0 && index < MAXTERTEX){
		for(int i = index; i < (nTex - 1); i++){
			Tex[i] = Tex[i + 1];
			Eco[i] = Eco[i + 1];
		}
		FreeIndexedImage(nTex - 1);
		return 1;
	}
	return 0;
}
/*
int ResourceManager::MakePalette(Quantizer *quant){	//Optionally pass in a Quantizer with some colors already loaded in.
	if(quant == NULL){
		quant = &Quant;
		quant->ClearPalette();
	}
	for(int i = 0; i < nTex; i++){	//Add palettes for terrain textures.
		if(Tex[i].Data()) quant->AddPalette(Tex[i].pe, 256, TERRAINSHADES);
	}
	ImageNode *node = ImageNodeHead.NextLink();
	while(node){	//Add palettes for named images.
		quant->AddPalette(node->pe, 256);
		node = node->NextLink();
	}
	quant->GetCompressedPalette(pe, 256);
	InvPal.Make(pe);
	return 1;
}
bool ResourceManager::MakeMixTable(){	//Computes mix table.
	return Mix.MakeLookup(pe, true);
}
int ResourceManager::RemapIndexedImages(){	//Sets remap tables on indexed images.
	for(int i = 0; i < nTex; i++){	//Set remap tables.
		if(Tex[i].Data()) Tex[i].InitRemapTable(&InvPal);//pe);
	}
	return 1;
}
int ResourceManager::RemapNamedImages(){	//Actually remaps currently loaded named images.
	ImageNode *node = ImageNodeHead.NextLink();
	while(node){	//Remap palettes for named images.
		node->Remap(&InvPal);//pe);
		node->AnalyzeLines();
		node = node->NextLink();
	}
	return 1;
}
int ResourceManager::MipMapNamedImages(){	//Creates mipmaps for named images that need them.
	ImageNode *node = ImageNodeHead.NextLink();
	while(node){	//Remap palettes for named images.
		if(node->MipMap){
		//	node->InitSet(HiBit(node->Width()) - 1 - 1);	//Allocate mipmaps down to 4x4.
			//Mipmaps will now automatically be made down to 1x1.
			node->MakeMipMap(&Mix, MIX75, node->Trans);
			node->AnalyzeLines();
		}
		node = node->NextLink();
	}
	return 1;
}
*/

void ResourceManager::UndownloadTextures(){
	ImageNode *node = ImageNodeHead.NextLink();
	while(node){
		for(int m = 0; m < node->Images(); m++){	//Now handle all images in set.
			if((*node)[m].id){
				//OutputDebugString("Calling DeleteTextures " + String((*node)[m].id) + ".\n");
				GLDeleteTextures(1, (unsigned int*)&(*node)[m].id);	//Delete old object before rebinding.
				(*node)[m].id = 0;
			}
		}
		node = node->NextLink();
	}
	Texels = 0;	//Texels downloaded to card.
}
void ResourceManager::FreeTextures(){
	UndownloadTextures();
	ImageNodeHead.DeleteList();
}
