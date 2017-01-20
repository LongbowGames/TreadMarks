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

#ifndef NETWORKING_H
#define NETWORKING_H

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

#include "Trees.h"
#include "CStr.h"

#include "BanList.h"

typedef unsigned int PacketID;
typedef unsigned int TimeStamp;
typedef unsigned int ClientID;

//Macros for inserting network-order shorts and longs into arbitrary memory locations.
//Can be used by App when reading/writing to mini-packets.
//
#define WLONG(ptr, lng) (*((uint32_t*)(ptr)) = htonl((lng)))
#define RLONG(ptr) (ntohl(*((uint32_t*)(ptr))))
//
//#define WFLOAT(ptr, flt) (*((long*)(ptr)) = htonl(*((long*)&(flt))))
//#define RFLOAT(ptr) (ntohl(*((long*)(ptr))))
inline void WFLOAT(void *ptr, float flt){
	*((long*)ptr) = htonl(*((long*)&flt));
};
inline float RFLOAT(const void *ptr){
	long t = htonl(*((long*)ptr));
	return *((float*)&t);
}
//
#define WSHORT(ptr, srt) (*((short*)(ptr)) = htons((srt)))
#define RSHORT(ptr) (ntohs(*((short*)(ptr))))
//
inline int CmpAddr(const sockaddr_in &a1, const sockaddr_in &a2){
	if(a1.sin_addr.s_addr == a2.sin_addr.s_addr && a1.sin_port == a2.sin_port) return 1;
	else return 0;
}
TimeStamp GetTime();

enum ClientConnectionStatus{
	CCS_None,
	CCS_OutRequestConnect,
//	CCS_InConnectReceived,
	CCS_InSendChallenge,
	CCS_OutReplyChallenge,
	CCS_Connected,
//	CCS_HostnameLookupFailed
};

enum TransmissionMode{
	TM_Unreliable = 0,	//No packet resending at all.
	TM_Reliable = 1,	//Packets are resent until delivered, but reception order is not guaranteed.
	TM_Ordered = 2		//Packet reception and order are guaranteed.
};

enum NetworkError{
	NE_OK = 0,
	NE_Unknown,
	NE_Disconnected,
	NE_TimedOut,
	NE_MissingRes,
	NE_ChallengeFailed,
	NE_ServerFull,
	NE_LookupFailed,
	NE_BindFailed
};

#define CLIENTID_BROADCAST	0xffffffff
#define CLIENTID_SERVER		0x7fffffff
#define CLIENTID_NONE		0x3fffffff

class PacketProcessorCallback{
public:
	virtual void Connect(ClientID source) = 0;
	virtual void Disconnect(ClientID source, NetworkError ne) = 0;
	virtual void PacketReceived(ClientID source, const char *data, int len) = 0;
	//
	virtual void OutOfBandPacket(sockaddr_in *src, const char *data, int len) = 0;
	//
	//The main app derives a class from this base to be used as a callback for when
	//packets arrive.  source will be CLIENTID_SERVER if packet is from server to
	//client, or if we are the client disconnecting or connecting to the server.
};

class MiniPacket : public LinklistBase<MiniPacket> {
friend class Network;	//Transmission Mode is implicit in list packet is part of.
friend class ClientConnection;
private:
	PacketID Ident;
	TimeStamp LastSent;
	int Acked;
	int Length;
	int Priority;
	char *Data;
public:
	MiniPacket();
	MiniPacket(const char *data, int length, PacketID id, int pri = 0);
	~MiniPacket();
	void Free();
	bool Init(const char *data, int length, PacketID id, int pri = 0);
	MiniPacket *InsertAfterPriority(MiniPacket *mp);
};

class ClientConnection{
friend class Network;
private:
	ClientConnectionStatus CCS;
	sockaddr_in Address;
	TimeStamp LastRecvTime;
	TimeStamp LastSendTime;
	unsigned int ChallengeValue;
	unsigned int ByteRate;
	//
	//Lists of pending packets
	MiniPacket OutUnreliableHead;
	MiniPacket OutReliableHead;
	MiniPacket OutOrderedHead;
	MiniPacket InUnreliableHead;
	MiniPacket InReliableHead;
	MiniPacket InOrderedHead;
	//
	PacketID NextOrderedToDeliver;	//ID of the next packet in the InOrdered queue (or not in it, as the case may be) to actually deliver to the high level app.
	PacketID NextReliableID;
	PacketID NextOrderedID;	//Send-IDs.
	//
	unsigned int UnreliableBytesIn;
	unsigned int ReliableBytesIn;
	unsigned int OrderedBytesIn;
private:
	//Routines and data for making sure Reliable packets are only delivered to app once each.
	//Will hold proper status for last 8,000 to 16,000 packets, will break down after that.
#define BYTES_IN_SET 1024
#define BITS_IN_SET (BYTES_IN_SET / 8)
	int GetDelivered(PacketID id);
	void SetDelivered(PacketID id);
	unsigned char BitSet[2][BYTES_IN_SET];
	int HighSet;
private:
	//History of bytes sent, for proper Rate controlling.
#define RATE_WINDOW_SIZE 32
	int RateHistBytes[RATE_WINDOW_SIZE];
	TimeStamp RateHistTime[RATE_WINDOW_SIZE];
	int RateHistIdx;
public:
	ClientConnection();
	~ClientConnection();
	void Initialize();
	bool QueueOutPacket(TransmissionMode mode, const char *data, int length, int pri = 0);
	bool QueueInPacket(TransmissionMode mode, const char *data, int length, PacketID id = 0);
	//
	bool ProcessInPackets(const char *data, int length);	//Takes a hunk of incoming data and breaks it into packets and acks on the incoming side of the client connection.
	int ProcessOutPackets(char *data, int maxlen, TimeStamp RetryTime);	//Returns how many bytes of output buffer (to be sent) were filled with mini-packet data.
	//
	bool DeliverPackets(ClientID who, PacketProcessorCallback *where);
	//Delivers pending in packets to defined callback, passing through specified client id.
	//
	unsigned int BytesQueuedOut(unsigned int *packets = NULL);	//Calculates the number of bytes, and optionally packets, queued for in and out transport.  May be slow if queues are big.
	unsigned int BytesQueuedIn(unsigned int *packets = NULL);
};

class Network{
private:
	unsigned int Sock;//ClientSock, ServerSock;
	bool Initialized, ServerActive, ClientActive;
	unsigned int ReliableTimeout;
	unsigned int ConnectionTimeout;
	unsigned int ConnectTimeout;	//Timeout for making a new connection.
	ClientConnection *Clients;
	CBanList Bans;
	int MaxClients;
	ClientConnection Server;	//Connection TO server.
	//
#ifdef WIN32
	WSADATA WsaData;
#endif
//	PacketProcessorCallback Callback;
	//
	CStr ConnectTo;
	short ConnectToPort;
	unsigned int ChallengeKey;
	//
	unsigned int bytesout, bytesin;
	int MaxServerClientRate;

public:
	Network();
	~Network();
	bool Initialize();
	void Free();
//	void SetCallback(PacketProcessorCallback *callback);	//This must be called in order to be able to process network events.
	bool InitServer(short port, int maxconn, int retrybind = 0);
	void FreeServer();
	bool InitClient(short port, int retrybind = 0);	//Retrybind is how many ports above requested port to attempt to bind to if requested port fails.
	void FreeClient();
	bool Configure(unsigned int conntimeout = 60000,
		unsigned int reltimeout = 1000,
		unsigned int newconntimeout = 15000);
	bool IsServerActive(){ return ServerActive; };
	bool IsClientActive(){ return ClientActive; };

	void LoadBanList(char *sFileName) { Bans.LoadBanList(sFileName); }
	void SaveBanList(char *sFileName) { Bans.SaveBanList(sFileName); }
	void BanClient(int ClientID, time_t Duration);

public:
	ClientConnectionStatus ConnectionStatus(ClientID who);	//Use CLIENTID_SERVER to query whether we are connected to a server.
public:
	bool ClientConnect(const char *name, short port);
	bool LookupAddress(const char *addr, sockaddr_in *ipout);	//Fills out ipout with the ip address and optionally port (x.x.x.x:port).
	bool ClientDisconnect(PacketProcessorCallback *callback = NULL,
		NetworkError reason = NE_Disconnected);	//Disconnect client (us) from server, optionally get called back, optionally give reason.
	bool ServerDisconnect(ClientID client,
		PacketProcessorCallback *callback = NULL,
		NetworkError reason = NE_Disconnected);	//Disconnect a client from server (us), optionally get called back, optionally give reason.
	bool ProcessTraffic(PacketProcessorCallback *callback);	//A NULL callback will process traffic without returning packets or connect notifications.
public:
	bool QueueSendServer(TransmissionMode mode, const char *data, int len, int pri = 0);
	bool QueueSendClient(ClientID client, TransmissionMode mode, const char *data, int len, int pri = 0);
	bool SendOutOfBandPacket(sockaddr_in *dest, const char *data, int len);
	int HTTPGet(sockaddr_in *dest, const char *file, char *buffer, int len);
public:
	bool SetClientRate(ClientID client, unsigned int rate);
	unsigned int GetClientRate(ClientID client);
	bool SetServerRate(unsigned int rate);	//Sets the maximum rate that a client can set for itself, useful for limiting outgoing server bandwidth.
	unsigned int GetServerRate();
	void SetChallengeKey(unsigned int key);	//This value must match on both client and server.
public:
	unsigned int TotalBytesOut();
	unsigned int TotalBytesIn();	//Statistic tracking functions.
	void ResetByteCounters();
	//
	unsigned int ClientQueueSize(ClientID client);	//Returns the number of bytes queued out to a specific client but not yet sent or delivered.
	unsigned int ServerQueueSize();	//Returns bytes queued for sending to the server, but not yet sent or delivered.
	bool GetServerInByteCounts(unsigned int *ubytes, unsigned int *rbytes, unsigned int *obytes);	//Gets the byte counts for individual packet types received, but not necessarily delivered, from the server to us as a client.  This can show if we are being flooded with extraneous Reliable or Ordered packets.
	void ResetServerInByteCounts();	//Resets above counters.
private:
	bool InitSocket();
	bool FreeSocket();
	int SendTo(int s, char *buf, int len, int flag, const sockaddr *addr, int addrlen);
	int RecvFrom(int s, char *buf, int len, int flag, sockaddr *addr, socklen_t *addrlen);
	//Private send and receive functions that can have packet loss and lag simulation applied to them.

public:
	//Added JM
	void GetClientIPString (int id,  char *szName);
};


#endif
