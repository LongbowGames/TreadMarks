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

#include "VoxelWorld.h"
#include "FileCRCList.h"
#include "TankGame.h"

bool FileCRCList::FileCRC(const char *file)
{
	int tempsize = 0;
	unsigned int tempcrc = 0;

	if (strlen(file) <= 2) return false;

	// Search for filename in list - no dupes
	if (this->FindName(file)) return true;

	CTankGame::Get().GetVW()->GetFileCRC(file, &tempcrc, &tempsize);

//	if (!tempcrc) return false;

	this->InsertObjectAfter(new FileCRCList(file, tempcrc, tempsize));
	return true;
}

FileCRCList *FileCRCList::FindName(const char *file)
{
	FileCRCList *ptr = this->NextLink();

	while (ptr) {
		if (ptr->filename == file) return ptr;
		ptr = ptr->NextLink();
	}

	return NULL;
}
