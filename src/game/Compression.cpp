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

#include "Compression.h"
#include <cstdlib>
#include <cstring>

#define ZLIB_DLL
//#define _WINDOWS
#include "zlib.h"


const int MemoryBuffer::BufSizeStep = 32;
#define GRAN(a) (((a) + (MemoryBuffer::BufSizeStep - 1)) & (~(MemoryBuffer::BufSizeStep - 1)))

MemoryBuffer::MemoryBuffer() : data(0), len(0) {
	return;
}
MemoryBuffer::MemoryBuffer(const MemoryBuffer &t) : data(0), len(0) {
	*this = t;
}
MemoryBuffer::MemoryBuffer(unsigned int size) : data(0), len(0) {
	Init(size);
}
MemoryBuffer::MemoryBuffer(const void *src, unsigned int size) : data(0), len(0) {
	Init(size);
	Suck(src, size);
}
MemoryBuffer::~MemoryBuffer(){
	Free();
}
bool MemoryBuffer::Init(unsigned int size){
	Free();
	if(size > 0){
		data = malloc(GRAN(size));
		if(data){
			len = size;
			return true;
		}
	}
	return false;
}
void MemoryBuffer::Free(){
	if(data) free(data);
	data = 0;
	len = 0;
}
int MemoryBuffer::Resize(unsigned int newsize){
	if(newsize > 0){
		if(true){//GRAN(len) != GRAN(newsize) || !data){
			void *data2 = malloc(GRAN(newsize));
			if(data2){
				unsigned int cpylen = newsize;
				if(len < cpylen) cpylen = len;
				if(data && cpylen) memcpy(data2, data, cpylen);	//Copy old data to new buffer.
				if(data) free(data);
				data = data2;
				len = newsize;
				return 1;
			}
		}else return 1;
	}else Free();
	return 0;
}
int MemoryBuffer::Suck(const void *src, unsigned int size){
	if(len < size) size = len;
	if(src && size && data){
		memcpy(data, src, size);
		return 1;
	}
	return 0;
}
unsigned int MemoryBuffer::Blow(void *out, unsigned int maxout){
	if(len < maxout) maxout = len;
	if(out && maxout && data){
		memcpy(out, data, maxout);
		return 1;
	}
	return 0;
}
MemoryBuffer &MemoryBuffer::operator=(const MemoryBuffer &t){
	if(this != &t){
		Resize(t.len);
		Suck(t.data, t.len);
	}
	return *this;
}


MemoryBuffer ZCompress(const void *src, unsigned int size){
	MemoryBuffer mem;
	if(src && size){
		z_stream c_stream; /* compression stream */
		c_stream.zalloc = (alloc_func)0;
		c_stream.zfree = (free_func)0;
		c_stream.opaque = (voidpf)0;
		//
		if(mem.Init(size + (size / 100) + 32) && mem.Data() && mem.Length()){
			if(Z_OK == deflateInit(&c_stream, Z_DEFAULT_COMPRESSION)){
				c_stream.next_in  = (Bytef*)src;
				c_stream.avail_in = size;
				c_stream.next_out = (unsigned char*)mem.Data();
				c_stream.avail_out = mem.Length();
				if(Z_STREAM_END == deflate(&c_stream, Z_FINISH)){
					unsigned int newsize = c_stream.total_out;//mem.Length() - c_stream.avail_out;
					deflateEnd(&c_stream);
					mem.Resize(newsize);
					return mem;
				}
				deflateEnd(&c_stream);
			}
			mem.Free();
		}
	}
	return mem;
}
MemoryBuffer ZCompress(MemoryBuffer *mem){
	if(mem) return ZCompress(mem->Data(), mem->Length());
	else return MemoryBuffer();
}
MemoryBuffer ZUncompress(const void *src, unsigned int size){
	MemoryBuffer mem;
	if(src && size){
		z_stream c_stream; /* compression stream */
		c_stream.zalloc = (alloc_func)0;
		c_stream.zfree = (free_func)0;
		c_stream.opaque = (voidpf)0;
		c_stream.next_in = (unsigned char*)src;
		c_stream.avail_in = size;
		//
		if(mem.Init(size /* * 2 */) && mem.Data() && mem.Length()){	//Temporarily forcing later size-up to test code.
			if(Z_OK == inflateInit(&c_stream)){
			//	c_stream.next_in  = (Bytef*)src;
			//	c_stream.avail_in = size;
				c_stream.next_out = (unsigned char*)mem.Data();
				c_stream.avail_out = mem.Length();
				int err;
				while(Z_STREAM_END != (err = inflate(&c_stream, Z_SYNC_FLUSH))){
					if(err != Z_OK) break;
					if(!mem.Resize(mem.Length() + size)) break;
					c_stream.next_out = (unsigned char*)mem.Data() + c_stream.total_out;
					c_stream.avail_out = mem.Length() - c_stream.total_out;
				}
				inflateEnd(&c_stream);
				if(err == Z_STREAM_END){
					mem.Resize(c_stream.total_out);
					return mem;
				}
			}
			mem.Free();
		}
	}
	return mem;
}
MemoryBuffer ZUncompress(MemoryBuffer *mem){
	if(mem) return ZUncompress(mem->Data(), mem->Length());
	else return MemoryBuffer();
}

