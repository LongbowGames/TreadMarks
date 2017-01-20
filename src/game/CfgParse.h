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

//Note, it uses fseek(f, 0, SEEK_END), so at the moment do not attempt to load
//config files from inside of packed data files!
//Scratch that, it'll work now if you simply pass in the length of the file as
//divined by other means.

#ifndef CFGPARSE_H
#define CFGPARSE_H

#include <cstdio>
#include <functional>

#include "CStr.h"

//using namespace std;

//Pass an array of these to the GetEnumVal function.
struct ConfigEnum{
	char name[256];
	int val;
};

class ConfigFile{
private:
//	FILE *f;
	char *data, *cdata;
	int len, clen;
	int keypos, nextkeypos, datapos, datalen;
	unsigned int checksum;
public:
	ConfigFile();
	~ConfigFile();
	int Read(const char *n, int length = 0);
	bool Read(FILE *file, int length = 0);
	bool ReadMemBuf(const char *buf, int length);	//Sucks text strings for searching out of a ram buffer.
	bool Free();
	bool FindKey(const char *n);
	bool FindNextKey(const char *n);
	int GetStringVal(char *s, int length);
	bool GetStringVal(CStr &str);
	bool GetIntVal(int *i);
	bool GetBoolVal(bool *pb);
	bool GetFloatVal(float *i);
	int GetFloatVals(float *i, int num);	//Reads comma-seperated list of floats
	bool GetEnumVal(int *i, ConfigEnum *enm, int nenm);
public:
	int GetCompressedLength();
	int GetCompressedData(char *dest, int maxlen, int offset = 0, int bytes = 0);
	unsigned int GetChecksum();
	//Compressed Data is a version of the config file text created at load time which has
	//all white space and comments removed, and key names converted to lower case, suitable
	//for faster network transmission.  A checksum of the compressed version is also created,
	//which can be easily compared with the checksum of other config files.  Changes to
	//comments or whitespace won't affect the checksum.
private:
	int CompressAndChecksum();
};

//Log file functions tacked on here.
typedef std::function<void(const char* text, int dbglevel)> DebugLogFuncPtr;
//
bool EnableLogFile(bool enable, const char *name = NULL);
void OutputDebugLog(const char *text, int dgblevel = 0);	//Outputs to debugger, and if enabled, to log file.

void RedirectDebugLog(DebugLogFuncPtr dlfp);	//Assigns a function to receive debugging output.

#endif
