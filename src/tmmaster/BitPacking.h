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

//
//Bit packing and bit unpacking classes, by Seumas McNally, July/August 1999.
//Thanks go to Jonathan Blow, of Bolt-Action Software.
//
//These classes allocate no memory, they are only for working on existing memory blocks.
//They will never, ever overwrite or overread the end of the block they are pointed at.
//

#ifndef BITPACKING_H
#define BITPACKING_H

#include "CStr.h"

#define BPW 32
//BitsPerWord

#define MAX_STRING 1024

class BitPackEngine{
private:
	unsigned char *data;
	int maxlen, bitlen, pos;
public:
	BitPackEngine(char *buffer, int buflength);
	BitPackEngine(unsigned char *buffer, int buflength);
	~BitPackEngine();
public:	//These funcs return 0 for failure and 1 for success.
	int PackInt(int val, int bits);
	int PackUInt(int val, int bits);
	int PackFloatInterval(float val, float min, float max, int bits);
	int PackFloat(float val, int bits);
	int PackString(const char *str, int bits);
	int SkipBits(int bits);
	void Reset();	//Resets packing cursor to start of defined buffer.
public:
	int BytesUsed();	//Returns number of bytes used in buffer.
	int BitsUsed();
	unsigned char *Data(){ return data; };
};

class BitUnpackEngine{
private:
	const unsigned char *data;
	int maxlen, bitlen, pos;
public:
	BitUnpackEngine(const char *buffer, int buflength);
	BitUnpackEngine(const unsigned char *buffer, int buflength);
	~BitUnpackEngine();
public:	//These funcs return 0 for failure and 1 for success.
	int UnpackInt(int &val, int bits);
	int UnpackUInt(unsigned int &val, int bits);
	int UnpackUInt(int &val, int bits){  unsigned int v = val;  int ret = UnpackUInt(v, bits);  val = v;  return ret; };
	int UnpackFloatInterval(float &val, float min, float max, int bits);
	int UnpackFloat(float &val, int bits);
	int UnpackString(CStr &strout, int bits);
	int SkipBits(int bits);
public:
	int BytesUsed();	//Returns number of bytes used in buffer.
	int BitsUsed();
	const unsigned char *Data(){ return data; };
};

//This is a very simple wrapper for BitPackEngine that lets you instantiate a stack
//object with a certain amount of buffer space to pack bits into, which can then be
//cast to a BitPackEngine for passing to another func to e.g. send the data in the buffer.
template <int LEN> class BitPacker : public BitPackEngine{
private:
	unsigned char buf[LEN];
public:
	BitPacker() : BitPackEngine(buf, LEN){ };
	~BitPacker(){ };
};

#endif
