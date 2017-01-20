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

#include <stdio.h>

#include "BanList.h"

CBanEntry::CBanEntry()
{
	IPAddress.s_addr = 0;
	Expiration = 0;
	next = NULL;
}

CBanEntry::CBanEntry(in_addr IP, long expire)
{
	IPAddress.s_addr = IP.s_addr;
	Expiration = expire;
	next = NULL;
}

CBanList::CBanList()
{
	head = NULL;
}

CBanList::~CBanList()
{
	while(head != NULL)
	{
		CBanEntry *tmp = head;

		head = tmp->next;
		delete tmp;
	}
}

void CBanList::AddBan(in_addr IP, long expire)
{
	CBanEntry *tmp = new CBanEntry(IP, expire);
	tmp->next = head;
	head = tmp;
	time(&tmp->Expiration);
	tmp->Expiration += expire * 60;
}

bool CBanList::IsBanned(in_addr IP)
{
	time_t	CurTime;
	CBanEntry *tmp = head;
	CBanEntry *lastentry = NULL;

	time(&CurTime);
	while (tmp != NULL)
	{
		if (tmp->Expiration < CurTime) // entry expired. remove it from list
		{
			if (lastentry == NULL)
			{
				head = tmp->next;
				delete tmp;
				tmp = head;
			}
			else
			{
				lastentry->next = tmp->next;
				delete tmp;
				tmp = lastentry->next;
			}
		}
		else
		{
			if (tmp->IPAddress.s_addr == IP.s_addr)
				return true;
			lastentry = tmp;
			tmp = tmp->next;
		}
	}
	return false;
}

void CBanList::SaveBanList(char *sFileName)
{
	FILE	*fp = fopen(sFileName, "w");

	if (fp != NULL)
	{
		CBanEntry *tmp = head;
		while (tmp != NULL)
		{
			fprintf(fp, "%d.%d.%d.%d %u\n", tmp->IPAddress.s_addr >> 24, (tmp->IPAddress.s_addr & 0xff0000) >> 16, (tmp->IPAddress.s_addr & 0xff00) >> 8, tmp->IPAddress.s_addr & 0xff, (unsigned int)tmp->Expiration);
			tmp = tmp->next;
		}
		fclose(fp);
	}
}

void CBanList::LoadBanList(char *sFileName)
{
	FILE	*fp = fopen(sFileName, "r");

	if (fp != NULL)
	{
		char	sTmp[255];
		while (fgets(sTmp, 255, fp) != NULL)
		{
			in_addr IP;
			unsigned int expire;
			unsigned int ia, ib, ic, id;

			sscanf(sTmp, "%d.%d.%d.%d %u", &ia, &ib, &ic, &id, &expire);
			IP.s_addr = (ia << 24) | (ib << 16) | (ic << 8) | id;

			CBanEntry *tmp = new CBanEntry(IP, expire);
			tmp->next = head;
			head = tmp;
		}
		fclose(fp);
	}
}
