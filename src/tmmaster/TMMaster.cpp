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



#include "../game/Networking.h"
#include "../game/CStr.h"
#include "../game/Timer.h"
#include "../game/Trees.h"
#include "../game/CfgParse.h"
#include "TMMaster.h"
#include <stdlib.h>
#include "../game/BitPacking.h"
#include "../game/version.h"
#include <algorithm>

#ifndef WIN32
#include <unistd.h>
inline void Sleep(unsigned int iMilliseconds)
{
    usleep(iMilliseconds*1000);
}
#endif

using namespace std;

const char AppName[] = "Tread Marks Master/Chat Server";
const char VersionString[] = VERSION_STRING;
const char Copyright[] = "Copyright 1999-2017 by Longbow Digital Arts, Incorporated";
const char UsageString[] = "Usage:  tmmaster.exe [-mMaxClients] [-pListenPort] [-sSleepMS] [-tTimeOut]";

#define CLIENT_TIMEOUT 60000
//1 minutes
#define RELIABLE_TIMEOUT 2000
//2 seconds.

#define MAX_CLIENT_RATE 500
#define QUEUE_SIZE_SANITY 10000

#define MAX_CLIENTS 4096
#define MAX_NAME_LEN 32
#define MAX_CHAN_LEN 32
int MasterPort = 12499;
int MaxClients = 1024;
int SleepMS = 1;
int ServerTimeout = 60;

struct ClientEntry{
	CStr Name, Channel;
	int Connected;
	ClientID id;
	void Init(){
		Name = Channel = "";
		Connected = 0;
	};
	ClientEntry(){
		Init();
	};
};
struct ClientEntryPtr : public LinklistBase<ClientEntryPtr> {
	ClientEntry *Client;
	ClientEntryPtr(ClientEntry *c) : Client(c) { };
	ClientEntryPtr() : Client(0) { };
};
struct ChannelEntry : public LinklistBase<ChannelEntry> {
	CStr Name;
	ClientEntryPtr ClientHead;
	ChannelEntry(const char *name) : Name(name) { };
	ChannelEntry(){};
};

int SendToClient(ClientID id, const char *string);
int SendToChannel(ChannelEntry *ch, const char *string);
int SendToChannel(const char *chan, const char *string);
int ChangeChannel(ClientEntry *client, const char *to);

ClientEntry Clients[MAX_CLIENTS];
ChannelEntry ChannelHead;
Network Net;
MasterPacketProcessor MasterProc;
ServerEntry ServerHead;

void Cleanup(){
	Net.ServerDisconnect(CLIENTID_BROADCAST);
	Net.FreeServer();
};

#ifdef WIN32
BOOL WINAPI CtrlCHandler(DWORD ctrl){
	Cleanup();
	ExitProcess(-1);
	return 0;
};
#endif

CStr MMSS(int secs){
	CStr m = String(secs / 60);
	if(m.len() < 2) m = "0" + m;
	CStr s = String(secs % 60);
	if(s.len() < 2) s = "0" + s;
	return m + ":" + s;
};
int main(int argc, char **argv){
	//
#ifdef WIN32
	SetConsoleCtrlHandler(CtrlCHandler, true);
#endif
	//
	for(int i = 0; i < MAX_CLIENTS; i++){
		Clients[i].id = i;	//So pointer to object can still find index id.
	}
	//
	for(int arg = 1; arg < argc; arg++){
		if(Left(argv[arg], 2) == "-m"){
			MaxClients = atoi(Mid(argv[arg], 2));
			MaxClients = min(max(MaxClients, 16), MAX_CLIENTS);
		}
		if(Left(argv[arg], 2) == "-p"){
			MasterPort = atoi(Mid(argv[arg], 2));
		}
		if(Left(argv[arg], 2) == "-s"){
			SleepMS = atoi(Mid(argv[arg], 2));
			SleepMS = min(max(SleepMS, 1), 1000);
		}
		if(Left(argv[arg], 2) == "-t"){
			ServerTimeout = atoi(Mid(argv[arg], 2));
			ServerTimeout = min(max(ServerTimeout, 1), 10000);
		}
	}
	printf(CStr(AppName) + ", version " + VersionString + ".\n");
	printf(CStr(Copyright) + ".\n");
	printf(CStr(UsageString) + "\n\n");
	//
	printf("Initializing Master Server on port " + String(MasterPort) + ".\n");
	printf("Allocating space for " + String(MaxClients) + " concurrent client connections.\n");
	printf("Preparing to sleep for " + String(SleepMS) + " milliseconds between each poll.\n");
	printf("Servers will be removed after " + String(ServerTimeout) + " seconds of inactivity.\n\n");
	//
	if(!Net.Initialize()){
		printf("Error initializing Networking!\n\n");
		return 1;
	}
	if(!Net.InitServer(MasterPort, MaxClients)){
		printf("Error initializing Server!\n\n");
		return 1;
	}
	//
	Net.Configure(CLIENT_TIMEOUT, RELIABLE_TIMEOUT);
	Net.SetServerRate(MAX_CLIENT_RATE);
	Net.SetChallengeKey(TMMASTER_CHALLENGE_KEY);
	//
	printf("Initialization complete, listening to the void...\n\n");
	//
	//
	int quit = 0;
	while(quit == 0){
		Net.ProcessTraffic(&MasterProc);
		//
		//Housekeeping.
		ServerEntry *se = ServerHead.NextLink();
		TimeStamp Now = GetTime();
		while(se){
			if(Now - se->Time > ServerTimeout * 1000){
				printf("Server " + CStr(inet_ntoa(se->Address.sin_addr)) + ",\"" + se->Name + "\" timed out.\n\n");
				ServerEntry *se2 = se;
				se = se->NextLink();
				se2->UnlinkItem();
				delete se2;
			}else{
				se = se->NextLink();
			}
		}
		//Clean up empty channels.
		ChannelEntry *ch = ChannelHead.NextLink();
		while(ch){
			if(ch->ClientHead.NextLink() == 0){
				ChannelEntry *ch2 = ch;
				ch = ch->NextLink();
				ch2->UnlinkItem();
				delete ch2;
			}else{
				ch = ch->NextLink();
			}
		}
		//
		Sleep(SleepMS);
	//	printf("Loop...\n");
	}
	//
	Cleanup();
	//
	return 0;
};

int SendChannelStatus(ClientEntry *client){
	if(client){
		if(client->Channel.len() > 0){
			SendToClient(client->id, "You are joined to channel \"" + client->Channel + "\".");
			return 1;
		}else{
			SendToClient(client->id, "You are not joined to a channel.");
			return 0;
		}
	}
	return 0;
}
int ChangeChannel(ClientEntry *client, const char *to){
	if(client && to){
		int FoundChannel = 0;
		for(ChannelEntry *ch = ChannelHead.NextLink(); ch; ch = ch->NextLink()){
			if(client->Channel.len() > 0 && CmpLower(ch->Name, client->Channel)){	//Found From channel.
				for(ClientEntryPtr *cp = ch->ClientHead.NextLink(); cp; cp = cp->NextLink()){
					if(cp->Client && CmpLower(cp->Client->Name, client->Name)){	//Found entry for client.
						//
						SendToChannel(ch, "User \"" + client->Name + "\" has left channel \"" + ch->Name + "\".");
						//
						cp->UnlinkItem();
						delete cp;	//Remove pointer to this client on this channel.
						//
						//
						break;
					}
				}
			}
			if(!FoundChannel && to[0] && CmpLower(ch->Name, to)){	//Found to channel.
				ch->ClientHead.InsertObjectAfter(new ClientEntryPtr(client));
				FoundChannel = 1;
			}
		}
		if(!FoundChannel){	//No channel found, must add new one.
			if(to[0]){
				ChannelEntry *ch = ChannelHead.InsertObjectAfter(new ChannelEntry(to));
				if(ch){
					ch->ClientHead.InsertObjectAfter(new ClientEntryPtr(client));	//Add back pointer to client from channel.
					client->Channel = to;
				}else{
					client->Channel = "";
		//			return 0;
				}
			}else{
				client->Channel = "";
		//		return 0;
			}
		}else{
			client->Channel = to;
		}
		//
		SendToChannel(client->Channel, "User \"" + client->Name + "\" has joined channel \"" + client->Channel + "\".");
		//
		SendChannelStatus(client);
		//
		return !!(client->Channel.len());	//Booleanize.
	}
	return 0;
};

int SendToClient(ClientID id, const char *string){
	if(string){
		return Net.QueueSendClient(id, TM_Ordered, string, strlen(string) + 1);
	}else return 0;
};
int SendToChannel(ChannelEntry *ch, const char *string){
	if(ch && string){
		int ret = 0;
		for(ClientEntryPtr *cp = ch->ClientHead.NextLink(); cp; cp = cp->NextLink()){
			if(cp->Client){	//Found entry for client.
				ret |= SendToClient(cp->Client->id, string);
			}
		}
		printf("<%s> %s\n", ch->Name.get(), string);
		return ret;
	}
	return 0;
};
int SendToChannel(const char *chan, const char *string){
	if(chan && string){
		for(ChannelEntry *ch = ChannelHead.NextLink(); ch; ch = ch->NextLink()){
			if(CmpLower(ch->Name, chan)){	//Found channel.
				return SendToChannel(ch, string);
			}
		}
	}
	return 0;
};

void MasterPacketProcessor::Connect(ClientID source){
	printf("Connection received on Client ID " + String((int)source) + ".\n\n");
	Clients[source].Connected = 1;
	SendToClient(source, "Welcome to the Tread Marks Master Server!");
}
void MasterPacketProcessor::Disconnect(ClientID source, NetworkError ne){
	printf("Disconnection of Client ID " + String((int)source) + ".\n\n");
	ChangeChannel(&Clients[source], "");
	Clients[source].Init();
}
void MasterPacketProcessor::PacketReceived(ClientID source, const char *data, int len){
//	printf("Packet received from Client ID " + String((int)source) + ".\n\n");
	//
	if(source < 0 || source >= MAX_CLIENTS){
		printf("ClientID out of range!\n\n");
		return;
	}
	//
//	printf("Client Out Queue is: %d\n", Net.ClientQueueSize(source));
	if(Net.ClientQueueSize(source) > QUEUE_SIZE_SANITY) return;
	//Break off if this client has too much data to send backed up, and ignore any new commands.
	//
	//Copy into safe string form.
	static char buf[1024] = {0};
	len = min(len, 1023);
	memcpy(buf, data, len);
	buf[len] = 0;
	//
	if(buf[0] == '/'){	//Command string.
		//
		ChannelEntry *ch;
		ServerEntry *se;
		//
		switch(tolower(buf[1])){
		case 'n' :	//Name.
			int a, b;
			a = Instr(buf, " ");
			b = Instr(buf, " ", a + 1);
			if(a > 1){
				CStr name1, name2;
				if(b <= a) name1 = Mid(buf, a + 1);
				else name1 = Mid(buf, a + 1, (b - a) - 1);
				int addnum = 1, nameok = 1;
				{	//Legalize name.
					char n[MAX_NAME_LEN + 4];
					int nlen = 0;
					for(int i = 0; i < min(MAX_NAME_LEN, name1.len()); i++){
						if(isalnum(name1[i]) || name1[i] == '_') n[nlen++] = name1[i];
					}
					n[nlen] = 0;
					name1 = n;
				}
				name2 = name1;
				do{	//Make sure there are no name collisions, and add numbers if need be.
					nameok = 1;
					for(int c = 0; c < MAX_CLIENTS; c++){
						if(Clients[c].Connected && CmpLower(Clients[c].Name, name2) && Clients[c].Name.len() > 0){
							//Name collision.
							nameok = 0;
							name2 = name1 + String(addnum++);
							break;
						}
					}
				}while(nameok == 0);
				//
				//Should make sure name isn't set to blank when already on a channel...
			//	if(name2.len() <= 0){
			//	}
				//
				if(Clients[source].Name.len() <= 0 && name2.len() > 0){	//Initial name set.
					SendToClient(Clients[source].id, CStr(AppName) + ", Version " + CStr(VersionString) + ",");
				//	SendToClient(Clients[source].id, );
					SendToClient(Clients[source].id, Copyright + CStr("."));
					SendToClient(Clients[source].id, "- - - - - - - - - - - - - - - - - - - - - -");
					SendToClient(Clients[source].id, "This is a public, unmoderated chat server!");
					SendToClient(Clients[source].id, "Neither the author nor the operator of this");
					SendToClient(Clients[source].id, "server can take any responsibility for any");
					SendToClient(Clients[source].id, "content or the bahavior of other users.");
					SendToClient(Clients[source].id, "Please act responsibly!  And have fun!  :)");
					SendToClient(Clients[source].id, "- - - - - - - - - - - - - - - - - - - - - -");
					SendToClient(Clients[source].id, "Version 1.5.9 is now available from");
					SendToClient(Clients[source].id, "www.treadmarks.com. It is required in order");
					SendToClient(Clients[source].id, "to run on most game servers");
					SendToClient(Clients[source].id, "- - - - - - - - - - - - - - - - - - - - - -");
				}
				//
				printf("User " + Clients[source].Name + " changed name to " + name2 + ".\n");
				SendToChannel(Clients[source].Channel, "User \"" + Clients[source].Name + "\" changed name to \"" + name2 + "\"");
				SendToClient(Clients[source].id, "You changed your name to \"" + name2 + "\".");
				if(name2.len() <= 0) ChangeChannel(&Clients[source], "");
				//Kick off channel when unsetting name.
				Clients[source].Name = name2;
			}else{
				if(Clients[source].Name.len() > 0){
					SendToClient(Clients[source].id, "Your name is \"" + Clients[source].Name + "\".");
				}else{
					SendToClient(Clients[source].id, "You do not have a name set.");
				}
			}
			break;
		case 'j' :
			if(Clients[source].Name.len() <= 0){
				printf("Attempted channel change without name being set, " + Clients[source].Name + ".\n\n");
				SendToClient(Clients[source].id, "You must set a name first.");
				break;
			}
			{
				int a, b;
				a = Instr(buf, " ");
				b = Instr(buf, " ", a + 1);
				if(a > 1){
					CStr chan;//, oldchan;
					if(b <= a) chan = Mid(buf, a + 1);
					else chan = Mid(buf, a + 1, (b - a) - 1);
					{	//Legalize channel name.
						char n[MAX_CHAN_LEN + 4];
						int nlen = 0;
						for(int i = 0; i < min(MAX_CHAN_LEN, chan.len()); i++){
							if(isalnum(chan[i]) || chan[i] == '_') n[nlen++] = chan[i];
						}
						n[nlen] = 0;
						chan = n;
					}
				//	oldchan = Clients[source].Channel;
					//
					if(CmpLower(Clients[source].Channel, chan)){	//Er, same channel!
						SendChannelStatus(&Clients[source]);
					}else{
						ChangeChannel(&Clients[source], chan);
					}
				//	SendToChannel(oldchan, "Client \"" + Clients[source].Name + "\" has left channel \"" + oldchan + "\".");
				//	SendToChannel(chan, "Client \"" + Clients[source].Name + "\" has joined channel \"" + chan + "\".");
				}else{
					SendChannelStatus(&Clients[source]);
				}
			}
			break;
		case 'h' :
			SendToClient(Clients[source].id, "/Help : displays this list.");
			SendToClient(Clients[source].id, "/Name <name> : sets client name.");
			SendToClient(Clients[source].id, "/Join <chan> : changes joined channel.");
			SendToClient(Clients[source].id, "/Chan : displays channel list.");
			SendToClient(Clients[source].id, "/Serv : displays game server list.");
			SendToClient(Clients[source].id, "/Msg <name> <message> : sends a private message.");
			SendToClient(Clients[source].id, "/Users : displays list of people on your channel.");
			break;
		case 'u' : {
			for(ch = ChannelHead.NextLink(); ch; ch = ch->NextLink()){
				if(CmpLower(Clients[source].Channel, ch->Name)) break;
			}
			if(ch){
				CStr out;// = "Users on channel: ";
				int cusers = 0;
				for(ClientEntryPtr *cp = ch->ClientHead.NextLink(); cp; cp = cp->NextLink()){
					if(cp->Client){
						out = out + (cusers > 0 ? ", " : "") + cp->Client->Name;
						cusers++;
					}
				}
				out = "Users on channel \"" + ch->Name + "\" (" + String(cusers) + "): " + out + ".";
				SendToClient(Clients[source].id, out);
			}
			break;}
		case 'c' : {
			CStr out;
			int nchans = 0;
			for(ch = ChannelHead.NextLink(); ch; ch = ch->NextLink()){
				if(ch->Name.len() > 0){
					out = out + (nchans > 0 ? ", " : "") + ch->Name;
					nchans++;
				}
			}
			out = "Channels on server (" + String(nchans) + "): " + out + ".";
			SendToClient(Clients[source].id, out);
			break;}
		case 's' :
			SendToClient(Clients[source].id, "Servers (" + String(ServerHead.CountItems(-1)) + "):");
			for(se = ServerHead.NextLink(); se; se = se->NextLink()){
				SendToClient(Clients[source].id, "ADDR: " + CStr(inet_ntoa(se->Address.sin_addr)) + ":" +
					String((int)ntohs(se->Address.sin_port)) + " NAME: " + se->Name + " INFO: " + se->Info +
					" MAP: " + se->Map + " TIME: " + MMSS(se->TimeInGame / 1000) + " WEB: " + se->WebSite + 
					" PLAYERS: " + String(se->Clients) + "/" + String(se->MaxClients) +
					" RATE: " + String(se->Rate) );
			}
			break;
		case 'z' :	//Machine-readable server list.
			{
				BitPacker<1024> pe;
				pe.PackUInt(LONG_HEAD_HEARTBEATEX, 32);
				pe.PackUInt(0, 32);
				pe.PackUInt(0, 16);	//null address and port signifies start of server dump.
				Net.QueueSendClient(Clients[source].id, TM_Ordered, (char*)pe.Data(), pe.BytesUsed());
				//
				for(se = ServerHead.NextLink(); se; se = se->NextLink()){

					auto correctedIP = inet_addr(se->CorrectedIP);					

					pe.Reset();
					pe.PackUInt(LONG_HEAD_HEARTBEATEX, 32);
					pe.PackUInt(ntohl(correctedIP == INADDR_NONE ? se->Address.sin_addr.s_addr : correctedIP), 32);
					pe.PackUInt(ntohs(se->Address.sin_port), 16);
					PackHeartbeat(pe, se);
					Net.QueueSendClient(Clients[source].id, TM_Ordered, (char*)pe.Data(), pe.BytesUsed());
				}
			}
			break;
		case 'm' :
			if(Clients[source].Name.len() > 0){
				int a, b;
				a = Instr(buf, " ");
				b = Instr(buf, " ", a + 1);
				if(a > 1 && b > a){
					CStr to, msg;//, oldchan;
				//	if(b <= a) chan = Mid(buf, a + 1);
					to = Mid(buf, a + 1, (b - a) - 1);
					msg = Mid(buf, b + 1);
					int sent = 0;
					for(int c = 0; c < MaxClients; c++){
						if(Clients[c].Connected && CmpLower(Clients[c].Name, to)){
							SendToClient(c, "Msg from \"" + Clients[source].Name + "\" on channel \"" +
								Clients[source].Channel + "\": " + msg);
							SendToClient(Clients[source].id, "Msg sent to \"" + Clients[c].Name + "\" on channel \"" +
								Clients[c].Channel + "\": " + msg);
							sent = 1;
							break;
						}
					}
					if(sent == 0){
						SendToClient(Clients[source].id, "Could not find user \"" + to + "\", msg not sent.");
					}
				}
			}else{
				SendToClient(Clients[source].id, "You must set a name first.");
			}
			break;
		case 'i' :	//Master Info.  Undocumented.
			{
				int totalc = 0, chatc = 0;
				for(int i = 0; i < MaxClients; i++){
					if(Clients[i].Connected){
						totalc++;
						if(Clients[i].Name.len() > 0) chatc++;
					}
				}
				SendToClient(Clients[source].id, "Clients connected to Master: (" + String(totalc) + ")");
				SendToClient(Clients[source].id, "Users in Chat: (" + String(chatc) + ")");
			}
			break;
		}
	}else{	//Chat string.
		if(Clients[source].Name.len() > 0 && Clients[source].Channel.len() > 0){
			if(strlen(buf) > 0 && buf[0] != '\n' && buf[0] != '\r'){
				SendToChannel(Clients[source].Channel, "[" + Clients[source].Name + "]: " + buf);
			}
		}else{
			printf("Client ID " + String((int)source) + " with no channel or with no name says: \"" + CStr(buf) + "\".\n\n");
			SendToClient(Clients[source].id, "You must set a Name and Join a Channel.  Type /Help.");
		}
	}
}

void MasterPacketProcessor::OutOfBandPacket(sockaddr_in *src, const char *data, int len){
//	printf("OutOfBand Packet received.\n");
	BitUnpackEngine pe(data, len);
	unsigned int head = 0;
	pe.UnpackUInt(head, 32);
	if(head == LONG_HEAD_HEARTBEAT){	//Refresh server in list, or add to list.
	//	printf("Heartbeat packet...\n");
		ServerEntry *se = ServerHead.NextLink();
		if(se) se = se->Find(src);
		if(!se) se = ServerHead.InsertObjectAfter(new ServerEntry);
		if(se){
			UnpackHeartbeat(pe, src, se);
		//	printf("ServerEntry added...\n");
			printf("Received Heartbeat from " + CStr(inet_ntoa(src->sin_addr)) + ":" + 
				String((int)ntohs(src->sin_port)) + 
				", \"" + se->Name + "\", \"" + se->WebSite + "\", \"" + se->Info + "\".\n");
		}
	}else{
		printf("Unknown OutOfBand Packet, header == 0x%x, sender == %s.\n\n", head, inet_ntoa(src->sin_addr));
	}
}


