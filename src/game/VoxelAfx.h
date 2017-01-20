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

//Fixed Point bits.
#define FP 8
#define FP2 16
//Value of fixed point chunk, for use in floating point calcs. etc.
#define FPVAL 256
#define FP2VAL 65536
#define INVFPVAL 0.00390625f
//Voxel Y coord to world Y coord bitwise scale factor.
#define VOXSQUEEZE 2
//Bitwise Y size of voxel in world coord fractions.
#define VP 6

#include "pi.h"

extern char DbgBuf[];
extern int DbgBufLen;
