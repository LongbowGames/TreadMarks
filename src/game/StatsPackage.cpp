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

#include "StatsPackage.h"
#include "IFF.h"
#include "Vector.h"
#include "EntityBase.h"
#include "EntityTank.h"
#include "TankGame.h"

StatsPackage::StatsPackage(){
}
bool StatsPackage::Init(){
	LastFileName = "";
	LastPlayerPos = -1;
	for(int i = 0; i < STAT_ENTRIES; i++){
		stats[i].name = "- - - - - - - - - - ";
		stats[i].fragsperhour = 0;
		stats[i].deathsperhour = 0;
		stats[i].bestlaptime = 0;
		stats[i].frags = 0;
		stats[i].deaths = 0;
		stats[i].laps = 0;
		stats[i].time = 0;
		stats[i].place = 0;
		stats[i].skill = 0.0f;
	}
	return true;
}
bool StatsPackage::Load(const char *name){
	IFF iff;
	Init();
	if(name) LastFileName = name;
	if(name && iff.OpenIn(name) && iff.IsType("STAT")){
		int vrsn = 1;
		if(iff.FindChunk("VRSN")){
			iff.ReadLong(&vrsn);
		}
		if(iff.FindChunk("STA1")){
			int l = iff.ReadLong();
			for(int n = 0; n < MIN(l, STAT_ENTRIES); n++){
				iff.ReadString(&stats[n].name);
				iff.ReadLong(&stats[n].fragsperhour);
				iff.ReadLong(&stats[n].deathsperhour);
				iff.ReadLong(&stats[n].bestlaptime);
				iff.ReadLong(&stats[n].frags);
				iff.ReadLong(&stats[n].deaths);
				iff.ReadLong(&stats[n].laps);
				iff.ReadLong(&stats[n].time);
				iff.ReadLong(&stats[n].place);
				if(vrsn >= 2){
					iff.ReadFloat(&stats[n].skill);
				}else{
					stats[n].skill = 0.0f;
				}
			}
		}
		iff.Close();
		return true;
	}
	iff.Close();
	return false;
}
bool StatsPackage::Save(const char *name){
	IFF iff;
	if(name) LastFileName = name;
	if(LastFileName.len() > 0 && iff.OpenOut(LastFileName, "STAT")){
		iff.StartChunk("VRSN");
		iff.WriteLong(2);
		iff.EndChunk();
		iff.StartChunk("STA1");
		iff.WriteLong(STAT_ENTRIES);
		for(int n = 0; n < STAT_ENTRIES; n++){
			iff.WriteString(stats[n].name);
			iff.WriteLong(stats[n].fragsperhour);
			iff.WriteLong(stats[n].deathsperhour);
			iff.WriteLong(stats[n].bestlaptime);
			iff.WriteLong(stats[n].frags);
			iff.WriteLong(stats[n].deaths);
			iff.WriteLong(stats[n].laps);
			iff.WriteLong(stats[n].time);
			iff.WriteLong(stats[n].place);
			iff.WriteFloat(stats[n].skill);
		}
		iff.EndChunk();
		iff.Close();
		return true;
	}
	return false;
}
int StatsPackage::GetRaceResults(){
	//
	EntityBase *PlayerEnt = CTankGame::Get().GetVW()->FindRegisteredEntity("PlayerEntity");
	EntityTankGod *GodEnt = (EntityTankGod*)CTankGame::Get().GetVW()->FindRegisteredEntity("TANKGOD");
	//
	LastPlayerPos = -1;
	if(PlayerEnt && PlayerEnt->QueryInt(ATT_AUTODRIVE) == false){	//Skip if autodrive is on for player.
		StatsEntry stemp, t;
		t.name = PlayerEnt->QueryString(ATT_NAME);
		t.bestlaptime = PlayerEnt->QueryInt(ATT_BESTLAPTIME);
		t.frags = PlayerEnt->QueryInt(ATT_FRAGS);
		t.deaths = PlayerEnt->QueryInt(ATT_DEATHS);
		t.place = PlayerEnt->QueryInt(ATT_PLACE);
		t.time = PlayerEnt->QueryInt(ATT_RACETIME);
		t.laps = PlayerEnt->QueryInt(ATT_LAPS);
		t.fragsperhour = (double)t.frags / MAX(0.05, (double)t.time / (double)(1000 * 60 * 60));
		t.deathsperhour = (double)t.deaths / MAX(0.05, (double)t.time / (double)(1000 * 60 * 60));
		//Find the average opponent skill.
		float skill = 0.0f;
		if(GodEnt){
			int i = 0, n = 0;
			for(EntityBase *e = GodEnt->TankByIndex(i); e; e = GodEnt->TankByIndex(++i)){
				if(e && e != PlayerEnt){
					n++;
					skill += e->QueryFloat(ATT_SKILL);
				}
			}
			skill /= (float)MAX(1, n);
		}
		t.skill = skill;
		stemp = stats[STAT_ENTRIES - 1];	//Set temp to bottom of list.
		if((t.bestlaptime > 0 && (stemp.bestlaptime <= 0 || t.bestlaptime < stemp.bestlaptime)) ||
			(t.bestlaptime <= 0 && t.fragsperhour > stemp.fragsperhour) ){	//We make it into list.
			stats[STAT_ENTRIES - 1] = t;	//Stick player on bottom, then sort.
			LastPlayerPos = STAT_ENTRIES - 1;
			for(int i = 0; i < STAT_ENTRIES; i++){	//Simple bubble sort.
				for(int j = 0; j < STAT_ENTRIES - 1; j++){
					if((stats[j+1].bestlaptime > 0 && (stats[j].bestlaptime <= 0 || stats[j+1].bestlaptime < stats[j].bestlaptime)) ||
						(stats[j+1].bestlaptime <= 0 && stats[j+1].fragsperhour > stats[j].fragsperhour) ){	//Swap!
						t = stats[j];
						stats[j] = stats[j+1];
						stats[j+1] = t;
						if(LastPlayerPos == j) LastPlayerPos = j + 1;
						else if(LastPlayerPos == j + 1) LastPlayerPos = j;	//Track player's position in list.
					}
				}
			}
		}
		return 1;
	}
	return 0;
}

