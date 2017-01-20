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

#include <stdio.h>
#include <stdlib.h>

#include "TxtBlock.hpp"

CTextBlock TextBlock;
CTextBlock TextBlock2;


bool CTextBlock::Free()
{
    int i;

    if (this->Buffer) free(this->Buffer);

    // clear pointers, reset count
    for (i=0;i<MAX_TEXTBLOCKS;i++) this->Blocks[i] = NULL;
    this->NumBlocks = 0;

    return false;
}

int CTextBlock::Load(char *file)
{
    FILE *fp;
    int l;
    char *ptr;

    this->Free();

    fp = fopen(file, "rb");
    if (!fp)
		return -1;
    fseek(fp,0,SEEK_END);
	l=ftell(fp);
	fseek(fp,0,SEEK_SET);
	if(this->Buffer)
		free(this->Buffer);
    this->Buffer = (char *)malloc(l+1);
    fread(this->Buffer,1,l,fp);
	fclose(fp);
    this->Buffer[l] = 0;

    ptr = this->Buffer;
    this->Blocks[this->NumBlocks] = ptr;
    while (*ptr) {
        if (this->NumBlocks == MAX_TEXTBLOCKS) return false;

        if ( (ptr[0] == '%') && (ptr[1] == '%') ) {
            // End of quote
            this->NumBlocks++;

            // Erase to EOL
            while (*ptr && (*ptr!='\n') && (*ptr!='\r')) *ptr++=0;
            // Erase EOL
            while ((*ptr=='\n') || (*ptr=='\r')) *ptr++=0;

            this->Blocks[this->NumBlocks] = ptr;

            continue;
        }


        ptr++;
    }

    return 0;
}

char *CTextBlock::Get(int id)
{
    if (id < 0) return NULL;
    if (id >= this->NumBlocks) return NULL;

    return this->Blocks[id];
}
