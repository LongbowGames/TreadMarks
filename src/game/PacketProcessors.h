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

#ifndef __PACKETPROCESSORS_H__
#define __PACKETPROCESSORS_H__

#include "Networking.h"

extern NetworkError LastDisconnectError;
void OOBPing(sockaddr_in *to);	//Sends an out of band ping using MasterNet object.

class TankPacketProcessor : public PacketProcessorCallback {
	virtual void Connect(ClientID source);
	virtual void Disconnect(ClientID source, NetworkError ne);
	virtual void PacketReceived(ClientID source, const char *data, int len);
	virtual void OutOfBandPacket(sockaddr_in *src, const char *data, int len);
};

class MasterClientPacketProcessor : public TankPacketProcessor {
	virtual void Connect(ClientID source);
	virtual void Disconnect(ClientID source, NetworkError ne);
	virtual void PacketReceived(ClientID source, const char *data, int len);
};
//
#endif // __PACKETPROCESSORS_H__
