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

//ASCII text config file parser.

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>

#include "CfgParse.h"
#include "Crypt.h"

using namespace std;

ConfigFile::ConfigFile(){
//	f = NULL;
	data = NULL;
	len = 0;
	cdata = 0;	//Compressed Data/Len (whitespace removed, lowercase keys, no comments).
	clen = 0;
	keypos = 0;
	datapos = 0;
	datalen = 0;
	checksum = 0;
}
ConfigFile::~ConfigFile(){
	Free();
}
int ConfigFile::Read(const char *n, int length){	//Reads named file into internal data area for parsing.
	FILE *f = fopen(n, "rb");
	int ret = Read(f, length);
	if(f) fclose(f);
	return ret;
}
bool ConfigFile::Read(FILE *f, int length){	//Reads named file into internal data area for parsing.
	Free();
	if(f){//(f = fopen(n, "rb"))){
		size_t pos, endpos;
		pos = ftell(f);
		if(length <= 0){
			fseek(f, 0, SEEK_END);
			endpos = ftell(f);
		}else{
			endpos = pos + length;
		}
		fseek(f, pos, SEEK_SET);
		len = endpos - pos;// + 1;
		if((data = (char*)malloc((len + 1) * 2))){
			fread(data, len, 1, f);
			data[len] = '\0';
			cdata = &data[len + 1];
			cdata[len] = '\0';	//CompressedData.
			clen = 0;
			CompressAndChecksum();
		//	fclose(f);
		//	f = NULL;
			return true;
		}
		Free();
	}
	return false;
}
bool ConfigFile::ReadMemBuf(const char *buf, int length){	//Sucks text strings for searching out of a ram buffer.
	Free();
	if(buf && length > 0){
		len = length;
		if((data = (char*)malloc((len + 1) * 2))){
			memcpy(data, buf, len);
			data[len] = '\0';
			cdata = &data[len + 1];
			cdata[len] = '\0';	//CompressedData.
			clen = 0;
			CompressAndChecksum();
			return true;
		}
		Free();
	}
	return false;
}

bool ConfigFile::Free(){
//	if(f) fclose(f);
//	f = NULL;
	if(data) free(data);
	data = NULL;
	len = 0;
	cdata = 0;
	clen = 0;
	checksum = 0;
	return true;
}
int ConfigFile::GetCompressedLength(){
	return clen;
}
int ConfigFile::GetCompressedData(char *dest, int maxlen, int offset, int bytes){
	if(cdata && clen > 0 && dest && maxlen > 0 && offset < clen && offset >= 0){
		if(bytes <= 0) bytes = clen - offset;
		bytes = std::min(maxlen, bytes);
		memcpy(dest, &cdata[offset], bytes);
		return bytes;
	}
	return 0;
}
unsigned int ConfigFile::GetChecksum(){
	return checksum;
}

int ConfigFile::CompressAndChecksum(){
	if(!data || !cdata || len <= 0) return 0;
	int i = 0;
	clen = 0;
	int linestate = 0, eol = 0, comment = 0;
	while(i < len){
		if(data[i] == '\0'){	//Stop at first null.
			break;
		}
		if(data[i] == '\r' || data[i] == '\n'){
			if(eol == 0){
				if(comment == 0) cdata[clen++] = '\n';	//Emit one end of line, if not a comment line!
				eol = 1;
			}
			i++;
			continue;
		}else{
			if(eol){	//Were in eol, now are out of it.
				eol = 0;
				linestate = 0;
				comment = 0;
			}
		}
		if(data[i] == '#') comment = 1;
		if(comment){	//Keep going until end of line resets comment.
			i++;
			continue;
		}
		if(linestate == 0){	//Pre-space.
			if(data[i] == ' ' || data[i] == '\t'){
				i++;
				continue;
			}else{
				linestate = 1;
			}
		}
		if(linestate == 1){	//The key.
			if(data[i] == ' ' || data[i] == '\t' || data[i] == '='){
				linestate = 2;
				cdata[clen++] = ' ';
			}else{
				cdata[clen++] = tolower(data[i++]);
				continue;
			}
		}
		if(linestate == 2){	//Post-space.
			if(data[i] == ' ' || data[i] == '\t' || data[i] == '='){
				i++;
				continue;
			}else{
				linestate = 3;
			}
		}
		if(linestate == 3){	//Value.
			cdata[clen++] = data[i++];
			continue;	//Copy value verbatim until end of line.
		}
	}
	cdata[clen++] = 0;
	//Data is compressed, now checksum.
	checksum = Checksum(cdata, clen);
	return 1;
}

bool ConfigFile::FindKey(const char *n){
	keypos = 0;
	nextkeypos = 0;
	datapos = 0;
	datalen = 0;
	return FindNextKey(n);
}
bool ConfigFile::FindNextKey(const char *n){
	if(data == NULL || len <= 0) return false;
	int i, namelen, match, linestate;
	bool comment;
	char tname[256];
	namelen = std::min(254, int(strlen(n)));
	for(i = 0; i < namelen; i++) tname[i] = tolower(n[i]);	//Get lower version of name to find.
	if(tname[namelen - 1] != ' '){
		tname[namelen++] = ' ';	//Adds a space after the name to find, so no substring mismatches.
	}
	tname[namelen] = '\0';	//Adds a null after everything, just in case.
	keypos = 0;
	datapos = 0;
	datalen = 0;
	match = 0;
	comment = false;
	linestate = 0;	//Prevents matching key strings in data areas.
	i = nextkeypos;
	while(i < len && match < namelen){	//End on match or end of file.
		if(data[i] == '#') comment = true;
		if(data[i] == 10 || data[i] == 13 || data[i] == 0){	//End of line.
			comment = false;	//Stop comment at line end.
			linestate = 0;
			match = 0;
			i++; continue;
		}
		if(comment){
			i++; continue;	//Skip rest of loop.
		}
	//	if(toupper(data[i]) == tname[match] && linestate < 2){
		if(tolower(data[i]) == tname[match] && ((match == 0 && linestate == 0) || (match > 0 && linestate == 1)) ){
			//This change will prevent end-of-substring matches.
			match++;
		}else{
			match = 0;
		}
	//	if(isalpha(data[i])) linestate = std::max(linestate, 1);
		if(isalnum(data[i]) || data[i] == '_') linestate = std::max(linestate, 1);	//Allows keys to start with digits and underscores.
		if((data[i] == ' ' || data[i] == '\t') && linestate == 1) linestate = 2;	//First space AFTER alpha chars turns off key search for this line.  Check is done after search to catch one space char at end of key name.
		i++;
	}
	if(match == namelen){	//Found the key.
		keypos = i - match;
		while(i < len && (data[i] == ' ' || data[i] == '=' || data[i] == '\t')) i++;	//Jump over whitespace and equals sign to find data position.
		datapos = i;
		while(i < len && data[i] != 10 && data[i] != 13 && data[i] != 0) i++;	//Go until end of line char to find data length.
		datalen = i - datapos;
		nextkeypos = i;
		return true;
	}
	return false;
}
int ConfigFile::GetStringVal(char *s, int length){
	if(s && length > 0 && data && datapos){
		int dpos = 0, spos = datapos, quotes = 0;
		while(dpos < length - 1 && spos < datapos + datalen && quotes < 2){
			if(data[spos] == '\"' && (spos == datapos || quotes > 0)){	//Do not copy quote char, and break at second quote.
				//Fix, if first char is not quote, then treat all following quotes as data.
				quotes++;
			}else{
				s[dpos++] = data[spos];
			}
			spos++;
		}
		s[dpos] = '\0';	//Add null.
		return strlen(s);
	}
	return 0;
}
bool ConfigFile::GetStringVal(CStr &str){
	char buf[1024];
	if(GetStringVal(buf, 1024)){
		str = buf;
		return true;
	}
	return false;
}

bool ConfigFile::GetEnumVal(int *i, ConfigEnum *enm, int nenm){
	char buf[1024];
	if(i && enm && nenm > 0 && GetStringVal(buf, 1024)){
		for(int n = 0; n < nenm; n++){
			if(CmpLower(enm[n].name, buf)){
				*i = enm[n].val;
				return true;
			}
		}
	}
	return false;
}

bool ConfigFile::GetIntVal(int *pi){
	if(pi && data && datapos){
		*pi = atoi(&data[datapos]);
		return true;
	}
	return false;
}
bool ConfigFile::GetBoolVal(bool *pb){
	if(!pb)
		return false;

	CStr str;
	if(!GetStringVal(str))
		return false;

	str = Lower(str.get());
	if(str == "1" || str == "true")
	{
		*pb = true;
		return true;
	}
	else if(str == "0" || str == "false")
	{
		*pb = false;
		return true;
	}

	// Slight hack; we also want to accept any non-zero number as true, because some
	// entity files use other numbers to mean true, and I'm not sure if any mods in
	// the wild do as well.
	for(int i=0; i<str.len(); ++i)
		if((str[i] < '0' || str[i] > '9') && (i != 0 || str[i] != '-'))
			return false;
	*pb = true;

	return true;
}
bool ConfigFile::GetFloatVal(float *pf){
	if(pf && data && datapos){
		*pf = (float)atof(&data[datapos]);
		return true;
	}
	return false;
}
int ConfigFile::GetFloatVals(float *pf, int num){
	if(pf && num >= 0 && data && datapos){
		char buf[100];
		int converted = 0;
		int li = 0;
		for(int i = 0; i <= datalen; i++){
			if(data[datapos + i] == ',' || i == (datalen)){
				if(converted < num && i > li){
					memcpy(buf, &data[datapos + li], std::min(100, i - li));
					buf[std::min(99, i - li)] = 0;
					pf[converted] = (float)atof(buf);
					converted++;
					li = i + 1;
				}
			}
		}
		return converted;
	}
	return 0;
}


//Log file code tacked on here.
//Gads this is ugly crud...
//
//Nothing to see here, move along folks, that's right, keep moving, nothing to see here!
//
class tLogFile{
public:
	FILE *f;
	tLogFile() : f(NULL) {};
	~tLogFile(){
		if(f) fclose(f);
	};
};

bool LogFileEnabled = false;
tLogFile tlog;
DebugLogFuncPtr DLFP = NULL;

bool EnableLogFile(bool enable, const char *name){
	if(enable){
		if(LogFileEnabled && tlog.f){
			fclose(tlog.f);
			tlog.f = NULL;
			LogFileEnabled = false;
		}
		if(name && (tlog.f = fopen(name, "w"))){
			return LogFileEnabled = true;
		}
	}else{
		if(LogFileEnabled && tlog.f){
			fclose(tlog.f);
			tlog.f = NULL;
			LogFileEnabled = false;
			return true;
		}
	}
	return false;
}
void OutputDebugLog(const char *text, int dgblevel){	//Outputs to debugger, and if enabled, to log file.
	if(text){
		if(DLFP) DLFP(text, dgblevel);

		if(LogFileEnabled && tlog.f){
			fwrite(text, strlen(text), 1, tlog.f);
		}
	}
}
void RedirectDebugLog(DebugLogFuncPtr dlfp){
	DLFP = dlfp;
}

