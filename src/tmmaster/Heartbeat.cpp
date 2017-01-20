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


#include "Heartbeat.h"

//Ah yes, for master to client, simply define a super-heartbeat that includes the server address/port before this data.
int UnpackHeartbeat(BitUnpackEngine &pe, sockaddr_in *src, ServerEntry *se){
	if(src && se){
		se->Address.sin_addr = src->sin_addr;
		se->Address.sin_port = src->sin_port;
		pe.UnpackUInt(se->Clients, 8);
		pe.UnpackUInt(se->MaxClients, 8);
		pe.UnpackUInt(se->Rate, 32);
		pe.UnpackUInt(se->GameMode, 32);
		pe.UnpackUInt(se->TimeInGame, 32);
		pe.UnpackString(se->Name, 8);
		pe.UnpackString(se->WebSite, 8);
		pe.UnpackString(se->Info, 8);
		pe.UnpackString(se->Map, 8);
		//
		pe.UnpackUInt(se->version[0], 8);	//Version of server.
		pe.UnpackUInt(se->version[1], 8);
		pe.UnpackUInt(se->version[2], 8);
		pe.UnpackUInt(se->version[3], 8);
		pe.UnpackUInt(se->Bots, 8);	//Bot count.
		pe.UnpackUInt(se->DediFPS, 8);	//Server FPS.
		pe.UnpackUInt(se->TimeLimit, 8);
		pe.UnpackUInt(se->FragLimit, 8);
		pe.UnpackString(se->CorrectedIP, 8);
		//
		se->Time = GetTime();
		return 1;
	}
	return 0;
}
int PackHeartbeat(BitPackEngine &pe, ServerEntry *se){
	if(se){
		pe.PackUInt(se->Clients, 8);
		pe.PackUInt(se->MaxClients, 8);
		pe.PackUInt(se->Rate, 32);
		pe.PackUInt(se->GameMode, 32);
		pe.PackUInt(se->TimeInGame, 32);
		pe.PackString(se->Name, 8);
		pe.PackString(se->WebSite, 8);
		pe.PackString(se->Info, 8);
		pe.PackString(se->Map, 8);
		//
		pe.PackUInt(se->version[0], 8);	//Version of server.
		pe.PackUInt(se->version[1], 8);
		pe.PackUInt(se->version[2], 8);
		pe.PackUInt(se->version[3], 8);
		pe.PackUInt(se->Bots, 8);	//Bot count.
		pe.PackUInt(se->DediFPS, 8);	//Server FPS.
		pe.PackUInt(se->TimeLimit, 8);
		pe.PackUInt(se->FragLimit, 8);
		pe.PackString(se->CorrectedIP, 8);
		//
		return 1;
	}
	return 0;
}

