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

//Generic IFF file parser, intended for LWOB loading.

#include <memory>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#ifdef WIN32
#include <winsock.h>

// Stupid, stupid windows.h defines.
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif
#include "IFF.h"

using namespace std;

IFF::IFF(){
	F = NULL;
	OwnFile = false;
	IsOpen = false;
	IsWrite = false;
	Start = 0;
	IFFLength = 0;
	IFFType = 0;
}
IFF::~IFF(){
	Close();
}
int IFF::Even(){
	if(IsOpen && F){
		if(ftell(F) & 1){	//We are at an odd byte pos.
			if(IsWrite){
				WriteByte(0);	//Write out a null byte.
			}else{
				fseek(F, 1, SEEK_CUR);	//Seek ahead 1 byte.
			}
			return 1;
		}
	}
	return 0;
}
int IFF::OpenIn(const char *name, int offset){	//On OpenIn, Start points to first real chunk, on OpenOut, Start points to very start of FORM chunk!
	Close();
	int ret = 0;
	FILE *f = fopen(name, "rb");
	if(f){
		fseek(f, offset, SEEK_SET);
		ret = OpenIn(f);
		OwnFile = true;
	}
	return ret;
}
int IFF::OpenIn(FILE *f){	//On OpenIn, Start points to first real chunk, on OpenOut, Start points to very start of FORM chunk!
//	Close();
	OwnFile = false;
//	if(name && offset >= 0 && (F = fopen(name, "rb"))){
	if(F = f){
		IsOpen = true;
		IsWrite = false;
		Start = ftell(F);	//offset;
		if(ReadLong() == StringToUlong("FORM")){	//FindChunk() likes the file to be properly open, so we'll switch to non-findchunk code for initial opening.
	//	if(FindChunk("FORM")){
	//		IFFLength = ChunkLength;
	//		IFFType = ReadLong();
			IFFLength = ReadLong();
			IFFType = ReadLong();
			Start += 12;	//Start now points to the first real data chunk.
			ChunkPos = 0;
			ChunkLength = 0;
			return 1;
		}
	//	Close();
	}
	return 0;
}
ulong IFF::Close(){
	if(IsOpen){
		if(IsWrite){
			IFFLength = ftell(F) - (Start + 8);
			fseek(F, Start + 4, SEEK_SET);
			WriteLong(IFFLength);
			IsWrite = false;
		}
		if(OwnFile) fclose(F);	//Only close file descriptor if we own it!
		F = NULL;
		IsOpen = false;
	}
	return IFFLength + 8;
}
float IFF::ReadFloat(){
	ulong TempL = ReadLong();
	float TempF = *((float*)(&TempL));
	return TempF;
}
float IFF::ReadFloat(float *pnt){
	if(pnt) return *pnt = ReadFloat();
	return 0.0f;
}
double IFF::ReadFloat(double *pnt){
	if(pnt) return *pnt = (double)ReadFloat();
	return 0.0;
}
ulong IFF::ReadLong(){
	unsigned long Temp;
	if(IsOpen && fread(&Temp, sizeof(Temp), 1, F)){
		return ntohl(Temp);
	}
	return 0;
}
ulong IFF::ReadLong(ulong *pnt){
	if(pnt) return *pnt = ReadLong();
	return 0;
}
int IFF::ReadLong(int *pnt){
	if(pnt) return *pnt = (int)ReadLong();
	return 0;
}
ushort IFF::ReadShort(){
	unsigned short Temp;
	if(IsOpen && fread(&Temp, sizeof(Temp), 1, F)){
		return ntohs(Temp);
	}
	return 0;
}
ushort IFF::ReadShort(ushort *pnt){
	if(pnt) return *pnt = ReadShort();
	return 0;
}
uchar IFF::ReadByte(){
	unsigned char Temp;
	if(IsOpen && fread(&Temp, sizeof(Temp), 1, F)){
		return Temp;
	}
	return 0;
}
uchar IFF::ReadByte(uchar *pnt){
	if(pnt) return *pnt = ReadByte();
	return 0;
}
int IFF::ReadBytes(uchar *p, ulong n){
	if(p && n > 0 && IsOpen && fread(p, n, 1, F)){
		return 1;
	}
	return 0;
}
CStr IFF::ReadString(){
	CStr t;
	ReadString(&t);
	return t;
}
int IFF::ReadString(CStr *str){
	if(IsOpen && str){
		ushort l = 0;
		if(ReadShort(&l)){
			char *p = (char*)malloc((unsigned int)l + 1);
			if(p && (ReadBytes(p, l) || l == 0)){	//Okay, this should work with 0 length strings...
				p[l] = 0;	//Set null.
				str->cpy(p);	//Copy to CStr.
				free(p);
				Even();
				return 1;
			}
			if(p) free(p);	//Cleanup on failure.
		}
	}
	return 0;
}
ulong IFF::FindChunk(const char *c){
	ulong chunk = StringToUlong(c);
	ChunkPos = 0;
	ChunkType = 1;
	if(IsOpen){
		fseek(F, Start, SEEK_SET);
		while(!feof(F) && ChunkType && ChunkPos < Start + IFFLength - 4){	//Start bypasses FORM type, Length includes it.
			Even();
			ChunkPos = ftell(F);
			ChunkType = ReadLong();
			ChunkLength = ReadLong();
			if(ChunkType == chunk){
				return ChunkLength;
			}else{
				fseek(F, ChunkLength, SEEK_CUR);
			}
		}
	}
	ChunkType = 0;
	return 0;
}
ulong IFF::FindChunkNext(const char *c){
	if(ChunkPos == 0) return FindChunk(c);	//If first chunk call, go to find.
	ulong chunk = StringToUlong(c);
	if(IsOpen){
		fseek(F, ChunkPos + 8 + ChunkLength, SEEK_SET);
		while(!feof(F) && ChunkType && ChunkPos < Start + IFFLength - 4){
			Even();
			ChunkPos = ftell(F);
			ChunkType = ReadLong();
			ChunkLength = ReadLong();
			if(ChunkType == chunk){
				return ChunkLength;
			}else{
				fseek(F, ChunkLength, SEEK_CUR);
			}
		}
	}
	ChunkType = 0;
	return 0;
}
ulong IFF::StringToUlong(const char *sc){
	ulong Temp = 0;
	uchar *uc = (uchar*)sc;
	if(sc && strlen(sc) >= 4){
		Temp |= *(uc + 0) << 24;
		Temp |= *(uc + 1) << 16;
		Temp |= *(uc + 2) << 8;
		Temp |= *(uc + 3) << 0;
	}
	return Temp;
}
int IFF::IsType(const char *c){
	if(IFFType == StringToUlong(c)) return 1;
	return 0;
}
int IFF::OpenOut(const char *name, const char *type, int offset){
	OpenOut(name, offset);
	return SetType(type);
}
int IFF::OpenOut(const char *name, int offset){
	Close();
	if(name && offset >= 0 && (F = fopen(name, "wb"))){
		OwnFile = true;
		IsOpen = true;
		IsWrite = true;
		Start = offset;
		fseek(F, Start, SEEK_SET);
		WriteLong(StringToUlong("FORM"));
		WriteLong(0);	//Zero size, fill this in later.
		WriteLong(0);	//No type, fill in type later.  //IFFType = StringToUlong(type));
		return 1;
	}
	Close();
	return 0;
}
int IFF::SetType(const char *type){
	if(type && IsOpen && IsWrite){
		long pos = ftell(F);
		fseek(F, Start + 8, SEEK_SET);
		WriteLong(IFFType = StringToUlong(type));
		fseek(F, pos, SEEK_SET);
		return 1;
	}
	return 0;
}
int IFF::StartChunk(const char *c){
	if(IsOpen && IsWrite && c){
		Even();
		ChunkPos = ftell(F);
		WriteLong(ChunkType = StringToUlong(c));
		WriteLong(0);	//Size, fill in later.
		return 1;
	}
	return 0;
}
int IFF::EndChunk(){	//Writes length of chunk to file.  NEEDED!
	if(IsOpen && IsWrite){
		ulong tpos;
		ChunkLength = (tpos = ftell(F)) - (ChunkPos + 8);
		fseek(F, ChunkPos + 4, SEEK_SET);
		WriteLong(ChunkLength);
		fseek(F, tpos, SEEK_SET);
		return 1;
	}
	return 0;
}
int IFF::WriteFloat(float v){
	ulong TempL = *((ulong*)(&v));
	return WriteLong(TempL);
}
int IFF::WriteLong(ulong v){
	v = htonl(v);
	if(IsOpen && IsWrite){
		if(fwrite(&v, sizeof(v), 1, F)) return 1;
	}
	return 0;
}
int IFF::WriteShort(ushort v){
	v = htons(v);
	if(IsOpen && IsWrite){
		if(fwrite(&v, sizeof(v), 1, F)) return 1;
	}
	return 0;
}
int IFF::WriteByte(uchar v){
	if(IsOpen && IsWrite){
		if(fwrite(&v, sizeof(v), 1, F)) return 1;
	}
	return 0;
}
int IFF::WriteBytes(const uchar *p, ulong n){
	if(IsOpen && IsWrite && p && n > 0){
		if(fwrite(p, n, 1, F)) return 1;
	}
	return 0;
}
int IFF::WriteString(const char *s){	//Writes a string of up to 65k into file: 2 bytes for len, string, Even().
	if(IsOpen && IsWrite && s){
		int l = strlen(s);
		if(WriteShort(std::min((ushort)0xffff, static_cast<unsigned short>(l)))){
			if(WriteBytes(s, std::min((ushort)0xffff, static_cast<unsigned short>(l))) || l == 0){	//Zero length strings should be ok.
				Even();
				return 1;
			}
		}
	}
	return 0;
}
