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

//File finder wrapper for Win32 by Seumas McNally.

#ifndef FIND_H
#define FIND_H

#include "CStr.h"

class Find{
private:
	CStrList ListHead;
	int nItems;
public:
	Find();
	Find(const char *name);
	~Find();
	bool Clear();
	int Search(const char *name, bool recursive = false, const char *addpath = NULL);
	int AddSearch(const char *name, bool recursive = false, const char *addpath = NULL);
	//Recursive will search all subdirectories as well.  Addpath, if set, will be
	//prepended to all found file names.  Set it to the directory you're looking
	//in to get full pathnames returned.  Must end in a slash!!
	int Items(){ return nItems; };
	const char *operator[](int n);
};

#endif
