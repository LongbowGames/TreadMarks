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

#ifndef __ENTITYFLAG_H__
#define __ENTITYFLAG_H__

#include "EntityTank.h"
#include "VoxelWorld.h"

#define ATT_FLAG_TEAM_INDEX		0x8c940400
#define ATT_FLAG_DROPPED		0x8c940401
#define ATT_FLAG_ARMORSCALE		0x8c940402
#define ATT_FLAG_FIRERATESCALE	0x8c940403
#define ATT_FLAG_SPEEDSCALE		0x8c940404

//FLAG superclass, derived from MESH.
class EntityFlagType : public EntityInsigniaType {
friend class EntityFlag;
private:
	int type_fragadd;
	int type_teamid;
	int type_droptime;
	int type_unique;
	int type_flagkill;
	CStr type_polemeshtype;
	float type_carrierarmorscale;
	float type_carrierspeedscale;
	float type_carrierfireratescale;

	CStr type_pickupsound, type_inentity, type_spawnsound, type_outentity, type_spawnentity;
public:
	EntityFlagType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	int InterrogateInt(const char *thing); // Repsonds to "unique"
};
class EntityFlag : public EntityInsignia {
private:
	EntityGID	tankgod;
	EntityGID	AttachedToTankID;
	EntityGID	FlagDeathEntity;
	int			iTeamIndex;
	bool		bDropped;
	int			iDropTime;
	EntityGID	PoleEntity;
public:
	EntityFlag(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	~EntityFlag();
	bool Think();
	bool CollisionWith(EntityBase *collider, Vec3 colpnt);
	int QueryInt(int type);
	float QueryFloat(int type);
	bool SetInt(int type, int att);
	void DeliverPacket(const unsigned char *data, int len);
};

#endif // __ENTITYFLAG_H__
