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

#ifndef _FILECRCLIST_H
#define _FILECRCLIST_H

#include "CStr.h"
#include "Trees.h"

class FileCRCList : public LinklistBase<FileCRCList> {
public:
	CStr filename;
	unsigned long crc;
	int size;
	FileCRCList() {filename = ""; crc = 0;}
//	FileCRCList(char *file, unsigned long hash) {filename = file; crc = hash;}
	FileCRCList(const char *file, unsigned long hash, int len) {filename = file; crc = hash; size = len;}

	bool FileCRC(const char *file);

	FileCRCList *FindName(const char *file);
};


#endif /* _FILECRCLIST_H */
