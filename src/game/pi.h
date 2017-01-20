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

#ifndef PI_H
#define PI_H

#define RAD2DEG  57.2957795147f
//114.5915590294
#define DEG2RAD  0.01745329251944f
//0.008726646259722
const double pi = 3.1415926535;
const double pi2 = 3.1415926535 * 2;
const double pid2 = 3.1415926535 / 2;
const double ipi = 1.0 / 3.1415926535;

int fpcos(float a);// {return (int)(cos(a - 90) * FPVAL);};
int fpsin(float a);// {return (int)(sin(a - 90) * FPVAL);};

float tcos(float a);	//Lookup table with interpolation driven cos and sin.
float tsin(float a);

#endif
