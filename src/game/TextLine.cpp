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

#include "TextLine.hpp"

CText Text;
CText Names;
CText Weapons;
CText Insignia;
CText Language;
CText Paths;
CText SoundPaths;
CText ControlText;

bool CText::Free()
{
    int i;

    if (this->Buffer) free(this->Buffer);

    // clear pointers, reset count
    for (i=0;i<MAX_TEXTLINES;i++) this->Lines[i] = NULL;
    this->NumLines = 0;

    return false;
}

int CText::Load(char *file)
{
    FILE *fp;
    int l;
    char *ptr;

    this->Free();

    fp = fopen(file, "rb");
    if (!fp) return -1;
    fseek(fp,0,SEEK_END); l=ftell(fp); fseek(fp,0,SEEK_SET);
    this->Buffer = (char *)malloc(l+1);
    fread(this->Buffer,1,l,fp); fclose(fp);
    this->Buffer[l] = 0;    // Force an end to the buffer

    ptr = this->Buffer;
    while (*ptr) {
        if (*ptr == '#') {
            // Comment - skip to EOL
            while (*ptr && (*ptr!='\n') && (*ptr!='\r')) ptr++;
            while ((*ptr=='\n') || (*ptr=='\r')) ptr++;
            continue;
        }

        // OK, got a non-comment line
        this->Lines[this->NumLines] = ptr;
        this->NumLines++;
        while (*ptr && (*ptr!='\n') && (*ptr!='\r')) ptr++;
        while ((*ptr=='\n') || (*ptr=='\r')) *ptr++=0;

        if (this->NumLines == MAX_TEXTLINES) return 0;
    }

    return 0;
}

char *CText::Get(int id)
{
    if (id < 0) return NULL;
    if (id >= this->NumLines) return NULL;

    return this->Lines[id];
}

bool Text_FreeText() {
	return Text.Free();
}
int Text_LoadLines(char *file) {
	return Text.Load(file);
}
char *Text_GetLine(int id) {
	return Text.Get(id);
}
