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

#include "Crypt.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

unsigned int Checksum(const void *data, int length){
	unsigned int sum = 0x5e04a58c ^ (unsigned int)length;
	if(data && length > 0){
		for(int n = 0; n < length; n++){
			unsigned int f = sum & 0x80000000;
			sum = (sum << 1) | (f ? 1 : 0);
			unsigned int d = (unsigned int)*((unsigned char*)data + n);
			sum = (sum ^ d) ^ (unsigned int)n;
		}
		return sum;
	}
	return 0;
}

int Crypt(const void *data, int length, void **out, int *outlen){
#ifndef CRYPT_DISABLE_CRYPT
	if(!data || length <= 0 || !out || !outlen) return 0;
	unsigned char *buf;
	int buflen = length;
	buf = (unsigned char*)malloc(length + 256 + 4);	//Max key len and iters is 15.
	if(!buf) return 0;
	srand(time(NULL) + length);
	int iters = rand() % 9 + 7;
	memcpy(buf + 4, data, length);
	buflen = length + 4;
	//Length of encrypted data is in first dword, so keys can be found.
	for(int i = 0; i < iters; i++){
		unsigned char key[16];
		int keylen;
		if(i == iters - 1) keylen = iters;
		else keylen = rand() % 9 + 7;
		int cryptlen = buflen;
		for(int k = 0; k < keylen; k++){
			key[k] = (rand() >> 3) & 255;
			buf[buflen++] = key[k];
		}
		buf[buflen++] = keylen | (rand() & 0xf0);
		//Key and key length stored after data.
		//Now crypt.
		int ik = 0;
		for(int c = 4; c < cryptlen; c++){
			buf[c] ^= key[ik++];
			if(ik >= keylen) ik = 0;
		}
	}
	*((unsigned long*)buf) = (unsigned long)buflen ^ 0xabbafad5;
	*out = buf;
	*outlen = buflen;
	return 1;
#else
	return 0;
#endif
}

int Decrypt(const void *data, int length, void **out, int *outlen){
	if(!data || length <= 0 || !out || !outlen) return 0;
	unsigned char *buf;
//	int buflen = length;
	buf = (unsigned char*)malloc(length);	//Max key len and iters is 15.
	if(!buf){
	//	printf("Bad malloc.\n");
		return 0;
	}
	unsigned long t = *((unsigned long*)data);
	int buflen = (int)(t ^ 0xabbafad5);
	if(buflen != length){
	//	printf("Bad sizes.  External: %d  Internal: %d  T: %x\n", length, buflen, t);
		free(buf);
		return 0;	//Actual length and encoded length must match.
	}
	memcpy(buf, (unsigned char*)data + 4, length - 4);
	buflen -= 4;
	int iters = buf[buflen - 1] & 0x0f;
	for(int i = 0; i < iters; i++){
		unsigned char key[16];
		int keylen = buf[--buflen] & 0x0f;
		for(int k = keylen - 1; k >= 0; k--){
			key[k] = buf[--buflen];
		}
		int ik = 0;
		for(int c = 0; c < buflen; c++){
			buf[c] ^= key[ik++];
			if(ik >= keylen) ik = 0;
		}
	}
	*out = buf;
	*outlen = buflen;
	return 1;
}
