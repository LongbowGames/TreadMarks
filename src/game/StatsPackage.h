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

#ifndef __STATSPACKAGE_H__
#define __STATSPACKAGE_H__

#include "CStr.h"

struct StatsEntry{
	CStr name;
	int fragsperhour;
	int deathsperhour;
	int bestlaptime;
	int frags, deaths, laps, time, place;
	float skill;
};
#define STAT_ENTRIES 30
struct StatsPackage{
	StatsEntry stats[STAT_ENTRIES];
	CStr LastFileName;
	int LastPlayerPos;
public:
	StatsPackage();
	bool Init();
	bool Load(const char *name);
	bool Save(const char *name = NULL);
	int GetRaceResults();
};

#endif // __STATSPACKAGE_H__
