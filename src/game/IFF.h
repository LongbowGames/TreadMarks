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

//General IFF reader, but intended for LWOB reading.
//Adding writing ability, for general data file stuff.
//TODO: Nested chunks.
//Done:  Multiple chunks of the same type allowed in a file.  Still no nesting.

//Must link with wsock32.lib for conversion macros.  All
//data stored on disk in Network (big endian) format.

//Ok, read up on the IFF spec.  FORM<size> is a chunk, so <size> includes the
//following form <type>.  Then it's <chunk type><chunk size> as I thought, with
//<chunk size> including only data bytes past the header.  There's nothing
//mentioned about sub-chunks or nesting, so NewTek must have added some custom
//stuff to their chunks.  Also the spec calls for all short/long data to be
//aligned on an even byte.  Argh!  There shouldn't be any problem with reading
//unaligned 4 byte blocks from a disk file for pete's sake, so I might just
//ignore that part of the standard.  This is mostly for internal file formats
//anyway.  But to be fully compatible with e.g. writing LWOBs...  Drat.  Ok,
//my file formats look safe at the moment, so internal to the chunk finding
//evening will be automatic, but within each chunk evening is up to the app.

#ifndef IFF_H
#define IFF_H

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

//#include "stdio.h"
#include "cstdio"

#include "CStr.h"

//using namespace std;

class IFF{
private:
	FILE *F;
	bool IsOpen, IsWrite, OwnFile;
	ulong Start;
	ulong IFFLength;
	ulong IFFType;
	ulong ChunkPos, ChunkLength, ChunkType;
public:
	IFF();
	~IFF();
	ulong	StringToUlong(const char *c);
	ulong	Close();	//Returns file length.
	int		Even();	//Aligns read/write pointer on an even boundary, after byte read/writes.
	int		OpenIn(const char *name, int offset = 0);	//Bool.  Please be using EVEN offsets, danke!
	int		OpenIn(FILE *f);	//Attempts to read IFF header from current position in passed in stream.  Stream WILL NOT be closed when IFF object is closed!
	int		IsType(const char *c);	//Returns bool.
	bool	IsFileOpen(){ return IsOpen; };
	bool	IsFileRead(){ return !IsWrite; };	//Check the status of the file.
	bool	IsFileWrite(){ return IsWrite; };
	ulong	FindChunk(const char *c);	//Finds chunk searching from start of file.  Returns length of data in chunk.
	ulong	FindChunkNext(const char *c);	//Finds chunk starting after currently found chunk.  Returns length of data in chunk.
	//A real "NextChunk" would be for iterating through ALL chunks.
	float	ReadFloat();
	float	ReadFloat(float *pnt);
	double	ReadFloat(double *pnt);
	ulong	ReadLong();
	ulong	ReadLong(ulong *pnt);
	int		ReadLong(int *pnt);
	ushort	ReadShort();
	ushort	ReadShort(ushort *pnt);
	uchar	ReadByte();
	uchar	ReadByte(uchar *pnt);
	int		ReadBytes(uchar *p, ulong n);	//The following return boolean values.
	int		ReadBytes(char *p, ulong n){ return ReadBytes((uchar*)p, n); };	//The following return boolean values.
	CStr	ReadString();
	int		ReadString(CStr *str);	//Reads a string of up to 65k into supplied CStr.
	int		OpenOut(const char *name, const char *type, int offset = 0);	//Please be using EVEN offsets, danke!
	int		OpenOut(const char *name, int offset = 0);	//Please be using EVEN offsets, danke!
	int		SetType(const char *type);	//SetType and the second OpenOut are so an anonymous opened IFF file can be passed to something for writing.  CALL ONCE, FIRST THING!
	int		StartChunk(const char *c);
	int		EndChunk();	//Writes length of chunk to file.  NEEDED!
	int		WriteFloat(float v);
	int		WriteLong(ulong v);
	int		WriteShort(ushort v);
	int		WriteByte(uchar v);
	int		WriteBytes(const uchar *p, ulong n);
	int		WriteBytes(const char *p, ulong n){ return WriteBytes((uchar*)p, n); };
	int		WriteString(const char *s);	//Writes a string of up to 65k into file: 2 bytes for len, string, Even().
};

#endif
