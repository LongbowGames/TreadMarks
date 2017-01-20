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

#include "PacketProcessors.h"
#include "TankGame.h"
#include "EntityTank.h"
#include "TextLine.hpp"

NetworkError LastDisconnectError = NE_OK;

void TankPacketProcessor::Connect(ClientID source){
	//
	OutputDebugLog("Client ID " + String((int)source) + " Connected.\n", 0);
};
void TankPacketProcessor::Disconnect(ClientID source, NetworkError ne){
	//
	OutputDebugLog("Client ID " + String((int)source) + " Disconnected.\n", 0);
	//
	LastDisconnectError = ne;
};
void TankPacketProcessor::PacketReceived(ClientID source, const char *data, int len){
	if(source != CLIENTID_SERVER){
		if(data[0] == BYTE_HEAD_COMMAND){
			BitUnpackEngine bp(data + 1, len - 1);
			CStr cmd;
			bp.UnpackString(cmd, 7);
			if(CmpLower(cmd, "kill")){
				EntityBase *e = CTankGame::Get().GetVW()->GetEntity(CTankGame::Get().GetVW()->Clients[source].ClientEnt);
				if(e) e->SetInt(ATT_CMD_KILL, 1);
			}
			return;
		}
		if(data[0] == BYTE_HEAD_CONTROLS){
			EntityBase *e = CTankGame::Get().GetVW()->GetEntity(CTankGame::Get().GetVW()->Clients[source].ClientEnt);
			if(e){
				BitUnpackEngine bp(data + 1, len - 1);
				int i = 0;
				float f = 0;
				bp.UnpackFloatInterval(f, -2, 2, 10);  e->SetFloat(ATT_ACCEL, f);//RFLOAT(&data[d])); d += 4;
				bp.UnpackFloatInterval(f, -2, 2, 10);  e->SetFloat(ATT_STEER, f);//RFLOAT(&data[d])); d += 4;
				bp.UnpackUInt(i, 1);  e->SetInt(ATT_FIRE, i);//RLONG(&data[d])); d += 4;
				bp.UnpackFloatInterval(f, -2, 2, 10);
				if(fabsf(f) < 0.1f) f = 0.0f;
				e->SetFloat(ATT_TURRETSTEER, f);//RFLOAT(&data[d])); d += 4;
				//
				i = 0;
				bp.UnpackUInt(i, 1);
				if(i){
					ClassHash insig2 = 0, insig1 = 0;
					bp.UnpackUInt(insig2, 32);
					bp.UnpackUInt(insig1, 32);
					e->SetInt(ATT_INSIGNIA2_HASH, insig2);
					//
					//DONE:  Add team type validation here!  Don't set team id the server won't allow!
					for(i = 0; i < CTankGame::Get().GetNumTeams(); i++){	//Make sure team insignia is on OK list.
						if(CTankGame::Get().GetTeam(i).hash == insig1) break;
					}
					if(i >= CTankGame::Get().GetNumTeams()){	//Team not found in valid list.
						ClassHash th = e->QueryInt(ATT_INSIGNIA1_HASH);
						if(th){
							insig1 = th;	//If player already has a team on server entity, just ignore bad team request.
						}else{
							if(CTankGame::Get().GetVW()->GameMode() & GAMEMODE_TEAMPLAY){
								insig1 = CTankGame::Get().GetTeam(CTankGame::Get().GetTeamFillSpot()).hash;	//Set random team.
							}else{
								insig1 = 0;	//No team play, bad team choice, no team set, so leave team at none.
							}
						}
					}
					//
					if((CTankGame::Get().GetVW()->GameMode() & GAMEMODE_TEAMPLAY) == 0){	//No team play.
						insig1 = 0;
					}
					//
					if(insig1 != e->QueryInt(ATT_INSIGNIA1_HASH)){	//New team.
						EntityTypeBase *et = CTankGame::Get().GetVW()->FindEntityType(insig1);
						if(et){
							char	tmpString[1024];
							sprintf(tmpString, Text.Get(TEXT_PLAYERJOINTEAM), CTankGame::Get().GetVW()->Clients[source].ClientName.get(), et->dname.get());
							CTankGame::Get().GetVW()->StatusMessage(tmpString, STATUS_PRI_NETWORK);
							if(e->QueryInt(ATT_INSIGNIA1_HASH) != 0){	//Only if other team already set, kill player.
								e->SetInt(ATT_CMD_KILL, 0);	//Kill player when switching teams.
							}
						}
					}
					//
					e->SetInt(ATT_INSIGNIA1_HASH, insig1);
				}
				i = 0;
				bp.UnpackUInt(i, 1);	//Chat flag.
				e->SetInt(ATT_CHATTING, i);
			}
			return;
		}
	}
};
void TankPacketProcessor::OutOfBandPacket(sockaddr_in *src, const char *data, int len){
	unsigned int head = 0;
	BitUnpackEngine pe(data, len);
	pe.UnpackUInt(head, 32);
	if(head == LONG_HEAD_HEARTREQ && CTankGame::Get().GetGameState()->ActAsServer && CTankGame::Get().GetVW()->Net.IsServerActive()){
		CTankGame::Get().Heartbeat(src);	//Send a heartbeat to whoever asks for it.
		return;
	}
	if(head == LONG_HEAD_HEARTBEAT && CTankGame::Get().GetServerHead()->CountItems() < 100){	//Accept a heartbeat from a local game server.
		ServerEntry *se = CTankGame::Get().GetServerHead()->NextLink();	//Count server list for a bit of sanity, so someone can't overflow us.  This heartbeat receipt is only for LAN servers.
		if(se) se = se->Find(src);
		if(!se) se = CTankGame::Get().GetServerHead()->InsertObjectAfter(new ServerEntryEx);
		if(se){
			UnpackHeartbeat(pe, src, se);
			CTankGame::Get().FindMapTitle((ServerEntryEx*)se);
		}
		return;
	}
	if(head == LONG_HEAD_PING){
		char b[32];
		WLONG(&b[0], LONG_HEAD_PONG);
		WLONG(&b[4], CTankGame::Get().GetVW()->Time());	//Echo back voxel clock and up to 16 bytes of ping cookie.
		int l = CLAMP(len - 4, 0, 16);
		if(l > 0) memcpy(&b[8], &data[4], l);
		CTankGame::Get().GetVW()->Net.SendOutOfBandPacket(src, b, l + 8);
		return;
	}
	if(head == LONG_HEAD_PONG && len >= 12){	//OOB pongs can only be from a server to a client.
		unsigned int t = RLONG(&data[8]);	//Returned time stamp for when we sent ping.
		for(ServerEntryEx *se = (ServerEntryEx*)CTankGame::Get().GetServerHead()->NextLink(); se; se = (ServerEntryEx*)se->NextLink()){
			if(CmpAddr(se->Address, *src)){	//Found server that ping was for.
				se->PingCount++;
				se->PingTime += (CTankGame::Get().GetVW()->Time() - t);
			}
		}
		return;
	}
};

void OOBPing(sockaddr_in *to){	//Sends an out of band ping using MasterNet object.
	if(to){
		BitPacker<32> pe;
		pe.PackUInt(LONG_HEAD_PING, 32);
		pe.PackUInt(CTankGame::Get().GetVW()->Time(), 32);
		CTankGame::Get().GetMasterNet()->SendOutOfBandPacket(to, (char*)pe.Data(), pe.BytesUsed());
	}
};
//
void MasterClientPacketProcessor::Connect(ClientID source){
	OutputDebugLog("MasterNet: Client ID " + String((int)source) + " Connected.\n", 0);
};
void MasterClientPacketProcessor::Disconnect(ClientID source, NetworkError ne){
	OutputDebugLog("MasterNet: Client ID " + String((int)source) + " Disconnected.\n", 0);
	CTankGame::Get().GetGameState()->MasterError = "";
	switch(ne){
	case NE_Unknown : CTankGame::Get().GetGameState()->MasterError = "Unknown Error"; break;
	case NE_Disconnected : CTankGame::Get().GetGameState()->MasterError = "Disconnected"; break;
	case NE_TimedOut : CTankGame::Get().GetGameState()->MasterError = "Connection Timed Out"; break;
	case NE_ChallengeFailed : CTankGame::Get().GetGameState()->MasterError = "Challenge Failed"; break;
	case NE_ServerFull : CTankGame::Get().GetGameState()->MasterError = "Master Server Full"; break;
	case NE_LookupFailed : CTankGame::Get().GetGameState()->MasterError = "Name Lookup Failed"; break;
	case NE_BindFailed : CTankGame::Get().GetGameState()->MasterError = "Bind Failed"; break;
	}
};
void MasterClientPacketProcessor::PacketReceived(ClientID source, const char *data, int len){
	unsigned int head = 0;
	BitUnpackEngine pe(data, len);
	pe.UnpackUInt(head, 32);
	if(head == LONG_HEAD_HEARTBEATEX && CTankGame::Get().GetServerHead()->CountItems() < 1000){	//Accept a heartbeat from master server.
		unsigned int a = 0, p;
		pe.UnpackUInt(a, 32);
		pe.UnpackUInt(p, 16);
		if(a == 0){	//Null address signals start of dump.
			CTankGame::Get().GetServerHead()->DeleteList();
			CTankGame::Get().GetGameState()->MasterPingIter = 0;
		}else{
			sockaddr_in s;
			s.sin_family = AF_INET;
			s.sin_addr.s_addr = htonl(a);	//HEARTBEATEX packets come from the Master, and include the server's address as data.
			s.sin_port = htons((short)p);
			ServerEntry *se = CTankGame::Get().GetServerHead()->NextLink();
			if(se) se = se->Find(&s);
			if(!se) se = CTankGame::Get().GetServerHead()->InsertObjectAfter(new ServerEntryEx);
			if(se){
				UnpackHeartbeat(pe, &s, se);
				CTankGame::Get().FindMapTitle((ServerEntryEx*)se);
			}
		}
		return;
	}
	//
	//Handle chat comms here.
	if(data[len - 1] == 0){	//Properly null-terminated string.
		CStr str = data;
		//
		//Parse for user list.
		if(Instr(str, "Users on channel") == 0){
			CTankGame::Get().GetUserHead()->DeleteList();
			int i = Instr(str, ":") + 2;
			int j = i;
			while(j < str.len()){
				if(str.chr(j) == ',' || str.chr(j) == '.' && j > i){
					CTankGame::Get().GetUserHead()->AddObject(new CStrLink(Mid(str, i, j - i)));
					i = j + 1;
				}else if(str.chr(j) == ' ') i = j + 1;
				j++;
			}
		}
		//Parse for channel list.
		if(Instr(str, "Channels on server") == 0){
			CTankGame::Get().GetChannelHead()->DeleteList();
			int i = Instr(str, ":") + 2;
			int j = i;
			while(j < str.len()){
				if(str.chr(j) == ',' || str.chr(j) == '.' && j > i){
					CTankGame::Get().GetChannelHead()->AddObject(new CStrLink(Mid(str, i, j - i)));
					i = j + 1;
				}else if(str.chr(j) == ' ') i = j + 1;
				j++;
			}
		}
		//Watch for your current channel status.
		if(Instr(str, "You are joined to channel") == 0){
			int i = Instr(str, "\"") + 1;
			CTankGame::Get().SetCurrentChannel(Mid(str, i, Instr(str, "\"", i) - i));
		}
		if(Instr(str, "You are not joined to a channel") == 0){
			CTankGame::Get().SetCurrentChannel("None");
		}
		//User joined/left channel message.  For now, assume all such messages are for the channel you're on.
		if(Instr(str, "User") == 0){
			if(Instr(str, "has left channel") > 0){
				int i = Instr(str, "\"") + 1;
				CStr user = Mid(str, i, Instr(str, "\"", i) - i);
				for(CStrLink *u = CTankGame::Get().GetUserHead()->NextLink(); u; u = u->NextLink()){
					if(CmpLower(*u, user)){
						u->DeleteItem();	//Find and remove user from users on channel list.
						break;
					}
				}
			}
			if(Instr(str, "has joined channel") > 0){
				int i = Instr(str, "\"") + 1;
				CStr user = Mid(str, i, Instr(str, "\"", i) - i);
				for(CStrLink *u = CTankGame::Get().GetUserHead()->NextLink(); u; u = u->NextLink()){
					if(CmpLower(*u, user)){
						user = "";	//Already here, don't add.
						break;
					}
				}
				if(user.len() > 0) CTankGame::Get().GetUserHead()->InsertObjectAfter(new CStrLink(user));
			}
			if(Instr(str, "changed name to") > 0){
				int i = Instr(str, "\"") + 1;
				CStr user = Mid(str, i, Instr(str, "\"", i) - i);
				i = Instr(str, "\"", i + user.len() + 2) + 1;
				CStr name = Mid(str, i, Instr(str, "\"", i) - i);
				for(CStrLink *u = CTankGame::Get().GetUserHead()->NextLink(); u; u = u->NextLink()){
					if(CmpLower(*u, user)){
						if(name.len() <= 0){
							u->DeleteItem();	//Name changed to null, remove user from list.
						}else{
							u->cpy(name.get());	//Found original name, changing to new name.
						}
						break;
					}
				}
			}
		}
		//
	//	OutputDebugLog("Received chat packet from master: " + str + "\n");
		int i = 0;
		while(i < str.len()){	//Wrap onto multiple lines if need be.
			CTankGame::Get().SetCurChatLine(CTankGame::Get().GetCurChatLine() + 1);
			if(CTankGame::Get().GetCurChatLine() >= MaxChatLines) CTankGame::Get().SetCurChatLine(0);
			int len = CTankGame::Get().GetCurChatLineLen();
			while(len > 0){	//Do word wrap.
				char c = str.chr(i + len);
				if(c == 0 || c == ' ') break;
				len--;
			}
			if(len <= CTankGame::Get().GetCurChatLineLen() / 2) len = CTankGame::Get().GetCurChatLineLen();	//If a word is bigger than half line length, screw it.
			CTankGame::Get().SetCurChatLineText(Mid(str, i, len));
			i += len;
		}
		return;
	}
//	OutputDebugLog("Received bad packet on Master Comms object.\n");
};
