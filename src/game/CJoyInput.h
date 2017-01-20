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

#ifndef _C_INPUT_H_
#define _C_INPUT_H_

#include <cstdio>
#include <cstring>

#include "CStr.h"

// limits
#define K_MAXJOYAXES		6
#define K_MAXJOYBUTTONS		32
#define K_MAXDEVNAME		MAXPNAMELEN
#define K_MAXCONTROLLERS	16

// axis labels
#define _XAXIS	0
#define _YAXIS	1
#define _ZAXIS	2
#define _RAXIS	3
#define _UAXIS	4
#define _VAXIS	5

// device structures

struct trJoyAxis
{
	float	position;
	bool	bActive;

	trJoyAxis() {position = 0.0f; bActive = false; }
};

struct trDeviceID
{
	int		iDevID;
	CStr	sDevName;

	trDeviceID() {sDevName = ""; iDevID = -2;}
};

struct trControllerStatus
{
	int			iName;
	trJoyAxis	aAxes[K_MAXJOYAXES];
	bool		aButtons[K_MAXJOYBUTTONS];
	int			iButtons;
	int			iAxes;
	bool		bHasHat;
	float		aHatVector[2];
	trDeviceID	rDeviceID;

	trControllerStatus() {iName = -2; std::memset(aAxes, 0, sizeof(trJoyAxis)); std::memset(aButtons, 0, sizeof(aButtons)); iButtons = 0; iAxes = 0; bHasHat = false; std::memset(aHatVector, 0, sizeof(aHatVector));}
};

class CJoyInput
{
private:
	trControllerStatus	m_rController;
	int					idJoystick;

	bool				LastButtons[K_MAXJOYBUTTONS];

	// Not allowed for now
	CJoyInput(const CJoyInput&);
	CJoyInput& operator=(const CJoyInput&);

public:
	CJoyInput(void);
	~CJoyInput(void);

	bool InitController ( int iController = -1 );
	int UnInitController ( void );

	int	 GetNumberOfControllers( void );
	bool GetControllerName ( int id, CStr* Name);
	bool GetDeviceInfo( trDeviceID *pDevInfo, int iID  = -1 ); // -1 will give current info
	int GetDevIDFromInfo ( trDeviceID *pDevInfo );  // if returns -1 then contoller was not found and using defalut
	int GetFirstControllerID ( void );

	int NumberOfButtons() {return m_rController.iButtons;}
	int NumberOfAxes() {return m_rController.iAxes;}
	bool HasHat() {return m_rController.bHasHat;}
	bool AxisSupported(unsigned int i){return (i < K_MAXJOYAXES) ? m_rController.aAxes[i].bActive : false;}

	bool ButtonGoingUp(unsigned int i) {return (i >= K_MAXJOYBUTTONS) ? false : ((m_rController.aButtons[i] == 0) && (LastButtons[i] == 1));}
	bool ButtonGoingDown(unsigned int i) {return (i >= K_MAXJOYBUTTONS) ? false : ((m_rController.aButtons[i] == 1) && (LastButtons[i] == 0));}
	float GetAxis(unsigned int i) {return (i < K_MAXJOYAXES) ? m_rController.aAxes[i].position : 0.0f;}
	float GetHatX() {return m_rController.aHatVector[0];}
	float GetHatY() {return m_rController.aHatVector[1];}

	void Update(void);
};
#endif
