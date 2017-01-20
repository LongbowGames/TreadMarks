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

//
// UDP based networking class, by Seumas McNally,
//


#include "Networking.h"
#include "Timer.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <cstring>
#include <algorithm>

using namespace std;

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#define closesocket close
#endif

TimeStamp MS = 0;

TimeStamp GetMS(){
	return MS;
}

void SetMS(){
	MS = Timer::GetClock();
}

TimeStamp GetTime(){
	return Timer::GetClock();
}

//MiniPacket, holds actual low-overhead mini-packets of data en route to or from a host.
//Transmission will aggregate them into larger UDP packets.

MiniPacket::MiniPacket(){
	Ident = 0;
	LastSent = 0;
	Length = 0;
	Data = 0;
	Acked = 0;
	Priority = 0;
}
MiniPacket::MiniPacket(const char *data, int length, PacketID id, int pri){
	Ident = 0;
	LastSent = 0;
	Length = 0;
	Data = 0;
	Acked = 0;
	Priority = 0;
	Init(data, length, id, pri);
}
MiniPacket::~MiniPacket(){
	Free();
}
void MiniPacket::Free(){
	if(Data) free(Data);
	Data = 0;
	Length = 0;
}
bool MiniPacket::Init(const char *data, int length, PacketID id, int pri){
	Free();
	LastSent = 0;
	Ident = id;
	Acked = 0;
	Priority = pri;
	if(data && length > 0 && (Data = (char*)malloc(length))){
		memcpy(Data, data, length);
		Length = length;
		return true;
	}
	return false;
}
//Usually called on a Head object, will compare priorities with Next object and
//put highest priority object closest to head.
MiniPacket *MiniPacket::InsertAfterPriority(MiniPacket *mp){
	if(mp){
		if(Next && Priority < Next->Priority){
			return Next->InsertAfterPriority(mp);
		}else{
			return InsertObjectAfter(mp);
		}
	}
	return 0;
}

//ClientConnection, holds all the state required for one side of a two-ended communication.
//Contains lists of mini-packets for outgoing transmission and incoming delivery for
//unreliable, reliable, and ordered reliable channels.

ClientConnection::ClientConnection(){
	Initialize();
}
void ClientConnection::Initialize(){
	CCS = CCS_None;
	Address.sin_family = AF_INET;
	Address.sin_addr.s_addr = 0;
	Address.sin_port = 0;
	LastRecvTime = LastSendTime = 0;
	NextOrderedToDeliver = 0;
	NextReliableID = 0;
	NextOrderedID = 0;
	ByteRate = 2500;
	//
	RateHistIdx = 0;
	for(int i = 0; i < RATE_WINDOW_SIZE; i++){
		RateHistBytes[i] = 0;
		RateHistTime[i] = 0;
	}
	//
	HighSet = -1;	//Causes first set access to initialize it.
	//
	OutUnreliableHead.DeleteList();
	OutReliableHead.DeleteList();
	OutOrderedHead.DeleteList();
	InUnreliableHead.DeleteList();
	InReliableHead.DeleteList();
	InOrderedHead.DeleteList();
}
ClientConnection::~ClientConnection(){
	//Lists should delete selves.
}
bool ClientConnection::QueueOutPacket(TransmissionMode mode, const char *data, int length, int pri){
	if(data && length > 0){
		switch(mode){
		case TM_Unreliable : OutUnreliableHead.InsertAfterPriority(new MiniPacket(data, length, 0, pri)); break;
		case TM_Reliable : OutReliableHead.InsertAfterPriority(new MiniPacket(data, length, NextReliableID++, pri)); break;
		case TM_Ordered : OutOrderedHead.InsertObjectAfter(new MiniPacket(data, length, NextOrderedID++, pri)); break;
		}	//Priorities are meaningless (and bad!!!) for Ordered packets!
		return true;
	}
	return false;
}
bool ClientConnection::QueueInPacket(TransmissionMode mode, const char *data, int length, PacketID id){
	if(data && length > 0){
		switch(mode){
		case TM_Unreliable : InUnreliableHead.InsertObjectAfter(new MiniPacket(data, length, id)); break;
		case TM_Reliable : InReliableHead.InsertObjectAfter(new MiniPacket(data, length, id)); break;
		case TM_Ordered :
			MiniPacket *mp;	//Check for dupes.  Not needed on Reliable since they are delivered immediately; different mechanism there.
			for(mp = InOrderedHead.NextLink(); mp; mp = mp->NextLink()){
				if(mp->Ident == id){
					mp->Acked = 0;
					return true;	//Set so it's acked again, then discard incoming packet.
				}
			}
			InOrderedHead.InsertObjectAfter(new MiniPacket(data, length, id));
			break;
		}
		return true;
	}
	return false;
}
unsigned int ClientConnection::BytesQueuedOut(unsigned int *packets){	//Calculates the number of bytes, and optionally packets, queued for in and out transport.  May be slow if queues are big.
	unsigned int packs = 0, bytes = 0;
	MiniPacket *mp;
	for(mp = OutUnreliableHead.NextLink(); mp; mp = mp->NextLink()){ bytes += mp->Length; packs++; }
	for(mp = OutReliableHead.NextLink(); mp; mp = mp->NextLink()){ bytes += mp->Length; packs++; }
	for(mp = OutOrderedHead.NextLink(); mp; mp = mp->NextLink()){ bytes += mp->Length; packs++; }
	if(packets) *packets = packs;
	return bytes;
}
unsigned int ClientConnection::BytesQueuedIn(unsigned int *packets){
	unsigned int packs = 0, bytes = 0;
	MiniPacket *mp;
	for(mp = InUnreliableHead.NextLink(); mp; mp = mp->NextLink()){ bytes += mp->Length; packs++; }
	for(mp = InReliableHead.NextLink(); mp; mp = mp->NextLink()){ bytes += mp->Length; packs++; }
	for(mp = InOrderedHead.NextLink(); mp; mp = mp->NextLink()){ bytes += mp->Length; packs++; }
	if(packets) *packets = packs;
	return bytes;
}

////////////////////////////////////////////////////////////////
//Network object, handles connections and transfer.
////////////////////////////////////////////////////////////////

#define MAX_PACKET 990
#define PACKET_HEAD_CONNECT			0xefefefef
#define PACKET_HEAD_CHALLENGE		0xcfcfcfcf
#define PACKET_HEAD_CHALLENGEREPLY	0xcacacaca
#define PACKET_HEAD_NORMAL			0xafafafaf
#define PACKET_HEAD_DISCONNECT		0xdcdcdcdc
#define PACKET_HEAD_OUTOFBAND		0x0b0b0b0b

Network::Network(){
	Initialized = ServerActive = ClientActive = false;
	ReliableTimeout = 1000;
	ConnectionTimeout = 60000;
	ConnectTimeout = 15000;
	Clients = NULL;
	MaxClients = 0;
	Sock = INVALID_SOCKET;
	ResetByteCounters();
	MaxServerClientRate = 10000;
	ChallengeKey = 0x55555555;
}
Network::~Network(){
	Free();
}
bool Network::Initialize(){
	Free();
#ifdef WIN32
	WORD WantVer = MAKEWORD(1, 1);
	if(WSAStartup(WantVer, &WsaData) != 0){
		return false;
	}
	if(WsaData.wVersion != WantVer){
		WSACleanup();
		return false;
	}
#endif
	Initialized = true;
printf("Networking initialized.\n");
	return true;
}
bool Network::InitSocket(){
	FreeSocket();
	//
	Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(Sock == INVALID_SOCKET){// || ServerSock == INVALID_SOCKET){
		FreeSocket();
printf("Error creating socket.\n");
		return false;
	}
	unsigned long yes = 1;
	#ifdef WIN32
		ioctlsocket(Sock, FIONBIO, &yes);
	#else
		fcntl(Sock, F_SETFL, O_NONBLOCK);
	#endif
	setsockopt(Sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
	//Enable Broadcast sends so we can hunt down LAN servers.
	//
	return true;
}
void Network::Free(){
	//Free winsock...
printf("Freeing networking.\n");
	if(Initialized){
		FreeSocket();
		#ifdef WIN32
            WSACleanup();
		#endif
		Initialized = false;
	}
	FreeServer();
}
bool Network::FreeSocket(){
	if(Sock != INVALID_SOCKET){
		closesocket(Sock);
		Sock = INVALID_SOCKET;
		return true;
	}
	return false;
}
bool Network::InitServer(short port, int maxconn, int retrybind){
	if(!Initialized) return false;
	FreeServer();
	InitSocket();
	if(Sock != INVALID_SOCKET && port > 0 && maxconn > 0){
		sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
		while(SOCKET_ERROR == bind(Sock, (sockaddr*)&sin, sizeof(sin)) && retrybind >= 0){
printf("Error binding socket.\n");
			sin.sin_port = htons(++port);	//Try next port.
			retrybind--;
			if(retrybind < 0) return false;
		}
		Clients = new ClientConnection[maxconn];
		if(!Clients) return false;
		MaxClients = maxconn;
		ServerActive = true;
printf("Server initialized.\n");
		return true;
	}
	return false;
}
void Network::FreeServer(){
	FreeSocket();
	if(Clients) delete [] Clients;
	Clients = NULL;
	MaxClients = 0;
	ServerActive = false;
}
//
//TODO: Sockets can't be re-bound, so I really should kill and recreate the socket each time the
//port used might change.
//Done.  Note that port for client socket can be different from that of server socket, or undefined.
//
bool Network::InitClient(short port, int retrybind){
	if(!Initialized) return false;
	FreeClient();
	InitSocket();
	if(Sock != INVALID_SOCKET && port > 0){
		Server.Initialize();	//Connection to server.
		sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
		if(port != 0){
			while(SOCKET_ERROR == bind(Sock, (sockaddr*)&sin, sizeof(sin)) && retrybind >= 0){
printf("Error binding socket.\n");
				sin.sin_port = htons(++port);
				retrybind--;
				if(retrybind < 0) return false;
			}
		}
		ClientActive = true;
		return true;
	}
	return false;
}
void Network::FreeClient(){
	FreeSocket();
	Server.Initialize();
	ClientActive = false;
}
bool Network::Configure(unsigned int conntimeout, unsigned int reltimeout, unsigned int newconntimeout){
	ConnectionTimeout = conntimeout;
	ReliableTimeout = reltimeout;
	ConnectTimeout = newconntimeout;
	return true;
}
bool Network::ClientConnect(const char *name, short port){
	ConnectTo = name;	//Parse this later...  Done!  addr:port.
	ConnectToPort = port;
	return true;
}
//
#define DISCONNECT_FLOOD 16
//Number of disconnect packets to flood to other end to mostly guarantee reception.
//
bool Network::ClientDisconnect(PacketProcessorCallback *callback, NetworkError reason){
	if(ClientActive){
		if(Server.CCS != CCS_None){
			char buf[16];
			WLONG(&buf[0], PACKET_HEAD_DISCONNECT);
			WLONG(&buf[4], Server.ChallengeValue);
			WLONG(&buf[8], reason);
			for(int i = 0; i < DISCONNECT_FLOOD; i++){
				sendto(Sock, buf, 12, 0, (sockaddr*)&Server.Address, sizeof(sockaddr_in));
				//Send immediately, since we may be called from last minute shut-down.  Also avoid rate controls.
			}
			Server.Initialize();
			if(callback) callback->Disconnect(CLIENTID_SERVER, reason);	//Notify app.
			//Drop connection.
			printf("Sent disconnect to server.\n");
			return true;
		}
	}
	return false;
}
bool Network::ServerDisconnect(ClientID client, PacketProcessorCallback *callback, NetworkError reason){
	if(ServerActive && Clients){
		for(int c = 0; c < MaxClients; c++){
			if(c == client || client == CLIENTID_BROADCAST){
				if(Clients[c].CCS != CCS_None){
					char buf[16];
					WLONG(&buf[0], PACKET_HEAD_DISCONNECT);
					WLONG(&buf[4], Clients[c].ChallengeValue);
					WLONG(&buf[8], reason);
					for(int i = 0; i < DISCONNECT_FLOOD; i++){
						sendto(Sock, buf, 12, 0, (sockaddr*)&Clients[c].Address, sizeof(sockaddr_in));
						//Send immediately, since we may be called from last minute shut-down.  Also avoid rate controls.
					}
					Clients[c].Initialize();
					if(callback) callback->Disconnect(c, reason);	//Notify app.
					//Drop connection.
					printf("Sent disconnect to client.\n");
				}
			}
		}
		return true;
	}
	return false;
}

bool Network::QueueSendServer(TransmissionMode mode, const char *data, int len, int pri){
	if(data && len > 0 && Server.CCS == CCS_Connected){
		return Server.QueueOutPacket(mode, data, len, pri);
	}
	return false;
}
bool Network::QueueSendClient(ClientID client, TransmissionMode mode, const char *data, int len, int pri){
	if(Clients && ((client >= 0 && client < MaxClients) || client == CLIENTID_BROADCAST) && data && len > 0){
		for(int c = 0; c < MaxClients; c++){
			if(c == client || client == CLIENTID_BROADCAST){
				if(Clients[c].CCS == CCS_Connected){
					Clients[c].QueueOutPacket(mode, data, len, pri);
				}
			}
		}
		return true;
	}
	return false;
}
//
bool Network::SetClientRate(ClientID client, unsigned int rate){
	if(Clients && ((client >= 0 && client < MaxClients) || client == CLIENTID_BROADCAST)){
		for(int c = 0; c < MaxClients; c++){
			if(c == client || client == CLIENTID_BROADCAST){
				Clients[c].ByteRate = std::max(500, std::min(int(rate), MaxServerClientRate));
			}
		}
		return true;
	}
	return false;
}
unsigned int Network::GetClientRate(ClientID client){
	if(Clients && ((client >= 0 && client < MaxClients))){
		return Clients[client].ByteRate;
	}
	return 0;
}
bool Network::SetServerRate(unsigned int rate){
	MaxServerClientRate = std::max(500, std::min(int(rate), 10000));
	if(Clients && MaxClients > 0){
		for(int c = 0; c < MaxClients; c++){
			Clients[c].ByteRate = std::min(int(Clients[c].ByteRate), MaxServerClientRate);
		}
	}
	return true;
}
unsigned int Network::GetServerRate(){
	return MaxServerClientRate;
}
//
unsigned int Network::TotalBytesOut(){
	return bytesout;
}
unsigned int Network::TotalBytesIn(){	//Statistic tracking functions.
	return bytesin;
}
void Network::ResetByteCounters(){
	bytesin = 0;
	bytesout = 0;
}
bool Network::GetServerInByteCounts(unsigned int *ubytes, unsigned int *rbytes, unsigned int *obytes){	//Gets the byte counts for individual packet types received, but not necessarily delivered, from the server to us as a client.  This can show if we are being flooded with extraneous Reliable or Ordered packets.
	if(ubytes) *ubytes = Server.UnreliableBytesIn;
	if(rbytes) *rbytes = Server.ReliableBytesIn;
	if(obytes) *obytes = Server.OrderedBytesIn;
	return true;
}
void Network::ResetServerInByteCounts(){	//Resets above counters.
	Server.UnreliableBytesIn = 0;
	Server.ReliableBytesIn = 0;
	Server.OrderedBytesIn = 0;
}
void Network::SetChallengeKey(unsigned int key){
	ChallengeKey = key;
}
unsigned int Network::ClientQueueSize(ClientID client){	//Returns the number of bytes queued out to a specific client but not yet sent or delivered.
	if(Clients && ((client >= 0 && client < MaxClients))){
		return Clients[client].BytesQueuedOut();
	}
	return 0;
}
unsigned int Network::ServerQueueSize(){	//Returns bytes queued for sending to the server, but not yet sent or delivered.
	return Server.BytesQueuedOut();
}
ClientConnectionStatus Network::ConnectionStatus(ClientID who){
	if(who == CLIENTID_SERVER && ClientActive){
		return Server.CCS;
	}else{
		if(ServerActive && Clients && who >= 0 && who < MaxClients) return Clients[who].CCS;
	}
	return CCS_None;
}
int Network::SendTo(int s, char *buf, int len, int flag, const sockaddr *addr, int addrlen){
	//Optionally add packet loss and lag simulation here.
	if(1){//rand() % 2 == 1){
		int r = sendto(s, buf, len, flag, addr, addrlen);
		if(r != SOCKET_ERROR && r > 0) bytesout += (unsigned int)r;
		return r;
	}else{
		return 0;
	}
}
int Network::RecvFrom(int s, char *buf, int len, int flag, sockaddr *addr, socklen_t *addrlen){
	//Optionally add packet loss and lag simulation here.
	if(1){//rand() % 2 == 1){
		int r = recvfrom(s, buf, len, flag, addr, addrlen);
		if(r != SOCKET_ERROR && r > 0) bytesin += (unsigned int)r;
		return r;
	}else{
		return 0;
	}
}
//
//
//inline int CmpAddr(const SOCKADDR_IN &a1, const SOCKADDR_IN &a2){
//	if(a1.sin_addr.s_addr == a2.sin_addr.s_addr && a1.sin_port == a2.sin_port) return 1;
//	else return 0;
//}

bool Network::SendOutOfBandPacket(sockaddr_in *dest, const char *data, int len){
	if((ServerActive || ClientActive) && dest && data && len > 0){
		char senbuf[MAX_PACKET];
		WLONG(&senbuf[0], PACKET_HEAD_OUTOFBAND);
		len = std::min(len, MAX_PACKET - 4);
		memcpy(&senbuf[4], data, len);
		return (SOCKET_ERROR != SendTo(Sock, senbuf, len + 4, 0, (sockaddr*)dest, sizeof(sockaddr_in))) ? true : false;
	}
	return false;
}

int Network::HTTPGet(sockaddr_in *dest, const char *file, char *buffer, int len){
	if(dest && file && buffer && len > 0 && Initialized){
		unsigned int httpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(httpsock != INVALID_SOCKET){
			sockaddr_in sin;
			sin = *dest;
			sin.sin_family = AF_INET;
			sin.sin_port = htons(80);
			if(SOCKET_ERROR != connect(httpsock, (sockaddr*)&sin, sizeof(sin))){
				CStr get = "GET " + CStr(file) + " HTTP/1.0\r\n\r\n";
				if(SOCKET_ERROR != send(httpsock, get.get(), get.len(), 0)){
					int rec = 0, r = 1;
					while(rec < len && r != 0 && r != SOCKET_ERROR){
						r = recv(httpsock, &buffer[rec], len - rec, 0);
						if(r > 0 && r != SOCKET_ERROR) rec += r;
					}
					closesocket(httpsock);
					return rec;
				}
			}
			closesocket(httpsock);
		}
	}
	return 0;
}

void Network::GetClientIPString (int id,  char *szName)
{
	char *ptr = (char*)&Clients[id].Address.sin_addr;
	int a = ptr[0],b = ptr[1],c = ptr[2],d = ptr[3];
	sprintf(szName,"%d.%d.%d.%d",a,b,c,d);
}

bool Network::ProcessTraffic(PacketProcessorCallback *callback){
	char recbuf[MAX_PACKET];
	char senbuf[MAX_PACKET];
	sockaddr_in sin;
	socklen_t sinlen = sizeof(sin);
	//
	SetMS();
	//
	//Process incoming packets.
	//
	int cont = 1, ret;
	while(cont){	//Suck pipe dry.
		ret = RecvFrom(Sock, recbuf, MAX_PACKET, 0, (sockaddr*)&sin, &sinlen);
		if(ret == 0 || ret == SOCKET_ERROR){
			cont = 0;
		}else{
//			printf("Received something.\n");
			if(ret >= 4 && ServerActive && Clients){
				if(RLONG(&recbuf[0]) == PACKET_HEAD_CONNECT){
					int c, ok = 1;
					for(c = 0; c < MaxClients; c++){
						if(CmpAddr(sin, Clients[c].Address) && Clients[c].CCS != CCS_None){
							ok = 0;
							printf("Connecting client already exists.\n");
							break;	//Client already being dealt with.
						}
						if(Bans.IsBanned(sin.sin_addr))
						{
							ok = 0;
							printf("Connecting client is banned.\n");
							break;
						}
					}
					if(ok){
						for(c = 0; c < MaxClients; c++){
							if(Clients[c].CCS == CCS_None){	//We have an open slot.
								Clients[c].CCS = CCS_InSendChallenge;
								Clients[c].Address.sin_addr.s_addr = sin.sin_addr.s_addr;//htonl(RLONG(&recbuf[4]));
								Clients[c].Address.sin_port = sin.sin_port;//htons((short)RLONG(&recbuf[8]));
								Clients[c].ChallengeValue = rand() * rand() + rand();
								Clients[c].LastRecvTime = GetMS();
								printf("Set send challenge state.\n");
								break;
							}
						}
					}
				}
				if(RLONG(&recbuf[0]) == PACKET_HEAD_CHALLENGEREPLY){
					for(int c = 0; c < MaxClients; c++){
						if(Clients[c].CCS == CCS_InSendChallenge && CmpAddr(sin, Clients[c].Address)){
							if((RLONG(&recbuf[4]) ^ ChallengeKey) == Clients[c].ChallengeValue){	//Successful challenge.
								Clients[c].CCS = CCS_Connected;
								Clients[c].LastRecvTime = GetMS();
								if(callback) callback->Connect(c);	//Notify app.
								printf("Successful challenge reply, we're connected!\n");
								break;
							}else{
							////	Clients[c].CCS = CCS_None;
							//	Clients[c].Initialize();
							//	if(callback) callback->Disconnect(c, NE_ChallengeFailed);	//Notify app.
								ServerDisconnect(c, callback, NE_ChallengeFailed);	//Notify server app and client of challenge failure.
								printf("Unsuccessful challenge reply!\n");
								break;
							}
						}
					}
				}
				if(ret >= 8 && RLONG(&recbuf[0]) == PACKET_HEAD_DISCONNECT){
					for(int c = 0; c < MaxClients; c++){
						if(CmpAddr(sin, Clients[c].Address) && Clients[c].CCS != CCS_None){
							if((RLONG(&recbuf[4]) ^ ChallengeKey) == Clients[c].ChallengeValue){
								NetworkError ne = NE_Disconnected;
								if(ret >= 12) ne = (NetworkError)RLONG(&recbuf[8]);
								Clients[c].Initialize();
								if(callback) callback->Disconnect(c, ne);	//Notify app.
								printf("Client requested disconnection.\n");
								break;
							}
						}
					}
				}
			}
			if(ret >= 4 && ClientActive){
				if(RLONG(&recbuf[0]) == PACKET_HEAD_CHALLENGE && Server.CCS == CCS_OutRequestConnect){
					Server.CCS = CCS_OutReplyChallenge;
					Server.ChallengeValue = RLONG(&recbuf[4]) ^ ChallengeKey;	//Client stores ChallengeValue XOR Challenge Key, server stores ChallengeValue only.  Each end XORs with Key on receipt, to confirm validity.  Client always sends one version of value (XOR), server another (nonXOR).
					Server.LastRecvTime = GetMS();
					printf("Set challenge reply state.\n");
				}
				if(Server.CCS == CCS_OutReplyChallenge && RLONG(&recbuf[0]) == PACKET_HEAD_NORMAL){
					//Server has started talking, we're in!
					Server.CCS = CCS_Connected;
					if(callback) callback->Connect(CLIENTID_SERVER);	//Notify app.
					printf("Client connected!\n");
				}
				if(ret >= 8 && RLONG(&recbuf[0]) == PACKET_HEAD_DISCONNECT){
					if((RLONG(&recbuf[4]) ^ ChallengeKey) == Server.ChallengeValue){
						NetworkError ne = NE_Disconnected;
						if(ret >= 12) ne = (NetworkError)RLONG(&recbuf[8]);
						//Server has told us to disconnect.
					//	ClientDisconnect(CLIENTID_SERVER, ne);	//Whoops, would cause loops...
						Server.Initialize();
						if(callback) callback->Disconnect(CLIENTID_SERVER, ne);	//Notify app.
						//Drop connection.
						printf("Connection terminated by server.\n");
					}
				}
			}
			if(ret >= 4 && RLONG(&recbuf[0]) == PACKET_HEAD_NORMAL){
				//
				//Process normal packets.
				//
				if(ClientActive && CmpAddr(sin, Server.Address)){
					if(ret > 4) Server.ProcessInPackets(&recbuf[4], ret - 4);	//Don't process empty packets.
					Server.LastRecvTime = GetMS();
				}else{
					if(ServerActive && Clients){
						for(int c = 0; c < MaxClients; c++){
							if(CmpAddr(sin, Clients[c].Address)){
								if(ret > 4) Clients[c].ProcessInPackets(&recbuf[4], ret - 4);	//Don't process empty packets.
								Clients[c].LastRecvTime = GetMS();
								break;
							}
						}
					}
				}
			}
			if(ret >= 4 && RLONG(&recbuf[0]) == PACKET_HEAD_OUTOFBAND){
				if(callback) callback->OutOfBandPacket(&sin, &recbuf[4], ret - 4);	//Deliver out of band packets to app.
				break;
			}
		}
	}
	//
	//End of incoming, begin outgoing.
	//
	if(ServerActive && Clients){	//Process client connections.
		for(int c = 0; c < MaxClients; c++){
			if(Clients[c].CCS == CCS_None) continue;
			if(Clients[c].CCS == CCS_InSendChallenge && (GetMS() - Clients[c].LastSendTime) > ReliableTimeout){
				Clients[c].LastSendTime = GetMS();
				WLONG(&senbuf[0], PACKET_HEAD_CHALLENGE);	//Send challenge.
				WLONG(&senbuf[4], Clients[c].ChallengeValue);
				SendTo(Sock, senbuf, 8, 0, (sockaddr*)&Clients[c].Address, sizeof(sockaddr_in));
				printf("Sent challenge.\n");
			}
			if(Clients[c].CCS != CCS_None && (GetMS() - Clients[c].LastRecvTime) > ConnectionTimeout && Clients[c].LastRecvTime != 0){
			//	Clients[c].Initialize();	//Drop connection.
			//	if(callback) callback->Disconnect(c, NE_TimedOut);	//Notify app.
				ServerDisconnect(c, callback, NE_TimedOut);
				printf("Dropped connection to client %d.\n", c);
			}
			//
			//Add sending of queued packets.
			//
			int outlen = 0;
			if(Clients[c].CCS == CCS_Connected){
				//Rate control.
			//	int maxout = ((GetMS() - Clients[c].LastSendTime) * Clients[c].ByteRate) / 1000;
				//Only send as many bytes as Rate * seconds since last send allows.
			//	if(maxout < 0) maxout = MAX_PACKET;
				if((outlen = Clients[c].ProcessOutPackets(&senbuf[4], MAX_PACKET - 4, ReliableTimeout)) > 0){
					//
					WLONG(&senbuf[0], PACKET_HEAD_NORMAL);
					SendTo(Sock, senbuf, outlen + 4, 0, (sockaddr*)&Clients[c].Address, sizeof(sockaddr_in));
					//
					Clients[c].LastSendTime = GetMS();
				}
			}
		}
	}
	if(ClientActive){// && Server.CCS != CCS_None){
		if(Server.CCS == CCS_OutReplyChallenge && (GetMS() - Server.LastSendTime) > ReliableTimeout){
			Server.LastSendTime = GetMS();
			WLONG(&senbuf[0], PACKET_HEAD_CHALLENGEREPLY);
			WLONG(&senbuf[4], Server.ChallengeValue);
			SendTo(Sock, senbuf, 8, 0, (sockaddr*)&Server.Address, sizeof(sockaddr_in));
			printf("Sent reply to challenge.\n");
		}
		if(Server.CCS == CCS_OutRequestConnect && (GetMS() - Server.LastSendTime) > ReliableTimeout){
			Server.LastSendTime = GetMS();
			if(Server.LastRecvTime == 0) Server.LastRecvTime = GetMS();
			if((GetMS() - Server.LastRecvTime) > ConnectTimeout){
				Server.Initialize();
				if(callback) callback->Disconnect(CLIENTID_SERVER, NE_TimedOut);	//Tell app connection failed.
				printf("Connection request timed out.\n");
			}else{
				WLONG(&senbuf[0], PACKET_HEAD_CONNECT);
				SendTo(Sock, senbuf, 4, 0, (sockaddr*)&Server.Address, sizeof(sockaddr_in));
				printf("Sent connection request to %s, port %d.\n", inet_ntoa(Server.Address.sin_addr), (int)(ntohs(Server.Address.sin_port)));
			}
		}
		if(Server.CCS != CCS_None && (GetMS() - Server.LastRecvTime) > ConnectionTimeout && Server.LastRecvTime != 0){
		//	Server.Initialize();
		//	if(callback) callback->Disconnect(CLIENTID_SERVER, NE_TimedOut);	//Notify app.
			ClientDisconnect(callback, NE_TimedOut);
			//Drop connection.
			printf("Dropped connection to server.\n");
		}
		//
		//Add sending of queued packets.
		//
		int outlen = 0;
		if(Server.CCS == CCS_Connected &&
			(outlen = Server.ProcessOutPackets(&senbuf[4], MAX_PACKET - 4, ReliableTimeout)) > 0){
			//
			WLONG(&senbuf[0], PACKET_HEAD_NORMAL);
			SendTo(Sock, senbuf, outlen + 4, 0, (sockaddr*)&Server.Address, sizeof(sockaddr_in));
			//
			Server.LastSendTime = GetMS();
		}
	}
	if(ConnectTo.len() > 0){// && ConnectToPort > 0){
	//	LPHOSTENT lphe = NULL;
	//	printf("Looking up hostname.\n");
	//	//
	//	//Convert into seperate thread for lookup.
	//	//
	//	IN_ADDR inadr;
	//	inadr.s_addr = inet_addr(ConnectTo);
	//	//
	//	if(inadr.s_addr == INADDR_NONE){
	//		lphe = gethostbyname(ConnectTo);
	//	}
	//	if(lphe){
	//		Server.Address.sin_addr = *((LPIN_ADDR)*lphe->h_addr_list);
	//	}else{
	//		Server.Address.sin_addr = inadr;
	//	}
		Server.Initialize();	//I think this is OK here...  And it will reset LastRecvTime for connection timeout.
		Server.Address.sin_port = htons(ConnectToPort);
		if(LookupAddress(ConnectTo, &Server.Address)){
			Server.CCS = CCS_OutRequestConnect;
			printf("Set connect attempt state.\n");
		}else{
			Server.CCS = CCS_None;//CCS_HostnameLookupFailed;
			printf("Host name lookup failed.\n");
			if(callback) callback->Disconnect(CLIENTID_SERVER, NE_LookupFailed);	//Tell app connection failed.
		}
		ConnectTo = "";
	}
	//
	//Now yank delivered packets off of connections and send back to app.
	//
	if(callback){
		if(ClientActive && Server.CCS == CCS_Connected){	//Deliver packets from server to client.
			Server.DeliverPackets(CLIENTID_SERVER, callback);
		}
		if(ServerActive && Clients){
			for(int c = 0; c < MaxClients; c++){
				Clients[c].DeliverPackets(c, callback);
			}
		}
	}
	//
	return true;
}
bool Network::LookupAddress(const char *addr, sockaddr_in *ipout){
	if(addr && ipout){
		ipout->sin_family = AF_INET;
		CStr a;
		int i;
		if((i = Instr(addr, ":")) >= 0){	//Separate address and port.
			a = Left(addr, i);
			ipout->sin_port = htons((short)atoi(addr + i + 1));
		}else{
			a = addr;
		}
		hostent* lphe = NULL;
		printf("Looking up hostname.\n");
		//
		ipout->sin_addr.s_addr = inet_addr(a);
		//
		if(ipout->sin_addr.s_addr == INADDR_NONE){
			lphe = gethostbyname(a);
		}
		if(lphe){
			ipout->sin_addr = *((in_addr*)*lphe->h_addr_list);
		}
		if(ipout->sin_addr.s_addr != INADDR_NONE){
			return true;	//Success!
		}
	}
	return false;
}

#define MINIPACKET_UNRELIABLE	0x4000
#define MINIPACKET_RELIABLE		0x8000
#define MINIPACKET_ORDERED		0xc000
#define MINIPACKET_LENMASK		0x3fff
#define MINIPACKET_TYPEMASK		0xc000
//
//Be sure to call ProcessOutPackets BEFORE externally removing any packets from queues, so that the
//incoming queue can be checked for un-acked packets and acks can be sent.
//
bool ClientConnection::ProcessInPackets(const char *data, int length){	//Takes a hunk of incoming data and breaks it into packets and acks on the incoming side of the client connection.
	if(!data || length <= 0) return false;
	int RAcks = (unsigned char)data[0];	//Reliable acks
	int OAcks = (unsigned char)data[1];	//Ordered acks
	int Packs = (unsigned char)data[2];
	//data[3] is pad.
	int d = 4;
	while(RAcks){
		PacketID acked = RLONG(&data[d]);
		for(MiniPacket *mp = OutReliableHead.NextLink(); mp; mp = mp->NextLink()){
			if(mp->Ident == acked){
			//	if(rand() & 1){	//Temp test!
				mp->DeleteItem();	//We're acked, so remove.
			//	}
				break;
			}
		}
		d += 4;
		RAcks--;
	}
	while(OAcks){
		PacketID acked = RLONG(&data[d]);
		for(MiniPacket *mp = OutOrderedHead.NextLink(); mp; mp = mp->NextLink()){
			if(mp->Ident == acked){
				mp->DeleteItem();	//We're acked, so remove.
				break;
			}
		}
		d += 4;
		OAcks--;
	}
	while(Packs && d < length){
		PacketID id = 0;
		short type = RSHORT(&data[d]);
		short len = type & MINIPACKET_LENMASK;	//Extract type and length of minipacket.
		type &= MINIPACKET_TYPEMASK;
		d += 2;
		if(type == (short)MINIPACKET_RELIABLE || type == (short)MINIPACKET_ORDERED){	//Read packet id.
			id = RLONG(&data[d]);
			d += 4;
		}
		//
		if(len + d <= length){	//Sanity, make sure packet wouldn't break buffer.
			switch((unsigned short)type){
			case MINIPACKET_UNRELIABLE : QueueInPacket(TM_Unreliable, &data[d], len, id); break;
			case MINIPACKET_RELIABLE : QueueInPacket(TM_Reliable, &data[d], len, id); break;
			case MINIPACKET_ORDERED :
				QueueInPacket(TM_Ordered, &data[d], len, id);
				break;
			default :
				printf("Bad minipacket type! %x %x\n", type, len);
				break;
			}
		}
		d += len;
		//
		Packs--;
	}
	return true;
}
int ClientConnection::ProcessOutPackets(char *data, int maxlen, TimeStamp RetryTime){	//Returns how many bytes of output buffer (to be sent) were filled with mini-packet data.
	if(!data || maxlen <= 4) return 0;
	//
	//Now do rate controls internally here.
	int ratelen = 0;	//ratelen is a SOFT length max...  Once it is _broken_, processing will stop.
//	int nextidx = (RateHistIdx + 1) % RATE_WINDOW_SIZE;	//As of now, the oldest entry in the queue.
	TimeStamp totaltime = GetMS() - RateHistTime[RateHistIdx];	//Time window our history encompasses.
	for(int i = 0; i < RATE_WINDOW_SIZE; i++) ratelen += RateHistBytes[i];	//Total of bytes sent so far in time window.
	ratelen = ((ByteRate * totaltime) / 1000) - ratelen;	//What we are allowed to send in this time, minus what we have sent in this time.
	//
	int RAcks = 0;	//TODO: Scan incoming list for packets that need acking.
	int OAcks = 0;	//Ditto.
	int Packs = 0;	//Inc this as we go.
	int d = 4;
	MiniPacket *mp;
	//
	//Send Acks for packets that came in.
	for(mp = InReliableHead.NextLink(); mp; mp = mp->NextLink()){
		if(mp->Acked == 0 && d + 4 <= maxlen && RAcks < 255 && d <= ratelen){
			mp->Acked = 1;
			WLONG(&data[d], mp->Ident);
			d += 4;
			RAcks++;
		}
	}
	for(mp = InOrderedHead.NextLink(); mp; mp = mp->NextLink()){
		if(mp->Acked == 0 && d + 4 <= maxlen && OAcks < 255 && d <= ratelen){
			mp->Acked = 1;
			WLONG(&data[d], mp->Ident);
			d += 4;
			OAcks++;
		}
	}
	//
	//Send reliable and ordered packets too.
//	mp = OutOrderedHead.NextLink();
	mp = OutOrderedHead.Tail();
	while((mp != &OutOrderedHead) && mp && d < maxlen && Packs < 255){
		if((mp->LastSent == 0 || (GetMS() - mp->LastSent) > RetryTime) && mp->Data && mp->Length > 0 && mp->Length + d + 6 <= maxlen && d <= ratelen){	//Add mini-packed
			short head = (short)MINIPACKET_ORDERED | (short)mp->Length;
			WSHORT(&data[d], head);
			d += 2;
			WLONG(&data[d], mp->Ident);
			d += 4;
			memcpy(&data[d], mp->Data, mp->Length);
			d += mp->Length;
			mp->LastSent = GetMS();
			Packs++;
		}
	//	mp = mp->NextLink();
		mp = mp->PrevLink();
	}
	//Packets are added to the head of the list, but we should send oldest packets first to avoid lag bubbles
	//during heavy rate control, so must start sending from the tail of the list.
//	mp = OutReliableHead.NextLink();
	mp = OutReliableHead.Tail();
	while((mp != &OutReliableHead) && mp && d < maxlen && Packs < 255){
		if((mp->LastSent == 0 || (GetMS() - mp->LastSent) > RetryTime) && mp->Data && mp->Length > 0 && mp->Length + d + 6 <= maxlen && d <= ratelen){	//Add mini-packed
			short head = (short)MINIPACKET_RELIABLE | (short)mp->Length;
			WSHORT(&data[d], head);
			d += 2;
			WLONG(&data[d], mp->Ident);
			d += 4;
			memcpy(&data[d], mp->Data, mp->Length);
			d += mp->Length;
			mp->LastSent = GetMS();
			Packs++;
		}
	//	mp = mp->NextLink();
		mp = mp->PrevLink();
	}
	//
	//Send all unreliable packets we can.  With unreliable last, and over-lengthers deleted regardless,
	//rate control can now be implemented properly by sending a restrictive length to this function,
	//and the most appropriate packets will be chucked first.
	mp = OutUnreliableHead.NextLink();
	while(mp){// && d < maxlen && Packs < 255){
		if(Packs < 255 && mp->Data && mp->Length > 0 && mp->Length + d + 2 <= maxlen && d <= ratelen){	//Add mini-packed
			short head = MINIPACKET_UNRELIABLE | mp->Length;
			WSHORT(&data[d], head);
			d += 2;
			memcpy(&data[d], mp->Data, mp->Length);
			d += mp->Length;
			Packs++;
		}
		//Now delete mini-packet whether we could actually send it or not.
		MiniPacket *tmp = mp;
		mp = mp->NextLink();
		tmp->DeleteItem();	//Remove mini-packet from queue, no longer needed.
	//	}else{
	//		MiniPacket *tmp = mp;
	//		mp = mp->NextLink();
	//		tmp->DeleteItem();	//Remove mini-packet from queue, no longer needed.
	//	}
	}
	//
	//Post-set count values.
	data[0] = (unsigned char)RAcks;
	data[1] = (unsigned char)OAcks;
	data[2] = (unsigned char)Packs;
	data[3] = 0;
	if(RAcks == 0 && OAcks == 0 && Packs == 0) d = 0;
	//
	//Store rate history.
	RateHistTime[RateHistIdx] = GetMS();
	RateHistBytes[RateHistIdx] = d;
	RateHistIdx = (RateHistIdx + 1) % RATE_WINDOW_SIZE;
	//
	return d;
}

bool ClientConnection::DeliverPackets(ClientID who, PacketProcessorCallback *where){
	if(!where) return false;
	//
	MiniPacket *mp;
	//New, deliver from tail first, to deliver oldest first.
//	mp = InUnreliableHead.NextLink();
	mp = InUnreliableHead.Tail();
	while(mp && mp != &InUnreliableHead){
		MiniPacket *tmp = mp;
		if(mp->Data && mp->Length > 0) where->PacketReceived(who, mp->Data, mp->Length);	//Deliver packet.
	//	mp = mp->NextLink();
		//
		UnreliableBytesIn += mp->Length;	//Record bytes in even if already delivered.
		//
		mp = mp->PrevLink();
		tmp->DeleteItem();	//Remove packet, it's been delivered.
	}
	//
	//Deliver reliable packets.
//	mp = InReliableHead.NextLink();
	mp = InReliableHead.Tail();
	while(mp && mp != &InReliableHead){
		MiniPacket *tmp = mp;
		if(!GetDelivered(mp->Ident) && mp->Data && mp->Length > 0){
			where->PacketReceived(who, mp->Data, mp->Length);	//Deliver packet.
			SetDelivered(mp->Ident);
		}
	//	mp = mp->NextLink();
		//
		ReliableBytesIn += mp->Length;	//Record bytes in even if already delivered.
		//
		mp = mp->PrevLink();
		tmp->DeleteItem();	//Remove packet, it's been delivered.
	}
	//
	int cont = 1;
	while(cont){
		cont = 0;
	//	mp = InOrderedHead.NextLink();
		if (!InOrderedHead.NextLink()) break;	// Check for disconnection
		mp = InOrderedHead.Tail();
		while(mp && mp != &InOrderedHead){
			MiniPacket *tmp = mp;
			if(mp->Ident == NextOrderedToDeliver){
				if(mp->Data && mp->Length > 0) where->PacketReceived(who, mp->Data, mp->Length);	//Deliver packet.
				if (!InOrderedHead.NextLink()) break;	// Check for disconnection
				NextOrderedToDeliver++;
			//	mp = mp->NextLink();
				//
				OrderedBytesIn += mp->Length;
				//
				mp = mp->PrevLink();
				tmp->DeleteItem();	//Remove packet, it's been delivered.
				cont = 1;	//Go around again, since we changed NOTD.
			}else if(mp->Ident < NextOrderedToDeliver){	//Stale packet.
			//	mp = mp->NextLink();
				//
				OrderedBytesIn += mp->Length;	//Still record bytes in, even if stale.
				//
				mp = mp->PrevLink();
				tmp->DeleteItem();	//Remove packet, it's been delivered.
			}else{
			//	mp = mp->NextLink();
				mp = mp->PrevLink();
			}
		}
	}
	//
	return true;
}

//This function uses a history bitfield to make sure Reliable packets aren't delivered twice.
int ClientConnection::GetDelivered(PacketID id){
	int nSet = (id / BITS_IN_SET);
	int Set = nSet & 1;
	int Bit = id % BITS_IN_SET;
	if(nSet > HighSet){	//Clear set when we get to a new one.
		HighSet = nSet;
		memset(BitSet[Set], 0, BYTES_IN_SET);
	}
	return (BitSet[Set][Bit >>3] & (1 << (Bit & 7))) != 0 ? 1 : 0;
}
void ClientConnection::SetDelivered(PacketID id){
	int nSet = (id / BITS_IN_SET);
	int Set = nSet & 1;
	int Bit = id % BITS_IN_SET;
	if(nSet > HighSet){	//Clear set when we get to a new one.
		HighSet = nSet;
		memset(BitSet[Set], 0, BYTES_IN_SET);
	}
	BitSet[Set][Bit >>3] |= (1 << (Bit & 7));
}

void Network::BanClient(int ClientID, time_t Duration)
{
	Bans.AddBan(Clients[ClientID].Address.sin_addr, Duration);
}
