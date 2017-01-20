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

//FileManager class, handles searching multiple directories and packed
//files for files.  Has internal read/seek members or can return FILE*.

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <cstdio>
#include "CStr.h"
#include "Find.h"

#define FILE_STACK_SIZE 32

class FileManager{
private:
	FILE *f;
	Find find;
	int findIndex;
	//Search paths and packed files.
	CStrList SearchDirHead;
	CStrList PackedFileHead;
	CStr FileName;
	unsigned long FileOffset;
	//
	FILE *FileStack[FILE_STACK_SIZE];
	CStr FileNameStack[FILE_STACK_SIZE];
	unsigned long FileOffsetStack[FILE_STACK_SIZE];
	int nFileStack;
public:
	//Access members.
	FileManager();
	~FileManager();
	void ClearSearchDirs();	//Empties the search dir and packed file list.
	int AddSearchDir(const char *dir);	//Adds a directory to the search path.  Trailing slash added if not present.
	int FindPackedFiles();	//Scans for packed files in current search directories and makes note of them for further searches.
public:
	FILE *Open(const char *name);	//The returned pointer is for convenience only!  DO NOT fclose the file stream!
	FILE *GetFile();	//Ditto.
	FILE *OpenWildcard(const char *wild, char *nameret = NULL, int namelen = NULL, bool recursive = false, bool scanbasedir = true);	//Opens a file by wildcard.
	FILE *NextWildcard(char *nameret = NULL, unsigned int namelen = NULL);	//Continues searching by previous wildcard.
	//Note that when wildcarding, only the first occurance of multiple files with the same name in the search path will ever be opened.
	//
	bool PushFile();	//Used to nest access to files through FileManager.  MUST CALL POP FOR EVERY PUSH!  Nesting wildcard opens will fail.
	bool PopFile();	//Returns true if file stack was successfully popped, false if stack is at bottom.
	//
	CStr GetFileName();
	unsigned long GetFileOffset();	//These are for external routines that MUST open a file via name and offset.
	void Close();
	//These work on the currently opened file, either physical or in pack.
	size_t fread(void *buf, size_t size, size_t count);
	int fseek(long offset, int origin);
	long ftell();
	long length();	//Returns the length in bytes of currently opened file.
	long ReadLong();
	char ReadByte();
};

#endif
