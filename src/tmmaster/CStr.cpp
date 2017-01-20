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

/*
	CStr.cpp - Generic String Class by Seumas McNally, based on code in
	"C++ In Plain English" by Brian Overland.
*/

//June 98:  Yick, this needs some serious fixing up.  This is showing its age.

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <algorithm>

#include "CStr.h"

const char CStr::Dummy = '\0';

char CStr::Playground[] = "\0\0";

#define GRANULARITY 32
//Minimum memory alloc.

#define GRAN(a) (((a) + (GRANULARITY - 1)) & (~(GRANULARITY - 1)))

void CStr::InitialInit(){
	pData = new char[GRANULARITY];
	nLength = 0;
	if(pData) pData[0] = '\0';

	return;
}
CStr::CStr(){
	InitialInit();
}
CStr::CStr(const char *s){
	if(!s){
		InitialInit();
		return;
	}
	nLength = strlen(s);
	pData = new char[GRAN(nLength +1)];
	if(!pData){
		nLength = 0;
		return;
	}
	strcpy(pData, s);
}
CStr::CStr(const CStr &s){
	nLength = s.len();
	pData = new char[GRAN(nLength + 1)];
	if(!pData){
		nLength = 0;
		return;
	}
	strcpy(pData, s.get());
}
CStr::CStr(const char *s, const char *ins, int pos){
	if(!s || !ins){
		InitialInit();
		return;
	}
	nLength = strlen(s) + strlen(ins);
	pData = new char[GRAN(nLength + 1)];
	if(!pData){
		nLength = 0;
		return;
	}
	pos = Range(pos, 0, strlen(s));
	if(pos > 0) memcpy(pData, s, pos);
	strcpy(pData + pos, ins);
	strcpy(pData + pos + strlen(ins), s + pos);
}

CStr::~CStr(){
	delete []pData;
}

int CStr::alloc(int size){
	if(size > 0){
		if(GRAN(nLength + 1) != GRAN(size + 1) || pData == NULL){
			delete []pData;
			pData = new char[GRAN(size + 1)];
		}
		if(pData){
			pData[size] = 0;	//Set null, to be safe.
			nLength = size;
			return nLength;
		}else{
			nLength = 0;
		}
	}
	return 0;
}

void CStr::cpy(const char *s){
	if(s && s != pData){
		int n = strlen(s);
		if(GRAN(nLength + 1) != GRAN(n + 1)){
			delete []pData;
			pData = new char[GRAN(n + 1)];
			if(!pData){
				nLength = 0;
				return;
			}
			nLength = n;
		}
		strcpy(pData, s);
		nLength = n;
	}
}

void CStr::cat(const char *s){
	if(s){
		int n = strlen(s);
		if(n <= 0) return;
		if(GRAN(nLength + 1) != GRAN(nLength + n + 1)){
			char *pTemp;
			pTemp = new char[GRAN(n + nLength + 1)];
			if(!pTemp) return;
			if(pData) strcpy(pTemp, pData);
			strcat(pTemp, s);
			delete []pData;
			pData = pTemp;
		}else{
			strcat(pData, s);
		}
		nLength += n;
	}
}

bool CStr::cmp(const char *s) const{
	if(!pData || !s) return false;	//NULL pData means there's been a malloc problem
	int n = 0;
	while(n < nLength && s[n] == pData[n] && s[n]) n++;
	if(n == nLength && s[n] == 0) return true;
	return false;
}

CStr operator+(const CStr &str1, const CStr &str2){
	CStr new_string(str1);
	new_string.cat(str2.get());
	return new_string;
}

CStr operator+(const CStr &str, const char *s){
	CStr new_string(str);
	new_string.cat(s);
	return new_string;
}

CStr operator+(const char *s, const CStr &str){
	CStr new_string(s);
	new_string.cat(str.get());
	return new_string;
}

#define LOWER(a) ( ((a) >= 'A' && (a) <= 'Z') ? (a) + ('a' - 'A') : (a))
#define UPPER(a) ( ((a) >= 'a' && (a) <= 'z') ? (a) - ('a' - 'A') : (a))

CStr Mid(const CStr &str, int pos, int num){
	CStr nstr;
	if(num == 0) return nstr;
	char *tmp;
	int L = str.len();
	if(L == 0 || pos >= L || pos < 0)
		return nstr;
	if(num < 0) num = L;
	num = Range(num, 0, L - pos);
	tmp = new char[num + 1];
	if(!tmp)
		return nstr;
	memcpy(tmp, str.get() + pos, num);
	tmp[num] = '\0';
	nstr.cpy(tmp);
	delete []tmp;
	return nstr;
}

//Returns position of fnd in str starting at 0, searching from pos, or -1 if
//string isn't found.
int Instr(const CStr &str, const CStr &fnd, int pos){
	const char *pstr = str.get();
	const char *pfnd = fnd.get();
	const char *result;
	int lstr = str.len();
	int lfnd = fnd.len();
	pos = Range(pos, 0, lstr);
	if(!pstr || !pfnd || !lstr || !lfnd || pos + lfnd > lstr) return -1;
	result = strstr(pstr + pos, pfnd);
	if(result != NULL) return (int)(result - pstr);
	return -1;
}

CStr Lower(const char *str){
	if(str){
		CStr out = str;
		for(int i = 0; i < out.len(); i++){
			char c = out[i];
			out[i] = LOWER(c);
		}
		return out;
	}
	return CStr();
}
CStr Upper(const char *str){
	if(str){
		CStr out = str;
		for(int i = 0; i < out.len(); i++){
			char c = out[i];
			out[i] = UPPER(c);
		}
		return out;
	}
	return CStr();
}

//Returns a CStr with the input number in human readable ASCII string form.
CStr String(const double a){
	char Buf[100];
	sprintf(Buf, "%f", a);
	return CStr(Buf);
}
//Returns a CStr with the input number in human readable ASCII string form.
CStr String(const int a){
	char Buf[100];
	sprintf(Buf, "%i", a);
	return CStr(Buf);
}

//Returns a CStr containing the input single char and a terminating null.
CStr String(const char c){
	char Buf[2];
	Buf[0] = c;
	Buf[1] = '\0';
	return CStr(&Buf[0]);
}

CStr FileExtension(const char *n){
	CStr str;
	if(n){
		for(int i = strlen(n) - 1; i >= 0; i--){
			if(n[i] == '.'){ str.cpy(&n[i + 1]); break; }
			if(n[i] == '/' || n[i] == '\\' || n[i] == ':') break;
		}
	}
	return str;
}
CStr FileNoExtension(const char *n){
	CStr str(n);
	if(n){
		for(int i = strlen(n) - 1; i >= 0; i--){
			if(n[i] == '.'){ str = Left(str, i); break; }
			if(n[i] == '/' || n[i] == '\\' || n[i] == ':') break;
		}
	}
	return str;
}
CStr FilePathOnly(const char *n){
	CStr str(n);
	if(n){
		for(int i = strlen(n) - 1; i >= 0; i--){
			if(n[i] == '/' || n[i] == '\\' || n[i] == ':'){ str = Left(str, i + 1); break; }
			if(i == 0) str = "";
		}
	}
	return str;
}
CStr FileNameOnly(const char *n){
	CStr str(n);
	if(n){
		for(int i = strlen(n) - 1; i >= 0; i--){
			if(n[i] == '/' || n[i] == '\\' || n[i] == ':'){ str.cpy(&n[i + 1]); break; }
		}
	}
	return str;
}
bool FileInPath(const char *n, const char *path){
	if(n && path && strlen(path) < strlen(n)){
		for(int i = 0; i < (int)strlen(path); i++){
			if(tolower(n[i]) == tolower(path[i]) ||
				(n[i] == '/' && path[i] == '\\') ||
				(n[i] == '\\' && path[i] == '/')){	//Handle mixed up slashes, and mixed case.
				//We're OK.
				continue;
			}else{
				return false;
			}
		}
		return true;
	}
	return false;
}
CStr FileMinusPath(const char *n, const char *path){
	CStr str;
	if(FileInPath(n, path)){
		str = Mid(n, strlen(path));
	}
	return str;
}

CStr FileNameSafe(const char *n){	//Kills path chars and anything not a letter, number, space, or underscore.
	CStr str;
	char buf[1024];
	if(n){
		char c;
		int i = 0;
		while(i < 1024 && (c = n[i]) != 0){
			if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
				c == ' ' || c == '_' || (c == '.' && i > 0)){	//Dots after initial char are ok.
				buf[i] = c;
			}else{
				buf[i] = 'x';	//Replacement for bad char.
			}
			i++;
		}
		buf[std::min(1023, i)] = 0;
		str.cpy(buf);
	}
	return str;
}

bool CmpLower(const char *s1, const char *s2){
	if(s1 && s2){
		int n = 0;
		while(s1[n] && s2[n] && LOWER(s1[n]) == LOWER(s2[n])) n++;
		if((s1[n] | s2[n]) == 0) return true;
		return false;
	}
	return false;
}

CStr PadString(const char *str, int padlen, char padchar, bool clip){
	static char padbuf[1024];
	if(str && padlen > 0){
		int len = strlen(str);
		len = std::min(1023, len);
		padlen = std::min(1023, padlen);
		memset(padbuf, padchar, padlen);
		memcpy(padbuf, str, len);
		if(clip) padbuf[padlen] = 0;
		else padbuf[len] = 0;
		return padbuf;
	}
	return "";
}

