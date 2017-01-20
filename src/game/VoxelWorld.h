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

//VoxelWorld class, handling all the mundane details of keeping
//entities in line.

#ifndef VOXELWORLD_H
#define VOXELWORLD_H

#include "EntityBase.h"
#include "FileManager.h"
#include "ResourceManager.h"
#include "CfgParse.h"
#include "Terrain.h"
#include "Render.h"
#ifndef HEADLESS
#include "GL/GLPolyRender.h"
#endif
#include "Vector.h"

#include "FileCRCList.h"

#include "Networking.h"

#include "BitPacking.h"

#ifndef HEADLESS
#include "GL/GLTerrainRender.h"
#include "GL/GLTerrainRender2.h"
#include "GL/GLTerrainRender3.h"
#endif

#define ENTITYFILEVERSION 1

#define THINK_NOTRACKS		0x01
#define THINK_NOWALKIES		0x10
#define THINK_NOEFFECTS		0x20
#define THINK_LIGHTSMOKE	0x40

//Entity Bucketing Class.
//Handles 2D (spatial) and 1D (identificationary) bucketing.
//Everything inline.

//struct EntityLink{
//	EntityBase *ent;
//	EntityLink *next;
//};
class EntityBucket{
private:
//	EntityLink *linkfarm;
//	EntityLink **bucket;
	EntityBucketNode *bucket;
	int bucketw, bucketh;	//number of buckets in x and y.
	int shiftw, shifth;
	int shiftx, shifty;	//pow2 size of each bucket.
	int halfx, halfy;
//	int nlinkfarm;
//	int link;
	bool AddEnt(int b, EntityBucketNode *bn){//EntityBase *ent){	//Does not sanity check bucket number.
	//	if(link < nlinkfarm && ent){
	//		linkfarm[link].ent = ent;
	//		linkfarm[link].next = bucket[b];
	//		bucket[b] = &linkfarm[link];
	//		link++;
	//		return true;
	//	}
	//	return false;
		if(bucket && bn){
			bn->UnlinkItem();
			bucket[b].InsertObjectAfter(bn);
			return true;
		}
		return false;
	};
public:
	EntityBucket(){
	//	linkfarm = NULL;
		bucket = NULL;
		bucketw = bucketh = shiftx = shifty = shiftw = shifth = 0;
	//	nlinkfarm = link = 0;
	};
	~EntityBucket(){
		Free();
	};
	bool InitBuckets(int w, int h, int x, int y){//, int maxents){	//Init two dimensional buckets.
		Free();
		shiftw = HiBit(w);
		shifth = HiBit(h);
		bucketw = 1 << shiftw;
		bucketh = 1 << shifth;
		shiftx = HiBit(x);
		shifty = HiBit(y);
		halfx = 1 << (shiftx - 1);
		halfy = 1 << (shifty - 1);
		if(bucketw > 0 && bucketh > 0){// && maxents > 0){
		//	maxents = std::max(maxents, bucketw * bucketh * 2);
		//	if(linkfarm = new EntityLink[maxents]){
		//		nlinkfarm = maxents;
		//	if(bucket = new EntityLink*[bucketw * bucketh]){
			if(bucket = new EntityBucketNode[bucketw * bucketh]){
				ClearBuckets();
				return true;
			}
		//	}
		}
		Free();
		return false;
	};
	bool InitBuckets(int n){//, int maxents){	//Init one dimensional bucket array.
		return InitBuckets(n, 1, 1, 1);//, maxents);
	};
	void ClearBuckets(){	//We are using pool-allocated link objects, so no need to delete them!
		//No were not, now we're using link objects created and deleted externally.
		for(int b = 0; b < bucketw * bucketh; b++) bucket[b].UnlinkItem();
			//bucket[b] = NULL;
	//	link = 0;
	};
	int WhichBucket(int x, int y){
		return ((x >> shiftx) & (bucketw - 1)) + (((y >> shifty) & (bucketh - 1)) << shiftw);
	};
	int WhichBucket(int x, int y, int quad){	//Returns one of the 4 buckets in a box-filter pattern around the wanted point.
		return ((((x - halfx) >> shiftx) + (quad & 1)) & (bucketw - 1)) +
			(((((y - halfy) >> shifty) + (quad >> 1)) & (bucketh - 1)) << shiftw);
	};
	bool AddEntity(EntityBucketNode *bn, int x, int y){//EntityBase *ent, int x, int y){	//Two dimensional bucket add.
		return AddEnt(WhichBucket(x, y), bn);//ent);
	};
//	bool AddEntity(EntityBucketNode *bn, int x, int y, int w, int h){	//Two-dimensional with size, so possible multiple-bucket add.
//		int w2 = w >>1;	//Add bucket entries at all four corners of bound, if necessary.
//		int h2 = h >>1;
//		int b1 = WhichBucket(x - w2, y - h2);	//UL
//		int b2 = WhichBucket(x + w2, y - h2);	//UR
//		int b3 = WhichBucket(x - w2, y + h2);	//DL
//		int b4 = WhichBucket(x + w2, y + h2);	//DR
//		if(b2 != b1) AddEnt(b2, ent);	//UR not UL?
//		if(b3 != b1) AddEnt(b3, ent);	//DL not UL?
//		if(b4 != b2 && b4 != b3) AddEnt(b4, ent);	//DR not DL or UR?
//		return AddEnt(b1, ent);	//Add UL regardless.
//	};
	bool AddEntity(EntityBucketNode *bn, int n){	//EntityBase *ent, int n){	//One dimensional add-entity.
		return AddEnt(n & (bucketw - 1), bn);//ent);
	};
	EntityBucketNode *GetBucket(int x, int y){
		if(bucket) return bucket[WhichBucket(x, y)].NextLink();
		return NULL;
	};
	EntityBucketNode *GetBucket(int x, int y, int quad){
		if(bucket) return bucket[WhichBucket(x, y, quad)].NextLink();
		return NULL;
	};
	EntityBucketNode *GetBucket(int n){
		if(bucket) return bucket[n & (bucketw - 1)].NextLink();
		return NULL;
	};
	void Free(){
	//	if(linkfarm) delete [] linkfarm;
	//	linkfarm = NULL;
	//	if(bucket) delete [] bucket;
		if(bucket){
			for(int n = 0; n < bucketw * bucketh; n++){
				bucket[n].UnlinkItem();
			}
			delete [] bucket;
		}
		bucket = NULL;
		bucketw = bucketh = shiftx = shifty = shiftw = shifth = 0;
	//	nlinkfarm = link = 0;
	};
};

class RegEntList : public CStr, public LinklistBase<RegEntList> {
public:
	EntityBase *entity;
	RegEntList() : entity(0) {};
	RegEntList(EntityBase *ent, const char *name) : CStr(name), entity(ent) {};
};


//Application-side handling of client connects and disconnects.

#define BYTE_HEAD_CREATEENT	0x01
#define BYTE_HEAD_MSGENT	0x02
#define BYTE_HEAD_DELETEENT	0x03
#define BYTE_HEAD_CONTROLS	0x04
#define BYTE_HEAD_CONNACK	0x05
#define BYTE_HEAD_CONNINFO	0x06
#define BYTE_HEAD_SERVINFO	0x07
#define BYTE_HEAD_DUMPEND	0x08
#define BYTE_HEAD_CRATER	0x09
#define BYTE_HEAD_TIMESYNCH	0x0a
#define BYTE_HEAD_GAMEMODE	0x0b
#define BYTE_HEAD_STATUSMSG	0x0c
#define BYTE_HEAD_CHATMSG	0x0d
#define BYTE_HEAD_PING		0x0e
#define BYTE_HEAD_PONG		0x0f
#define BYTE_HEAD_ENTTYPE	0x10
#define BYTE_HEAD_RESCRC	0x11
#define BYTE_HEAD_DOWNLOAD	0x12
#define BYTE_HEAD_COMMAND	0x74

#define MSG_HEAD_PLAYERENT	0xcc
#define MSG_HEAD_POSITION	0x02
#define MSG_HEAD_LINKENTITY	0x03
#define MSG_HEAD_STATUS		0x04
#define MSG_HEAD_NAME		0x05
#define MSG_HEAD_USERMSG	0x10
#define MSG_HEAD_FLAGSTATUS	0x11
//These shouldn't be out here...  Unless they're handled in a high-level manner, by a high base entity
//class or the EntityBase itself.  Any class-specific message identifiers should be private only.
//PlayerEnt is a special case though, which should be global, but only acknowledged by e.g. Racetanks.

#define MAX_CLIENTS 32

//Crater propogation.

struct CraterEntry : public LinklistBase<CraterEntry>{
	int x, y;
	float r, d, scorch;
	CraterEntry(){ };
	CraterEntry(int X, int Y, float R, float D, float Scorch) : x(X), y(Y), r(R), d(D), scorch(Scorch) { };
	~CraterEntry(){ };
};

#define TEAMID_NONE 0

class VoxelClient{
public:
	VoxelClient(){ Init(); };
	~VoxelClient(){};
	void Init(){
		ClientEnt = 0;
		EntType = 0;
		ClientName = "";
		ClientRate = 0;
		Connected = false;
		LastCraterSent = NULL;
		TeamID = TEAMID_NONE;
	};
	EntityGID ClientEnt;
	ClassHash EntType;
	CStr ClientName;
	int ClientRate;
	bool Connected;
	CraterEntry *LastCraterSent;
	int TeamID;
	//TODO:  Actually receive this from client and set it in tank.  Not done yet!!!
};

enum ClientStatus{
	STAT_Disconnect = 0,
	STAT_Connecting,
	STAT_ConnectionAccepted,
	STAT_ConnectionInfoSent,
	STAT_SynchEntityTypes,
	STAT_MapLoaded,
	STAT_MapLoadFailed,
	STAT_RunGameLoop
};

//Stringed config files.

class ConfigFileList : public ConfigFile, public LinklistBase<ConfigFileList> {
public:
	CStr cname, tname;
	ClassHash thash;
	unsigned int ClientChecksum[MAX_CLIENTS];	//For use on server, for checksum replies returned from clients.  Must match to not have to send back config data.
	int ServerSynch;	//Should the server check checksums on this entity type and send to clients?  (No, for meaningless entities such as GUI.)
	ConfigFileList(){
		for(int i = 0; i < MAX_CLIENTS; i++) ClientChecksum[i] = 0;
		ServerSynch = 1;
		thash = 0;
	};
	void Quantify(){	//Must call after loading in config file.
		if(FindKey("class")) GetStringVal(cname);
		if(FindKey("type")) GetStringVal(tname);
		if(FindKey("ServerSynch")) GetIntVal(&ServerSynch);
		thash = MakeHash(cname, tname);
	};
	ConfigFileList *Find(const char *c, const char *t){
		ConfigFileList *cfl = this;
		while(cfl){
			if(CmpLower(t, cfl->tname) && CmpLower(c, cfl->cname)) return cfl;
			cfl = cfl->NextLink();
		}
		return 0;
	};
	ConfigFileList *Find(ClassHash hash){
		ConfigFileList *cfl = this;
		while(cfl){
			if(hash == cfl->thash) return cfl;
			cfl = cfl->NextLink();
		}
		return 0;
	};
};

class InputFIFOEntry{
public:
	CStr name;
	int type;
	float extra;
public:
	InputFIFOEntry(const char *n, int t, float e) : name(n), type(t), extra(e) {};
	InputFIFOEntry() : type(0), extra(0) {};
	void Set(const char *n, int t, float e = 0.0f){
		name = n;
		type = t;
		extra = e;
	};
	void Get(CStr &n, int &t, float &e){
		n = name;
		t = type;
		e = extra;
	};
};

template <int D> class InputFIFO{
private:
	InputFIFOEntry fifo[D];
	int nextin, nextout;
public:
	InputFIFO() : nextin(0), nextout(0) {};
	bool Set(const char *n, int t, float e = 0.0f){
		fifo[nextin].Set(n, t, e);
		nextin = (nextin + 1) % D;
		return true;
	};
	bool Get(CStr &n, int &t, float &e){
		if(nextout != nextin){
			fifo[nextout].Get(n, t, e);
			nextout = (nextout + 1) % D;
			return true;
		}
		return false;
	};
};

//Voxel World handling class.

#define BUCKET_SIZE_ID 1024
//#define BUCKET_SIZE_2D 16
#define BUCKET_SIZE_2D 16

#define MAX_COLLIDERS 16

//#define SOUNDDISTANCE 350.0f
//Distance at which sound is inaudible.

//enum AddEntFlags{
//	ADDENTITY_NORMAL = 0x00,
//	ADDENTITY_FORCENET = 0x01,
//	ADDENTITY_NONET = 0x02
//};
class VoxelWorld : public ResourceManager, public PacketProcessorCallback {
private:
	PacketProcessorCallback *UCB;
	ClientStatus Status;
public:	//Callback functions inherited from PPC above.
	virtual void Connect(ClientID source);
	virtual void Disconnect(ClientID source, NetworkError ne);
	virtual void PacketReceived(ClientID source, const char *data, int len);
	virtual void OutOfBandPacket(sockaddr_in *src, const char *data, int len);
public:
	FileManager FM;
	Terrain Map;
	//
//	RenderEngine VoxelRend;
	RenderEngine *VoxelRend;
#ifndef HEADLESS
	GLRenderEngine GLVoxelRend;
	GLRenderEngine2 GLVoxelRend2;
	GLRenderEngine3 GLVoxelRend3;
#endif
	//
	int SelectTerrainDriver(int driver = -1);	//-1 causes a cycling.
	//
	//
#ifndef HEADLESS
	GLPolyRender GLPolyRend;
#endif
	PolyRender *PolyRend;
	//
	Camera Cam;
	Network Net;

	VoxelClient Clients[MAX_CLIENTS];
	//
	VoxelClient OutgoingConnection;
	//
	CStr MapFileName;
	//
	CStr MapCfgString;	//Config text file storage for map specific stuff, to be accessed by entities.
	ConfigFile MapCfg;
	//
	Vec3 gravity;	//Global repository for current 3D gravitic force, m/s^2.  Usually set by GOD entity, read by other entities.
	Vec3 worldcenter;	//Set by VoxelWorld, this is the center of the world, to be used when offsetting positions for network transmission.
	//
private:
	CraterEntry CraterHead;	//Global queue of craters made, for sending to clients.
	CraterEntry *CraterTail;	//Pointer to tail, for faster tail-adds.  Must delete craterqueue on map loads!!
public:
	EntityTypeBaseHead EntTypeHead;		//Oh BS.  Can't do it, since classes are pure virtual...  Gah!
	ConfigFileList ConfigListHead;
	EntityBaseHead EntHead[ENTITY_GROUPS];
	EntityBucket EntBucket[ENTITY_GROUPS];
	EntityBucket IDBucket;
	//
	ClassHash DefClientEntType;	//The default client entity type if asked for type is invalid.
	//
	int EntitiesToSynch;	//readable status variables for when client is receiving entity synchs.
	int EntitiesSynched;
	//
public:
	void PulseNetwork(PacketProcessorCallback *usercallback = NULL);
	bool BeginClientConnect(const char *address, short serverport, short clientport, EntityTypeBase *enttype, const char *clientname, int clientrate);
	ClientStatus GetStatus(){ return Status; };
	bool InitServer(short port, int maxclients);
	bool SendClientInfo(EntityTypeBase *enttype = NULL, const char *clientname = NULL, int clientrate = 0);	//Resends client entity type, name, and/or rate to server.
	int CountClients();
private:
	bool PrivSendClientInfo(bool firsttime);
public:
	//
#define STATUS_PRI_NORMAL	0
#define STATUS_PRI_PLAYER	1
#define STATUS_PRI_CHAT		2
#define STATUS_PRI_NETWORK	3
	bool StatusMessage(const char *text, int pri = 0, ClientID dest = CLIENTID_BROADCAST, unsigned int teamid = TEAMID_NONE, bool localdisplay = true);
		//Tells the local GOD entity to display a status message, and optionally sends to clients if on server.
	int ChatMessage(const char *text, unsigned int teamid = TEAMID_NONE);	//Sends a chat message to the server, who sends it on to clients.
public:
	VoxelWorld();
	~VoxelWorld();
	bool LoadEntityClasses();	//Be careful using this, as any entities alive through the change will have bad entity type pointers!
	bool RefreshEntityClasses();	//Deletes and rebuilds entity types from config file list.  Make SURE all entities are deleted and all entity classes are resource unlinked before calling this!!!
	int CountEntities(int group);
	int CountEntities(){ return CountEntities(-1); };
	bool ClearEntities();
	bool LoadEntities(IFF *iff);
	bool AppendEntities(IFF *iff);
	bool SaveEntities(IFF *iff);
	bool InitializeEntities();	//Sets up collision buckets and such.  Use after main entities added.
	//Now it is used internally BEFORE entities are loaded...  Hmm.
	//
	int Crater(int cx, int cy, float radius, float depth, float scorch);	//Propogates to clients.
	int TreadMark(float val, int x, int y, int dx, int dy);	//Does not propogate (non-authoritative second order effect, just coloring).
	int AreCratersUpdating(){ return CraterUpdateFlag; };	//Use this to detect when large numbers of craters are being updated.
	//
	EntityTypeBase *FindEntityType(const char *Class, const char *Type);
	EntityTypeBase *FindEntityType(ClassHash hash);
	int EnumEntityTypes(const char *Class, EntityTypeBase **ret, int cookie);	//Enumerates all Types within a Class.  Pass in 0 to begin, then the value returned from then on, until 0 is returned.  Null class string specifies ALL classes.
	//
#define ADDENTITY_NORMAL	0x00
#define ADDENTITY_FORCENET	0x01
//Forces created entity to be broadcast to all clients.
#define ADDENTITY_NONET		0x02
#define ADDENTITY_NOSKIP	0x04
//Causes creation to NOT be skipped by distant entity packet priorities.
	//
	EntityBase *AddEntity(const char *Class, const char *Type,
		Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags, EntityGID specificgid, int AddFlags);
	EntityBase *AddEntity(ClassHash hash,
		Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags, EntityGID specificgid, int AddFlags);
	EntityBase *AddEntity(EntityTypeBase *et,
		Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags, EntityGID specificgid, int AddFlags);
	//Adds an entity to the list and returns a pointer to it.  Do NOT hold pointer for long.
	EntityBase *GetEntity(EntityGID gid);	//Retrieves an entity pointer by GID number.  Don't hold long, unless entity guaranteed to stay alive.
	bool RemoveEntity(EntityBase *ent);	//Removes an entity.
	//
	bool CheckCollision(EntityBase *ent, EntityGroup maxgroup);
	EntityBase *NextCollider(Vec3 returnpnt = NULL);
	//
	int AddGIDBucket(EntityBucketNode *bn, EntityGID gid);
	bool AddPosBucket(EntityBucketNode *bn, Vec3 pos, EntityGroup grp);
	//
	bool LoadVoxelWorld(const char *name, bool loadentities = true, bool mirror = false);
	int ClearVoxelWorld();
	bool TextureVoxelWorld();
	bool TextureVoxelWorld32();
		//Textures from TexIDMap and lightshades in one pass to 32bit color map buffer.
	bool DownloadTextures(bool UpdateOnly = false);
	void UndownloadTextures();
	//
	bool SaveVoxelWorld(const char *name);
	int CacheEntityResources();
		//Returns entities processed.  Caches resources of entities currently
		//active in world, and resources of entities they will spawn.
	bool UnlinkResources();
		//Unlinks (makes bad, requiring recaching) resources of all Entity Types,
		//in world or not.
	bool ListAllResources();
		// Go through entities and create a list of files and their CRCs
	bool ListResources(const char *Class, const char *Type);
		// List resources for a specific entity type
	bool CacheResources(const char *Class, const char *Type);
		//Caches resources of a specific entity type, mainly used by entity type's
		//resource caching to cache spawned entity resources.
	int ThinkEntities(int flags);
		//Returns entities processed.  Pass in msecs for this frame.
	void InputTime(int framemsec);
		//Sets msecs for upcoming frame, for think entities and such.
	void BeginningOfTime(){ msec = 0; vmsec = 0; ffrac = 0.0f; msecdiff = 0; };
		//Resets internal time counter.
	//Add members to Think all entities, and setup collision buckets, and check
	//for collisions between entities.
	void ForceBaseGroup(bool force){ ForceGroup = force; };
		//Activates or deactivates the forcing of all entities into the zeroth group,
		//which might make things easier on the map editor.
private:
	bool ForceGroup;
	unsigned long msec;	//Time stamp for global think.
	signed long msecdiff;	//When client, difference from current client clock to get server clock.
	int vmsec;	//Time since last global think ("length" of this think).
	float ffrac;
#define MSDIFF_HISTORY 16
	signed long msecdiffa[MSDIFF_HISTORY];
	int msecdiffan;
	//
#define SYNCH_EVERY 100
	unsigned long lastsynch;
#define PING_EVERY 1000
	unsigned long lastping;
	//
	int gamemode;
	//
	void SendEntityCreate(ClientID client, EntityBase *e, float transpri = 1.0f);
	void SendEntityDelete(ClientID client, EntityBase *e);
	//
	float PacketPriDist;
	int ProbabilityPacket(EntityBase *ent, TransmissionMode tm, char *data, int len, float priority);
	//
	int CraterUpdateFlag;
	//
	int mirrored;
	//
public:	//Members to be used by entities while thinking etc.
	int SetGameMode(int newmode);	//Game-specific mode flags
	int GameMode(){ return gamemode; };
	//
	int Time(){ return msec; };
		//Global millisecond time stamp.
	int ClientTimeOffset(){ return msecdiff; };
	int FrameTime(){ return vmsec; };
		//Millisecs for current frame.
	float FrameFrac(){ return ffrac; };
		//Fraction of a second for current frame.
	int FrameFlags;
	//
	void SetPacketPriorityDistance(float range);
	//
	int QueueEntityPacket(EntityBase *ent, TransmissionMode tm, BitPackEngine &bpe, float priority = 1.0f);
	int QueueEntityPacket(EntityBase *ent, TransmissionMode tm, char *data, int len, float priority = 1.0f);
	//These functions will send TM_Reliable and TM_Ordered packets no matter what, but TM_Unreliable packets
	//will be probabilistically sent to clients based on distance between the client's entity and this entity.
	//Increase priority (up to anything) to encourage every-frame updating for long distances, decrease it to
	//encourage less packets to be sent.
	//
	int PackPosition(BitPackEngine &bpe, const Vec3 pos, int bits);
	int UnpackPosition(BitUnpackEngine &bpe, Vec3 pos, int bits);
	//
public:
	EntityBase *FindRegisteredEntity(const char *name);	//Finds a registered entity by name.
	int RegisterEntity(EntityBase *ent, const char *name);	//Registers a named entity, if name not taken.
	int UnregisterEntity(EntityBase *ent);
private:
	RegEntList RegEntHead;
	FileCRCList CRCListHead;
	FileCRCList ClientCRCs[MAX_CLIENTS];
	//Will have to flag entity when it is registered, and when entity is deleted, if
	//flagged as regged, walk regent list looking for it.  Hmm, that would require
	//closer cooperation with the EntityBase class itself...  Oh well.
	int nColliders;
	EntityBase *Colliders[MAX_COLLIDERS];
	Vec3 ColliderPoints[MAX_COLLIDERS];
public:
	void InputMousePos(float x, float y);
	void InputMouseButton(int but);	//Use these to set mouse state from operating system.
	float GetMouseX();
	float GetMouseY();	//GUI entities use these to access mouse state.
	int GetMouseButton();
	void InputMouseClicked(int b);
	void InputMouseReleased(int b);
	void ClearMouseFlags();
	int GetMouseClicked();
	int GetMouseReleased();
	//
#define VWK_UP		1
#define VWK_LEFT	2
#define VWK_DOWN	3
#define VWK_RIGHT	4
	//
	void SetChar(char c);	//Adds an ascii char to internal buffer.
	void ClearChar();	//Clears key buffer.
	char GetChar();	//Used by GUI entities to read a character.
	void SetChatMode(int mode);
	int GetChatMode();
	//
	InputFIFO<32> Ififo;
	//
	void SetFocus(EntityGID gid);
	EntityGID GetFocus();	//Used by gui entities to set and read input focus.
	void SetActivated(EntityGID gid);
	EntityGID GetActivated();	//Used by gui entities to set and read input focus.
public:
	int NumMajorWayPts;
private:
	float mousex, mousey;
	int mousebutton, mouseclicked, mousereleased;
#define KEYBUF 64
	char keybuf[KEYBUF];
	int keyin, keyout, chatmode;
	EntityGID guifocus, guiactivated;
};

int CollideBoundingBoxes(Vec3 min1, Vec3 max1, Mat43 mat1,
						 Vec3 min2, Vec3 max2, Mat43 mat2, Vec3 returnpnt);
	//Returns box-1-relative collision point in returnpnt.

//Split entities into three groups:  Nernies, Props, and Actors.
//
//Nernies will probably have NO collisions, but will use a very rarely
//updated spatial bucket for PVS determination (nernies will only be
//displayed up close).  They WILL NOT think.  They will be e.g. plants,
//small rocks, shrubs, etc.  Max around 10,000.
//
//Props will use a seldom-updated spatial bucket for collision
//detection, but will probably be 100% displayed.  It'd be nice to have a
//way to dynamically update just a segment of the spatial buckets to
//allow one entity to move between buckets without pain.  Props
//might think, but probably won't for the most part.  e.g. trees, rocks
//you can smash into, bridges, checkpoints, etc.  Max around 1,000.
//
//Actors will use an every-frame updated spatial bucket for collisions,
//and will always think all the time.  They will be e.g. tanks, cars,
//bullets, explosions, smoke, dirt particles, etc.  Actors can skip adding
//themselves to the collision buckets for e.g. bullets which won't be
//colliding with each other at all.  Max around 100.
//

#endif
