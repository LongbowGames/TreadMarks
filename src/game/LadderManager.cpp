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

#include "EntityTank.h"
#include "LadderManager.h"

LadderManager::LadderManager(){
	PlayerName = "None";
	PlayerRank = 0;
	RacesRun = 0;
	FirstRankInRace = 0;
	NumRanksInRace = 0;
	Mirrored = false;
	RacesStarted = 0;
	RacesAsChamp = 0;
	Frags = 0;
	Deaths = 0;
	//
	Maps = NULL;
	NumMaps = 0;
	NextMap = 0;
	RaceInProgress = 0;
}
LadderManager::~LadderManager(){
	Free();
}
void LadderManager::Free(){
	if(Maps) delete [] Maps;
	Maps = NULL;
	NumMaps = 0;
}
bool LadderManager::Init(const char *playername, int startingrank){
	if(!playername) return false;
	Free();
	int i = 0;
	for(i = 0; i < LADDER_SIZE; i++){
		CTankGame::Get().SetNextTankName((CTankGame::Get().GetNextTankName() + 1) % MAX(1, CTankGame::Get().GetNumTankNames()));	//Cycle through tank names.
		ladder[i].name = CTankGame::Get().GetTankName(CTankGame::Get().GetNextTankName());
		ladder[i].skill = 1.0f - ((float)i * 0.01f);
	}
	startingrank = CLAMP(startingrank, 0, LADDER_SIZE - 1);
	PlayerRank = startingrank;
	RacesRun = 0;
	RacesStarted = 0;
	RacesAsChamp = 0;
	Frags = 0;
	Deaths = 0;
	PlayerName = playername;
	ladder[PlayerRank].name = playername;
	ladder[PlayerRank].skill = 1.0f;
	FirstRankInRace = NumRanksInRace = 0;
	Mirrored = false;
	NextMap = 0;
	RaceInProgress = 0;
	LastFileName = "";
	//
	int racemaps = 0;
	for(i = 0; i < CTankGame::Get().GetNumMaps(); i++){	//Count racing maps.
		if(CTankGame::Get().GetMap(i)->maptype == MapTypeRace)
			racemaps++;
	}
	if(racemaps > 0){	//Initialize map list, and shuffle.
		Maps = new MapInfo[racemaps];
		if(Maps){
			NumMaps = racemaps;
			int n = 0;
			for(i = 0; i < CTankGame::Get().GetNumMaps(); i++){	//Count racing maps.
				if(CTankGame::Get().GetMap(i)->maptype == MapTypeRace && n < NumMaps){
					Maps[n] = *CTankGame::Get().GetMap(i);	//Fill our map array with race maps from global array.
					n++;
				}
			}
			MapInfo tmap;	//Now shuffle our list.
			for(i = 0; i < NumMaps; i++){
				n = TMrandom() % MAX(1, NumMaps);
				tmap = Maps[i];
				Maps[i] = Maps[n];
				Maps[n] = tmap;
			}
		}
	}
	//
	return true;
}
bool LadderManager::Load(const char *name){
	IFF iff;
	Free();
	if(name && iff.OpenIn(name) && iff.IsType("LADR")){
		LastFileName = name;
		if(iff.FindChunk("LDR1")){
			iff.ReadLong(&PlayerRank);
			iff.ReadLong(&RacesRun);
			iff.ReadString(&PlayerName);
			iff.ReadLong((int*)&Mirrored);
			iff.ReadLong(&NextMap);
		}
		if(iff.FindChunk("LDR4")){
			iff.ReadLong(&RacesStarted);
			iff.ReadLong(&RacesAsChamp);
			iff.ReadLong(&Frags);
			iff.ReadLong(&Deaths);
		}
		if(iff.FindChunk("LDR2")){
			int l = iff.ReadLong();
			l = MIN(l, LADDER_SIZE);
			for(int i = 0; i < l; i++){
				iff.ReadFloat(&ladder[i].skill);
				iff.ReadString(&ladder[i].name);
			}
		}
		if(iff.FindChunk("LDR3")){
			NumMaps = iff.ReadLong();
			if(Maps = new MapInfo[NumMaps]){
				for(int n = 0; n < NumMaps; n++){
					iff.ReadString(&Maps[n].title);
					iff.ReadString(&Maps[n].file);
				}
			}
		}
		iff.Close();
		return true;
	}
	iff.Close();
	return false;
}
bool LadderManager::Save(const char *name){
	IFF iff;
	if(name) LastFileName = name;
	if(iff.OpenOut(LastFileName, "LADR")){
		iff.StartChunk("VRSN");
		iff.WriteLong(1);
		iff.EndChunk();
		iff.StartChunk("LDR1");
		iff.WriteLong(PlayerRank);
		iff.WriteLong(RacesRun);
		iff.WriteString(PlayerName);
		iff.WriteLong(Mirrored);
		iff.WriteLong(NextMap);
		iff.EndChunk();
		//
		iff.StartChunk("LDR4");
		iff.WriteLong(RacesStarted);
		iff.WriteLong(RacesAsChamp);
		iff.WriteLong(Frags);
		iff.WriteLong(Deaths);
		iff.EndChunk();
		//
		iff.StartChunk("LDR2");
		iff.WriteLong(LADDER_SIZE);
		for(int i = 0; i < LADDER_SIZE; i++){
			iff.WriteFloat(ladder[i].skill);
			iff.WriteString(ladder[i].name);
		}
		iff.EndChunk();
		//
		if(Maps && NumMaps){
			iff.StartChunk("LDR3");
			iff.WriteLong(NumMaps);
			for(int n = 0; n < NumMaps; n++){
				iff.WriteString(Maps[n].title);
				iff.WriteString(Maps[n].file);
			}
			iff.EndChunk();
		}
		iff.Close();
		return true;
	}
	return false;
}
int LadderManager::SetupRace(int tanks){
	tanks = MIN(tanks, LADDER_SIZE);
	FirstRankInRace = CLAMP(PlayerRank - (tanks / 2), 0, LADDER_SIZE - tanks);
	NumRanksInRace = tanks;
	RaceInProgress = 1;
	RacesStarted++;
	return 1;
}
int LadderManager::GetRaceResults(){
	//
	EntityBase *PlayerEnt = CTankGame::Get().GetVW()->FindRegisteredEntity("PlayerEntity");
	EntityTankGod *GodEnt = (EntityTankGod*)CTankGame::Get().GetVW()->FindRegisteredEntity("TANKGOD");
	//
	if(PlayerEnt && GodEnt){
		EntityBase *e = NULL;
		for(int n = 0; e = GodEnt->TankByIndex(n); n++){
			if(n < NumRanksInRace){
				if(e->GID == PlayerEnt->GID){	//Found player tank.
					PlayerRank = FirstRankInRace + n;
				}else{	//Probably AI tank.
					CStr s = e->QueryString(ATT_NAME);
					if(Left(s, CTankGame::Get().GetAIPrefix().len()) == CTankGame::Get().GetAIPrefix()){	//Proven AI tank.
						int i = FirstRankInRace + n;
						ladder[i].name = Mid(s, CTankGame::Get().GetAIPrefix().len());
						ladder[i].skill = e->QueryFloat(ATT_SKILL);
					}
				}
			}
		}
	}
	//
	if(PlayerEnt){
		Frags += PlayerEnt->QueryInt(ATT_FRAGS);
		Deaths += PlayerEnt->QueryInt(ATT_DEATHS);
	}
	//
	//Just to be sure player is in ladder...
	PlayerRank = CLAMP(PlayerRank, 0, LADDER_SIZE - 1);
	ladder[PlayerRank].name = PlayerName;
	ladder[PlayerRank].skill = 1.0f;
	RacesRun++;
	if(PlayerRank == 0) RacesAsChamp++;

	do
	{
		NextMap++;
		if(NextMap >= NumMaps){
			NextMap = 0;
			Mirrored = !Mirrored;	//Toggle mirrored status.
		}
	} while (Maps[NextMap].maptype == MapTypeTraining);
	RaceInProgress = 0;
	//
	return 1;
}
int LadderManager::GetAICount(){	//Number of AI tanks to create when setting up race.
	return MAX(0, NumRanksInRace - 1);
}
CStr LadderManager::GetAIName(int n){	//Name of nth AI tank.
	n += FirstRankInRace;
	if(n >= PlayerRank) n++;	//Skip over player entry in ladder.
	return ladder[CLAMP(n, 0, LADDER_SIZE - 1)].name;
}
float LadderManager::GetAISkill(int n){	//Skill of nth AI tank.
	n += FirstRankInRace;
	if(n >= PlayerRank) n++;	//Skip over player entry in ladder.
	return ladder[CLAMP(n, 0, LADDER_SIZE - 1)].skill;
}
CStr LadderManager::GetNextMapName(){
	if(Maps && NextMap >= 0 && NextMap < NumMaps) return Maps[NextMap].title;
	return "";
}
CStr LadderManager::GetNextMapFile(){
	if(Maps && NextMap >= 0 && NextMap < NumMaps) return Maps[NextMap].file;
	return "";
}
