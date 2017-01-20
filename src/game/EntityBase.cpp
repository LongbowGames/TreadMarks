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

//EntityBase

//
//May need some switch to force all entities to be Generic due to weirdness with
//Waypoint entities and level editing.  Although as long as no thinking goes on,
//the set of individual living entities for each way point should remain, so maybe
//there isn't a problem (as long as no thinking in the editor).
//

#include "EntityBase.h"
#include "VoxelWorld.h"
#include "EntityTank.h"
#include "TextLine.hpp"

const int nEnumBlend = 4;
ConfigEnum EnumBlend[nEnumBlend] = {
	{"alpha", SPRITE_BLEND_ALPHA},
	{"add", SPRITE_BLEND_ADD},
	{"sub", SPRITE_BLEND_SUB},
	{"alpha2", SPRITE_BLEND_ALPHA2}
};

VoxelWorld *EntityTypeBase::VW = NULL;
VoxelWorld *EntityBase::VW = NULL;
EntityGID EntityBase::NextGID = 1;	//ID 0 will be NO entity!
EntityGID EntityBase::SpecialNextGID = 0;

void EntityBase::SetSpecificNextGID(EntityGID nextone){
	SpecialNextGID = nextone;
}

#define MAXETCS 10
ENTITYTYPECREATORPTR EntityTypeCreator[MAXETCS];
int nETC = 0;

bool RegisterEntityTypeCreator(ENTITYTYPECREATORPTR etcp){
	if(nETC < MAXETCS && etcp){
		EntityTypeCreator[nETC++] = etcp;
		return true;
	}
	return false;
}//Use this to register each additional Entity Type creation function.

#ifdef WIN32
#define strcasecmp(a, b) _stricmp(a, b)
#endif

EntityTypeBase *CreateEntityType(ConfigFile *cfg){
	if(!cfg) return 0;
	char superclass[256], subclass[256];
	EntityTypeBase *etb;
	//
	//First try registered Entity Type Creators.
	for(int i = 0; i < nETC; i++){
		etb = (EntityTypeCreator[i])(cfg);
		if(etb) return etb;
	}

	//
	//Nope, well then make a Generic entity type with this class/type name.
	if(cfg->FindKey("class") && cfg->GetStringVal(superclass, 256)){
		if(cfg->FindKey("type") && cfg->GetStringVal(subclass, 256)){
			//First try any global entity classes other than generic defined here.
			if(strcasecmp(superclass, "skyplane") == 0) return new EntitySkyplaneType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "skybox") == 0) return new EntitySkyboxType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "sprite") == 0) return new EntitySpriteType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "waypoint") == 0) return new EntityWaypointType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "text") == 0) return new EntityTextType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "icon") == 0) return new EntityIconType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "Chamfered2DBox") == 0) return new EntityChamfered2DBoxType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "sound") == 0) return new EntitySoundType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "mesh") == 0) return new EntityMeshType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "god") == 0) return new EntityGodType(cfg, superclass, subclass);
			//Then just spawn a generic entity type with class/type name.
			return new EntityGenericType(cfg, superclass, subclass);
		}
	}
	return 0;
}

ClassHash MakeHash(const char *c){
	CStr cname = c;
	ClassHash chash = 0;	//Unique identifier for class.
	for(int i = 0; i < cname.len(); i++){	//This should hash 8 letters very damn uniquely, and should be decent for more as well.
		chash ^= (ClassHash)tolower(cname.chr(i));	//Xor in character.
		chash = ((chash <<4) & (~0xf)) | ((chash >>28) & 0xf);	//Rotate hash.
	}
	return chash;
}
ClassHash MakeHash(const char *c, const char *t){
	CStr tname = t;
	ClassHash thash = MakeHash(c);
	for(int i = 0; i < tname.len(); i++){
		thash ^= (ClassHash)tolower(tname.chr(i));	//Xor in character.
		thash = ((thash <<3) & (~0x7)) | ((thash >>29) & 0x7);	//Rotate hash.
	}
	return thash;
}

//
//Basic entity types.
//
EntityTypeBase::EntityTypeBase(ConfigFile *cfg, const char *c, const char *t){
	cname = c;
	tname = t;
	nameid = -1;
	ResCached = false;
	ResListed = false;
	Transitory = false;
	// FIXME: russ, needs to be text ID...
	if(cfg && cfg->FindKey("name")) cfg->GetStringVal(dname);	//Descriptive name, such as "Swarmer Rack".
	if(cfg && cfg->FindKey("nameid")) cfg->GetIntVal(&this->nameid); // String ID of descriptive name
//	thash = chash;	//Class+Type hash.
	chash = MakeHash(c);
	thash = MakeHash(c, t);
}

CStr EntityTypeBase::InterrogateString(const char *thing)
{
	if(thing && CmpLower(thing, "tname")) return tname;
	if(thing && CmpLower(thing, "cname")) return cname;
	if(thing && CmpLower(thing, "dname")) return dname;
	return "";
}

EntityBase::EntityBase(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	TypePtr = et;
	for(int i = 0; i < 3; i++) Velocity[i] = Position[i] = Rotation[i] = 0.0f;
	SetSpatial(Pos, Rot, Vel);
	ID = id;
	Flags = flags;
	if(SpecialNextGID != 0){
		GID = SpecialNextGID;
		SpecialNextGID = 0;
	}else{
		GID = NextGID++;
		if(!VW->Net.IsClientActive()) GID |= ENTITYGID_NETWORK;	//For all entities created on non-client (server), set net flag.
		//This should keep GID comms between client and server from getting mucked up.  Halves the GID namespace,
		//but who cares, it'll run for days without overflowing anyway.
	}
	RemoveMe = REMOVE_NONE;
	CanCollide = false;
	CanThink = true;
	Registered = 0;
	BoundRad = 0.0f;
	Mass = 1.0f;
	GIDNode.entity = PosNode.entity = this;
	MirroredOnClients = false;
}
void EntityBase::InitialInit(){
	if(VW){
		VW->AddGIDBucket(&GIDNode, GID);
		Rebucket();
	}
}
EntityBase::~EntityBase(){
	if(VW){
		for(int n = Registered; n > 0; n--) VW->UnregisterEntity(this);	//Try to unregister as many times as we've been registered.
	}
	GIDNode.UnlinkItem();	//Unlink so they don't 'delete' anyone!
	PosNode.UnlinkItem();
}
int EntityBase::RegisterEntity(const char *name){
	if(VW && VW->RegisterEntity(this, name)) Registered++;
	return Registered;
}
EntityBase *EntityBase::FindRegisteredEntity(const char *name){
	if(VW) return VW->FindRegisteredEntity(name);
	else return 0;
}
int EntityBase::UnregisterEntity(){
	if(VW && VW->UnregisterEntity(this)) Registered--;
	return Registered;
}
bool EntityBase::Rebucket(){
	if(VW) return VW->AddPosBucket(&PosNode, Position, Group());
	return false;
}
/*
int EntityBase::PacketToSelf(TransmissionMode tm, const char *data, int len){
	static char b[1024];
	if(data && len < 1000 && VW){
		b[0] = BYTE_HEAD_MSGENT;
		WLONG(&b[1], GID);
		memcpy(&b[5], data, len);
		return VW->Net.QueueSendClient(CLIENTID_BROADCAST, tm, b, len + 5);
	}
	return 0;
}
*/
int EntityBase::QueuePacket(TransmissionMode tm, BitPackEngine &bpe, float priority){
	return VW->QueueEntityPacket(this, tm, bpe, priority);
};
int EntityBase::QueuePacket(TransmissionMode tm, char *data, int len, float priority){
	return VW->QueueEntityPacket(this, tm, data, len, priority);
};

EntityBase *EntityGenericType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGeneric(this, Pos, Rot, Vel, id, flags);
}
EntityGeneric::EntityGeneric(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntityBase(et, Pos, Rot, Vel, id, flags) {
}


//******************************************************************
//**  GOD Entity  --  Derived Diety
//******************************************************************
EntityGodType::EntityGodType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t){
	type_gravity[0] = 0.0f;
	type_gravity[1] = -10.0f;
	type_gravity[2] = 0.0f;
	type_friction = 0.5f;
	Transitory = false;
	type_statusx = 0.0f;
	type_statusy = 0.5f;
	type_statusw = 0.05f;
	type_statush = 0.05f;
	type_statuslife = 2.0f;
	type_numstatuslines = 10;
	for(int i = 0; i < MAX_STATUS_PRIORITY; i++){
		SetVec3(1, 1, 1, type_statpricol[i]);
	}
	//
	ReadCfg(cfg);
}
void EntityGodType::ReadCfg(ConfigFile *cfg){
	if(cfg){
		if(cfg->FindKey("statusfont")) cfg->GetStringVal(type_statusfont);
		if(cfg->FindKey("statusx")) cfg->GetFloatVal(&type_statusx);
		if(cfg->FindKey("statusy")) cfg->GetFloatVal(&type_statusy);
		if(cfg->FindKey("statusw")) cfg->GetFloatVal(&type_statusw);
		if(cfg->FindKey("statush")) cfg->GetFloatVal(&type_statush);
		if(cfg->FindKey("statuslife")) cfg->GetFloatVal(&type_statuslife);
		if(cfg->FindKey("numstatuslines")) cfg->GetIntVal(&type_numstatuslines);
		//
		if(cfg->FindKey("StatusPriorityColor")){	//Propogate default.
			cfg->GetFloatVals(&type_statpricol[0][0], 3);
			for(int j = 1; j < MAX_STATUS_PRIORITY; j++) CopyVec3(type_statpricol[0], type_statpricol[j]);
		}
		if(cfg->FindKey("StatusPrioritySound")){	//Propogate default.
			cfg->GetStringVal(type_statprisound[0]);
			for(int j = 1; j < MAX_STATUS_PRIORITY; j++) type_statprisound[j] = type_statprisound[0];
		}
		for(int j = 0; j < MAX_STATUS_PRIORITY; j++){
			if(cfg->FindKey("StatusPrioritySound" + String(j))) cfg->GetStringVal(type_statprisound[j]);
			if(cfg->FindKey("StatusPriorityColor" + String(j))) cfg->GetFloatVals(&type_statpricol[j][0], 3);
		}
		//
		if(cfg->FindKey("gravity")) cfg->GetFloatVals(&type_gravity[0], 3);
		if(cfg->FindKey("friction")) cfg->GetFloatVal(&type_friction);
		//
		//EcoCrud entity info.
		if(cfg->FindKey("EcoCrud")){	//Propogate default.
			cfg->GetStringVal(EcoCrud[0]);
			for(int i = 1; i < MAXECO; i++) EcoCrud[i] = EcoCrud[0];
		}
		if(cfg->FindKey("EcoCrudForce")){	//Propogate default.
			cfg->GetFloatVal(&EcoCrudForce[0]);
			for(int i = 1; i < MAXECO; i++) EcoCrudForce[i] = EcoCrudForce[0];
		}
		for(int i = 0; i < MAXECO; i++){
			if(cfg->FindKey("EcoCrud" + String(i))) cfg->GetStringVal(EcoCrud[i]);
			if(cfg->FindKey("EcoCrudForce" + String(i))) cfg->GetFloatVal(&EcoCrudForce[i]);
		}
	}
}
bool EntityGodType::CacheResources(){
	if(ResCached) return true;
	ReadCfg(&VW->MapCfg);
	//
	for(int i = 0; i < MAXECO; i++){	//Cache possibly used crud types.
		VW->CacheResources("crud", EcoCrud[i]);
	}
/*
	for(int j = 0; i < MAX_STATUS_PRIORITY; j++){
		VW->CacheResources("sound", type_statprisound[j]);
	}
*/
    VW->CacheResources("text", type_statusfont);
	//
	return ResCached = true;
}
void EntityGodType::ListResources(FileCRCList *fl){
	if (ResListed) return;

	for(int i = 0; i < MAXECO; i++){
		VW->ListResources("crud", EcoCrud[i]);
	}
/*
	for(int j = 0; i < MAX_STATUS_PRIORITY; j++){
		VW->ListResources("sound", type_statprisound[j]);
	}
*/
    VW->ListResources("text", type_statusfont);

	ResListed = true;
}
void EntityGodType::UnlinkResources(){
	ResCached = false;
	ResListed = false;
}
EntityBase *EntityGodType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGod(this, Pos, Rot, Vel, id, flags);
}
EntityGod::EntityGod(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityBase(et, Pos, Rot, Vel, id, flags) {
	EntityGodType *TP = (EntityGodType*)et;
	CanCollide = false;
	RegisterEntity("GOD");
	CopyVec3(TP->type_gravity, gravity);
	friction = TP->type_friction;
	MaxStatusLines = TP->type_numstatuslines;

	StatusLines = new CStr[MaxStatusLines];
	StatusPriorities = new int[MaxStatusLines];
	StatusTimes = new int[MaxStatusLines];
	StatusEnt = new EntityGID[MaxStatusLines];

	for(int i = 0; i < MaxStatusLines; i++) StatusEnt[i] = 0;
	curstat = 0;
	chatent = 0;
	msglines = 0;
	//
	//announcer_id =
	a_id = 0;
	//announcer_hash = announcer_hash2 =
	a_hash = a_hash2 = 0;
	announceent = 0;
	annfifohead = annfifotail = 0;
}
EntityGod::~EntityGod()
{
	delete []StatusLines;
	delete []StatusPriorities;
	delete []StatusTimes;
	delete []StatusEnt;
}

bool EntityGod::Think()
{
	EntityGodType *TP = (EntityGodType*)TypePtr;
	//
	//Audio Announcer.
	if(announceent)
	{
		EntityBase *e = VW->GetEntity(announceent);
		if(e == 0)
		{
			if(announcer_hash2[annfifotail])
			{	//Kick off secondary chained announcement.
				e = VW->AddEntity(announcer_hash2[annfifotail], CVec3(0, 0, 0), NULL, NULL, announcer_id[annfifotail], FLAG_SOUND_PLAYANNOUNCER, 0, ADDENTITY_NONET);
				announcer_hash2[annfifotail] = 0;	//Prevent a loop.
				if(e) announceent = e->GID;
				else announceent = 0;
			}
			else
			{
				announceent = 0;
				if(annfifotail != annfifohead)
				{
					annfifotail = (annfifotail + 1) % MAX_ANNOUNCEMENTS;	//Move up tail since ann is done.
				}
			}
		}
	}
	if(!announceent && annfifohead != annfifotail)
	{	//Nothing playing, and an announcement is queued up.
		EntityBase *e = VW->AddEntity(announcer_hash[annfifotail], CVec3(0, 0, 0), NULL, NULL, announcer_id[annfifotail], FLAG_SOUND_PLAYANNOUNCER, 0, ADDENTITY_NONET);
		if(e)
		{
			announceent = e->GID;
		}
		else
		{	//Whoops, this is bad, but let's recover anyway.
			annfifotail = (annfifotail + 1) % MAX_ANNOUNCEMENTS;	//Move up tail since ann is done.
		}
	}
	//
	CopyVec3(gravity, VW->gravity);	//Here's where we'd do fancy stuff like temporary gravitic inversion.
	//
	//Chat buffer.
	if(NULL == VW->GetEntity(VW->GetFocus())){	//If no GUI ents have focus.
		char c = VW->GetChar();
		if(c == '\n' || c == '\r'){
			if(VW->GetChatMode()){	//Chat mode was on.
				unsigned int teamid = TEAMID_NONE;
				if(VW->GetChatMode() == 2){	//Team chat.
					EntityBase *e = FindRegisteredEntity("PlayerEntity");
					if(e) teamid = e->QueryInt(ATT_TEAMID);
				}
				VW->ChatMessage(chatline, teamid);
				VW->SetChatMode(0);
				chatline = "";
		//	}else{
		//		VW->SetChatMode(1);
		//		chatline = "";
			}
		}
		if(VW->GetChatMode()){
			if(isgraph(c) || c == ' '){
				if(chatline.len() < (int)(((1.0f - TP->type_statusx * 2.0f) / MAX(0.01f, TP->type_statusw)) * 2.5f)){
					char b[2] = {c, 0};
					chatline = chatline + b;
				}
			}
			if(c == '\b'){
				chatline = Left(chatline, chatline.len() - 1);
			}
			if(c == 27){	//Escape.
				VW->SetChatMode(0);
				chatline = 0;
			}
	//	}else{
		//	if(c == 't'){
		//		VW->SetChatMode(1);	//Alternate way to turn on chat mode.
		//		chatline = "";
		//	}
		}
	}
	//
	//Status text.
	int i = curstat;
	float y = TP->type_statusy;
	//
	//Display chat send line.
	EntityBase *e = VW->GetEntity(chatent);
	if(VW->GetChatMode()){
		if(!e) e = VW->AddEntity("text", TP->type_statusfont, CVec3(TP->type_statusx, y, 0),
			CVec3(TP->type_statusw, fabsf(TP->type_statush), 0), TP->type_statpricol[0], 0, 0, 0, 0);
		if(e){
			chatent = e->GID;
			CStr t;
			if(VW->GetChatMode() == 2){
				t = "Team: " + chatline + "<";
			}else{
				t = "Chat: " + chatline + "<";
			}
			int chars = (1.0f - TP->type_statusx * 2.0f) / MAX(0.001f, TP->type_statusw);
			e->SetString(ATT_TEXT_STRING, Right(t, chars));
			y += TP->type_statush;
		}
	}else{
		if(e) e->Remove();
	}
	//
	for(int j = 0; j < MaxStatusLines; j++){
		EntityBase *e;
		if(e = VW->GetEntity(StatusEnt[i])){
			e->SetPos(CVec3(TP->type_statusx, y, 0));
			e->SetRot(CVec3(TP->type_statusw, fabsf(TP->type_statush), 0));
			e->SetVel(TP->type_statpricol[CLAMP(StatusPriorities[i], 0, MAX_STATUS_PRIORITY)]);
			StatusTimes[i] += VW->FrameTime();
			if(StatusTimes[i] > (int)(TP->type_statuslife * 1000.0f)){
				e->Remove();
				StatusEnt[i] = 0;
			}
			y += TP->type_statush;
		}
		i = (i - 1) < 0 ? (MaxStatusLines - 1) : (i - 1);
	}
	//
	return true;
}
bool EntityGod::SetString(int type, const char *s){
	EntityGodType *TP = (EntityGodType*)TypePtr;
	if(type == ATT_STATUS_MESSAGE && s){
		int chars = (1.0f - TP->type_statusx * 2.0f) / MAX(0.001f, TP->type_statusw);
		CStr t = s;
		int i = 0;
		msglines = 0;
		while(i < t.len()){
			curstat = (curstat + 1) % MaxStatusLines;
			StatusLines[curstat] = Mid(t, i, chars);//s;
			StatusPriorities[curstat] = 0;
			StatusTimes[curstat] = 0;
			EntityBase *e;
			if(e = VW->GetEntity(StatusEnt[curstat])) e->Remove();	//Remove any text entities we may be walking over.
			StatusEnt[curstat] = 0;
			e = VW->AddEntity("text", TP->type_statusfont, CVec3(TP->type_statusx, TP->type_statusy, 0),
				CVec3(TP->type_statusw, TP->type_statush, 0), CVec3(1, 1, 1), 0, 0, 0, 0);
			if(e){
				StatusEnt[curstat] = e->GID;
				e->SetString(ATT_TEXT_STRING, StatusLines[curstat]);//s);
			}
			i += chars;
			msglines++;
		}
		return true;
	}
	return false;
}
bool EntityGod::SetInt(int type, int attr){
	EntityGodType *TP = (EntityGodType*)TypePtr;
	if(type == ATT_ANNOUNCER_ID){
		a_id = attr;
		return true;
	}
	if(type == ATT_ANNOUNCER_HASH2){
		a_hash2 = attr;
		return true;
	}
	if(type == ATT_CMD_ANNOUNCER){
		a_hash = attr;
		if(VW->Net.IsServerActive()){	//Xmit to clients.
			BitPacker<32> pe;
			pe.PackUInt(MSG_HEAD_ANNOUNCER, 8);
			pe.PackUInt(a_id, 32);
			pe.PackUInt(a_hash, 32);
			pe.PackUInt(a_hash2, 32);
			QueuePacket(TM_Reliable, pe, 1.0f);
		}
		EntityBase *e = VW->FindRegisteredEntity("PlayerEntity");
//OutputDebugString("\nGod heard the call.\n\n");
		//if(announceent == 0 &&
		if((a_id == 0 || (e && e->GID == a_id)) && ((annfifohead + 1) % MAX_ANNOUNCEMENTS) != annfifotail){
			annfifohead = annfifohead % MAX_ANNOUNCEMENTS;	//head is next to fill, tail is entry currently being played, when equal we are idle.
			announcer_id[annfifohead] = a_id;
			announcer_hash[annfifohead] = a_hash;
			announcer_hash2[annfifohead] = a_hash2;
			a_hash = a_hash2 = 0;
			//
			annfifohead = (annfifohead + 1) % MAX_ANNOUNCEMENTS;
			//
		//	e = VW->AddEntity(announcer_hash, CVec3(0, 0, 0), NULL, NULL, announcer_id, FLAG_SOUND_PLAYANNOUNCER, 0, ADDENTITY_NONET);
		//	if(e) announceent = e->GID;
//OutputDebugString("\nGod added entity " + String((int)announceent) + ".\n\n");
			//
			return true;
		}else return false;
	}
	if(type == ATT_CMD_REFRESHSTAT)
	{
		for(int i = 0; i < MaxStatusLines; i++)
		{
			StatusTimes[i] = 0;

			if(StatusEnt[i] == 0)
			{
				EntityBase *e = VW->AddEntity("text", TP->type_statusfont, CVec3(TP->type_statusx, TP->type_statusy, 0), CVec3(TP->type_statusw, TP->type_statush, 0), CVec3(1, 1, 1), 0, 0, 0, 0);
				if(e){
					StatusEnt[i] = e->GID;
					e->SetString(ATT_TEXT_STRING, StatusLines[i]);//s);
				}
			}
		}
	}
	if(type == ATT_CMD_HIDESTAT)
	{
		for(int i = 0; i < MaxStatusLines; i++)
		{
			StatusTimes[i] = TP->type_statuslife * 1001.0f;
		}
	}
	if(type == ATT_CMD_CHAT){
		if(VW->GetChatMode() == 0){
			VW->SetChatMode(1);
			VW->ClearChar();
			chatline = "";
		}
		return true;
	}
	if(type == ATT_CMD_TEAMCHAT){
		if(VW->GetChatMode() == 0){
			VW->SetChatMode(2);
			VW->ClearChar();
			chatline = "";
		}
		return true;
	}
	if(type == ATT_STATUS_PRIORITY){
		attr = std::min(std::max(attr, 0), MAX_STATUS_PRIORITY - 1);
		int j = curstat;
		for(int i = 0; i < CLAMP(msglines, 1, MaxStatusLines); i++){
			StatusPriorities[j] = attr;
			j = (j - 1 + MaxStatusLines) % MaxStatusLines;
		}
		VW->AddEntity("sound", TP->type_statprisound[attr], NULL, NULL, NULL, 0, 0, 0, 0);
		return true;
	}
	return false;
}
int EntityGod::QueryInt(int type){
	if(type == ATT_CHATLINELEN) return chatline.len();
	return EntityBase::QueryInt(type);
}
float EntityGod::QueryFloat(int type){
	EntityGodType *TP = (EntityGodType*)TypePtr;
	if(type >= ATT_ECO_CRUD && type < ATT_ECO_CRUD + MAXECO){
		return TP->EcoCrudForce[type - ATT_ECO_CRUD];
	}
	if(type == ATT_GRAVITY_X) return gravity[0];
	if(type == ATT_GRAVITY_Y) return gravity[1];
	if(type == ATT_GRAVITY_Z) return gravity[2];
	if(type == ATT_FRICTION) return friction;
	//
	return EntityBase::QueryFloat(type);	//Get into this habit, as this will be needed for Derived Gods!
}
CStr EntityGod::QueryString(int type){
	EntityGodType *TP = (EntityGodType*)TypePtr;
	if(type >= ATT_ECO_CRUD && type < ATT_ECO_CRUD + MAXECO){
		return TP->EcoCrud[type - ATT_ECO_CRUD];
	}
	return EntityBase::QueryString(type);
}
void EntityGod::DeliverPacket(const unsigned char *data, int len){
	if(data && len > 1 && data[0] == MSG_HEAD_ANNOUNCER){
		BitUnpackEngine pe(&data[1], len - 1);
		pe.UnpackUInt(a_id, 32);
		pe.UnpackUInt(a_hash, 32);
		pe.UnpackUInt(a_hash2, 32);
		SetInt(ATT_ANNOUNCER_ID, a_id);
		SetInt(ATT_ANNOUNCER_HASH2, a_hash2);
		SetInt(ATT_CMD_ANNOUNCER, a_hash);
		return;
	}
	EntityBase::DeliverPacket(data, len);
}


//******************************************************************
//**  Sky Plane Entity  --  Type Handler
//******************************************************************
EntitySkyplaneType::EntitySkyplaneType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t) {
	texture = NULL;
	detailtexture = NULL;
	watertexture = NULL;
	wateralpha = 0.75f;
	waterscale = 0.25f;
	waterenv = 0.5f;	//Percentage of water surface opacity to be environment mapped.
	//fogr = fogg = fogb = 0.6f;
	SetVec3(0.6f, 0.6f, 0.6f, fogcolor);
	SetVec3(-1, 1, 0, lightvec);
	ambterrain = ambentity = 0.0f;
	scorcheco = -1;
	detailrotspeed = 0.0f;
	detailrotradius = 0.0f;
	skyrotatespeed = 0.0f;
	ReadCfg(cfg);
}
void EntitySkyplaneType::ReadCfg(ConfigFile *cfg){
//	ConfigFile *cfg = ocfg;
//	for(int i = 0; i < 2; i++){
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("texture")) cfg->GetStringVal(texturefile);
		if(cfg->FindKey("detailtexture")) cfg->GetStringVal(detailtexturefile);
		if(cfg->FindKey("watertexture")) cfg->GetStringVal(watertexturefile);
		if(cfg->FindKey("wateralpha")) cfg->GetFloatVal(&wateralpha);
		if(cfg->FindKey("waterscale")) cfg->GetFloatVal(&waterscale);
		if(cfg->FindKey("waterenv")) cfg->GetFloatVal(&waterenv);
		if(cfg->FindKey("FogRed")) cfg->GetFloatVal(&fogcolor[0]);
		if(cfg->FindKey("FogGreen")) cfg->GetFloatVal(&fogcolor[1]);
		if(cfg->FindKey("FogBlue")) cfg->GetFloatVal(&fogcolor[2]);
		if(cfg->FindKey("FogColor")) cfg->GetFloatVals(fogcolor, 3);
		if(cfg->FindKey("LightVector")) cfg->GetFloatVals(lightvec, 3);
		if(cfg->FindKey("AmbientTerrain")) cfg->GetFloatVal(&ambterrain);
		if(cfg->FindKey("AmbientEntity")) cfg->GetFloatVal(&ambentity);
		if(cfg->FindKey("scorcheco")) cfg->GetIntVal(&scorcheco);
		if(cfg->FindKey("detailrotspeed")) cfg->GetFloatVal(&detailrotspeed);
		if(cfg->FindKey("detailrotradius")) cfg->GetFloatVal(&detailrotradius);
		if(cfg->FindKey("skyrotatespeed")) cfg->GetFloatVal(&skyrotatespeed);
	}
//		cfg = &VW->MapCfg;	//Read map-stored config file second, to override values.
//	}
}
EntityBase *EntitySkyplaneType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntitySkyplane(this, Pos, Rot, Vel, id, flags);
}
bool EntitySkyplaneType::CacheResources(){
	if(ResCached) return true;
	ResCached = true;	//Prevents circular caching.  Be sure to set false if failure.
	//
	ReadCfg(&VW->MapCfg);	//Override internal cfgfile defaults with map settings.
	//
	texture = VW->GetImage(texturefile, false);
	detailtexture = VW->GetImage(detailtexturefile, true, true, ALPHAGRAD_NORMAL);	//MipMap, Alpha, Gradiate.
	watertexture = VW->GetImage(watertexturefile, true);
	if(detailtexture && detailtexture->Data()){
		detailtexturemt = VW->GetCreateImage(detailtexturefile + "_mt", detailtexture->Width(), detailtexture->Height(), 8, true, false, ALPHAGRAD_NONE);
		if(detailtexturemt && detailtexturemt->Data()){
			Image *d1 = detailtexture, *d2 = detailtexturemt;
			d2->Suck(d1);
			for(int i = 0; i < 256; i++){
				int v = 255 - (((d1->pe[i].peRed + d1->pe[i].peGreen + d1->pe[i].peBlue) / 3) / 2);
				d2->pe[i].peRed = v - TMrandom() % 4;
				d2->pe[i].peGreen = v - TMrandom() % 4;
				d2->pe[i].peBlue = v - TMrandom() % 4;
			}
		}
	}
	//Create 4-way mirror of sky texture for env map.
	if(texture && texture->Data()){
		Image tex;
		if(texture->BPP() == 8){
			tex = *texture;
		}else{
			texture->Quantize32to8(&tex);
		}
		envtexture = VW->GetCreateImage(texturefile + "_env", texture->Width() * 2, texture->Height() * 2, 32, true, false, ALPHAGRAD_NONE);
		if(envtexture && envtexture->Data() && tex.Data()){
			int x,y;
			envtexture->SetPalette(tex.pe);
			envtexture->Clear(0);
			float xscale = 2.0f / (float)envtexture->Width();
			float yscale = 2.0f / (float)envtexture->Height();
			float sxscale = (float)tex.Width() / PI;
			float syscale = (float)tex.Height();
			unsigned char *dst;
			unsigned char *src = tex.Data();
			int srcw = tex.Width(), srcp = tex.Pitch(), dstp = envtexture->Pitch();
			PaletteEntry *ppe = tex.pe;
			for(y = 0; y < envtexture->Height(); y++){
				dst = envtexture->Data() + y * envtexture->Pitch();
				float fy = (float)y * yscale - 1.0f;
				for(x = 0; x < envtexture->Width(); x++){
				//	int sx = x < texture->Width() ? x : (envtexture->Width() - 1) - x;
				//	int sy = y < texture->Height() ? y : (envtexture->Height() - 1) - y;
					float fx = (float)x * xscale - 1.0f;
					float fsx = fabsf(atan2f(fx, fy) * sxscale);
					float fsy = (float)(tex.Height() - 2) - fabsf(((fx * fx + fy * fy) * 2.0f - 1.0f) * syscale);
					int sx = fsx, sy = fsy;
				//	*dst = *(texture->Data() + min(sx, texture->Width() - 1) + max(0, sy) * texture->Pitch());
					//
					//Do bilerping.
					PaletteEntry *pe1, *pe2, *pe3, *pe4;
					pe1 = &ppe[*(src + std::min(sx, srcw - 1) + std::max(0, sy) * srcp)];
					pe2 = &ppe[*(src + std::min(sx + 1, srcw - 1) + std::max(0, sy) * srcp)];
					pe3 = &ppe[*(src + std::min(sx, srcw - 1) + std::max(0, sy + 1) * srcp)];
					pe4 = &ppe[*(src + std::min(sx + 1, srcw - 1) + std::max(0, sy + 1) * srcp)];
					unsigned int q1 = ((fsx - (float)sx) * 255.0f);
					unsigned int q2 = ((fsy - (float)sy) * 255.0f);
				//	unsigned int q1 = 128;
				//	unsigned int q2 = 128;
					unsigned int iq1 = 255 - q1, iq2 = 255 - q2;
					dst[0] = ((pe1->peBlue * iq1 + pe2->peBlue * q1) * iq2 + (pe3->peBlue * iq1 + pe4->peBlue * q1) * q2) >>16;
					dst[1] = ((pe1->peGreen * iq1 + pe2->peGreen * q1) * iq2 + (pe3->peGreen * iq1 + pe4->peGreen * q1) * q2) >>16;
					dst[2] = ((pe1->peRed * iq1 + pe2->peRed * q1) * iq2 + (pe3->peRed * iq1 + pe4->peRed * q1) * q2) >>16;
					dst[3] = 255;
				//	if(x > 0 && y > 0){	//Cheesy filter.
				//		for(int i = 0; i < 3; i++){
				//			dst[i] = (dst[i]  + dst[i - 4] + dst[i - dstp] + dst[(i - 4) - dstp]) >>2;
				//		}
				//	}
					dst += 4;
				}
			}
			//Optional cheesy box filter pass.
			for(y = 0; y < envtexture->Height() - 1; y++){
				dst = envtexture->Data() + y * envtexture->Pitch();
			//	float fy = (float)y * yscale - 1.0f;
				for(x = 0; x < envtexture->Width() - 1; x++){
					for(int i = 0; i < 3; i++)
						dst[i] = (dst[i] * 5 + dst[i + 4] + dst[i + dstp] + dst[i + 4 + dstp]) >>3;
					dst += 4;
				}
			}
	//		envtexture->SaveBMP("d:\\temptank.bmp");	//Testing.
		}
	}
	if(texture) return ResCached = true;
	return ResCached = false;
}
void EntitySkyplaneType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	ResListed = true;

	fl->FileCRC(texturefile);
	fl->FileCRC(detailtexturefile);
	fl->FileCRC(watertexturefile);
//	fl->FileCRC(texturefile+"_env");	// FIXME: GetCreateImage()?
}
void EntitySkyplaneType::UnlinkResources(){
	ResCached = false;
	ResListed = false;
	texture = detailtexture = watertexture = NULL;	//ResourceManager owns resources, no need to free.
}
//******************************************************************
//**  Sky Plane Entity  --  Main Code
//******************************************************************
EntitySkyplane::EntitySkyplane(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityBase(et, Pos, Rot, Vel, id, flags) {
	EntitySkyplaneType *TP = (EntitySkyplaneType*)et;
	Active = true;
	//
	RegisterEntity("SkyEntity");
	//
	//Set lighting vectors.
//	float lx = -1, ly = 1, lz = 0, amb = 0.5;
//	float lx = -1, ly = 1, lz = 0, amb = 0.33;
//	float lx = 1, ly = 0, lz = 0, amb = 0.25;
//	VW->Map.SetLightVector(lx, ly, lz, amb);
//	VW->PolyRend->SetLightVector(lx, ly, lz, amb);
	VW->Map.SetLightVector(TP->lightvec[0], TP->lightvec[1], TP->lightvec[2], TP->ambterrain);
	VW->PolyRend->SetLightVector(TP->lightvec[0], TP->lightvec[1], TP->lightvec[2], TP->ambentity);
	//
}
bool EntitySkyplane::SetInt(int type, int val){//{ Active = val; return 1; };
	if(type == ATT_SKYPLANE_ACTIVE){ Active = (val != 0); return true; }////{ return Active; };
	return false;
}
int EntitySkyplane::QueryInt(int type){
	if(type == ATT_SKYPLANE_ACTIVE) return int(Active);//{ return Active; };
	return 0;
}
float EntitySkyplane::QueryFloat(int type){
	EntitySkyplaneType *TP = (EntitySkyplaneType*)TypePtr;
	return 0.0f;
}
CStr EntitySkyplane::QueryString(int type){
	EntitySkyplaneType *TP = (EntitySkyplaneType*)TypePtr;
	return "";
}
bool EntitySkyplane::Think(){
	EntitySkyplaneType *TP = (EntitySkyplaneType*)TypePtr;
	if(Active){
	//	if(TP->texture)
		VW->VoxelRend->SetSkyTexture(TP->texture, TP->envtexture);
		if(TP->envtexture) VW->PolyRend->SetEnvMap1(TP->envtexture);
		if(TP->detailtexture) VW->VoxelRend->SetDetailTexture(TP->detailtexture, TP->detailtexturemt);
		if(TP->watertexture) VW->VoxelRend->SetWaterTexture(TP->watertexture, TP->wateralpha, TP->waterscale, TP->waterenv);
		VW->VoxelRend->SetFogColor(TP->fogcolor[0], TP->fogcolor[1], TP->fogcolor[2]);
		VW->Map.SetScorchEco(TP->scorcheco);
		VW->PolyRend->SetLightVector(TP->lightvec[0], TP->lightvec[1], TP->lightvec[2], TP->ambentity);
		VW->VoxelRend->SetDetailRot(TP->detailrotspeed, TP->detailrotradius);
		VW->VoxelRend->SetSkyRotate(TP->skyrotatespeed);
	}
	return true;
}
//******************************************************************
//**  Sky Box Entity  --  Type Handler
//******************************************************************
const char SkyboxTackon[6][8] = {"__front", "__right", "__back", "__left", "__up", "__down"};
EntitySkyboxType::EntitySkyboxType(ConfigFile *cfg, const char *c, const char *t) : EntitySkyplaneType(cfg, c, t) {
	for(int i = 0; i < SKYBOX_IMAGES; i++) type_skybox[i] = NULL;
	type_halfskybox = 1;
	SetVec3(0.5f, 0.5f, 0.5f, type_colorgain);
	SetVec3(0.5f, 0.5f, 0.5f, type_colorbias);
	SetVec3(1.0f, 1.0f, 1.0f, type_colorscale);
	SetVec3(0.0f, 0.0f, 0.0f, type_colorshift);
	type_filtersky = 1;
	type_autofog = 1;
	ReadCfg(cfg);
}

void EntitySkyboxType::ReadCfg(ConfigFile *cfg){
	if(cfg){
		if(cfg->FindKey("skyboximages")) cfg->GetStringVal(type_skyboxname);
		if(cfg->FindKey("halfskybox")) cfg->GetIntVal(&type_halfskybox);
		if(cfg->FindKey("filtersky")) cfg->GetIntVal(&type_filtersky);
		if(cfg->FindKey("autofog")) cfg->GetIntVal(&type_autofog);
		if(cfg->FindKey("ColorGain")) cfg->GetFloatVals(&type_colorgain[0], 3);
		if(cfg->FindKey("ColorBias")) cfg->GetFloatVals(&type_colorbias[0], 3);
		if(cfg->FindKey("ColorScale")) cfg->GetFloatVals(&type_colorscale[0], 3);
		if(cfg->FindKey("ColorShift")) cfg->GetFloatVals(&type_colorshift[0], 3);
	}
}
EntityBase *EntitySkyboxType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntitySkybox(this, Pos, Rot, Vel, id, flags);
}
bool EntitySkyboxType::CacheResources(){
	if(ResCached) return true;
	EntitySkyplaneType::CacheResources();
	ResCached = true;	//Prevents circular caching.  Be sure to set false if failure.
	//
	ReadCfg(&VW->MapCfg);	//Override internal cfgfile defaults with map settings.
	//
	for(int i = 0; i < SKYBOX_IMAGES; i++){
		type_skybox[i] = VW->GetImage(type_skyboxname + SkyboxTackon[i] + ".bmp", true, false, ALPHAGRAD_NONE);
		if(type_skybox[i]){
			type_skybox[i]->To32Bit();
			type_skybox[i]->Flushable = false;
			if(i < 4 && type_halfskybox && type_skybox[i]->Width() == type_skybox[i]->Height()){
				Bitmap tbmp;
				tbmp.Init(type_skybox[i]->Width(), type_skybox[i]->Height() / 2, type_skybox[i]->BPP());
				tbmp.Suck(type_skybox[i]);	//Suck out top half.
				*(Bitmap*)type_skybox[i] = tbmp;
			}
			//Color correct images.
			type_skybox[i]->ColorCorrect(type_colorgain[0], type_colorgain[1], type_colorgain[2], 0.5f,
										type_colorbias[0], type_colorbias[1], type_colorbias[2], 0.5f,
										type_colorscale[0], type_colorscale[1], type_colorscale[2], 1.0f,
										type_colorshift[0], type_colorshift[1], type_colorshift[2], 0.0f);
			//Filter skybox images.
			if(type_filtersky){
				Bitmap tbmp;
				tbmp.Init(type_skybox[i]->Width(), type_skybox[i]->Height(), type_skybox[i]->BPP());
				tbmp.Suck(type_skybox[i]);	//So edges will be OK.
				if(tbmp.Data()){
					for(int y = 0; y < tbmp.Height() - 1; y++){
						unsigned char *dst = type_skybox[i]->Data() + y * type_skybox[i]->Pitch();
						unsigned char *src = tbmp.Data() + y * tbmp.Pitch();	//Suck back into real image.
						for(int x = 0; x < tbmp.Width() - 1; x++){
							for(int p = 0; p < 4; p++){
								*dst = ((int)src[0] + (int)src[4] + (int)src[0 + tbmp.Pitch()] + (int)src[4 + tbmp.Pitch()]) >>2;
								src++;
								dst++;
							}
						}
					}
				}
			}
		}
	}
	if(!type_skybox[0]) return ResCached = false;
	//
	envtexture = VW->GetCreateImage(type_skyboxname + "_env", 512, 512, 32, true, false, ALPHAGRAD_NONE);
	if(envtexture && envtexture->Data()){
		int x,y;
		envtexture->Clear(0);
		float xscale = 2.0f / (float)envtexture->Width();
		float yscale = 2.0f / (float)envtexture->Height();
	//	float sxscale = (float)tex.Width() / PI;
	//	float syscale = (float)tex.Height();
		unsigned char *dst;
//		unsigned char *src = tex.Data();
		int dstp = envtexture->Pitch();
	//	PALETTEENTRY *ppe = tex.pe;
		for(y = 0; y < envtexture->Height(); y++){
		//	dst = envtexture->Data() + y * envtexture->Pitch();
			float fy = (float)y * yscale - 1.0f;

			for(x = 0; x < envtexture->Width(); x++){
				float fx = (float)x * xscale - 1.0f;
				//
				float refy = (fx * fx + fy * fy) * 2.0f - 1.0f;
				float s = sqrtf(fx * fx + fy * fy);
				s = sqrtf(1.0f - refy * refy) / std::max(0.0001f, s);
				float refx = fx * s, refz = -fy * s;
				//Ok, this should be the reflection vector now.
				//
				float sx, sy;
				int face = 0;	//Determine which bitmap to suck from.
				if(fabsf(refx) >= fabsf(refz) && fabsf(refx) >= fabsf(refy)){
					float s = 1.0f / fabsf(refx);
				//	refz *= s; refy *= s;
					sx = -refz * s;
					sy = -fabsf(refy) * s;
					if(refx > 0.0f) face = SKYBOX_RIGHT;
					else{
						face = SKYBOX_LEFT;
						sx = -sx;
					}
				}else if(fabsf(refz) >= fabsf(refx) && fabsf(refz) >= fabsf(refy)){
					float s = 1.0f / fabsf(refz);
				//	refx *= s; refy *= s;
					sx = refx * s;
					sy = -fabsf(refy) * s;
					if(refz > 0.0f) face = SKYBOX_FRONT;
					else{
						face = SKYBOX_BACK;
						sx = -sx;
					}
				}else{
					float s = 1.0f / fabsf(refy);
				//	refx *= s; refz *= s;
					sx = refx * s;
					sy = refz * s;
					if(refy > 0.0f) face = SKYBOX_UP;
					else{
					//	face = SKYBOX_DOWN;
					//	sy = -sy;
						face = SKYBOX_UP;	//For vertical mirroring...
					}
				}
				//
				if(face < SKYBOX_IMAGES && type_skybox[face]){
					Bitmap &bmp = *type_skybox[face];
					if(bmp.Data()){
						int ix = FastFtoL((sx + 1.0f) * (float)((bmp.Width() >>1)));
						int iy = FastFtoL((sy + 1.0f) * (float)((bmp.Width() >>1)));	//Intentional using width twice!  Not bug.
					//	ix = ix & (bmp.Width() - 1);	//This ASSUMES half-skyboxes with underside mirroring!!!!!
					//	iy = iy & (bmp.Height() - 1);
						ix = std::max(ix, 0); ix = std::min(ix, bmp.Width() - 1);
						iy = std::max(iy, 0); iy = std::min(iy, bmp.Height() - 1);
						*((long*)(envtexture->Data() + (x <<2) + y * envtexture->Pitch())) =
							*((long*)(bmp.Data() + (ix <<2) + iy * bmp.Pitch()));
					}
				}
			}
		}
		//Optional cheesy box filter pass.  Gets rid of high frequency artifacts.
		for(y = 0; y < envtexture->Height() - 1; y++){
			dst = envtexture->Data() + y * envtexture->Pitch();
		//	float fy = (float)y * yscale - 1.0f;
			for(x = 0; x < envtexture->Width() - 1; x++){
				for(int i = 0; i < 3; i++)
					dst[i] = (dst[i] * 1 + dst[i + 4] + dst[i + dstp] + dst[i + 4 + dstp]) >>2;
				dst += 4;
			}
		}
		//
		//Try analytical fog coloring, read from sky textures.
		//
		if(type_autofog){
			int steps = 32;
			float caccum[3] = {0.0f, 0.0f, 0.0f};
			for(int ai = 0; ai < steps; ai++){
				float a = (float)ai * (PI2 / steps);
				float fx = ((sin(a) * 0.707) * 0.5f + 0.5f) * (float)envtexture->Width();
				float fy = ((cos(a) * 0.707) * 0.5f + 0.5f) * (float)envtexture->Height();
				unsigned char *pnt = envtexture->Data() + (int)fx * 4 + (int)fy * envtexture->Pitch();
				for(int p = 0; p < 3; p++){
					caccum[2 - p] += (float)pnt[p] / 255.0f;
				}
			}
			for(int p = 0; p < 3; p++){
				fogcolor[p] = caccum[p] / (float)steps;
			}
		}
		//
	//	envtexture->SaveBMP("d:\\temptank.bmp");	//Testing.
	}
	return ResCached = true;
}
void EntitySkyboxType::ListResources(FileCRCList *fl){
	if (ResListed) return;

	EntitySkyplaneType::ListResources(fl);

	ResListed = true;

	//
	for(int i = 0; i < SKYBOX_IMAGES; i++){
		fl->FileCRC(type_skyboxname + SkyboxTackon[i] + ".bmp");
	}
	//
//	envtexture = VW->GetCreateImage(type_skyboxname + "_env", 512, 512, 32, true, false, false);
	// FIXME??
}
void EntitySkyboxType::UnlinkResources(){
	for(int i = 0; i < SKYBOX_IMAGES; i++) type_skybox[i] = NULL;
	EntitySkyplaneType::UnlinkResources();
}
EntitySkybox::EntitySkybox(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntitySkyplane(et, Pos, Rot, Vel, id, flags) {
	EntitySkyboxType *TP = (EntitySkyboxType*)et;
}
bool EntitySkybox::Think(){
	EntitySkyboxType *TP = (EntitySkyboxType*)TypePtr;
	EntitySkyplane::Think();
	if(Active){
		for(int i = 0; i < SKYBOX_IMAGES; i++){
			VW->VoxelRend->SetSkyBoxTexture(i, TP->type_skybox[i]);
		}
	}
	return true;
}

//******************************************************************
//**  Sprite Entity  --  Type Handler
//******************************************************************
EntitySpriteType::EntitySpriteType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t) {
	char buf[256];
	texture = NULL;
	type_w = type_h = 1.0f; type_base = 0.0f;
	type_fade = 1.0f;
	type_fadebias = 0.5f;
	type_rendflags = SPRITE_BLEND_ALPHA;//COPY;
	type_alphagrad = ALPHAGRAD_NONE;
	type_transparent = true;
	type_sizebias = 0;
	type_alphagradbias = 0.5f;
	type_restonground = 0;
	type_groundoffset = 0.0f;
	type_forcemaxtexres = 0;
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("texture") && cfg->GetStringVal(buf, 256)) texturefile = buf;
		if(cfg->FindKey("width")) cfg->GetFloatVal(&type_w);
		if(cfg->FindKey("height")) cfg->GetFloatVal(&type_h);
	//	base = h / 2.0f;	//Assume tree-style.
		if(cfg->FindKey("base")) cfg->GetFloatVal(&type_base);
		if(cfg->FindKey("blend")) cfg->GetEnumVal(&type_rendflags, EnumBlend, nEnumBlend);
		if(cfg->FindKey("fade")) cfg->GetFloatVal(&type_fade);
		if(cfg->FindKey("fadebias")) cfg->GetFloatVal(&type_fadebias);
		if(cfg->FindKey("alphagrad")) cfg->GetIntVal((int*)&type_alphagrad);
		if(cfg->FindKey("transparent")) cfg->GetBoolVal(&type_transparent);
		if(cfg->FindKey("sizebias")) cfg->GetIntVal(&type_sizebias);
		if(cfg->FindKey("alphagradbias")) cfg->GetFloatVal(&type_alphagradbias);
		if(cfg->FindKey("restonground")) cfg->GetIntVal(&type_restonground);
		if(cfg->FindKey("groundoffset")) cfg->GetFloatVal(&type_groundoffset);
		if(cfg->FindKey("ForceMaxTexRes")) cfg->GetIntVal(&type_forcemaxtexres);
	}
}
EntityBase *EntitySpriteType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntitySprite(this, Pos, Rot, Vel, id, flags);
}
bool EntitySpriteType::CacheResources(){
	if(ResCached) return true;
	if(texturefile.len() <= 0) return ResCached = true;	//If no texture is specified, return "successful" res cache.
	ResCached = true;	//Prevents circular caching.  Be sure to set false if failure.
	texture = VW->GetImage(texturefile, true, (type_rendflags & SPRITE_BLEND_ADD) ? false : type_transparent, type_alphagrad);	//MipMap and Transparent.
	if(texture){
	//	if((texture->flags & BFLAG_ROTATED) == 0) texture->RotateLeft90();
	//	texture->AnalyzeLines();	//Temporary, make non-vertical sprite drawer some time...
		texture->SizeBias = MAX(abs(type_sizebias), abs(texture->SizeBias));	//Set size bias (to largest ever set).
		if(type_alphagradbias != 0.5f) texture->AlphaGradBias = type_alphagradbias;
		texture->ForceMaxTexRes = MAX(type_forcemaxtexres, texture->ForceMaxTexRes);	//Set to largest ever set.
		return ResCached = true;
	}
	return ResCached = false;
}
void EntitySpriteType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	ResListed = true;

	if(texturefile.len() <= 0) return;
	fl->FileCRC(texturefile);
}
void EntitySpriteType::UnlinkResources(){
	ResCached = false;
	ResListed = false;
	texture = NULL;	//ResourceManager owns resources, no need to free.
}
CStr EntitySpriteType::InterrogateString(const char *thing){	//Responds to "TextureName"
	if(thing && CmpLower(thing, "texturename")) return texturefile;
	return EntityTypeBase::InterrogateString(thing);
}
//******************************************************************
//**  Sprite Entity  --  Main Code
//******************************************************************
EntitySprite::EntitySprite(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityBase(et, Pos, Rot, Vel, id, flags) {
	EntitySpriteType *ET = (EntitySpriteType*)et;
	Active = true;
	Position[1] += ET->type_base;
	CanCollide = false;
	w = ET->type_w;
	h = ET->type_h;
	frame = 0;
	fade = ET->type_fade;
	spriteobj.IntRot = 0;
	TerrainModified();
}
bool EntitySprite::SetInt(int type, int val){//{ Active = val; return 1; };
	if(type == ATT_SPRITE_ACTIVE){ Active = (val != 0); return true; }
	if(type == ATT_SPRITE_FRAME){ frame = val; return true; }
	return false;
}
int EntitySprite::QueryInt(int type){
	EntitySpriteType *TP =(EntitySpriteType*)TypePtr;
	if(type == ATT_SPRITE_ACTIVE) return int(Active);//{ return Active; };
	if(type == ATT_SPRITE_FRAME) return frame;//{ return Active; };
	if(type == ATT_RENDFLAGS) return TP->type_rendflags;
	return EntityBase::QueryInt(type);
}
bool EntitySprite::Think(){
	EntitySpriteType *ET =(EntitySpriteType*)TypePtr;
	if(Active && ET->texture){
	//	VW->PolyRend->AddSprite(&(*(ET->texture))[frame], w, h, Position, false, ET->rendflags);
		spriteobj.Configure(&(*(ET->texture))[frame], Position, w, h, ET->type_rendflags, fade);
		VW->PolyRend->AddTransObject(&spriteobj);
	}
	return true;
}
int EntitySprite::TerrainModified(){
	EntitySpriteType *TP = (EntitySpriteType*)TypePtr;
	if(TP->type_restonground) Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + TP->type_groundoffset;
	return 1;
}
bool EntitySprite::SetFloat(int type, float val){
	if(type == ATT_FADE){  fade = val;  return true;  }
	return EntityBase::SetFloat(type, val);
}
float EntitySprite::QueryFloat(int type){
	EntitySpriteType *TP =(EntitySpriteType*)TypePtr;
	if(type == ATT_FADE) return fade;
	if(type == ATT_TYPE_FADE) return TP->type_fade;
	return EntityBase::QueryFloat(type);
}
CStr EntitySprite::QueryString(int type){
	EntitySpriteType *TP = (EntitySpriteType*)TypePtr;
	if(type == ATT_TEXTURE_NAME) return TP->texturefile;
	return EntityBase::QueryString(type);
}
bool EntitySprite::QueryVec(int type, void *out){	//ATT_TEXTURE_PTR
	EntitySpriteType *TP = (EntitySpriteType*)TypePtr;
	if(type == ATT_TEXTURE_PTR && out){
		*((ImageNode**)out) = TP->texture;
		return true;
	}
	return EntityBase::QueryVec(type, out);
}

//******************************************************************
//**           Generic Polygon Mesh Entity
//******************************************************************
EntityMeshType::EntityMeshType(ConfigFile *cfg, const char *c, const char *t)
	: EntitySpriteType(cfg, c, t), MixinEntityChain(cfg, c) {//EntityTypeBase(c, t) {
	mesh = NULL;
//	texture = NULL;
	type_collide = false;
	type_scale = 1.0f;
//	type_fade = 1.0f;
	type_mass = 1.0f;
	type_lodbias = 0.1f;
	type_center = false;
//	type_rendflags = MESH_BLEND_ALPHA;
	type_transparent = false;
	ClearVec3(type_meshoffset);
	SetVec3(1.0f, 1.0f, 1.0f, type_scalematrix);
	type_scalematrixon = 0;
	ClearVec3(type_constrotate);
	ClearVec3(type_constwave);
	type_constwavetime = 0.0f;
	type_directmatrix = 0;
	type_group = GROUP_PROP;
	type_secondaryfrustum = 0;
	constmovement = 0;
	int group = 1;
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("ConstRotate")){
			cfg->GetFloatVals(&type_constrotate[0], 3);
			for(int i = 0; i < 3; i++) type_constrotate[i] *= DEG2RAD;
			constmovement = 1;
		}
		if(cfg->FindKey("ConstWave")){
			cfg->GetFloatVals(&type_constwave[0], 3);
			constmovement = 1;
		}
		if(cfg->FindKey("constwavetime")) cfg->GetFloatVal(&type_constwavetime);
//		if(cfg->FindKey("texture")) cfg->GetStringVal(texturefile);
		if(cfg->FindKey("meshoffset")) cfg->GetFloatVals(&type_meshoffset[0], 3);
		if(cfg->FindKey("mesh")) cfg->GetStringVal(meshfile);
		if(cfg->FindKey("mesh1")) cfg->GetStringVal(meshfile1);
		if(cfg->FindKey("mesh2")) cfg->GetStringVal(meshfile2);
		if(cfg->FindKey("lodbias")) cfg->GetFloatVal(&type_lodbias);
		if(cfg->FindKey("collide")) cfg->GetBoolVal(&type_collide);
		if(cfg->FindKey("Transitory")) cfg->GetBoolVal(&Transitory);
		if(cfg->FindKey("center")) cfg->GetBoolVal(&type_center);
		if(cfg->FindKey("scale")) cfg->GetFloatVal(&type_scale);
//		if(cfg->FindKey("fade")) cfg->GetFloatVal(&type_fade);
		if(cfg->FindKey("mass")) cfg->GetFloatVal(&type_mass);
//		if(cfg->FindKey("blend")) cfg->GetEnumVal(&type_rendflags, EnumBlend, nEnumBlend);
		ReadMeshRendFlags(cfg, type_rendflags);
		if(cfg->FindKey("transparent")) cfg->GetBoolVal(&type_transparent);
		if(cfg->FindKey("ScaleMatrix")) cfg->GetFloatVals(&type_scalematrix[0], 3);
		if(cfg->FindKey("directmatrix")) cfg->GetIntVal(&type_directmatrix);
		if(cfg->FindKey("group")) cfg->GetIntVal(&group);
		if(cfg->FindKey("SecondaryFrustum ")) cfg->GetIntVal(&type_secondaryfrustum);
	}
	ScaleVec3(type_meshoffset, type_scale);
	switch(group){	//Allow specification of entity group in entity config file.
	case 0 : type_group = GROUP_ACTOR; break;
	case 1 : type_group = GROUP_PROP; break;
	case 2 : type_group = GROUP_ETHEREAL; break;
	}
	if(type_scalematrix[0] != 1.0f || type_scalematrix[1] != 1.0f || type_scalematrix[2] != 1.0f){
		type_scalematrixon = 1;
		type_rendflags |= MESH_SCALE_MATRIX;
	}
}
void ReadMeshRendFlags(ConfigFile *cfg, int &type_rendflags){
	int smooth = 0, solid = 0, cull = 1, envlight = 0, edge = 0, envbasismodel = 0;
	if(cfg){
		if(cfg->FindKey("solidlight") && cfg->GetIntVal(&solid)){
			if(solid) type_rendflags |= MESH_SHADE_SOLID;
		}
		if(cfg->FindKey("smoothshade") && cfg->GetIntVal(&smooth)){
			if(smooth) type_rendflags |= (solid ? 0 : MESH_SHADE_SMOOTH) | MESH_ENVMAP_SMOOTH;
		}
		if(cfg->FindKey("edgetrans") && cfg->GetIntVal(&edge)){
			if(edge) type_rendflags |= MESH_SHADE_EDGETRANS;
		}
		if(cfg->FindKey("cullface") && cfg->GetIntVal(&cull)){
			if(!cull) type_rendflags |= MESH_DISABLE_CULL;
		}
		if(cfg->FindKey("envlight") && cfg->GetIntVal(&envlight)){
			if(envlight) type_rendflags |= MESH_ENVBASIS_LIGHT;
		}
		if(cfg->FindKey("envbasismodel") && cfg->GetIntVal(&envbasismodel)){
			if(envbasismodel) type_rendflags |= MESH_ENVBASIS_MODEL;
		}
		if(cfg->FindKey("envmode")){
			CStr envmode;
			cfg->GetStringVal(envmode);
			if(CmpLower(envmode, "texture")) type_rendflags |= MESH_ENVMAPT_SPHERE;// | MESH_ENVMAP_SMOOTH;
			if(CmpLower(envmode, "global")) type_rendflags |= MESH_ENVMAP1_SPHERE;// | MESH_ENVMAP_SMOOTH;
			if(CmpLower(envmode, "blend")) type_rendflags |= MESH_BLEND_ENVMAP | MESH_ENVMAP1_SPHERE;// | MESH_ENVMAP_SMOOTH;
		}
	}
}
EntityBase *EntityMeshType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityMesh(this, Pos, Rot, Vel, id, flags);
}
bool EntityMeshType::CacheResources(){
	if(ResCached) return true;
	if(EntitySpriteType::CacheResources()){
		MixinEntityChain::CacheResources(VW);
	//	ResCached = true;	//Prevents circular caching.  Be sure to set false if failure.
	//	texture = VW->GetImage(texturefile, true);
		if(meshfile.len() <= 0) return ResCached;	//If texture/sprite caches OK and no mesh specified, return successful rescache.
		Mesh *mesh1, *mesh2;
		bool smooth = (type_rendflags & (MESH_SHADE_SMOOTH | MESH_ENVMAP_SMOOTH)) != 0;
		mesh = VW->GetMesh(meshfile, type_scale, smooth);
		mesh1 = VW->GetMesh(meshfile1, type_scale, smooth);
		mesh2 = VW->GetMesh(meshfile2, type_scale, smooth);
		if(texture && mesh){
			mesh->SetTexture(texture);
			if(type_center) mesh->Center();
			mesh->SetLod(0, type_lodbias, mesh1);
			if(mesh1){
				mesh1->SetTexture(texture);
				mesh1->SetShift(mesh->Offset);
				mesh1->SetLod(1, type_lodbias, mesh2);
				if(mesh2){
					mesh2->SetTexture(texture);
					mesh2->SetShift(mesh->Offset);
				}
			}
			return ResCached = true;
		}
	}
	return ResCached = false;
}
void EntityMeshType::ListResources(FileCRCList *fl){
	if (ResListed) return;

	EntitySpriteType::ListResources(fl);

	ResListed = true;

	fl->FileCRC(meshfile);
	fl->FileCRC(meshfile1);
	fl->FileCRC(meshfile2);
}
void EntityMeshType::UnlinkResources(){
	EntitySpriteType::UnlinkResources();
	ResCached = false;
	ResListed = false;
//	texture = NULL;
	mesh = NULL;//ResourceManager owns resources, no need to free.
}
//Main code.
EntityMesh::EntityMesh(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntitySprite(et, Pos, Rot, Vel, id, flags) {
	EntityMeshType *TP = (EntityMeshType*)TypePtr;
	CanCollide = TP->type_collide;
	fade = TP->type_fade;
	Mass = TP->type_mass;
	if(TP->mesh){
		CopyVec3(TP->mesh->BndMin, BoundMin);
		CopyVec3(TP->mesh->BndMax, BoundMax);
		BoundRad = TP->mesh->BndRad;
		if(TP->type_scalematrixon){
			BoundRad *= std::max(std::max(TP->type_scalematrix[0], TP->type_scalematrix[1]), TP->type_scalematrix[2]);
		}
	}
	teamid = 0;
	//
	TP->SpawnChains(this);
}
bool EntityMesh::Think(){
	EntityMeshType *TP = (EntityMeshType*)TypePtr;
	if(!TP->mesh) return false;
	if(!TP->type_directmatrix) Rot3ToMat3(Rotation, meshobj.Orient);
	//Add mesh-local offset.
	Vec3 tpos;
	Vec3MulMat3(TP->type_meshoffset, meshobj.Orient, tpos);
	AddVec3(Position, tpos);
	if(TP->type_scalematrixon && !TP->type_directmatrix){
		ScaleVec3(meshobj.Orient[0], TP->type_scalematrix[0]);
		ScaleVec3(meshobj.Orient[1], TP->type_scalematrix[1]);
		ScaleVec3(meshobj.Orient[2], TP->type_scalematrix[2]);
	}
	if(TP->constmovement){	//Constant undulating movement and/or rotation.
		ScaleAddVec3(TP->type_constrotate, VW->FrameFrac(), Rotation);
		NormRot3(Rotation);	//constwavetime is seconds for complete undulation.
		double t = fmod((double)VW->Time() * 0.001, std::max(0.01, double(TP->type_constwavetime))) / std::max(0.01, double(TP->type_constwavetime));
		Vec3 tv, tv2;
		ScaleVec3(TP->type_constwave, sin(t * PI2), tv);	//Sin wave undulation.
		Vec3MulMat3(tv, meshobj.Orient, tv2);	//Push undulation through orientation.
		AddVec3(tv2, tpos);	//Add to world pos.
	}
	meshobj.Configure(TP->texture, TP->mesh, meshobj.Orient, tpos, TP->type_rendflags, fade, BoundRad, TP->type_lodbias);
	if(TP->type_secondaryfrustum){
		VW->PolyRend->AddSecondaryObject(&meshobj);
	}else{
		if( (fade < 1.0f || (TP->type_rendflags & MESH_BLEND_ADD)) && !(TP->type_rendflags & MESH_BLEND_ENVMAP))
			VW->PolyRend->AddTransObject(&meshobj);
		else VW->PolyRend->AddSolidObject(&meshobj);
	}
	return true;
}
bool EntityMesh::SetInt(int type, int val){
	if(type == ATT_TEAMID){
		teamid = val;
		return true;
	}
	return EntitySprite::SetInt(type, val);
}
int EntityMesh::QueryInt(int type){
	if(type == ATT_TEAMID) return teamid;
	return EntitySprite::QueryInt(type);
}
bool EntityMesh::SetVec(int type, const void *v){
	if(v && (type == ATT_MATRIX3 || type == ATT_MATRIX43)){
		for(int i = 0; i < 9; i++) ((float*)meshobj.Orient)[i] = ((float*)v)[i];
		if(type == ATT_MATRIX43) CopyVec3(((float*)v) + 9, meshobj.Pos);
		return true;
	}
	return EntitySprite::SetVec(type, v);
}
bool EntityMesh::QueryVec(int type, void *out){
	if(out && (type == ATT_MATRIX3 || type == ATT_MATRIX43)){
		for(int i = 0; i < 9; i++) ((float*)out)[i] = ((float*)meshobj.Orient)[i];
		if(type == ATT_MATRIX43) CopyVec3(meshobj.Pos, ((float*)out) + 9);
		return true;
	}
	return EntitySprite::QueryVec(type, out);
}
EntityGroup EntityMesh::Group(){
	return ((EntityMeshType*)TypePtr)->type_group;
}

//******************************************************************
//**           Text Font Entity
//******************************************************************
EntityTextType::EntityTextType(ConfigFile *cfg, const char *c, const char *t) : EntitySpriteType(cfg, c, t) {//TypeBase(c, t) {
	charsx = charsy = 16;
	type_mixedcase = 0;
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("charsx")) cfg->GetIntVal(&charsx);
		if(cfg->FindKey("charsy")) cfg->GetIntVal(&charsy);
		if(cfg->FindKey("mixedcase")) cfg->GetIntVal(&type_mixedcase);
		memset(charmap, 255, sizeof(charmap));
		CStr tmap;
		if(cfg->FindKey("map")){	//Build character map.
			cfg->GetStringVal(tmap);
			for(int i = 0; i < tmap.len(); i++){
				unsigned char c = tmap.get()[i];
				if(type_mixedcase){
					charmap[(unsigned char)c] = i;
				}else{
					charmap[(unsigned char)tolower(c)] = i;
					charmap[(unsigned char)toupper(c)] = i;
				}
			}
		}
	}
	Transitory = true;
}
EntityBase *EntityTextType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityText(this, Pos, Rot, Vel, id, flags);
}
bool EntityTextType::CacheResources(){
	if(ResCached) return true;
	if(EntitySpriteType::CacheResources()){
		if(texture){
			texture->BlackOutX = charsx;
			texture->BlackOutY = charsy;	//This will black out lines between glyphs on mip-maps.
		}
	}
	return ResCached;
}
void EntityTextType::ListResources(FileCRCList *fl){
	if (ResListed) return;

	EntitySpriteType::ListResources(fl);

	ResListed = true;
}
void EntityTextType::UnlinkResources(){
	EntitySpriteType::UnlinkResources();
}
//Main code.
EntityText::EntityText(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntitySprite(et, Pos, Rot, Vel, id, flags) {
	timeofbirth = VW->Time();
	if(id > 0) timetodie = VW->Time() + id;
	else timetodie = 0;
	CanCollide = false;
	if(!Vel){
		Velocity[0] = Velocity[1] = Velocity[2] = 1.0f;	//Color.
	}
	opac = 1.0f;
}
bool EntityText::SetString(int type, const char *s){
	if(s && type == ATT_TEXT_STRING){
		str = s;
		if(MirroredOnClients && str.len() < 1000){
//OutputDebugLog(" *** Sending SetString to clients.\n");
			char b[1024];
			b[0] = (char)MSG_HEAD_SETTEXT;
			if(str.len() > 0) memcpy(&b[1], str.get(), str.len() + 1);
		//	PacketToSelf(TM_Ordered, b, str.len() + 2);
			QueuePacket(TM_Ordered, b, str.len() + 2);
			//TODO:  IF this is the problem...  Have to send Ordered or else packet arrives before
			//creation packet, and never gets delivered.  Fixable by queuing packets for not-yet-
			//created entities...
		}
		return true;
	}
	return false;
}
CStr EntityText::QueryString(int type){
	if(type == ATT_TEXT_STRING) return str;//{ return Active; };
	return "";
}
float EntityText::QueryFloat(int type){
	if(type == ATT_TEXT_OPACITY) return opac;
	return 0.0f;
}
bool EntityText::SetFloat(int type, float val){
	if(type == ATT_TEXT_OPACITY){ opac = val; return true; }
	return false;
}
void EntityText::DeliverPacket(const unsigned char *data, int len){
//OutputDebugLog(" *** Text Entity got packet.\n");
	if(data[0] == MSG_HEAD_SETTEXT){
		char b[1024];
		memcpy(b, &data[1], std::min(1024, len - 1));
		b[std::min(1023, len - 1)] = 0;
		str = b;
//OutputDebugLog(" *** Text Entity got SetText.\n");
	}
}
bool EntityText::Think(){
	EntityTextType *ET =(EntityTextType*)TypePtr;
//	float opac = 1.0f;
	if(timetodie > 0){
		if((VW->Time() + 1) >= timetodie) Remove();
		float t = (float)(VW->Time() - timeofbirth) / (float)(timetodie - timeofbirth);
		if(Flags & FLAG_TEXT_FADE) opac = 1.0f - t;
	}
	if(ET->texture && str.len() > 0){
		unsigned char glyph[1024];
		for(int i = 0; i < std::min(str.len(), 1024); i++) {
			unsigned char c;
			// Russ - compiler bug in VC6.0 :(
			c = str.get()[i];
			glyph[i] = ET->charmap[c];
		}
		float x = Position[0], y = Position[1];
		if(Flags & FLAG_TEXT_CENTERX) x -= ((float)str.len() * Rotation[0]) * 0.5f;
		if(Flags & FLAG_TEXT_CENTERY) y -= Rotation[1] * 0.5f;
		stringobj.Configure(ET->texture, ET->charsx, ET->charsy,
			x, y, Rotation[0], Rotation[1],
			glyph, std::min(str.len(), 1024), opac,
			MIN(1.0f, Velocity[0]), MIN(1.0f, Velocity[1]), MIN(1.0f, Velocity[2]));
		//	(Velocity[0] * 255.0f), (Velocity[1] * 255.0f), (Velocity[2] * 255.0f));
		stringobj.Z = Position[2];
		stringobj.AddRed = Velocity[0] - 1.0f;
		stringobj.AddGreen = Velocity[1] - 1.0f;
		stringobj.AddBlue = Velocity[2] - 1.0f;
		stringobj.GlyphScale = (Rotation[2] == 0.0f ? 1.0f : Rotation[2]);
		if(stringobj.AddRed > 0.0f || stringobj.AddGreen > 0.0f || stringobj.AddBlue > 0.0f){
			stringobj.AddGlyphScale = stringobj.GlyphScale;
			stringobj.GlyphScale = 1.0f;//stringobj.GlyphScale * 0.5f + 0.5f;
			stringobj.AddIters = 5;
		}else{
			stringobj.AddIters = 1;
		}
		VW->PolyRend->AddOrthoObject(&stringobj);
	//	VW->PolyRend->AddText(ET->texture, ET->charsx, ET->charsy,
	//		x, y, Rotation[0], Rotation[1],
	//		glyph, min(str.len(), 1024), opac);
	}
	return true;
}

EntityIconType::EntityIconType(ConfigFile *cfg, const char *c, const char *t) : EntitySpriteType(cfg, c, t) {
	Transitory = true;
}
EntityBase *EntityIconType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityIcon(this, Pos, Rot, Vel, id, flags);
}
EntityIcon::EntityIcon(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags) : EntitySprite(et, Pos, Rot, Vel, id, flags) {
	CanCollide = false;
	opac = 1.0f;
}
float EntityIcon::QueryFloat(int type){
	if(type == ATT_TEXT_OPACITY) return opac;
	return 0.0f;
}
bool EntityIcon::SetFloat(int type, float val){
	if(type == ATT_TEXT_OPACITY){ opac = val; return true; }
	return false;
}
bool EntityIcon::Think()
{
	float x = Position[0];
	float y = Position[1];
	float w = Rotation[0];
	float h = Rotation[1];
	EntityIconType *ET =(EntityIconType*)TypePtr;
	if(ET->texture)
	{
		screenobj.Z = Position[2];
		screenobj.Configure(ET->texture, 0, ET->type_rendflags,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f, 1, 1, 1, opac);
		screenobj.ConfigurePoly(x, y, x+w, y, x+w, y+h, x, y+h);
		VW->PolyRend->AddOrthoObject(&screenobj);
	}
	return true;
}

EntityChamfered2DBoxType::EntityChamfered2DBoxType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t)
{
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("chamferwidth ")) cfg->GetFloatVal(&chamferwidth);
	}
	Transitory = true;
}
EntityBase *EntityChamfered2DBoxType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityChamfered2DBox(this, Pos, Rot, Vel, id, flags);
}
EntityChamfered2DBox::EntityChamfered2DBox(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags) : EntityBase(et, Pos, Rot, Vel, id, flags)
{
	CanCollide = false;
	if(!Vel){
		Velocity[0] = Velocity[1] = Velocity[2] = 1.0f;	//Color.
	}
	red = Velocity[0];
	green = Velocity[1];
	blue = Velocity[2];
	opac = 1.0f;
}
float EntityChamfered2DBox::QueryFloat(int type){
	if(type == ATT_TEXT_OPACITY) return opac;
	return 0.0f;
}
bool EntityChamfered2DBox::SetFloat(int type, float val){
	if(type == ATT_TEXT_OPACITY){ opac = val; return true; }
	return false;
}
bool EntityChamfered2DBox::Think()
{
	float x = Position[0];
	float y = Position[1];
	float w = Rotation[0];
	float h = Rotation[1];
	EntityChamfered2DBoxType *ET =(EntityChamfered2DBoxType*)TypePtr;

	if(ET)
	{
		screenobj.Z = Position[2];
		screenobj.Configure(x, y, w, h, ET->chamferwidth, red, green, blue, opac);
		VW->PolyRend->AddOrthoObject(&screenobj);
	}
	return true;
}


//***************************************************************
//             Way-Point Entity
//***************************************************************
//Type Handler.
EntityWaypointType::EntityWaypointType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t) {
	type_hitradius = 10.0f;
	type_hitradiusadd = 0.0f;
	if(cfg){	//Read config file and setup static info.
		if(cfg->FindKey("HitRadius")) cfg->GetFloatVal(&type_hitradius);
		if(cfg->FindKey("HitRadiusAdd")) cfg->GetFloatVal(&type_hitradiusadd);
	}
}
EntityBase *EntityWaypointType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityWaypoint(this, Pos, Rot, Vel, id, flags);
}
bool EntityWaypointType::CacheResources(){
	if(ResCached) return true;
	return ResCached = true;
}
void EntityWaypointType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	ResListed = true;
}
void EntityWaypointType::UnlinkResources(){
	ResCached = false;
	ResListed = false;
}

//Main Code.
EntityWaypoint::EntityWaypoint(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags, int spawntype) : EntityBase(et, Pos, Rot, Vel, id, flags) {
	EntityWaypointType *TP = (EntityWaypointType*)et;
	cylindrical = false;
	CStr WayName = "WaypointPath" + String(id / PATHWAYID);
	EntityWaypoint *ent = (EntityWaypoint*)FindRegisteredEntity(WayName);
	WaypointNode *wn = NULL;//ent->wayhead
	int mid = (id % PATHWAYID) / MAINWAYID, sid = id % MAINWAYID;
	if(ent){
		wn = &ent->wayhead;
		while(wn->NextLink() &&
			(wn->NextLink()->mainid * MAINWAYID + wn->NextLink()->subid < id % PATHWAYID)){
			wn = wn->NextLink();
		}
	//	RemoveMe = REMOVE_NORMAL;
		//Not removing so entity will still be sent to new clients over the net.
	}else{
		RegisterEntity(WayName);	//NOW I AM THE MASTER!
		wn = &wayhead;
	}
	if(wn && (wn = wn->InsertObjectAfter(new WaypointNode))){
		wn->mainid = mid;
		wn->subid = sid;
		wn->flags = flags;
		wn->suggestedradius = TP->type_hitradius;
		wn->type = TP;
		wn->teamspawntype = spawntype;
		if(Pos) CopyVec3(Pos, wn->Pos);
		else ClearVec3(wn->Pos);
		if(Vel) CopyVec3(Vel, wn->Vel);
		else ClearVec3(wn->Vel);
	}
	CanThink = false;
}
bool EntityWaypoint::Think(){
	return true;
}
float EntityWaypoint::HitRadius(){
	return ((EntityWaypointType*)TypePtr)->type_hitradius;
}
int EntityWaypoint::WalkWaypoints(int current, Vec3 pos, Vec3 vel, WaypointNode **wayreturn, WaypointNode **majorway){
	//Starts with 'current' waypoint index and walks forwards a maximum of 1
	//waypoints, returning a pointer to the new travel-to waypoint in wayreturn.
	//Failure is a -1 return index.
	EntityWaypointType *TP = (EntityWaypointType*)TypePtr;
	WaypointNode *wn = WaypointByIndex(current), *wnn;
	int next = current + 1;
	if(wn && pos && vel){
		if(!(wnn = wn->NextLink())){
			next = 0;
			wnn = WaypointByIndex(next);
		}
		if(wnn){
			float vel1, vel2;
			Vec3 tv;
			ScaleAddVec3(vel, 0.1f, pos, tv);
			//Closing velocity for current way.
			vel1 = DistVec3(pos, wn->Pos);
			vel1 -= DistVec3(tv, wn->Pos);
			//Closing velocity for next way.
			vel2 = DistVec3(pos, wnn->Pos);
			vel2 -= DistVec3(tv, wnn->Pos);
			//
			float wndist;
			if(cylindrical){
				float dx = pos[0] - wn->Pos[0], dz = pos[2] - wn->Pos[2];
				wndist = sqrtf(dx * dx + dz * dz);
			}else{
				wndist = DistVec3(pos, wn->Pos);
			}
			//
			if( (wn->subid != 0 && DistVec3(pos, wnn->Pos) < DistVec3(pos, wn->Pos)) ||
				(wn->subid != 0 && vel1 < 0.0f && vel2 > 0.0f) ||
				(wndist < TP->type_hitradius + TP->type_hitradiusadd) ){
				current = next;
				wn = wnn;
			}
			if(wayreturn) *wayreturn = wn;
			if(majorway){	//Caller wants next major waypoint returned too.
				WaypointNode *nm = wn;
				while(nm->subid != 0){
					nm = nm->NextLink();
					if(!nm) nm = wayhead.NextLink();
					if(nm == wn) break;
				}
				*majorway = nm;
			}
			return current;
		}
	}
	return -1;
}

int EntityWaypoint::CountMajorWayPoints()
{
	WaypointNode *wn = WaypointByIndex(0);
	int iCnt = 0;
	while(wn)
	{
		if(wn->subid == 0)
			iCnt++;
		wn = wn->NextLink();
	}
	return iCnt;
}

WaypointNode *EntityWaypoint::WaypointByIndex(int index){
	//Returns waypoint at index in list (0 is first waypoint).
	if(index >= 0 && wayhead.NextLink()){
		return wayhead.NextLink()->FindItem(index);
	}
	return NULL;
}
int EntityWaypoint::ClosestWaypoint(Vec3 pos, WaypointNode **wayreturn){
	//Returns closest major waypoint to position.
	if(pos && wayhead.NextLink()){
		int index = 0;
		WaypointNode *wn = wayhead.NextLink();
		int closest = -1;
		WaypointNode *wnc = NULL;
		float dist = 1000000.0f;
		while(wn){
			float d = DistVec3(pos, wn->Pos);
			if(d < dist && wn->subid == 0){
				dist = d;
				closest = index;
				wnc = wn;
			}
			wn = wn->NextLink();
			index++;
		}
		if(wayreturn) *wayreturn = wnc;
		return closest;
	}
	return -1;
}

//******************************************************************
//**           Sound Entity
//******************************************************************
EntitySoundType::EntitySoundType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t)
 {
	int i;
	for(i = 0; i < MAX_SOUNDS; i++){
		sound[i] = 0;
		soundfile[i] = "";
		soundid[i] = -1;
	}
	type_volume = 1.0f;
	type_freqvariance = 0.0f;
	type_priority = 0.5f;
	type_loop = false;
	type_minmaxdistscale = 1.0f;
	type_is2dsound = 0;
	type_rampup = type_rampdown = type_rampfreq = 0.0f;
	type_announcer = 0;
	type_volumebias = 0.5f;
	localise = 0;

	//
	if(cfg)
	{	//Read config file and setup static info.
		if(cfg->FindKey("announcer")) cfg->GetIntVal(&type_announcer);
		if(cfg->FindKey("sound")) cfg->GetStringVal(soundfile[0]);
		if(cfg->FindKey("altsound")) cfg->GetStringVal(soundfile[1]);
		for(i = 2; i < MAX_SOUNDS; i++) {
			if(cfg->FindKey("altsound" + String(i))) cfg->GetStringVal(soundfile[i]);
		}

		if(cfg->FindKey("soundid")) cfg->GetIntVal(&soundid[0]);
		if(cfg->FindKey("altsoundid")) cfg->GetIntVal(&soundid[1]);
		for (i = 2; i < MAX_SOUNDS; i++) {
			if(cfg->FindKey("altsoundid" + String(i))) cfg->GetIntVal(&soundid[i]);
		}
		if(cfg->FindKey("local")) cfg->GetIntVal(&localise);

		if(cfg->FindKey("volume")) cfg->GetFloatVal(&type_volume);
		if(cfg->FindKey("freqvariance")) cfg->GetFloatVal(&type_freqvariance);
		if(cfg->FindKey("priority")) cfg->GetFloatVal(&type_priority);
		if(cfg->FindKey("loop")) cfg->GetBoolVal(&type_loop);
		if(cfg->FindKey("is2dsound")) cfg->GetIntVal(&type_is2dsound);
		if(cfg->FindKey("DistanceScale"))
		{
			float t = 1.0f;
			cfg->GetFloatVal(&t);
			type_minmaxdistscale = 1.0f / std::max(0.01f, t);
		}
		if(cfg->FindKey("RampUp")) cfg->GetFloatVal(&type_rampup);
		if(cfg->FindKey("RampDown")) cfg->GetFloatVal(&type_rampdown);
		if(cfg->FindKey("RampFreq")) cfg->GetFloatVal(&type_rampfreq);
		if(cfg->FindKey("volumebias")) cfg->GetFloatVal(&type_volumebias);
	}
	Transitory = true;
}

EntityBase *EntitySoundType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags)
{
	return new EntitySound(this, Pos, Rot, Vel, id, flags);
}

bool EntitySoundType::CacheResources()
{
	if(ResCached) return true;
	ResCached = true;	//Prevents circular caching.  Be sure to set false if failure.
	for(int i = 0; i < MAX_SOUNDS; i++)
	{
		if (soundid[i] == -1) {
			sound[i] = VW->GetSound(soundfile[i], type_loop, type_volumebias);
		} else {
			// FIXME: where are paths relative to?
			sound[i] = VW->GetSound(SoundPaths.Get(soundid[i]), type_loop, type_volumebias);
		}
	}
	if(sound[0])
	{
		return ResCached = true;
	}
	return ResCached = false;
}
void EntitySoundType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	ResListed = true;

	for(int i = 0; i < MAX_SOUNDS; i++)
	{
		if (soundid[i] == -1) {
			fl->FileCRC(soundfile[i]);
		} else {
			// FIXME: where are paths relative to?
			fl->FileCRC(SoundPaths.Get(soundid[i]));
		}
	}
}
void EntitySoundType::UnlinkResources()
{
	ResCached = false;
	ResListed = false;
	for(int i = 0; i < MAX_SOUNDS; i++)
	{
		sound[i] = NULL;
	}
}

int EntitySoundType::InterrogateInt(const char *thing)
{
	if(thing && CmpLower(thing, "loop"))
		return type_loop;

	return EntityTypeBase::InterrogateInt(thing);
}

//Main code.
EntitySound::EntitySound(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,int id, int flags): EntityBase(et, Pos, Rot, Vel, id, flags)
{
	EntitySoundType *TP = (EntitySoundType*)TypePtr;
	CanCollide = false;
	chan = -1;
	wave = -1;
	volume = TP->type_volume;
	freq = 1.0f + (((double)TMrandom() / (double)(RAND_MAX / 2)) - 1.0f) * TP->type_freqvariance;
	connectedent = 0;
	rampup = rampdown = 0;
	loiter = 0;
}
EntitySound::~EntitySound()
{
	if(VW && chan > -1)
	{
		VW->snd.Stop(chan);
	}
}
bool EntitySound::Think()
{
	EntitySoundType *TP = (EntitySoundType*)TypePtr;
	if(TP->type_announcer)
	{
		if((Flags & FLAG_SOUND_PLAYANNOUNCER) == 0)
		{
			// you got nuke , and other announcements should not play in training mode
			if(!(VW->GameMode() & GAMEMODE_TRAINING))
			{
				EntityBase *e = VW->FindRegisteredEntity("GOD");
				if(e && VW->Net.IsClientActive() == false)
				{
					//OutputDebugString("\nSending announcement to God.\n\n");
					//Tell GOD to queue this announcer sound, and possibly propagate it too.
					e->SetInt(ATT_ANNOUNCER_ID, ID);
					e->SetInt(ATT_CMD_ANNOUNCER, TP->thash);
				}
			}
			Remove();
			return true;
		}
	}
	if(chan < 0)
	{
		if(TP->sound[0])
			wave = *TP->sound[0];	//Failsafe.
		int i, cnt = 0, snd;
		for(i = 0; i < MAX_SOUNDS; i++)
			if(TP->sound[i]) cnt++;	//Count valid sounds.

		snd = GID % std::max(1, cnt);	//Use GID to select one of the valid sounds.

		for(i = 0; i < MAX_SOUNDS; i++)
		{
			if(TP->sound[i] && snd-- == 0)
				wave = *TP->sound[i];	//Pick one of the sounds randomly.
		}
		//
		if(wave < 0)
		{	//Oops!  Probably trying to play before download.
			if(loiter < 4)
			{
				loiter++;
				return true;	//Loiter up to 4 frames waiting for sound to be downloaded.
			}
			else
			{
				Remove();	//Too long, remove self.
				return true;
			}
		}
		//
		VW->snd.PlayBuffer(wave, TP->type_priority);
		//
		chan = VW->snd.GetLastVoice();
		if(chan >= 0 && TP->type_minmaxdistscale != 1.0f)
			VW->snd.SetVoiceDistanceScale(chan, TP->type_minmaxdistscale, TP->type_minmaxdistscale);
		//Multiplies minimum and maximum distances to change sound attenuation speed.
	}
	//
	if(connectedent)
	{
		EntityBase *e = VW->GetEntity(connectedent);
		if(e){
			Vec3 v;
			SubVec3(e->Position, Position, v);	//Amount we're pulled along is velocity, converted to meters/sec.
			ScaleVec3(v, 1.0f / std::max(0.0001f, VW->FrameFrac()), Velocity);
			CopyVec3(e->Position, Position);	//Pull us along with connected ent.
		}
		else
		{
			if(SetInt(ATT_CMD_NICEREMOVE, 1) == 0) Remove();	//Try nice remove (rampdown).
		}
	}
	else
	{
		ScaleAddVec3(Velocity, VW->FrameFrac(), Position);	//We're flying!
	}
	//
	if(chan >= 0)
	{
		//
		//Do frequency ramping up and down.
		float f = freq;
		if(rampup <= (TP->type_rampup * 1000.0f) && rampdown == 0)
		{
			rampup += VW->FrameTime();
			if(rampup <= (TP->type_rampup * 1000.0f))
			{	//If it's past rampup time, leave f equal to freq.
				f = LERP(TP->type_rampfreq, freq, (float)rampup / (MAX(TP->type_rampup, 0.01f) * 1000.0f));	//Otherwise set f to a fraction of freq.
			}
		}
		if(rampdown > 0)
		{
			if(rampdown < (TP->type_rampdown * 1000.0f))
			{	//Still in ramp-down phase.
				rampdown += VW->FrameTime();
				f = LERP(f, TP->type_rampfreq, (float)rampdown / (MAX(TP->type_rampdown, 0.01f) * 1000.0f));
			}
			else
			{	//Time to remove.
				f = TP->type_rampfreq;
				Remove();
			}
		}
		//
		if(VW->snd.GetVoiceStatus(chan) == 0 || VW->snd.GetVoiceWave(chan) != wave)
		{
			chan = -1;
			if(TP->type_loop == false) Remove();
		}
		else
		{
			VW->snd.SetVoice3D(chan, Position, Velocity, volume, MAX(f, 0.01), TP->type_priority);
			if(TP->type_is2dsound)
				VW->snd.SetVoice2D(chan);
		}
	}
	else
		Remove();

	return true;
}

bool EntitySound::SetInt(int type, int attr)
{
	if(type == ATT_CONNECTED_ENT)
	{
		connectedent = attr; return true;
	}

	if(type == ATT_CMD_NICEREMOVE)
	{
		rampdown = std::max(1, rampdown);
		return true;
	}
	return false;
}
int EntitySound::QueryInt(int type)
{
	EntitySoundType *TP = (EntitySoundType*)TypePtr;

	if(type == ATT_CONNECTED_ENT)
		return connectedent;

	if(type == ATT_LOOP)
		return TP->type_loop;

	return 0;
}

//
//  MixIn Classes for specific helper actions.
//
//***************************************************************
//             Chains a number of additional entity creations to initial one.  Created ents don't chain further.
//             Up to 10 entity class/types can be chained.
//***************************************************************
MixinEntityChain::MixinEntityChain(ConfigFile *cfg, const char *c){
	nChains = ChainType = NextChain = 0;
	MultiplyChain = 1;
	NochainOverride = 0;
	if(c) DefClass = c;
	if(cfg){
		for(int i = 0; i < MAX_MIXIN_CHAIN; i++){
			if(cfg->FindKey("EntityChain" + String(i))){
				cfg->GetStringVal(Chains[i]);
				nChains = i + 1;
			}else{
				break;
			}
		}
		if(cfg->FindKey("EntityChainMode")){
			CStr m;
			cfg->GetStringVal(m);
			if(CmpLower(m, "random")) ChainType = CHAINTYPE_RANDOM;
			if(CmpLower(m, "cycle")) ChainType = CHAINTYPE_CYCLE;
		}
		if(cfg->FindKey("EntityChainMultiply")) cfg->GetIntVal(&MultiplyChain);
		if(cfg->FindKey("EntityChainNochainOverride")) cfg->GetIntVal(&NochainOverride);
	}
}
MixinEntityChain::~MixinEntityChain(){
}
bool MixinEntityChain::CacheResources(VoxelWorld *VW){	//Call in resource caching function of Class.
	if(!VW) return false;
	bool result = true;
	for(int i = 0; i < nChains; i++){
		if(!VW->CacheResources(DefClass, Chains[i])) result = false;
	}
	return result;
}
void MixinEntityChain::ListResources(VoxelWorld *VW){	//Call in resource caching function of Class.
//Russ	if (ResListed) return;
//Russ	ResListed = true;

	if(!VW) return;
	for(int i = 0; i < nChains; i++){
		VW->ListResources(DefClass, Chains[i]);
	}
}
void MixinEntityChain::SpawnChains(EntityBase *ent){	//Call this in your entity constructor, passing in "this".
	if(nChains > 0 && ent && (ent->Flags & ENTITYFLAG_NOCHAIN) == 0){
		for(int m = 0; m < MultiplyChain; m++){
			int i;
			int server = !(ent->GetVW()->Net.IsClientActive());	//ent->GetVW()->Net.IsServerActive();
			switch(ChainType){
			case CHAINTYPE_ALL :
				for(i = 0; i < nChains; i++){
				//	ent->GetVW()->AddEntity(DefClass, Chains[i], ent->Position, ent->Rotation,
				//		ent->Velocity, ent->ID, ent->Flags | ENTITYFLAG_NOCHAIN, 0, 0);
					Spawn(ent, i, server);
				}
				break;
			case CHAINTYPE_RANDOM :
			//	ent->GetVW()->AddEntity(DefClass, Chains[rand() % nChains], ent->Position, ent->Rotation,
			//		ent->Velocity, ent->ID, ent->Flags | ENTITYFLAG_NOCHAIN, 0, 0);
				Spawn(ent, TMrandom() % nChains, server);
				break;
			case CHAINTYPE_CYCLE :
			//	ent->GetVW()->AddEntity(DefClass, Chains[NextChain++], ent->Position, ent->Rotation,
			//		ent->Velocity, ent->ID, ent->Flags | ENTITYFLAG_NOCHAIN, 0, 0);
				Spawn(ent, NextChain++, server);
				if(NextChain >= nChains) NextChain = 0;
				break;
			}
		}
	}
}
void MixinEntityChain::Spawn(EntityBase *ent, int which, int server){
	EntityTypeBase *TP = ent->GetVW()->FindEntityType(DefClass, Chains[which]);
	if(TP && (TP->Transitory || server)){	//This makes sure that NonTransitory entities are ONLY chained on server.
		int flags = ent->Flags;
		if(NochainOverride && (flags & ENTITYFLAG_NOCHAINFORCE) == 0){	//This allows the chained entities to spawn their own chains, once.
			flags |= ENTITYFLAG_NOCHAINFORCE;	//Spawned entities aren't prevented from chaining more, but are prevented from continueing the cycle.
		}else{
			flags |= ENTITYFLAG_NOCHAIN;	//Normally, force chained ents to not chain any more.
		}
		ent->GetVW()->AddEntity(TP, ent->Position, ent->Rotation,
			ent->Velocity, ent->ID, flags, 0, 0);
	}
}
