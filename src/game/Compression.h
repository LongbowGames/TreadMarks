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

#ifndef COMPRESSION_H
#define COMPRESSION_H

class MemoryBuffer{
private:
	void *data;
	unsigned int len;
	static const int BufSizeStep;
public:
	MemoryBuffer();
	MemoryBuffer(const MemoryBuffer &t);
	MemoryBuffer(unsigned int size);
	MemoryBuffer(const void *src, unsigned int size);
	~MemoryBuffer();
	bool Init(unsigned int size);
	void Free();
	int Resize(unsigned int newsize);
	int Suck(const void *src, unsigned int size);
	unsigned int Blow(void *out, unsigned int maxout);
	void *Data(){ return data; };
	unsigned int Length(){ return len; };
public:
	MemoryBuffer &operator=(const MemoryBuffer &t);
	operator const char*(){ if(data && len) *((char*)data + (len - 1)) = 0; return (const char*)data; };	//Assure trailing null when accessed as string.
};

MemoryBuffer ZCompress(const void *src, unsigned int size);
MemoryBuffer ZCompress(MemoryBuffer *mem);
MemoryBuffer ZUncompress(const void *src, unsigned int size);
MemoryBuffer ZUncompress(MemoryBuffer *mem);

#endif

