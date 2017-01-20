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

#ifndef __LADDERMANAGER_H__
#define __LADDERMANAGER_H__

#include "CStr.h"

struct LadderEntry{
	CStr name;
	float skill;
};

struct MapInfo;

#define LADDER_SIZE 100
struct LadderManager{
	LadderEntry ladder[LADDER_SIZE];
	int PlayerRank;
	int RacesRun, RacesStarted, RacesAsChamp;
	int Frags, Deaths;
	CStr PlayerName;
	int FirstRankInRace, NumRanksInRace;	//Define the window of tanks participating in the current race.
	bool Mirrored;
	int RaceInProgress;
	CStr LastFileName;
	//
	MapInfo *Maps;
	int NumMaps, NextMap;
public:
	LadderManager();
	~LadderManager();
	void Free();
	bool Init(const char *playername, int startingrank);
	bool Load(const char *name);
	bool Save(const char *name = NULL);
	int SetupRace(int tanks);
	int GetAICount();	//Number of AI tanks to create when setting up race.
	CStr GetAIName(int n);	//Name of nth AI tank.
	float GetAISkill(int n);	//Skill of nth AI tank.
	int GetRaceResults();
	CStr GetNextMapName();
	CStr GetNextMapFile();
};

#endif // __LADDERMANAGER_H__
