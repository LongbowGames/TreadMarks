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

//Entity code for Race game.
//Entity Class is overclass, specifying actual C++ class.  Entity Type is sub-type
//of class, specifying dynamically read in data.

#ifndef ENTITYRACE_H
#define ENTITYRACE_H

#include "EntityBase.h"
#include "VoxelWorld.h"
#include "Trees.h"
#include "CStr.h"
#include "Physics.h"
#include "TankAI.h"

extern float TurretSpeedScale;

//
//This function MUST be called ONCE by your main app to register the RaceTank classes.
int RegisterRacetankEntities();
//
//Combine these with bit-and.  Stored in VW->GameMode();
#define GAMEMODE_RACE		0x00000001
#define GAMEMODE_DEATHMATCH	0x00000002
#define GAMEMODE_CTF		0x00000004
#define GAMEMODE_TRAINING	0x00000008
//The above flags are exclusive.
#define GAMEMODE_NOWEAPONS	0x00100000
#define GAMEMODE_TEAMPLAY	0x00000100
#define GAMEMODE_TEAMDAMAGE	0x00000200
#define GAMEMODE_TEAMSCORES	0x00000400
//
#define GAMEMODE_NOCANDRIVE	0x10000000
//Used for start-of-race or end of level freezing.  Includes no shooting.
#define GAMEMODE_NOCANSHOOT	0x01000000
//Just disallows weapons fire.
//
#define GAMEMODE_SHOWSCORES	0x20000000
//Show scoreboard.
#define GAMEMODE_RECONNECT	0x40000000
//Clients should reconnect automatically if disconnected.
//

//TANKGOD Entity.  Does Godly stuff.  One per world.  Worship it, peon!
//Registers itself as "GOD".
class EntityTankGodType : public EntityGodType {
friend class EntityTankGod;
private:
	//Other worldly config options here...
	float type_mmlinew, type_mmopac, type_mmcliprad, type_mmclipbias, type_mmx, type_mmy, type_mmscale;
	float type_mmpcol[3], type_mmecol[3], type_mmlcol[3], type_mmtcol[3], type_mmfcol[3];
	int type_racepeashooter, type_dmpeashooter;
	float type_globalmultikilldecevery;
	int type_globalmultikillthresh;
	CStr type_globalmultikillsound;
public:
	EntityTankGodType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);	//Manufactures an entity of type of current entity type object.
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
private:
	void ReadCfg(ConfigFile *cfg);	//We override defaults from map config file.
};
struct Target : public LinklistBase<Target> {
	EntityGID gid;
	int lap, nextway, frags, deaths;
	float waydist, health;
	Vec3 pos, rot, vel;
	int targetok, serverplace;
	int racetime;
	unsigned int teamid;
	int Human;
	Target() : gid(0), lap(0), nextway(0), waydist(0), health(0), frags(0),
		targetok(0), serverplace(0), racetime(0), teamid(0) { };
	bool operator>(const Target &t) const{
		if(serverplace > 0 && t.serverplace > 0){	//Server has dictated placing to us, as client.
			if(serverplace < t.serverplace) return 1;
			else return 0;
		}
		if(racetime > 0){	//Tank has posted a final race time.
			if(t.racetime == 0) return 1;	//Other guy hasn't finished yet.
			if(racetime < t.racetime) return 1;	//Lower race time.
			else return 0;
		}else{
			if(t.racetime > 0) return 0;	//We have not finished, they have.
		}
		if(lap > t.lap) return 1;
		else if(lap < t.lap) return 0;
		else if(((nextway > t.nextway && t.nextway > 0) || (nextway == 0 && t.nextway > 0))) return 1;	//This handles heading toward the finish line, which is waypoint 0.
		else if(t.nextway == nextway && waydist < t.waydist) return 1;
		else if(t.nextway == nextway && waydist == t.waydist && frags > t.frags) return 1;
		else if(t.nextway == nextway && waydist == t.waydist && frags == t.frags && deaths < t.deaths) return 1;	//If frags identical, differentiate based on deaths.
		else if(t.nextway == nextway && waydist == t.waydist && frags == t.frags && deaths == t.deaths && health > t.health) return 1;	//If frags/deaths identical, differentiate based on health
		else if(t.nextway == nextway && waydist == t.waydist && frags == t.frags && deaths == t.deaths && health == t.health && gid > t.gid) return 1;	// Differentiate based on health
		else return 0;	//Argh, always need the identity checks!
	};
	int operator<(const Target &t){
		return t.operator>(*this);
	};
};
struct PowerUp : public LinklistBase<PowerUp> {
	EntityGID gid;
	Vec3 pos;
	int level;
	PowerUp() : gid(0), level(0) { };
};
//Send in this order with SetInt.
#define ATT_FRAGMSG_KILLER	0x12130001
//GId of killer.
#define ATT_FRAGMSG_WEAPON	0x12130002
//Type Hash of weapon.
#define ATT_FRAGMSG_KILLEE	0x12130003
//GID of poor sap.
//							0x12130004 is reserved!  Do not use!
#define ATT_CMD_STATUS_RESPAWN	0x12130010
//Set every frame to keep the "Press Fire to Respawn" text up.
//
#define ATT_RACE_LAPS		0x12130020
//Set/QueryInt the number of laps in the race.
#define ATT_INITIAL_PEASHOOTER_AMMO	0x12130022
//
#define ATT_HOWITZERTIME_TOGGLE		0x12130025
//An entity should turn this on if it wants HowitzerTime to start; the game
//should then start HowitzerTime and turn this toggle off.
//
#define MSG_HEAD_FRAGMSG		(MSG_HEAD_USERMSG + 30)
#define MSG_HEAD_TANKGODSTATUS	(MSG_HEAD_USERMSG + 31)
class EntityTankGod : public EntityGod {
private:
	Target TgtHead;
	PowerUp PowHead;
	LineMapObject mm;
	EntityGID fragmsg_killer, fragmsg_killee;
	ClassHash fragmsg_weapon;
	int tellrespawnflag;
	int racelaps, statuscounter;
	int racetime;
	int HowitzerTimeToggle;
	int globalmultikill, timesincegmkdec;
public:
	EntityTankGod(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
	void DeliverPacket(const unsigned char *data, int len);
	bool SetInt(int type, int attr);
	int QueryInt(int type);
	float QueryFloat(int type);
	CStr QueryString(int type);
	EntityGroup Group(){ return GROUP_ACTOR; };//GROUP_OMNIPOTENT
public:
	//This is the public interface for praying to the Tank God to be told what place you are in the race
	//and what juicy targets of opportunity are closest at hand.
	int UpdateTank(EntityBase *tank, int lap, int nextmainway, float distance, int frags, unsigned int teamid);	//Returns Place in race.
	int RemoveTank(EntityBase *tank);	//Removes you from update list.
	EntityGID ClosestTank(Vec3 pos, EntityGID skipme = 0, Vec3 dir = NULL, float rearbias = 1.0f,
		unsigned int skipteam = 0, unsigned int fromteam = 0);
		//rearbias scales the apparent distance of things to the rear.  2.0 makes rear targets half as likely to be targetted.
	EntityBase *TankByIndex(int n);	//Tanks move around in list, indices not constant!
	//
	int UpdatePowerUp(EntityBase *powerup, int level);
	EntityGID ClosestPowerUp(Vec3 pos, Vec3 dir = NULL, float rearbias = 1.0f);
	EntityBase* ClosestFlag(const Vec3 Pos, const int MyTeam, const bool bWantMyTeam);
	void KillTeam(const int TeamHash, const int Killer);

	friend class CTankAI;
};

//Settable attributes.
#define ATT_ACCEL		0x7a940000
#define ATT_STEER		0x7a940001
#define ATT_TURRETSTEER	0x7a940002
#define ATT_AUTODRIVE	0x7a940003
#define ATT_PLAYERTANK	0x7a940004
#define ATT_FIRE		0x7a940005
#define ATT_LACCEL		0x7a940006
#define ATT_RACCEL		0x7a940007
#define ATT_TANKMODE	0x7a940008
#define ATT_HEALTH		0x7a940010
#define ATT_AMMO		0x7a940011
#define ATT_FRAGS		0x7a940012
#define ATT_CHATTING	0x7a940013
#define ATT_DEATHS		0x7a940022
//Set/Query as Float.
#define ATT_SKILL		0x7a940015
//Turns on floating name display for x msecs.
#define ATT_SHOWNAME		0x7a940016
#define ATT_SHOWTEAMNAME	0x7a940017
//Readable attributes.
#define ATT_LAPS		0x7a940100
#define ATT_WAYPOINT	0x7a940101
#define ATT_PLACE		0x7a940102
#define ATT_LAPTIME		0x7a940105
#define ATT_RACETIME	0x7a940106
#define ATT_LASTLAPTIME	0x7a940107
#define ATT_BESTLAPTIME	0x7a940108
//QueryString this.
#define ATT_NAMESOUND	0x7a940109
//
//Kill self.
#define ATT_CMD_KILL	0x7a940666
//
#define ATT_DAMAGE		0x7a940200
//Reads amount of damage said entity does during collisions.
#define ATT_WEAPON_ENTITY	0x7a940300
//Gets or sets GID of current bolton weapon.
#define ATT_WEAPON_PICKUP_OK	0x7a940303
//Is it OK for the tank to pick up a weapon?
#define ATT_CMD_FIXTANK		0x7a940333
#define ATT_MAXSPEED		0x7a940400
#define ATT_ACCELSPEED		0x7a940401
#define ATT_TURRETHEADING	0x7a940410
#define ATT_TANK_FIRERATESCALE	0x7a940411
//
#define ATT_TARGET_OK		0x7a940440
#define ATT_FLAGID			0x7a940441
//Queries whether tank should be targetted by other tanks and weapons.
//

//Racing Tank superclass.
#define NUM_BOLTONS 4
#define NUM_TURRET_BOLTONS 2
class EntityRacetankType : public EntityMeshType {
public:
	CStr turretmeshfile, gunmeshfile;
	CStr turretmeshfile1, gunmeshfile1;
	CStr turretmeshfile2, gunmeshfile2;
	Mesh *turretmesh, *gunmesh;
	CStr engsnd1file, engsnd2file, rumblesndfile;
	SoundNode *engsnd1, *engsnd2, *rumblesnd;
	float type_maxspeed, type_accelspeed, type_turretspeed, crudforce;
	CStr smoketype, onfiretype;
	CStr type_turretblowexplo, type_tankblowexplo, type_respawnexplo;
	CStr projectiletype;
	float reloadtime;
	CStr type_baubletype;
	float type_healthregen;
	float type_armor;
	int type_ammomax;
	float type_gravityscale, type_frictionscale;
	CStr type_namesound, type_multikillsound;
	float type_multikilltime;
	//
	Vec3 type_bolton[NUM_BOLTONS];	//bolton location 0 will be for bauble
	//
	int minimalrescache;
public:
	EntityRacetankType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);	//Manufactures an entity of type of current entity type object.
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
	int InterrogateInt(const char *thing);	//Responds to "minimal rescache" and "normal rescache".
	CStr InterrogateString(const char *thing);	//Responds to "namesound".
};
class EntityRacetank;
//Vehicle subclass.
class TankVehicle : public Vehicle {
public:
	bool CheckCollision();
	EntityRacetank *tank;
	TankVehicle() : tank(NULL) { return; };//EntityRacetank *t) : tank(t) {};
};
//
#define TANK_STATUS_TIME 1000

class EntityRacetank : public EntityMesh {
private:
	CTankAI	*AI;
	float flag_armorscale;
	float flag_speedscale;
	float flag_fireratescale;
public:
	MeshObject turretmeshobj, gunmeshobj;
	TankVehicle vh;
	float maxspeed, accelspeed, turretspeed;
	float Accel, Steer, TurretSteer, LAccel, RAccel, TurretSteerVel;
	int Fire, TankMode;
	Mat43 tankMat, turMat;
	//
	Vec3 turVel;	//Turret blow off variables.
	int turretattached, turretblow, turretblowtime;	//blowturret is bool that we should blow, turblowtime is ms since blow.
	int frags, deaths;
	int lastfrags, timesincelastfrag;
	//
	Vec3 turRot;
	int smoketime;
	int ltreadx, ltready, rtreadx, rtready;	//For tread marks.
	bool playertank;
	int engsnd1chan, engsnd2chan, rumblechan;
	float motsndfreq;
	float rumblevol;
	Vec3 LastVel;
	EntityGID finderid;
	float finderangle;
	int thudtime, crashsoundmade;
	int lastfiretime;
	EntityGID targetent;
	int retargetdelay;
	//Movement ai stuff.
	Vec3 ai_moveto, ai_moveto2;
	float ai_movespeed, ai_movetoradius;
	int ai_state, ai_defstate, ai_statedelay, ai_thinkdelay;
	Vec3 lastpos1, lastpos2, lastpos3;	//One second interval position memory.
	EntityGID baubleentity;
	float health;	//Primary damage indicator.
	float healthregen;	//Speed of regeneration.
	float armor;	//Armor percentage.
	float skill;	//Brains da tank got.  0.0f - 1.0f.
	CStr Name;
	int ping;
	int nextWayPt;
	//

	int FixedSpawn;
	int Immovable;
	int SpawnAtFirst;

	EntityGID weaponentity, ourkiller;
	ClassHash ourkillerweapontype;
	//
	int ammo;	//pea shooter ammo.
	int sendreliablepos;
	int sendstatuscounter;
	int ghost;
	//
	ClassHash insignia1, insignia2;
	//
	unsigned int LapTime, RaceTime, LastLapTime, BestLapTime;
	int waypoint, laps, lastlaps, place;
	bool autodrive;
	EntityGID wayentity, godentity;
	//
	unsigned int shownametime;
	EntityGID shownameent;
	int showteamname;	//bool.
	//
	int chatting;
	unsigned int alivetime;
	int multikilltimer;

	EntityGID FlagEntity;
public:
	EntityRacetank(EntityRacetankType *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntityRacetank();
	void SetPos(Vec3 pos);
	void SetRot(Rot3 rot);
	void SetVel(Vec3 vel);
	bool Think();
	void DeliverPacket(const unsigned char *data, int len);
	int ExtraCreateInfo(unsigned char *data, int len);
	bool SetFloat(int type, float attr);
	bool SetInt(int type, int attr);
	bool SetString(int type, const char *s);
	int QueryInt(int type);
	float QueryFloat(int type);
	CStr QueryString(int type);
	bool InitCollide();
	bool CollisionWith(EntityBase *collider, Vec3 colpnt);
	bool Collide(EntityBase *collider, Vec3 colpnt, bool callother);	//Specific to derived class function that holds collison guts.  Set CallOther to call collidee's CollisionWith member.
	bool CheckCollision();	//Specific function to do whole collision checking shebang.
	EntityGroup Group(){ return GROUP_ACTOR; };
	int ReadyToFire();
	void FlagDeath(const int KillerID);
};

//
//EntityRacetankDoppelganger superclass, derived from EntityRacetank.
//Used for spinning preview of racetanks.
#define ATT_DOPPELGANG	0x77170001
class EntityRacetankDoppelgangerType : public EntityRacetankType {
public:
	EntityRacetankDoppelgangerType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);	//Manufactures an entity of type of current entity type object.
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityRacetankDoppelganger : public EntityRacetank {
public:
	EntityRacetankType *RTP;
public:
	EntityRacetankDoppelganger(EntityRacetankDoppelgangerType *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
	bool SetString(int type, const char *s);
};
//

//TREE superclass, derived from SPRITE.
class EntityTreeType : public EntitySpriteType {
public:
	float type_trunk;	//Trunk radius for collisions.
	CStr type_explodefol, type_explokill;
public:
	EntityTreeType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityTree : public EntitySprite {
public:
	int mssincesmoke;
public:
	EntityTree(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CollisionWith(EntityBase *collider, Vec3 colpnt);
	bool Think();
};

//SMOKE superclass, derived from SPRITE.
//Set Velocity for direction smoke should move (meters/second), set ID to msecs to live.
//
class EntitySmokeType : public EntitySpriteType {
public:
	float fadestart;
	float type_lifespan, type_grow;
	Vec3 type_defaultvel;
	float type_gravity;
	int type_randomrotate;
public:
	EntitySmokeType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntitySmoke : public EntitySprite {
public:
	int timetodie, timeofbirth;
	EntitySmoke(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
	virtual EntityGroup Group(){ return GROUP_ETHEREAL; };	//Queries group type of entity object.
};

//CRUD superclass, handling debris kicked up from treads.
class EntityCrudType : public EntitySpriteType {
public:
	float type_startrad, type_endrad;
	int type_timetolive, type_particles;
	float type_endfade, type_colorfade;
	float type_gravity;
	int type_usegroundcolor;
public:
	EntityCrudType(ConfigFile *cfg, const char *c, const char *t);// : EntityTypeBase(c, t) { };
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
};
class EntityCrud : public EntitySprite {
public:
	ParticleCloudObject particleobj;
	int timeofbirth, timetodie, particles;
	unsigned char r, g, b;
public:
	EntityCrud(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityCrud(){ };
	bool Think();
	virtual EntityGroup Group(){ return GROUP_ETHEREAL; };	//Queries group type of entity object.
};

//COURSE superclass, deriving from Waypoint.
//First, a helper class to hold a list of power-up spawning points.
class PowerUpSpawnPoint : public LinklistBase<PowerUpSpawnPoint> {
friend class EntityCourse;
private:
	Vec3 Pos;
	EntityGID entity;
	EntityTypeBase *SpawnType;
	unsigned int elapsedtime;
	int state;
	int lastbestlap;
	int bestlapcount;
public:
	PowerUpSpawnPoint() : entity(0), elapsedtime(0), state(0), lastbestlap(-1), bestlapcount(0) { };
};
//Enumerated list of power ups and weapons available.
class PowerUpTypeEnum : public LinklistBase<PowerUpTypeEnum> {
friend class EntityCourse;
private:
	ClassHash type;
	int level, weight;	//Quality Level and Weight for random chance of appearing.
public:
	PowerUpTypeEnum() : type(0), level(0), weight(0) { };
	PowerUpTypeEnum(EntityTypeBase *et){
		if(et){
			type = et->thash;
			level = et->InterrogateInt("powerup-level");
			weight = et->InterrogateInt("powerup-weight");
		}
	};
};
//TYPE handler.
class EntityCourseType : public EntityWaypointType {
public:
	float type_nextweapondelay;
	CStr type_weaponspawnentity, type_checkpointentity, type_startpointentity;
	CStr type_alwaysspawnentityclass, type_alwaysspawnentitytype;
	int type_teamspawntype; // -1 = no one, 0 = anyone, >= 1 = team number
	float type_pushstartpoints;
public:
	EntityCourseType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void ReadCfg(ConfigFile *cfg);
	virtual CStr InterrogateString(const char *thing);
};
//Actual object.
class EntityCourse : public EntityWaypoint {
public:
	bool preprocessed, master;
	int total_weights;
	PowerUpSpawnPoint powhead;	//Only for use by Master Course Point object...
	PowerUpTypeEnum powenumhead;
public:
	EntityCourse(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	bool Think();
};

//EXPLO superclass, derived from SPRITE.  Explosions via packs of clouds.
//Set Velocity for direction smoke should move (meters/second), set ID to msecs to live.
//
#define FLAG_EXPLO_UP 0x01
class EntityExploType : public EntitySmokeType, public MixinEntityChain {
public:
//	float fadestart;
	int type_count;
	float type_expand, type_expandstart;
	Vec3 type_randveladdmax, type_randveladdmin;
	int randveladd;	//flag
	int type_flyingtrailmode;
	float type_flyingtrailtime;
	float type_timewarp;
	CStr type_soundenttype;
public:
	EntityExploType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class ExploSpriteObject : public SpriteObject {
public:
	Vec3 ExploOffset;
};
class EntityExplo : public EntitySmoke {
public:
	int count;
	ExploSpriteObject *spriteobjs;
	int nextflyingtrail;
	int flyingtrailms, timewarpms;
public:
	EntityExplo(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntityExplo();
	bool Think();
};

//EXPLOSPHERE superclass, derived from MESH.  Explosions via transparent spheres.
//Set Velocity for direction explo should move (meters/second), set ID to msecs to live.
//
class EntityExploSphereType : public EntityMeshType {//, public MixinEntityChain {
	//Mesh class has chaining ability now!
public:
	float type_lifespan;
	float type_expand, type_expandstart, type_rotatespeed, type_rotatespeedz;
	CStr type_soundenttype;
	EntityExploSphereType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityExploSphere : public EntityMesh {
public:
	Vec3 OriRotation;
	int timetodie, timeofbirth;
	EntityExploSphere(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
};

//PROJECTILE superclass, derived from SPRITE.
//Set Velocity to direction of shot, speed will come from ent file.
//
#define ATT_WEAPON_OWNER	0x797d2004
//The tank that owns this weapon.

class EntityProjectileType : public EntityMeshType {
public:
//	float fadestart;
	CStr exploent, soundent, smokeent, smoketrailent;
	CStr explohitent;
	float crater, depth, speed, scorch, kick, type_gravity;
	float type_damage;
	bool checkcollisions, type_smoketrail;
	float type_spiral, type_spiralramp, type_spiralperiod;
	float type_guidespeed, type_guidemax, type_guidebias, type_guideramp;
	float type_timetolive;
	float type_launchinertia, type_bounce, type_followground;
	float type_splashdamage, type_splashradius, type_splashpush;
	int type_detonate;
	Vec3 type_smoketrailoffset;
	int type_hitscan;
	float type_hitscanrange;
	float type_selfhitdelay;
	CStr type_flareent;
	int type_iterations;
	float type_smoketrailtime;
public:
	EntityProjectileType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	float InterrogateFloat(const char *thing);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityProjectile : public EntityMesh {
public:
	EntityGID godentity, flareentity;
	int timeofbirth;
	Vec3 lPos;
	int firstthink;
	float damage;
	int lastsmoketime;
private:
	static EntityGID curowner;	//These are used so that any projectile entities in any way spawned from another projectile can inherit the original's owner and team ids.
public:
	static EntityGID GetCurrentOwner();
	static EntityGID SetCurrentOwner(EntityGID owner);
public:
	EntityProjectile(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntityProjectile();
	float QueryFloat(int type);
	int QueryInt(int type);
	bool Think();
	void DeliverPacket(const unsigned char *data, int len);
private:
	void Detonate(EntityGID nosplash);	//Creates crater and calcs splash damage.  Does not do explo or direct damage.
};

//BAUBLE superclass, derived from MESH.
//Dangly bouncy antenna thingy.
#define ATT_INSIGNIA1_HASH	0x854d0001
#define ATT_INSIGNIA2_HASH	0x854d0002
#define ATT_INSIGNIA3ID		0x854d0003
class EntityBaubleType : public EntityMeshType {
public:
	float type_stiffness, type_dampening;
	EntityBaubleType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityBauble : public EntityMesh {
public:
	Vec3 lasttippos, lasttipvel, baublepos, baublevel;
	Mat3 baublemat;
	EntityGID insignia[3];
	ClassHash insigniatype[3];
public:
	EntityBauble(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntityBauble();
	bool Think();
	bool SetInt(int type, int attr);
	int QueryInt(int type);
};

//WEAPON superclass, derived from MESH.
//Actual bolt-on secondary weapon model and controller.
#define ATT_WHICH_BOLTON	0x56b21001
//Returns which bolt-on location weapon is designed for.
#define ATT_WEAPON_STATUS	0x56b21002
//Returns whether weapon is attached to tank or not.
#define ATT_WEAPON_AMMO		0x56b21003
//Ammunition left.
#define ATT_WEAPON_FULLAMMO	0x56b21005
//How much ammo in a full clip.
#define ATT_WEAPON_MULT		0x56b21004
//Display multiplier for ammo.
#define ATT_WEAPON_TRANSTEX	0x56b21006
//Whether texture should be inherited by tank.
#define ATT_WEAPON_TRANSPERTURB	0x56b21007
#define ATT_WEAPON_LASTFIRETIME 0x56b21008
#define ATT_WEAPON_PROJECTILE_TYPE 0x56b21009

#define MAX_DESC_LINES 20
class EntityWeaponType : public EntityMeshType {
friend class EntityWeapon;
private:
	CStr secmeshfile, secmeshfile1, secmeshfile2;//, texturefile;
	Mesh *secmesh;
private:
	Vec3 type_launchangle;
	Vec3 type_launchcoords;
	CStr type_projectile, type_firesoundent;
	float type_reloadtime;
	int type_ammo;
	int type_bolton;
	int type_weight, type_level;
	Vec3 type_secrotmin, type_secrotmax, type_secmeshoffset;
	float type_secrotwindup;
	int type_ammodisplaymult;
	Rot3 type_spread;
	int type_launchcount;
	int type_globalcoords;
	CStr type_secentname;
	Vec3 type_secentoffset;
	CStr type_pickupsound, type_inentity, type_spawnsound, type_outentity, type_spawnentity;
	int type_transmutetexture;
	float type_transmuteperturb;
	int type_unique;
public:
	EntityWeaponType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	int InterrogateInt(const char *thing);	//Responds to "powerup-family", "powerup-level", and "powerup-weight".
	float InterrogateFloat(const char *thing);
};
class EntityWeapon : public EntityMesh {
	MeshObject secmeshobj;
	EntityGID tank, ltank, firesound, tankgod;
	int ammo;
	unsigned int lastfiretime;
	int Fire;
	float secrotlerp;
	Rot3 secrot;
	EntityGID secent;
public:
	EntityWeapon(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntityWeapon();
	bool Think();
	bool CollisionWith(EntityBase *collider, Vec3 colpnt);
	int QueryInt(int type);
	float QueryFloat(int type);
	CStr QueryString(int type);
	bool SetInt(int type, int attr);
	void DeliverPacket(const unsigned char *data, int len);
};

//POWERUP superclass, derived from MESH.
class EntityPowerUpType : public EntityMeshType {
friend class EntityPowerUp;
private:
	float type_healthfix;
	int type_ammoadd;
	float type_boltonammoadd;
	int type_weight, type_level;
	CStr type_pickupsound, type_inentity, type_spawnsound, type_outentity, type_spawnentity;
public:
	EntityPowerUpType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	int InterrogateInt(const char *thing);	//Responds to "powerup-family", "powerup-level", "powerup-weight", "ammoadd"
	float InterrogateFloat(const char *thing); //Responds to "healthfix", and "boltonammoadd"
};
class EntityPowerUp : public EntityMesh {
	EntityGID tankgod;
public:
	EntityPowerUp(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
	bool CollisionWith(EntityBase *collider, Vec3 colpnt);
	int QueryInt(int type);
	bool SetInt(int type, int attr);
};

//SPEWER superclass, derived from MESH.
#define MAXSPEWTYPE 10
class EntitySpewerType : public EntityMeshType {
friend class EntitySpewer;
private:
	CStr type_spewtype[MAXSPEWTYPE];
	int type_cycle, type_numspewtype;
	Vec3 type_spewmin, type_spewmax;
	float type_spewtime;
	bool type_randomtime, type_propagatespew;
	Vec3 type_launchcoords;
	int type_maxspewcount, type_iterations;
public:
	EntitySpewerType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntitySpewer : public EntityMesh {
	int lastspewtime;
	int lastspewtype;
	int spewcount;
	EntityGID curowner;
public:
	EntitySpewer(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
};


//SQUISHY superclass, derived from MESH.
class EntitySquishyType : public EntityMeshType {
friend class EntitySquishy;
private:
	float type_bounce, type_bouncemin, type_bouncethreshold, type_pitchscale, type_expandbbox;
	float type_bounceevery;
	CStr type_bouncesound;
public:
	EntitySquishyType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntitySquishy : public EntityMesh {
public:
	EntitySquishy(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
	void DeliverPacket(const unsigned char *data, int len);
};


//INSIGNIA superclass, derived from MESH.
class EntityInsigniaType : public EntityMeshType {
friend class EntityInsignia;
private:
	float type_perturb, type_perturbscale, type_perturbspeed;
	Rot3 type_rotation;
	int type_team;
	int hflip;
public:
	EntityInsigniaType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	int InterrogateInt(const char *thing);	//Responds to "TEAM".
};
class EntityInsignia : public EntityMesh {
public:
	Mat3 rotMat;
public:
	EntityInsignia(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
	bool SetVec(int type, const void *v);
	void SetPos(Vec3 pos);
	void SetRot(Rot3 rot);
};

#define MAX_DESC_STRINGS 40
//Tutorial superclass, derived from MESH.
class EntityTutorialType : public EntityMeshType {
friend class EntityTutorial;
private:
	CStr secmeshfile, secmeshfile1, secmeshfile2;//, texturefile;
	CStr NameString;
	CStr ImageDesc;		// image used for description, rather than text
	int TutID;
	Mesh *secmesh;
	Mesh *secentmesh;
	float type_secrotwindup;
	Vec3 type_secrotmin, type_secrotmax, type_secmeshoffset;
	CStr type_secentname;
	Vec3 type_secentoffset;
public:
	int descid;
	CStr DescStrings[MAX_DESC_STRINGS];
	EntityTutorialType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	CStr InterrogateString(const char *thing);
	int InterrogateInt(const char *thing);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityTutorial : public EntityMesh {
private:
	MeshObject secmeshobj, secentmeshobj;
	float secrotlerp;
	Rot3 secrot;
	CStr secmeshfile, secmeshfile1, secmeshfile2;//, texturefile;
	EntityTutorialType *TP;
	EntityGID secent;
public:
	~EntityTutorial();
	EntityTutorial(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool Think();
	bool SetString(int type, const char *s);
};

#endif
