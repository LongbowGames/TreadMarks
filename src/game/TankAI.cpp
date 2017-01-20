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

#include "TankAI.h"
#include "TankGame.h"
#include "Basis.h"
#include "EntityTank.h"

PowerUpNode* PowerUpQueue::ClosestWeapon()
{
	PowerUpNode *tmp = Head();
	while(tmp) // search all the power ups for a weapon
	{
		if((fabsf(tmp->health) < 0.005f) && (tmp->ammo == 0) && (fabsf(tmp->boltonammo) < 0.005f))
			break;
		tmp = tmp->Next();
	}
	return tmp;
}

PowerUpNode* PowerUpQueue::ClosestHealth()
{
	PowerUpNode *tmp = Head();
	while(tmp) // search all the power ups for a health goodie
	{
		if(tmp->health > 0.005f)
			break;
		tmp = tmp->Next();
	}
	return tmp;
}

PowerUpNode* PowerUpQueue::ClosestAmmo()
{
	PowerUpNode *tmp = Head();
	while(tmp) // search all the power ups for an ammo goodie
	{
		if(tmp->ammo > 0)
			break;
		tmp = tmp->Next();
	}
	return tmp;
}

PowerUpNode* PowerUpQueue::ClosestBoltOnAmmo()
{
	PowerUpNode *tmp = Head();
	while(tmp) // search all the power ups for a bolt on ammo goodie
	{
		if(tmp->boltonammo > 0.005f)
			break;
		tmp = tmp->Next();
	}
	return tmp;
}

CTankAI::CTankAI(EntityRacetank *_Tank, float _Skill)
{
	iAccumThinkInterval = 0;
	iThinkInterval = 2000 - int(1000.0f * _Skill / 100.0f);
	ThisTank = _Tank;
	Enemy = NULL;
	fSkill = _Skill;
	BackUpOK = true;
	iAccumRethinkCountDown = 0;
}

void CTankAI::SortPowerUps(EntityTankGod *egod)
{
	if(!egod) return;

	PowerUpList.Free();

	PowerUp *tmpPowUp = egod->PowHead.NextLink();

    Vec3 Direction;
    Direction[0] = sin(ThisTank->Rotation[1]);
    Direction[1] = 0.0f;
    Direction[2] = cos(ThisTank->Rotation[1]);

	while(tmpPowUp)
	{
		Vec3 tmp;

		SubVec3(tmpPowUp->pos, ThisTank->Position, tmp);
		float slope = fabsf(tmp[1]) / (fabsf(tmp[0]) + fabsf(tmp[2]));
		float Distance = LengthVec3(tmp);
		ScaleVec3(tmp, 1.0f / std::max(0.000001f, Distance)); // Normalize tmp

		float Dot = (DotVec3(tmp, Direction) + 1.0f) * 0.5f;
		float fPriority = (Distance * (1.0f - Dot)) + (Distance * Dot);

		PowerUpList.Add(tmpPowUp->gid, fPriority);
		tmpPowUp = tmpPowUp->NextLink();
	}
}

void CTankAI::SortEnemies(EntityTankGod *egod)
{
	if(!egod) return;

	TargetList.Free();

	Target *tmpTarget = egod->TgtHead.NextLink();

    Vec3 Direction;
    Direction[0] = sin(ThisTank->Rotation[1]);
    Direction[1] = 0.0f;
    Direction[2] = cos(ThisTank->Rotation[1]);

	while(tmpTarget)
	{
		if((tmpTarget->gid != ThisTank->GID) && (tmpTarget->teamid != ThisTank->teamid || ThisTank->teamid == 0) && tmpTarget->targetok)
		{
			Vec3 tmp;

			SubVec3(tmpTarget->pos, ThisTank->Position, tmp);
			float slope = fabsf(tmp[1]) / (fabsf(tmp[0]) + fabsf(tmp[2]));
			float Distance = LengthVec3(tmp);
			ScaleVec3(tmp, 1.0f / std::max(0.000001f, Distance)); // Normalize tmp

			float Dot = (DotVec3(tmp, Direction) + 1.0f) * 0.5f;
			float fPriority = (Distance * (1.0f - Dot)) + (Distance * Dot);

			TargetList.Add(tmpTarget->gid, fPriority);
		}
		tmpTarget = tmpTarget->NextLink();
	}
}

void CTankAI::UpdateDriving(EntityTankGod *egod)
{
	float	ModMaxSpeed;
	float	VelocityAngle;
	Vec3	MoveToVector, MoveToVector2;
	float	dist;
	float	eta;
	float	steerboost;
	float	okcone;
	float	oversteer;
	float	AngleToMoveTo;
	float	AngleToMoveTo2;
	float	anticipate;
	float	anticidist;
	float	DeltaAngleToMoveTo;
	float	ta;
	float	anticicap;
	float	WantSpeed;
	float	ang;
	bool	DriveBackWards = false;

	if(!egod) return;

	ModMaxSpeed = ThisTank->maxspeed;
	if((!ThisTank->playertank) && (CTankGame::Get().GetVW()->GameMode() & GAMEMODE_RACE))
	{
		// find the highest placed human
		int iHighestWayPoint = 0;
		int iThisTankWayPoint;

		if (ThisTank->nextWayPt == 0)
			if(ThisTank->laps == 0)
				iThisTankWayPoint = CTankGame::Get().GetVW()->NumMajorWayPts; // start of race or end of first lap
			else
				iThisTankWayPoint = (ThisTank->laps + 1) * CTankGame::Get().GetVW()->NumMajorWayPts; // end of a lap
		else
			iThisTankWayPoint = ThisTank->laps * CTankGame::Get().GetVW()->NumMajorWayPts + ThisTank->nextWayPt; // middle of a lap

		Target *tmpTarget = egod->TgtHead.NextLink();
		while(tmpTarget)
		{
			if(tmpTarget->Human)
			{
				if (tmpTarget->nextway == 0)
					if(tmpTarget->lap == 0)
						iHighestWayPoint = std::max(0, CTankGame::Get().GetVW()->NumMajorWayPts); // start of race or end of first lap
					else
						iHighestWayPoint = std::max(iHighestWayPoint, (tmpTarget->lap + 1) * CTankGame::Get().GetVW()->NumMajorWayPts); // end of a lap
				else
					iHighestWayPoint = std::max(iHighestWayPoint, tmpTarget->lap * CTankGame::Get().GetVW()->NumMajorWayPts + tmpTarget->nextway); // middle of a lap
			}
			tmpTarget = tmpTarget->NextLink();
		}
		if(iThisTankWayPoint > iHighestWayPoint) // slow down if ahead of highest player
			ModMaxSpeed -= std::min((0.20f * (iThisTankWayPoint - iHighestWayPoint)), 0.6f) * ModMaxSpeed;
	}

	//Generic move to location routine.
	VelocityAngle = atan2f(ThisTank->vh.Vel[0], ThisTank->vh.Vel[2]);
	//Add course wandering.
	SubVec3(ThisTank->ai_moveto, ThisTank->Position, MoveToVector);
	SubVec3(ThisTank->ai_moveto2, ThisTank->Position, MoveToVector2);

	dist = LengthVec3(MoveToVector);

	// Walkies adds some random fudging to the desination
	if(!(CTankGame::Get().GetVW()->FrameFlags & THINK_NOWALKIES))
	{
		float walkies = std::min(dist, 200.0f) * 0.05f;
        Vec3 tw;
        tw[0] = sin((float)(((ThisTank->GID + ThisTank->waypoint + ThisTank->laps * 7) ^ 0x55) & 255)) * walkies;
        tw[1] = 0.0f;
        tw[2] = cos((float)(((ThisTank->GID + ThisTank->waypoint + ThisTank->laps * 7) ^ 0x55) & 255)) * walkies;
		AddVec3(ThisTank->ai_moveto, tw);
		SubVec3(tw, ThisTank->Position, MoveToVector);
		dist = LengthVec3(MoveToVector);
	}

	eta = dist / std::max(1.0f, ThisTank->vh.LinearVelocity);	//Estimated Time of Arrival in seconds.

	okcone = fabsf(atan2f(ThisTank->ai_movetoradius, dist) * 0.50f);

	//Oversteer is actually the amount to FACE past the desired angle to destination.
	oversteer = 1.0 + std::min(1.0f, ThisTank->vh.LinearVelocity / ModMaxSpeed);

	AngleToMoveTo = atan2f(MoveToVector[0], MoveToVector[2]);

	if(fabsf(NormRot(AngleToMoveTo - VelocityAngle)) > okcone && eta < 3.0f)
		steerboost = 3.0f;	//REALLY steer back into line if out of good cone...
	else
		steerboost = 1.0f;

	AngleToMoveTo2 = atan2f(MoveToVector2[0], MoveToVector2[2]);
	DeltaAngleToMoveTo = NormRot(AngleToMoveTo2 - AngleToMoveTo);

	//Add anticipation using MoveTo2 as a blending point
	anticipate = 2.0f;
	anticidist = 50.0f;
	ta = (1.0f - std::min(dist, anticidist) / anticidist) * std::min(1.0f, ThisTank->vh.LinearVelocity / ModMaxSpeed) * anticipate;
	anticicap = okcone;
	AngleToMoveTo = NormRot(AngleToMoveTo + CLAMP(DeltaAngleToMoveTo * std::min(ta, 1.0f), -anticicap, anticicap));	//Start at 50 meters out, full speed and zero distance mean full anticipation.

//	if(ThisTank->vh.LinearVelocity < 10.0f)
		ang = NormRot(AngleToMoveTo - ThisTank->Rotation[1]);
//	else
//	{
//		float a = NormRot(AngleToMoveTo - VelocityAngle) * oversteer;
//		ang = NormRot((VelocityAngle + min(max(-PI * 0.9f, a), PI * 0.9f)) - ThisTank->Rotation[1]);
//	}
	if(BackUpOK)
	{
		if(fabsf(ang) > PIOVER2)
		{
			ang = NormRot(ang + PI); // add 180 degrees to the angle
			DriveBackWards = true;
		}
	}

	ThisTank->Steer = Bias(0.6f, std::min((float)fabsf(ang) * steerboost, 1.0f)) * (ang < 0.0f ? -1.0f : 1.0f);	//Boost up for slower steer at 20hz.

	if(ThisTank->ai_movespeed > 0)
		WantSpeed = std::min(ModMaxSpeed, (float)ThisTank->ai_movespeed + dist * 0.15f);
	else
		WantSpeed = ModMaxSpeed;

	float AccelAtten = std::min(std::max(-1.0f, (WantSpeed - ThisTank->vh.LinearVelocity) * 0.6f), 1.0f);
	ThisTank->Accel = (1.0f - fabsf(ThisTank->Steer)) * AccelAtten * (DriveBackWards ? -1 : 1);
}

void CTankAI::UpdateDestination(EntityTankGod *egod)
{
	if(TargetList.Head())
		ThisTank->targetent = TargetList.Head()->TargetGID;
	if(!egod) return;
	if(CTankGame::Get().GetVW()->GameMode() & GAMEMODE_RACE)
	{
		EntityWaypoint *ew = (EntityWaypoint*)CTankGame::Get().GetVW()->GetEntity(ThisTank->wayentity);
		WaypointNode *waypt = NULL, *wayptmajor = NULL;
		if(ew)
		{
			int newway = ew->WalkWaypoints(ThisTank->waypoint, ThisTank->Position, ThisTank->Velocity, &waypt, &wayptmajor);
			if(waypt)
			{
				CopyVec3(waypt->Pos, ThisTank->ai_moveto);
				ThisTank->ai_movespeed = waypt->flags % 100;
				//Set up following waypoint pos, for anticipating turns.
				WaypointNode *nwaypt = waypt->NextLink();
				if(!nwaypt)
					nwaypt = ew->WaypointByIndex(0);
				if(nwaypt)
					CopyVec3(nwaypt->Pos, ThisTank->ai_moveto2);
				ThisTank->ai_movetoradius = waypt->suggestedradius;
				if(waypt->subid != 0)
					ThisTank->ai_movetoradius *= 2.0f;
			}
		}
		BackUpOK = true;
		// check if weapon is side or rear mount and if so, disable backwards driving
		EntityWeapon *weap = (EntityWeapon*)CTankGame::Get().GetVW()->GetEntity(ThisTank->weaponentity);
		if(weap)
		{
			if(weap->QueryInt(ATT_WHICH_BOLTON) != 1)
				BackUpOK = false;
		}
	}
	else
	{	//Not a race.
		EntityBase *t = egod->ClosestFlag(ThisTank->Position, ThisTank->teamid, ThisTank->FlagEntity != 0);
		if(!t)
		{
			PowerUpNode		*Health = PowerUpList.ClosestHealth();
			PowerUpNode		*Ammo = PowerUpList.ClosestAmmo();
			PowerUpNode		*BoltOnAmmo = PowerUpList.ClosestBoltOnAmmo();
			PowerUpNode		*Weapon = PowerUpList.ClosestWeapon();
			TargetNode		*Enemy = TargetList.Head();

			float HealthPriority = 99999.0f;
			float AmmoPriority = 99999.0f;
			float BoltOnAmmoPriority = 99999.0f;
			float WeaponPriority = 99999.0f;
			float EnemyPriority = 99999.0f;

			if(Health)
				HealthPriority = ThisTank->health * Health->priority * 2.0f;
			if(ThisTank->weaponentity == 0) // we don't have a weapon
			{
				if(Weapon)
					WeaponPriority = Weapon->priority * 0.25; // we want one real bad
				if(Ammo)
					AmmoPriority = float(ThisTank->ammo) * 0.25f * Ammo->priority; // will take pea shooter ammo if we're getting low and its close
				if(Enemy)
					EnemyPriority = Enemy->priority * (1.0f / float(ThisTank->ammo)) * 3.0f; // if we have pea shooter ammo and tank is close go after it
			}
			else // we do have a bolt-on weapon
			{
				if(BoltOnAmmo)
					BoltOnAmmoPriority = BoltOnAmmo->priority * 4.0f; // a hack .. should be looking at bolt-on ammo remaining to determine priority
				if(Enemy)
					EnemyPriority = Enemy->priority;
			}

			int DriveToGID = 0;
			float tmpPriority = 99998.0f; // by making it a bit less than default priorities I avoid a second pointer check
			if(HealthPriority < tmpPriority)
			{
				DriveToGID = Health->PowerUpGID;
				tmpPriority = HealthPriority;
			}
			if(AmmoPriority < tmpPriority)
			{
				DriveToGID = Ammo->PowerUpGID;
				tmpPriority = AmmoPriority;
			}
			if(BoltOnAmmoPriority < tmpPriority)
			{
				DriveToGID = BoltOnAmmo->PowerUpGID;
				tmpPriority = BoltOnAmmoPriority;
			}
			if(WeaponPriority < tmpPriority)
			{
				DriveToGID = Weapon->PowerUpGID;
				tmpPriority = WeaponPriority;
			}
			if(EnemyPriority < tmpPriority)
			{
				DriveToGID = Enemy->TargetGID;
				tmpPriority = EnemyPriority;

				BackUpOK = false; // always face your enemy
			}

			t = CTankGame::Get().GetVW()->GetEntity(DriveToGID);
		}
		if(t){
			ThisTank->ai_movespeed = 0;
			CopyVec3(t->Position, ThisTank->ai_moveto);
			CopyVec3(t->Position, ThisTank->ai_moveto2);
			ThisTank->ai_movetoradius = 100.0f;
		}
	}
}

bool CTankAI::CheckLineOfSight(EntityBase *Enemy)
{
	Vec3 LOSVec;
	float length;

	SubVec3(Enemy->Position, ThisTank->Position, LOSVec);
	length = LengthVec3(LOSVec);
	NormVec3(LOSVec);
	for(float f = 0.1f; f < length; f += 0.1f)
	{
		Vec3	Offset;
		Vec3	TestPos;

		ScaleVec3(LOSVec, f, Offset);
		AddVec3(ThisTank->Position, Offset, TestPos);

		if(CTankGame::Get().GetVW()->Map.FGetI(TestPos[0], -TestPos[2]) > TestPos[1])
			return false;
	}
	return true;
}

bool CTankAI::CheckFire()
{
	EntityBase *e = CTankGame::Get().GetVW()->GetEntity(ThisTank->targetent);
	if(e)
	{
		Vec3 tv;
		SubVec3(e->Position, ThisTank->Position, tv);
		float wanta = atan2f(tv[0], tv[2]);
		float cura = ThisTank->turRot[1] + ThisTank->Rotation[1];
		float va = NormRot(wanta - cura);
		ThisTank->TurretSteer = std::min(std::max(va * 5.0f * (ThisTank->skill * 0.8f + 0.2f), -1.0f), 1.0f);

		EntityBase *ew = CTankGame::Get().GetVW()->GetEntity(ThisTank->weaponentity);
		if(ew)
		{
			float			ModifiedRange = 15.0f; // trying to catch the drill
			CStr			sProjName = ew->QueryString(ATT_WEAPON_PROJECTILE_TYPE);
			EntityTypeBase	*ep = CTankGame::Get().GetVW()->FindEntityType("projectile", sProjName);
			if (ep)
				ModifiedRange = std::max(ModifiedRange, ep->InterrogateFloat("Range") * ThisTank->skill);
			switch(ew->QueryInt(ATT_WHICH_BOLTON))
			{
			case 1: // turret mounted weapon
				{
					float la = LengthVec3(tv);
                    Vec3 tv2;
                    tv[0] = sin(cura) * la;
                    tv[1] = 0;
                    tv[2] = cos(cura) * la;
					if((la < ModifiedRange) && (DistVec3(tv, tv2) < 7.0f * (2.0f - ThisTank->skill)))
						return CheckLineOfSight(e);
					else
						return false;
					break;
				}
			case 2: // side mounted weapon
				{
					float la = LengthVec3(tv);
                    Vec3 tv2;
                    tv2[0] = sin(ThisTank->Rotation[1]) * la;
                    tv2[1] = 0;
                    tv2[2] = cos(cura) * la;
					if((la < ModifiedRange) && (DistVec3(tv, tv2) < 7.0f * (2.0f - ThisTank->skill)))
						return CheckLineOfSight(e);
					else
						return false;
					break;
				}
			case 3: // rear mounted weapon
				if (ThisTank->vh.LinearVelocity > 5.0f)
					return true;
				break;
			default:
				return true;
				break;
			}
		}
		else // basic turret
		{
			float la = LengthVec3(tv);
            Vec3 tv2;
            tv2[0] = sin(cura) * la;
            tv2[1] = 0;
            tv2[2] = cos(cura) * la;
			if(DistVec3(tv, tv2) < 7.0f * (2.0f - ThisTank->skill))
				return CheckLineOfSight(e);
			else
				return false;
		}
	}
	return false;
}

void CTankAI::Think(unsigned int msSinceLastThink)
{
	EntityTankGod *egod = (EntityTankGod*)CTankGame::Get().GetVW()->FindRegisteredEntity("TANKGOD");
	if(!egod)
		return;

	iAccumThinkInterval += msSinceLastThink;

	if(iAccumThinkInterval > iThinkInterval)
	{
		SortPowerUps(egod);
		SortEnemies(egod);
		iAccumThinkInterval = 0;
	}

	if(DistVec3(ThisTank->lastpos1, ThisTank->lastpos2) < 0.5f && DistVec3(ThisTank->lastpos2, ThisTank->lastpos3) < 0.5f && CTankGame::Get().GetVW()->Time() > 10000 && fabsf(ThisTank->Steer) < 0.2f) 	//We is stuck.
	{
		if(iAccumRethinkCountDown == 0)
		{
/*			ThisTank->ai_moveto[0] = ThisTank->Position[0] + (TMrandom() % 20 - 10);
			ThisTank->ai_moveto[1] = ThisTank->Position[1];
			ThisTank->ai_moveto[2] = ThisTank->Position[2] + (TMrandom() % 20 - 10);
*/
			Vec3 DestVec, DestVec2;
			Rot3 tmpRot = {0, 115, 0};
			Mat3 RotMat;

			SubVec3(ThisTank->ai_moveto, ThisTank->Position, DestVec);
			DestVec[1] = 0.0f;
			NormVec3(DestVec);

			Rot3ToMat3(tmpRot, RotMat);
			Vec3MulMat3(DestVec, RotMat, DestVec2);

			ThisTank->ai_moveto[0] = ThisTank->Position[0] + (30 * DestVec2[0]);
			ThisTank->ai_moveto[1] = ThisTank->Position[1];
			ThisTank->ai_moveto[2] = ThisTank->Position[2] + (30 * DestVec2[2]);

			ThisTank->ai_movespeed = 0;
			iAccumRethinkCountDown = 3000;
		}
	}

	iAccumRethinkCountDown = std::max(0, int(iAccumRethinkCountDown - msSinceLastThink));

	if(iAccumRethinkCountDown == 0)
		UpdateDestination(egod);

	UpdateDriving(egod);
	ThisTank->Fire = CheckFire();
}
