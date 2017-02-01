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

#ifndef TMMASTER_H
#define TMMASTER_H

#include "../game/Networking.h"
#include "../game/BitPacking.h"
#include "../game/Heartbeat.h"

//#define TMMASTER_CHALLENGE_KEY		0xc8a10001
#define TMMASTER_CHALLENGE_KEY		0xc8a10002
//For now this is identical to the game client/server challenge key.

class MasterPacketProcessor : public PacketProcessorCallback {
	virtual void Connect(ClientID source);
	virtual void Disconnect(ClientID source, NetworkError ne);
	virtual void PacketReceived(ClientID source, const char *data, int len);
	virtual void OutOfBandPacket(sockaddr_in *src, const char *data, int len);
};

#endif

