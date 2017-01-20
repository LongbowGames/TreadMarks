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
//Implementation for Race Tank entity of Tread Marks.
//

#pragma inline_depth(255)

#include "EntityTank.h"
#include "Basis.h"

#include "TankGame.h"
#include "EntityFlag.h"

#include "BitPacking.h"

#if(_MSC_VER >= 1200)
using namespace std;
#endif

float TurretSpeedScale = 1.0f;

//******************************************************************
//**  Racing Tank Entity  --  Type Handler
//******************************************************************
EntityRacetankType::EntityRacetankType(ConfigFile *cfg, const char *c, const char *t) : EntityMeshType(cfg, c, t) {
	char buf[256];
	turretmesh = gunmesh = NULL;
	engsnd1 = engsnd2 = rumblesnd = NULL;
	type_maxspeed = 20.0f;
	type_accelspeed = 1.0f;
	type_turretspeed = 80.0f;
	crudforce = 1.0f;	//Now just a scalar.
	reloadtime = 3.0f;
	type_baubletype = "bauble1";
	type_healthregen = 0.02f;
	type_armor = 1.3333f;
	onfiretype = "fire1";
	type_ammomax = 16;
	type_turretblowexplo = "explosphere/tealenv1";
	type_tankblowexplo = "tankdeath1";
	type_gravityscale = 1.0f;
	type_frictionscale = 1.0f;
	type_respawnexplo = "TankSpawn";
	type_multikillsound = "multikill";
	type_multikilltime = 20.0f;
	//
	type_center = true;
	for(int i = 0; i < NUM_BOLTONS; i++){
		ClearVec3(type_bolton[i]);	//Read bauble/weapon bolt-on coordinates.
	}
	//
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("turretmesh")) cfg->GetStringVal(turretmeshfile);
		if(cfg->FindKey("gunmesh")) cfg->GetStringVal(gunmeshfile);
		if(cfg->FindKey("turretmesh1")) cfg->GetStringVal(turretmeshfile1);
		if(cfg->FindKey("gunmesh1")) cfg->GetStringVal(gunmeshfile1);
		if(cfg->FindKey("turretmesh2")) cfg->GetStringVal(turretmeshfile2);
		if(cfg->FindKey("gunmesh2")) cfg->GetStringVal(gunmeshfile2);
		if(cfg->FindKey("enginesound1") && cfg->GetStringVal(buf, 256)) engsnd1file = buf;
		if(cfg->FindKey("enginesound2") && cfg->GetStringVal(buf, 256)) engsnd2file = buf;
		if(cfg->FindKey("rumblesound") && cfg->GetStringVal(buf, 256)) rumblesndfile = buf;
		if(cfg->FindKey("maxspeed")) cfg->GetFloatVal(&type_maxspeed);
		if(cfg->FindKey("accelspeed")) cfg->GetFloatVal(&type_accelspeed);
		if(cfg->FindKey("turretspeed"))
		{
			cfg->GetFloatVal(&type_turretspeed);
			type_turretspeed *= TurretSpeedScale;
		}

		if(cfg->FindKey("crudforce")) cfg->GetFloatVal(&crudforce);
		if(cfg->FindKey("smoketype")) cfg->GetStringVal(smoketype);
		if(cfg->FindKey("onfiretype")) cfg->GetStringVal(onfiretype);
		if(cfg->FindKey("projectiletype")) cfg->GetStringVal(projectiletype);
		if(cfg->FindKey("reloadtime")) cfg->GetFloatVal(&reloadtime);
		if(cfg->FindKey("baubletype")) cfg->GetStringVal(type_baubletype);
		if(cfg->FindKey("healthregen")) cfg->GetFloatVal(&type_healthregen);
		if(cfg->FindKey("armor")) cfg->GetFloatVal(&type_armor);
		for(int i = 0; i < NUM_BOLTONS; i++){
		//	ClearVec3(type_bolton[i]);	//Read bauble/weapon bolt-on coordinates.
			if(cfg->FindKey("bolton" + String(i))) cfg->GetFloatVals(&type_bolton[i][0], 3);
		}
		if(cfg->FindKey("ammomax")) cfg->GetIntVal(&type_ammomax);
		if(cfg->FindKey("turretblowexplo")) cfg->GetStringVal(type_turretblowexplo);
		if(cfg->FindKey("tankblowexplo")) cfg->GetStringVal(type_tankblowexplo);
		if(cfg->FindKey("GravityScale")) cfg->GetFloatVal(&type_gravityscale);
		if(cfg->FindKey("FrictionScale")) cfg->GetFloatVal(&type_frictionscale);
		if(cfg->FindKey("respawnexplo")) cfg->GetStringVal(type_respawnexplo);
		if(cfg->FindKey("namesound")) cfg->GetStringVal(type_namesound);
		if(cfg->FindKey("multikillsound")) cfg->GetStringVal(type_multikillsound);
		if(cfg->FindKey("multikilltime")) cfg->GetFloatVal(&type_multikilltime);
	}
	for(int j = 0; j < NUM_BOLTONS; j++){
		ScaleVec3(type_bolton[j], type_scale);
	}
	//Testing!!!!
//	type_baubletype = "baublered";
	//
	minimalrescache = 0;
}
EntityBase *EntityRacetankType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	//Manufactures an entity of type of current entity type object.
	return new EntityRacetank(this, Pos, Rot, Vel, id, flags);
}
bool EntityRacetankType::CacheResources(){
	if(ResCached) return true;
	if(!EntityMeshType::CacheResources()) return false;
	Mesh *turretmesh1, *turretmesh2, *gunmesh1, *gunmesh2;
	bool smooth = (type_rendflags & (MESH_SHADE_SMOOTH | MESH_ENVMAP_SMOOTH)) != 0;
	turretmesh = VW->GetMesh(turretmeshfile, type_scale, smooth);
	gunmesh = VW->GetMesh(gunmeshfile, type_scale, smooth);
	turretmesh1 = VW->GetMesh(turretmeshfile1, type_scale, smooth);
	gunmesh1 = VW->GetMesh(gunmeshfile1, type_scale, smooth);
	turretmesh2 = VW->GetMesh(turretmeshfile2, type_scale, smooth);
	gunmesh2 = VW->GetMesh(gunmeshfile2, type_scale, smooth);
	//
	if(minimalrescache == 0){	//Only cache ancillary entities if minimal cache is off.  This is so doppelgangers can cache racetank resources without caching unneeded crud.
		VW->CacheResources("smoke", smoketype);
		VW->CacheResources("smoke", onfiretype);
		VW->CacheResources("sound", "thump1");
		VW->CacheResources("mesh", "redarrow");
		VW->CacheResources("sound", "hornlow");
		VW->CacheResources("sound", "metal1");
		VW->CacheResources("explo", "smoketiny");
		VW->CacheResources("projectile", projectiletype);
		VW->CacheResources("sound", "reload");
		VW->CacheResources("bauble", type_baubletype);
		VW->CacheResources("explo", type_turretblowexplo);
		VW->CacheResources("explo", type_tankblowexplo);
		VW->CacheResources("explo", type_respawnexplo);
		VW->CacheResources("sound", type_multikillsound);
		//Sounds.
		engsnd1 = VW->GetSound(engsnd1file, true);
		engsnd2 = VW->GetSound(engsnd2file, true);
		rumblesnd = VW->GetSound(rumblesndfile, true);
	}
	//
	if(texture && mesh && turretmesh){
		turretmesh->SetTexture(texture);
		turretmesh->SetLod(0, type_lodbias, turretmesh1);
		if(turretmesh1){
			turretmesh1->SetTexture(texture);
			turretmesh1->SetLod(1, type_lodbias, turretmesh2);
			if(turretmesh2) turretmesh2->SetTexture(texture);
		}
		if(gunmesh){
			gunmesh->SetTexture(texture);
			gunmesh->SetLod(0, type_lodbias, gunmesh1);
			if(gunmesh1){
				gunmesh1->SetTexture(texture);
				gunmesh1->SetLod(1, type_lodbias, gunmesh2);
				if(gunmesh2) gunmesh2->SetTexture(texture);
			}
		}
		return ResCached = true;
	}
	return ResCached = false;
}
void EntityRacetankType::ListResources(FileCRCList *fl){
	EntityMeshType::ListResources(fl);

	fl->FileCRC(turretmeshfile);
	fl->FileCRC(gunmeshfile);
	fl->FileCRC(turretmeshfile1);
	fl->FileCRC(gunmeshfile1);
	fl->FileCRC(turretmeshfile2);
	fl->FileCRC(gunmeshfile2);
	//
//	if(minimalrescache == 0){	//Only cache ancillary entities if minimal cache is off.  This is so doppelgangers can cache racetank resources without caching unneeded crud.
	if (1) {
		VW->ListResources("smoke", smoketype);
		VW->ListResources("smoke", onfiretype);
		VW->ListResources("sound", "thump1");
		VW->ListResources("mesh", "redarrow");
		VW->ListResources("sound", "hornlow");
		VW->ListResources("sound", "metal1");
		VW->ListResources("explo", "smoketiny");
		VW->ListResources("projectile", projectiletype);
		VW->ListResources("sound", "reload");
		VW->ListResources("bauble", type_baubletype);
		VW->ListResources("explo", type_turretblowexplo);
		VW->ListResources("explo", type_tankblowexplo);
		VW->ListResources("explo", type_respawnexplo);
		VW->ListResources("sound", type_multikillsound);
		//Sounds.
		fl->FileCRC(engsnd1file);
		fl->FileCRC(engsnd2file);
		fl->FileCRC(rumblesndfile);
	}
	//
}
void EntityRacetankType::UnlinkResources(){
	EntityMeshType::UnlinkResources();
//	ResCached = false;
//	texture = NULL;	//ResourceManager owns resources, no need to free.
	mesh = turretmesh = gunmesh = NULL;
	engsnd1 = engsnd2 = rumblesnd = NULL;
}
int EntityRacetankType::InterrogateInt(const char *thing){	//Responds to "minimal rescache" and "normal rescache".
	if(thing && CmpLower(thing, "minimal rescache")){ minimalrescache = 1; return 1; }
	if(thing && CmpLower(thing, "normal rescache")){ minimalrescache = 0; ResCached = false; return 1; }
	return EntityMeshType::InterrogateInt(thing);
}
CStr EntityRacetankType::InterrogateString(const char *thing){	//Responds to "namesound".
	if(thing && CmpLower(thing, "namesound")){ return type_namesound; }
	return EntityMeshType::InterrogateString(thing);
}

//******************************************************************
//**  Racing Tank Entity  --  Main Code
//******************************************************************
#define AI_WAYPOINT 1
#define AI_MOVETO 2
EntityRacetank::EntityRacetank(EntityRacetankType *et, Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags)
	: EntityMesh(et, Pos, Rot, Vel, id, flags), vh() {
	accelspeed = et->type_accelspeed;
	maxspeed = et->type_maxspeed;
	turretspeed = et->type_turretspeed;
	//
	//Something's broken with the rotational velocity to make the different time steps different.
//	vh.Init(10.0f, 10.0f, true, VEHICLE_TANK, 0.1f);
	vh.Init(10.0f, 7.5f, true, VEHICLE_TANK, 0.05f);
	//Gravity and friction here are now meaningless.
	//
	vh.Config(1, accelspeed, maxspeed);
//	vh.Config(et->turnspeed, 15.0f, et->maxspeed);
	//
	vh.SetPos(Position);
	vh.SetRot(Rotation);
	//
	ClearVec3(turRot);
	IdentityMat43(tankMat);
	IdentityMat43(turMat);
	Accel = Steer = TurretSteer = TurretSteerVel = 0.0f;
	TankMode = 0;
	LAccel = RAccel = 0;
	Fire = 0;
	smoketime = 0;
	wayentity = 0;	//Mark as not found yet.
	waypoint = 1;	//Start heading to first way after start/finish line.
	godentity = 0;
	laps = 0;
	lastlaps = 0;
	LapTime = RaceTime = LastLapTime = BestLapTime = 0;
	//
	autodrive = false;
	CanCollide = true;
	engsnd1chan = -1;
	engsnd2chan = -1;
	playertank = false;
	motsndfreq = 0.0f;	//Variable for filtering accelerative sound.
	rumblechan = -1;
	rumblevol = 0;
	finderid = 0;
	finderangle = 0;
	lastfiretime = 0;
	targetent = 0;
	retargetdelay = 0;
	ClearVec3(ai_moveto);
	ai_defstate = ai_state = AI_WAYPOINT;
	ai_thinkdelay = ai_statedelay = 0;
	baubleentity = 0;
	armor = et->type_armor;
	health = 1.0f;
	healthregen = et->type_healthregen;
	ammo = 0;	//Start tanks ammoless.
	//
	skill = 1.0f;
	//
	place = 0;
	frags = 0;
	lastfrags = 0;
	deaths = 0;
	//
	ClearVec3(turVel);
	turretattached = 1;
	turretblow = 0;
	turretblowtime = 0;
	ourkiller = 0;	//Last tank to smack us.
	ourkillerweapontype = 0;	//Type hash of weapon killer used.
	//
//	Name = "AI-Tank-" + String((int)(GID & (~ENTITYGID_NETWORK)));	//TODO: Propogate this through net.
	Name = "AI-Tank-" + String((int)(GID & (~ENTITYGID_NETWORK)) % 255);	//TODO: Propogate this through net.  DONE.
	sendreliablepos = 0;
	ping = 0;

	FixedSpawn = 0;
	Immovable = 0;
	SpawnAtFirst = 0;
	//
	meshobj.PerturbX = turretmeshobj.PerturbX = gunmeshobj.PerturbX = 0.0f;	//Reset initial perturb coords.
	meshobj.PerturbZ = turretmeshobj.PerturbZ = gunmeshobj.PerturbZ = 0.0f;
	//
	if(VW->GameMode() & GAMEMODE_DEATHMATCH){
		SetInt(ATT_CMD_FIXTANK, 0);	//Unfix.  This is needed for networked creation.
//		//In case we're not auto-spawned, set to invisible mode.
//		turretattached = 0;
//		turretblowtime = 0;
//		turretblow = 0;
//		fade = 0.0f;
//		CanCollide = false;
		ghost = 1;
	}else{
		ghost = 0;
	}
	insignia1 = 0;
	insignia2 = 0;
	//
	sendstatuscounter = TMrandom() % TANK_STATUS_TIME;
	//
	shownametime = 0;
	shownameent = 0;
	showteamname = 0;
	chatting = 0;
	alivetime = 0;
	timesincelastfrag = 0;
	multikilltimer = 0;

	FlagEntity = 0;
	flag_armorscale = 1;
	flag_speedscale = 1;
	flag_fireratescale = 1;

	nextWayPt = 0;
	AI = new CTankAI(this, 100);
}
EntityRacetank::~EntityRacetank(){
	if(VW){
		EntityBase *e = VW->GetEntity(baubleentity);
		if(e) e->Remove();
		e = VW->GetEntity(shownameent);
		if(e) e->Remove();
		e = VW->GetEntity(finderid);
		if(e) e->Remove();
	}
	delete AI;
}
void EntityRacetank::SetPos(Vec3 pos){
	vh.SetPos(pos);
	EntityBase::SetPos(pos);
}
void EntityRacetank::SetRot(Rot3 rot){
	vh.SetRot(rot);
	EntityBase::SetRot(rot);
}
void EntityRacetank::SetVel(Vec3 vel){
	vh.SetVel(vel);
	EntityBase::SetVel(vel);
}

int EntityRacetank::ReadyToFire()
{
	EntityRacetankType *TP = (EntityRacetankType*)TypePtr;
	if(weaponentity)
	{
		EntityWeapon *ew = (EntityWeapon*)VW->GetEntity(weaponentity);
		EntityWeaponType *et = (EntityWeaponType*)ew->TypePtr;
		if(flag_fireratescale == 0.0f)
			return 0;
		int relms = (int)(et->InterrogateFloat("reloadtime") * 1000.0f * (3.0f - skill * 2.0f) / flag_fireratescale);
		return (ew->QueryInt(ATT_WEAPON_AMMO) > 0 && ew->QueryInt(ATT_WEAPON_LASTFIRETIME) + relms < VW->Time());	//Ve can shoot at dem bastids!!!
	}
	else
	{
		if(flag_fireratescale == 0.0f)
			return 0;
		int relms = (int)(TP->reloadtime * 1000.0f * (3.0f - skill * 2.0f) / flag_fireratescale);
		return (ammo > 0 && lastfiretime + relms < VW->Time());	//Ve can shoot at dem bastids!!!
	}
}

#define SMOKE_INTERVAL 100
//const char SmokeEnts[2][10] = {"gray1", "gray2"};
bool EntityRacetank::Think(){
	EntityRacetankType *TP = (EntityRacetankType*)TypePtr;
	//
	if(playertank){	//Keep trying to register, since if our player's tank is being changed, the old playerentity might be around while we are being created.
		RegisterEntity("PlayerEntity");
	}
	//
	vh.tank = this;
	//
	crashsoundmade = 0;
	//
	if(playertank) skill = 1.0f;
	//
	alivetime += VW->FrameTime();
	//
	//Multi-Kill announcements.
	if ( ! (VW->GameMode() & GAMEMODE_TRAINING)) // no multi kill in training
	{
		timesincelastfrag += VW->FrameTime();
		if(frags > lastfrags){
			if(timesincelastfrag < (TP->type_multikilltime * 1000.0f) || (frags - lastfrags) > 1){	//Catch simultaneous doubles, too!
				multikilltimer = MAX(multikilltimer, 1);
			}
			timesincelastfrag = 0;
		}
		lastfrags = frags;
		if(multikilltimer)
		{	//When non-zero, counts up till half a second is reached, then announces multikill.
			multikilltimer += VW->FrameTime();
			if(multikilltimer > 500)
			{
				VW->AddEntity("sound", TP->type_multikillsound, Position, NULL, NULL, GID, 0, 0, 0);
				multikilltimer = 0;
			}
		}
	}
	//
	if(VW->Time() / 1000 != (VW->Time() + -VW->FrameTime()) / 1000){
		CopyVec3(lastpos2, lastpos3);
		CopyVec3(lastpos1, lastpos2);
		CopyVec3(Position, lastpos1);
	}
	//Locate Tank God.
	EntityTankGod *egod = NULL;
	if(!godentity){
		egod = (EntityTankGod*)FindRegisteredEntity("TANKGOD");
		if(egod) godentity = egod->GID;
	}else{
		egod = (EntityTankGod*)VW->GetEntity(godentity);
	}
	//
	if (autodrive)
		AI->Think(VW->FrameTime());
	//
	//Waypoint following.  First locate GID of waypoint entity.
	if(wayentity == 0){
		EntityBase *e = FindRegisteredEntity("WaypointPath0");
		if(e) wayentity = e->GID;
	}
	float findera = 0;
	if(VW->GameMode() & GAMEMODE_RACE){
		//Now do waypoint following.
		EntityWaypoint *ew = (EntityWaypoint*)VW->GetEntity(wayentity);
		WaypointNode *waypt = NULL, *wayptmajor = NULL;
		if(ew){
			int newway = ew->WalkWaypoints(waypoint, Position, Velocity, &waypt, &wayptmajor);
			if(waypoint == 0 && newway >= 1 && !VW->Net.IsClientActive()){
				laps++;	//Only increment lap counter on server.  It will propagate to clients.
			}
			waypoint = newway;
			if(wayptmajor){	//Finder now points to next checkpoint!  Yes.
				Vec3 t;
				SubVec3(wayptmajor->Pos, Position, t);
				findera = atan2f(t[0], t[2]);
				//
				nextWayPt = wayptmajor->mainid;
				if(egod){// && VW->Net.IsClientActive() == false){	//Update self to God.  For global race-placing and targetting.
					int p = egod->UpdateTank(this, laps, nextWayPt, DistVec3(wayptmajor->Pos, Position), frags, teamid);
					if(VW->Net.IsClientActive() == false) place = p;
				}
				//Will need to make sure we UpdateTank even when in deathmatch mode...
			}
		}
		//Handle lap completion and lap times.
		if(laps > lastlaps && egod && lastlaps < egod->QueryInt(ATT_RACE_LAPS)){
			LastLapTime = LapTime;
			LapTime = 0;
			if(LastLapTime > 0 && (LastLapTime < BestLapTime || BestLapTime <= 0)) BestLapTime = LastLapTime;
			//Add chime sound if player tank.
			if(playertank) VW->AddEntity("sound", "hornlow", Position, NULL, NULL, 0, 0, 0, 0);
		}
	}else{	//Not a race.
		if(egod){
			int p = egod->UpdateTank(this, 0, 0, 0, frags, teamid);
			if(VW->Net.IsClientActive() == false) place = p;
		}
	}
	//
	if(!autodrive) // used for the arrow
	{
        Vec3 ahead;
        ahead[0] = sin(Rotation[1]);
        ahead[1] = 0.0f;
        ahead[2] = cos(Rotation[1]);
		if(egod){
			targetent = egod->ClosestTank(Position, GID, ahead, 1.0f / std::max(0.01f, skill), teamid);
		}
	}
	EntityBase *e = VW->GetEntity(targetent);
	if(e){
		if(playertank){
			e->SetInt(ATT_SHOWNAME, 2000);	//Tell target to display name for 2 seconds.  This is a CLIENT SIDE EFFECT.
			if(teamid != 0){	//Find closest team mate and tell them to show name, too.
				EntityGID g = egod->ClosestTank(Position, GID, NULL, 1.0f, 0, teamid);
				EntityBase *t = VW->GetEntity(g);
				if(t) t->SetInt(ATT_SHOWTEAMNAME, 2000);
			}
		}
		if(VW->GameMode() & GAMEMODE_DEATHMATCH)
		{
			EntityBase *flag = NULL;
			if(egod)
				flag = egod->ClosestFlag(Position, teamid, FlagEntity != 0);

			if(flag)
				e = flag;

			Vec3 tv;
			SubVec3(e->Position, Position, tv);
			float wanta = atan2f(tv[0], tv[2]);
			float cura = turRot[1] + Rotation[1];
			float va = NormRot(wanta - cura);
			findera = wanta;	//Point arrow at nearest enemy in DM.
		}
	}
	///////////////////////////////////////
	//Follow the arrow.
	///////////////////////////////////////
	if(playertank && !autodrive){
		if(finderid == 0){
			EntityBase *e = VW->AddEntity("mesh", "redarrow", NULL, NULL, NULL, 0, 0, 0, 0);
			if(e) finderid = e->GID;
		}else{
			EntityBase *e = VW->GetEntity(finderid);
			if(e){
				float t = NormRot(findera - finderangle);
				finderangle += t * VW->FrameFrac() * 2.0f;	//Filter finder arrow motion.
				Vec3 tv1 = {3, 2, 0}, tv2;
				Vec3MulMat43(tv1, tankMat, tv2);
				e->SetPos(tv2);
				e->SetRot(CVec3(0, finderangle, fmod((double)VW->Time() / 1000.0, (double)PI2)));
				if(health <= 0.0f || ghost){
					e->SetPos(CVec3(1, 1, 1));	//Hide arrow when dead.
				}
			}
		}
	}
	//
	//Set up bounding box for collisions and physics.
	if(TP->mesh){
		CopyVec3(TP->mesh->BndMin, BoundMin);//vh.BndMin);
		CopyVec3(TP->mesh->BndMax, BoundMax);//vh.BndMax);
		if(TP->turretmesh){	//Expand hull bbox to include turret for now.
			BoundMax[1] += (TP->turretmesh->BndMax[1] - TP->turretmesh->BndMin[1]);
		}
		CopyVec3(BoundMin, vh.BndMin);//, BoundMin);
		CopyVec3(BoundMax, vh.BndMax);//, BoundMax);
		BoundRad = TP->mesh->BndRad;
	}
	//
	//Calculate tread gas pedals based on forward/back and steer inputs.
	float ltread = Accel, rtread = Accel;
	if(TankMode && autodrive == false){
		ltread = LAccel;
		rtread = RAccel;
	}else{
		ltread = std::min(std::max(-1.0f, ltread + Steer), 1.0f);
		rtread = std::min(std::max(-1.0f, rtread - Steer), 1.0f);
	}
	//
	if(VW->GameMode() & GAMEMODE_NOCANDRIVE){	//No driving!
		ltread = rtread = 0.0f;
		TurretSteer = TurretSteerVel = 0.0f;
		Fire = 0;
	}else{
		if(VW->GameMode() & GAMEMODE_DEATHMATCH || (egod && laps < egod->QueryInt(ATT_RACE_LAPS))){	//Stop clocks after race completed.
		//	RaceTime += VW->FrameTime();
			if(!VW->Net.IsClientActive()) RaceTime = egod->QueryInt(ATT_RACETIME);	//Pull total race time out of God.
			LapTime += VW->FrameTime();
		}
	}
	//
	////////////////////////////////////
	//Do interpolating physics.
	////////////////////////////////////
	TurretSteerVel = CLAMP(TurretSteer, -1.0f, 1.0f) * turretspeed * DEG2RAD // * VW->FrameFrac()
		* std::min(std::max(health - 0.2f, 0.1f) * 2.5f, 1.0f);	//Start slowing past 60%, lock at 30%.
	//
	float fric = TP->type_frictionscale;
	if(egod) fric *= egod->QueryFloat(ATT_FRICTION);
	vh.SetFriction(fric);
	Vec3 grav;
	ScaleVec3(VW->gravity, TP->type_gravityscale, grav);
	vh.SetGravity(grav);
	if(VW->Net.IsClientActive()){	//Predictive client-side stuff (no collisions!).
		vh.SetCurrentTime(VW->Time());
		vh.SetControlInput(VW->Time(), ltread, rtread, TurretSteerVel);
		//Let's try lerping all the values by 0.5 each frame to smooth out some of the kinks.
		float tR;
		Vec3 P, R, V;
		vh.PredictCurrentTime(P, R, V, &tR, &VW->Map);
		LerpVec3(Position, P, 0.5f, Position);
		LerpVec3(Velocity, V, 0.5f, Velocity);
		float d = NormRot(turRot[1] - tR);
		turRot[1] = tR + (d * 0.5f);
		Vec3 rd;
		SubVec3(Rotation, R, rd);
		NormRot3(rd);
		ScaleAddVec3(rd, 0.5f, R, Rotation);
	}else{
		//Think normally if we are the server.
		if (turretattached || (VW->GameMode() & GAMEMODE_RACE) || (turretblowtime < 5000))
			vh.Think(ltread, rtread, VW->Time(), Position, Rotation, Velocity, &turRot[1], TurretSteerVel, &VW->Map);
	}
	//Turret blow-off.
	if(health <= 0.0f)
	{
		turretblow = 1;
	}
	if(health >= 1.0f) turretblow = 0;
	if(health < 0.0f && FixedSpawn == 1)
		Remove();
	//Separate parts to logic so turretblow can be set through net layer.
	if(turretblow == turretattached){	//If same, turretblow has been changed, so propogate to clients (attach and detach).
		if(!VW->Net.IsClientActive()){	//Propagate blow-off to clients.
			BitPacker<32> pe;
			pe.PackUInt(MSG_HEAD_STATUS, 8);
			pe.PackUInt(0x01, 8);
			pe.PackUInt(!!turretblow, 1);
			QueuePacket(TM_Reliable, pe);
		}
		sendreliablepos = 1;	//Make sure ghost flag is sent.  And health.
	}
	if(turretblow && turretattached){
		ScaleVec3(tankMat[1], 10.0f, turVel);	//Initial turret disconnect.
		AddVec3(Velocity, turVel);	//Add tank's vel too...
		turretblowtime = 0;
		turretattached = 0;
		//
		deaths++;	//Record a death.
		//
		EntityBase *e = VW->GetEntity(ourkiller);
		if(e){	//No self-fragging.  ;)
			int f = 1;
			if(e->QueryInt(ATT_TEAMID) == teamid && teamid != 0) f = 0;	//No frag for killing team mate.
			if(e == this) f = -1;	//Lose a frag for killing self.

			int DisableFrags = 0;

			if(VW->MapCfg.FindKey("DisableFrags"))
				VW->MapCfg.GetIntVal(&DisableFrags);

			if(!DisableFrags)
				e->SetInt(ATT_FRAGS, e->QueryInt(ATT_FRAGS) + f);
		}
		if(egod){
			egod->SetInt(ATT_FRAGMSG_KILLER, ourkiller);
			egod->SetInt(ATT_FRAGMSG_WEAPON, ourkillerweapontype);
			egod->SetInt(ATT_FRAGMSG_KILLEE, GID);
			//
			if(playertank){
				egod->SetInt(ATT_HOWITZERTIME_TOGGLE, 1);	//Activate HowitzerTime!
			}
		}
		ourkiller = 0;
		VW->AddEntity("explo", TP->type_turretblowexplo, Position, Rotation, Velocity, 0, 0, 0, 0);
		if(VW->GameMode() & GAMEMODE_DEATHMATCH){	//Try insta-kill in Deathmatch.
			VW->AddEntity("explo", TP->type_tankblowexplo, Position, Rotation, Velocity, 0, 0, 0, 0);
			CanCollide = false;
		}
		sendreliablepos = 1;	//Make sure ghost flag is sent.
	}
	if(!turretblow && !turretattached){
		turretattached = 1;
	}
	//
	Mat43 tmpMat;//tankMat,
	Rot3ToMat3(Rotation, tankMat);
	CopyVec3(Position, tankMat[3]);	//Make tank matrix.
	Rot3ToMat3(turRot, tmpMat);
	if(TP->mesh) CopyVec3(TP->mesh->Offset, tmpMat[3]);	//Make turret-local matrix.
	else ClearVec3(tmpMat[3]);
	float turretfade = fade;
	if(turretattached){
		Mat43MulMat43(tmpMat, tankMat, turMat);	//Transform by tank matrix.
		if(VW->Net.IsClientActive() == false) ghost = 0;
	}else{
		turretblowtime += VW->FrameTime();
		if(egod && turretblowtime < 10000){	//Less than 10 seconds since.
			float t = VW->FrameFrac();
			for(int i = 0; i < 3; i++){
				turMat[3][i] += turVel[i] * t + egod->gravity[i] * t * t * 0.5f;	//Physics.
				turVel[i] += egod->gravity[i] * t;// * t;
			}
		}
		turretfade = 1.0f - ((float)turretblowtime / 5000.0f);
		if(VW->GameMode() & GAMEMODE_DEATHMATCH){
			ghost = 1;
		}
	}
	//
	//////////////////////////////////////////////////
	//New ghost flag, for ghost tank mode.
	//////////////////////////////////////////////////
	if(ghost){
		fade = 0.0f;
		CanCollide = false;
	}else{
		fade = TP->type_fade;
		CanCollide = true;
	}
	//
	//////////////////////////////////////////////////
	//Perturb model when low on health!
	//////////////////////////////////////////////////
	int rendflags = TP->type_rendflags;
	ImageNode *texture = TP->texture;
		float Ptb = (1.0f - (health / 0.5f)) * 2.0f;	//Wanted perturb.
	//
	EntityBase *weapone = NULL;
	if(weaponentity){
		weapone = VW->GetEntity(weaponentity);
		if(weapone){
			if(weapone->QueryInt(ATT_WEAPON_TRANSTEX)){	//Transmute texture from weapon.
				rendflags = weapone->QueryInt(ATT_RENDFLAGS);
				ImageNode *ttex = NULL;
				if(weapone->QueryVec(ATT_TEXTURE_PTR, &ttex) && ttex){
					texture = ttex;
				}
			}
			if(weapone->QueryFloat(ATT_WEAPON_TRANSPERTURB) > 0.0f){	//Transmute texture from weapon.
				meshobj.PerturbX = turretmeshobj.PerturbX = gunmeshobj.PerturbX = Position[0] * 0.5f;	//Reset initial perturb coords.
				meshobj.PerturbZ = turretmeshobj.PerturbZ = gunmeshobj.PerturbZ = Position[2] * 0.5f;
				float t = weapone->QueryFloat(ATT_WEAPON_TRANSPERTURB);
				Ptb = MAX(Ptb, t);	//Let damage push it higher.
			}
		}
	}
	float PtbD = VW->FrameFrac() * 1.0f;	//Delta perturb change.
	if(meshobj.Perturb > Ptb){	//Clamp so we don't overshoot.
		PtbD = MAX(-PtbD, Ptb - meshobj.Perturb);	//Other direction.
	}else{
		PtbD = MIN(PtbD * 5.0f, Ptb - meshobj.Perturb);
	}
	meshobj.Perturb = MAX(0.0f, meshobj.Perturb + PtbD);	//This is main "sliding perturb", add delta.
	turretmeshobj.Perturb = gunmeshobj.Perturb = meshobj.Perturb;	//Copy others from main.
	meshobj.PerturbScale = turretmeshobj.PerturbScale = gunmeshobj.PerturbScale = 1.0f;
	if(meshobj.Perturb > 0.0f) rendflags |= MESH_PERTURB_NOISE;	//Set flag if we are perturbed.
	//
	//
	int trans;	//Decide if we're transparent or not.  Add opacity checking once there is opacity!
	if(!(rendflags & MESH_BLEND_ADD) && (fade >= 1.0f || (rendflags & MESH_BLEND_ENVMAP))) trans = 0;
	else trans = 1;
	//
	int tankvis = 0;
	if(TP->mesh){
		meshobj.Configure(texture, TP->mesh, tankMat, tankMat[3], rendflags, fade, TP->mesh->BndRad, TP->mesh->LodBias);
		if(trans) tankvis = VW->PolyRend->AddTransObject(&meshobj);
		else tankvis = VW->PolyRend->AddSolidObject(&meshobj);
	}
	if(turretfade > 0.0f){
		if(TP->turretmesh){
			turretmeshobj.Configure(texture, TP->turretmesh, turMat, turMat[3], rendflags, turretfade, TP->turretmesh->BndRad, TP->turretmesh->LodBias);
			if(trans || turretfade < 1.0f) VW->PolyRend->AddTransObject(&turretmeshobj);
			else VW->PolyRend->AddSolidObject(&turretmeshobj);
		}
		if(TP->gunmesh){
			gunmeshobj.Configure(texture, TP->gunmesh, turMat, turMat[3], rendflags, turretfade, TP->gunmesh->BndRad, TP->gunmesh->LodBias);
			if(trans || turretfade < 1.0f) VW->PolyRend->AddTransObject(&gunmeshobj);
			else VW->PolyRend->AddSolidObject(&gunmeshobj);
		}
	}
	//
	//Do name hanging over head.//////////////////////////////
	if((shownametime > VW->Time() || chatting) && tankvis){
		EntityBase *e = VW->GetEntity(shownameent);
		if(!e) e = VW->AddEntity("text", "font1", NULL, CVec3(0.02f, 0.02f, 0), CVec3(0, 0.5f, 1.0f),
			0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY, 0, 0);
		if(e){
			shownameent = e->GID;
			Vec3 tv2, tv1 = {Position[0], Position[1] + 3.0f, Position[2]};	//Push through view matrix to find screen coords.
			Vec3MulMat43(tv1, VW->PolyRend->view, tv2);
			if(tv2[2] > 0.0f){
				float iz = (VW->PolyRend->Cam.viewplane / tv2[2]);
				tv2[0] = 0.5f + (tv2[0] * iz) / (MAX(1.0f, VW->PolyRend->Cam.viewwidth) * 1.0f);	//Do z divide to get screen coords.
				tv2[1] = 0.5f - (tv2[1] * iz) / (MAX(1.0f, VW->PolyRend->Cam.viewheight) * 1.0f);
				e->SetPos(tv2);
				if(chatting){
					e->SetVel(CVec3(0.8f, 0.8f, 0.8f));
				}else if(showteamname){
					e->SetVel(CVec3(0, 1.0f, 0.0f));
				}else{
					e->SetVel(CVec3(1, 0.0f, 0.0f));
				}
				e->SetFloat(ATT_TEXT_OPACITY, 0.5f);
				e->SetString(ATT_TEXT_STRING, CStr(chatting ? "[CHAT] " : "") + Name);
			}
		}
	}else{	//Don't need name entity anymore.
		EntityBase *e = VW->GetEntity(shownameent);
		if(e){
			e->Remove();
			shownameent = 0;
		}
	}

	// Do Flag ///////////////////////////////////////////////
	flag_armorscale = 1;
	flag_fireratescale = 1;
	flag_speedscale = 1;
	if(FlagEntity)
	{
		EntityBase *FlagE = VW->GetEntity(FlagEntity);
		if(FlagE)
		{
			if(health <= 0.0f)
			{
				FlagE->SetInt(ATT_WEAPON_STATUS, 0); // drop it
				FlagEntity = 0;
			}
			else
			{
				flag_armorscale = FlagE->QueryFloat(ATT_FLAG_ARMORSCALE);
				flag_fireratescale = FlagE->QueryFloat(ATT_FLAG_FIRERATESCALE);
				flag_speedscale = FlagE->QueryFloat(ATT_FLAG_SPEEDSCALE);
			}
		}
		else
			FlagEntity = 0; // lost the entity
	}
	//
	//Do Bauble.//////////////////////////////////////////////
	if(baubleentity == 0){
		EntityBase *e = VW->AddEntity("bauble", TP->type_baubletype, NULL, NULL, NULL, 0, 0, 0, 0);
		if(e) baubleentity = e->GID;
	}
	if(baubleentity && TP->turretmesh){
		EntityBase *e = VW->GetEntity(baubleentity);
		if(e){
			Vec3 tv2;
			Vec3MulMat43(TP->type_bolton[0], turMat, tv2);
			e->SetPos(tv2);
			Mat3ToRot3(turMat, tv2);
			e->SetRot(tv2);
			e->SetInt(ATT_INSIGNIA1_HASH, insignia1);
			e->SetInt(ATT_INSIGNIA2_HASH, insignia2);
			e->SetInt(ATT_INSIGNIA3ID, FlagEntity);
			//
			teamid = e->QueryInt(ATT_TEAMID);	//Read back team ident from bauble.
		}else{
			baubleentity = 0;
		}
	}
	if(baubleentity == 0){
		OutputDebugLog("*** Tank \"" + Name + "\" with type " + TP->tname + " has no Bauble entity!  Baubletype = " + TP->type_baubletype + "\n");
	}
	//
	//Bolt-On Weapons.
	if(weaponentity){
		if(!weapone){
			weaponentity = 0;	//Whoops, no weapon.
			lastfiretime = VW->Time() + 1000 - (TP->reloadtime * 1000.0f);	//Prevent insta-shot after bolt-on loss.
		}else{
			int bolt = weapone->QueryInt(ATT_WHICH_BOLTON);
			bolt = std::min(std::max(0, bolt), NUM_BOLTONS - 1);
			Vec3 tv, tr;
			if(bolt < NUM_TURRET_BOLTONS){
				Vec3MulMat43(TP->type_bolton[bolt], turMat, tv);
				Mat3ToRot3(turMat, tr);
			}else{
				Vec3 tb;
				CopyVec3(TP->type_bolton[bolt], tb);
				if(TP->mesh) AddVec3(TP->mesh->Offset, tb);
				Vec3MulMat43(tb, tankMat, tv);
				Mat3ToRot3(tankMat, tr);
			}
			if(weapone->QueryInt(ATT_WEAPON_STATUS) == GID){	//Only set weapon pos if weapon points to us, to fix bad client weapon positions after being dropped from exploding tank.
				weapone->SetPos(tv);
				weapone->SetRot(tr);
				weapone->SetInt(ATT_FIRE, Fire);
			}
			if(!turretattached){
				weapone->SetInt(ATT_WEAPON_STATUS, 0); // drop it
				weaponentity = 0;
			}
		}
	}
	//
	//
	//Do weapon fire, now that turret matrix is made so tip of gun is known.
	if(flag_fireratescale != 0.0f)
	{
		int relms = (int)(TP->reloadtime * 1000.0f * (3.0f - skill * 2.0f) / flag_fireratescale);
		int relms2 = relms - 1000;
		if(relms2 > 200 && lastfiretime != 0 && lastfiretime + relms2 < VW->Time() && lastfiretime + relms2 >= VW->Time() - VW->FrameTime()){
			if(playertank && ammo > 0) VW->AddEntity("sound", "reload", Position, NULL, NULL, 0, 0, 0, 0);	//Reloading sound.
		}
		if(ammo > 0 && lastfiretime + relms < VW->Time() && Fire && !weaponentity && turretattached){	//Ve can shoot at dem bastids!!!
			if(TP->turretmesh && VW->Net.IsClientActive() == false && (VW->GameMode() & GAMEMODE_NOCANSHOOT) == 0){
				//Don't shoot if we're a client.
				Vec3 tv = {0, 0, TP->turretmesh->BndMax[2]}, v, p;
				if(TP->gunmesh) tv[2] = TP->gunmesh->BndMax[2];
				Vec3MulMat3(tv, turMat, v);
				AddVec3(v, turMat[3], p);

				NormVec3(v);

				EntityBase *etarget = VW->GetEntity(targetent);
				if(etarget)
				{
					Vec3 targetvec;
					SubVec3(etarget->Position, p, targetvec);
					NormVec3(targetvec);
				}

				EntityBase *ew = VW->AddEntity("projectile", TP->projectiletype, p, NULL, v, GID, 0, 0, ADDENTITY_FORCENET);
				//Projectile will instead suck teamid out of owner.
				lastfiretime = VW->Time();
				ammo--;
			}
		}
	}
	//
	//Continuously looping engine sounds.
	//The little if hack will make non-player tanks only use one sound sample.
	if(TP->engsnd1 && TP->engsnd2 && alivetime > 1000){	//Wait till 1000ms into game.
		if(engsnd1chan < 0 && (playertank || (GID & 1) == 1)){
			VW->snd.PlayBuffer(*TP->engsnd1, (playertank ? 1.0f : 0.01f));
			engsnd1chan = VW->snd.GetLastVoice();
		}
		if(engsnd2chan < 0 && (playertank || (GID & 1) == 0)){
			VW->snd.PlayBuffer(*TP->engsnd2, (playertank ? 1.0f : 0.01f));
			engsnd2chan = VW->snd.GetLastVoice();
		}
		float t1 = (vh.LinearVelocity / (vh.MaxSpeed));
		float t2 = (fabsf(ltread) + fabsf(rtread)) * 0.5f;
		t2 = motsndfreq = LERP(motsndfreq, t2, 1.0f - DAMP(1.0f, 0.2f, VW->FrameFrac()));	//Go to 80% after 1 sec.
		float t = t1 * 0.5f + t2 * 0.5f;
		t1 = std::min(t1, 1.0f);
		t = std::min(1.0f, t);
		float e1p, e1v, e2p, e2v;
		e1p = 0.5f + t1 * 1.4f;
		e2p = 0.5f + t * 1.4f;
		e1v = (1.0f + t) * 0.5f;// - t;
		e2v = std::min(1.0f, t * 2.0f);
		if(playertank){ e1v += 1.0f;  e2v += 1.0f; }
		//
		if(ghost) e1v = e2v = 0.0f;	//Turn off engines when ghost.
		//
		if(engsnd1chan >= 0){
			if(VW->snd.GetVoiceStatus(engsnd1chan) == 0 || VW->snd.GetVoiceWave(engsnd1chan) != *TP->engsnd1)
				engsnd1chan = -1;
			else
				VW->snd.SetVoice3D(engsnd1chan, Position, Velocity, MIN(1.0f, e1v), e1p, (playertank ? 1.0f : 0.01f));
		}
		if(engsnd2chan >= 0){
			if(VW->snd.GetVoiceStatus(engsnd2chan) == 0 || VW->snd.GetVoiceWave(engsnd2chan) != *TP->engsnd2)
				engsnd2chan = -1;
			else
				VW->snd.SetVoice3D(engsnd2chan, Position, Velocity, MIN(1.0f, e2v), e2p, (playertank ? 1.0f : 0.01f));
		}
	}//Start rumbling sound, to be volume adjusted later.
	if(TP->rumblesnd && rumblechan < 0 && playertank && alivetime > 1000){
		VW->snd.PlayBuffer(*TP->rumblesnd, (playertank ? 1.0f : 0.01f));
		rumblechan = VW->snd.GetLastVoice();
		VW->snd.SetVoice3D(rumblechan, Position, Velocity, 0.0f, 1.0f, (playertank ? 1.0f : 0.2f));
	}
	//
	//Handle damage regeneration.
	if(health < 1.0f && VW->GameMode() & GAMEMODE_RACE){	//Only do it in race mode.
		if (turretattached)
			health += healthregen * VW->FrameFrac();	//Health points per second to restore.
		else
			health += healthregen * VW->FrameFrac() * 2.0f;	// regen twice as fast when turret is blown off
	}
	health = std::max(0.0f, std::min(1.0f, health));
	vh.Config(1, accelspeed * (health * 0.01f + 0.99f), maxspeed * (health * 0.5f + 0.5f) * (skill * 0.5f + 0.5f) * flag_speedscale);	//Make damage felt.
	//
	//Smoke.
	float rumblev = 0;
	if(smoketime < VW->Time() && !(VW->FrameFlags & THINK_NOEFFECTS)
		&& !(!turretattached && VW->GameMode() & GAMEMODE_DEATHMATCH) && ghost == 0){// && Accel != 0.0f){// && 0){	//Temporarily disabled!
		//
		//Damage induced smoke/fire.
		if(health < 0.7f){
			Vec3 tp1 = {0.0f, vh.BndMax[1] * 0.8f, 0.0f}, tp2;
			Vec3MulMat43(tp1, tankMat, tp2);
			if(health > 0.4f || Position[1] < WATERHEIGHT){	//New, no fire if under water!
				VW->AddEntity("smoke", TP->smoketype, tp2, NULL, CVec3(0.0f, 4.0f, 0.0f), 1000, NULL, 0, 0);
			}else{
		//	if(health < 0.6f){
				tp2[0] += ((float)TMrandom() / (float)RAND_MAX) * 0.2f;
				tp2[1] += 0.1f;
				tp2[2] += ((float)TMrandom() / (float)RAND_MAX) * 0.2f;
				VW->AddEntity("smoke", TP->onfiretype, tp2, NULL, CVec3(0.0f, 3.0f, 0.0f), 1000, NULL, 0, 0);
				if(!playertank){	//Hacky extra fire for non-player with slower smoke.
					tp2[1] -= 0.3f;
					ScaleAddVec3(Velocity, 0.1f, tp2);
					VW->AddEntity("smoke", TP->onfiretype, tp2, NULL, CVec3(0.0f, 3.0f, 0.0f), 1000, NULL, 0, 0);
				}
			}
		}
		//
		//
		Vec3 sm1 = {vh.BndMin[0] + 0.5f, vh.BndMin[1] + 1.0f, vh.BndMin[2] - 0.5f};
		Vec3 sm2 = {vh.BndMax[0] - 0.5f, vh.BndMin[1] + 1.0f, vh.BndMin[2] - 0.5f};
		Vec3 tp;
		int tdx, tdy, ttx, tty;
		//
		Vec3MulMat43(sm1, tankMat, tp);
		if(fabsf(ltread) > 0.5f)
			VW->AddEntity("smoke", TP->smoketype, tp, NULL, CVec3(0.0f, 2.0f, 0.0f), 1000, NULL, 0, 0);
		//
		CopyVec3(vh.WP[vh.BackLeft], tp);
		ttx = tp[0];
		tty = -tp[2];
		if(vh.InGround[vh.BackLeft]){//vh.LeftOnGround > 0.0f && vh.RearOnGround > 0.0f){
			if(!(VW->FrameFlags & THINK_NOTRACKS)){
				tdx = std::min(std::max(-10, ltreadx - ttx), 10);
				tdy = std::min(std::max(-10, ltready - tty), 10);
				VW->TreadMark(0.9f, ttx, tty, tdx, tdy);
			}
		}
		ltreadx = ttx;
		ltready = tty;
		//
		Vec3MulMat43(sm2, tankMat, tp);
		if(fabsf(rtread) > 0.5f)
			VW->AddEntity("smoke", TP->smoketype, tp, NULL, CVec3(0.0f, 2.0f, 0.0f), 1000, NULL, 0, 0);
		//
		CopyVec3(vh.WP[vh.BackRight], tp);
		ttx = tp[0];
		tty = -tp[2];
		if(vh.InGround[vh.BackRight]){
			if(!(VW->FrameFlags & THINK_NOTRACKS)){
				tdx = std::min(std::max(-10, rtreadx - ttx), 10);
				tdy = std::min(std::max(-10, rtready - tty), 10);
				VW->TreadMark(0.9f, ttx, tty, tdx, tdy);
			}
		}
		rtreadx = ttx;
		rtready = tty;
		//
		int thud = 0;
		float rumbleinc = 1.0f / (float)vh.GroundWheelPoints;
		//Launch crud from wheel points if lotsa speed change.
		if(egod){
			for(int w = 0; w < vh.GroundWheelPoints; w++){
				if(vh.InGround[w]){
					int eco = VW->Map.GetTwrap(vh.WP[w].x, -vh.WP[w].z);
					float c1 = egod->QueryFloat(ATT_ECO_CRUD + eco) * TP->crudforce;	//Find specified crud spewing speed from sky ent.
					float c2 = c1 * 0.4f;
					if((fabsf(vh.AccelWP[w].z) > c1 ||
						fabsf(vh.AccelWP[w].x) > c2 ||
						vh.WP[w].y < WATERHEIGHT)){	//Last is "in water".
						//
						Vec3 tv = {vh.WP[w].x, std::max(WATERHEIGHT * 0.5f, vh.WP[w].y), vh.WP[w].z};
						//
						VW->AddEntity("crud", egod->QueryString(ATT_ECO_CRUD + eco), tv, NULL,
							CVec3(0.0f, std::max(2.0f, vh.LinearVelocity * 0.2f), 0.0f), 0, NULL, 0, 0);
						rumblev += rumbleinc;
					}
				}
			}
		}
		if(fabsf(LengthVec3(LastVel) - LengthVec3(Velocity)) > TP->crudforce * 2.0f) thud = 1;
		//Crash into ground sounds.
		if(thud && thudtime > 1000 && playertank){
			VW->AddEntity("sound", "thump1", Position,
				CVec3(LengthVec3(LastVel) / (maxspeed * 0.5f), 0, 0), NULL, 0, 0, 0, 0);
			thudtime = 0;
		}
		//Adjust crud rumble volume.
		rumblevol = LERP(rumblevol, rumblev, 0.2f);
		//
		CopyVec3(Velocity, LastVel);
		//
		smoketime = smoketime + (SMOKE_INTERVAL * std::max((2 - playertank), (VW->FrameFlags & THINK_LIGHTSMOKE ? 4 : 1) )) + TMrandom() % 20 - 10;
		smoketime = std::max(smoketime, VW->Time());
	}
	//Adjust crud rumble volume.
	if(rumblechan >= 0){
		if(ghost) rumblevol = 0.0f;
		if(VW->snd.GetVoiceStatus(rumblechan) == 0 || VW->snd.GetVoiceWave(rumblechan) != *TP->rumblesnd)
			rumblechan = -1;
		else
			VW->snd.SetVoice3D(rumblechan, Position, Velocity, MIN(1.0f, rumblevol * 0.8f), 1.0f, (playertank ? 1.0f : 0.2f));
	}
	//
	//
	if(!turretattached && VW->GameMode() & GAMEMODE_DEATHMATCH && turretblowtime > 5000){
		if((Fire || turretblowtime > 30000 || autodrive == true || FixedSpawn == 1)){	//Time to respawn.  When fire pressed, or 30 seconds.
			SetInt(ATT_CMD_SPAWN, 1);	//Respawn self.
			if(!VW->Net.IsClientActive()){	//Propagate blow-off to clients.
				//
				BitPacker<32> pe;
				pe.PackUInt(MSG_HEAD_STATUS, 8);
				pe.PackUInt(0x02, 8);
				pe.PackUInt(1, 1);
				QueuePacket(TM_Reliable, pe);
				//
				//Note, !Client means _either_ dedicated server or single player.  client as in Dumb Client.
				VW->AddEntity("explo", TP->type_respawnexplo, Position, Rotation, CVec3(0, 0, 0), 0, 0, 0, ADDENTITY_FORCENET);
			}
		}
		if(playertank && egod){
			egod->SetInt(ATT_CMD_STATUS_RESPAWN, 1);
		}
	}
	//
	//Recalc collision bucketing.
	Rebucket();
	//
	thudtime = std::max(0, thudtime + VW->FrameTime());	//Keeps thuds from coming too fast.
	//
	lastlaps = laps;
	//
	//Send network data packet.
	if(VW->Net.IsServerActive()){
		BitPacker<128> bp;
		//
		bp.PackInt(MSG_HEAD_POSITION, 8);
		bp.PackUInt(0xff, 8);
		int i;
		//
		bp.PackUInt((VW->Time() / vh.FractionMS) * vh.FractionMS + (vh.FractionMS / 2), 32);
		//Write a value in the middle of the current physics step for better placement on client.
		//
	//	for(i = 0; i < 3; i++){ bp.PackFloatInterval(vh.Pos[i] - VW->worldcenter[i], -1000, 1000, 17); }
		//
		VW->PackPosition(bp, vh.Pos, 17);
		//
		for(i = 0; i < 3; i++){ bp.PackFloatInterval(vh.Vel[i], -1000, 1000, 17); }
		Rot3 rot, rotvel;
		Mat3ToRot3(vh.RotM, rot);
		Mat3ToRot3(vh.RotVelM, rotvel);
		for(i = 0; i < 3; i++){ bp.PackFloatInterval(rot[i], -4, 4, 17); }
		for(i = 0; i < 3; i++){ bp.PackFloatInterval(rotvel[i], -4, 4, 17); }
		bp.PackFloatInterval(vh.TurRot, -8, 8, 11);
		bp.PackFloatInterval(TurretSteerVel, -3.0f, 3.0f, 8);
		bp.PackFloatInterval(Accel, -2, 2, 11);
		bp.PackFloatInterval(Steer, -2, 2, 11);
		bp.PackFloatInterval(health, -1, 10, 11);
		bp.PackUInt(ammo, 6);
		bp.PackUInt(vh.InGroundBits, 16);
		bp.PackUInt(!!chatting, 1);
		//
		//This is an UGLY HACK to keep Test 2.1 compatible with Test 2.  This should be shuffled back to normal location as soon as network version is revved.
		//
		//TODO:  Ugly Hack!  Send only every second or so!
		//
	//	if(sendreliablepos){	//Sends one reliable update, which burns through random skipping of distant updates.
		QueuePacket((sendreliablepos ? TM_Reliable : TM_Unreliable), bp, (VW->GameMode() & GAMEMODE_NOCANDRIVE ? 0.1f : 1.0f));
		//Every 1-second less-important status packet.
		sendstatuscounter -= VW->FrameTime();
		if(sendstatuscounter <= 0 || sendreliablepos){
			sendstatuscounter = (TANK_STATUS_TIME / 2) + (TMrandom() % TANK_STATUS_TIME);
			//
			bp.Reset();	//Reset and send new packet.
			bp.PackUInt(MSG_HEAD_STATUS, 8);
			bp.PackUInt(0x04 | 0x08 | 0x10 | 0x20 | 0x40, 8);
			bp.PackUInt(insignia1, 32);
			bp.PackUInt(insignia2, 32);
			bp.PackUInt(FlagEntity, 32);
			bp.PackUInt(ghost, 1);
			bp.PackUInt(laps, 6);
			bp.PackUInt(place, 8);
			bp.PackUInt(frags, 8);
			bp.PackUInt(waypoint, 8);
			bp.PackUInt(ping, 16);
			//
			bp.PackUInt(RaceTime / 1000, 16);
			bp.PackUInt(LapTime / 1000, 10);
			bp.PackUInt(LastLapTime / 1000, 10);
			bp.PackUInt(BestLapTime / 1000, 10);
			//
			QueuePacket((sendreliablepos ? TM_Reliable : TM_Unreliable), bp, 10.0f);	//High priority.
		}
		sendreliablepos = 0;
	}
	//
	return true;
}
//Process incoming packets directed at this entity.
void EntityRacetank::DeliverPacket(const unsigned char *data, int len){
	if(data){
		if(data[0] == MSG_HEAD_PLAYERENT && len >= 2){
			SetInt(ATT_PLAYERTANK, data[1]);
			return;
		}
		if(data[0] == MSG_HEAD_POSITION){
			BitUnpackEngine bp(data + 2, len - 2);
			//
			Vec3 pos, vel;
			Rot3 rot, rotvel;
			Mat3 rotm, rotvelm;
			int i;
			unsigned int time;
			bp.UnpackUInt(time, 32);
			//
		//	for(i = 0; i < 3; i++){
		//		bp.UnpackFloatInterval(pos[i], -1000, 1000, 17);
		//		pos[i] += VW->worldcenter[i];
		//	}
			//
			VW->UnpackPosition(bp, pos, 17);
			//
			for(i = 0; i < 3; i++){ bp.UnpackFloatInterval(vel[i], -1000, 1000, 17); }
			for(i = 0; i < 3; i++){ bp.UnpackFloatInterval(rot[i], -4, 4, 17); }
			for(i = 0; i < 3; i++){ bp.UnpackFloatInterval(rotvel[i], -4, 4, 17); }
			Rot3ToMat3(rot, rotm);
			Rot3ToMat3(rotvel, rotvelm);
			//
			float ServSteer, ServAccel, ServTurretSteerVel, ServTurRot;
			//
			bp.UnpackFloatInterval(ServTurRot, -8, 8, 11);
		//	bp.UnpackFloatInterval(ServTurretSteerVel, -1.1f, 1.1f, 8);
			bp.UnpackFloatInterval(ServTurretSteerVel, -3.0f, 3.0f, 8);
		//	if(fabsf(turRot[1]) <= 0.1) turRot[1] = 0.0f;
			bp.UnpackFloatInterval(ServAccel, -2, 2, 11);
			bp.UnpackFloatInterval(ServSteer, -2, 2, 11);
			bp.UnpackFloatInterval(health, -1, 10, 11);
		//	bp.UnpackUInt(laps, 6);
			bp.UnpackUInt(ammo, 6);
		//	bp.UnpackUInt(place, 8);	//Handled in less-often status message now.
		//	bp.UnpackUInt(frags, 8);
			int igb = 0;
			bp.UnpackUInt(igb, 16);
			//
			chatting = 0;
			bp.UnpackUInt(chatting, 1);	//Client externally tells server when it's chatting, on server, SetInt() sets that tank's chat bit, then server tank propagates chat bit to all clients for display.
			//
			//
			//This is an UGLY HACK to keep Test 2.1 compatible with Test 2.  This should be shuffled back to normal location as soon as network version is revved.
		//	bp.UnpackFloatInterval(ServTurretSteerVel, -3.0f, 3.0f, 8);
			//
			float ltread = ServAccel, rtread = ServAccel;
			ltread = std::min(std::max(-1.0f, ltread + ServSteer), 1.0f);
			rtread = std::min(std::max(-1.0f, rtread - ServSteer), 1.0f);
			//
			vh.SetAuthoritativeState(time - VW->ClientTimeOffset(), pos, vel, rotm, rotvelm,
				ltread, rtread, ServTurRot, ServTurretSteerVel, igb);
			//
			if(!playertank){	//Keep short-cuircuting of controls working for player driven tank on client.
				Steer = ServSteer;
				Accel = ServAccel;
			//	TurretSteer =
				//Hmm, this might just keep turret steering being a little jumpy on non-player-tanks on the client.
				//Problem is the difference between Control Input, TurretSteer, and rotational speed, TSVel...
			}
			//
			//TODO: Ugly Hack!  Send this only every second or so!!
		//	int flag = 0;
		//	bp.UnpackUInt(flag, 1);
		//	if(flag){
		//		bp.UnpackUInt(insignia1, 32);
		//		bp.UnpackUInt(insignia2, 32);
		//	}
			//
			return;
		}
		if(data[0] == MSG_HEAD_STATUS && len >= 2){	//Generic, flag controlled status message.
			BitUnpackEngine pe(&data[1], len - 1);
			int flag = 0, t = 0;
			pe.UnpackUInt(flag, 8);
			if(flag & 0x01) pe.UnpackUInt(turretblow, 1);
			if(flag & 0x02) pe.UnpackUInt(t, 1); if(t) SetInt(ATT_CMD_FIXTANK, 1);
			if(flag & 0x04){
				pe.UnpackUInt(insignia1, 32);
				pe.UnpackUInt(insignia2, 32);
				pe.UnpackUInt(FlagEntity, 32);
			}
			if(flag & 0x08) pe.UnpackUInt(ghost, 1);
			if(flag & 0x10){
				pe.UnpackUInt(laps, 6);
				pe.UnpackUInt(place, 8);
				pe.UnpackUInt(frags, 8);
				pe.UnpackUInt(waypoint, 8);
			//	OutputDebugString("Status for tank " + String((int)GID) + " says place = " + String(place) + ".\n");
			}
			if(flag & 0x20) pe.UnpackUInt(ping, 16);
			if(flag & 0x40){
				if(pe.UnpackUInt(RaceTime, 16)) RaceTime *= 1000;
				if(pe.UnpackUInt(LapTime, 10)) LapTime *= 1000;
				if(pe.UnpackUInt(LastLapTime, 10)) LastLapTime *= 1000;
				if(pe.UnpackUInt(BestLapTime, 10)) BestLapTime *= 1000;
			}
		//	if(data[1] & 0x01 && len >= 3) turretblow = data[2];	//Send this Reliable!
		//	if(data[1] & 0x02 && len >= 4 && data[3]) SetInt(ATT_CMD_FIXTANK, 1);
			return;
		}
		if(data[0] == MSG_HEAD_NAME && len >= 2){
			BitUnpackEngine bp(data + 1, len - 1);
			bp.UnpackString(Name, 7);
			return;
		}
	}
}
//Send our name along when spawned on clients.
int EntityRacetank::ExtraCreateInfo(unsigned char *data, int len){
	BitPackEngine bp(data + 1, len - 1);
	data[0] = MSG_HEAD_NAME;
	bp.PackString(Name, 7);
	return bp.BytesUsed() + 1;
}
bool EntityRacetank::SetFloat(int type, float attr){
	if(type == ATT_ACCEL){ Accel = CLAMP(attr, -1.0f, 1.0f); return true; }
	if(type == ATT_LACCEL){ LAccel = CLAMP(attr, -1.0f, 1.0f); return true; }
	if(type == ATT_RACCEL){ RAccel = CLAMP(attr, -1.0f, 1.0f); return true; }
	if(type == ATT_STEER){ Steer = CLAMP(attr, -1.0f, 1.0f); return true; }
	if(type == ATT_TURRETSTEER){ TurretSteer = CLAMP(attr, -1.0f, 1.0f); return true; }
	if(type == ATT_TURRETHEADING){ turRot[1] = attr; return true; }
	if(type == ATT_HEALTH){ health = std::min(1.0f, attr); return true; }
	if(type == ATT_SKILL){ skill = CLAMP(attr, 0.0f, 1.0f); return true; }
	return EntityMesh::SetFloat(type, attr);
}
float EntityRacetank::QueryFloat(int type){
	if(type == ATT_HEALTH) return health;
	if(type == ATT_MAXSPEED) return maxspeed;
	if(type == ATT_ACCELSPEED) return accelspeed;
	if(type == ATT_TURRETHEADING) return turRot[1];
	if(type == ATT_SKILL) return skill;
	if(type == ATT_TANK_FIRERATESCALE) return flag_fireratescale;
	return EntityMesh::QueryFloat(type);
}
bool EntityRacetank::SetInt(int type, int attr){
	EntityRacetankType *TP = (EntityRacetankType*)TypePtr;
	if(type == ATT_CHATTING){ chatting = attr; return true; }
	if(type == ATT_SHOWNAME){ shownametime = VW->Time() + attr; showteamname = 0; return true; }
	if(type == ATT_SHOWTEAMNAME){ shownametime = VW->Time() + attr; showteamname = 1; return true; }
	if(type == ATT_PING){ ping = attr; return true; }
	if(type == ATT_INSIGNIA1_HASH){ insignia1 = attr; return true; }
	if(type == ATT_INSIGNIA2_HASH){ insignia2 = attr; return true; }
	if(type == ATT_AUTODRIVE){ autodrive = (attr != 0); return true; }
	if(type == ATT_PLAYERTANK){
		if(attr){
		//	RegisterEntity("PlayerEntity");	//This is done in main think, until it succeeds, now.
			playertank = true;
		}else{
			UnregisterEntity();
			playertank = false;
		}
		return true;
	}
	if(type == ATT_FIRE){ Fire = attr; return true; }
	if(type == ATT_TANKMODE){ TankMode = attr; return true; }
	if(type == ATT_CMD_KILL){
		health = -0.1f;
		if(attr) ourkiller = GID;
		else ourkiller = 0;	//Set attr to 1 to remove a frag, to 0 to not lose a frag.  (Self-kill vs world-kill.)
		ourkillerweapontype = 0;
		return true;
	}
	if(type == ATT_SPAWNATFIRSTWAYPOINT) {SpawnAtFirst = attr; return true; }
	if(type == ATT_CMD_SPAWN || type == ATT_CMD_FIRSTSPAWN){
		ClearVec3(Velocity);
		ClearVec3(Rotation);
		//
		lastfiretime = VW->Time() + 1000 - (TP->reloadtime * 1000.0f);
		turRot[1] = 0.0f;
		vh.TurRot = 0.0f;	//Reset turret heading.
		//
		meshobj.PerturbX = turretmeshobj.PerturbX = gunmeshobj.PerturbX = (TMrandom() & 255);	//Reset perturb pos on spawn.
		meshobj.PerturbZ = turretmeshobj.PerturbZ = gunmeshobj.PerturbZ = (TMrandom() & 255);
		//
		//Make sure our new position gets sent reliably at least once!
		sendreliablepos = 1;
		//
		if (FixedSpawn == 0)
		{
			//Spawn entity at a spawn point.
			EntityWaypoint *ew = (EntityWaypoint*)FindRegisteredEntity("WaypointPath0");
			WaypointNode *waypt = NULL, *waypt2;
			if(ew){
				waypt = ew->WaypointByIndex(0);
				if(waypt){
					int spawnradius = 20;
					if(VW->GameMode() & GAMEMODE_DEATHMATCH){	//Random (deathmatch) spawn.
						int total = waypt->CountItems();
						if (SpawnAtFirst) // spawn at first waypoint with the flag set .. only makes sense for player tanks in training mode .. should be replaced with new team variable
						{
							for (int i = 0; i < total; i++)
							{
								waypt = ew->WaypointByIndex(i);
								if (waypt->flags == WAYPOINT_SPAWNHEREFLAG)
									break;
							}
						}
						else // spawn at a random waypoint
						{
							waypt = NULL;
							int i = 0;
							while((waypt == NULL) && (i < 100))
							{
								waypt = ew->WaypointByIndex(TMrandom() % total);
								if(!waypt) waypt = ew->WaypointByIndex(0);
								switch(waypt->teamspawntype)
								{
								case -1:
									waypt = NULL;
									break;
								case 0:
									break;
								default:
									if(VW->GameMode() & GAMEMODE_TEAMPLAY)
									{
										// Compare team to team in the spawn type
										int iteam = CLAMP(waypt->teamspawntype - 1, 0, CTankGame::Get().GetNumTeams() - 1);
										if (iteam != waypt->teamspawntype - 1)
										{
											waypt = NULL;
										}
										else
										{
											if(teamid != CTankGame::Get().GetTeam(iteam).hash)
												waypt = NULL;
										}
									}
									break;
								}
							}
							if(i == 100)
								waypt = ew->WaypointByIndex(0); // never found a good waypoint, so just use this one
						}
						Vec3 v = {waypt->Pos[0], VW->Map.FGetI(waypt->Pos[0], -waypt->Pos[2]) + 20.0f, waypt->Pos[2]};
						v[0] += TMrandom() % 10 - 5;
						v[2] += TMrandom() % 10 - 5;
						SetPos(v);
                        Rot3 r;
                        r[0] = 0;
                        r[1] = (float)(TMrandom() % 1000) * 0.001 * PI2;
                        r[1] = 0;
						SetRot(r);
						Vec3 vel;
						ClearVec3(vel);
						SetVel(vel);
					}else{
						if(waypt2 = ew->WaypointByIndex(1)){
							Rot3 r = {0, atan2f(waypt2->Pos[0] - waypt->Pos[0], waypt2->Pos[2] - waypt->Pos[2]), 0};
							SetRot(r);
						}
						int randrad1 = std::max(1, spawnradius / 2);	//Side to side radius.
						int randrad2 = std::max(1, spawnradius * 2);	//Front to back (towards/away second cp) radius.
						Mat3 tm;
						Rot3ToMat3(Rotation, tm);
						Vec3 v1;
						if(playertank){
							ScaleAddVec3(tm[2], -(TMrandom() % (randrad2 * 2) + randrad2), waypt->Pos, v1);
						}else{
							ScaleAddVec3(tm[2], -(TMrandom() % (randrad2 * 2)), waypt->Pos, v1);
						}
						Vec3 v = {v1[0] + TMrandom() % (randrad1 * 2) - randrad1, 0, v1[2] + TMrandom() % (randrad1 * 2) - randrad1};
						v[1] = VW->Map.FGetI(v[0], -v[2]) + 20.0f;
						SetPos(v);
					}
				}
			}
		}
		if(attr == 1){	//Fix tank.
			if(type == ATT_CMD_FIRSTSPAWN && VW->GameMode() & GAMEMODE_DEATHMATCH){
				SetInt(ATT_CMD_FIXTANK, 0);	//Unfix, for first spawn.
			}else{
				SetInt(ATT_CMD_FIXTANK, 1);
			}
		}
		return true;
	}
	if(type == ATT_CMD_FIXTANK){
		//
		ghost = 0;
		//
		health = 1.0f;
		ammo = 0;
		turretattached = 1;
		turretblow = 0;
		fade = TP->type_fade;
		CanCollide = true;
		weaponentity = 0;
	//	if(VW->GameMode() & GAMEMODE_DEATHMATCH) SetInt(ATT_AMMO, 6);	//Initial deathmatch ammo.
		//
		EntityBase *god = FindRegisteredEntity("TANKGOD");
		if(god){
			SetInt(ATT_AMMO, god->QueryInt(ATT_INITIAL_PEASHOOTER_AMMO));
		}
		//
		if(attr == 0){	//Unfix.
			//
			if(VW->GameMode() & GAMEMODE_DEATHMATCH) ghost = 1;
			//
			turretattached = 0;
			turretblow = 1;	//Okay, this must be 1 to keep turret off...
			fade = 0.0f;
			CanCollide = false;
			health = 0.0f;	//Need this too!
		}
		return true;
	}
	if(type == ATT_WEAPON_ENTITY){
		weaponentity = attr;
		return true;
	}
	if(type == ATT_AMMO){
		ammo = std::min(((EntityRacetankType*)TypePtr)->type_ammomax, attr);
		return true;
	}
	if(type == ATT_FRAGS){ frags = std::max(0, attr); sendreliablepos = 1; return true;}
	if(type == ATT_FLAGID){ FlagEntity = attr; return true; }
	//
	return EntityMesh::SetInt(type, attr);
}
bool EntityRacetank::SetString(int type, const char *s){
	if(type == ATT_NAME){
		Name = s;
		//
		//TODO: Propogate this to client copies of this ent, if on server.  DONE.
		if(VW->Net.IsServerActive()){
			char b[1024];
			BitPackEngine bp(b + 6, 1024);
			b[0] = BYTE_HEAD_MSGENT;
			WLONG(&b[1], GID);
			b[5] = MSG_HEAD_NAME;
			bp.PackString(Name, 7);
			VW->Net.QueueSendClient(CLIENTID_BROADCAST, TM_Ordered, b, bp.BytesUsed() + 6);
		}
		//
		return true;
	};
	return EntityMesh::SetString(type, s);
}
int EntityRacetank::QueryInt(int type){
	if(type == ATT_LAPTIME) return LapTime;
	if(type == ATT_RACETIME) return RaceTime;
	if(type == ATT_LASTLAPTIME) return LastLapTime;
	if(type == ATT_BESTLAPTIME) return BestLapTime;
	if(type == ATT_PING) return ping;
	if(type == ATT_LAPS) return laps;
	if(type == ATT_WAYPOINT) return waypoint;
	if(type == ATT_WEAPON_ENTITY) return weaponentity;
	if(type == ATT_AUTODRIVE) return autodrive;
	if(type == ATT_PLAYERTANK) return playertank;
	if(type == ATT_AMMO) return ammo;
	if(type == ATT_PLACE) return place;
	if(type == ATT_FRAGS) return frags;
	if(type == ATT_DEATHS) return deaths;
	if(type == ATT_WEAPON_PICKUP_OK) return (turretattached ? 1 : 0);
	if(type == ATT_TARGET_OK) return (turretattached ? 1 : 0);
	if(type == ATT_CHATTING) return chatting;
	if(type == ATT_INSIGNIA1_HASH) return insignia1;
	if(type == ATT_INSIGNIA2_HASH) return insignia2;
	if(type == ATT_FLAGID) return FlagEntity;
	return EntityMesh::QueryInt(type);
}
CStr EntityRacetank::QueryString(int type){
	EntityRacetankType *TP = (EntityRacetankType*)TypePtr;
	if(type == ATT_NAME) return Name;
	if(type == ATT_NAMESOUND) return TP->type_namesound;
	return EntityMesh::QueryString(type);
}
//******************************************************************
//**  Racetank Collision Physics!
//******************************************************************
bool EntityRacetank::InitCollide(){
	return true;
}
bool EntityRacetank::CollisionWith(EntityBase *collider, Vec3 colpnt){
	Collide(collider, colpnt, false);
	return true;
}
void EntityRacetank::FlagDeath(const int KillerID)
{
	health = 0.0f;
	ourkiller = KillerID;
	ourkillerweapontype = -1; // flag it as a flag death
}
//#define MINCOLLIDE 5.0f
#define MINCOLLIDE 3.0f
bool EntityRacetank::Collide(EntityBase *col, Vec3 colpnt, bool callother){
//	float VelocityFrac;
	int Solid = 0;
	if(callother){
		if(col->CollisionWith(this, colpnt)){	//If other returns true, means it accepts portion of velocity.
	//		VelocityFrac = 0.5f;
		}else{
	//		VelocityFrac = 1.0f;
			Solid = 1;
		}
	}else{
	//	VelocityFrac = 0.5f;
	}
	//Handle damage.
	if(col->QueryInt(ATT_TEAMID) == 0 || col->QueryInt(ATT_TEAMID) != teamid
		|| VW->GameMode() & GAMEMODE_TEAMDAMAGE || col->QueryInt(ATT_WEAPON_OWNER) == GID){	//Don't do damage if teamids are the same, or if teamid is 0, but do damage if team damage is on.
		Vec3 tv;
		Vec3IMulMat43(colpnt, tankMat, tv);
		NormVec3(tv);
		float t = std::min(0.0f, tv[2]);
		float factor = 2.0f;	//Amount to boost rear end damage.
		float damage = col->QueryFloat(ATT_DAMAGE);
		health -= (damage / MAX(0.01f, armor * flag_armorscale)) * (-t * (factor - 1.0f) + 1.0f);
		if(damage > 0.0f){// && col->QueryInt(ATT_WEAPON_OWNER)){
			ourkiller = col->QueryInt(ATT_WEAPON_OWNER);
			ourkillerweapontype = col->TypePtr->thash;
		}
	}
	//
	retargetdelay = 0;	//Target the bastid who done hit us.
	//Testing!
	//

	if (Immovable == 1)
		return true;

	Vec3 ts, tv;
//	SubVec3(Position, col->Position, ts);
	SubVec3(Position, colpnt, ts);
	NormVec3(ts);	//Direction away from collision.
//	SubVec3(Velocity, col->Velocity, tv);
	float v;// = LengthVec3(tv);	//Combined colliding velocities.
	//
	//Find real, signed relative velocity change.
	Vec3 tpos1, tpos2;
	float len1, len2;
	//
	//Me to col point.
	SubVec3(colpnt, Position, tpos1);
	len1 = LengthVec3(tpos1);
	ScaleAddVec3(col->Velocity, 0.01f, colpnt, tpos1);
//	CopyVec3(colpnt, tpos1);
	ScaleAddVec3(Velocity, 0.01f, Position, tpos2);
	SubVec3(tpos1, tpos2, tpos1);
	len2 = LengthVec3(tpos1);
	v = (len1 - len2) * 100.0f;	//Negative == retreat, positive == approach.
//	v = std::max(MINCOLLIDE, v);
	//
	//Them to col point.
	if(!Solid){
		SubVec3(colpnt, col->Position, tpos1);
		len1 = LengthVec3(tpos1);
		ScaleAddVec3(Velocity, 0.01f, colpnt, tpos1);
	//	CopyVec3(colpnt, tpos1);
		ScaleAddVec3(col->Velocity, 0.01f, col->Position, tpos2);
		SubVec3(tpos1, tpos2, tpos1);
		len2 = LengthVec3(tpos1);
		v += (len1 - len2) * 100.0f;	//Negative == retreat, positive == approach.
	}else{
		v *= 2.0f;	//Just double velocity if collider is solid.
	}
	//
//	ScaleVec3(ts, v * VelocityFrac * 1.1f, tv);//vh.AddAccelWP[vh.FrontLeft]);
//	//This should be 1.2...
	v = std::max(MINCOLLIDE, v);
	float r = 1.0f / (Mass + col->Mass);	//Use ratio of two masses.
//	v = v * 1.8f * col->Mass * r;
	v = v * 0.9f * col->Mass * r;	//Already doubled this time...
	ScaleVec3(ts, v, tv);
	//
	//Find out if we're moving towards or apart.
	//
//	if(DotVec3(ts, Velocity) < 0.0f){
	if(DotVec3(ts, Velocity) < v){	//Only if we're moving towards collison.
		//Find closest two corners.
		int idx[4] = {vh.FrontLeft, vh.BackLeft, vh.FrontRight, vh.BackRight};
		float dist = 0, dist2 = 9999;
		int n = 0, n2 = 1;
		for(int i = 0; i < 4; i++){
			Vec3 t;
			SubVec3(vh.WP[idx[i]], colpnt, t);
			float l = LengthVec3(t);
		//	if(l < dist){
		//		dist2 = dist; n2 = n;
		//		dist = l; n = i;
		//	}else{
		//		if(l < dist2){ dist2 = l; n2 = i; }
		//	}
			if(l > dist){ dist = l; n = i; }	//Find FURTHEST point now...
		}
		for(int j = 0; j < 4; j++)
		//	if(j != n) CopyVec3(tv, vh.AddAccelWP[idx[j]]);
			if(j != n) ScaleVec3(tv, 0.5f, vh.AddAccelWP[idx[j]]);
		//
	//	CopyVec3(tv, vh.AddAccel);
		ScaleVec3(tv, 0.5f, vh.AddAccel);
	//	for(int j = 0; j < 3; j++) vh.AddAccel[j] = -Velocity[j] * 1.2f;
		//
	//	CopyVec3(tv, vh.AddAccelWP[idx[n]]);
	//	CopyVec3(tv, vh.AddAccelWP[idx[n2]]);
		//
	//	CopyVec3(tv, vh.AddAccelWP[vh.BackLeft]);
	//	CopyVec3(tv, vh.AddAccelWP[vh.FrontRight]);
	//	CopyVec3(tv, vh.AddAccelWP[vh.BackRight]);
		//
		//Explosion sounds/smoke.
		if(v > 8.0f && crashsoundmade == 0 &&
			(callother || CmpLower(col->TypePtr->cname, "projectile"))){
			VW->AddEntity("sound", "metal1", colpnt, CVec3(v / (maxspeed / 2.0f), 0, 0), NULL, 0, 0, 0, ADDENTITY_FORCENET);
			VW->AddEntity("explo", "smoketiny", colpnt, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
			crashsoundmade = 1;
		}
	}
	return true;
}

//Overloaded virtual function to check collisions every physics tick.
bool TankVehicle::CheckCollision(){
	if(tank) return tank->CheckCollision();
	return false;
}
bool EntityRacetank::CheckCollision(){
	//Check collisions.
	if(CanCollide){
		if(VW->CheckCollision(this, GROUP_PROP)){
			EntityBase *col;
			Vec3 colpnt;
			while(col = VW->NextCollider(colpnt)){
				//Need to push away relative to collision point, for trees to work properly...
				//Or, move tree "center" to ground level, and change Sprites to allow for
				//uncentered ones.
				//Naahh, REAL problem is 10hz collisions, which is being worked on...
				Collide(col, colpnt, true);
			}
			return true;
		}
	}
	return false;
}



//
//EntityRacetankDoppelganger superclass, derived from EntityRacetank.
//Used for spinning preview of racetanks.
EntityRacetankDoppelgangerType::EntityRacetankDoppelgangerType(ConfigFile *cfg, const char *c, const char *t)
	: EntityRacetankType(cfg, c, t) {
	//
	Transitory = true;	//Very important!
	//
}
EntityBase *EntityRacetankDoppelgangerType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	//Manufactures an entity of type of current entity type object.
	return new EntityRacetankDoppelganger(this, Pos, Rot, Vel, id, flags);
}
bool EntityRacetankDoppelgangerType::CacheResources(){
	return ResCached = true;
}
void EntityRacetankDoppelgangerType::ListResources(FileCRCList *fl){
}
//
EntityRacetankDoppelganger::EntityRacetankDoppelganger(EntityRacetankDoppelgangerType *et,
	Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags) : EntityRacetank(et, Pos, Rot, Vel, id, flags){
	RTP = NULL;
	CanCollide = false;
}
bool EntityRacetankDoppelganger::Think(){
	//
	if(RTP){	//If we have a doppelgangee.
		//
		fade = 1.0f;
		//
		Mat43 tmpMat;
		Rot3ToMat3(Rotation, tankMat);
		CopyVec3(Position, tankMat[3]);	//Make tank matrix.
		Rot3ToMat3(turRot, tmpMat);
		if(RTP->mesh) CopyVec3(RTP->mesh->Offset, tmpMat[3]);	//Make turret-local matrix.
		else ClearVec3(tmpMat[3]);
		float turretfade = fade;
		Mat43MulMat43(tmpMat, tankMat, turMat);	//Transform by tank matrix.
		//
		int rendflags = RTP->type_rendflags;
		int trans;	//Decide if we're transparent or not.  Add opacity checking once there is opacity!
		if(!(rendflags & MESH_BLEND_ADD) && (fade >= 1.0f || (rendflags & MESH_BLEND_ENVMAP))) trans = 0;
		else trans = 1;
		//
		int tankvis = 0;
		if(RTP->mesh){
			meshobj.Configure(RTP->texture, RTP->mesh, tankMat, tankMat[3], rendflags, fade, RTP->mesh->BndRad, 10.0f);//RTP->mesh->LodBias);
		//	if(trans) tankvis = VW->PolyRend->AddTransObject(&meshobj);
			tankvis = VW->PolyRend->AddSecondaryObject(&meshobj);
		//	VW->PolyRend->AddMeshMat(TP->mesh, tankMat, false, TP->type_rendflags);
		}
		if(RTP->turretmesh){
			turretmeshobj.Configure(RTP->texture, RTP->turretmesh, turMat, turMat[3], rendflags, turretfade, RTP->turretmesh->BndRad, 10.0f);//TP->turretmesh->LodBias);
		//	if(trans || turretfade < 1.0f) VW->PolyRend->AddTransObject(&turretmeshobj);
			VW->PolyRend->AddSecondaryObject(&turretmeshobj);
		//	VW->PolyRend->AddMeshMat(TP->turretmesh, turMat, true, TP->type_rendflags);
		}
		if(RTP->gunmesh){
			gunmeshobj.Configure(RTP->texture, RTP->gunmesh, turMat, turMat[3], rendflags, turretfade, RTP->gunmesh->BndRad, 10.0f);//TP->gunmesh->LodBias);
		//	if(trans || turretfade < 1.0f) VW->PolyRend->AddTransObject(&gunmeshobj);
			VW->PolyRend->AddSecondaryObject(&gunmeshobj);
		//	VW->PolyRend->AddMeshMat(TP->gunmesh, turMat, true, TP->type_rendflags);
		}
		//
	}
	//
	return true;
}
bool EntityRacetankDoppelganger::SetString(int type, const char *s){
	if(type == ATT_DOPPELGANG && s){
		EntityRacetankType *rtp = (EntityRacetankType*)VW->FindEntityType("racetank", s);
		if(rtp && rtp != RTP){
			rtp->InterrogateInt("minimal rescache");
			rtp->CacheResources();	//Make sure resources for the doppelgangee are cached.
			rtp->InterrogateInt("normal rescache");
		}
		RTP = rtp;
		return true;
	}
	return EntityRacetank::SetString(type, s);
}
//

