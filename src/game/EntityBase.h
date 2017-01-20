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

//Entities for Voxel World use.
//EntityBase is the purish virtual base class of all Entity types.

#ifndef ENTITYBASE_H
#define ENTITYBASE_H

#include "Trees.h"
#include "Vector.h"
#include "CfgParse.h"
#include "CStr.h"
#include "Image.h"
#include "Poly.h"
#include "ResourceManager.h"
#include "Networking.h"
#include "BitPacking.h"
#include "FileCRCList.h"

#define ENTITY_GROUPS 3
enum EntityGroup{
	GROUP_ACTOR = 0,
	GROUP_PROP = 1,
	GROUP_ETHEREAL = 2
};
const char GroupNames[ENTITY_GROUPS][10] = {
	"actor",
	"prop",
	"ethereal"
};

extern const int nEnumBlend;
extern ConfigEnum EnumBlend[];

//This function must be defined by the source file for the app-specific entities.
//extern EntityBase *CreateEntity(const char *superclass, const char *subclass, Vec3 pos, Rot3 rot);
//Configures appropriate Entity sub class based on config file data.  Define in app specific entity file.
//extern int InitEntitySubclass(ConfigFile *cfg);

//forward declarations.
class VoxelWorld;
class EntityTypeBase;
class EntityBase;
class EntityGenericType;
class EntityGeneric;


typedef EntityTypeBase* (*ENTITYTYPECREATORPTR)(ConfigFile *cfg);

bool RegisterEntityTypeCreator(ENTITYTYPECREATORPTR etcp);	//Use this to register each additional Entity Type creation function.

EntityTypeBase *CreateEntityType(ConfigFile *cfg);	//Asks registered Entity Type creators if they can handle class/type in config file.

typedef unsigned int EntityGID;
typedef unsigned int ClassHash;

#define ENTITYGID_NETWORK 0x80000000
//High bit of GID is ON for network persistent entities.

//Entity bucket object.
class EntityBucketNode : public LinklistBase<EntityBucketNode> {
public:
	EntityBase *entity;
	EntityBucketNode() : entity(NULL) {};
	EntityBucketNode(EntityBase *ent) : entity(ent) {};
};
ClassHash MakeHash(const char *c, const char *t);	//Generate 32-bit hashes from text strings.
ClassHash MakeHash(const char *c);

//Base class for SubType identifier data.  CreateEntity() is pure virtual, and must be overridden.
class EntityTypeBase : public LinklistBase<EntityTypeBase> {
protected:
	static VoxelWorld *VW;
	bool ResCached;	//Resources cached?
	bool ResListed; // Fix for cyclic includes
public:
	static void SetVoxelWorld(VoxelWorld *vw){ VW = vw; };
	VoxelWorld *GetVW(){ return VW; };
	CStr cname, tname, dname;	//read-only, plain text names, Class, Type, and Descriptive (never used for comparisons).
	int nameid; // read-only string ID for dname equivalent
	ClassHash chash, thash;	//read-only, 32-bit hashes of just class name and class plus type, should be totally unique...
	bool Transitory;	//If this is true, entity type will NOT be deleted by Server, and can safely be deleted
	//client-side.  ONLY set if entity type has a fixed low Time To Live, or if entity is a client-side effect
	//and will delete itself if it detects the deletion of higher-up non-transitory entities (e.g. bauble).
	//If this is false, client can NOT delete entity without command from server.  If it is true, server will
	//NOT send delete commands to clients.
	EntityTypeBase *Find(const char *c, const char *t){
		ClassHash hash = MakeHash(c, t);
		EntityTypeBase *et = this;
		while(et){
			if(hash == et->thash) return et;
			et = et->NextLink();
		}
	//	if(CmpLower(tname, t) && CmpLower(cname, c)) return this;
	//	if(NextLink()){
	//		return NextLink()->Find(c, t);
	//	}else{
		OutputDebugLog(CStr("Could not find Entity ") + c + "/" + t + "\n");
		return 0;
	//	}
	};
	EntityTypeBase *Find(ClassHash hash){
		EntityTypeBase *et = this;
		while(et){
			if(hash == et->thash) return et;
			et = et->NextLink();
		}
	//	if(hash == thash) return this;
	//	if(NextLink()){
	//		return NextLink()->Find(hash);
	//	}else{
		OutputDebugLog(CStr("Could not find Entity with Hash ") + String((int)hash) + "\n");
		return 0;
	//	}
	};
	EntityTypeBase(ConfigFile *cfg, const char *c, const char *t);
	virtual ~EntityTypeBase(){ };
public:
	virtual EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0) = 0;	//Manufactures an entity of type of current entity type object.
	virtual bool CacheResources(){
		if(ResCached) return true;	//Follow this model in Entity Type sub-classes!
		ResCached = true;
		//Cache resources here.  Upon critical failure, do:
		//return ResCached = false;
		return ResCached = true;
	};
	virtual void ListResources(FileCRCList *fl){
		// Do nothing - should find CRCs of all files to be cached
		if(!ResListed) ResListed = true;
	};
	virtual void UnlinkResources(){
		ResCached = false;
		ResListed = false;
	};
public:
	virtual int InterrogateInt(const char *thing){ return 0; };
	virtual float InterrogateFloat(const char *thing){ return 0.0f; };	//Generic key/value interface for interrogating unknown entity types.
	virtual CStr InterrogateString(const char *thing);
};
typedef LinklistBase<EntityTypeBase> EntityTypeBaseHead;

//Detail about resource caching:  Avoid Circular Caching at all costs!!  To do this, set ResCached
//to true as caching function is entered, and set to final value only when returning.

#define ATT_NAME			0x80000003
//Set/read Name string
#define ATT_PING			0x80000004
//Set/read Ping time
#define ATT_CONNECTED_ENT	0x80000005
//Set/get Primary Connected Entity (parent, usually)
#define ATT_CMD_SPAWN				0x80000008
#define ATT_CMD_FIRSTSPAWN			0x80000009
//turn off/on spawning at first waypoint for non-race maps
#define ATT_SPAWNATFIRSTWAYPOINT	0x8000000A
//Use with SetInt to spawn entity at a spawn point.
#define ATT_CMD_NICEREMOVE	0x8000000f
//For certain ents, asks it to remove itself at its leisure.
#define ATT_MATRIX3			0x80001000
#define ATT_MATRIX43		0x80001001
//
#define ATT_TEAMID			0x80002001
//Queries the Team Identifier of the Entity (Meshes and derivatives).

//Base class for entity types.  Think() is pure virtual, and must be overridden in subclass.
enum RemoveType
{
	REMOVE_NONE = 0,
	REMOVE_NORMAL = 0x01,
	REMOVE_FORCE = 0x02
};

class EntityBase : public LinklistBase<EntityBase> {
private:
	static EntityGID NextGID;
	static EntityGID SpecialNextGID;
protected:
	static VoxelWorld *VW;
public:
	static void SetVoxelWorld(VoxelWorld *vw){ VW = vw; };
	VoxelWorld *GetVW(){ return VW; };
	static void SetSpecificNextGID(EntityGID nextone);
public:
//	EntityBase(){ };
	EntityBase(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags);
	void InitialInit();	//Must be called immediately after construction!  This is so virtual functions of derived class can be called.
	virtual ~EntityBase();
protected:
	//For derived class use only.
	int RegisterEntity(const char *name);
	EntityBase *FindRegisteredEntity(const char *name);
	int UnregisterEntity();
//	int PacketToSelf(TransmissionMode tm, const char *data, int len);
	int QueuePacket(TransmissionMode tm, BitPackEngine &bpe, float priority = 1.0f);//{ return VW->QueueEntityPacket(this, tm, bpe, priority); };
	int QueuePacket(TransmissionMode tm, char *data, int len, float priority = 1.0f);//{ return VW->QueueEntityPacket(this, tm, data, len, priority); };
	//
	bool Rebucket();	//Removes current spatial bucketing and rebuckets based on new pos.
public:
	//Linklist nodes for bucketing by VoxelWorld.
	EntityBucketNode GIDNode;
	EntityBucketNode PosNode;
public:	//Entity functions common to all types.
	virtual bool Think() = 0;	//Return value is false if object should be deleted.
	virtual void DeliverPacket(const unsigned char *data, int len){ return; };	//Override this if you want to receive packets sent by the server destined for this entity.
	virtual int ExtraCreateInfo(unsigned char *data, int len){ return 0; };	//Override to fill out an _outgoing_ packet delivered to self along with Create spawned to clients; return amount of buffer used.
//	virtual bool InitCollide(){ return false; };	//Entity will add itself to collision sector buckets in VW.
	virtual bool CollisionWith(EntityBase *collider, Vec3 colpnt){ return false; };	//When your entity detects a collision with another, call this member passing (this).  Do not call CollisionWith of other entity inside CollisionWith, lest circular loops ensue!
		//Return false if velocity is NOT shared, true if the collidee takes half the whack.
	virtual void SetPos(Vec3 pos){ if(pos) CopyVec3(pos, Position); Rebucket(); };
	virtual void SetRot(Rot3 rot){ if(rot) CopyVec3(rot, Rotation); };	//Set spatial values.
	virtual void SetVel(Vec3 vel){ if(vel) CopyVec3(vel, Velocity); };
	virtual void SetSpatial(Vec3 pos, Rot3 rot, Vec3 vel){
		SetPos(pos); SetRot(rot); SetVel(vel); Rebucket();
	};
	virtual bool SetInt(int type, int att){ return false; };	//Set arbitrary attribute of entity.
	virtual int QueryInt(int type){ return 0; };	//Arbitrary status query.  Entity defined.
	virtual bool SetFloat(int type, float att){ return false; };	//Set arbitrary attribute of entity.
	virtual float QueryFloat(int type){ return 0.0f; };
	virtual bool SetString(int type, const char *s){ return false; };
	virtual CStr QueryString(int type){ return ""; };
	virtual bool SetVec(int type, const void *v){ return false; };
	virtual bool QueryVec(int type, void *out){ return false; };
	virtual EntityGroup Group(){ return GROUP_ACTOR; };	//Queries group type of entity object.
	virtual bool CacheResources(){ return TypePtr->CacheResources(); };
	virtual void ListResources(FileCRCList *fl){ TypePtr->ListResources(fl); };
	virtual void UnlinkResources(){ TypePtr->UnlinkResources(); };	//Override these if you need to do entity-local stuff after global entity resources cached.
	void Remove(){
		if(RemoveMe == REMOVE_NONE) RemoveMe = REMOVE_NORMAL;
	};
//	virtual int AddVelocity(Vec3 vel){ return 0; };	//Use this to send shock-wave style velocity to an entity, and to accept such velocity.
//	virtual int AddDamage(float damage, int teamid = 0, Vec3 source = NULL){ return 0; };	//Use this to apply damage (1.0 == healthy tank).  teamid is for teams of units that can't damage each other, source is to make angular (usually armor) modifications to damage value.
	//Scratch that, may try handling splash damage through standard collision interface.
	virtual int TerrainModified(){ return 0; };	//Lets an entity know the terrain under it may have been modified.
public:
	EntityTypeBase *TypePtr;	//Pointer to static type info for subtype.
//	int Class;	//Entity class type identifier.
	Vec3 Position;	//Position of entity.
	Rot3 Rotation;	//Rotation of entity.
	Vec3 Velocity;	//Velocity.
	float Mass;		//For collision physics.
	int ID;			//Identification number, world designer assigned.
	int Flags;		//Bitflags, designer assigned.
	Vec3 BoundMin;	//Bounding box minimums.
	Vec3 BoundMax;	//Bounding box maximums.
	float BoundRad;	//Bounding radius.
	EntityGID GID;	//Globally unique numeric ideintifier for this entity.
	RemoveType RemoveMe;	//Kill on next pass?
	bool CanCollide;	//Will be added to collision buckets?  (Even if 0 can still CHECK for collision with others itself)
	bool CanThink;	//Thinkable?
	bool MirroredOnClients;	//Informative flag set externally; has entity been broadcast to clients?
private:
	char Registered;	//Registered as a named entity?
};
typedef LinklistBase<EntityBase> EntityBaseHead;


//Generic entity type holding class.
class EntityGenericType : public EntityTypeBase {
public:
	EntityGenericType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t) { };
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
};

//Generic entity class.
class EntityGeneric : public EntityBase {
public:
	EntityGeneric(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityGeneric(){ };
public:
	bool Think(){ return true; };
};

//
//  MixIn Classes for specific helper actions.
//
//***************************************************************
//             Chains a number of additional entity creations to initial one.  Created ents don't chain further.
//             Up to 10 entity class/types can be chained.
//***************************************************************
//Usage:  Use as a base class in an EntityType derived class.  Call constructor with config file
//and class name in EntityType constructor.  Call CacheResources with VW pointer in CacheResources
//member.  Call TypePtr->SpawnChains(this) in EntityBase derived class.
//
#define ENTITYFLAG_NOCHAIN		0x40000000
//This gets passed on to disable entity chaining in children.
#define ENTITYFLAG_NOCHAINFORCE	0x20000000
//Forces entity with this flag to pass nochain flag onto chainees, regardless of nochain override.
#define MAX_MIXIN_CHAIN 10
#define CHAINTYPE_ALL		0x00
#define CHAINTYPE_RANDOM	0x01
#define CHAINTYPE_CYCLE		0x02
class MixinEntityChain{
private:
	CStr DefClass;
	CStr Chains[MAX_MIXIN_CHAIN];
	int nChains;
	int ChainType;
	int NextChain;
	int MultiplyChain;
	int NochainOverride;
public:
	MixinEntityChain(ConfigFile *cfg, const char *c);
	~MixinEntityChain();
	bool CacheResources(VoxelWorld *VW);	//Call in resource caching function of Class.
	void ListResources(VoxelWorld *VW);	
	void SpawnChains(EntityBase *passinthis);	//Call this in your entity constructor, passing in "this".
protected:
	void Spawn(EntityBase *ent, int which, int server);
};


//GOD Entity.  Does Godly stuff.  One per world.  Worship it, peon!
//Registers itself as "GOD".
#define ATT_ECO_CRUD		0x40000000
//For getting ecosystem crud parameters from GOD.  Add index of ecosystem requested.
#define ATT_GRAVITY_X		0x20000001
#define ATT_GRAVITY_Y		0x20000002
#define ATT_GRAVITY_Z		0x20000003
#define ATT_FRICTION		0x20000004
//For getting meters per second gravitic forces.  Though after finding ent registered as GOD, should be safe to cast and grab vector...
#define ATT_STATUS_MESSAGE	0x20000010
//SetString to add a message to the status area.
#define ATT_STATUS_PRIORITY	0x20000011
//SetInt to set the priority of the message, 0 being lowest, for colors and beeps.
#define ATT_CHATLINELEN		0x20000020
//QueryInt to find the length of the current player chat line being typed.
//
//SetInt these to activate chatting
#define ATT_CMD_CHAT		0x20001001
#define ATT_CMD_TEAMCHAT	0x20001002
//
#define MAX_STATUS_PRIORITY 10
//
#define ATT_ANNOUNCER_ID	0x20002001
//To specify the PlayerEnt GID who should hear this, or 0 for all.
#define ATT_ANNOUNCER_HASH2	0x20002002
//To specify a second, immediately chained announcement.
#define ATT_CMD_ANNOUNCER	0x20002003
#define ATT_CMD_REFRESHSTAT	0x20002004
#define ATT_CMD_HIDESTAT	0x20002005
//Set int these, with the CMD last, param holding sound thash.
//
class EntityGodType : public EntityTypeBase {
friend class EntityGod;
protected:
	//These hold data on the ecosystems for which crud entity to spew, and with what force required.
	CStr EcoCrud[MAXECO];
	float EcoCrudForce[MAXECO];
	//
	Vec3 type_gravity;	//Not necessarily straight down only.
	float type_friction;
	//Other worldly config options here...
	//List of tanks with placings too.
	CStr type_statusfont;
	float type_statusx, type_statusy, type_statusw, type_statush, type_statuslife;
	int type_numstatuslines;
	CStr type_statprisound[MAX_STATUS_PRIORITY];
	Vec3 type_statpricol[MAX_STATUS_PRIORITY];
public:
	EntityGodType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);	//Manufactures an entity of type of current entity type object.
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
private:
	void ReadCfg(ConfigFile *cfg);
};
//#define MAX_STATUS_LINES 10
#define MSG_HEAD_ANNOUNCER		(MSG_HEAD_USERMSG + 0)
#define MAX_ANNOUNCEMENTS 4
class EntityGod : public EntityBase {
public:
	Vec3 gravity;
	float friction;
	int MaxStatusLines;
	CStr *StatusLines;
	int *StatusPriorities;
	int *StatusTimes;
	EntityGID *StatusEnt;
	int curstat, msglines;
	CStr chatline;
	EntityGID chatent;
	//
	EntityGID announcer_id[MAX_ANNOUNCEMENTS], a_id;
	ClassHash announcer_hash[MAX_ANNOUNCEMENTS], announcer_hash2[MAX_ANNOUNCEMENTS], a_hash, a_hash2;
	EntityGID announceent;
	int annfifohead, annfifotail;
public:
	EntityGod(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntityGod();
	bool Think();
	void DeliverPacket(const unsigned char *data, int len);
//	bool SetFloat(int type, float attr);
	bool SetInt(int type, int attr);
	bool SetString(int type, const char *s);
	int QueryInt(int type);
	float QueryFloat(int type);
	CStr QueryString(int type);
	EntityGroup Group(){ return GROUP_ACTOR; };//GROUP_OMNIPOTENT
};


//***************************************************************
//             And now for a useful entity type.
//***************************************************************
//Holds image for sky background in voxel world.
class EntitySkyplaneType : public EntityTypeBase {
public:
	CStr texturefile, detailtexturefile, watertexturefile;
	Image *texture, *detailtexture, *detailtexturemt, *watertexture, *envtexture;
	float wateralpha, waterscale, waterenv;
	Vec3 fogcolor;
	Vec3 lightvec;
	float ambterrain, ambentity;
	int scorcheco;
	float detailrotspeed, detailrotradius;
	float skyrotatespeed;
	//
//	//These hold data on the ecosystems for which crud entity to spew, and with what force required.
//	CStr EcoCrud[MAXECO];
//	float EcoCrudForce[MAXECO];
//	//
public:
	EntitySkyplaneType(ConfigFile *cfg, const char *c, const char *t);// : EntityTypeBase(c, t) { };
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
private:
	void ReadCfg(ConfigFile *cfg);
};
#define ATT_SKYPLANE_ACTIVE 0
//#define ATT_ECO_CRUD 1024
class EntitySkyplane : public EntityBase {
public:
	bool Active;
public:
	EntitySkyplane(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntitySkyplane(){ };
	bool Think();
	bool SetInt(int type, int val);//{ Active = val; return 1; };
	int QueryInt(int type);//{ return Active; };
	float QueryFloat(int type);
	CStr QueryString(int type);
	EntityGroup Group(){ return GROUP_ACTOR; };
};
//***************************************************************
//             SkyBox Entity, version of SkyPlane, 6 sided sky-cube.
//***************************************************************
#define SKYBOX_IMAGES 5
#define SKYBOX_FRONT 0
#define SKYBOX_RIGHT 1
#define SKYBOX_BACK 2
#define SKYBOX_LEFT 3
#define SKYBOX_UP 4
#define SKYBOX_DOWN 5
class EntitySkyboxType : public EntitySkyplaneType {
friend class EntitySkybox;
private:
	CStr type_skyboxname;
	ImageNode *type_skybox[SKYBOX_IMAGES];
	int type_halfskybox, type_filtersky, type_autofog;	//Are we just the upper half of a skybox?
	Vec3 type_colorgain, type_colorbias, type_colorscale, type_colorshift;
public:
	EntitySkyboxType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
private:
	void ReadCfg(ConfigFile *cfg);
};
class EntitySkybox : public EntitySkyplane {
public:
	EntitySkybox(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntitySkybox(){ };
	bool Think();
};
//***************************************************************
//             Sprite Entity.
//***************************************************************
class EntitySpriteType : public EntityTypeBase {
public:
	CStr texturefile;
	ImageNode *texture;
	float type_w, type_h, type_base;
	int type_rendflags;
	float type_fade, type_fadebias;
	AlphaGradType type_alphagrad;
	bool type_transparent;
	int type_sizebias;
	float type_alphagradbias;
	int type_restonground;
	float type_groundoffset;
	int type_forcemaxtexres;
public:
	EntitySpriteType(ConfigFile *cfg, const char *c, const char *t);// : EntityTypeBase(c, t) { };
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
	CStr InterrogateString(const char *thing);	//Responds to "TextureName"
};
//
//Use QueryString with this to retrieve ResourceManager name of tank's texture.
#define ATT_TEXTURE_NAME	0x74850001
//QueryVec for ptr.
#define ATT_TEXTURE_PTR		0x74850002
//QueryInt for flags.
#define ATT_RENDFLAGS		0x74850003
//
#define ATT_SPRITE_ACTIVE	0x74850101
#define ATT_SPRITE_FRAME	0x74850102
//
#define ATT_FADE			0x74850105
#define ATT_TYPE_FADE		0x74850106
//
class EntitySprite : public EntityBase {
public:
	bool Active;
	float w, h;
	int frame;
	float fade;
	SpriteObject spriteobj;
public:
	EntitySprite(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntitySprite(){ };
	bool Think();
	bool SetInt(int type, int val);//{ ifActive = val; return true; };
	int QueryInt(int type);//{ return Active; };	//ATT_RENDFLAGS
	bool SetFloat(int type, float att);//{ return false; };	//Set arbitrary attribute of entity.
	float QueryFloat(int type);//{ return 0.0f; };
	CStr QueryString(int type);
	bool QueryVec(int type, void *out);	//ATT_TEXTURE_PTR
	EntityGroup Group(){ return GROUP_PROP; };
	int TerrainModified();
};
//***************************************************************
//             Generic Polygon Mesh entity.
//***************************************************************
class EntityMeshType : public EntitySpriteType, public MixinEntityChain {//EntityTypeBase {
public:
	CStr meshfile, meshfile1, meshfile2;//, texturefile;
	Mesh *mesh;
//	ImageNode *texture;
	bool type_collide;
	float type_scale;
	//, type_fade,
	float type_mass, type_lodbias;
	//int type_rendflags,
	bool type_center;
	Vec3 type_meshoffset;
	Vec3 type_scalematrix;
	int type_scalematrixon;
	Rot3 type_constrotate;
	Vec3 type_constwave;
	float type_constwavetime;
	int constmovement;	//flag
	int type_directmatrix;	//Means rotation is ignored, and matrix in meshobj is used directly.
	//When this is set, use SetPos() and SetVec(ATT_MATRIX3, mat33) to punch a position and a
	//rotation matrix directly into the mesh for the entity.  The advantages are no euler to
	//matrix conversions, and entity-Think()-order independence for the orientation change, since
	//the meshobj isn't rendered until all entity thinking is done.
	EntityGroup type_group;
	int type_secondaryfrustum;
public:
	EntityMeshType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
};
//Global function, for use by other more simplistic mesh loading classes.
void ReadMeshRendFlags(ConfigFile *cfg, int &rendflags);
//
class EntityMesh : public EntitySprite {//EntityBase {
public:
//	float fade;
	MeshObject meshobj;
	unsigned int teamid;
public:
	EntityMesh(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityMesh(){ };
	bool Think();
	EntityGroup Group();//{ return GROUP_PROP; };
	bool SetInt(int type, int val);
	int QueryInt(int type);
	bool SetVec(int type, const void *v);
	bool QueryVec(int type, void *out);
};
//***************************************************************
//             Text Font entity.
//***************************************************************
//Specify character X and Y (top left relative) in Pos X and Y, and character size
//in Rotation A and B.
class EntityTextType : public EntitySpriteType {//TypeBase {
public:
	int charsx, charsy;
	unsigned char charmap[256];
	int type_mixedcase;
public:
	EntityTextType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
};
#define ATT_TEXT_STRING		0xdb9a1000
#define ATT_TEXT_OPACITY	0xdb9a1001
#define FLAG_TEXT_FADE 0x01
#define FLAG_TEXT_CENTERX 0x02
#define FLAG_TEXT_CENTERY 0x04
//
#define MSG_HEAD_SETTEXT 0xfe
//For propagating text string to clients, if text is force-displayed on clients.
//
class EntityText : public EntitySprite {//EntityBase {
public:
	StringObject stringobj;
	float w, h, x, y, opac;
	CStr str;
	int timeofbirth, timetodie;
public:
	EntityText(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityText(){ };
	bool Think();
	bool SetFloat(int type, float val);
	float QueryFloat(int type);
	bool SetString(int type, const char *s);
	CStr QueryString(int type);
	EntityGroup Group(){ return GROUP_ACTOR; };
	void DeliverPacket(const unsigned char *data, int len);
};
//***************************************************************
//             Icon entity.
//***************************************************************
//Specify icon X and Y (top left relative) in Pos X and Y, and size
//in Rotation A and B.
class EntityIconType : public EntitySpriteType {
public:
	EntityIconType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
};

class EntityIcon : public EntitySprite{
private:
	TilingTextureObject screenobj;
	float opac;
public:
	EntityIcon(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityIcon(){ };
	bool SetFloat(int type, float val);
	float QueryFloat(int type);
	bool Think();
};

class EntityChamfered2DBoxType : public EntityTypeBase {
public:
	float chamferwidth;
public:
	EntityChamfered2DBoxType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
};
class EntityChamfered2DBox : public EntityBase{
private:
	Chamfered2DBoxObject screenobj;
	float red, green, blue;
	float opac;
public:
	EntityChamfered2DBox(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityChamfered2DBox(){ };
	bool SetFloat(int type, float val);
	float QueryFloat(int type);
	bool Think();
};


//***************************************************************
//             Way-Point Entity 
//***************************************************************

#define MAINWAYID 100
#define PATHWAYID 1000000

class EntityWaypointType;

class WaypointNode : public LinklistBase<WaypointNode> {
public:
	Vec3 Pos, Vel;	//Position of waypoint and suggested velocity vector.
	float suggestedradius;	//Recommended radius of approach.
	unsigned int mainid, subid;	//Main waypoint and sub-waypoint ids.
	unsigned int flags;
	int teamspawntype;
	EntityWaypointType *type;
};

#define WAYPOINT_SPAWNHEREFLAG 0x0001

class EntityWaypointType : public EntityTypeBase {
public:
	float type_hitradius;
	float type_hitradiusadd;
public:
	EntityWaypointType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
};
class EntityWaypoint : public EntityBase {
protected:
	WaypointNode wayhead;
	bool cylindrical;	//Is Y distance ignored when testing for passage?
public:
	EntityWaypoint(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0, int teamspawn = 0);
	bool Think();
	EntityGroup Group(){ return GROUP_PROP; };
public:
	int WalkWaypoints(int current, Vec3 pos, Vec3 vel, WaypointNode **wayreturn = NULL, WaypointNode **majorway = NULL);
		//Starts with 'current' waypoint index and walks forwards a maximum of 1
		//waypoints, returning a pointer to the new travel-to waypoint in wayreturn.
		//Can optionally return a pointer to the next major waypoint as well.
		//Failure is a -1 return index.
	int CountMajorWayPoints();
	WaypointNode *WaypointByIndex(int index);
		//Returns waypoint at index in list (0 is first waypoint).
	int ClosestWaypoint(Vec3 pos, WaypointNode **wayreturn = NULL);
		//Returns closest major waypoint to position.
	float HitRadius();
};

//***************************************************************
//             Sound entity.
//***************************************************************
//Specify volume in Rot[0].  NYET, NON, NO!  Bad idea!  Argh...
#define ATT_LOOP	0x82481007
//QueryInt() this.
#define MAX_SOUNDS 10
//
#define FLAG_SOUND_PLAYANNOUNCER	0x10000000
//Set in flag to allow Announcer sounds to play.
class EntitySoundType : public EntityTypeBase {
public:
	CStr soundfile[MAX_SOUNDS];
//	CStr soundfile, altsoundfile, altsoundfile2, altsoundfile3;
	SoundNode *sound[MAX_SOUNDS];//, *altsound, *altsound2, *altsound3;
	int soundid[MAX_SOUNDS];	// Russ - local vox stuff
	float type_volume;
	float type_freqvariance;
	float type_priority;
	bool type_loop;
	float type_minmaxdistscale;
	int type_is2dsound;
	float type_rampup, type_rampdown, type_rampfreq;
	int type_announcer;
	float type_volumebias;	//This is a load time param for the sound.
	int localise;	// Russ - local vox stuff
public:
	EntitySoundType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
	int InterrogateInt(const char *thing);	//Responds to "loop".
};
class EntitySound : public EntityBase {
public:
	int chan, wave;
	float volume, freq;
	EntityGID connectedent;
	int rampup, rampdown, loiter;
public:
	EntitySound(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntitySound();
	bool Think();
	EntityGroup Group(){ return GROUP_ACTOR; };
	int QueryInt(int type);
	bool SetInt(int type, int attr);	//Responds to ATT_CONNECTED_ENT
};


#endif

