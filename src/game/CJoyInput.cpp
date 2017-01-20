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

#include "CJoyInput.h"

#include <cmath>

#include <SFML/Window.hpp>

using namespace std;

CJoyInput::CJoyInput() : idJoystick(-1)
{
}

CJoyInput::~CJoyInput()
{
}

CStr getJoystickName(int iId)
{
	// SFML doesn't currently support retrieving the name of a joystick, so we'll make our own.
	return "Joystick " + String(iId+1);
}

bool CJoyInput::InitController ( int iController )
{
	// Default controller in SFML-talk is 0
	if(iController == -1)
		iController = 0;

	if(m_rController.iName == iController)
		return true;

	if(!sf::Joystick::isConnected(iController))
		return false;

	memset(LastButtons, 0, sizeof(LastButtons));
		
	memset(&m_rController, 0, sizeof(trControllerStatus));
	
	m_rController.iName = iController;
	m_rController.iAxes = 0;
	for(auto it = 0; it <= sf::Joystick::AxisCount; ++it)
		m_rController.iAxes += sf::Joystick::hasAxis(iController, sf::Joystick::Axis(it));
	m_rController.iButtons = sf::Joystick::getButtonCount(iController);

	m_rController.rDeviceID.iDevID = m_rController.iName;
	m_rController.rDeviceID.sDevName = getJoystickName(iController);

	if(sf::Joystick::hasAxis(iController, sf::Joystick::PovX) || sf::Joystick::hasAxis(iController, sf::Joystick::PovY))
		m_rController.bHasHat = true;
	else
		m_rController.bHasHat = false;

	m_rController.aHatVector[0] = m_rController.aHatVector[1] = 0;

	if (m_rController.iAxes <= 0)
		return false;

	if (sf::Joystick::hasAxis(iController, sf::Joystick::X))
		m_rController.aAxes[_XAXIS].bActive = true;

	if (sf::Joystick::hasAxis(iController, sf::Joystick::Y))
		m_rController.aAxes[_YAXIS].bActive = true;

	if (sf::Joystick::hasAxis(iController, sf::Joystick::Z))
		m_rController.aAxes[_ZAXIS].bActive = true;

	if (sf::Joystick::hasAxis(iController, sf::Joystick::R))
		m_rController.aAxes[_RAXIS].bActive = true;

	if (sf::Joystick::hasAxis(iController, sf::Joystick::U))
		m_rController.aAxes[_UAXIS].bActive = true;

	if (sf::Joystick::hasAxis(iController, sf::Joystick::V))
		m_rController.aAxes[_VAXIS].bActive = true;

	idJoystick = iController;

	return true;
}

int CJoyInput::UnInitController ( void )
{
	idJoystick = -1;
	
	memset(&m_rController, 0, sizeof(trControllerStatus));

	return 0;
}

int CJoyInput::GetFirstControllerID ( void )
{
	if(GetNumberOfControllers() > 0)
		return 0;
	return -1;
}

bool CJoyInput::GetDeviceInfo(trDeviceID *pDevInfo, int iID )
{
	if (iID == -1)
	{
		if (idJoystick == -1) // if no controller then try to init the defalut
			if (!InitController()) // if it don't exist we need to bail
				return false;

		// copy the current controlers setup to the sruct
		pDevInfo->iDevID = m_rController.rDeviceID.iDevID;
		pDevInfo->sDevName = m_rController.rDeviceID.sDevName;
		return true;
	}

	if(!sf::Joystick::isConnected(iID))
		return false;

	pDevInfo->iDevID = iID;
	pDevInfo->sDevName = getJoystickName(iID);

	return true;
}

int CJoyInput::GetDevIDFromInfo ( trDeviceID *pDevInfo )
{
	if (!pDevInfo)
		return -1;

	for (int i = 0; i < sf::Joystick::Count; i ++)
	{
		if(getJoystickName(i) == pDevInfo->sDevName)
		{
			pDevInfo->iDevID = i;
			return i;
		}
	}
	return -1;
}

bool CJoyInput::GetControllerName ( int id, CStr* Name )
{
	if ( (GetNumberOfControllers() <= 0) || (id > GetNumberOfControllers()) )
		return false;

	if (!Name)
		return false;

	if (id == -1)
	{
		if (idJoystick == -1) // if no controller then try to init the defalut
			if (!InitController()) // if it don't exist we need to bail
				return false;
		// copy the name of the defalut
		*Name = m_rController.rDeviceID.sDevName;
		return true;
	}

	*Name = getJoystickName(id);

	return true;
}

int CJoyInput::GetNumberOfControllers( void )
{
	int i = 0;
	for(; i < sf::Joystick::Count; ++i)
		if(!sf::Joystick::isConnected(i))
			return i;

	return i;
}

void CJoyInput::Update(void)
{
	bool	bError = false;

	if(idJoystick != -1)
	{
		memcpy(LastButtons, m_rController.aButtons, sizeof(LastButtons));

		for(int i = _XAXIS; i <= _VAXIS; ++i)
		{
			if (m_rController.aAxes[i].bActive)
			{
				m_rController.aAxes[i].position = sf::Joystick::getAxisPosition(idJoystick, sf::Joystick::Axis(i)) / 100.0f;
			}
		}

		if (m_rController.bHasHat)
		{
			m_rController.aHatVector[0] = sf::Joystick::getAxisPosition(idJoystick, sf::Joystick::PovX) / 100.0f;
			m_rController.aHatVector[1] = sf::Joystick::getAxisPosition(idJoystick, sf::Joystick::PovY) / 100.0f;
		}
		else
			m_rController.aHatVector[0] = m_rController.aHatVector[1] = 0;

		for (int i = 0; i < m_rController.iButtons; i ++)
			m_rController.aButtons[i] = sf::Joystick::isButtonPressed(idJoystick, i);
	}
}
