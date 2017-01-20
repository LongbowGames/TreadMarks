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

//Simple checksum and encryption algorithms.

unsigned int Checksum(const void *data, int length);

//The following return malloc()ed buffers that you must
//free() yourself.

int Crypt(const void *data, int length, void **out, int *outlen);

int Decrypt(const void *data, int length, void **out, int *outlen);

