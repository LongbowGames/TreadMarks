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

//
//Implementation for Tread Marks entities, other than Race Tanks.

//Remember that all new incidental entities must have their resources force-cached
//by an entity that starts on the world, such as a tree or tank!
//NEW: Not necessarily, pre-caching helps but anything not will be dynamic-loaded
//properly by the main entity system.

#pragma inline_depth(255)

#include "EntityTank.h"
#include "Basis.h"
#include "EntityFlag.h"

#include "TextLine.hpp"

#ifdef WIN32
#define strcasecmp(a, b) _stricmp(a, b)
#endif

//*****************************************************************
//**  Global entity type setup function.  Must be modified to
//**  handle all available entity classes.
//*****************************************************************
extern EntityTypeBase *CreateRacetankType(ConfigFile *cfg){
	if(!cfg) return 0;
	char superclass[256], subclass[256];
	if(cfg->FindKey("class") && cfg->GetStringVal(superclass, 256)){
		if(cfg->FindKey("type") && cfg->GetStringVal(subclass, 256)){
			if(strcasecmp(superclass, "racetank") == 0) return new EntityRacetankType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "tree") == 0) return new EntityTreeType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "smoke") == 0) return new EntitySmokeType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "crud") == 0) return new EntityCrudType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "course") == 0) return new EntityCourseType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "explo") == 0) return new EntityExploType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "projectile") == 0) return new EntityProjectileType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "bauble") == 0) return new EntityBaubleType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "explosphere") == 0) return new EntityExploSphereType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "weapon") == 0) return new EntityWeaponType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "powerup") == 0) return new EntityPowerUpType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "flag") == 0) return new EntityFlagType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "tankgod") == 0) return new EntityTankGodType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "spewer") == 0) return new EntitySpewerType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "squishy") == 0) return new EntitySquishyType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "insignia") == 0) return new EntityInsigniaType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "racetankdoppelganger") == 0) return new EntityRacetankDoppelgangerType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "tutorial") == 0) return new EntityTutorialType(cfg, superclass, subclass);
//			if(strcasecmp(superclass, "target") == 0) return new EntityTargetType(cfg, superclass, subclass);
		}
	}
	return 0;
}

//This function MUST be called ONCE by your main app to register the RaceTank classes.
int RegisterRacetankEntities(){
	return RegisterEntityTypeCreator(CreateRacetankType);
}


//******************************************************************
//**  TANK GOD Entity  --  Derived Diety
//******************************************************************
EntityTankGodType::EntityTankGodType(ConfigFile *cfg, const char *c, const char *t) : EntityGodType(cfg, c, t){
	type_racepeashooter = 0;
	type_dmpeashooter = 6;
	type_globalmultikilldecevery = 2.0f;
	type_globalmultikillthresh = 4;
	type_globalmultikillsound = "chaos";
	//
	ReadCfg(cfg);
}
void EntityTankGodType::ReadCfg(ConfigFile *cfg){
	if(cfg){
		if(cfg->FindKey("mmaplinewidth")) cfg->GetFloatVal(&type_mmlinew);
		if(cfg->FindKey("mmapopacity")) cfg->GetFloatVal(&type_mmopac);
		if(cfg->FindKey("mmapcliprad")) cfg->GetFloatVal(&type_mmcliprad);
		if(cfg->FindKey("mmapclipbias")) cfg->GetFloatVal(&type_mmclipbias);
		if(cfg->FindKey("mmapx")) cfg->GetFloatVal(&type_mmx);
		if(cfg->FindKey("mmapy")) cfg->GetFloatVal(&type_mmy);
		if(cfg->FindKey("mmapscale")) cfg->GetFloatVal(&type_mmscale);
		if(cfg->FindKey("mmapplayercolor")) cfg->GetFloatVals(&type_mmpcol[0], 3);
		if(cfg->FindKey("mmaplinecolor")) cfg->GetFloatVals(&type_mmlcol[0], 3);
		if(cfg->FindKey("mmapenemycolor")) cfg->GetFloatVals(&type_mmecol[0], 3);
		if(cfg->FindKey("mmapteamcolor")) cfg->GetFloatVals(&type_mmtcol[0], 3);
		if(cfg->FindKey("mmapflagcolor")) cfg->GetFloatVals(&type_mmfcol[0], 3);
		if(cfg->FindKey("racepeashooter")) cfg->GetIntVal(&type_racepeashooter);
		if(cfg->FindKey("dmpeashooter")) cfg->GetIntVal(&type_dmpeashooter);
		if(cfg->FindKey("globalmultikilldecevery")) cfg->GetFloatVal(&type_globalmultikilldecevery);
		if(cfg->FindKey("globalmultikillthresh")) cfg->GetIntVal(&type_globalmultikillthresh);
		if(cfg->FindKey("globalmultikillsound")) cfg->GetStringVal(type_globalmultikillsound);
	}
}
bool EntityTankGodType::CacheResources(){
	if(ResCached) return true;
	if(EntityGodType::CacheResources()){
		ReadCfg(&VW->MapCfg);
	}
	return ResCached;
}
void EntityTankGodType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	EntityGodType::ListResources(fl);
}
void EntityTankGodType::UnlinkResources(){
	EntityGodType::UnlinkResources();
}
EntityBase *EntityTankGodType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityTankGod(this, Pos, Rot, Vel, id, flags);
}
EntityTankGod::EntityTankGod(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityGod(et, Pos, Rot, Vel, id, flags) {
	EntityTankGodType *TP = (EntityTankGodType*)et;
	CanCollide = false;
	RegisterEntity("TANKGOD");	//So peons can specifically find the TANK GOD and cast to him.  Accept no substitutes.
	racelaps = 1;
	statuscounter = 0;
	racetime = 0;
	HowitzerTimeToggle = 0;
	globalmultikill = 0;
	timesincegmkdec = 0;
}
bool EntityTankGod::Think(){
	EntityTankGodType *TP = (EntityTankGodType*)TypePtr;
	//Do godly stuff here.
	//
	//Global, everyone-hears-it multi-kill sound for major fraggage.

	// do not play multikill in training.
	if(!(VW->GameMode() & GAMEMODE_TRAINING))
	{
		timesincegmkdec += VW->FrameTime();
		if(timesincegmkdec > (TP->type_globalmultikilldecevery * 1000.0f)){
			timesincegmkdec = 0;
			if(globalmultikill > 0) globalmultikill--;
		}
		if(globalmultikill >= TP->type_globalmultikillthresh){
			timesincegmkdec = 0;
			globalmultikill = 0;
			VW->AddEntity("sound", TP->type_globalmultikillsound, NULL, NULL, NULL, 0, 0, 0, 0);
		}
	}
	//
	if( !(VW->GameMode() & GAMEMODE_NOCANDRIVE) ){
		racetime += VW->FrameTime();	//Authoritative total time since start of race.
	}
	//
	statuscounter -= VW->FrameTime();
	if(statuscounter < 0){
		BitPacker<8> pe;
		pe.PackUInt(MSG_HEAD_TANKGODSTATUS, 8);
		pe.PackUInt(racelaps, 8);
		QueuePacket(TM_Unreliable, pe, 1000.0f);
		statuscounter = 1000;
	}
	//
	//Find player (for later).
	EntityBase *tnk = FindRegisteredEntity("PlayerEntity");
	//Do mini-map.
	mm.ResetLines();
	mm.ResetPoints();
	//
	Target *t;	//Remove any links for tanks that aren't found.
	for(t = TgtHead.NextLink(); t; t = t->NextLink()){
		if(t->gid == 0 || !VW->GetEntity(t->gid)){
			t = t->PrevLink();
			if(t) t->NextLink()->DeleteItem();
		}else{	//Also do mini-map stuff.
			if(!(tnk && tnk->GID == t->gid) && t->targetok){	//Skip player's local tank.  And skip non-targets.
				if(t->teamid && tnk && tnk->QueryInt(ATT_TEAMID) == t->teamid){	//Same team.
					mm.AddPoint(t->pos[0], -t->pos[2], TP->type_mmlinew * 1.5f, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2], 0.2f);
				}else{
					mm.AddPoint(t->pos[0], -t->pos[2], TP->type_mmlinew * 1.5f, TP->type_mmecol[0], TP->type_mmecol[1], TP->type_mmecol[2], 0.2f);
				}
			}
		}
	}
	//
	PowerUp *p;	//Remove any links for PowerUps that aren't found, or are on a tank.
	for(p = PowHead.NextLink(); p; p = p->NextLink())
	{
		EntityBase *e  = VW->GetEntity(p->gid);
		if(e)
		{
			if(e->TypePtr->cname == "flag")
			{
				mm.AddPoint(p->pos[0], -p->pos[2], TP->type_mmlinew * 3.25f, TP->type_mmfcol[0], TP->type_mmfcol[1], TP->type_mmfcol[2], 0.3f);
			}
			else if(p->gid == 0 || e->QueryInt(ATT_WEAPON_STATUS))
			{
				p = p->PrevLink();
				if(p)
					p->NextLink()->DeleteItem();
			}
		}
		else
		{
			p = p->PrevLink();
			if(p)
				p->NextLink()->DeleteItem();
		}
	}
	//
	EntityWaypoint *ew = (EntityWaypoint*)FindRegisteredEntity("WaypointPath0");
	if(ew){
		WaypointNode *wn = ew->WaypointByIndex(0);
		WaypointNode *fwn = wn;
		for(; wn; wn = wn->NextLink()){
			WaypointNode *wnn = wn->NextLink();
			if(!wnn) wnn = fwn;
			if(VW->GameMode() & GAMEMODE_RACE){	//No lines in non-race.
				mm.AddLine(wn->Pos[0], -wn->Pos[2], wnn->Pos[0], -wnn->Pos[2], TP->type_mmlcol[0], TP->type_mmlcol[1], TP->type_mmlcol[2]);
			}
			if(wn->subid == 0){
				mm.AddPoint(wn->Pos[0], -wn->Pos[2], TP->type_mmlinew * 2.5f, TP->type_mmlcol[0], TP->type_mmlcol[1], TP->type_mmlcol[2], 0.0f);
			}
		}
	}
	//Do tanks as points.
	//
	mm.Opacity = TP->type_mmopac;
	mm.ClipRad = TP->type_mmcliprad;
	mm.ClipBias = TP->type_mmclipbias;
	mm.Scale = TP->type_mmscale;
	mm.X = TP->type_mmx;
	mm.Y = TP->type_mmy;
	mm.LineWidth = TP->type_mmlinew;
	if(tnk){
		mm.CenterX = tnk->Position[0];
		mm.CenterY = -tnk->Position[2];
		mm.Rot = -tnk->Rotation[1];
		//
		mm.AddPoint(tnk->Position[0], -tnk->Position[2], TP->type_mmlinew * 3.0f, TP->type_mmpcol[0], TP->type_mmpcol[1], TP->type_mmpcol[2], 0.8f);
		//
		//Do compass.
		float cx = tnk->Position[0] + sin((tnk->Rotation[1]) + PI * 0.5f) * TP->type_mmcliprad * 0.7f;
		float cy = (-tnk->Position[2]) - cos((tnk->Rotation[1]) + PI * 0.5f) * TP->type_mmcliprad * 0.7f;
		float s = TP->type_mmcliprad * 0.1f, s2 = s * 2.0f, s3 = s * 3.0f, sh = s * 0.5f;
		//Main arrows.
		mm.AddLine(cx, cy, cx, cy - s2, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(cx, cy - s2, cx - sh, cy - s, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(cx, cy - s2, cx + sh, cy - s, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(cx, cy, cx, cy + s2, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(cx, cy + s2, cx - sh, cy + s, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(cx, cy + s2, cx + sh, cy + s, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		//Cross bits.
		mm.AddLine(cx, cy, cx + s, cy, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(cx, cy, cx - s, cy, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
	//	mm.AddLine(cx, cy, cx, cy + s, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		//Letter N.
		float nx = cx - s, ny = cy - s * 1.5f, ns = s * 0.25f;
		mm.AddLine(nx - ns, ny + ns, nx - ns, ny - ns, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(nx - ns, ny - ns, nx + ns, ny + ns, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
		mm.AddLine(nx + ns, ny + ns, nx + ns, ny - ns, TP->type_mmtcol[0], TP->type_mmtcol[1], TP->type_mmtcol[2]);
	}
	//
	VW->PolyRend->AddOrthoObject(&mm);
	//
	if(tellrespawnflag && !(VW->GameMode() & GAMEMODE_SHOWSCORES) && !(VW->GameMode() & GAMEMODE_NOCANDRIVE)){
		EntityBase *e = VW->AddEntity("text", TP->type_statusfont, CVec3(0.5f, 0.3f, 20), CVec3(0.05f, 0.1f, 0), CVec3(1, 1, 1), 1, FLAG_TEXT_CENTERX, 0, 0);
		if(e) e->SetString(ATT_TEXT_STRING, Text_GetLine(TEXT_PRESSFIRE));	//Single frame life text entity.
		tellrespawnflag = 0;
	}
	if(VW->AreCratersUpdating()){
		EntityBase *e = VW->AddEntity("text", TP->type_statusfont, CVec3(0.5f, 0.7f, 2), CVec3(0.03f, 0.06f, 0), CVec3(1, 1, 1), 1, FLAG_TEXT_CENTERX, 0, 0);
		if(e) e->SetString(ATT_TEXT_STRING, Text_GetLine(TEXT_UPDATINGCRATERS));	//Single frame life text entity.
		e = VW->AddEntity("text", TP->type_statusfont, CVec3(0.5f, 0.75f, 2), CVec3(0.03f, 0.06f, 0), CVec3(1, 1, 1), 1, FLAG_TEXT_CENTERX, 0, 0);
		if(e) e->SetString(ATT_TEXT_STRING, Text_GetLine(TEXT_PLEASEWAIT));	//Single frame life text entity.
	}
	//
	return EntityGod::Think();
}
int EntityTankGod::UpdateTank(EntityBase *tank, int lap, int nextmainway, float distance, int frags, unsigned int teamid){	//Returns Place in race.
	if(!tank) return 0;
	Target *t;
	for(t = TgtHead.NextLink(); t; t = t->NextLink()) if(t->gid == tank->GID) break;
	if(!t){	//Not found in list.
		t = TgtHead.InsertObjectAfter(new Target);
	}
	if(t){
		t->gid = tank->GID;
		CopyVec3(tank->Position, t->pos);
		CopyVec3(tank->Rotation, t->rot);
		CopyVec3(tank->Velocity, t->vel);
		t->lap = lap;
		t->nextway = nextmainway;
		t->waydist = distance;
		t->health = tank->QueryFloat(ATT_HEALTH);
		t->frags = frags;
		t->deaths = tank->QueryInt(ATT_DEATHS);
		t->targetok = tank->QueryInt(ATT_TARGET_OK);	//This prevents tanks from being targetted when turret is blown off.
		t->teamid = teamid;
		t->Human = tank->QueryInt(ATT_PLAYERTANK) || !tank->QueryInt(ATT_AUTODRIVE);
		if(VW->Net.IsClientActive()){
			t->serverplace = tank->QueryInt(ATT_PLACE);	//If client, pull placing out of tank entity, which comes from server.  This will override ordering.
		}else{
			t->serverplace = 0;
		}
		if(lap > 0 && lap >= racelaps){	//Tank has finished the race.
			t->racetime = tank->QueryInt(ATT_RACETIME);
		}else{
			t->racetime = 0;
		}
		//Now sort into list.
		while(t->PrevLink() && t->PrevLink() != &TgtHead && *t > *(t->PrevLink())) t->ShiftItemUp();
		while(t->NextLink() && *t < *(t->NextLink())) t->ShiftItemDown();
		//And count place.
		int place = 1;
		while(t->PrevLink() && t->PrevLink() != &TgtHead){
			t = t->PrevLink();
			place++;
		}
		return place;
	}
	return 0;
}
int EntityTankGod::RemoveTank(EntityBase *tank){	//Removes you from update list.
	if(!tank) return 0;
	Target *t;
	for(t = TgtHead.NextLink(); t; t = t->NextLink()) if(t->gid == tank->GID) break;
	if(t){
		t->DeleteItem();
		return 1;
	}
	return 0;
}
EntityGID EntityTankGod::ClosestTank(Vec3 pos, EntityGID skipme, Vec3 dir, float rearbias, unsigned int skipteam, unsigned int fromteam){
	if(!pos) return 0;
	Vec3 ndir = {1, 0, 0}, vd;
	if(dir) NormVec3(dir, ndir);
	EntityGID gid = 0;
	float dist = 1000000000.0f;
	for(Target *t = TgtHead.NextLink(); t; t = t->NextLink()){
		SubVec3(t->pos, pos, vd);
		float d = LengthVec3(vd);
		ScaleVec3(vd, 1.0f / std::max(0.000001f, d));
		float dot = DotVec3(vd, ndir) * 0.5f + 0.5f;
		d = d * dot + d * rearbias * (1.0f - dot);	//Lerp between real distance and rear scaled distance based on dot product.
		if(d < dist && t->gid != skipme && t->targetok && (t->teamid != skipteam || skipteam == 0) &&
			(fromteam == 0 || t->teamid == fromteam) ){
			dist = d;
			gid = t->gid;
		}
	}
	return gid;
}

void EntityTankGod::KillTeam(const int TeamHash, const int Killer)
{
	for(Target *t = TgtHead.NextLink(); t; t = t->NextLink())
	{
		if(t->teamid == TeamHash)
		{
			EntityRacetank *tank = (EntityRacetank*)VW->GetEntity(t->gid);
			if(CmpLower(tank->TypePtr->cname, "racetank")) // make sure its really a racetank
			{
				tank->FlagDeath(Killer);
			}
		}
	}
}

EntityBase* EntityTankGod::ClosestFlag(const Vec3 Pos, const int MyTeam, const bool bWantMyTeam)
{
	float Distance = 9999999.0f;
	EntityBase	*Flag = NULL;
	PowerUp	*pow = PowHead.NextLink();

	if(CTankGame::Get().GetSettings()->TeamPlay && CTankGame::Get().GetSettings()->TeamScores && CTankGame::Get().GetNumTeams() > 1)
	{
		while (pow != NULL)
		{
			EntityBase *ent = VW->GetEntity(pow->gid);
			if(ent)
			{
				if(CmpLower(ent->TypePtr->InterrogateString("cname"), "flag"))
				{
					int iTeamIndex = -1;
					CTankGame::Get().TeamIndexFromHash(ent->QueryInt(ATT_TEAMID), &iTeamIndex);
					if(CTankGame::Get().GetNumTeams() > iTeamIndex) // team is active, go after their flag
					{
						if(ent->QueryInt(ATT_TEAMID) == MyTeam)
						{
							if(bWantMyTeam || ent->QueryInt(ATT_FLAG_DROPPED) || ent->QueryInt(ATT_WEAPON_STATUS)) // either we want our flag cause I have the enemy flag, our flag is on the ground, or someone is carrying our flag
								return ent;
							// else ignore our flag
						}
						else if(!bWantMyTeam) // other team's flag .. only if I don't want our flag
						{
							// check and see if the flag is being held
							int iFlagHolder = ent->QueryInt(ATT_WEAPON_STATUS);
							if(iFlagHolder)
							{
								EntityBase *FlagHolderEnt = VW->GetEntity(iFlagHolder);
								if(FlagHolderEnt)
								{
									if(FlagHolderEnt->QueryInt(ATT_TEAMID) != MyTeam)
										return ent;  // enemy has someone elses flag .. kill them before they score!
		//							(should only do this if enemy doesn't have our flag)
		//							(in the case where there are multiple flags, not doing this sends our allies off after another flag, when they should be defending the flag carrier)
		//							(a catch-22 ... for now, this will have to be sufficient)
		//							else
		//								return NULL; // our team has the flag .. kill anyone who gets close to us
								}
							}
							else // enemy flag is at the base or on the ground, go get it
							{
								Vec3 vd;
								SubVec3(Pos, ent->Position, vd);
								float dist = LengthVec3(vd);
								if(dist < Distance)
								{
									Distance = dist;
									Flag = ent;
								}
							}
						}
					}
				}
			}
			pow = pow->NextLink();
		}
	}
	return Flag;
}
EntityBase *EntityTankGod::TankByIndex(int n){
	Target *t = TgtHead.FindItem(n + 1);
	if(t){
		return VW->GetEntity(t->gid);
	}
	return NULL;
}
int EntityTankGod::UpdatePowerUp(EntityBase *powerup, int level){
	if(!powerup) return 0;
	PowerUp *t;
	for(t = PowHead.NextLink(); t; t = t->NextLink()) if(t->gid == powerup->GID) break;
	if(!t){	//Not found in list.
		t = PowHead.InsertObjectAfter(new PowerUp);
	}
	if(t){
		t->gid = powerup->GID;
		CopyVec3(powerup->Position, t->pos);
		t->level = level;
		return 1;
	}
	return 0;
}
EntityGID EntityTankGod::ClosestPowerUp(Vec3 pos, Vec3 dir, float rearbias){
	if(!pos) return 0;
	Vec3 ndir = {1, 0, 0}, vd;
	if(dir) NormVec3(dir, ndir);
	EntityGID gid = 0;
	float dist = 1000000000.0f;
	for(PowerUp *t = PowHead.NextLink(); t; t = t->NextLink()){
		SubVec3(t->pos, pos, vd);
		float d = LengthVec3(vd);
		ScaleVec3(vd, 1.0f / std::max(0.000001f, d));
		float dot = DotVec3(vd, ndir) * 0.5f + 0.5f;
		d = d * dot + d * rearbias * (1.0f - dot);	//Lerp between real distance and rear scaled distance based on dot product.
		if(d < dist){
			dist = d;
			gid = t->gid;
		}
	}
	return gid;
}
int EntityTankGod::QueryInt(int type){
	EntityTankGodType *TP = (EntityTankGodType*)TypePtr;
	if(type == ATT_INITIAL_PEASHOOTER_AMMO){
		if(VW->GameMode() & GAMEMODE_DEATHMATCH) return TP->type_dmpeashooter;
		if(VW->GameMode() & GAMEMODE_RACE) return TP->type_racepeashooter;
		return 0;
	}
	if(type == ATT_RACE_LAPS) return racelaps;
	if(type == ATT_RACETIME) return racetime;
	if(type == ATT_HOWITZERTIME_TOGGLE) return HowitzerTimeToggle;
	return EntityGod::QueryInt(type);
}
bool EntityTankGod::SetInt(int type, int attr){
	if(type == ATT_RACE_LAPS){
		racelaps = MAX(1, attr);
		return true;
	}
	if(type == ATT_CMD_STATUS_RESPAWN){
		tellrespawnflag = attr;
		return true;
	}
	if(type == ATT_FRAGMSG_KILLER){
		fragmsg_killer = (EntityGID)attr;
		return true;
	}
	if(type == ATT_FRAGMSG_WEAPON){
		fragmsg_weapon = (ClassHash)attr;
		return true;
	}
	if(type == ATT_HOWITZERTIME_TOGGLE){
		HowitzerTimeToggle = attr;
		return true;
	}
	if(type == ATT_FRAGMSG_KILLEE || type == ATT_FRAGMSG_KILLEE + 1){
		fragmsg_killee = (EntityGID)attr;
		if(VW->Net.IsServerActive()){
			BitPacker<32> pe;
			pe.PackInt(MSG_HEAD_FRAGMSG, 8);
			pe.PackUInt(fragmsg_killer, 32);
			pe.PackUInt(fragmsg_weapon, 32);
			pe.PackUInt(fragmsg_killee, 32);
			QueuePacket(TM_Reliable, pe);
		}else{
			if(VW->Net.IsClientActive()){
				if(type != ATT_FRAGMSG_KILLEE + 1) return 1;	//Plus1 means it was delivered from server, so it is OK to display on a client.
			}
		}
		if(VW->Net.IsClientActive() == false){
			globalmultikill++;	//Add a global kill to the counter, if on server or single player.
		}
		EntityBase *killer, *killee, *player;
		EntityTypeBase *weapon;
		killer = VW->GetEntity(fragmsg_killer);
		killee = VW->GetEntity(fragmsg_killee);
		player = VW->FindRegisteredEntity("PlayerEntity");
		CStr mkiller, mkillee, mweapon, frag;
		int pri = 0;
		char msg[4096];	// FIXME - check lengths of name segments

		if (killer) mkiller = killer->QueryString(ATT_NAME);
		if (killee) mkillee = killee->QueryString(ATT_NAME);
		if(fragmsg_weapon != -1) // a weapon or deathwish was used
		{
			weapon = VW->FindEntityType(fragmsg_weapon);
			if(weapon) {
				if (weapon->nameid != -1) {
					mweapon = Weapons.Get(weapon->nameid);
				} else {
					mweapon = weapon->dname;
				}
			}

			if (!killer) {
				if (killee == player) {
					pri = 4;
					sprintf(msg, Text_GetLine(TEXT_WORLDKILLPLAYER), mweapon.get());
				} else if (killee) {
					sprintf(msg, Text_GetLine(TEXT_WORLDKILL), mkillee.get(), mweapon.get());
				} else {
					// Can't happen
				}
			} else if (killer == player) {
				if (killee == player) {
					pri = 4;
					if (weapon) {
						sprintf(msg, Text_GetLine(TEXT_KILLSELF), mweapon.get());
					} else {
						sprintf(msg, Text_GetLine(TEXT_KILLSELFSUICIDE));
					}
				} else if (killee) {
					pri = 5;
					sprintf(msg, Text_GetLine(TEXT_KILLOTHER), mkillee.get(), mweapon.get());
				} else {
					// Can't happen
				}
			} else {
				// killer is not player or world
				if (killer == killee) {
					if (weapon) {
						sprintf(msg, Text_GetLine(TEXT_OTHERKILLSELF), mkiller.get(), mweapon.get());
					} else {
						sprintf(msg, Text_GetLine(TEXT_OTHERKILLSELFSUICIDE), mkiller.get());
					}
				} else if (killee == player) {
					pri = 4;
					sprintf(msg, Text_GetLine(TEXT_OTHERKILLPLAYER), mkiller.get(), mweapon.get());
				} else if (killee) {
					sprintf(msg, Text_GetLine(TEXT_OTHERKILLOTHER), mkiller.get(), mkillee.get(), mweapon.get());
				} else {
					// Can't happen
				}
			}
		}
		else // its a flag kill
		{
			sprintf(msg, Text_GetLine(TEXT_FLAGKILL), mkiller.get(), mkillee.get());
		}
		//
	//	SetString(ATT_STATUS_MESSAGE, msg);	//Get EntityGod parent class to display.
	//	SetInt(ATT_STATUS_PRIORITY, pri);
		VW->StatusMessage(msg, pri, CLIENTID_NONE);	//Do not forward to clients.
		//
		return true;
	}
	return EntityGod::SetInt(type, attr);
}
float EntityTankGod::QueryFloat(int type){
	return EntityGod::QueryFloat(type);
}
CStr EntityTankGod::QueryString(int type){
	return EntityGod::QueryString(type);
}
void EntityTankGod::DeliverPacket(const unsigned char *data, int len){
	if(data[0] == MSG_HEAD_FRAGMSG){	//Receive (on client) message from server god stating a frag has occured and needs to be textified.
		BitUnpackEngine pe(data, len);
		pe.SkipBits(8);
		pe.UnpackUInt(fragmsg_killer, 32);
		pe.UnpackUInt(fragmsg_weapon, 32);
		pe.UnpackUInt(fragmsg_killee, 32);
		SetInt(ATT_FRAGMSG_KILLER, fragmsg_killer);
		SetInt(ATT_FRAGMSG_WEAPON, fragmsg_weapon);
		SetInt(ATT_FRAGMSG_KILLEE + 1, fragmsg_killee);
		return;
	}
	if(data[0] == MSG_HEAD_TANKGODSTATUS){
		BitUnpackEngine pe(data, len);
		pe.SkipBits(8);
		pe.UnpackUInt(racelaps, 8);
		return;
	}
	EntityGod::DeliverPacket(data, len);
}



//******************************************************************
//**  Tree Entity  --  Derived Class
//******************************************************************
#define TREESMOKEVEL 10
#define TREEEXPLOVEL 20
EntityTreeType::EntityTreeType(ConfigFile *cfg, const char *c, const char *t) :
	EntitySpriteType(cfg, c, t){
//	type_trunk = 0.5f;
	type_trunk = 1.0f;
	type_explodefol = "smokebig";
	type_explokill = "firebig";
	if(cfg){
		if(cfg->FindKey("trunk")) cfg->GetFloatVal(&type_trunk);
		if(cfg->FindKey("explodefol")) cfg->GetStringVal(type_explodefol);
		if(cfg->FindKey("explokill")) cfg->GetStringVal(type_explokill);
	}
	if(type_restonground == 0){	//Force trees to rest on ground.
		type_restonground = 1;
		type_groundoffset = type_base - 1.0f;
	}
	Transitory = false;
}
bool EntityTreeType::CacheResources(){
	if(!ResCached){
		if(EntitySpriteType::CacheResources()){
			VW->CacheResources("explo", type_explodefol);
			VW->CacheResources("explo", type_explokill);
		}
	}
	return ResCached;
}
void EntityTreeType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	EntitySpriteType::ListResources(fl);
	VW->ListResources("explo", type_explodefol);
	VW->ListResources("explo", type_explokill);
}
EntityBase *EntityTreeType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityTree(this, Pos, Rot, Vel, id, flags);
}
EntityTree::EntityTree(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntitySprite(et, Pos, Rot, Vel, id, flags) {
	EntityTreeType *ET = (EntityTreeType*)et;
//	Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + ET->type_base - 1.0f;
	//Handled by EntitySprite's TerrainModified handler now.
	frame = id;
	CanCollide = true;
	BoundRad = ET->type_trunk;
	BoundMin[0] = -BoundRad;  BoundMin[1] = -ET->type_base;  BoundMin[2] = -BoundRad;
	BoundMax[0] = BoundRad;  BoundMax[1] = ET->type_h - ET->type_base;  BoundMax[2] = BoundRad;
	mssincesmoke = 0;
}
bool EntityTree::CollisionWith(EntityBase *collider, Vec3 colpnt){
	EntityTreeType *ET = (EntityTreeType*)TypePtr;
	if(LengthVec3(collider->Velocity) > TREEEXPLOVEL){
		VW->AddEntity("explo", ET->type_explokill, Position, NULL, NULL, 0, NULL, 0, ADDENTITY_FORCENET);
		Remove();
		return true;
	}else{
		if(LengthVec3(collider->Velocity) > TREESMOKEVEL && abs(mssincesmoke) > 500){
			VW->AddEntity("explo", ET->type_explodefol, Position, NULL, NULL, 0, NULL, 0, ADDENTITY_FORCENET);
			frame = std::max(0, frame - 1);
			mssincesmoke = 0;
		}
	}
	return false;
}
bool EntityTree::Think(){
	mssincesmoke += VW->FrameTime();
	return EntitySprite::Think();
}
//******************************************************************
//**  Smoke Entity  --  Derived Class
//******************************************************************
EntitySmokeType::EntitySmokeType(ConfigFile *cfg, const char *c, const char *t) :
	EntitySpriteType(cfg, c, t){
	fadestart = 1.0f;
	type_lifespan = 1.0f;
	type_grow = 1.0f;	//Expand by 100% through lifespan.
	SetVec3(0, 2.0f, 0, type_defaultvel);
	type_gravity = 0.0f;
	type_randomrotate = 1;
	Transitory = true;	//We have time to live, so we are transitory.
	if(cfg){
		if(cfg->FindKey("fade")) cfg->GetFloatVal(&fadestart);
		if(cfg->FindKey("lifespan")) cfg->GetFloatVal(&type_lifespan);
			if(cfg->FindKey("timetolive")) cfg->GetFloatVal(&type_lifespan);
		if(cfg->FindKey("grow")) cfg->GetFloatVal(&type_grow);
		if(cfg->FindKey("DefaultVel")) cfg->GetFloatVals(&type_defaultvel[0], 3);
		if(cfg->FindKey("gravity")) cfg->GetFloatVal(&type_gravity);
		if(cfg->FindKey("Transitory")) cfg->GetBoolVal(&Transitory);
		if(cfg->FindKey("randomrotate")) cfg->GetIntVal(&type_randomrotate);
	}
}
EntityBase *EntitySmokeType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntitySmoke(this, Pos, Rot, Vel, id, flags);
}
bool EntitySmokeType::CacheResources(){
	if(EntitySpriteType::CacheResources()){
	//	texture->AlphaGrad = true;	//Ask for gradiated alpha.
		//No, set this in .ent file through sprite now.
	}
	return ResCached;
}
void EntitySmokeType::ListResources(FileCRCList *fl){
	EntitySpriteType::ListResources(fl);
}
EntitySmoke::EntitySmoke(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntitySprite(et, Pos, Rot, Vel, id, flags) {
	EntitySmokeType *TP = (EntitySmokeType*)TypePtr;
	timeofbirth = VW->Time();
	if(id == 0){	//Set defaults.
		timetodie = VW->Time() + (int)(TP->type_lifespan * 1000.0f);
	}else{
		timetodie = VW->Time() + id;//(id == 0 ? 1000 : id);
	}
	if(!Vel){
	//	Velocity[1] = 2.0f;
		CopyVec3(TP->type_defaultvel, Velocity);
	}
	CanCollide = false;
	//
	if(TP->type_randomrotate) spriteobj.IntRot = TMrandom();	//Give random rotation to sprite.
	else spriteobj.IntRot = 0;
}
bool EntitySmoke::Think(){
	EntitySmokeType *TP = (EntitySmokeType*)TypePtr;
	if(!TP->texture) return false;
	if(!Active) return false;
	if(VW->Time() > timetodie) Remove();
	//
	if(TP->type_gravity != 0.0f){	//Physics.
		Vec3 accel;
		ScaleVec3(VW->gravity, TP->type_gravity, accel);
		float t = VW->FrameFrac();
		for(int i = 0; i < 3; i++){
			Position[i] += Velocity[i] * t + accel[i] * t * t * 0.5f;	//Physics.
			Velocity[i] += accel[i] * t;// * t;
		}
	}else{
		ScaleAddVec3(Velocity, VW->FrameFrac(), Position);
	}
	//
	float t = (float)(VW->Time() - timeofbirth) / (float)(timetodie - timeofbirth);
	frame = (int)((float)TP->texture->Images() * std::min(t, 0.99f));
	w = TP->type_w * (t * TP->type_grow + 1.0f);
	h = TP->type_h * (t * TP->type_grow + 1.0f);
//	VW->PolyRend->AddSprite(&(*(TP->texture))[frame], w, h, Position, false,
//		TP->rendflags, TP->fadestart * (1.0f - t));//1.0f - (t * 0.7f));//(GID & 1 ? SPRITE_BLEND_HALF : SPRITE_BLEND_ADD));
	fade = TP->fadestart * Bias(TP->type_fadebias, 1.0f - t);
	return EntitySprite::Think();
//	return true;
}
//******************************************************************
//**  Explo Entity  --  Derived Class
//******************************************************************
//#define FSEED(seed) (srand(seed))
#define FRAND (((float)TMrandom() - (float)(RAND_MAX / 2)) / (float)(RAND_MAX / 2))
EntityExploType::EntityExploType(ConfigFile *cfg, const char *c, const char *t) :
	EntitySmokeType(cfg, c, t), MixinEntityChain(cfg, c) {
	type_count = 16;
	type_expand = 10;
	type_expandstart = 2;
	type_flyingtrailmode = 0;
	randveladd = 0;
	ClearVec3(type_randveladdmin);
	ClearVec3(type_randveladdmax);
	type_flyingtrailtime = 0.075f;
	type_timewarp = 0.0f;	//This only applies for Flying Trail explos, and starts the trail this many seconds into the future (i.e. starts it already moved some amount).
	if(cfg){
		if(cfg->FindKey("count")) cfg->GetIntVal(&type_count);
		if(cfg->FindKey("expand")) cfg->GetFloatVal(&type_expand);
		if(cfg->FindKey("expandstart")) cfg->GetFloatVal(&type_expandstart);
		if(cfg->FindKey("soundentity")) cfg->GetStringVal(type_soundenttype);
		if(cfg->FindKey("flyingtrailmode")) cfg->GetIntVal(&type_flyingtrailmode);
		if(cfg->FindKey("flyingtrailtime")) cfg->GetFloatVal(&type_flyingtrailtime);
		if(cfg->FindKey("timewarp")) cfg->GetFloatVal(&type_timewarp);
		if(cfg->FindKey("randveladdmin")){
			cfg->GetFloatVals(&type_randveladdmin[0], 3);
			randveladd = 1;
		}
		if(cfg->FindKey("randveladdmax")){
			cfg->GetFloatVals(&type_randveladdmax[0], 3);
			randveladd = 1;
		}
	}
}
EntityBase *EntityExploType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityExplo(this, Pos, Rot, Vel, id, flags);
}
bool EntityExploType::CacheResources(){
	if(!ResCached){
		if(EntitySmokeType::CacheResources()){
			if(type_soundenttype.len() > 0) VW->CacheResources("sound", type_soundenttype);
			MixinEntityChain::CacheResources(VW);
		}
	}
	return ResCached;
}
void EntityExploType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	EntitySmokeType::ListResources(fl);
	if(type_soundenttype.len() > 0) VW->ListResources("sound", type_soundenttype);
	MixinEntityChain::ListResources(VW);
}

//Main code.
EntityExplo::EntityExplo(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntitySmoke(et, Pos, Rot, Vel, id, flags) {
	EntityExploType *TP = (EntityExploType*)TypePtr;
	count = std::max(1, std::min(1024, TP->type_count));//TP->type_count;
	if(TP->type_soundenttype.len() > 0 && !(TP->type_flyingtrailmode && flags & ENTITYFLAG_NOCHAIN)){	//Don't make sound if we've been chained and are flying trail.
		VW->AddEntity("sound", TP->type_soundenttype, Position, NULL, NULL, 0, 0, 0, 0);
	}
	//
	TP->SpawnChains(this);
	//
	if(TP->randveladd){
		for(int i = 0; i < 3; i++){
			Velocity[i] += TP->type_randveladdmin[i] +
				(TP->type_randveladdmax[i] - TP->type_randveladdmin[i]) * ((float)TMrandom() / (float)RAND_MAX);
		}
	}
	spriteobjs = new ExploSpriteObject[count];
	if(spriteobjs){
		for(int i = 0; i < count; i++){
			if(TP->type_flyingtrailmode == 0){
				SetVec3(FRAND, FRAND, FRAND, spriteobjs[i].ExploOffset);	//Don't bother with rands if we're a flying trail.
			}else{
				CopyVec3(Velocity, spriteobjs[i].ExploOffset);	//Cheat and use this to store particle velocity.
				CopyVec3(Position, spriteobjs[i].Pos);	//Prime position for flyingtrail mode.
			}
			if(TP->type_randomrotate) spriteobjs[i].IntRot = TMrandom();
			else spriteobjs[i].IntRot = 0;
			spriteobjs[i].Texture = NULL;
		}
	}
	//
	nextflyingtrail = 0;
	flyingtrailms = 0;
	timewarpms = 0;
}
EntityExplo::~EntityExplo(){
	if(spriteobjs) delete [] spriteobjs;
}
bool EntityExplo::Think(){
	EntityExploType *TP = (EntityExploType*)TypePtr;
	if(!TP->texture || !Active || !spriteobjs) return false;
//	if(!Active) return false;
	if(VW->Time() > timetodie) Remove();
	//
//	ScaleAddVec3(Velocity, VW->FrameFrac(), Position);
	if(TP->type_gravity != 0.0f){	//Physics.
		Vec3 accel;
		ScaleVec3(VW->gravity, TP->type_gravity, accel);
		float t = VW->FrameFrac(), tt5 = t * t * 0.5f;
		for(int i = 0; i < 3; i++){
			Position[i] += Velocity[i] * t + accel[i] * tt5;	//Physics.
			Velocity[i] += accel[i] * t;// * t;
		}
	}else{
		ScaleAddVec3(Velocity, VW->FrameFrac(), Position);
	}
	//
	float t = (float)(VW->Time() - timeofbirth) / (float)(timetodie - timeofbirth);
	frame = (int)((float)TP->texture->Images() * std::min(t, 0.99f));
	w = TP->type_w * (t * TP->type_grow + 1.0f);
	h = TP->type_h * (t * TP->type_grow + 1.0f);
	//
//	FSEED(GID);	//Fix this to use a table!
	//
	float s = (TP->type_expand - TP->type_expandstart) * t + TP->type_expandstart;
	float fade = TP->fadestart * Bias(TP->type_fadebias, (1.0f - t));
	if(TP->type_flyingtrailmode){
		int loop = 1;
		while(loop){
			float framefrac = VW->FrameFrac();
			int frametime = VW->FrameTime();
			if(TP->type_timewarp > 0.0f && timewarpms < (int)(TP->type_timewarp * 1000.0f)){
				loop = 1;
				framefrac = MAX(0.05, framefrac);
				frametime = MAX(50, frametime);
				timewarpms += frametime;
			}else loop = 0;
			flyingtrailms += frametime;//VW->FrameTime();
			int tms = (TP->type_flyingtrailtime * 1000.0f);
			if(nextflyingtrail < count && (flyingtrailms >= tms || flyingtrailms < 0)){
				spriteobjs[nextflyingtrail].Configure(&(*(TP->texture))[frame],
					spriteobjs[nextflyingtrail].Pos, w, h, TP->type_rendflags, fade);
				flyingtrailms -= tms;
				nextflyingtrail++;
			}
			float fadestep = fade / (float)std::max(1, count);
			float fadestart = fade;
			Vec3 accel;
			ScaleVec3(VW->gravity, TP->type_gravity, accel);
			float t = framefrac;//VW->FrameFrac();
			float tt5 = t * t * 0.5f;
			Vec3 acceltt5 = {accel[0] * tt5, accel[1] * tt5, accel[2] * tt5};	//Pre-calculate variables that will be the same for all trail particles.
			Vec3 accelt = {accel[0] * t, accel[1] * t, accel[2] * t};
			for(int n = 0; n < count; n++){
				if(spriteobjs[n].Texture){	//Physics for trail bits.
					ScaleAddVec3(spriteobjs[n].ExploOffset, t, spriteobjs[n].Pos);
					AddVec3(acceltt5, spriteobjs[n].Pos);
					AddVec3(accelt, spriteobjs[n].ExploOffset);
					spriteobjs[n].Opacity = fadestart;
					if(loop == 0) VW->PolyRend->AddTransObject(&spriteobjs[n]);	//Only add on last pass through timewarp loop.
				}
				fadestart -= fadestep;
			}
		}
	}else{
		for(int i = 0; i < count; i++){
			Vec3 tpos;// = {FRAND, FRAND, FRAND};
			CopyVec3(spriteobjs[i].ExploOffset, tpos);
			//Trying with this disabled, now that keeping puffs above ground seems to look good.
		//	if(Flags & FLAG_EXPLO_UP) tpos[1] = fabsf(tpos[1]);
			ScaleAddVec3(tpos, s, Position, tpos);
			float y = VW->Map.FGetI(tpos[0], -tpos[2]) + TP->type_base;
			tpos[1] = std::max(tpos[1], y);	//Try forcing explo puffs above ground.  :)
			spriteobjs[i].Configure(&(*(TP->texture))[frame], tpos, w, h, TP->type_rendflags, fade);
			VW->PolyRend->AddTransObject(&spriteobjs[i]);
		}
	}
	return true;
}
//******************************************************************
//**  ExploSphere Entity  --  Derived Class
//******************************************************************
EntityExploSphereType::EntityExploSphereType(ConfigFile *cfg, const char *c, const char *t) :
EntityMeshType(cfg, c, t) {//, MixinEntityChain(cfg, c) {
	type_expand = 10;
	type_expandstart = 2;
	type_lifespan = 1.0f;
	type_rotatespeed = 0.0f;
	type_rotatespeedz = 0.0f;
//	rendflags |= MESH_SHADE_SOLID;// | MESH_ENVMAP_SMOOTH;
	if(cfg){
		if(cfg->FindKey("expand")) cfg->GetFloatVal(&type_expand);
		if(cfg->FindKey("expandstart")) cfg->GetFloatVal(&type_expandstart);
		if(cfg->FindKey("soundentity")) cfg->GetStringVal(type_soundenttype);
		if(cfg->FindKey("lifespan")) cfg->GetFloatVal(&type_lifespan);
		if(cfg->FindKey("rotatespeed")) cfg->GetFloatVal(&type_rotatespeed);
		if(cfg->FindKey("rotatespeedz")) cfg->GetFloatVal(&type_rotatespeedz);
	}
	Transitory = true;
}
EntityBase *EntityExploSphereType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityExploSphere(this, Pos, Rot, Vel, id, flags);
}
bool EntityExploSphereType::CacheResources(){
	if(!ResCached){
		if(EntityMeshType::CacheResources()){
			if(type_soundenttype.len() > 0) VW->CacheResources("sound", type_soundenttype);
		//	MixinEntityChain::CacheResources(VW);
		}
	}
	return ResCached;
}
void EntityExploSphereType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	EntityMeshType::ListResources(fl);
	if(type_soundenttype.len() > 0) VW->ListResources("sound", type_soundenttype);
}
//Main code.
EntityExploSphere::EntityExploSphere(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntityExploSphereType *TP = (EntityExploSphereType*)TypePtr;
	timeofbirth = VW->Time();
	if(id == 0) timetodie = VW->Time() + (int)(TP->type_lifespan * 1000.0f);
		else timetodie = VW->Time() + id;
	if(TP->type_soundenttype.len() > 0) VW->AddEntity("sound", TP->type_soundenttype, Position, NULL, NULL, 0, 0, 0, 0);
//	if(TP->mesh) TP->mesh->BndRad = std::max(TP->type_expand, TP->mesh->BndRad);
	//The above is a hack...  All ents using this mesh will have a culling bounding radius
	//equal to the largest.  Bad for e.g. nukes...
	CopyVec3(Rotation, OriRotation);
//	TP->SpawnChains(this);
}

bool EntityExploSphere::Think(){
	EntityExploSphereType *TP = (EntityExploSphereType*)TypePtr;
	if(!TP->texture || !TP->mesh) return false;
	if(VW->Time() > timetodie) RemoveMe = REMOVE_NORMAL;
	ScaleAddVec3(Velocity, VW->FrameFrac(), Position);
	float t = (float)(VW->Time() - timeofbirth) / (float)(timetodie - timeofbirth);
	float s = (TP->type_expand - TP->type_expandstart) * t + TP->type_expandstart;
	CopyVec3(OriRotation, Rotation);
//	double r = (VW->Time() & 1023) / 1024.0f * PI2;
	double r = ((double)VW->Time() / 1000.0) * PI2;
//	float r = VW->FrameFrac() * PI2;
	Rotation[1] += fmod(r * (double)TP->type_rotatespeed, double(PI2));
	Rotation[2] += fmod(r * (double)TP->type_rotatespeedz, double(PI2));
	Mat43 mat;// = {{s, 0, 0}, {0, s, 0}, {0, 0, s}, {Position[0], Position[1], Position[2]}};	//Assumes explo object sized at 1 meter by default.
	Rot3ToMat3(Rotation, mat);
	for(int i = 0; i < 3; i++) ScaleVec3(mat[i], s);
	if(TP->type_scalematrixon){
		ScaleVec3(mat[0], TP->type_scalematrix[0]);
		ScaleVec3(mat[1], TP->type_scalematrix[1]);
		ScaleVec3(mat[2], TP->type_scalematrix[2]);
		s = std::max(std::max(TP->type_scalematrix[0], TP->type_scalematrix[1]), TP->type_scalematrix[2]);
		//Expand bounding radius by proper amount so lasers show up when fired from behind.
	}
	CopyVec3(Position, mat[3]);
	meshobj.Configure(TP->texture, TP->mesh, mat, mat[3], TP->type_rendflags | MESH_SCALE_MATRIX | MESH_ENVBASIS_MODEL,
		TP->type_fade * Bias(TP->type_fadebias, (1.0f - t)), s * TP->mesh->BndRad, TP->type_lodbias);
	VW->PolyRend->AddTransObject(&meshobj);
//	if(TP->mesh) VW->PolyRend->AddMeshMat(TP->mesh, mat, false, TP->rendflags | MESH_SCALE_MATRIX | MESH_ENVBASIS_MODEL,
//		TP->type_fade * (1.0f - t));
	return true;
}

//******************************************************************
//**  Crud Entity  --  Straight from base.
//******************************************************************
EntityCrudType::EntityCrudType(ConfigFile *cfg, const char *c, const char *t) : EntitySpriteType(cfg, c, t) {
	type_endfade = 0.0f;
	type_colorfade = 1.0f;
	type_usegroundcolor = 1;
	type_gravity = 1.0f;
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("startrad")) cfg->GetFloatVal(&type_startrad);
		if(cfg->FindKey("endrad")) cfg->GetFloatVal(&type_endrad);
		if(cfg->FindKey("timetolive")) cfg->GetIntVal(&type_timetolive);
		if(cfg->FindKey("particles")) cfg->GetIntVal(&type_particles);
		if(cfg->FindKey("endfade")) cfg->GetFloatVal(&type_endfade);
		if(cfg->FindKey("colorfade")) cfg->GetFloatVal(&type_colorfade);
		if(cfg->FindKey("usegroundcolor")) cfg->GetIntVal(&type_usegroundcolor);
		if(cfg->FindKey("gravity")) cfg->GetFloatVal(&type_gravity);
	}
	Transitory = true;
}
EntityBase *EntityCrudType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityCrud(this, Pos, Rot, Vel, id, flags);
}
bool EntityCrudType::CacheResources(){
	if(EntitySpriteType::CacheResources()){
	//	texture->AlphaGrad = true;	//Ask for gradiated alpha.
	}
	return ResCached;
//	return EntitySpriteType::CacheResources();//ResCached = true;
}
void EntityCrudType::ListResources(FileCRCList *fl){
	EntitySpriteType::ListResources(fl);
}
void EntityCrudType::UnlinkResources(){
	EntitySpriteType::UnlinkResources();//ResCached = false;
}
//Entity code.
EntityCrud::EntityCrud(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntitySprite(et, Pos, Rot, Vel, id, flags) {
	EntityCrudType *ET =(EntityCrudType*)et;
	CanCollide = false;
	if(ET->type_usegroundcolor == 0){
		r = g = b = 255;
	}else{
		Position[1] += ET->type_startrad;
		VW->Map.GetRGB(Position[0], -Position[2], &r, &g, &b);
		if(VW->Map.GetHwrap(Position[0], -Position[2]) > WATERHEIGHTRAW){
			float add = (TMrandom() % 16);
			r = std::min(255.0f, (float)r * ET->type_colorfade + add);// - 32);
			g = std::min(255.0f, (float)g * ET->type_colorfade + add);// - 32);	//Darken slightly.  Er, lighten.
			b = std::min(255.0f, (float)b * ET->type_colorfade + add);// - 32);
		}else{	//Assume water, and brighten.
			r = 255;//std::min(255, r <<2);
			g = 255;//std::min(255, g <<2);
			b = 255;//std::min(255, b <<2);
		}
	}
	timeofbirth = VW->Time();
	timetodie = timeofbirth + (id ? id : ET->type_timetolive);
	particles = (flags & 255 ? flags & 255 : ET->type_particles);
	particleobj.IntRot = TMrandom();	//Give random rotation to particles.
}
//bool EntityCrud::SetInt(int type, int val){
//	return false;
//}
//int EntityCrud::QueryInt(int type){
//	return 0;
//}
bool EntityCrud::Think(){
	EntityCrudType *ET =(EntityCrudType*)TypePtr;
	if(VW->Time() > timetodie) Remove();
//	ScaleAddVec3(Velocity, VW->FrameFrac(), Position);
	float t = (float)(VW->Time() - timeofbirth) / (float)(timetodie - timeofbirth);
	float time = (float)(VW->Time() - timeofbirth) * 0.001;
	Vec3 p = {//Position[0] + Velocity[0] * time,
		Position[0] + (Velocity[0] * time + VW->gravity[0] * ET->type_gravity * time * time * 0.5f),	//Gravity!
		Position[1] + (Velocity[1] * time + VW->gravity[1] * ET->type_gravity * time * time * 0.5f),	//Gravity!
		Position[2] + (Velocity[2] * time + VW->gravity[2] * ET->type_gravity * time * time * 0.5f)};	//Gravity!
	//	Position[2] + Velocity[2] * time};
//	frame = (int)((float)TP->texture->Images() * std::min(t, 0.99f));
	//
	float tfade = Bias(ET->type_fadebias, 1.0f - t);
	particleobj.Configure(r, g, b, GID <<4, p, w, h, ET->type_startrad * (1.0f - t) + ET->type_endrad * t,
		particles, ET->type_rendflags, ET->type_fade * tfade + ET->type_endfade * (1.0f - tfade), ET->texture);
	VW->PolyRend->AddTransObject(&particleobj);
//	VW->PolyRend->AddParticle(r, g, b, GID, p,
//		ET->type_startrad * (1.0f - t) + ET->type_endrad * t, particles, 1.0f - (t * 0.6f));
	return true;
}

//******************************************************************
//**  Course Entity  --  derived from Waypoint.
//******************************************************************
EntityCourseType::EntityCourseType(ConfigFile *cfg, const char *c, const char *t) : EntityWaypointType(cfg, c, t) {
	type_pushstartpoints = 0.0f;
	type_nextweapondelay = 0.0f;
	type_teamspawntype = 0; // all teams
	ReadCfg(cfg);
}
void EntityCourseType::ReadCfg(ConfigFile *cfg){
	if(cfg){
		if(cfg->FindKey("HitRadius")) cfg->GetFloatVal(&type_hitradius);	//This is so hitradius can be specified in map config strings.
		if(cfg->FindKey("HitRadiusAdd")) cfg->GetFloatVal(&type_hitradiusadd);	//This is so hitradius can be specified in map config strings.
		if(cfg->FindKey("nextweapondelay")) cfg->GetFloatVal(&type_nextweapondelay);
		if(cfg->FindKey("weaponspawnentity")) cfg->GetStringVal(type_weaponspawnentity);
		if(cfg->FindKey("CheckPointEntity ")) cfg->GetStringVal(type_checkpointentity);
		if(cfg->FindKey("StartPointEntity ")) cfg->GetStringVal(type_startpointentity);
		if(cfg->FindKey("pushstartpoints")) cfg->GetFloatVal(&type_pushstartpoints);
		if(cfg->FindKey("AlwaysSpawnEntityClass")) cfg->GetStringVal(type_alwaysspawnentityclass);
		if(cfg->FindKey("AlwaysSpawnEntityType")) cfg->GetStringVal(type_alwaysspawnentitytype);
		if(cfg->FindKey("TeamSpawnType"))
			cfg->GetIntVal(&type_teamspawntype);
	}
}
EntityBase *EntityCourseType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	//Hey cool, entities are always created through their EntityType, so I can put
	//pre-construction doodad code here!!
	Vec3 p = {Pos[0], VW->Map.FGetI(Pos[0], -Pos[2]), Pos[2]};	//Rest on ground.
	return new EntityCourse(this, p, Rot, Vel, id, flags);
}
bool EntityCourseType::CacheResources(){
	if(!ResCached){
		ResCached = true;
		ReadCfg(&VW->MapCfg);
		VW->CacheResources("mesh", type_checkpointentity);
		VW->CacheResources("mesh", type_startpointentity);
		for(EntityTypeBase *et = VW->EntTypeHead.NextLink(); et; et = et->NextLink()){
			if(et->InterrogateInt("powerup-family")){
				et->CacheResources();	//Let's make it official, we're gonna be spawning 'em all anyway...
			}
		}
	}
	return ResCached = true;
}
void EntityCourseType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	ResListed = true;
	VW->ListResources("mesh", type_checkpointentity);
	VW->ListResources("mesh", type_startpointentity);
	for(EntityTypeBase *et = VW->EntTypeHead.NextLink(); et; et = et->NextLink()){
		if(et->InterrogateInt("powerup-family")){
			et->ListResources(fl);	//Let's make it official, we're gonna be spawning 'em all anyway...
		}
	}
}

//Main code.
EntityCourse::EntityCourse(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
					int id, int flags) : EntityWaypoint(et, Pos, Rot, Vel, id, flags, ((EntityCourseType*)et)->type_teamspawntype) {
	if(VW->Net.IsClientActive()){	//If we are a client, do not add checkposts, as server will be sending them as ents.
		preprocessed = true;
		CanThink = false;
	}else{
		preprocessed = false;
		CanThink = true;
	}
	cylindrical = true;
	master = false;
}

CStr EntityCourseType::InterrogateString(const char *thing)
{
	if(thing && CmpLower(thing, "alwaysspawnentityclass")) return type_alwaysspawnentityclass;
	if(thing && CmpLower(thing, "alwaysspawnentitytype")) return type_alwaysspawnentitytype;
	return CStr("");
}

bool EntityCourse::Think(){
	EntityCourseType *TP = (EntityCourseType*)TypePtr;
	if(master){	//Do powerup spawn stuff.
		int numpowerups = powenumhead.CountItems(-1);
		for(PowerUpSpawnPoint *pow = powhead.NextLink(); pow; pow = pow->NextLink()){	//Iterate spawn points.
			EntityBase *e = VW->GetEntity(pow->entity);
			if(!pow->entity || !e || e->QueryInt(ATT_WEAPON_STATUS)){	//No ent, or ent on tank.
				if(pow->state){
					pow->elapsedtime = 0;
					pow->state = 0;
				}
				if(pow->elapsedtime >= (TP->type_nextweapondelay * 1000.0f))
				{
					if (pow->SpawnType != NULL)
					{
						if(!pow->SpawnType->InterrogateInt("unique") || !pow->entity || !e) // if it isn't unique or the ent doesn't exist, spawn it
						{
							e = VW->AddEntity(pow->SpawnType, pow->Pos, NULL, NULL, 0, 0, 0, 0);
							if (e)
							{
								if (CmpLower(pow->SpawnType->cname, "racetank"))
								{
									((EntityRacetank*)e)->FixedSpawn = 1;
									((EntityRacetank*)e)->SetString(ATT_NAME, Text_GetLine(TEXT_NAMEDUMMY));
									((EntityRacetank*)e)->Immovable = 0;
								}
/*								if (CmpLower(pow->SpawnType->cname, "target"))
								{
									((EntityTarget*)e)->FixedSpawn = 1;
									((EntityTarget*)e)->SetString(ATT_NAME, "Target");
									((EntityTarget*)e)->SetString(ATT_NAME, Text_GetLine(TEXT_NAMETARGET));
								}
*/								e->CacheResources();
							}
						}
					}
					else
					{
						PowerUpTypeEnum *pt = powenumhead.NextLink();
						//Find appropriate power-up based on totalled probability weights.
						int rn = TMrandom() % total_weights, n = 0;
						while(pt && n + pt->weight <= rn){
							n += pt->weight;
							pt = pt->NextLink();
						}
						if(pt){
							e = VW->AddEntity(pt->type, pow->Pos, NULL, NULL, 0, 0, 0, 0);
						}
					}

					if(e){
						pow->entity = e->GID;
						pow->state = 1;
						if(TP->type_weaponspawnentity.len() > 0){
							VW->AddEntity("explosphere", TP->type_weaponspawnentity, e->Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
						}
					}
					pow->elapsedtime = 0;
				}
			}
			pow->elapsedtime += VW->FrameTime();
		}
		return true;
	}
	if(!preprocessed && wayhead.NextLink()){	//As intermediate waypoint entities will be sticking around now, only preprocess if we are the master (and have waypoint list).
		//
		//Enumerate PowerUp and Weapon classes available to create.
		total_weights = 0;
		for(EntityTypeBase *et = VW->EntTypeHead.NextLink(); et; et = et->NextLink()){
			if(et->InterrogateInt("powerup-family")){
				PowerUpTypeEnum *pte = powenumhead.AddObject(new PowerUpTypeEnum(et));
				if(pte){
					total_weights += pte->weight;	//Count total of all integer propability weights.
				}
			}
		}
		//
		WaypointNode *w = wayhead.NextLink(), *wp, *wn;
		while(w){
			if((wp = w->PrevLink()) == &wayhead) wp = wayhead.Tail();
			if((wn = w->NextLink()) == NULL) wn = wayhead.NextLink();
			Vec3 t1, t2;
			SubVec3(w->Pos, wp->Pos, t1);
			NormVec3(t1);
			SubVec3(wn->Pos, w->Pos, t2);
			NormVec3(t2);
			AddVec3(t1, t2, w->Vel);
			NormVec3(w->Vel);
			//
			w->suggestedradius = TP->type_hitradius * DotVec3(w->Vel, t1);
			//
			//Add one standard waypost entity for the time being.  Add some way to
			//specify variables like this in the map file itself.
			if(w->subid == 0){
				//
				PowerUpSpawnPoint *pow = powhead.AddObject(new PowerUpSpawnPoint);
				if(pow){
					CopyVec3(w->Pos, pow->Pos);	//Construct list of power-up spawn points.
					pow->SpawnType = VW->FindEntityType(w->type->InterrogateString("alwaysspawnentityclass"), w->type->InterrogateString("alwaysspawnentitytype"));
				}
				CStr posttype = TP->type_checkpointentity;
				float pushpost = 0.0f;
				if(w->mainid == 0){	//Starting line.
					posttype = TP->type_startpointentity;
					pushpost = TP->type_pushstartpoints;
				}
				Rot3 rots = {0, atan2f(w->Vel[0], w->Vel[2]) + PI, 0};
				//
				if(VW->GameMode() & GAMEMODE_RACE){	//Only set posts for race.
					float x1 = w->Pos[0], y1 = w->Pos[2], x, y;
					x = x1 + -w->Vel[2] * (TP->type_hitradius + pushpost);
					y = y1 + w->Vel[0] * (TP->type_hitradius + pushpost);
					VW->AddEntity("mesh", posttype, CVec3(x, VW->Map.FGetI(x, -y) - 2.0f, y), rots, NULL, 0, 0, 0, 0);
					x = x1 + w->Vel[2] * (TP->type_hitradius + pushpost);
					y = y1 + -w->Vel[0] * (TP->type_hitradius + pushpost);
					VW->AddEntity("mesh", posttype, CVec3(x, VW->Map.FGetI(x, -y) - 2.0f, y), rots, NULL, 0, 0, 0, 0);
				}
			}
			//
			w = w->NextLink();
		}
		preprocessed = true;
		master = true;
		OutputDebugLog("Preprocessed waypoints.\n");
		return true;
	}
	CanThink = false;
	return true;
}

//******************************************************************
//**  Projectile Entity  --  Derived Class
//******************************************************************
EntityProjectileType::EntityProjectileType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t) {
	speed = 10;
	type_damage = 0.1f;
	crater = 0;
	depth = 0;
	scorch = 1.0;
	kick = 0.5f;
	checkcollisions = true;
	type_smoketrail = false;
	type_spiral = 0.0f;
	type_spiralramp = type_spiralperiod = 1.0f;
	type_gravity = 1.0f;
	type_guidespeed = 0.0f;	//Percent of distance to target to use as accel per second.
	type_guidemax = 5.0f;	//Cap on accelerative speed, meters per second.
	type_guidebias = 3.0f;	//Go for guys in front.
	type_guideramp = 1.0f;	//1 second to get guiding fully.
	type_timetolive = 5.0f;
	type_launchinertia = 1.0f;
	type_followground = 0.0;
	type_bounce = 0.0f;
	type_splashradius = 0.0f;
	type_splashdamage = 0.0f;
	type_splashpush = 0.0f;
	type_detonate = 0;
	ClearVec3(type_smoketrailoffset);
	type_hitscan = 0;
	type_hitscanrange = 100.0f;
	type_selfhitdelay = 2.0f;
	type_iterations = 10;
	type_smoketrailtime = 0.02f;	//Max at 50 smokes per second.
	Transitory = true;
	if(cfg){
		if(cfg->FindKey("Transitory")) cfg->GetBoolVal(&Transitory);
		if(cfg->FindKey("speed")) cfg->GetFloatVal(&speed);
		if(cfg->FindKey("damage")) cfg->GetFloatVal(&type_damage);
		if(cfg->FindKey("crater")) cfg->GetFloatVal(&crater);
		if(cfg->FindKey("depth")) cfg->GetFloatVal(&depth);
		if(cfg->FindKey("scorch")) cfg->GetFloatVal(&scorch);
		if(cfg->FindKey("kick")) cfg->GetFloatVal(&kick);
		if(cfg->FindKey("gravity")) cfg->GetFloatVal(&type_gravity);
		if(cfg->FindKey("exploent")){
			cfg->GetStringVal(exploent);
			if(explohitent.len() <= 0) explohitent = exploent;
		}
		if(cfg->FindKey("ExploHitEnt")) cfg->GetStringVal(explohitent);
		if(cfg->FindKey("smokeent")) cfg->GetStringVal(smokeent);
		if(cfg->FindKey("smoketrailent")) cfg->GetStringVal(smoketrailent);
		if(cfg->FindKey("soundent")) cfg->GetStringVal(soundent);
		if(cfg->FindKey("checkcollisions")) cfg->GetBoolVal(&checkcollisions);
		if(cfg->FindKey("smoketrail")) cfg->GetBoolVal(&type_smoketrail);
		if(cfg->FindKey("spiral")) cfg->GetFloatVal(&type_spiral);
		if(cfg->FindKey("spiralramp")) cfg->GetFloatVal(&type_spiralramp);
		if(cfg->FindKey("spiralperiod")) cfg->GetFloatVal(&type_spiralperiod);
		if(cfg->FindKey("guidespeed")) cfg->GetFloatVal(&type_guidespeed);
		if(cfg->FindKey("guidemax")) cfg->GetFloatVal(&type_guidemax);
		if(cfg->FindKey("guidebias")) cfg->GetFloatVal(&type_guidebias);
		if(cfg->FindKey("guideramp")) cfg->GetFloatVal(&type_guideramp);
		if(cfg->FindKey("timetolive")) cfg->GetFloatVal(&type_timetolive);
		if(cfg->FindKey("selfhitdelay")) cfg->GetFloatVal(&type_selfhitdelay);
		if(cfg->FindKey("launchinertia")) cfg->GetFloatVal(&type_launchinertia);
		if(cfg->FindKey("followground")) cfg->GetFloatVal(&type_followground);
		if(cfg->FindKey("bounce")) cfg->GetFloatVal(&type_bounce);
		if(cfg->FindKey("splashradius")) cfg->GetFloatVal(&type_splashradius);
		if(cfg->FindKey("splashdamage")) cfg->GetFloatVal(&type_splashdamage);
		if(cfg->FindKey("splashpush")) cfg->GetFloatVal(&type_splashpush);
		if(cfg->FindKey("detonate")) cfg->GetIntVal(&type_detonate);
		if(cfg->FindKey("smoketrailoffset")) cfg->GetFloatVals(&type_smoketrailoffset[0], 3);
		if(cfg->FindKey("hitscan")) cfg->GetIntVal(&type_hitscan);
		if(cfg->FindKey("hitscanrange")) cfg->GetFloatVal(&type_hitscanrange);
		if(cfg->FindKey("flareent")) cfg->GetStringVal(type_flareent);
		if(cfg->FindKey("iterations")) cfg->GetIntVal(&type_iterations);
		if(cfg->FindKey("smoketrailtime")) cfg->GetFloatVal(&type_smoketrailtime);
	}
	ScaleVec3(type_smoketrailoffset, type_scale);
};
EntityBase *EntityProjectileType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityProjectile(this, Pos, Rot, Vel, id, flags);
}
bool EntityProjectileType::CacheResources(){
	if(!ResCached){
		if(EntityMeshType::CacheResources()){
		//	if(!mesh) texture->AlphaGrad = true;	//Ask for gradiated alpha.  (Only if sprite!)
			VW->CacheResources("smoke", smokeent);
			VW->CacheResources("sound", soundent);
			VW->CacheResources("explo", exploent);
			VW->CacheResources("explo", explohitent);
			VW->CacheResources("smoke", smoketrailent);
			VW->CacheResources("smoke", type_flareent);
		}
	}
	return ResCached;
}
void EntityProjectileType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	EntityMeshType::ListResources(fl);
	VW->ListResources("smoke", smokeent);
	VW->ListResources("sound", soundent);
	VW->ListResources("explo", exploent);
	VW->ListResources("explo", explohitent);
	VW->ListResources("smoke", smoketrailent);
	VW->ListResources("smoke", type_flareent);
}


float EntityProjectileType::InterrogateFloat(const char *thing)
{
	if(!strcasecmp(thing, "Range"))
	{
		if (speed < 0.0005f)
			return type_timetolive;
		else
			return type_timetolive * speed;
	}
	return EntityMeshType::InterrogateFloat(thing);
}

EntityGID EntityProjectile::curowner = 0;	//These are used so that any projectile entities in any way spawned from another projectile can inherit the original's owner and team ids.
//
EntityGID EntityProjectile::GetCurrentOwner(){
	return curowner;
}
//
EntityGID EntityProjectile::SetCurrentOwner(EntityGID owner){
	EntityGID foo = curowner;
	curowner = owner;
	return foo;
}

EntityProjectile::EntityProjectile(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags) :
	EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntityProjectileType *TP = (EntityProjectileType*)TypePtr;
	//
	int ownerset = !!ID;	//True if owner id was set.
	//
	if(ownerset){	//An owner was explicitly specified.
		curowner = ID;	//Put our owner in the last-set-owner static.
	}else{	//No owner specified.
		ID = curowner;	//Pull owner from last-set-owner static.
	}
//	curowner = id;
	//
	timeofbirth = VW->Time();
	CanCollide = false;	//That just means other entities can't see projectile in their collision checks.
	if(VW->Net.IsClientActive() == false){	//Only add vel if we are master, with net spawn, vel is already added.
		NormVec3(Velocity);
		ScaleVec3(Velocity, TP->speed);
	}
	Vec3 smokevel;
	ClearVec3(smokevel);
//	ScaleVec3(Velocity, 0.05f, smokevel);
	EntityBase *e;
	if(e = VW->GetEntity(ID)){	//Find launching entity.
		//
		//We have an Owner!  Pull out team id.
		teamid = e->QueryInt(ATT_TEAMID);
		//
		if(VW->Net.IsClientActive() == false){	//Only add vel if we are master, with net spawn, vel is already added.
			ScaleAddVec3(e->Velocity, TP->type_launchinertia, Velocity);	//Add launch platform's velocity to self.
		//	ScaleAddVec3(e->Velocity, TP->type_launchinertia, smokevel);	//Add launch platform's velocity to self.
		}
		AddVec3(e->Velocity, smokevel);	//Add launch platform's velocity to self.
		//Do this even on client now, to keep smoke moving properly.
	}
	Rotation[0] = asinf(-(Velocity[1] / std::max(1.0f, LengthVec3(Velocity))));
	Rotation[1] = atan2f(Velocity[0], Velocity[2]);
	Rotation[2] = 0;
	EntityBase *snd = VW->AddEntity("sound", TP->soundent, Position, Rotation, Velocity, ID, 0, 0, 0);
	if(snd && snd->QueryInt(ATT_LOOP)){
		snd->SetInt(ATT_CONNECTED_ENT, GID);
	}
	VW->AddEntity("smoke", TP->smokeent, Position, Rotation, smokevel, 0, 0, 0, 0);
	BoundMin[0] = BoundMin[1] = -0.5f;
	BoundMax[0] = BoundMax[1] = 0.5f;
	BoundMin[2] = -0.5f;//-TP->speed * 0.05;
	BoundMax[2] = std::max(0.5f, TP->speed * 0.1f);
	BoundRad = std::max(1.0f, TP->speed * 0.1f);
	Mass = TP->kick;
	CopyVec3(Position, lPos);
//	ScaleAddVec3(Velocity, -VW->FrameFrac(), lPos);
	//TODO:  Do something about bolt-on weapons colliding with things behind the tank...
	//DONE, above back-shifting of lPos is bad, as collision will occur with firing tank or weapon, then
	//position will back up to behind the tank and rear objects will be collided with.
	godentity = NULL;
	firstthink = 1;
	damage = TP->type_damage;
	lastsmoketime = 0;
	//
	flareentity = 0;
	if(TP->type_flareent.len() > 0){
		EntityBase *e = VW->AddEntity("smoke", TP->type_flareent, Position, NULL, CVec3(0, 0, 0), 0, 0, 0, 0);
		if(e) flareentity = e->GID;
	}
	//
	if(ownerset){	//If owner id was set when we were created...
		curowner = 0;	//Reset curowner static to empty so world created projectiles will be owned by world.
		//Also chained/subcreated projectiles that started with owner 0 won't reset the curowner static by mistake.
	}
}
//
EntityProjectile::~EntityProjectile(){
	if(VW && flareentity){
		EntityBase *e = VW->GetEntity(flareentity);
		if(e) e->Remove();	//Remove attached flare entity if present.
	}
}
//TODO:
//Will probably need to make projectiles non-Transitory so that guided weapons can be accurate across
//client and server.
//
void EntityProjectile::Detonate(EntityGID nosplash){
	EntityProjectileType *TP = (EntityProjectileType*)TypePtr;
	//First do cratering, scaled by height off ground.
	if(Position[1] > WATERHEIGHT * 0.5f || TP->depth > 0){	//Not in water, or is a hole-maker.
		float scale = 1.0f;
		float y = VW->Map.FGetI(Position[0], -Position[2]);
		if(y < Position[1]) scale = 1.0f - ((Position[1] - y) / std::max(std::max(TP->crater, TP->depth), 0.1f));
		if(scale > 0.1f){
			VW->Crater(Position[0], -Position[2], TP->crater * scale, TP->depth * scale, TP->scorch);
		}
	}
	//Then do splash damage.
	if(TP->type_splashradius > 0.1f){
		//Fake up bounding box so we can use collision detection system to figure splash recipients.
		BoundMin[0] = BoundMin[1] = BoundMin[2] = -TP->type_splashradius;
		BoundMax[0] = BoundMax[1] = BoundMax[2] = TP->type_splashradius;
		BoundRad = TP->type_splashradius;
		Vec3 oriPos;
		CopyVec3(Position, oriPos);	//Save current center point of blast.
		ClearVec3(Rotation);
		if(VW->CheckCollision(this, GROUP_PROP)){
			EntityBase *col;
			Vec3 colpnt;
			while(col = VW->NextCollider(colpnt)){
				if((col->GID | ENTITYGID_NETWORK) != (nosplash | ENTITYGID_NETWORK)){
					//Okay, now fake up our center, collision point, damage, mass, and velocity.
					Mass = 1.0f;	//One tank's worth.
					Vec3 vec, norm;
					SubVec3(col->Position, oriPos, vec);	//Vector from blast to blastee.
					NormVec3(vec, norm);
					ScaleAddVec3(vec, 0.5f, oriPos, Position);
					ScaleAddVec3(vec, 0.75f, oriPos, colpnt);
					float scale = 1.0f - (LengthVec3(vec) / TP->type_splashradius);
					damage = TP->type_splashdamage * scale;
					ScaleVec3(norm, scale * TP->type_splashpush, Velocity);
					AddVec3(col->Velocity, Velocity);	//Add victims velocity so that we are adding a RELATIVE push.
					//
					if(scale > 0.01f){	//Only if not outside of radius.
						col->CollisionWith(this, colpnt);
					}
				}
			}
		}
	}
}
bool EntityProjectile::Think(){
	EntityProjectileType *TP = (EntityProjectileType*)TypePtr;
	if(!TP->texture) return false;
	//
	EntityProjectile::curowner = ID;
	//This is so any projectiles that happen to be spawned from smoke or smoke trails have same owner.
	//
	//Draw self.
//	if(Active){
//		VW->PolyRend->AddSprite(&(*(TP->texture))[0], w, h, Position, false, TP->rendflags, 1.0f);
//	}
	float lv = LengthVec3(Velocity);
	if(lv > 0.01f){
		Rotation[0] = asinf(-(Velocity[1] / LengthVec3(Velocity)));
		Rotation[1] = atan2f(Velocity[0], Velocity[2]);
		Rotation[2] = 0;
	}else{
		ClearVec3(Rotation);
	}
	//
	if(TP->mesh){
		EntityMesh::Think();
	}else{
		EntitySprite::Think();
	}
	//
	float t = VW->FrameFrac();
	int iters = 1;
	float VWTime = VW->Time();
	if(TP->type_hitscan){
		iters = (int)((TP->type_hitscanrange / TP->speed) * (float)TP->type_iterations);	//Allow specification of how many pieces to chop the virtual second into for a hitscan weapon.
		t = 1.0f / (float)MAX(TP->type_iterations, 1);//0.1f;
	}
	while(iters > 0 && RemoveMe == REMOVE_NONE){
		iters--;
		//
		if(TP->type_smoketrail == true || flareentity){	//Continuous smoke trail, or attached flare.
			Mat3 tm;
			Rot3ToMat3(Rotation, tm);
			Vec3 smokepos;
			Vec3MulMat3(TP->type_smoketrailoffset, tm, smokepos);
			AddVec3(Position, smokepos);
			if(TP->type_smoketrail == true){
				int sms = (TP->type_smoketrailtime * 1000.0f);
				if(TP->type_hitscan || VW->Time() - lastsmoketime > sms ){
					VW->AddEntity("smoke", TP->smoketrailent, smokepos, NULL, NULL, 0, 0, 0, 0);
					if(abs(VW->Time() - lastsmoketime) < sms * 2) lastsmoketime += sms;
					else lastsmoketime = VW->Time();
				}
			}
			if(flareentity){
				EntityBase *e = VW->GetEntity(flareentity);
				if(e) e->SetPos(smokepos);
				else flareentity = 0;
			}
		}
		//
		//Get distance travelled.
		Vec3 disp;
		SubVec3(Position, lPos, disp);
		//
		//Check entity collisions.
		if(TP->checkcollisions && VW->CheckCollision(this, GROUP_PROP)){
			CopyVec3(lPos, Position);
			for(int i = 0; i < 5; i++){	//Interpolate collision.
				ScaleAddVec3(disp, 0.2f, Position);
				if(VW->CheckCollision(this, GROUP_PROP)){
					EntityBase *col;
					Vec3 colpnt;
					while(col = VW->NextCollider(colpnt)){
						if( (VWTime - timeofbirth > (TP->type_selfhitdelay * 1000.0f)) ||
						 (col->GID | ENTITYGID_NETWORK) != (ID | ENTITYGID_NETWORK)  ){
						//	ScaleVec3(Velocity, TP->kick);	//Lessen perceived kinetic force.
							if(VW->Net.IsClientActive() == false){
								col->CollisionWith(this, colpnt);
								VW->AddEntity("explo", TP->explohitent, colpnt, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
								CopyVec3(colpnt, Position);
								Detonate(col->GID);
								Remove();
							}else{
								if(TP->Transitory) Remove();	//If client, only predict removal on transitories.
							}
							EntityProjectile::curowner = 0;
							return true;
						}
					}
				}
			}
		}
		//Check for ground collision.
		if(TP->type_followground > 0.0f){
			float y = VW->Map.FGetI(Position[0], -Position[2]) + TP->type_followground;
			if(Position[1] < y){
				Position[1] = y;
				Velocity[1] = fabsf(Velocity[1]) * TP->type_bounce;
			}
		}else{
		//	if(firstthink == 0){
				int s = (int)(LengthVec3(disp) * 0.5f);
				s = std::max(1, s);
				float S = 1.0f / (float)s;
				Vec3 lP;
				CopyVec3(lPos, lP);
				for(; s; s--){
					if(VW->Map.FGetI(lP[0], -lP[2]) > lP[1]){
						if(VW->Net.IsClientActive() == false){	//Only spawn these ourselves if we are NOT a client.
							VW->AddEntity("explo", TP->exploent, lP, NULL, NULL, 0, /*FLAG_EXPLO_UP*/0, 0, ADDENTITY_FORCENET);
							//Do cratering here!
							CopyVec3(lP, Position);
							Detonate(NULL);
							Remove();
						}else{
							if(TP->Transitory) Remove();	//If client, only predict removal on transitories.
						}
						EntityProjectile::curowner = 0;
						return true;
					}
					ScaleAddVec3(disp, S, lP);
				}
		//	}
		}
		//Do movement.
		CopyVec3(Position, lPos);
		//
		//Gravity.
		Vec3 accel = {VW->gravity[0] * TP->type_gravity, VW->gravity[1] * TP->type_gravity, VW->gravity[2] * TP->type_gravity};
		//Guidance!
		if(TP->type_guidespeed != 0.0f){
			EntityTankGod *g;
			if(!godentity && (g = (EntityTankGod*)FindRegisteredEntity("TANKGOD"))) godentity = g->GID;
			if(g = (EntityTankGod*)VW->GetEntity(godentity)){
				EntityBase *target = VW->GetEntity(g->ClosestTank(Position, ID, Velocity, TP->type_guidebias, teamid));
				if(target){
					float g = TP->type_guidespeed;
					if(TP->type_guideramp != 0.0f){
						g = (((float)(VWTime - timeofbirth) * 0.001f) / TP->type_guideramp) * TP->type_guidespeed;	//Ramp up guiding forces over guideramp seconds.
					}
					Vec3 tv, a, tv2, acd;
					SubVec3(target->Position, Position, tv);	//Us to target.
					CrossVec3(tv, Velocity, tv2);	//Pivot for rotate.
					CrossVec3(Velocity, tv2, acd);	//Accel direction, and plane vector.
					NormVec3(acd);
					float pdist = fabsf(DotVec3(acd, tv));
					ScaleVec3(acd, g * pdist, a);	//Scale by ramp * guidespeed.
					float l = LengthVec3(a);	//Accelerative strength.
					if(l > TP->type_guidemax) ScaleVec3(a, TP->type_guidemax / std::max(0.1f, l));	//Scale down to guidemax length.
					AddVec3(a, accel);	//Add guidance accel to base accel.
				}
			}
		}
		//Spiraling.
		if(TP->type_spiral > 0.0f){
			Rotation[2] = ((double)VWTime * 0.001) / std::max(0.01f, TP->type_spiralperiod) * PI2;	//Spiral repeats in spiralperiod seconds.
			Mat3 tm;
			Rot3ToMat3(Rotation, tm);	//Spiral force is based on "up" vector, with Z rotation.
			float s = ((float)(VWTime - timeofbirth) * 0.001f) / std::max(0.01f, TP->type_spiralramp) * TP->type_spiral;	//Ramp up spiral force over spiralramp seconds.
			ScaleAddVec3(tm[1], std::min(TP->type_spiral, s), accel);	//Add spiral force to acceleration vector.
		}
	//	float t = VW->FrameFrac();
		for(int i = 0; i < 3; i++){
			Position[i] += Velocity[i] * t + accel[i] * t * t * 0.5f;	//Physics.
			Velocity[i] += accel[i] * t;// * t;
		}
		//
		firstthink = 0;
		VWTime += (t * 1000.0f);
	}
	//Sanity.
	if(VW->Time() - timeofbirth > (TP->type_timetolive * 1000.0f) || TP->type_hitscan){
		if(TP->type_detonate){
			if(VW->Net.IsClientActive() == false){	//Only spawn these ourselves if we are NOT a client.
				VW->AddEntity("explo", TP->exploent, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
				Detonate(NULL);	//Blow up at end of life.
			}
		}
		Remove();
	}
//	if(TP->type_hitscan){
//		Remove();
//	}
	if(TP->Transitory == 0 && RemoveMe == REMOVE_NONE && VW->Net.IsServerActive()){	//Send state if persistent entity.
		BitPacker<128> pe;
		pe.PackInt(MSG_HEAD_POSITION, 8);
		VW->PackPosition(pe, Position, 14);
		for(int i = 0; i < 3; i++){
	//		pe.PackFloatInterval(Position[i] - VW->worldcenter[i], -1000, 1000, 14);
			pe.PackFloatInterval(Velocity[i], -1000, 1000, 14);
		}
		QueuePacket(TM_Unreliable, pe);
	}
	EntityProjectile::curowner = 0;
	return true;
}
void EntityProjectile::DeliverPacket(const unsigned char *data, int len){
	if(data[0] == MSG_HEAD_POSITION){
		BitUnpackEngine pe(data + 1, len - 1);
		VW->UnpackPosition(pe, Position, 14);
		for(int i = 0; i < 3; i++){
	//		pe.UnpackFloatInterval(Position[i], -1000, 1000, 14);
	//		Position[i] += VW->worldcenter[i];
			pe.UnpackFloatInterval(Velocity[i], -1000, 1000, 14);
		}
	}
}

float EntityProjectile::QueryFloat(int type){
	EntityProjectileType *TP = (EntityProjectileType*)TypePtr;
	if(type == ATT_DAMAGE) return damage;
	return EntitySprite::QueryFloat(type);
}
int EntityProjectile::QueryInt(int type){
	if(type == ATT_WEAPON_OWNER) return ID;
	return EntityMesh::QueryInt(type);
}
//******************************************************************
//**  Bauble Entity  --  Derived Class
//******************************************************************
EntityBaubleType::EntityBaubleType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t){
	type_stiffness = 0.5f;
	type_dampening = 1.0f;
	if(cfg){
		if(cfg->FindKey("stiffness")) cfg->GetFloatVal(&type_stiffness);
		if(cfg->FindKey("dampening")) cfg->GetFloatVal(&type_dampening);
	}
	Transitory = true;
}
EntityBase *EntityBaubleType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityBauble(this, Pos, Rot, Vel, id, flags);
}
bool EntityBaubleType::CacheResources(){
	return EntityMeshType::CacheResources();
}
void EntityBaubleType::ListResources(FileCRCList *fl){
	EntityMeshType::ListResources(fl);
}

EntityBauble::EntityBauble(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntityBaubleType *TP = (EntityBaubleType*)TypePtr;
	CanCollide = false;
//	CopyVec3(Position, lastpos);
	IdentityMat3(baublemat);
	ClearVec3(lasttippos);
	ClearVec3(lasttipvel);
	ClearVec3(baublepos);
	ClearVec3(baublevel);
	for(int i = 0; i < 3; i++){
		insignia[i] = 0;
		insigniatype[i] = 0;
	}
}
EntityBauble::~EntityBauble(){
	if(VW){
		for(int i = 0; i < 2; i++){	//Remove insignia entities when bauble goes byebye.
			if(insigniatype[i]){
				EntityBase *e = VW->GetEntity(insignia[i]);
				if(e) e->Remove();
			}
		}
	}
}
bool EntityBauble::Think(){
	int i;
	EntityBaubleType *TP = (EntityBaubleType*)TypePtr;
	if(!TP->mesh) return false;
	Mat43 mat;
	Rot3ToMat3(Rotation, mat);
	CopyVec3(Position, mat[3]);	//Make matrix.
	//
	//Do baubling.
	Vec3 tipdiff, tv2, tv = {0.0f, TP->mesh->BndMax[1], 0.0f};
	Vec3MulMat43(tv, mat, tv2);	//Get current tip of bauble.
	SubVec3(tv2, lasttippos, tipdiff);	//Amount tip moved from previous tip pos.
	CopyVec3(tv2, lasttippos);	//Save current bauble tip for next time.
	ScaleVec3(tipdiff, 1.0f / MAX(0.001f, VW->FrameFrac()));	//Scale up to meters per second.
	SubVec3(tipdiff, lasttipvel, tv);	//Difference between predicted and actual vel.
	Vec3IMulMat3(tv, mat, tv2);	//Invert rotational xform to get bauble relative displacement.
	CopyVec3(tipdiff, lasttipvel);	//Set predictive velocity for next time.
	//
	ScaleAddVec3(tv2, -1.0f, baublevel);	//Add moment velocity change to bauble-around velocity.
	Vec3MulMat3(baublevel, baublemat, tv);	//Rotate vel into bauble tip space.
	ScaleAddVec3(tv, VW->FrameFrac(), baublepos);	//Add to baubling-pos.
	ScaleAddVec3(baublepos, -VW->FrameFrac() * TP->type_stiffness * 10.0f, baublevel);	//Yank back towards neutral.
//	ScaleVec3(baublevel, 1.0f - (VW->FrameFrac() * TP->type_dampening * 1.0f));	//Dampen movement.
	ScaleVec3(baublevel, DAMP(1.0f, 1.0f - TP->type_dampening, MAX(0.001f, VW->FrameFrac())));	//Dampen movement.
	for(i = 0; i < 3; i++){
		baublepos[i] = std::min(std::max(baublepos[i], -0.99f), 0.99f);
		baublevel[i] = std::min(std::max(baublevel[i], -3.0f), 3.0f);
	}
	//
	//Now hacky matrix screwing to show bauble effect.
	Mat3 tm;//, tm2;
	baublemat[1][0] = baublepos[0]; baublemat[1][1] = baublepos[1] + 1.0f; baublemat[1][2] = baublepos[2];
	NormVec3(baublemat[1]);
	CopyVec3(baublemat[1], baublepos);	//Stick normalised vector back in baublepos in some way.
	baublepos[1] -= 1.0f;
	baublemat[2][0] = 0.0f; baublemat[2][1] = 0.0f; baublemat[2][2] = 1.0f;
	CrossVec3(baublemat[1], baublemat[2], baublemat[0]);
	NormVec3(baublemat[0]);	//This is needed because crossing two non-orthogonal unit vectors produces a non-unit vector!
	CrossVec3(baublemat[0], baublemat[1], baublemat[2]);
	Mat3MulMat3(baublemat, mat, tm);
	for(int n = 0; n < 3; n++) CopyVec3(tm[n], mat[n]);
	//
	if(TP->mesh){
		meshobj.Configure(TP->texture, TP->mesh, mat, mat[3], TP->type_rendflags, fade, TP->mesh->BndRad, TP->mesh->LodBias);
		VW->PolyRend->AddSolidObject(&meshobj);
	//	VW->PolyRend->AddMeshMat(TP->mesh, mat, false, TP->rendflags, fade);
	}
	//
	for(i = 0; i < 3; i++){
		if(insigniatype[i]){
			EntityBase *e = VW->GetEntity(insignia[i]);
			if(e){
				e->SetPos(mat[3]);
			//	e->SetRot(Rotation);
				e->SetVec(ATT_MATRIX3, mat);
				//
			}else if (i != 2){	//Create if not existing and not the CTF flag entity
				e = VW->AddEntity(insigniatype[i], NULL, NULL, NULL, 0, 0, 0, 0);
				if(e){
					insignia[i] = e->GID;
					if(i == 0){	//The team flag spot.
						teamid = e->QueryInt(ATT_TEAMID);
					}
				}else{
					insigniatype[i] = 0;
					if(i == 0){	//The team flag spot.
						teamid = 0;
					}
				}
			}
		}
		else if (i != 2) // don't do this if its the CTF flag entity
		{
			if(insignia[i]){	//Remove insignia if no type specified.
				EntityBase *e = VW->GetEntity(insignia[i]);
				if(e) e->Remove();
				insignia[i] = 0;
				if(i == 0){	//The team flag spot.
					teamid = 0;
				}
			}
		}
	}
	//
	return true;
}
bool EntityBauble::SetInt(int type, int attr){
	if(type == ATT_INSIGNIA1_HASH){
		EntityBase *e;
		if(attr != insigniatype[0] && (e = VW->GetEntity(insignia[0]))){	//If insignia is different, kill ent so it gets recreated.
			e->Remove();
			insignia[0] = 0;
		}
		insigniatype[0] = attr;
		return true;
	}
	if(type == ATT_INSIGNIA2_HASH){
		EntityBase *e;
		if(attr != insigniatype[1] && (e = VW->GetEntity(insignia[1]))){
			e->Remove();
			insignia[1] = 0;
		}
		insigniatype[1] = attr;
		return true;
	}
	if(type == ATT_INSIGNIA3ID){
		EntityBase *e = VW->GetEntity(attr);
		if(e)
		{
			insignia[2] = attr;
			insigniatype[2] = e->TypePtr->chash;
		}
		else
		{
			insignia[2] = 0;
			insigniatype[2] = 0;
		}
		return true;
	}
	return EntityMesh::SetInt(type, attr);
}
int EntityBauble::QueryInt(int type){
	if(type == ATT_INSIGNIA1_HASH) return insigniatype[0];
	if(type == ATT_INSIGNIA2_HASH) return insigniatype[1];
	return EntityMesh::QueryInt(type);
}

//******************************************************************
//**  Weapon Entity  --  Derived Class
//******************************************************************
#define WEAPON_FLOAT 4.0f
EntityWeaponType::EntityWeaponType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t){
	type_bolton = 0;
	type_ammo = 1;
	type_reloadtime = 1.0f;
	ClearVec3(type_launchangle);
	ClearVec3(type_launchcoords);
	type_weight = 1;
	type_level = 0;
	ClearVec3(type_secrotmin);
	ClearVec3(type_secrotmax);
	type_secrotwindup = 1.0f;
	ClearVec3(type_secmeshoffset);
	type_ammodisplaymult = 1;
	ClearVec3(type_spread);
	type_launchcount = 1;
	type_globalcoords = 0;
	ClearVec3(type_secentoffset);
	type_transmutetexture = 0;
	type_transmuteperturb = 0.0f;
	//
	type_pickupsound = "pickup";
	type_inentity = "explosphere/blipin";
	type_spawnsound = "";
	type_outentity = "explosphere/blipout";
	type_spawnentity = "explosphere/blipin";

	type_unique = 0;
	//
	if(cfg){
		int i;
		if(cfg->FindKey("pickupsound")) cfg->GetStringVal(type_pickupsound);
		if(cfg->FindKey("spawnsound")) cfg->GetStringVal(type_spawnsound);
		if(cfg->FindKey("inentity")) cfg->GetStringVal(type_inentity);
		if(cfg->FindKey("outentity")) cfg->GetStringVal(type_outentity);
		if(cfg->FindKey("spawnentity")) cfg->GetStringVal(type_spawnentity);
		//
		if(cfg->FindKey("secmesh")) cfg->GetStringVal(secmeshfile);
		if(cfg->FindKey("secmesh1")) cfg->GetStringVal(secmeshfile1);
		if(cfg->FindKey("secmesh2")) cfg->GetStringVal(secmeshfile2);
		if(cfg->FindKey("weight")) cfg->GetIntVal(&type_weight);
		if(cfg->FindKey("level")) cfg->GetIntVal(&type_level);
		if(cfg->FindKey("bolton")) cfg->GetIntVal(&type_bolton);
		if(cfg->FindKey("ammo")) cfg->GetIntVal(&type_ammo);
		if(cfg->FindKey("reloadtime")) cfg->GetFloatVal(&type_reloadtime);
		if(cfg->FindKey("projectile")) cfg->GetStringVal(type_projectile);
		if(cfg->FindKey("launchangle")) cfg->GetFloatVals(&type_launchangle[0], 3);
		for(i = 0; i < 3; i++){ type_launchangle[i] *= DEG2RAD; };
		if(cfg->FindKey("secrotmin")) cfg->GetFloatVals(&type_secrotmin[0], 3);
		if(cfg->FindKey("secrotmax")) cfg->GetFloatVals(&type_secrotmax[0], 3);
		for(i = 0; i < 3; i++){ type_secrotmin[i] *= DEG2RAD; };
		for(i = 0; i < 3; i++){ type_secrotmax[i] *= DEG2RAD; };
		if(cfg->FindKey("secrotwindup")) cfg->GetFloatVal(&type_secrotwindup);
		if(cfg->FindKey("secmeshoffset")) cfg->GetFloatVals(&type_secmeshoffset[0], 3);
		if(cfg->FindKey("launchcoords")) cfg->GetFloatVals(&type_launchcoords[0], 3);
		if(cfg->FindKey("FireSoundEnt")) cfg->GetStringVal(type_firesoundent);
		if(cfg->FindKey("ammodisplaymult")) cfg->GetIntVal(&type_ammodisplaymult);
		if(cfg->FindKey("launchspread")) cfg->GetFloatVals(&type_spread[0], 3);
		for(i = 0; i < 3; i++){ type_spread[i] *= DEG2RAD; };
		if(cfg->FindKey("launchcount")) cfg->GetIntVal(&type_launchcount);
		if(cfg->FindKey("globalcoords")) cfg->GetIntVal(&type_globalcoords);
		if(cfg->FindKey("secent")) cfg->GetStringVal(type_secentname);
		if(cfg->FindKey("secentoffset")) cfg->GetFloatVals(&type_secentoffset[0], 3);
		if(cfg->FindKey("transmutetexture")) cfg->GetIntVal(&type_transmutetexture);
		if(cfg->FindKey("transmuteperturb")) cfg->GetFloatVal(&type_transmuteperturb);
		if(cfg->FindKey("unique")) cfg->GetIntVal(&type_unique);
	}
	ScaleVec3(type_secmeshoffset, type_scale);
	ScaleVec3(type_launchcoords, type_scale);
	ScaleVec3(type_secentoffset, type_scale);
	Transitory = false;
}
EntityBase *EntityWeaponType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityWeapon(this, Pos, Rot, Vel, id, flags);
}
bool EntityWeaponType::CacheResources(){
	if(!ResCached){
		if(EntityMeshType::CacheResources()){
			Mesh *secmesh1, *secmesh2;
			bool smooth = (type_rendflags & (MESH_SHADE_SMOOTH | MESH_ENVMAP_SMOOTH)) != 0;
			secmesh = VW->GetMesh(secmeshfile, type_scale, smooth);
			secmesh1 = VW->GetMesh(secmeshfile1, type_scale, smooth);
			secmesh2 = VW->GetMesh(secmeshfile2, type_scale, smooth);
			if(texture && mesh && secmesh){
				secmesh->SetTexture(texture);
				secmesh->SetLod(0, type_lodbias, secmesh1);
				if(secmesh1){
					secmesh1->SetTexture(texture);
					secmesh1->SetLod(1, type_lodbias, secmesh2);
					if(secmesh2) secmesh2->SetTexture(texture);
				}
			}
			VW->CacheResources("projectile", type_projectile);
			VW->CacheResources("sound", type_firesoundent);
			VW->CacheResources("mesh", type_secentname);
			//
			VW->CacheResources("sound", type_pickupsound);
			VW->CacheResources("sound", type_spawnsound);
			VW->CacheResources("explosphere", type_inentity);
			VW->CacheResources("explosphere", type_outentity);
			VW->CacheResources("explosphere", type_spawnentity);
		}
	}
	return ResCached;
}
void EntityWeaponType::ListResources(FileCRCList *fl){
	if (ResListed) return;

	EntityMeshType::ListResources(fl);

	fl->FileCRC(secmeshfile);
	fl->FileCRC(secmeshfile1);
	fl->FileCRC(secmeshfile2);

	VW->ListResources("projectile", type_projectile);
	VW->ListResources("sound", type_firesoundent);
	VW->ListResources("mesh", type_secentname);
	//
	VW->ListResources("sound", type_pickupsound);
	VW->ListResources("sound", type_spawnsound);
	VW->ListResources("explosphere", type_inentity);
	VW->ListResources("explosphere", type_outentity);
	VW->ListResources("explosphere", type_spawnentity);
}
int EntityWeaponType::InterrogateInt(const char *thing){	//Responds to "powerup-family", "powerup-level", and "powerup-weight".
	if(!thing) return 0;
	if(CmpLower(thing, "powerup-family")) return 1;
	if(CmpLower(thing, "powerup-level")) return type_level;	//TODO: return "level" (goodness) of power-up.
	if(CmpLower(thing, "powerup-weight")) return type_weight;	//TODO: return "weight" for chance of thing appearing.
	if(CmpLower(thing, "unique")) return type_unique;
	return 0;
}
float EntityWeaponType::InterrogateFloat(const char *thing)
{
	if(!thing) return 0;
	if(CmpLower(thing, "reloadtime")) return type_reloadtime;
	return 0;
}

EntityWeapon::EntityWeapon(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntityWeaponType *TP = (EntityWeaponType*)TypePtr;
	CanCollide = false;
	tank = 0;
//	if(TP->mesh){
//		CopyVec3(TP->mesh->BndMin, BoundMin);
//		CopyVec3(TP->mesh->BndMax, BoundMax);
//	}
	ScaleVec3(BoundMax, 2.0f);	//Double collision bounding box.
	ScaleVec3(BoundMin, 2.0f);
	BoundMin[1] -= (WEAPON_FLOAT);// + 1.0f);
	BoundMax[1] += (WEAPON_FLOAT);// + 1.0f);
	BoundRad += WEAPON_FLOAT;	//Expand bbox to collide with tanks below.
	Mass = 0.0f;
	ammo = TP->type_ammo;
	lastfiretime = 0;
	Fire = 0;
	ltank = tank;
	firesound = 0;
	Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + WEAPON_FLOAT;
	tankgod = NULL;
	ClearVec3(secrot);
	secrotlerp = 0.0f;
	secent = 0;
	//
	VW->AddEntity("explosphere", TP->type_spawnentity, Position, NULL, NULL, 0, 0, 0, 0);
	VW->AddEntity("sound", TP->type_spawnsound, Position, NULL, NULL, 0, 0, 0, 0);
	//This should be a transitory entity.
}
EntityWeapon::~EntityWeapon(){
	if(VW && secent){
		EntityBase *e = VW->GetEntity(secent);
		if(e) e->Remove();
	}
}

int EntityWeapon::QueryInt(int type){
	EntityWeaponType *TP = (EntityWeaponType*)TypePtr;
	if(type == ATT_WHICH_BOLTON) return TP->type_bolton;
	if(type == ATT_WEAPON_STATUS) return tank;
	if(type == ATT_WEAPON_AMMO) return ammo;
	if(type == ATT_WEAPON_MULT)	return TP->type_ammodisplaymult;
	if(type == ATT_WEAPON_FULLAMMO) return TP->type_ammo;
	if(type == ATT_WEAPON_TRANSTEX) return TP->type_transmutetexture;
	if(type == ATT_WEAPON_TRANSPERTURB) return TP->type_transmuteperturb;
	if(type == ATT_WEAPON_LASTFIRETIME) return lastfiretime;
	return EntityMesh::QueryInt(type);
}
float EntityWeapon::QueryFloat(int type){
	EntityWeaponType *TP = (EntityWeaponType*)TypePtr;
	if(type == ATT_WEAPON_TRANSPERTURB) return TP->type_transmuteperturb;
	return EntityMesh::QueryFloat(type);
}
CStr EntityWeapon::QueryString(int type)
{
	EntityWeaponType *TP = (EntityWeaponType*)TypePtr;
	if (type == ATT_WEAPON_PROJECTILE_TYPE) return TP->type_projectile;
	return EntityMesh::QueryString(type);
}
bool EntityWeapon::SetInt(int type, int attr){
	if(type == ATT_WEAPON_AMMO){
		ammo = attr;
		return true;
	}
	if(type == ATT_FIRE){
		if(VW->Net.IsClientActive() == false){ Fire = attr; return true; }	//Only allow direct setting if we are master, otherwise Fire is set through net packets.
	}
	if(type == ATT_WEAPON_STATUS){
	//	if(attr == 0) tank = 0;
		tank = attr;
		if(VW->Net.IsServerActive()){	//Propagate weapon drops to clients.
			//Make outgoing packet to relay tank connection.
			char b[64];
			b[0] = BYTE_HEAD_MSGENT;
			WLONG(&b[1], GID);
			b[5] = MSG_HEAD_LINKENTITY;
			//Send uncompressed data for now.
			b[6] = 0x0;
			WLONG(&b[7], tank);
			WFLOAT(&b[11], Position[0]);
			WFLOAT(&b[15], Position[2]);
			VW->Net.QueueSendClient(CLIENTID_BROADCAST, TM_Ordered, b, 19);
			//Possible problem here...  If sent Reliable and packet happens to
			//arrive before entity creation, then what?  May need to queue packets destined
			//for not-yet-created entities for a time.
		}
		return true;
	}	//For disconnecting.
	return EntityMesh::SetInt(type, attr);
}
bool EntityWeapon::Think(){
	EntityWeaponType *TP = (EntityWeaponType*)TypePtr;
	int SendPacket = 0;
	if(tank == 0){
		Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + WEAPON_FLOAT + 0.5f * sin((double)VW->Time() / 500.0f);
		Rotation[0] = CLAMP(Rotation[0], -PI * 0.1f, PI * 0.1f);
		Rotation[1] = (double)VW->Time() / 900.0f;
		Rotation[2] = CLAMP(Rotation[2], -PI * 0.1f, PI * 0.1f);
	//	CanCollide = true;
		if(VW->CheckCollision(this, GROUP_ACTOR)){
			EntityBase *col;
			Vec3 colpnt;
			while(col = VW->NextCollider(colpnt)) CollisionWith(col, colpnt);
		}
		//
		EntityTankGod *god;	//Register self with GOD.
		if(!tankgod){
			god = (EntityTankGod*)FindRegisteredEntity("TANKGOD");
			if(god) tankgod = god->GID;
		}
		if(tankgod && (god = (EntityTankGod*)VW->GetEntity(tankgod))){
			god->UpdatePowerUp(this, TP->type_level);
		}
		//Disable fire sound if weapon is dropped from tank...
		if(firesound){	//Fire released, stop attached sound if need be...
			EntityBase *e = VW->GetEntity(firesound);
			if(e){
				if(e->SetInt(ATT_CMD_NICEREMOVE, 1) == false) e->Remove();	//Try nice remove, if that fails, normal remove.
			}
			firesound = NULL;
		}
		//
//if(Fire) OutputDebugLog("*** Weapon's tank is 0 but Fire is set.\n");
	}else{
		if(ltank == 0)
		{	//First time riding.
			VW->AddEntity("explosphere", TP->type_inentity, Position, NULL, NULL, 0, 0, 0, 0);//, ADDENTITY_FORCENET);	//Force this
	// you got nuke sound
			VW->AddEntity("sound", TP->type_pickupsound, Position, NULL, NULL, tank, 0, 0, 0);	//Client and server side sounds spawned.
		}
		EntityBase *tankent = VW->GetEntity(tank);
	//	CanCollide = false;
		if(tankent && tankent->QueryFloat(ATT_TANK_FIRERATESCALE) != 0.0f && Fire && abs(int(VW->Time() - lastfiretime)) >= (TP->type_reloadtime * 1000.0f / tankent->QueryFloat(ATT_TANK_FIRERATESCALE)) &&
			(lastfiretime < VW->Time() || (lastfiretime - VW->Time() > 1000)) ){	//Fix to bug where lastfiretime could be set astronomical by low VW->Time() and high reload time with calc in pick-up code.
		//	if(VW->Net.IsClientActive() == false) ammo--;
			if(ammo > 0 && (VW->GameMode() & GAMEMODE_NOCANSHOOT) == 0){
				if(VW->Net.IsClientActive() == false){
					//Only launch projectile if server.  And if shooting is allowed.
					for(int iter = 0; iter < TP->type_launchcount && ammo > 0; iter++){	//Multiple launches per fire press.
						ammo--;
						EntityBase *e = VW->GetEntity(tank);
						if(e) CopyVec3(e->Velocity, Velocity);	//So projectile inherits proper velocity.
						Mat3 tm, tml, tms;
						Rot3ToMat3(Rotation, tm);
						Rot3ToMat3(TP->type_launchangle, tml);
						Rot3 spread = {TP->type_spread[0] * ((float)TMrandom() / (float)RAND_MAX) - TP->type_spread[0] * 0.5f,
							TP->type_spread[1] * ((float)TMrandom() / (float)RAND_MAX) - TP->type_spread[1] * 0.5f,
							TP->type_spread[2] * ((float)TMrandom() / (float)RAND_MAX) - TP->type_spread[2] * 0.5f };
						Rot3ToMat3(spread, tms);
						Vec3 tv = {0, 0, 1}, v, p;
						Vec3MulMat3(tv, tms, v);	//Random Spread matrix.
						Vec3MulMat3(v, tml, tv);	//Launch angle matrix.
						if(TP->type_globalcoords){
							CopyVec3(tv, v);
							CopyVec3(TP->type_launchcoords, p);	//Global launch offset coordinates, not affected by weapon rotation.
						}else{
							Vec3MulMat3(tv, tm, v);		//Rotation of weapon.
							Vec3MulMat3(TP->type_launchcoords, tm, p);
						}
						AddVec3(Position, p);
						EntityBase *ep = VW->AddEntity("projectile", TP->type_projectile, p, NULL, v, tank, 0, 0, ADDENTITY_FORCENET);
					//	if(e && ep) ep->SetInt(ATT_TEAMID, e->QueryInt(ATT_TEAMID));
						//Projectile will instead suck teamid out of owner.
					}
				}
				//
				if(TP->type_firesoundent.len() > 0 && !firesound){	//Kick off initial fire sound entity.
					EntityTypeBase *et = VW->FindEntityType("sound", TP->type_firesoundent);
				//	EntityBase *e = VW->AddEntity("sound", TP->type_firesoundent, Position, NULL, Velocity, 0, 0, 0, 0);
					if(et){
						bool loop = et->InterrogateInt("loop") != 0;	//Find out if sound is a looping one.
						if(loop || VW->Net.IsClientActive() == false){	//If it is a looper, or we are the server, start it up.
							EntityBase *e = VW->AddEntity(et, Position, NULL, Velocity, tank, 0, 0, (loop == 0 ? ADDENTITY_FORCENET : 0));
							if(e){	//Forcenet it if non-loop, and we're on server, obviously.
								if(loop){//e->QueryInt(ATT_LOOP)){
									firesound = e->GID;
									e->SetInt(ATT_CONNECTED_ENT, GID);	//Connect to us.
								}//No loop, so fire as independent one-shot.
							}
						}
					}
				}
				SendPacket = 1;
				//
			//	lastfiretime = VW->Time();
				int tms = (TP->type_reloadtime * 1000.0f);
				if(lastfiretime < VW->Time() - tms * 2) lastfiretime = VW->Time();
			//	else lastfiretime += tms;
				else lastfiretime = std::max(lastfiretime + tms, static_cast<unsigned int>(VW->Time() - VW->FrameTime()));	//Okay, limit it so the max firetime creep is the frame ms.
			}
			if(ammo <= 0){
				//
				if(TP->type_ammo > 1 && ammo == 0){	//No blipout for single-shot weapons like nuke missiles.
					VW->AddEntity("explosphere", TP->type_outentity, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
					ammo--;	//Set it to negative so we don't get multiple blips if delete packet is delayed.
				}	//Not forcing net here is relying on the fact that the client will get the ammo=0 packet and run this code before the server can send a delete.  Provides better looking blipout placement.
				//Ok, now we are forcing one from server to client, and doing one locally, best of both worlds.
				//
				Remove();
			}
		}
		if(!Fire && firesound){	//Fire released, stop attached sound if need be...
//OutputDebugLog("*** Removing firing sound entity.\n");
			EntityBase *e = VW->GetEntity(firesound);
			if(e){
				if(e->SetInt(ATT_CMD_NICEREMOVE, 1) == false) e->Remove();	//Try nice remove, if that fails, normal remove.
			}
			firesound = NULL;
			SendPacket = 1;
		}
		//
		if(TMrandom() % 1000 < VW->FrameTime()) SendPacket = 1;
		if(SendPacket && VW->Net.IsServerActive()){
			char b[16];
			b[0] = BYTE_HEAD_MSGENT;
			WLONG(&b[1], GID);
			b[5] = MSG_HEAD_POSITION;
		//	b[6] = (char)ammo;	//Send ammo level to clients.
			WSHORT(&b[6], ammo);
			b[8] = Fire;
			WLONG(&b[9], tank);
			VW->Net.QueueSendClient(CLIENTID_BROADCAST, TM_Unreliable, b, 13);
			//Send this as a probability packet, and use a bitflag to tell if tank should be
			//zero, and if it is, then send x and z coords instead.
		}
		//
		if(NULL == VW->GetEntity(tank)) Remove();	//Whoops, tank disappeared.
		ltank = tank;	//This must only be set if tank is set at very beginning of think.
		//
		//Secondary mesh rotation.
		secrotlerp += ((Fire && lastfiretime < VW->Time()) ? 1.0f : -1.0f) * VW->FrameFrac() * (1.0f / std::max(0.001f, TP->type_secrotwindup));
		secrotlerp = std::min(1.0f, std::max(0.0f, secrotlerp));
		//
	}
	//
	EntityMesh::Think();
	if(TP->secmesh){
		//
		Vec3 secrotvel;
		LerpVec3(TP->type_secrotmin, TP->type_secrotmax, secrotlerp, secrotvel);	//secrotlerp will go from 0 to 1 to denote speed of rotation.
		ScaleAddVec3(secrotvel, VW->FrameFrac(), secrot);
		//
		Mat3 tm1;
		Rot3ToMat3(secrot, tm1);	//Rot mat from secondary mesh angles.
		Mat3MulMat3(tm1, meshobj.Orient, secmeshobj.Orient);	//Rotate through primary mesh orientation to be child.
		Vec3MulMat3(TP->type_secmeshoffset, meshobj.Orient, secmeshobj.Pos);	//Push offset position through primary matrix too.
		AddVec3(meshobj.Pos, secmeshobj.Pos);
		//
		secmeshobj.Configure(TP->texture, TP->secmesh, secmeshobj.Orient, secmeshobj.Pos, TP->type_rendflags, fade, TP->mesh->BndRad, TP->type_lodbias);
		if( (fade < 1.0f || (TP->type_rendflags & MESH_BLEND_ADD)) && !(TP->type_rendflags & MESH_BLEND_ENVMAP))
			VW->PolyRend->AddTransObject(&secmeshobj);
		else VW->PolyRend->AddSolidObject(&secmeshobj);
	}
	//
	//Do xecondary entity.
	if(TP->type_secentname.len() > 0){
		if(TP->secmesh == 0){
			//
			Vec3 secrotvel;	//Duplicate of the above if there is no secmesh.
			LerpVec3(TP->type_secrotmin, TP->type_secrotmax, secrotlerp, secrotvel);	//secrotlerp will go from 0 to 1 to denote speed of rotation.
			ScaleAddVec3(secrotvel, VW->FrameFrac(), secrot);
			//
		}
		EntityBase *e = VW->GetEntity(secent);
		if(!e){
			e = VW->AddEntity("mesh", TP->type_secentname, Position, Rotation, Velocity, 0, 0, 0, 0);
			if(e) secent = e->GID;
		}
		if(e){	//Set position and rotation of secondary entity.
			Vec3 tv;
			Vec3MulMat3(TP->type_secentoffset, meshobj.Orient, tv);
			AddVec3(Position, tv);
			e->SetPos(tv);
			AddVec3(Rotation, secrot, tv);
			e->SetRot(tv);
		}
	}
	//
	return true;
}
bool EntityWeapon::CollisionWith(EntityBase *collider, Vec3 colpnt){
	EntityWeaponType *TP = (EntityWeaponType*)TypePtr;
	//Check to see if a tank hit us.
	if(CmpLower(collider->TypePtr->cname, "racetank") && tank == 0){	//Tank touched us.
		if( (collider->QueryInt(ATT_WEAPON_ENTITY) == 0 || collider->QueryInt(ATT_WEAPON_ENTITY) == GID) &&
			collider->QueryInt(ATT_WEAPON_PICKUP_OK) ){
			//Except the situation where tank points to us, but we don't point to tank.  Bug fix...
			if(VW->Net.IsClientActive() == false){
				collider->SetInt(ATT_WEAPON_ENTITY, GID);
				tank = collider->GID;
				VW->AddEntity("explosphere", TP->type_outentity, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
				SetInt(ATT_WEAPON_STATUS, tank);
				lastfiretime = (VW->Time() + 1000) - (TP->type_reloadtime * 1000.0f);
			}
		}
	}
	return true;
}
//Process incoming packets directed at this entity.
void EntityWeapon::DeliverPacket(const unsigned char *data, int len){
	EntityWeaponType *TP = (EntityWeaponType*)TypePtr;
	if(data){
		if(data[0] == MSG_HEAD_LINKENTITY && len >= 6){
			EntityBase *e = VW->GetEntity(RLONG(&data[2]));
			if(e){
				tank = e->GID;
				e->SetInt(ATT_WEAPON_ENTITY, GID);
				//
				//This adds a 1 second delay before being able to fire, when picked up.
				lastfiretime = (VW->Time() + 1000) - (TP->type_reloadtime * 1000.0f);
				//
//OutputDebugLog("*** Weapon received link packet, tank set to " + String((int)tank) + ".\n");
			}else{
				tank = 0;
			}
			if(len >= 14){
				Position[0] = RFLOAT(&data[6]);
				Position[2] = RFLOAT(&data[10]);
			}
		}
		if(data[0] == MSG_HEAD_POSITION && len >= 4){
		//	ammo = (unsigned char)data[1];
			ammo = RSHORT(&data[1]);	//TODO:  Redo this, so negative ammo values are xfered correctly...
			Fire = data[3];
			if(len >= 8){
				EntityBase *e = VW->GetEntity(RLONG(&data[4]));
				if(e){
					tank = e->GID;
					e->SetInt(ATT_WEAPON_ENTITY, GID);
				}
			}
//OutputDebugLog("*** Weapon received Stat packet, fire is " + String(Fire) + ".\n");
		}
	}
}

//******************************************************************
//**  PowerUp Entity  --  Derived Class
//******************************************************************
#define POWERUP_FLOAT 4.0f
EntityPowerUpType::EntityPowerUpType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t){
	type_healthfix = 0.0f;
	type_ammoadd = 0;
	type_level = 0;
	type_weight = 1;
	type_boltonammoadd = 0.0f;
	//
	type_pickupsound = "pickup";
	type_inentity = "explosphere/blipin";
	type_spawnsound = "";
	type_outentity = "explosphere/blipout";
	type_spawnentity = "explosphere/blipin";
	//
	if(cfg){
		if(cfg->FindKey("pickupsound")) cfg->GetStringVal(type_pickupsound);
		if(cfg->FindKey("spawnsound")) cfg->GetStringVal(type_spawnsound);
		if(cfg->FindKey("inentity")) cfg->GetStringVal(type_inentity);
		if(cfg->FindKey("outentity")) cfg->GetStringVal(type_outentity);
		if(cfg->FindKey("spawnentity")) cfg->GetStringVal(type_spawnentity);
		//
		if(cfg->FindKey("healthfix")) cfg->GetFloatVal(&type_healthfix);
		if(cfg->FindKey("ammoadd")) cfg->GetIntVal(&type_ammoadd);
		if(cfg->FindKey("weight")) cfg->GetIntVal(&type_weight);
		if(cfg->FindKey("level")) cfg->GetIntVal(&type_level);
		if(cfg->FindKey("boltonammoadd")) cfg->GetFloatVal(&type_boltonammoadd);
	}
	Transitory = false;
}
EntityBase *EntityPowerUpType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityPowerUp(this, Pos, Rot, Vel, id, flags);
}
bool EntityPowerUpType::CacheResources(){
	if(!ResCached){
		if(EntityMeshType::CacheResources()){
			VW->CacheResources("sound", type_pickupsound);
			VW->CacheResources("sound", type_spawnsound);
			VW->CacheResources("explosphere", type_inentity);
			VW->CacheResources("explosphere", type_outentity);
			VW->CacheResources("explosphere", type_spawnentity);
		}
	}
	return ResCached;
}
void EntityPowerUpType::ListResources(FileCRCList *fl){
	if (ResListed) return;

	EntityMeshType::ListResources(fl);
	VW->ListResources("sound", type_pickupsound);
	VW->ListResources("sound", type_spawnsound);
	VW->ListResources("explosphere", type_inentity);
	VW->ListResources("explosphere", type_outentity);
	VW->ListResources("explosphere", type_spawnentity);
}
int EntityPowerUpType::InterrogateInt(const char *thing){	//Responds to "powerup-family", "powerup-level", "powerup-weight", and "ammoadd"
	if(!thing) return 0;
	if(CmpLower(thing, "powerup-family")) return 1;
	if(CmpLower(thing, "powerup-level")) return type_level;	//TODO: return "level" (goodness) of power-up.
	if(CmpLower(thing, "powerup-weight")) return type_weight;	//TODO: return "weight" for chance of thing appearing.
	if(CmpLower(thing, "ammoadd")) return type_ammoadd;
	return 0;
}
float EntityPowerUpType::InterrogateFloat(const char *thing){ //Responds to "healthfix", and "boltonammoadd"
	if(!thing) return 0;
	if(CmpLower(thing, "healthfix")) return type_healthfix;
	if(CmpLower(thing, "boltonammoadd")) return type_boltonammoadd;
	return 0;
}
EntityPowerUp::EntityPowerUp(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntityPowerUpType *TP = (EntityPowerUpType*)TypePtr;
	CanCollide = false;
	ScaleVec3(BoundMax, 2.0f);	//Double collision bounding box.
	ScaleVec3(BoundMin, 2.0f);
	BoundMin[1] -= (POWERUP_FLOAT + 1.0f);
	BoundRad += POWERUP_FLOAT;	//Expand bbox to collide with tanks below.
	Mass = 0.0f;
	Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + POWERUP_FLOAT;
	tankgod = 0;
	//
	VW->AddEntity("explosphere", TP->type_spawnentity, Position, NULL, NULL, 0, 0, 0, 0);
	VW->AddEntity("sound", TP->type_spawnsound, Position, NULL, NULL, 0, 0, 0, 0);
	//This should be a transitory entity.
}
int EntityPowerUp::QueryInt(int type){
	EntityPowerUpType *TP = (EntityPowerUpType*)TypePtr;
	return EntityMesh::QueryInt(type);
}
bool EntityPowerUp::SetInt(int type, int attr){
	return EntityMesh::SetInt(type, attr);
}
bool EntityPowerUp::Think(){
	EntityPowerUpType *TP = (EntityPowerUpType*)TypePtr;
		Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + POWERUP_FLOAT + 0.5f * sin((double)VW->Time() / 500.0f);
		Rotation[1] = (double)VW->Time() / 900.0f;
		if(VW->CheckCollision(this, GROUP_ACTOR)){
			EntityBase *col;
			Vec3 colpnt;
			while(col = VW->NextCollider(colpnt)) CollisionWith(col, colpnt);
		}
		//
		EntityTankGod *god;	//Register self with GOD.
		if(!tankgod){
			god = (EntityTankGod*)FindRegisteredEntity("TANKGOD");
			if(god) tankgod = god->GID;
		}
		if(tankgod && (god = (EntityTankGod*)VW->GetEntity(tankgod))){
			god->UpdatePowerUp(this, TP->type_level);
		}
		//
	return EntityMesh::Think();
}
bool EntityPowerUp::CollisionWith(EntityBase *collider, Vec3 colpnt){
	EntityPowerUpType *TP = (EntityPowerUpType*)TypePtr;
	//Check to see if a tank hit us.
	if(CmpLower(collider->TypePtr->cname, "racetank")){//Tank touched us.
		if(VW->Net.IsClientActive() == false){
				if(!RemoveMe){	//Only if still scheduled for life, thus no double-tanking.
					collider->SetFloat(ATT_HEALTH, collider->QueryFloat(ATT_HEALTH) + TP->type_healthfix);
					collider->SetInt(ATT_AMMO, collider->QueryInt(ATT_AMMO) + TP->type_ammoadd);

					//Do Bolt-On ammo increases.
					EntityBase *w = VW->GetEntity(collider->QueryInt(ATT_WEAPON_ENTITY));
					if(w){
						w->SetInt(ATT_WEAPON_AMMO, w->QueryInt(ATT_WEAPON_AMMO) +
							((float)w->QueryInt(ATT_WEAPON_FULLAMMO) * TP->type_boltonammoadd + 0.5f));
					}
				}
				Remove();
				//
				VW->AddEntity("explosphere", TP->type_outentity, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
				VW->AddEntity("sound", TP->type_pickupsound, Position, NULL, NULL, collider->GID, 0, 0, ADDENTITY_FORCENET);
					//
		}
	}
	return true;
}

//******************************************************************
//**  Spewer Entity  --  Derived Class, spews other entities
//******************************************************************
EntitySpewerType::EntitySpewerType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t){
	type_spewtime = 1.0f;
	type_randomtime = false;
	type_cycle = 0;
	type_numspewtype = 0;
	SetVec3(-10, -10, -10, type_spewmin);
	SetVec3(10, 10, 10, type_spewmax);
	type_propagatespew = true;
	type_maxspewcount = 0;
	type_iterations = 1;
	ClearVec3(type_launchcoords);
	if(cfg){
		if(cfg->FindKey("spewtime")) cfg->GetFloatVal(&type_spewtime);
		if(cfg->FindKey("randomtime")) cfg->GetBoolVal(&type_randomtime);
		if(cfg->FindKey("cycletype")) cfg->GetIntVal(&type_cycle);
		if(cfg->FindKey("spewmin")) cfg->GetFloatVals(&type_spewmin[0], 3);
		if(cfg->FindKey("spewmax")) cfg->GetFloatVals(&type_spewmax[0], 3);
		if(cfg->FindKey("launchcoords")) cfg->GetFloatVals(&type_launchcoords[0], 3);
		for(int i = 0; i < MAXSPEWTYPE; i++){
			if(cfg->FindKey("SpewType" + String(i))){
				cfg->GetStringVal(type_spewtype[i]);
				type_numspewtype = i + 1;
			}
		}
		if(cfg->FindKey("propagatespew")) cfg->GetBoolVal(&type_propagatespew);
		if(cfg->FindKey("iterations")) cfg->GetIntVal(&type_iterations);
		if(cfg->FindKey("maxspewcount")) cfg->GetIntVal(&type_maxspewcount);
	}
	Transitory = false;
}
EntityBase *EntitySpewerType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntitySpewer(this, Pos, Rot, Vel, id, flags);
}
bool EntitySpewerType::CacheResources(){
	if(EntityMeshType::CacheResources()){
		bool result = true;
		for(int i = 0; i < type_numspewtype; i++){
			if(!VW->CacheResources("", type_spewtype[i]) && type_spewtype[i].len() > 2) result = false;
		}
		return ResCached = result;
	}
	return ResCached = false;
}
void EntitySpewerType::ListResources(FileCRCList *fl){
	EntityMeshType::ListResources(fl);
	for(int i = 0; i < type_numspewtype; i++){
		VW->ListResources("", type_spewtype[i]);
		// FIXME: length > 2?
	}
	ResListed = true;
}
EntitySpewer::EntitySpewer(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntitySpewerType *TP = (EntitySpewerType*)TypePtr;
	CanCollide = false;
	lastspewtime = VW->Time();
	lastspewtype = 0;
	spewcount = 0;
	curowner = EntityProjectile::GetCurrentOwner();	//Moment we are created, store tank owner ID, if set by a projectile spawning us.
}
bool EntitySpewer::Think(){
	EntitySpewerType *TP = (EntitySpewerType*)TypePtr;
	//
	if(TP->type_numspewtype > 0 && (VW->Net.IsClientActive() == false || TP->type_propagatespew == false)){
		if( (TP->type_randomtime == false && abs(VW->Time() - lastspewtime) > (TP->type_spewtime * 1000.0f)) ||
			(TP->type_randomtime && ((float)TMrandom() / (float)RAND_MAX) < VW->FrameFrac() / std::max(0.01f, TP->type_spewtime)) ){
			for(int iter = 0; iter < TP->type_iterations; iter++){
				int type = lastspewtype + 1;
				if(TP->type_cycle){
					if(type >= TP->type_numspewtype) type = 0;
				}else{
					type = TMrandom() % TP->type_numspewtype;
				}
				Vec3 vel;
				for(int i = 0; i < 3; i++){
					vel[i] = TP->type_spewmin[i] + ((float)TMrandom() / (float)RAND_MAX) * (TP->type_spewmax[i] - TP->type_spewmin[i]);
				}
				Vec3 tv;
				AddVec3(Position, TP->type_launchcoords, tv);
				//
				EntityProjectile::SetCurrentOwner(curowner);	//This is so if a projectile creates us, any projectiles we create will have same tank owner id.
				VW->AddEntity("", TP->type_spewtype[type], tv, Rotation, vel, 0, 0, NULL, (TP->type_propagatespew ? ADDENTITY_FORCENET : NULL));
				EntityProjectile::SetCurrentOwner(0);
				//
				lastspewtime = VW->Time();
				lastspewtype = type;
				spewcount++;
			}
		}
		if(TP->type_maxspewcount > 0 && spewcount >= TP->type_maxspewcount) Remove();
	}else{
		Remove();
	}
	//
	return EntityMesh::Think();
}
//******************************************************************
//**  Squishy Entity  --  Derived Class, things like Sheep
//******************************************************************
EntitySquishyType::EntitySquishyType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t){
	type_bounce = 5.0f;
	type_bouncemin = 1.0f;
	type_bouncethreshold = 2.0f;
	type_pitchscale = 0.5f;
	type_expandbbox = 1.0f;
	type_bounceevery = 20.0f;
	if(cfg){
		if(cfg->FindKey("bounce")) cfg->GetFloatVal(&type_bounce);
		if(cfg->FindKey("bouncemin")) cfg->GetFloatVal(&type_bouncemin);
		if(cfg->FindKey("bouncethreshold")) cfg->GetFloatVal(&type_bouncethreshold);
		if(cfg->FindKey("pitchscale")) cfg->GetFloatVal(&type_pitchscale);
		if(cfg->FindKey("expandbbox")) cfg->GetFloatVal(&type_expandbbox);
		if(cfg->FindKey("bounceevery")) cfg->GetFloatVal(&type_bounceevery);
		if(cfg->FindKey("bouncesound")) cfg->GetStringVal(type_bouncesound);
	}
//	Transitory = false;
}
EntityBase *EntitySquishyType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntitySquishy(this, Pos, Rot, Vel, id, flags);
}
bool EntitySquishyType::CacheResources(){
	if(!ResCached){
		if(EntityMeshType::CacheResources()){
			if(type_bouncesound.len() > 0) VW->CacheResources("sound", type_bouncesound);
			return ResCached = true;
		}
		return ResCached = false;
	}
	return ResCached;
}
void EntitySquishyType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	EntityMeshType::ListResources(fl);
	if(type_bouncesound.len() > 0) VW->ListResources("sound", type_bouncesound);
	ResListed = true;
}
EntitySquishy::EntitySquishy(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntitySquishyType *TP = (EntitySquishyType*)TypePtr;
	CanCollide = TP->type_collide;
	ScaleVec3(BoundMin, TP->type_expandbbox);
	ScaleVec3(BoundMax, TP->type_expandbbox);
	BoundRad *= TP->type_expandbbox;
	Rotation[1] = ((float)TMrandom() / (float)RAND_MAX) * PI2;
}
bool EntitySquishy::Think(){
	EntitySquishyType *TP = (EntitySquishyType*)TypePtr;
	//
	if(((float)TMrandom() / (float)RAND_MAX) * TP->type_bounceevery < VW->FrameFrac() || VW->CheckCollision(this, GROUP_ACTOR) || Position[1] < WATERHEIGHT){
		if(Velocity[1] <= 1.0f){
			Velocity[0] = (((float)TMrandom() / (float)RAND_MAX) - 0.5f) * TP->type_bounce;
			Velocity[1] = (((float)TMrandom() / (float)RAND_MAX)) * TP->type_bounce + TP->type_bouncemin;
			Velocity[2] = (((float)TMrandom() / (float)RAND_MAX) - 0.5f) * TP->type_bounce;
			//TODO:  Play Baaaah sound.  :)
			if(!VW->Net.IsClientActive()){
				VW->AddEntity("sound", TP->type_bouncesound, Position, NULL, Velocity, 0, 0, 0, ADDENTITY_FORCENET);
			}
		}
		//TODO:  Make shootable?  Switch collision type to passive?  Jump away from tanks?  Flock?
	}
	//
	float grndoff = 0.0f;
	if(TP->mesh) grndoff = -TP->mesh->BndMin[1];
	float y = VW->Map.FGetI(Position[0], -Position[2]) + grndoff;
	if(Position[1] <= y && Velocity[1] <= 0.0f){
		if(Velocity[1] < -fabsf(TP->type_bouncethreshold)){
			Velocity[1] = ((float)TMrandom() / (float)RAND_MAX) * fabsf(Velocity[1]) * 0.5f + fabsf(Velocity[1]) * 0.5f;
		}else{
			ClearVec3(Velocity);
			Position[1] = y;
		}
	}
	//
	float lv = LengthVec3(Velocity);
	Rot3 wRotation;
//	CopyVec3(Rotation, wRotation);
	if(lv > 1.0f){
		wRotation[0] = asinf(-(Velocity[1] / lv)) * TP->type_pitchscale;
		wRotation[1] = atan2f(Velocity[0], Velocity[2]);
		wRotation[2] = 0;
	}else{
		wRotation[0] = wRotation[2] = 0;
		wRotation[1] = Rotation[1];
	}
	//Add damping to rotational changes.
	float tt = 1.0f - DAMP(1.0f, 0.2f, VW->FrameFrac() * 2.0f);
	for(int ii = 0; ii < 3; ii++){
		float d = NormRot(wRotation[ii] - Rotation[ii]);
		Rotation[ii] += d * tt;
	}
	//
	Vec3 accel;
	CopyVec3(VW->gravity, accel);
	float t = VW->FrameFrac();
	for(int i = 0; i < 3; i++){
		Position[i] += Velocity[i] * t + accel[i] * t * t * 0.5f;	//Physics.
		Velocity[i] += accel[i] * t;// * t;
	}
	//
	if(TP->type_collide) Rebucket();
	//
	if(TP->Transitory == 0 && RemoveMe == REMOVE_NONE && VW->Net.IsServerActive()){	//Send state if persistent entity.
		BitPacker<128> pe;
		pe.PackInt(MSG_HEAD_POSITION, 8);
		VW->PackPosition(pe, Position, 15);
		for(int i = 0; i < 3; i++){
	//		pe.PackFloatInterval(Position[i] - VW->worldcenter[i], -1000, 1000, 15);
			pe.PackFloatInterval(Velocity[i], -100, 100, 15);
		}
		QueuePacket(TM_Unreliable, pe, 0.2f);
	}
	return EntityMesh::Think();
}
void EntitySquishy::DeliverPacket(const unsigned char *data, int len){
	if(data[0] == MSG_HEAD_POSITION){
		BitUnpackEngine pe(data + 1, len - 1);
		VW->UnpackPosition(pe, Position, 15);
		for(int i = 0; i < 3; i++){
	//		pe.UnpackFloatInterval(Position[i], -1000, 1000, 15);
	//		Position[i] += VW->worldcenter[i];
			pe.UnpackFloatInterval(Velocity[i], -100, 100, 15);
		}
	}
}

//******************************************************************
//**  Insignia Entity  --  Derived Class, handles team flags
//******************************************************************
EntityInsigniaType::EntityInsigniaType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t){
	type_perturb = 0;
	type_perturbscale = 0;
	type_perturbspeed = 0;
	hflip = 0;
	ClearVec3(type_rotation);
	type_team = 0;
	if(cfg){
		if(cfg->FindKey("perturb")) cfg->GetFloatVal(&type_perturb);
		if(cfg->FindKey("perturbscale")) cfg->GetFloatVal(&type_perturbscale);
		if(cfg->FindKey("perturbspeed")) cfg->GetFloatVal(&type_perturbspeed);
		if(cfg->FindKey("rotation")) cfg->GetFloatVals(&type_rotation[0], 3);
		for(int i = 0; i < 3; i++) type_rotation[i] *= DEG2RAD;
		if(cfg->FindKey("team")) cfg->GetIntVal(&type_team);
		if(cfg->FindKey("hflip")) cfg->GetIntVal(&hflip);
	}
	if(type_perturb > 0.0f){
		type_rendflags |= MESH_PERTURB_NOISE_Z;	//Turn on flag waving.
	}
	//
	type_rendflags |= MESH_SHADE_SOLID;
	//
	Transitory = true;
	type_directmatrix = 1;
}
EntityBase *EntityInsigniaType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityInsignia(this, Pos, Rot, Vel, id, flags);
}
int EntityInsigniaType::InterrogateInt(const char *thing){	//Responds to "TEAM".
	if(thing && CmpLower(thing, "team")){
		return type_team;
	}
	else if(thing && CmpLower(thing, "hflip")) return hflip;

	return EntityMeshType::InterrogateInt(thing);
}
//
EntityInsignia::EntityInsignia(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	EntityInsigniaType *TP = (EntityInsigniaType*)TypePtr;
	CanCollide = false;
	Rot3ToMat3(TP->type_rotation, rotMat);
	teamid = TP->thash;//TP->type_team;
}
bool EntityInsignia::Think(){
	EntityInsigniaType *TP = (EntityInsigniaType*)TypePtr;
	//
	meshobj.PerturbZ = (float)(VW->Time() & 0xfffff) * 0.001f * TP->type_perturbspeed;
	meshobj.Perturb = TP->type_perturb;
	meshobj.PerturbScale = TP->type_perturbscale;
	//
	if(TP->texture) TP->texture->flags &= (~BFLAG_DONTDOWNLOAD);	//This assures that any flag textures actually used in the game WILL be downloaded, even if cached and set to not download by flag-browser GUI entity.
	//
	return EntityMesh::Think();
}
bool EntityInsignia::SetVec(int type, const void *v){
	EntityInsigniaType *TP = (EntityInsigniaType*)TypePtr;
	if(v && (type == ATT_MATRIX3 || type == ATT_MATRIX43)){
		Mat3 tm;
		for(int i = 0; i < 9; i++) ((float*)tm)[i] = ((float*)v)[i];
		Mat3MulMat3(rotMat, tm, meshobj.Orient);	//Especially apply fixed rotation matrix to direct matrix.
		if(TP->type_scalematrixon){
			ScaleVec3(meshobj.Orient[0], TP->type_scalematrix[0]);
			ScaleVec3(meshobj.Orient[1], TP->type_scalematrix[1]);
			ScaleVec3(meshobj.Orient[2], TP->type_scalematrix[2]);
		}
		if(type == ATT_MATRIX43) CopyVec3(((float*)v) + 9, meshobj.Pos);
		return true;
	}
	return EntityMesh::SetVec(type, v);
}
void EntityInsignia::SetPos(Vec3 pos){
	EntityMesh::SetPos(pos);
	CopyVec3(Position, meshobj.Pos);
}
void EntityInsignia::SetRot(Rot3 rot){	//Since we are a directmatrix mesh, this lets someone force an angular based rotation.
	EntityMesh::SetRot(rot);
	Rot3ToMat3(Rotation, meshobj.Orient);
}

//******************************************************************
//**  Tutorial Entity  --  Derived Class, Provides info to end user about specific topics
//******************************************************************
EntityTutorialType::EntityTutorialType(ConfigFile *cfg, const char *c, const char *t) :
	EntityMeshType(cfg, c, t){
	secmesh = secentmesh = NULL;
	ClearVec3(type_secrotmin);
	ClearVec3(type_secrotmax);
	type_secrotwindup = 1.0f;
	ClearVec3(type_secmeshoffset);
	ClearVec3(type_secentoffset);
	descid = -1;
	if(cfg){
		int i;
		for(i = 0; i < MAX_DESC_STRINGS; i++)
		{
			if(cfg->FindKey(CStr("descstring" + String(i))))
				cfg->GetStringVal(DescStrings[i]);
		}
		if(cfg->FindKey("name")) cfg->GetStringVal(NameString);
		if(cfg->FindKey("imagedesc")) cfg->GetStringVal(ImageDesc);
		if(cfg->FindKey("tutid")) cfg->GetIntVal(&TutID);
		if(cfg->FindKey("descid")) cfg->GetIntVal(&descid);
		if(cfg->FindKey("secmesh")) cfg->GetStringVal(secmeshfile);
		if(cfg->FindKey("secmesh1")) cfg->GetStringVal(secmeshfile1);
		if(cfg->FindKey("secmesh2")) cfg->GetStringVal(secmeshfile2);
		if(cfg->FindKey("secrotmin")) cfg->GetFloatVals(&type_secrotmin[0], 3);
		if(cfg->FindKey("secrotmax")) cfg->GetFloatVals(&type_secrotmax[0], 3);
		for(i = 0; i < 3; i++){ type_secrotmin[i] *= DEG2RAD; };
		for(i = 0; i < 3; i++){ type_secrotmax[i] *= DEG2RAD; };
		if(cfg->FindKey("secrotwindup")) cfg->GetFloatVal(&type_secrotwindup);
		if(cfg->FindKey("secmeshoffset")) cfg->GetFloatVals(&type_secmeshoffset[0], 3);
		if(cfg->FindKey("secent")) cfg->GetStringVal(type_secentname);
		if(cfg->FindKey("secentoffset")) cfg->GetFloatVals(&type_secentoffset[0], 3);
	}
	ScaleVec3(type_secmeshoffset, type_scale);
	ScaleVec3(type_secentoffset, type_scale);
	Transitory = true;
}
EntityBase *EntityTutorialType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityTutorial(this, Pos, Rot, Vel, id, flags);
}
CStr EntityTutorialType::InterrogateString(const char *thing){	//Responds to "name"
	if(thing && CmpLower(thing, "name")) return NameString;
	else if(thing && CmpLower(thing, "imagedesc")) return ImageDesc;
	return EntityMeshType::InterrogateString(thing);
}

int EntityTutorialType::InterrogateInt(const char *thing){	//Responds to "tutid"
	if(thing && CmpLower(thing, "tutid")) return TutID;
	return EntityMeshType::InterrogateInt(thing);
}
bool EntityTutorialType::CacheResources(){
	if(!ResCached){
		if(EntityMeshType::CacheResources()){
			Mesh *secmesh1, *secmesh2;
			bool smooth = (type_rendflags & (MESH_SHADE_SMOOTH | MESH_ENVMAP_SMOOTH)) != 0;
			secmesh = VW->GetMesh(secmeshfile, type_scale, smooth);
			secmesh1 = VW->GetMesh(secmeshfile1, type_scale, smooth);
			secmesh2 = VW->GetMesh(secmeshfile2, type_scale, smooth);
			if(texture && mesh && secmesh){
				secmesh->SetTexture(texture);
				secmesh->SetLod(0, type_lodbias, secmesh1);
				if(secmesh1){
					secmesh1->SetTexture(texture);
					secmesh1->SetLod(1, type_lodbias, secmesh2);
					if(secmesh2) secmesh2->SetTexture(texture);
				}
			}
			EntityMeshType *ET = (EntityMeshType*)VW->FindEntityType("mesh", type_secentname);
			if (ET)
			{
				secentmesh = VW->GetMesh(ET->meshfile, type_scale, smooth);
				ET->CacheResources();
			}
		}
	}
	return ResCached;
}
void EntityTutorialType::ListResources(FileCRCList *fl){
	if (ResListed) return;

	EntityMeshType::ListResources(fl);

	fl->FileCRC(secmeshfile);
	fl->FileCRC(secmeshfile1);
	fl->FileCRC(secmeshfile2);

	EntityMeshType *ET = (EntityMeshType*)VW->FindEntityType("mesh", type_secentname);
	if (ET) {
		fl->FileCRC(ET->meshfile);
		ET->ListResources(fl);
	}
}

EntityTutorial::EntityTutorial(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityMesh(et, Pos, Rot, Vel, id, flags) {
	TP = (EntityTutorialType*)TypePtr;
	ClearVec3(secrot);
	secrotlerp = 0.0f;
	secent = 0;
}

bool EntityTutorial::Think()
{
	Mat43 tmpMat;
	Vec3 tmpVec;

	Rot3ToMat3(Rotation, tmpMat);
	Vec3MulMat3(TP->type_meshoffset, tmpMat, tmpVec);	//Push offset position through primary matrix too.
	CopyVec3(Position, tmpMat[3]);	//Make Tutorial Mesh matrix.
	AddVec3(tmpVec, tmpMat[3]);

	if (TP->mesh)
	{
		meshobj.Configure(TP->texture, TP->mesh, tmpMat, tmpMat[3], TP->type_rendflags, fade, TP->mesh->BndRad, 10.0f);
		VW->PolyRend->AddSecondaryObject(&meshobj);
	}
	if(TP->secmesh){
		Vec3 secrotvel;
		LerpVec3(TP->type_secrotmin, TP->type_secrotmax, secrotlerp, secrotvel);	//secrotlerp will go from 0 to 1 to denote speed of rotation.
		ScaleAddVec3(secrotvel, VW->FrameFrac(), secrot);
		//
		Mat3 tm1;
		Rot3ToMat3(secrot, tm1);	//Rot mat from secondary mesh angles.
		Mat3MulMat3(tm1, meshobj.Orient, secmeshobj.Orient);	//Rotate through primary mesh orientation to be child.
		Vec3MulMat3(TP->type_secmeshoffset, meshobj.Orient, secmeshobj.Pos);	//Push offset position through primary matrix too.
		AddVec3(meshobj.Pos, secmeshobj.Pos);
		//
		secmeshobj.Configure(TP->texture, TP->secmesh, secmeshobj.Orient, secmeshobj.Pos, TP->type_rendflags, fade, TP->mesh->BndRad, TP->type_lodbias);
		VW->PolyRend->AddSecondaryObject(&secmeshobj);
	}
	//Do secondary entity.
	if(TP->type_secentname.len() > 0){
		if(TP->secmesh == 0){
			//
			Vec3 secrotvel;	//Duplicate of the above if there is no secmesh.
			LerpVec3(TP->type_secrotmin, TP->type_secrotmax, secrotlerp, secrotvel);	//secrotlerp will go from 0 to 1 to denote speed of rotation.
			ScaleAddVec3(secrotvel, VW->FrameFrac(), secrot);
		}

		EntityMeshType *ET = (EntityMeshType*)VW->FindEntityType("mesh", TP->type_secentname);
		if (ET)
		{
			Mat3 tm1;
			Rot3ToMat3(secrot, tm1);	//Rot mat from secondary mesh angles.
			Mat3MulMat3(tm1, meshobj.Orient, secentmeshobj.Orient);	//Rotate through primary mesh orientation to be child.
			Vec3MulMat3(TP->type_secentoffset, meshobj.Orient, secentmeshobj.Pos);	//Push offset position through primary matrix too.
			AddVec3(meshobj.Pos, secentmeshobj.Pos);

			/*
				The render flags for the ram drill and other sec ents want to modulate with the sky.
				Using the "correct" rendflags causes the textures to modulate with the last used
				texture since there is no sky texture. You can use the TP->type_rendflags instead and
				you'll get the base texture without modulation, but this is a black, ugly texture so it
				isn't used. The current method may cause problems later, but looks better for now.
			*/
			secentmeshobj.Configure(ET->texture, TP->secentmesh, secentmeshobj.Orient, secentmeshobj.Pos, ET->type_rendflags, fade, TP->secentmesh->BndRad, ET->type_lodbias);
			VW->PolyRend->AddSecondaryObject(&secentmeshobj);
		}
	}
	return false;
}

bool EntityTutorial::SetString(int type, const char *s)
{
	if(type == ATT_DOPPELGANG && s){
		EntityTutorialType *tp = (EntityTutorialType*)VW->FindEntityType("tutorial", s);
		if(tp && tp != TP){
			TP->UnlinkResources();
			tp->InterrogateInt("minimal rescache");
			tp->CacheResources();	//Make sure resources for the doppelgangee are cached.
			tp->InterrogateInt("normal rescache");
			ClearVec3(secrot);
			secrotlerp = 0.0f;
			secent = 0;
		}
		TP = tp;
		TP->CacheResources();
		return true;
	}
	return EntityMesh::SetString(type, s);
}

EntityTutorial::~EntityTutorial()
{
	if(VW && secent){
		EntityBase *e = VW->GetEntity(secent);
		if(e) e->Remove();
	}
	TP->UnlinkResources();
}

/*
EntityTargetType::EntityTargetType(ConfigFile *cfg, const char *c, const char *t) :
	EntityRacetankType(cfg, c, t)
{
}

EntityBase*	EntityTargetType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags)
{
	return new EntityTarget(this, Pos, Rot, Vel, id, flags);
}

EntityTarget::EntityTarget(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags)
 : EntityRacetank((EntityRacetankType*)et, Pos, Rot, Vel, id, flags)
{
	Immovable = 1;
}

EntityTarget::~EntityTarget()
{
	if(VW){
		EntityBase *e = VW->GetEntity(baubleentity);
		if(e) e->Remove();
		e = VW->GetEntity(shownameent);
		if(e) e->Remove();
		e = VW->GetEntity(finderid);
		if(e) e->Remove();
	}
}
*/
