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


#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "Networking.h"
#include "BitPacking.h"

#define LONG_HEAD_HEARTBEAT		0x92f73a4e
//Packet header for OutOfBand heartbeat from Dedicated Server to Master.
#define LONG_HEAD_HEARTREQ		0x92f73a4f
//Packet header for OutOfBand Request For Heartbeat from client to server, directly.
#define LONG_HEAD_HEARTBEATEX	0x92f73a4d
//Heartbeat with server address/port prepended, for master to client info forwarding.
#define LONG_HEAD_PING			0x29561fea
//Packet header for OutOfBand Ping requests.
#define LONG_HEAD_PONG			0xf327648b
//Packet header for OutOfBand Pong responses.
//

struct ServerEntry : public LinklistBase<ServerEntry> {
public:
	CStr Name, WebSite, Info, Map, CorrectedIP;
	int Clients, MaxClients, Rate, GameMode;
	sockaddr_in Address;
	TimeStamp Time, TimeInGame;
	unsigned int version[4];
	int FragLimit, TimeLimit, Bots, DediFPS;
public:
	ServerEntry(){
		Address.sin_family = AF_INET;
		Name = WebSite = Info = Map = CorrectedIP = "no server";
		Clients = MaxClients = Rate = GameMode = 0;
		Time = TimeInGame = 0;
		version[0] = version[1] = version[2] = version[3] = 0;
		FragLimit = TimeLimit = Bots = DediFPS = 0;
	};
	ServerEntry *Find(sockaddr_in *addr){
		if(addr && CmpAddr(*addr, Address)) return this;
		if(NextLink()) return NextLink()->Find(addr);
		return 0;
	};
	CStr AddressText(){
		return CStr(inet_ntoa(Address.sin_addr)) + ":" + String((int)ntohs(Address.sin_port));
	};
};

int UnpackHeartbeat(BitUnpackEngine &pe, sockaddr_in *src, ServerEntry *se);
int PackHeartbeat(BitPackEngine &pe, ServerEntry *se);

#endif

