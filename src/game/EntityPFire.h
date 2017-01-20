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

//ParticleFire effect Entity.

#ifndef ENTITYPFIRE_H
#define ENTITYPFIRE_H

#include "EntityBase.h"

//
//This function MUST be called ONCE by your main app to register the RaceTank classes.
int RegisterPFireEntities();
//

////////////////////////////////////////////////
//     Particle Fire effect entity.
////////////////////////////////////////////////

class EntityPFireType : public EntityTypeBase {
public:
	ImageNode *tex1, *tex2;
	float type_updaterate;
	int type_size, type_particles;
public:
	EntityPFireType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
};
#define ATT_PFIRE_PING		0x68c80000
//SetInt this to 1 on a regular basis (every second at least) to make sure particle fire effect is rendered.
#define ATT_PFIRE_FOLLOWX	0x68c80001
#define ATT_PFIRE_FOLLOWY	0x68c80002
//SetFloat these to specify a point in the burn buffer for particles to attract to.
#define ATT_PFIRE_FOLLOW	0x68c80003
//SetInt this to one to turn on attracting.
#define ATT_PFIRE_EXPLODE	0x68c80004
//SetInt this to cause an explosion.
//
#define PFirePingTimeout 1000
class EntityPFire : public EntityBase {
public:
	int mssinceping, mssinceburn;
	float followx, followy;
	bool follow;
public:
	EntityPFire(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
public:
	bool Think();
	int QueryInt(int type);
	bool SetInt(int type, int att);
	bool SetFloat(int type, float att);
	EntityGroup Group(){ return GROUP_PROP; };
};

#endif

