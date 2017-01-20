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

#include "Timer.h"

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif

Timer::Timer()
{
	Started = false;
	Start();
}

Timer::~Timer()
{
}

void Timer::Start()
{
	tstart = GetClock();
	Started = true;
}

unsigned int Timer::Check(unsigned int FracSec)
{
	if(!Started)
		return 0;

	tnow = GetClock();

	if(tnow < tstart){	//Oops, millisecond rollover.
		tnow = (unsigned int)(((tnow + (0xffffffff - tstart)) * FracSec) / 1000);
	}else{
		tnow = (int)(((tnow - tstart) * FracSec) / 1000);
	}

	return tnow;
}

#if __APPLE__
#include <mach/mach_time.h>
#endif

unsigned int Timer::GetClock()
{
#ifdef WIN32
	return timeGetTime();
#elif __APPLE__
    static uint64_t start = mach_absolute_time();
    static mach_timebase_info_data_t sTimebaseInfo = {0};
    if (sTimebaseInfo.denom == 0)
        mach_timebase_info(&sTimebaseInfo);
    
    uint64_t end = mach_absolute_time();
    uint64_t elapsed = end - start;
    
    return elapsed / 1000000 * sTimebaseInfo.numer / sTimebaseInfo.denom;
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC,&now);
	return now.tv_sec*1000+now.tv_nsec/1000000;
#endif
}
