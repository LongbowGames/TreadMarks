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

#ifndef _TXTBLOCK_H
#define _TXTBLOCK_H

#define MAX_TEXTBLOCKS 256

class CTextBlock {
private:
	char *Buffer;
	int NumBlocks;
	char *Blocks[MAX_TEXTBLOCKS];
public:
	CTextBlock() {Buffer=NULL; NumBlocks=0; Free();}
	~CTextBlock() {Free();}
	bool Free();
	int Load(char *file);
	char *Get(int id);

};

extern CTextBlock TextBlock;
extern CTextBlock TextBlock2;

#endif /*_TXTBLOCK_H*/
