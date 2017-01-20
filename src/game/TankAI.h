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

#ifndef __TANKAI_H__
#define __TANKAI_H__

#include "Vector.h"
#include "TankGame.h"

class EntityRacetank;
class EntityTankGod;
struct Target;
struct PowerUp;
class TargetNode
{
private:
	TargetNode	*next;
public:
	int			TargetGID;
	float		priority;

	TargetNode(int _tankID, float _priority) {next = NULL; TargetGID = _tankID; priority = _priority;}
	TargetNode*	Next()
	{
		TargetNode*	tmp = next;
		while(tmp)
		{
			if(CTankGame::Get().GetVW()->GetEntity(tmp->TargetGID) != NULL)
				break;
			tmp = tmp->next;
		}
		return tmp;
	}
	friend class TargetQueue;
};

class PowerUpNode
{
private:
	PowerUpNode		*next;
public:
	int				PowerUpGID;
	float			priority;
	float			health;
	int				ammo;
	float			boltonammo;

	PowerUpNode(int _powerupID, float _priority)
	{
		next = NULL;
		PowerUpGID = _powerupID;
		priority = _priority;
		EntityBase		*w = CTankGame::Get().GetVW()->GetEntity(PowerUpGID);
		if(w)
		{
			EntityTypeBase	*type = w->TypePtr;
			health = type->InterrogateFloat("healthfix");
			ammo = type->InterrogateInt("ammoadd");
			boltonammo = type->InterrogateFloat("boltonammoadd");
		}
		else
		{
			PowerUpGID = 0;
			priority = 999999.0f;
			health = 0;
			ammo = 0;
			boltonammo = 0;
		}
	}
	PowerUpNode*	Next()
	{
		PowerUpNode*	tmp = next;
		while(tmp)
		{
			if(CTankGame::Get().GetVW()->GetEntity(tmp->PowerUpGID) != NULL)
				break;
			tmp = tmp->next;
		}
		return tmp;
	}
	friend class PowerUpQueue;
};

class PowerUpQueue
{
private:
	PowerUpNode	*head;
public:
	PowerUpQueue() {head = NULL;}
	~PowerUpQueue(){Free();}

	void Free()
	{
		while(head)
		{
			PowerUpNode *tmpNode = head->next;
			delete head;
			head = tmpNode;
		}
	}

	void Add(int _powerupID, float _priority)
	{
		PowerUpNode	*tmpNext = head;
		PowerUpNode *tmpPrev = NULL;
		while(tmpNext)
		{
			if(tmpNext->priority > _priority)
			{
				PowerUpNode	*tmpNext2 = new PowerUpNode(_powerupID, _priority);
				tmpNext2->next = tmpNext;
				if(tmpPrev)
					tmpPrev->next = tmpNext2;
				else
					head = tmpNext2;
				return;
			}
			tmpPrev = tmpNext;
			tmpNext = tmpNext->next;
		}
		if(tmpPrev == NULL)
			head = new PowerUpNode(_powerupID, _priority);
		else
			tmpPrev->next =  new PowerUpNode(_powerupID, _priority);
	}

	PowerUpNode* Head()
	{
		PowerUpNode*	tmp = head;
		while(tmp)
		{
			if(CTankGame::Get().GetVW()->GetEntity(tmp->PowerUpGID) != NULL)
				break;
			tmp = tmp->next;
		}
		return tmp;
	}

	PowerUpNode* ClosestWeapon();
	PowerUpNode* ClosestHealth();
	PowerUpNode* ClosestAmmo();
	PowerUpNode* ClosestBoltOnAmmo();
};

class TargetQueue
{
private:
	TargetNode	*head;
public:
	TargetQueue() {head = NULL;}
	~TargetQueue(){ Free();}

	void Free()
	{
		while(head)
		{
			TargetNode *tmpNode = head->next;
			delete head;
			head = tmpNode;
		}
	}

	void Add(int _tankID, float _priority)
	{
		TargetNode	*tmpNext = head;
		TargetNode *tmpPrev = NULL;
		while(tmpNext)
		{
			if(tmpNext->priority > _priority)
			{
				TargetNode	*tmpNext2 = new TargetNode(_tankID, _priority);
				tmpNext2->next = tmpNext;
				if(tmpPrev)
					tmpPrev->next = tmpNext2;
				else
					head = tmpNext2;
				return;
			}
			tmpPrev = tmpNext;
			tmpNext = tmpNext->next;
		}
		if(tmpPrev == NULL)
			head = new TargetNode(_tankID, _priority);
		else
			tmpPrev->next =  new TargetNode(_tankID, _priority);
	}
	TargetNode* Head()
	{
		TargetNode*	tmp = head;
		while(tmp)
		{
			if(CTankGame::Get().GetVW()->GetEntity(tmp->TargetGID) != NULL)
				break;
			tmp = tmp->next;
		}
		return tmp;
	}
};

class CTankAI
{
private:
	unsigned int	iAccumThinkInterval;
	unsigned int	iThinkInterval;
	unsigned int	iAccumRethinkCountDown;
	EntityRacetank	*ThisTank;
	EntityRacetank	*Enemy;
	float			fSkill;
	TargetQueue		TargetList;
	PowerUpQueue	PowerUpList;
	bool			BackUpOK;

	void SortPowerUps(EntityTankGod *egod);
	void SortEnemies(EntityTankGod *egod);
	void UpdateDriving(EntityTankGod *egod);
	void UpdateDestination(EntityTankGod *egod);
	bool CheckLineOfSight(EntityBase *Enemy);
	bool CheckFire();
public:
	CTankAI(EntityRacetank *_Tank, float _Skill);

	void Think(unsigned int msSinceLastThink);
};

#endif // __TANKAI_H__
