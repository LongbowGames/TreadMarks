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

#ifndef __BANLIST_H__
#define __BANLIST_H__

#ifdef WIN32
#include <winsock.h>

// Stupid, stupid windows.h defines.
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include <time.h>

class CBanEntry
{
public:
	CBanEntry();
	CBanEntry(in_addr IP, long expire);

	in_addr		IPAddress;
	time_t		Expiration;
	CBanEntry	*next;
};

class CBanList
{
private:
	CBanEntry	*head;
public:
	CBanList();
	~CBanList();

	void AddBan(in_addr IP, long expire);
	bool IsBanned(in_addr IP);
	void SaveBanList(char *sFileName);
	void LoadBanList(char *sFileName);
};

#endif // __BANLIST_H__
