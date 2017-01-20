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

//High performance timer object.
//By Seumas McNally.
//
//Nov 98, now handles systems without perftimers properly, e.g. Cyrix.
//Downgrades to timeGetTime() and precision maxes at milliseconds.

#ifndef TIMER_H
#define TIMER_H

class Timer{
private:
	unsigned int tstart, tticks, tnow;
	bool Started;

public:
	Timer();
	~Timer();
	void Start();
	unsigned int Check(unsigned int FracSec);	//Enter the fractions of a second you'd like the result in, e.g. 1000 for ms.

	static unsigned int GetClock();
};

#endif
