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

#include "EntityFlag.h"
#include "EntityTank.h"
#include "TankGame.h"


#define FLAG_FLOAT 4.0f
//******************************************************************
//**  Flag Entity  --  Derived Class
//******************************************************************
EntityFlagType::EntityFlagType(ConfigFile *cfg, const char *c, const char *t) :
	EntityInsigniaType(cfg, c, t)
{
	type_fragadd = 0;
	type_teamid = 0;

	type_pickupsound = "pickup";
	type_inentity = "explosphere/blipin";
	type_spawnsound = "";
	type_outentity = "explosphere/blipout";
	type_spawnentity = "explosphere/blipin";

	type_carrierarmorscale = 1;
	type_carrierspeedscale = 1;
	type_carrierfireratescale = 1;

	type_droptime = 5;

	type_flagkill = 1;
	type_unique = 1;
	//
	if(cfg){
		if(cfg->FindKey("fragadd")) cfg->GetIntVal(&type_fragadd);
		if(cfg->FindKey("teamid")) cfg->GetIntVal(&type_teamid);
		if(cfg->FindKey("droptime")) cfg->GetIntVal(&type_droptime);
		if(cfg->FindKey("unique")) cfg->GetIntVal(&type_unique);

		if(cfg->FindKey("flagkill")) cfg->GetIntVal(&type_flagkill);

		if(cfg->FindKey("polemeshtype")) cfg->GetStringVal(type_polemeshtype);

		if(cfg->FindKey("carrierarmorscale")) cfg->GetFloatVal(&type_carrierarmorscale);
		if(cfg->FindKey("carrierspeedscale")) cfg->GetFloatVal(&type_carrierspeedscale);
		if(cfg->FindKey("carrierfireratescale")) cfg->GetFloatVal(&type_carrierfireratescale);

		if(cfg->FindKey("pickupsound")) cfg->GetStringVal(type_pickupsound);
		if(cfg->FindKey("spawnsound")) cfg->GetStringVal(type_spawnsound);
		if(cfg->FindKey("inentity")) cfg->GetStringVal(type_inentity);
		if(cfg->FindKey("outentity")) cfg->GetStringVal(type_outentity);
		if(cfg->FindKey("spawnentity")) cfg->GetStringVal(type_spawnentity);
	}
	Transitory = false;
	type_directmatrix = 0;
}

EntityBase *EntityFlagType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags)
{
	return new EntityFlag(this, Pos, Rot, Vel, id, flags);
}

bool EntityFlagType::CacheResources()
{
	if(!ResCached){
		if(EntityMeshType::CacheResources()){
			VW->CacheResources("sound", type_pickupsound);
			VW->CacheResources("sound", type_spawnsound);
			VW->CacheResources("explosphere", type_inentity);
			VW->CacheResources("explosphere", type_outentity);
			VW->CacheResources("explosphere", type_spawnentity);
			VW->CacheResources("mesh", type_polemeshtype);
		}
	}
	return ResCached;
}

void EntityFlagType::ListResources(FileCRCList *fl)
{
	if (ResListed) return;

	EntityInsigniaType::ListResources(fl);
	VW->ListResources("sound", type_pickupsound);
	VW->ListResources("sound", type_spawnsound);
	VW->ListResources("explosphere", type_inentity);
	VW->ListResources("explosphere", type_outentity);
	VW->ListResources("explosphere", type_spawnentity);
	VW->ListResources("mesh", type_polemeshtype);
}

int EntityFlagType::InterrogateInt(const char *thing)
{
	if(!thing) return 0;
	if(CmpLower(thing, "unique")) return type_unique;
	return EntityInsigniaType::InterrogateInt(thing);
}

EntityFlag::EntityFlag(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
						 int id, int flags) : EntityInsignia(et, Pos, Rot, Vel, id, flags)
{
	EntityFlagType *TP = (EntityFlagType*)TypePtr;
	CanCollide = false;
	ScaleVec3(BoundMax, 2.0f);	//Double collision bounding box.
	ScaleVec3(BoundMin, 2.0f);
	BoundMin[1] -= (FLAG_FLOAT + 1.0f);
	BoundRad += FLAG_FLOAT;	//Expand bbox to collide with tanks below.
	Mass = 0.0f;
	Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + FLAG_FLOAT;
	tankgod = 0;
	AttachedToTankID = 0;

	iTeamIndex = CLAMP(TP->type_teamid - 1, 0, CTankGame::Get().GetNumTeams());
	if(iTeamIndex == TP->type_teamid - 1)
		teamid = CTankGame::Get().GetTeam(iTeamIndex).hash;
	else
	{
		teamid = -1;
		iTeamIndex = 0;
	}

	PoleEntity = 0;

	bDropped = false;
	//
	VW->AddEntity("explosphere", TP->type_spawnentity, Position, NULL, NULL, 0, 0, 0, 0);
	VW->AddEntity("sound", TP->type_spawnsound, Position, NULL, NULL, 0, 0, 0, 0);
}

EntityFlag::~EntityFlag()
{
	if(VW && PoleEntity){
		EntityBase *e = VW->GetEntity(PoleEntity);
		if(e) e->Remove();
	}
}

bool EntityFlag::Think()
{
	EntityFlagType *TP = (EntityFlagType*)TypePtr;
	if(AttachedToTankID == 0)
	{
		TP->type_directmatrix = 0;
		if(TP->type_polemeshtype != "" && !PoleEntity && AttachedToTankID == 0 && !RemoveMe)
		{
			// make the pole
			EntityBase *pole = VW->AddEntity("mesh", TP->type_polemeshtype, Position, NULL, NULL, 0, 0, 0, 0);
			if(pole)
				PoleEntity = pole->GID;
		}
		Position[1] = VW->Map.FGetI(Position[0], -Position[2]) + FLAG_FLOAT;
		int iTeamIndex = -1;
		CTankGame::Get().TeamIndexFromHash(teamid, &iTeamIndex);
		if(CTankGame::Get().GetSettings()->TeamPlay && CTankGame::Get().GetSettings()->TeamScores && CTankGame::Get().GetNumTeams() > 1 && CTankGame::Get().GetNumTeams() > iTeamIndex)
		{
			if(VW->CheckCollision(this, GROUP_ACTOR))
			{
				EntityBase *col;
				Vec3 colpnt;
				while(col = VW->NextCollider(colpnt))
					CollisionWith(col, colpnt);
			}
			//
			if(VW->Net.IsClientActive() == false)
			{
				if(bDropped)
				{
					iDropTime += VW->FrameTime();
					// check droptime and if exceeded, remove entity so it respawns
					if(iDropTime > TP->type_droptime * 1000)
					{
						VW->AddEntity("explosphere", TP->type_outentity, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
						// remove self to auto-spawn back at waypoint
						Remove();
					}
				}
			}
		}
	}
	else
	{
		EntityBase *e = VW->GetEntity(AttachedToTankID);
		if(e == NULL) // lost the tank that was carrying the flag
			SetInt(ATT_WEAPON_STATUS, 0); // drop the flag
		else
			TP->type_directmatrix = 1;
	}
	EntityTankGod *god;	//Register self with GOD.
	if(!tankgod){
		god = (EntityTankGod*)FindRegisteredEntity("TANKGOD");
		if(god)
			tankgod = god->GID;
	}
	if(tankgod && (god = (EntityTankGod*)VW->GetEntity(tankgod)))
		god->UpdatePowerUp(this, 0);

	if(TMrandom() % 2000 < VW->FrameTime())
		SetInt(ATT_WEAPON_STATUS, AttachedToTankID); // refresh clients

	return EntityInsignia::Think();
}

bool EntityFlag::CollisionWith(EntityBase *collider, Vec3 colpnt)
{
	EntityFlagType *TP = (EntityFlagType*)TypePtr;
	//Check to see if a tank hit us.
	if(CmpLower(collider->TypePtr->cname, "racetank") && teamid != -1 && VW->Net.IsClientActive() == false)	//Tank touched us.
	{
		if(collider->QueryInt(ATT_TEAMID) != teamid) // teams don't match .. pick it up if you can
		{
			if(collider->QueryInt(ATT_FLAGID) == 0)
			{
				collider->SetInt(ATT_FLAGID, GID);
				SetInt(ATT_WEAPON_STATUS, collider->GID);

				// send announcement that the flag has been grabbed
				char sOutput[1024];
				sprintf(sOutput, Text.Get(TEXT_FLAG_PICKUP), collider->QueryString(ATT_NAME).get(), CTankGame::Get().GetTeam(iTeamIndex).name.get());
				VW->StatusMessage(sOutput, STATUS_PRI_NETWORK);
			}

			VW->AddEntity("explosphere", TP->type_outentity, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
			VW->AddEntity("sound", TP->type_pickupsound, Position, NULL, NULL, collider->GID, 0, 0, ADDENTITY_FORCENET);
		}
		else // this is the collider's team's flag
		{
			// first check to see if the flag is on the ground, if so, remove it so it repsawns back at the base
			if(bDropped)
			{
				// Announce that the flag has been returned
				char sOutput[1024];
				sprintf(sOutput, Text.Get(TEXT_FLAG_RETURN), collider->QueryString(ATT_NAME).get(), CTankGame::Get().GetTeam(iTeamIndex).name.get());
				VW->StatusMessage(sOutput, STATUS_PRI_NETWORK);

				EntityBase *pole = VW->GetEntity(PoleEntity);
				if(pole)
					pole->Remove();
				PoleEntity = 0;

				VW->AddEntity("explosphere", TP->type_outentity, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
//				bDropped = false;
				Remove();
			}
			else // it's at the base, check to see if the collider is carrying an enemy flag.
			{
				int iColFlag = collider->QueryInt(ATT_FLAGID);
				if(iColFlag != 0)
				{
					EntityBase *CapturedFlagEnt = VW->GetEntity(iColFlag);
					if(CapturedFlagEnt)
					{
						// Award frags and remove enemy flag so it respawns
						collider->SetInt(ATT_FRAGS, collider->QueryInt(ATT_FRAGS) + TP->type_fragadd);

						// Announce that the player has scored
						char sOutput[1024];
						sprintf(sOutput, Text.Get(TEXT_FLAG_CAPTURE), collider->QueryString(ATT_NAME).get(), CTankGame::Get().GetTeam(CapturedFlagEnt->QueryInt(ATT_FLAG_TEAM_INDEX)).name.get());
						VW->StatusMessage(sOutput, STATUS_PRI_NETWORK);

						if(TP->type_flagkill)
						{
							// loop through the tanks and flag kill the team of the flag that was captured
							EntityTankGod *egod = (EntityTankGod*)VW->FindRegisteredEntity("TANKGOD");
							if(egod)
							{
								egod->KillTeam(CapturedFlagEnt->QueryInt(ATT_TEAMID), collider->GID);
							}
						}
						VW->AddEntity("explosphere", TP->type_outentity, Position, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET);
						CapturedFlagEnt->Remove();
					}
				}
			}
		}
	}
	return true;
}

int EntityFlag::QueryInt(int type)
{
	if(type == ATT_WEAPON_STATUS) return AttachedToTankID;
	if(type == ATT_FLAG_TEAM_INDEX) return iTeamIndex;
	if(type == ATT_FLAG_DROPPED) return bDropped;

	return EntityInsignia::QueryInt(type);
}

float EntityFlag::QueryFloat(int type)
{
	EntityFlagType *TP = (EntityFlagType*)TypePtr;

	if(type == ATT_FLAG_ARMORSCALE) return TP->type_carrierarmorscale;
	if(type == ATT_FLAG_FIRERATESCALE) return TP->type_carrierfireratescale;
	if(type == ATT_FLAG_SPEEDSCALE) return TP->type_carrierspeedscale;

	return EntityInsignia::QueryFloat(type);
}

bool EntityFlag::SetInt(int type, int attr)
{
	EntityFlagType *TP = (EntityFlagType*)TypePtr;

	if(type == ATT_WEAPON_STATUS)
	{
		if(AttachedToTankID != attr)
		{
			AttachedToTankID = attr;
			if(AttachedToTankID == 0 && bDropped == false)	// someone just dropped it
			{
				char sOutput[1024];
				sprintf(sOutput, Text.Get(TEXT_FLAG_DROP), CTankGame::Get().GetTeam(iTeamIndex).name.get());
				VW->StatusMessage(sOutput, STATUS_PRI_NETWORK);
				bDropped = true;

				VW->AddEntity("explosphere", TP->type_spawnentity, Position, NULL, NULL, 0, 0, 0, 0);
			}
			else
			{
				bDropped = false;
				// reset drop timer
				iDropTime = 0;

				// Remove the pole
				EntityBase *pole = VW->GetEntity(PoleEntity);
				if(pole)
					pole->Remove();
				PoleEntity = 0;
			}
		}
		if(VW->Net.IsServerActive()){	//Propagate flag drops/pickups to clients
			char b[64];
			b[0] = BYTE_HEAD_MSGENT;
			WLONG(&b[1], GID);
			b[5] = MSG_HEAD_FLAGSTATUS; // 0

			b[6] = 0x0; // 1
			WLONG(&b[7], AttachedToTankID); //2
			if(bDropped) // someone dropped it, need to send accurate position
			{
				WFLOAT(&b[11], Position[0]); // 6
				WFLOAT(&b[15], Position[2]); // 10
				VW->Net.QueueSendClient(CLIENTID_BROADCAST, TM_Ordered, b, 19);
			}
			else
			{
				VW->Net.QueueSendClient(CLIENTID_BROADCAST, TM_Ordered, b, 11);
			}
		}
	}
	return EntityInsignia::SetInt(type, attr);
}

void EntityFlag::DeliverPacket(const unsigned char *data, int len){
	EntityFlagType *TP = (EntityFlagType*)TypePtr;

	if(data)
	{
		switch(data[0])
		{
		case MSG_HEAD_FLAGSTATUS:
			if(len >= 6)
			{
				if(AttachedToTankID != RLONG(&data[2]))
				{
					AttachedToTankID = RLONG(&data[2]);
					if(AttachedToTankID == 0) // dropped
						bDropped = true;
					else
					{
						bDropped = false;
						// Remove the pole
						EntityBase *pole = VW->GetEntity(PoleEntity);
						if(pole)
							pole->RemoveMe = REMOVE_FORCE;
						PoleEntity = 0;
					}
				}
			}
			if(len >= 14)
			{
				Position[0] = RFLOAT(&data[6]);
				Position[2] = RFLOAT(&data[10]);
			}
			break;
		default:
			EntityInsignia::DeliverPacket(data, len);
		}
	}
}

