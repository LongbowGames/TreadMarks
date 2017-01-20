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

//Disable exceptions not enabled warnings.
#pragma warning( disable : 4530 )

#include <new>
#include <iostream>
#include "EntityTank.h"
#include "TankGame.h"
#include "TankRacing.h"
#include "EntityGUI.h"
#include "VoxelWorld.h"
#include "GenericBuffer.h"
#include "Reg.h"
#include <ctime>

#include "BitPacking.h"
#include "TMMaster.h"
#include "TankGUI.h"
#include "vecmath.h"
#include "TxtBlock.hpp"
#include "TextLine.hpp"
#include "PacketProcessors.h"

#ifndef HEADLESS
#include <GL/glew.h>
#endif

#ifdef WIN32
#define strcasecmp(a, b) _stricmp(a, b)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace std;

CTankGame& CTankGame::Get()
{
	static CTankGame TGame;
	return TGame;
}

CGraphicsSettings::CGraphicsSettings()
{
	RendFlags = REND_FILTER | REND_SMOOTH | REND_OVERSAMPLE;
	Stretch = false;
	UseFullScreen = false;
	DisableMT = 0;
	UseFog = true;
	UseAlphaTest = false;
	MenuFire = 1;
	GLBWIDTH = 0;
	GLBHEIGHT = 0;
	Quality = 0.5f;
	WireFrame = false;
	StripFanMap = true;
	TreadMarks = false;
	DetailTerrain = false;
	PolyLod = 0;
	Particles = 1.0f;

	MaxTexRes = 256;
	TexCompress = false;
	TexCompressAvail = false;
	PalettedTextures = false;
	Trilinear = 0;

	ViewDistance = 500;

	DebugLockPatches = 0;
	DebugPolyNormals = false;
}

CInputSettings::CInputSettings()
{
	MouseSpeed = 1.0f;
	InvMouseY = 0;
	MouseEnabled = 1;
//	memset(AxisMapping, 0, sizeof(AxisMapping));
	UseJoystick = 0;
	sStickName = "";
	StickID = -1;
	DeadZone = 0.0f;

	AxisMapping[0] = 1;
	AxisMapping[1] = 2;
	AxisMapping[2] = 0;
	AxisMapping[3] = 3;
	AxisMapping[4] = 0;
	AxisMapping[5] = 0;
	AxisMapping[6] = 0;
	AxisMapping[7] = 0;
}

void CInputSettings::InitControls() // can't do this in the constructor because Text hasn't been loaded yet
{
	FirstCamControl = 13;

	Controls[0].Set(ControlText.Get(CONTROL_TURNLEFT), "LEFT", "A", "", CEID_Left);
	Controls[1].Set(ControlText.Get(CONTROL_TURNRIGHT), "RIGHT", "D", "", CEID_Right);
	Controls[2].Set(ControlText.Get(CONTROL_FORWARD), "UP", "W", "NUMLOCK", CEID_Up);
	Controls[3].Set(ControlText.Get(CONTROL_REVERSE), "DOWN", "S", "", CEID_Down);
	Controls[4].Set(ControlText.Get(CONTROL_GUNLEFT), ",", "Z", "", CEID_TLeft);
	Controls[5].Set(ControlText.Get(CONTROL_GUNRIGHT), ".", "C", "", CEID_TRight);
	Controls[6].Set(ControlText.Get(CONTROL_FIRE), "SPACE", "Mouse L", "Joy 1", CEID_Fire);
	Controls[7].Set(ControlText.Get(CONTROL_CHAT), "T", "RETURN", "", CEID_Chat);
	Controls[8].Set(ControlText.Get(CONTROL_TEAMCHAT), "Y", "", "", CEID_TeamChat);
	Controls[9].Set(ControlText.Get(CONTROL_SCORES), "TAB", "", "", CEID_Scores);
	Controls[10].Set(ControlText.Get(CONTROL_GUNTOFRONT), "[", "", "", CEID_GunToFront);
	Controls[11].Set(ControlText.Get(CONTROL_GUNTOBACK), "]", "", "", CEID_GunToBack);
	Controls[12].Set(ControlText.Get(CONTROL_GUNTOCAM), "P", "", "", CEID_GunToCam);
	Controls[13].Set(ControlText.Get(CONTROL_FREELOOK), "CAPSLOCK", "Mouse R", "", CEID_FreeLook);
	Controls[14].Set(ControlText.Get(CONTROL_TURRETCAM), "Mouse M", "M", "", CEID_TurretCam);
	Controls[15].Set(ControlText.Get(CONTROL_TILTUP), "INSERT", ";", "", CEID_TiltCamUp);
	Controls[16].Set(ControlText.Get(CONTROL_TILTDOWN), "DELETE", "/", "", CEID_TiltCamDn);
	Controls[17].Set(ControlText.Get(CONTROL_LOOKLEFT), "END", "K", "", CEID_SpinCamLt);
	Controls[18].Set(ControlText.Get(CONTROL_LOOKRIGHT), "PAGEDOWN", "L", "", CEID_SpinCamRt);
	Controls[19].Set(ControlText.Get(CONTROL_LOOKBACK), "N", "", "", CEID_SpinCamBk);
	Controls[20].Set(ControlText.Get(CONTROL_RESET), "HOME", "", "", CEID_CamReset);
}

CGameSettings::CGameSettings()
{
	HowitzerTimeStart = 5000;
	HowitzerTimeScale = 0.2f;
	ShowMPH = 0;
	ServerRate = 10000;
	LimitTankTypes = 0;	//All, Steel, Liquid.
	CamStyle = 1;
	LogFileName = GetCommonAppDataDir();
	LogFileName.cat("tank.log");
	EnemySkill = 0.0f;
	AIPrefix = "AI-";
	TeamPlay = 0;
	TeamDamage = 0;
	TeamScores = 0;
	NumTeams = 0;
	HiFiSound = 1;
	MirroredMap = false;
	DediMapMode = 0;	//1 is Randam.
	TimeLimit = 20;
	FragLimit = 20;
	CoolDownTime = 15;
	StartDelay = 5;
	ClientPort = 12340;
	ServerPort = 12300;
	MasterPort = 12399;
	MaxClients = 32;
	DedicatedServer = false;
	DedicatedFPS = 10;
	MaxFPS = 60;
	SendHeartbeats = 20;	//Heartbeats to Masters.
	ServerName = "Local Tread Marks Game";
	ServerWebSite = "No Web Site";
	ServerInfo = "No Info";
	ServerCorrectedIP = "";
	MasterPingsPerSecond = 10;
	MasterPings = 10;
	AllowSinglePlayerJoins = 0;
	AnimFPS = 15;
	PlayMusic = false;
	SoundVol = 1.0f;
	MusicVol = 0.6f;
	Laps = 0;
	Deathmatch = 0;
	ClientRate = 5000;
	PlayerName = "Player";
	BypassIntro = 0;
	LogFileActive = false;
	AIAutoFillCount = 10;
	AIAutoFill = 1;
	AutoStart = false;
}

CGameState::CGameState()
{
	sendstatuscounter = 0;
	ViewAngle = 53.0f;
	CurrentLeader = 0;
	LastLeader = 0;
	HowitzerTime = 0;
	PauseGame = 0;
	LadderMode = false;	//This is on if we are in ladder play mode.
	CurDediMap = -1;	//So first map in cycle is 0.
	ReStartMap = 0;
	CoolDownCounter = 0;
	SomeoneWonRace = 1;
	DemoMode = false;	//This is set when we are in automatic demo mode, so we can restart the game and stay in demo mode.
	CountdownTimer = 0;
	KillSelf = 0;
	WritingAnim = false;
	TakeMapSnapshot = 0;	//When set, causes a one-off power of 2 snapshot of the game to be taken and saved with the current map's name.
	FPSCounter = false;
	AnimFrame = 0;
	AutoDrive = false;
	NumAITanks = 0;
	ToggleMenu = false;
	ActAsServer = true;
	Quit = false;
	ShowPlayerList = 0;
	PauseScreenOn = 0;
	SwitchSoundMode = true;
	TeamHash = 0;
	MasterPingIter = 0;
	SortMode = MSM_Players;
	FPS = 0.1f;
	NetCPSOut = 0;
	NetCPSIn = 0;	//Networking stats.
	LastNetBytesOut = 0;
	LastNetBytesIn = 0;
}

CInputState::CInputState()
{
	GunTo = 1;
	TurretCam = 1;
	PointCamUD = 0;
	PointCamLR = 0;
	PointCamBK = 0;
	ResetCam = 0;
	FreeLook = 0;
	TurnLR = 0;
	MoveUD = 0;
	AltUD = 0;
	StrafeLR = 0;
	TreadL = 0;
	TreadR = 0;
	SpaceBar = 0;
	MouseLR = 0.0f;
	MouseUD = 0.0f;
}

CTankGame::CTankGame()
{
	PlayerEnt = NULL;
	GodEnt = NULL;

	frames = 0;
	framems = 0;
	polyms = 0;
	voxelms = 0;
	thinkms = 0;
	blitms = 0;

	LastSecs = -1;

	ClientPacketRate = 0.05f;

	NumTankNames = 0;
	NextTankName = 0;	//Since we are shuffling them at load time now.

	CurChatLine = 0;
	ChatLineLen = 59;

	AITankTeam = 0;
	NumAvailTeams = 0;

	NumMusicFiles = 0;
	NextMusicFile = 0;

	NumTankTypes = 0;

	NumMaps = 0;
	NumDediMaps = 0;


	DbgBufLen = 1024;


	HeartbeatTime = 0;
	LastMasterPingTime = 0;

	FramesOver100MS = 0;

	MaxAllowedTeams = 999999;
}

int CTankGame::Announcement(const char *ann1, const char *ann2, EntityGID tank){
	if(ann1){
		ClassHash h1 = 0, h2 = 0;
		EntityTypeBase *et = VW.FindEntityType("sound", ann1);
		if(et) h1 = et->thash;
		if(ann2){
			et = VW.FindEntityType("sound", ann2);
			if(et) h2 = et->thash;
		}
		EntityBase *god = VW.FindRegisteredEntity("GOD");
		if(god){
			god->SetInt(ATT_ANNOUNCER_ID, tank);
			god->SetInt(ATT_ANNOUNCER_HASH2, h2);
			god->SetInt(ATT_CMD_ANNOUNCER, h1);
			return 1;
		}
	}
	return 0;
}

CStr CTankGame::MMSS(int secs){
	CStr m = String(secs / 60);
	if(m.len() < 2) m = "0" + m;
	CStr s = String(secs % 60);
	if(s.len() < 2) s = "0" + s;
	return m + ":" + s;
}
const char NumTail[10][10] = {"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"};
CStr NumberTail(int n){
	CStr tail = NumTail[n % 10];
	if(n > 10 && n < 20) tail = "th";
	return tail;
}

struct ActiveTeam{
	CStr name;
	ClassHash hash;
	int frags, tanks, deaths;
	ActiveTeam() : hash(0), frags(0), tanks(0), deaths(0) {};
};

void CTankGame::DoHUD()
{
	static EntityGID KphEntID = 0;
	static EntityGID KphEnt2ID = 0;
	static EntityGID FpsEntID = 0;
	static EntityGID Ammo1EntID = 0;
	static EntityGID Ammo2EntID = 0;
	static EntityGID HealthEntID = 0;
	static EntityGID FragsEntID = 0;
	static EntityGID ReloadEntID = 0;
	static EntityGID AmmoIconID = 0;
	static EntityGID HealthIconID = 0;
	static EntityGID FragsIconID = 0;
	static EntityGID TopLineBoxID = 0;
	static EntityGID SpeedBoxID = 0;
	static EntityGID WeaponBoxID = 0;
	static EntityGID IconBoxID = 0;

	static EntityGID LapID = 0;
	static EntityGID PlaceID = 0;
	static EntityGID ThisTimeID = 0;
	static EntityGID BestTimeID = 0;
	static EntityGID PingID = 0;

	static EntityGID LapIconID = 0;
	static EntityGID Clock1ID = 0;
	static EntityGID Clock2ID = 0;

	static EntityGID FlagStatusID = 0;

	EntityBase *boxent;

	char tempstr[1024];// FIXME: check length for anything over the network
	//
	if(!PlayerEnt) return;
	//
	float z = 2.0f; // same as near clip plane

	EntityBase *flagstatusent = VW.GetEntity(FlagStatusID);
	if(!flagstatusent)
	{
		flagstatusent = VW.AddEntity("text", "font1", CVec3(0.5f, 0.08f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, FLAG_TEXT_CENTERX, 0, 0);
		if(flagstatusent) FlagStatusID = flagstatusent->GID;
	}
	if(flagstatusent)
	{
		EntityTankGod *egod = (EntityTankGod*)VW.FindRegisteredEntity("TANKGOD");
		EntityBase *flagent = egod->ClosestFlag(PlayerEnt->Position, PlayerEnt->QueryInt(ATT_TEAMID), true);
		if(flagent)
		{
			if(flagent->QueryInt(ATT_WEAPON_STATUS)) // someone has your flag
				flagstatusent->SetString(ATT_TEXT_STRING, Text_GetLine(TEXT_FLAG_ENEMYHASFLAG));
			else if(PlayerEnt->QueryInt(ATT_FLAGID)) // you have someone elses flag
				flagstatusent->SetString(ATT_TEXT_STRING, Text_GetLine(TEXT_FLAG_RETURNTOBASE));
			else
				flagstatusent->SetString(ATT_TEXT_STRING, "");
		}
	}

	EntityBase *kphent = VW.GetEntity(KphEntID);
	if(!kphent){
		kphent = VW.AddEntity("text", "font1", CVec3(0.02f, 0.08f, z), CVec3(0.05f, 0.04f, 0.0f), NULL, 0, 0, 0, 0);
		if(kphent) KphEntID = kphent->GID;
	}
	if(kphent){
		int iSpeed = (int)(LengthVec3(PlayerEnt->Velocity) * 3.6f / (GameSettings.ShowMPH ? 1.603f : 1.0f));
		sprintf(tempstr, "%d", iSpeed);
		kphent->SetString(ATT_TEXT_STRING, tempstr);
	}
	float Length = strlen(tempstr);

	EntityBase *kphent2 = VW.GetEntity(KphEnt2ID);
	if(!kphent2){
		kphent2 = VW.AddEntity("text", "font1", CVec3(0.02f, 0.09f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
		if(kphent2) KphEnt2ID = kphent2->GID;
	}
	if(kphent2){
		kphent2->SetString(ATT_TEXT_STRING, Text_GetLine(GameSettings.ShowMPH ? TEXT_MPH : TEXT_KPH));
		kphent2->Position[0] = 0.02f + (Length * 0.05f);
	}
	boxent = VW.GetEntity(SpeedBoxID);
	if(!boxent)
	{
		boxent = VW.AddEntity("Chamfered2DBox", "basic", CVec3(0.01f, 0.07f, z + 0.1f), CVec3(0.235f, 0.06f, 0.0f), CVec3(0.0f, 0.0f, 0.0f), 0, 0, 0, 0);
		boxent->SetFloat(ATT_TEXT_OPACITY, 0.125f);
		if(boxent) SpeedBoxID = boxent->GID;
	}
	if(boxent)
	{
		boxent->Rotation[0] = 0.02f + (Length  * 0.05f) + (strlen(Text_GetLine(GameSettings.ShowMPH ? TEXT_MPH : TEXT_KPH)) * 0.02f);
	}
	//
	EntityBase *fpsent = VW.GetEntity(FpsEntID);
	if(!fpsent){
		fpsent = VW.AddEntity("text", "font1", CVec3(0.63f, 0.02f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
		if(fpsent) FpsEntID = fpsent->GID;
	}
	if(fpsent){
		sprintf(tempstr, Text_GetLine(TEXT_FPS), (int)GameState.FPS );
		if(GameState.FPSCounter) fpsent->SetString(ATT_TEXT_STRING, tempstr);
		else fpsent->SetString(ATT_TEXT_STRING, "");
	}
	//
	if(VW.GameMode() & GAMEMODE_DEATHMATCH)
	{
		int RaceTime = PlayerEnt->QueryInt(ATT_RACETIME);
		int Place = PlayerEnt->QueryInt(ATT_PLACE) - 1;

		EntityBase *PlaceEnt = VW.GetEntity(PlaceID);
		if(!PlaceEnt)
		{
			PlaceEnt = VW.AddEntity("text", "font1", CVec3(0.02f, 0.02f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
			if(PlaceEnt) PlaceID = PlaceEnt->GID;
		}
		if(PlaceEnt)
			PlaceEnt->SetString(ATT_TEXT_STRING, Text.Get(TEXT_1ST + (Place%100)));

		EntityBase *ClockIconEnt = VW.GetEntity(Clock1ID);
		if(!ClockIconEnt)
		{
			ClockIconEnt = VW.AddEntity("icon", "clock", CVec3(0.43f, 0.02f, z), CVec3(0.025f, 0.0333f, 0.0f), NULL, 0, 0, 0, 0);
			if(ClockIconEnt) Clock1ID = ClockIconEnt->GID;
		}

		EntityBase *ThisTimeEnt = VW.GetEntity(ThisTimeID);
		if(!ThisTimeEnt)
		{
			ThisTimeEnt = VW.AddEntity("text", "font1", CVec3(0.464f, 0.02f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
			if(ThisTimeEnt) ThisTimeID = ThisTimeEnt->GID;
		}
		if(ThisTimeEnt)
		{
			sprintf(tempstr, "%02d:%02d", (RaceTime/1000)/60,(RaceTime/1000)%60);
			ThisTimeEnt->SetString(ATT_TEXT_STRING, tempstr);
		}
	}
	else
	{
		int Place = PlayerEnt->QueryInt(ATT_PLACE) - 1;
		int LapTime = PlayerEnt->QueryInt(ATT_LAPTIME);
		int BestLapTime = PlayerEnt->QueryInt(ATT_BESTLAPTIME);

		EntityBase *LapIconEnt = VW.GetEntity(LapIconID);
		if(!LapIconEnt)
		{
			LapIconEnt = VW.AddEntity("icon", "lap", CVec3(0.02f, 0.02f, z), CVec3(0.025f, 0.0333f, 0.0f), NULL, 0, 0, 0, 0);
			if(LapIconEnt) LapIconID = LapIconEnt->GID;
		}
		EntityBase *LapTextEnt = VW.GetEntity(LapID);
		if(!LapTextEnt)
		{
			LapTextEnt = VW.AddEntity("text", "font1", CVec3(0.05f, 0.02f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
			if(LapTextEnt) LapID = LapTextEnt->GID;
		}
		if(LapTextEnt)
		{
			int Lap = PlayerEnt->QueryInt(ATT_LAPS);
			int NumLaps = 0;
			if(GodEnt)
				NumLaps = GodEnt->QueryInt(ATT_RACE_LAPS);
			sprintf(tempstr, "%d/%d", Lap+1, NumLaps);
			LapTextEnt->SetString(ATT_TEXT_STRING, tempstr);
		}

		EntityBase *PlaceEnt = VW.GetEntity(PlaceID);
		if(!PlaceEnt)
		{
			PlaceEnt = VW.AddEntity("text", "font1", CVec3(0.15f, 0.02f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
			if(PlaceEnt) PlaceID = PlaceEnt->GID;
		}
		if(PlaceEnt)
			PlaceEnt->SetString(ATT_TEXT_STRING, Text.Get(TEXT_1ST + (Place%100)));

		EntityBase *ClockIconEnt = VW.GetEntity(Clock1ID);
		if(!ClockIconEnt)
		{
			ClockIconEnt = VW.AddEntity("icon", "clock", CVec3(0.33f, 0.02f, z), CVec3(0.025f, 0.0333f, 0.0f), NULL, 0, 0, 0, 0);
			if(ClockIconEnt) Clock1ID = ClockIconEnt->GID;
		}

		EntityBase *ThisTimeEnt = VW.GetEntity(ThisTimeID);
		if(!ThisTimeEnt)
		{
			ThisTimeEnt = VW.AddEntity("text", "font1", CVec3(0.364f, 0.02f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
			if(ThisTimeEnt) ThisTimeID = ThisTimeEnt->GID;
		}
		if(ThisTimeEnt)
		{
			sprintf(tempstr, "%02d:%02d / %02d:%02d", (LapTime/1000)/60,(LapTime/1000)%60, (BestLapTime/1000)/60,(BestLapTime/1000)%60);
			ThisTimeEnt->SetString(ATT_TEXT_STRING, tempstr);
		}
	}
	if(VW.Net.IsClientActive())
	{
		int Ping = PlayerEnt->QueryInt(ATT_PING);

		EntityBase *PingEnt = VW.GetEntity(PingID);
		if(!PingEnt)
		{
			PingEnt = VW.AddEntity("text", "font1", CVec3(0.8f, 0.02f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
			if(PingEnt) PingID = PingEnt->GID;
		}
		if(PingEnt)
		{
			sprintf(tempstr, Text.Get(TEXT_PING), Ping);
			PingEnt->SetString(ATT_TEXT_STRING, tempstr);
		}
	}
	boxent = VW.GetEntity(TopLineBoxID);
	if(!boxent)
	{
		boxent = VW.AddEntity("Chamfered2DBox", "basic", CVec3(0.01f, 0.01f, z + 0.1f), CVec3(0.98f, 0.05f, 0.0f), CVec3(0.0f, 0.0f, 0.0f), 0, 0, 0, 0);
		boxent->SetFloat(ATT_TEXT_OPACITY, 0.125f);
		if(boxent) TopLineBoxID = boxent->GID;
	}
	//
	int weaponammo = -1;
	EntityBase *ammoent = VW.GetEntity(Ammo2EntID);
	if(!ammoent){
		ammoent = VW.AddEntity("text", "font1", CVec3(0.02f, 0.15f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
		if(ammoent) Ammo2EntID = ammoent->GID;
	}
	if(ammoent){
		sprintf(tempstr, Text_GetLine(TEXT_WEAPONMAIN));
		EntityBase *we = VW.GetEntity(PlayerEnt->QueryInt(ATT_WEAPON_ENTITY));
		if(we){
			// FIXME: check length
			weaponammo = we->QueryInt(ATT_WEAPON_AMMO) * we->QueryInt(ATT_WEAPON_MULT);
			if(we->TypePtr->nameid != -1) {
				// Russ: New way, 'nameid' in ENT is an index into Names.TXT for localised names
				sprintf(tempstr, Names.Get(we->TypePtr->nameid) );
			} else {
				// Russ: Old way, should only be used for debugging and for mods
				if (we->TypePtr->dname.len() > 250) {
					sprintf(tempstr, "FIXME!");
				} else {
					sprintf(tempstr, we->TypePtr->dname.get());
				}
			}
		}
		ammoent->SetString(ATT_TEXT_STRING, tempstr);
	}
	boxent = VW.GetEntity(WeaponBoxID);
	if(!boxent)
	{
		boxent = VW.AddEntity("Chamfered2DBox", "basic", CVec3(0.01f, 0.14f, z + 0.1f), CVec3(0.02f, 0.05f, 0.0f), CVec3(0.0f, 0.0f, 0.0f), 0, 0, 0, 0);
		boxent->SetFloat(ATT_TEXT_OPACITY, 0.125f);
		if(boxent) WeaponBoxID = boxent->GID;
	}
	if(boxent)
	{
		float Length = strlen(tempstr);
		boxent->Rotation[0] = 0.02f + (Length * 0.02f);
	}
	//
	boxent = VW.GetEntity(IconBoxID);
	if(!boxent)
	{
		boxent = VW.AddEntity("Chamfered2DBox", "basic", CVec3(0.01f, 0.20f, z + 0.1f), CVec3(0.16f, 0.20875f, 0.0f), CVec3(0.0f, 0.0f, 0.0f), 0, 0, 0, 0);
		boxent->SetFloat(ATT_TEXT_OPACITY, 0.125f);
		if(boxent) IconBoxID = boxent->GID;
	}

	EntityBase *ammoiconent = VW.GetEntity(AmmoIconID);
	if(!ammoiconent)
	{
		ammoiconent = VW.AddEntity("icon", "ammo", CVec3(0.02f, 0.2f, z), CVec3(0.05f, 0.0665f, 0.0f), NULL, 0, 0, 0, 0);
		if(ammoiconent) AmmoIconID = ammoiconent->GID;
	}
	ammoent = VW.GetEntity(Ammo1EntID);
	if(!ammoent){
		ammoent = VW.AddEntity("text", "font1", CVec3(0.085f, 0.21825f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
		if(ammoent) Ammo1EntID = ammoent->GID;
	}
	if(ammoent){
		sprintf(tempstr, "%d",
			MAX(0, weaponammo >= 0 ? weaponammo : PlayerEnt->QueryInt(ATT_AMMO)) );
		ammoent->SetString(ATT_TEXT_STRING, tempstr);
	}
	//
	EntityBase *healthiconent = VW.GetEntity(HealthIconID);
	if(!healthiconent)
	{
		healthiconent = VW.AddEntity("icon", "health", CVec3(0.02f, 0.26625f, z), CVec3(0.05f, 0.0665f, 0.0f), NULL, 0, 0, 0, 0);
		if(healthiconent) HealthIconID = healthiconent->GID;
	}
	EntityBase *healthent = VW.GetEntity(HealthEntID);
	if(!healthent){
		healthent = VW.AddEntity("text", "font1", CVec3(0.085f, 0.2845f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
		if(healthent) HealthEntID = healthent->GID;
	}
	if(healthent){
		sprintf(tempstr, "%d", (int)(PlayerEnt->QueryFloat(ATT_HEALTH) * 100.0 + 0.999) );
		healthent->SetString(ATT_TEXT_STRING, tempstr);
	}
	//
	EntityBase *fragsiconent = VW.GetEntity(FragsIconID);
	if(!fragsiconent)
	{
		fragsiconent = VW.AddEntity("icon", "frags", CVec3(0.02f, 0.33275f, z), CVec3(0.05f, 0.0665f, 0.0f), NULL, 0, 0, 0, 0);
		if(fragsiconent) FragsIconID = fragsiconent->GID;
	}
	EntityBase *fragsent = VW.GetEntity(FragsEntID);
	if(!fragsent){
		fragsent = VW.AddEntity("text", "font1", CVec3(0.085f, 0.351f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
		if(fragsent) FragsEntID = fragsent->GID;
	}
	if(fragsent){
		sprintf(tempstr, "%d", PlayerEnt->QueryInt(ATT_FRAGS) );
		fragsent->SetString(ATT_TEXT_STRING, tempstr);
	}
	if (PlayerEnt)
	{
		EntityBase *reloadingent = VW.GetEntity(ReloadEntID);
		if(!reloadingent)
		{
			reloadingent = VW.AddEntity("text", "font1", CVec3(0.02f, 0.4175f, z), CVec3(0.02f, 0.03f, 0.0f), NULL, 0, 0, 0, 0);
			ReloadEntID = reloadingent->GID;
		}
		if(reloadingent)
		{
			if(((EntityRacetank*)PlayerEnt)->ReadyToFire())
				reloadingent->SetString(ATT_TEXT_STRING, "");
			else
				reloadingent->SetString(ATT_TEXT_STRING, Text_GetLine(TEXT_RELOADING));
		}
	}
	//
	//Scores list/frag list.
	if((GameState.ShowPlayerList || VW.GameMode() & GAMEMODE_SHOWSCORES) && GodEnt){
		EntityBase *e, *t;

		CStr	MapName;

		if(VW.MapCfg.FindKey("MapName"))
			VW.MapCfg.GetStringVal(MapName);

		t = VW.AddEntity("text", "font1", CVec3(0.3f, 0.07f, z), CVec3(0.015f, 0.0225f, 0), CVec3(1, 1, 0.5f), 1, 0, 0, 0);
		if(t) t->SetString(ATT_TEXT_STRING, MapName);

		t = VW.AddEntity("text", "font1", CVec3(0.3f, 0.10f, z), CVec3(0.015f, 0.0225f, 0), CVec3(1, 1, 0.5f), 1, 0, 0, 0);
		if(t) t->SetString(ATT_TEXT_STRING, Text_GetLine(TEXT_HUDSTATS));
		if(VW.GameMode() & GAMEMODE_TEAMPLAY){
			//Team scoreboard.
			ActiveTeam at[MaxTeams + 1];
			at[0].name = "No Team";
			int nat = 1, i, j, k, n;
			//Enumerate active teams.
			for(n = 0; e = GodEnt->TankByIndex(n); n++){
			//	int i = 0;
				for(i = 0; i < nat; i++){
					if(e->QueryInt(ATT_TEAMID) == at[i].hash){
						if(VW.GameMode() & GAMEMODE_TEAMSCORES)
						{
							at[i].frags += e->QueryInt(ATT_FRAGS);
							at[i].deaths += e->QueryInt(ATT_DEATHS);
							at[i].tanks++;
						}
						else
						{
							at[i].frags = max(e->QueryInt(ATT_FRAGS), at[i].frags);
						}
						break;
					}
				}
				if(i >= nat && nat < (MaxTeams + 1)){	//Team not found, must add new team.
					EntityTypeBase *et = VW.FindEntityType(e->QueryInt(ATT_TEAMID));
					if(et){
						if(VW.GameMode() & GAMEMODE_TEAMSCORES)
						{
							at[nat].name = "Team " + et->dname;
							at[nat].frags += e->QueryInt(ATT_FRAGS);
							at[nat].deaths += e->QueryInt(ATT_DEATHS);
							at[nat].hash = e->QueryInt(ATT_TEAMID);
							at[nat].tanks++;
						}
						else
						{
							at[nat].name = "Team " + et->dname;
							at[nat].frags = e->QueryInt(ATT_FRAGS);
							at[nat].deaths = 0;
							at[nat].hash = e->QueryInt(ATT_TEAMID);
							at[nat].tanks++;
						}
						nat++;
					}
				}
			}
			//Sort teams.
			ActiveTeam tat;
			for(j = 0; j < nat; j++){	//Eeek, bubble sort!
				for(k = 0; k < (nat - 1); k++){
					if(at[k].frags < at[k + 1].frags || at[k].tanks <= 0){	//Need to swap.
						tat = at[k];
						at[k] = at[k + 1];
						at[k + 1] = tat;
					}
				}
			}
			//Display scores.
			k = 1;	//Line counter.
			for(i = 0; i < nat; i++){
				if(at[i].tanks > 0){
					t = VW.AddEntity("text", "font1", CVec3(0.3f, 0.10f + 0.03f * (float)(k++), z), CVec3(0.015f, 0.0225f, 0), CVec3(1, 1, 1), 1, 0, 0, 0);
					if(t){
						int frags = at[i].frags;
						int deaths = at[i].deaths;

						if(VW.GameMode() & GAMEMODE_TEAMSCORES)
							sprintf(tempstr, Text_GetLine(TEXT_HUDSTATSTEAMLINE), Text.Get(TEXT_1ST + (i%100)), frags, deaths, at[i].name.get());
						else
							sprintf(tempstr, Text_GetLine(TEXT_HUDSTATSTEAMLINE2), at[i].name.get());

						t->SetString(ATT_TEXT_STRING, tempstr);
					}
					for(n = 0; e = GodEnt->TankByIndex(n); n++){
						if(e->QueryInt(ATT_TEAMID) == at[i].hash){	//Only display tanks that are in this team.
							Vec3 color = {1, 0.7f, 0};
							if(e->GID == PlayerEnt->GID) color[1] = 1.0;
							t = VW.AddEntity("text", "font1", CVec3(0.3f, 0.10f + 0.03f * (float)(k++), z), CVec3(0.015f, 0.0225f, 0), color, 1, 0, 0, 0);
							if(t){
								int frags = e->QueryInt(ATT_FRAGS);
								int deaths = e->QueryInt(ATT_DEATHS);
								int ping = e->QueryInt(ATT_PING);

								sprintf(tempstr, Text_GetLine(TEXT_HUDSTATSLINE), "", frags, deaths, ping, e->QueryString(ATT_NAME).get());
								t->SetString(ATT_TEXT_STRING, tempstr);
							}
						}
					}
				}
			}
			//
		}else{	//Not team mode.
			for(int n = 0; e = GodEnt->TankByIndex(n); n++){
				Vec3 color = {1, 0.7f, 0};
				if(e->GID == PlayerEnt->GID){
					color[1] = 1.0;
				}
				t = VW.AddEntity("text", "font1", CVec3(0.3f, 0.10f + 0.03f * (float)(n + 1), z), CVec3(0.015f, 0.0225f, 0), color, 1, 0, 0, 0);
				if(t){
					int frags = e->QueryInt(ATT_FRAGS);
					int deaths = e->QueryInt(ATT_DEATHS);
					int ping = e->QueryInt(ATT_PING);

					sprintf(tempstr, Text_GetLine(TEXT_HUDSTATSLINE), Text.Get(TEXT_1ST + (n%100)), frags, deaths, ping, e->QueryString(ATT_NAME).get());
					t->SetString(ATT_TEXT_STRING, tempstr);
				}
			}
		}
	}
}


float JoyAxes[K_MAXJOYAXES+2]; // 2 extra for the hat

void CTankGame::DoInput(){
	//Handle joystick.
	if(GameSettings.InputSettings.UseJoystick){
		if(InputState.input.InitController(GameSettings.InputSettings.StickID))
		{
			int i;
			InputState.input.Update();
			for(i = 0; i < K_MAXJOYAXES; i++)
			{
				JoyAxes[i] = InputState.input.GetAxis(i);
			}
			JoyAxes[K_MAXJOYAXES] = InputState.input.GetHatX();
			JoyAxes[K_MAXJOYAXES+1] = InputState.input.GetHatY();

			for(i = 0; i < K_MAXJOYBUTTONS; i++)
			{
				if(InputState.input.ButtonGoingDown(i))
					VW.Ififo.Set("Joy " + String(i + 1), 1);
				else if(InputState.input.ButtonGoingUp(i))
					VW.Ififo.Set("Joy " + String(i + 1), 0);
			}
		}
	}
	//
	CStr in;
	int type = 0;
	float ex = 0;
	while(VW.Ififo.Get(in, type, ex)){	//Suck input queue dry.
		if(type >= 0){
			LastControl = in;	//type of -1 is for joy/mouse positions, not individual keys.
			//
			ControlEntryID id = CEID_None;
			//
			if(in == "Mouse L"){
				if(type){
					VW.InputMouseClicked(1);
					VW.InputMouseButton(1);
				}else{
					VW.InputMouseReleased(1);
					VW.InputMouseButton(0);
				}
			}
			for(int i = 0; i < NumControls; i++){
				if((strcasecmp(in, GameSettings.InputSettings.Controls[i].ctrl1) == 0) || (strcasecmp(in, GameSettings.InputSettings.Controls[i].ctrl2) == 0) || (strcasecmp(in, GameSettings.InputSettings.Controls[i].ctrl3) == 0))
				{
					id = GameSettings.InputSettings.Controls[i].ctlid;
					break;
				}
			}
			if(VW.GetChatMode() == 0){
				if(type){	//Control Down.
					switch(id){
					case CEID_Left : InputState.TurnLR = -1; break;
					case CEID_Right : InputState.TurnLR = 1; break;
					case CEID_Up : InputState.MoveUD = 1; break;
					case CEID_Down : InputState.MoveUD = -1; break;
					case CEID_TLeft :
						InputState.StrafeLR = -1;
						InputState.GunTo = 0;
						break;
					case CEID_TRight :
						InputState.StrafeLR = 1;
						InputState.GunTo = 0;
						break;
					case CEID_Fire : if(CurrentMenu == MP_None) InputState.SpaceBar = 1; break;
					case CEID_Chat :
					case CEID_TeamChat :
						if(CurrentMenu == MP_None){
							EntityBase *e = VW.FindRegisteredEntity("GOD");
							if(e) e->SetInt((id == CEID_Chat ? ATT_CMD_CHAT : ATT_CMD_TEAMCHAT), 1);
						}
						break;
					case CEID_FreeLook :
						if(CurrentMenu == MP_None){
							InputState.ResetCam = 1;
							InputState.FreeLook = 1;
							VW.StatusMessage(Text.Get(TEXT_FREELOOKON), STATUS_PRI_NORMAL);
						}
						break;
					case CEID_Scores :
						{
							EntityBase	*egod = VW.FindRegisteredEntity("GOD");
							if(egod)
								egod->SetInt(ATT_CMD_REFRESHSTAT, 0);
							GameState.ShowPlayerList = 1;
							break;
						}
					case CEID_GunToFront : InputState.GunTo = 1; break;
					case CEID_GunToBack : InputState.GunTo = 2; break;
					case CEID_GunToCam : InputState.GunTo = 3; break;
					case CEID_CamReset : InputState.ResetCam = 1; break;
					case CEID_TurretCam:
						InputState.TurretCam = !InputState.TurretCam;
						if(InputState.TurretCam)
							VW.StatusMessage(Text.Get(TEXT_CAMATTACHTOTURRET), STATUS_PRI_NORMAL);
						else
							VW.StatusMessage(Text.Get(TEXT_CAMATTACHEDTOFRONT), STATUS_PRI_NORMAL);
						break;
					case CEID_TiltCamUp: InputState.PointCamUD = 1; break;
					case CEID_TiltCamDn: InputState.PointCamUD = -1; break;
					case CEID_SpinCamLt:
						InputState.PointCamLR = -1;
						break;
					case CEID_SpinCamRt:
						InputState.PointCamLR = 1;
						break;
					case CEID_SpinCamBk:
						InputState.PointCamBK = 1;
						break;
					}
				}else{	//Control Up.
					switch(id){
					case CEID_Left : if(InputState.TurnLR < 0) InputState.TurnLR = 0; break;
					case CEID_Right : if(InputState.TurnLR > 0) InputState.TurnLR = 0; break;
					case CEID_Up : if(InputState.MoveUD > 0) InputState.MoveUD = 0; break;
					case CEID_Down : if(InputState.MoveUD < 0) InputState.MoveUD = 0; break;
					case CEID_TLeft : if(InputState.StrafeLR < 0) InputState.StrafeLR = 0; break;
					case CEID_TRight : if(InputState.StrafeLR > 0) InputState.StrafeLR = 0; break;
					case CEID_Fire : InputState.SpaceBar = 0; break;
					case CEID_Scores:
						{
							EntityBase	*egod = VW.FindRegisteredEntity("GOD");
							if(egod)
								egod->SetInt(ATT_CMD_HIDESTAT, 0);
							GameState.ShowPlayerList = 0;
							break;
						}
					case CEID_FreeLook :
						if(CurrentMenu == MP_None){
							InputState.FreeLook = 0;
							VW.StatusMessage(Text.Get(TEXT_FREELOOKOFF), STATUS_PRI_NORMAL);
						}
						break;
					case CEID_CamReset : InputState.ResetCam = 0; break;
					case CEID_TiltCamUp: InputState.PointCamUD = 0; break;
					case CEID_TiltCamDn: InputState.PointCamUD = 0; break;
					case CEID_SpinCamLt: InputState.PointCamLR = 0; break;
					case CEID_SpinCamRt: InputState.PointCamLR = 0; break;
					case CEID_SpinCamBk: InputState.PointCamBK = 0; break;
					}
				}
			}
		}else{	//Special inputs, joy pos, mouse pos, etc.
		}
	}
};

inline float AveHeightVal(float x, float y)
{
	return max(4.0f, CTankGame::Get().VW.Map.FGetI(x, y));
}

inline bool PointsNotInTerrain(C3Point pt1, C3Point pt2, C3Point pt3)
{
	if ((AveHeightVal(pt1.x(), -pt1.z()) > pt1.y()) || (AveHeightVal(pt2.x(), -pt2.z()) > pt2.y()) || (AveHeightVal(pt3.x(), -pt3.z()) > pt3.y()))
		return false;
	return true;
}

#ifndef WIN32
inline bool _finite(float f)
{
    if(f != std::numeric_limits<float>::infinity() && f != -std::numeric_limits<float>::infinity()
        && f != std::numeric_limits<float>::quiet_NaN() && f != std::numeric_limits<float>::signaling_NaN())
        return true;
    else
        return false;
}
#endif

void CTankGame::DoPlayerAndCam()
{
	static float			CameraHeading = 0.0f;
	static float			CameraPitch = 0.0f;
	static unsigned int		LastClientPacketTime = 0;
	Camera					tCam;
	float					MUD = 0.0f;
	float					MLR = 0.0f;
	float					SLR = 0.0f;
	float					CamLR = 0.0f;
	float					CamUD = 0.0f;
	int						Fire = 0;

	if(!PlayerEnt)
		return;

	if(GameSettings.InputSettings.UseJoystick)
	{
		for(int i = 0; i < K_MAXJOYAXES + 2; i++)
		{
			switch(GameSettings.InputSettings.AxisMapping[i])
			{
			case 0: // unmapped
				break;
			case 1: // Tank Left / Right
				MLR += fabsf(JoyAxes[i]) > GameSettings.InputSettings.DeadZone ? JoyAxes[i] : 0;
				break;
			case 2: // Tank Forward / Backwards
				MUD -= fabsf(JoyAxes[i]) > GameSettings.InputSettings.DeadZone ? JoyAxes[i] : 0;
				break;
			case 3: // Turret Left / Right
				SLR += fabsf(JoyAxes[i]) > GameSettings.InputSettings.DeadZone ? JoyAxes[i] : 0;
				break;
			case 4: // Camera Left / Right
				CamLR += fabsf(JoyAxes[i]) > GameSettings.InputSettings.DeadZone ? JoyAxes[i] : 0;
				break;
			case 5: // Camera Up / Down
				CamUD += fabsf(JoyAxes[i]) > GameSettings.InputSettings.DeadZone ? JoyAxes[i] : 0;
				break;
			}
		}
	}
	MUD += InputState.MoveUD;
	MLR += InputState.TurnLR;

	if(GameSettings.InputSettings.MouseEnabled && !InputState.FreeLook) // turn turret based on mouse
		SLR += InputState.MouseLR;

	SLR += (float)InputState.StrafeLR; // turn turret based on keyboard / control input

	if(fabsf(SLR) > 0.005) // any user controled turret movement cancels GunTo
		InputState.GunTo = 0;

	switch(InputState.GunTo) // auto-turning
	{
	case 1:	// Go to front
		{
			float a = NormRot(-PlayerEnt->QueryFloat(ATT_TURRETHEADING));
			SLR = CLAMP(a * 6.0f, -1.0f, 1.0f);
			break;
		}
	case 2:	// Go to back
		{
			float a = NormRot(M_PI - PlayerEnt->QueryFloat(ATT_TURRETHEADING));
			SLR = CLAMP(a * 6.0f, -1.0f, 1.0f);
			break;
		}
	case 3:	// Go to camera
		{
			float a = NormRot(CameraHeading - PlayerEnt->QueryFloat(ATT_TURRETHEADING));
			SLR = CLAMP(a * 6.0f, -1.0f, 1.0f);
			break;
		}
	}

	if (InputState.FreeLook) // camera is free to spin around
	{
		CamLR += InputState.MouseLR;
		CamUD += InputState.MouseUD;
	}

	if(InputState.PointCamLR) // look 90 degrees left or 90 degrees right
		CamLR = (PIOVER2 * float(InputState.PointCamLR)) - CameraHeading;
	else
		CamLR *= VW.FrameFrac();
	if(InputState.PointCamBK)
		CamLR = PI - CameraHeading;

	CamUD += float(InputState.PointCamUD);

	if(InputState.SpaceBar)
		Fire = 1;

	// Update client entity information both locally and on the net if data rate allows it
	if(GameState.ActAsServer || abs(float(VW.Time() - LastClientPacketTime)) >= (ClientPacketRate * 1000.0f))
	{
		if(abs(float(VW.Time() - LastClientPacketTime)) > (ClientPacketRate * 1000.0f * 2.0f) || LastClientPacketTime > VW.Time())
			LastClientPacketTime = VW.Time();
		else
			LastClientPacketTime += (ClientPacketRate * 1000.0f);

		PlayerEnt->SetFloat(ATT_ACCEL, MUD);
		PlayerEnt->SetFloat(ATT_STEER, MLR);
		PlayerEnt->SetFloat(ATT_LACCEL, InputState.TreadL);
		PlayerEnt->SetFloat(ATT_RACCEL, InputState.TreadR);
		PlayerEnt->SetInt(ATT_TANKMODE, 0);
		PlayerEnt->SetFloat(ATT_TURRETSTEER, SLR);

		if(GameState.ActAsServer)
		{
			PlayerEnt->SetInt(ATT_FIRE, Fire);
		}
		else
		{
			BitPacker<128> bp;
			//
			int chat = 0;
			if(GodEnt && GodEnt->QueryInt(ATT_CHATLINELEN) > 4) chat = 1;	//Must have 4 chat chars to get chat symbol.
			//
			bp.PackInt(BYTE_HEAD_CONTROLS, 8);
			bp.PackFloatInterval(MUD, -2, 2, 10);
			bp.PackFloatInterval(MLR, -2, 2, 10);
			bp.PackUInt((chat ? 0 : Fire), 1);	//No weapons fire while chatting!
			bp.PackFloatInterval(SLR, -2, 2, 10);
			//
			if(GameState.sendstatuscounter <= 0)
			{
				EntityTypeBase *et = VW.FindEntityType("insignia", GameSettings.InsigniaType);
				ClassHash insig2 = 0;// = et->thash;
				if(et)
				{
					insig2 = et->thash;
				}
				bp.PackUInt(1, 1);
				bp.PackUInt(insig2, 32);
				bp.PackUInt(GameState.TeamHash, 32);
				GameState.sendstatuscounter = SEND_STATUS_TIME;
			}
			else
			{
				bp.PackUInt(0, 1);
			}
			bp.PackUInt(chat, 1);	//Send chatting flag.
			VW.Net.QueueSendServer(TM_Unreliable, (char*)bp.Data(), bp.BytesUsed());
		}
	}
	GameState.sendstatuscounter -= VW.FrameTime();
	//
	if(PlayerEnt && GodEnt && PlayerEnt->QueryInt(ATT_LAPS) >= GodEnt->QueryInt(ATT_RACE_LAPS)){
		if(!(VW.GameMode() & GAMEMODE_SHOWSCORES)){	//Finished race, but before final scoreboard.
			int Place = PlayerEnt->QueryInt(ATT_PLACE);
			EntityBase *e = VW.AddEntity("text", "font1",
				CVec3(0.5f, 0.5f, 5.0f), CVec3(0.07f, 0.1f, 0),
				NULL, 1, FLAG_TEXT_CENTERX, 0, 0);
			if(e) e->SetString(ATT_TEXT_STRING, String(Place) + NumberTail(Place) + " Place!");
			// TODO: string table
		}
	}

	float HowitzerAdd = 0.0f;	//Added rotation for HowitzerTime.
	if(GameState.HowitzerTime > 0)
		HowitzerAdd = ((float)GameState.HowitzerTime / (float)max(1, GameSettings.HowitzerTimeStart)) * PI2;

	if(InputState.ResetCam)
	{
		if(InputState.TurretCam)
			CameraHeading = PlayerEnt->QueryFloat(ATT_TURRETHEADING);
		else
			CameraHeading = 0.0f;
		CameraPitch = 0.0f;
		InputState.ResetCam = 0;
	}

	float t = 0.0f;
	static Vec3 tmpPos = {0,200,0};
	for(int i = 0; i < 3; i++)
	{
		tmpPos[i] = LERP(tmpPos[i], PlayerEnt->Position[i], min(VW.FrameFrac() * 40.0f, 0.95f));
	}
	const float mincamdist = 8.75f;
	const float maxcamdist = mincamdist + 8.5f + (GameState.HowitzerTime > 0 ? 24 : 0);
	const float mincamheight = 4.0f;
	const float maxcamheight = 6.0f;
	const float minpitch = -M_PI * 0.325f;
	const float maxpitch = M_PI * 0.325f;

	float angle = 0.0f;
	float camheightdist;
	float camheight = mincamheight;
	float fAspect = float(GenBuf.Width()) / float(GenBuf.Height());
	float viewangle = min(60.0f, 50.0f + (LengthVec3(PlayerEnt->Velocity) * 0.5f));
	GameState.ViewAngle = viewangle + DAMP(GameState.ViewAngle - viewangle, 0.1f, VW.FrameFrac());	//Damp to wanted viewangle - 10% in one second.
	float vertviewangle = 2.0f * RAD2DEG * (float)atan((float)tan(DEG2RAD * (GameState.ViewAngle / 2.0f)) / fAspect);

	// Near Plane
	const float fNear = 2.0f; // This value is hardcoded as 2.0f in GLTerainRender.cpp so I'm doing the same thing here
	float Right = (float)tan(DEG2RAD * GameState.ViewAngle / 2.0f) * fNear;
	float Left = -Right;
	float Bottom = (float)tan(DEG2RAD * vertviewangle / 2.0f) * -fNear;
	C3Point botleft, botright, botcenter;

	botleft.set(Left, Bottom, -fNear);
	botright.set(Right, Bottom, -fNear);
	botcenter.set(0.0f, Bottom, -fNear);

	if(fabsf(CamLR) > 0.005f || InputState.PointCamLR || InputState.PointCamBK || InputState.FreeLook) // user is turning the camera
		CameraHeading = NormRot(CameraHeading + CamLR);
	else
	{
		if(InputState.TurretCam) // look down the turret
			CameraHeading = PlayerEnt->QueryFloat(ATT_TURRETHEADING);
		else // look down the front of the tank
			CameraHeading = 0;
	}
	angle = NormRot(CameraHeading + PlayerEnt->Rotation[1] + HowitzerAdd);
	if(fabsf(CamUD) > 0.005f || InputState.FreeLook) // user is controlling pitch
	{
		CameraPitch += NormRot(VW.FrameFrac() * CamUD);
	}
	else if(!InputState.FreeLook) // handle different camera pitching methods if not in FreeLook
	{
		switch(GameSettings.CamStyle)
		{
			case 0:
			{
				float fFrontDist = 10.0f;
				float fTipOffset = 4.0f;
				static float fLastTipHeight = fTipOffset;
				float fTipX = tmpPos[0] + (sin(angle) * fFrontDist);
				float fTipY = tmpPos[2] + (cos(angle) * fFrontDist);
				float fTipHeight = (VW.Map.FGetI(fTipX, -fTipY) + VW.Map.FGetI(fTipX+1, -fTipY) + VW.Map.FGetI(fTipX-1, -fTipY) +
									VW.Map.FGetI(fTipX, -(fTipY-1)) + VW.Map.FGetI(fTipX, -(fTipY+1)) + VW.Map.FGetI(fTipX+1, -(fTipY+1)) +
									VW.Map.FGetI(fTipX+1, -(fTipY-1)) + VW.Map.FGetI(fTipX-1, -(fTipY+1)) + VW.Map.FGetI(fTipX-1, -(fTipY-1))) / 9.0f;
				fTipHeight += fTipOffset;

				fLastTipHeight = fTipHeight = (fLastTipHeight * 0.9f) + (fTipHeight * 0.1f);

				float fAnchortoTip = tmpPos[1] + camheight - fTipHeight;

				CameraPitch = -atan(fAnchortoTip / fFrontDist);

				// now adjust it if we're airborn
				float fDistAboveGround = tmpPos[1] - VW.Map.FGetI(tmpPos[0], -tmpPos[2]);
				float fNormalized = max(0.0f, min(fDistAboveGround / 5.0f, 1.0f));

				CameraPitch *= 1.0f - fNormalized;
				break;
			}
			case 1:
			{
				Mat3 rotm, trotm, trotm2;
				Vec3 trot;
				CopyVec3(PlayerEnt->Rotation, trot);
				Rot3ToMat3(trot, trotm);
				SetVec3(0, 0, 0, trot);
				if(InputState.TurretCam)
					trot[1] = PlayerEnt->QueryFloat(ATT_TURRETHEADING) + HowitzerAdd;
				Rot3ToMat3(trot, trotm2);
				Mat3MulMat3(trotm2, trotm, rotm);
				CameraPitch = atan(rotm[2][1] / std::max(0.0001f, LengthVec3(rotm[2])));
				break;
			}
			case 2:
				CameraPitch = 0.0f;
				break;
		}
	}

	CameraPitch = MAX(minpitch, MIN(maxpitch, CameraPitch));

	float fParam = 1.0f;
	float camdist;
	float tmpPitch;

	C4Matrix tmp3;
	tmp3.make_rot(C3Vector(0.0f, 1.0f, 0.0f), (angle) * RAD2DEG);
	const float SinAngle = sin(angle);
	const float CosAngle = cos(angle);
	while(fParam > -0.0005f)
	{
		C3Point tmpbotleft, tmpbotright, tmpbotcenter;

		camdist = mincamdist + (maxcamdist - mincamdist) * fParam;
		tmpPitch = CameraPitch - (CameraPitch - minpitch) * (1.0f - fParam);
		camheight = maxcamheight - (maxcamheight - mincamheight) * fParam;

		camheightdist = -tan(tmpPitch);

		C4Matrix CamMatrix, tmp2;
		tmp2.make_rot(C3Vector(1.0f, 0.0f, 0.0f), -tmpPitch * RAD2DEG);

		CamMatrix = tmp2 * tmp3;
		CamMatrix[3][0] = tmpPos[0] - SinAngle * camdist;
		CamMatrix[3][1] = tmpPos[1] + camheight + (camdist * camheightdist);
		CamMatrix[3][2] = tmpPos[2] - CosAngle * camdist;

		tmpbotleft = botleft * CamMatrix;
		tmpbotright = botright * CamMatrix;
		tmpbotcenter = botcenter * CamMatrix;

		if(PointsNotInTerrain(tmpbotleft, tmpbotright, tmpbotcenter))
		{
			// try to make sure the tank isn't obscured by the terrain
			float tmpdist = camdist;
			do
			{
				tmpdist -= 0.075f;

				CamMatrix[3][0] = tmpPos[0] - SinAngle * tmpdist;
				CamMatrix[3][1] = tmpPos[1] + camheight + (tmpdist * camheightdist);
				CamMatrix[3][2] = tmpPos[2] - CosAngle * tmpdist;

				tmpbotleft = botleft * CamMatrix;
				tmpbotright = botright * CamMatrix;
				tmpbotcenter = botcenter * CamMatrix;
			}while((tmpdist > mincamdist) && PointsNotInTerrain(tmpbotleft, tmpbotright, tmpbotcenter));

			if(tmpdist < mincamdist)
				break; // we made it to the min cam dist without hitting the terrain
		}

		fParam -= 0.015f;
	}

	if(!(fabsf(CamUD) > 0.005f || InputState.FreeLook)) // user is controlling pitch
		CameraPitch = tmpPitch;

	if (fParam <= -0.0005f) // worst case scenario .. center the camera on the player pos and move the camera up until its out of terrain
	{
		C3Point tmpbotleft, tmpbotright, tmpbotcenter;
		camdist = 0.0f;
		camheight = mincamdist;
		do
		{
			C4Matrix CamMatrix, tmp1, tmp2, tmp3;

			tmp1.make_trans(C3Vector((tmpPos[0] - sin(angle) * camdist),
				(tmpPos[1] + camheight + (camdist * camheightdist)),
				(tmpPos[2] - cos(angle) * camdist)));
			tmp2.make_rot(C3Vector(1.0f, 0.0f, 0.0f), -CameraPitch * RAD2DEG);
			tmp3.make_rot(C3Vector(0.0f, 1.0f, 0.0f), (angle) * RAD2DEG);

			CamMatrix = tmp2 * tmp3 * tmp1;

			tmpbotleft = botleft * CamMatrix;
			tmpbotright = botright * CamMatrix;
			tmpbotcenter = botcenter * CamMatrix;

			camheight += 0.1f;
		} while (!PointsNotInTerrain(tmpbotleft, tmpbotright, tmpbotcenter));
	}

	tCam.SetAll(
		tmpPos[0] - sin(angle) * camdist,
		tmpPos[1] + camheight + (camdist * camheightdist),
		tmpPos[2] - cos(angle) * camdist,
		0.0f,
		(angle) * RAD2DEG,
		0.0f,
		tan(CameraPitch), 0.0f, GameSettings.GraphicsSettings.GLBWIDTH);

	if(VW.Time() == 0)
		t = 1.0f;
	else
		t = MIN((float)VW.FrameTime() / 50.0f, 1.0f);
	//
	VW.Cam.viewplane = 1.0f / tan(DEG2RAD * GameState.ViewAngle / 2.0f) * (float)GenBuf.Width() * 0.5f;
	VW.Cam.SetView(GenBuf.Width(), GenBuf.Height(), GameSettings.GraphicsSettings.ViewDistance);
	//
	VW.Cam.LerpTo(&tCam, t);
	//
	Camera LastCam = VW.Cam;
	//
	//Set sound states for camera.
	Rot3 rot = {VW.Cam.a * DEG2RAD, VW.Cam.b * DEG2RAD, VW.Cam.c * DEG2RAD};
	Vec3 pos = {VW.Cam.x, VW.Cam.y, VW.Cam.z};
	Vec3 vel = {(VW.Cam.x - LastCam.x) / std::max(0.0001f, VW.FrameFrac()),
		(VW.Cam.y - LastCam.y) / std::max(0.0001f, VW.FrameFrac()),
		(VW.Cam.z - LastCam.z) / std::max(0.0001f, VW.FrameFrac())};
	//
	if(LengthVec3(vel) >= 50.0f) ScaleVec3(vel, 50.0f / LengthVec3(vel));	//Cap max camera vel for sound.
	//
	VW.snd.SetListenerPos(pos);
	VW.snd.SetListenerVel(vel);
	VW.snd.SetListenerRot(rot);
	//
	//Sanity checking.
	int nanpos = 0, nanrot = 0, nanvel = 0;
	for(int n = 0; n < 3; n++){
		if(!_finite(PlayerEnt->Position[n])) nanpos = 1;
		if(!_finite(PlayerEnt->Rotation[n])) nanrot = 1;
		if(!_finite(PlayerEnt->Velocity[n])) nanvel = 1;
	}
	if(nanpos) OutputDebugLog("NAN in player Pos!\n");
	if(nanrot) OutputDebugLog("NAN in player Rot!\n");
	if(nanvel) OutputDebugLog("NAN in player Vel!\n");
}

//
//Searches enumerated maps for a file name match, then stuffs the appropriate title into serverentryex structure.
void CTankGame::FindMapTitle(ServerEntryEx *se){
	if(se){
		for(int i = 0; i < NumMaps; i++){
			if(CmpLower(FileNameOnly(se->Map), FileNameOnly(Maps[i].file))){
				se->MapTitle = Maps[i].title;
				return;
			}
		}
		se->MapTitle = FileNameOnly(se->Map);
	}
}

bool CTankGame::DoFrame(){
#ifndef HEADLESS
	DoSfmlEvents();
	DoInput();
	VW.SetTrilinear(GameSettings.GraphicsSettings.Trilinear);
	VW.SetTextureFlush(true);
#endif
	int DoGLDisplay = 1;
	if(GameSettings.DedicatedServer){
		DoGLDisplay = 0;	//This will disable calling OpenGL rendering functions.
	}
	if(GameSettings.DedicatedServer == false && VW.Net.ConnectionStatus(CLIENTID_SERVER) != CCS_Connected && VW.CountClients() <= 0){
		if(GenBuf.Focused() == false){
			VW.FreeSound();
			GameState.SwitchSoundMode = true;
			return true;
		}

		if(GameState.PauseGame){
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSPAUSED));
			LoadingStatus.Draw(GenBuf);
			GameState.PauseScreenOn = 1;

			VW.snd.PauseMusic(true);
			VW.snd.StopAll();
			return true;
		}
		if(GameState.PauseScreenOn){
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSWAIT));
			LoadingStatus.Draw(GenBuf);
			GameState.PauseScreenOn = 0;
		}
	}else{
		if(GameState.PauseGame) GameState.PauseGame = 0;
	}
	//
	if(GameSettings.DedicatedServer == false && (GameState.SwitchSoundMode)){
		VW.FreeSound();
		//
		VW.InitSound();
		//
		int wantbits = (GameSettings.HiFiSound ? 0 : 8);
		int wantfreq = (GameSettings.HiFiSound ? 0 : 22050);
		if(wantbits != VW.GetMaxSoundBits() || wantfreq != VW.GetMaxSoundFreq()){
			VW.SetMaxSoundBits(wantbits);
			VW.SetMaxSoundFreq(wantfreq);
			VW.DownloadSounds(true);	//Force a reload from disk.
		}
		if(GameSettings.PlayMusic)
		{
			NewMusic();
			VW.snd.StartMusic(GameState.sMusicFile);
		}

		GameState.SwitchSoundMode = false;
	}

	PlayerEnt = VW.FindRegisteredEntity("PlayerEntity");
	GodEnt = (EntityTankGod*)VW.FindRegisteredEntity("TANKGOD");
	//
	//
	if(!GameState.ActAsServer && CurrentMenu == MP_None && VW.GetStatus() != STAT_RunGameLoop){
		switch(VW.GetStatus()){
		case STAT_Disconnect :
			if(VW.GameMode() & GAMEMODE_RECONNECT){
				LoadingStatus.Status(Text_GetLine(TEXT_STATUSRESTART));
				LoadingStatus.Draw(GenBuf);
				Sleep(1500);
				CurrentMenu = MP_None;
				VW.SetGameMode(VW.GameMode() & (~GAMEMODE_RECONNECT));	//Turn off reconnect flag.
				GameSettings.ServerAddress = GameState.ConnectedToAddress;	//Reconnect to last server we connected to, not necessarily currently selected server.
				StartGame();	//Start connecting again.
				return true;
			}else{
				switch(LastDisconnectError){
				case NE_ChallengeFailed : LoadingStatus.Status(Text_GetLine(TEXT_STATUSWRONGVERSION)); break;
				case NE_ServerFull : LoadingStatus.Status(Text_GetLine(TEXT_STATUSSERVERFULL)); break;
				case NE_LookupFailed : LoadingStatus.Status(Text_GetLine(TEXT_STATUSNAMELOOKUP)); break;
				case NE_TimedOut : LoadingStatus.Status(Text_GetLine(TEXT_STATUSTIMEOUT)); break;
				case NE_MissingRes : LoadingStatus.Status(Text_GetLine(TEXT_STATUSMISSINGRES)); break;
				default : LoadingStatus.Status(Text_GetLine(TEXT_STATUSDISCONNECTED)); break;
				}
				LoadingStatus.Draw(GenBuf);
				Sleep(1500);
				if(LastDisconnectError == NE_ChallengeFailed){
					CurrentMenu = MP_WrongVersion;
					ReturnToMenu = MP_ServerSelect;
				}else{
					CurrentMenu = MP_ServerSelect;
				}
				return true;
			}
			break;
		case STAT_Connecting :
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSTRYING));
			break;
		case STAT_ConnectionAccepted :
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSACCEPTED));
			break;
		case STAT_ConnectionInfoSent :
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSSENTINFO));
			break;
		case STAT_MapLoaded :
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSMAPLOAD));
			break;
		case STAT_MapLoadFailed :
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSMAPFAIL));
			break;
		case STAT_SynchEntityTypes :
//FIXME	TEXT_STATUSSYNCENTITIES		LoadingStatus.Status("Synching Entity Type " + String(VW.EntitiesSynched) + " of " + String(VW.EntitiesToSynch));
			LoadingStatus.Status("Synching Entity Type " + String(VW.EntitiesSynched) + " of " + String(VW.EntitiesToSynch));
			break;
		}
		LoadingStatus.Draw(GenBuf);
		VW.PulseNetwork(&TankPacket);
		return true;
	}
	//
	if(GameState.ActAsServer && IsGameRunning()){
		//
		if(GodEnt){
			EntityBase *e = GodEnt->TankByIndex(0);
			if(e){
				GameState.CurrentLeader = e->GID;
				if(!(VW.GameMode() & GAMEMODE_TRAINING))
				{
					if(GameState.CurrentLeader != GameState.LastLeader){
						Announcement("inlead", NULL, GameState.CurrentLeader);
						if(GameState.LastLeader != 0) Announcement("lostlead", NULL, GameState.LastLeader);
					}
				}
			}
			GameState.LastLeader = GameState.CurrentLeader;
		}
		//
		//Time and Frag limits.
		int maxfrags = 0;
		if(GodEnt && GameSettings.FragLimit){
			EntityBase *e;
			if(VW.GameMode() & GAMEMODE_TEAMPLAY && VW.GameMode() & GAMEMODE_TEAMSCORES)
			{
				ActiveTeam at[MaxTeams + 1];
				at[0].name = "No Team";
				int nat = 1, i, n;
				//Enumerate active teams.
				for(n = 0; e = GodEnt->TankByIndex(n); n++){
					for(i = 0; i < nat; i++){
						if(e->QueryInt(ATT_TEAMID) == at[i].hash){
							at[i].frags += e->QueryInt(ATT_FRAGS);
							at[i].tanks++;
							break;
						}
					}
					if(i >= nat && nat < (MaxTeams + 1)){	//Team not found, must add new team.
						EntityTypeBase *et = VW.FindEntityType(e->QueryInt(ATT_TEAMID));
						if(et){
							at[nat].name = "Team " + et->dname;
							at[nat].frags += e->QueryInt(ATT_FRAGS);
							at[nat].hash = e->QueryInt(ATT_TEAMID);
							at[nat].tanks++;
							nat++;
						}
					}
				}
				for (int j = 0; j < nat; j++)
				{
					maxfrags = max(maxfrags, at[j].frags);
				}
			}
			else
			{
				for(int n = 0; e = GodEnt->TankByIndex(n); n++){
					maxfrags = MAX(maxfrags, e->QueryInt(ATT_FRAGS));
				}
			}
		}
		int FirstHumanCrossFinish = 0;
		if(GameState.CoolDownCounter == 0 && GodEnt)
		{
			int rlaps = GodEnt->QueryInt(ATT_RACE_LAPS);
			int i = 0;
			EntityBase *e;
			while(e = GodEnt->TankByIndex(i++))
			{
				if(e->QueryInt(ATT_LAPS) >= rlaps)
				{
					if((e->QueryInt(ATT_AUTODRIVE) == false) || (e == PlayerEnt))
						FirstHumanCrossFinish = 1; // start the CoolDownCounter
					if(GameState.SomeoneWonRace == 0)
					{
						char	tmpString[1024];
						sprintf(tmpString, Text.Get(TEXT_RACEWONBY), e->QueryString(ATT_NAME).get());
						VW.StatusMessage(tmpString, STATUS_PRI_NETWORK);
						Announcement(e->QueryString(ATT_NAMESOUND), "winsrace");
						Announcement("youwin", NULL, e->GID);
						GameState.SomeoneWonRace = 1;
					}
				}
			}
		}
		int tlhit = 0, flhit = 0;
		if(GameState.CoolDownCounter == 0 && (GameState.LadderMode == false && GameSettings.TimeLimit && VW.Time() > (GameSettings.TimeLimit * 60000 + GameSettings.StartDelay * 1000))){
			tlhit = 1;
			VW.StatusMessage(Text.Get(TEXT_TIMELIMITELAPSED), STATUS_PRI_NETWORK);
			if(GodEnt)
			{
				if(!(VW.GameMode() & GAMEMODE_TRAINING))
				{
					EntityBase *e = GodEnt->TankByIndex(0);
					if(e){
						Announcement(e->QueryString(ATT_NAMESOUND), (GameSettings.Deathmatch ? "winsmatch" : "winsrace"));
						Announcement("youwin", NULL, e->GID);
					}
				}
			}
		}
		if(GameState.CoolDownCounter == 0 && GameState.LadderMode == false && GameSettings.FragLimit && maxfrags >= GameSettings.FragLimit){
			flhit = 1;
			VW.StatusMessage(Text.Get(TEXT_FRAGLIMITREACHED), STATUS_PRI_NETWORK);
			if(GodEnt){
				if(!(VW.GameMode() & GAMEMODE_TRAINING))
				{
					EntityBase *e = GodEnt->TankByIndex(0);
					if(e){
						Announcement(e->QueryString(ATT_NAMESOUND), (GameSettings.Deathmatch ? "winsmatch" : "winsrace"));
						Announcement("youwin", NULL, e->GID);
					}
				}
			}
		}
		if((tlhit) || (flhit) || GameState.CoolDownCounter > 0 || GameState.ReStartMap || FirstHumanCrossFinish ){
			GameState.ReStartMap = 0;
			if(GameState.CoolDownCounter == 0){
				if(GameSettings.DedicatedServer){	//Only set reconnect if we're dedicated.
					VW.SetGameMode(VW.GameMode() | GAMEMODE_RECONNECT);
				}
				if(GameSettings.Deathmatch == 1){
					VW.SetGameMode(VW.GameMode() | GAMEMODE_NOCANSHOOT | GAMEMODE_SHOWSCORES);
				}else{
					VW.SetGameMode(VW.GameMode() | GAMEMODE_NOCANSHOOT);
				}
				//Tell tanks to not drive, and tell clients to show scoreboard, and to reconnect when dumped.
				GameState.CoolDownCounter = 1;
				OutputDebugLog("\nEntering Cool Down Period, preparing to restart server...\n\n");
				if(GameSettings.DedicatedServer){
					VW.ChatMessage("*** Level Changing in " + String(GameSettings.CoolDownTime) + " Seconds ***");
				}
			}else{
				int lcdc = GameState.CoolDownCounter;
				GameState.CoolDownCounter += VW.FrameTime();
				if(GameState.CoolDownCounter > GameSettings.CoolDownTime * 500 && lcdc <= GameSettings.CoolDownTime * 500){	//Transition to half-way time.
					VW.SetGameMode(VW.GameMode() | GAMEMODE_NOCANSHOOT | GAMEMODE_SHOWSCORES);
					//Turn on score showing if we were in race mode and hadn't had them on yet.
				}
				if(GameState.CoolDownCounter > GameSettings.CoolDownTime * 1000){	//Time's up.
					GameState.CoolDownCounter = 0;
					if(GameSettings.DedicatedServer){
						StartGame();	//Cool down time has expired, re-start dedicated server.
						return true;
					}else{	//Single player.
						//
						Stats.GetRaceResults();	//Update stats package.
						Stats.Save();
						//
						if(GameState.LadderMode && Ladder.RaceInProgress){
							Ladder.GetRaceResults();
							Ladder.Save();
							Sleep(100);
							StopGame();
							if(Ladder.PlayerRank <= 0){
								JumpToMenu(MP_Trophy);	//Yer at the top, dude!
							}else{
								JumpToMenu(MP_Ladder);
							}
							return true;
						}else{
							Sleep(100);
							if(GameState.DemoMode){
								StartGame();	//Continuously restart demo mode.
								return true;
							}else{
								StopGame();
								ReturnToMenu = MP_MapSelect;
								JumpToMenu(MP_Stats);	//Show map hall of fame.
								return true;
							}
						}
					}
				}
			}
		}
		//Dedicated server stuff.
		if(GameSettings.DedicatedServer){
			//
			//Heartbeats to masters.
			HeartbeatTime += VW.FrameTime();
			if(GameSettings.SendHeartbeats && HeartbeatTime > GameSettings.SendHeartbeats * 1000){
				for(int i = 0; i < MAX_MASTERS; i++){
					if(Master[i].ip.sin_addr.s_addr){
						Heartbeat(&Master[i].ip);
					}
				}
				HeartbeatTime = 0;
			}
		}else{	//Single player/server but NOT dedicated.
			if(PlayerEnt){
				//Set single-player game insignia flag entity.
				EntityTypeBase *insig = VW.FindEntityType("insignia", GameSettings.InsigniaType);
				if(insig){
					PlayerEnt->SetInt(ATT_INSIGNIA2_HASH, insig->thash);
				}else{
					PlayerEnt->SetInt(ATT_INSIGNIA2_HASH, NULL);
				}
				int i = 0;
				for(i = 0; i < GetNumTeams(); i++){	//Make sure team insignia is on OK list.
					if(Teams[i].hash == GameState.TeamHash){
						PlayerEnt->SetInt(ATT_INSIGNIA1_HASH, GameState.TeamHash);
						break;
					}
				}
				if(i >= GetNumTeams()){	//Team not found in valid list.
					if(GameSettings.TeamPlay)
//						GameState.TeamHash = Teams[0].hash;
						GameState.TeamHash = GetTeam(GetTeamFillSpot()).hash;
					else
						GameState.TeamHash = 0;
					PlayerEnt->SetInt(ATT_INSIGNIA1_HASH, GameState.TeamHash);
				}
				//
				//Do HowitzerTime!
				if(VW.CountClients() <= 0){	//Only do HowitzerTime if no clients connected!
					if(GodEnt && GodEnt->QueryInt(ATT_HOWITZERTIME_TOGGLE)){
						if(GameState.HowitzerTime <= 0){
							GameState.HowitzerTime = GameSettings.HowitzerTimeStart;
						}
						GodEnt->SetInt(ATT_HOWITZERTIME_TOGGLE, 0);
					}
				}else{
					GameState.HowitzerTime = 0;
				}
				//
			}
		}
	}
	if(GameState.KillSelf){	//Client side suicide handling.
		if(VW.Net.IsClientActive()){
			BitPacker<16> bp;
			bp.PackUInt(BYTE_HEAD_COMMAND, 8);
			bp.PackString("kill", 7);
			VW.Net.QueueSendServer(TM_Ordered, (char*)bp.Data(), bp.BytesUsed());
		}else{
			if(PlayerEnt) PlayerEnt->SetInt(ATT_CMD_KILL, 1);
		}
		GameState.KillSelf = 0;
	}
	//
	//Ping server list.  Set MasterPingIter to 0 to begin or restart pinging.
	if(CurrentMenu == MP_ServerSelect || CurrentMenu == MP_Chat){
		if(abs(int(VW.Time() - LastMasterPingTime)) >= 1000 && GameState.MasterPingIter >= 0){	//Send pings once per second.
			int pinged = 0;
			for(ServerEntryEx *se = (ServerEntryEx*)ServerHead.NextLink(); se; se = (ServerEntryEx*)se->NextLink()){
				if(se->PingCount <= GameState.MasterPingIter){	//Server should be pinged this round.
					pinged++;
					OOBPing(&se->Address);
				}
				if(pinged >= GameSettings.MasterPingsPerSecond) break;	//Only send this many each second.
			}
			if(pinged < GameSettings.MasterPingsPerSecond && GameState.MasterPingIter < GameSettings.MasterPings){	//This iteration is finished.
				GameState.MasterPingIter++;
			}
			LastMasterPingTime = VW.Time();
		}
		//Iteratively sort master server list.
		for(ServerEntryEx *se = (ServerEntryEx*)ServerHead.NextLink(); se; se = (ServerEntryEx*)se->NextLink()){
			ServerEntryEx *se2 = (ServerEntryEx*)se->NextLink();
			if(se2){
				switch(GameState.SortMode){
				case MSM_Name : if(strcmp(se->Name, se2->Name) > 0) se->ShiftItemDown(); break;
				case MSM_Map : if(strcmp(FileNameOnly(se->Map), FileNameOnly(se2->Map)) > 0) se->ShiftItemDown(); break;
				case MSM_Mode : if(se->GameMode > se2->GameMode) se->ShiftItemDown(); break;
				case MSM_Time : if(se->TimeInGame > se2->TimeInGame) se->ShiftItemDown(); break;
				case MSM_Players : if(se->Clients < se2->Clients) se->ShiftItemDown(); break;
				case MSM_Ping : if((se->PingTime / MAX(1, se->PingCount) > se2->PingTime / MAX(1, se2->PingCount) ||
									se->PingTime <= 0) && se2->PingTime > 0) se->ShiftItemDown(); break;
				}
			}
		}
	}
	//
	//
	int msecs;
	msecs = tmr.Check(1000);

	// Limit the frame rate
	int minFrameTimeMs = 1000 / (GameSettings.DedicatedServer ? GameSettings.DedicatedFPS : GameSettings.MaxFPS);
	if(msecs < minFrameTimeMs)
	{
		Sleep(minFrameTimeMs - msecs);
		msecs = tmr.Check(1000);
	}

	if(!GameSettings.DedicatedServer)
	{
		//Lock to FPS when writing an anim.
		if(GameState.WritingAnim && GameSettings.AnimFPS > 0){
			msecs = 1000 / GameSettings.AnimFPS;
		}
		//
		//HowitzerTime temporal scaling.
		if(GameState.HowitzerTime > 0){
			GameState.HowitzerTime -= msecs;
			if(GameState.HowitzerTime < 0) GameState.HowitzerTime = 0;
			msecs = MAX(1, (int)((float)msecs * GameSettings.HowitzerTimeScale));
		}
		//
	}
	tmr.Start();
	//
	VW.InputTime(msecs);
	//
	//Watch for slow frame rates and kill menu fire.
	if(VW.FrameTime() > 100) FramesOver100MS++;
	else FramesOver100MS = 0;	//Reset as soon as we get a fast fame; slow frames must be consecutive.
	if(FramesOver100MS > 10) GameSettings.GraphicsSettings.MenuFire = 0;
	//
	//Textual heads up displays.
	if(DoGLDisplay){
		DoHUD();
	}
	//

	//Do race-start countdown.
	int Secs = VW.Time() / 1000;

	if(LastSecs != Secs && GameState.CountdownTimer > 0 && !VW.Net.IsClientActive()){
		int t = --GameState.CountdownTimer;
		CStr s = String(t);
		if(t <= 0){
			s = "GO!";
			VW.AddEntity("sound", "hornmid", NULL, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET | ADDENTITY_NOSKIP);
			VW.SetGameMode(VW.GameMode() & (~GAMEMODE_NOCANDRIVE));
		}else{
			if(t <= 5){	//Only horn for numbers at or less than 5.
				VW.AddEntity("sound", "hornlow", NULL, NULL, NULL, 0, 0, 0, ADDENTITY_FORCENET | ADDENTITY_NOSKIP);
			}
		}
		EntityBase *e = VW.AddEntity("text", "font1",
			CVec3(0.5f, 0.5f, 0.0f), CVec3(0.2f, 0.2f, 1), NULL,
			1000, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY | FLAG_TEXT_FADE, 0, ADDENTITY_FORCENET | ADDENTITY_NOSKIP);
		if(e) e->SetString(ATT_TEXT_STRING, s);
	}

	LastSecs = Secs;
	//
	//
	DoPlayerAndCam();
	//
	VW.snd.PauseMusic(false);
	//
	//This one is to send player inputs quickly.
	VW.PulseNetwork(&TankPacket);
	//
	//
	VW.PolyRend->LimitLOD(GameSettings.GraphicsSettings.PolyLod);
	VW.PolyRend->Particles(GameSettings.GraphicsSettings.Particles);
	VW.PolyRend->PolyNormals(GameSettings.GraphicsSettings.DebugPolyNormals);
	VW.PolyRend->Fog(GameSettings.GraphicsSettings.UseFog);
	VW.PolyRend->AlphaTest(GameSettings.GraphicsSettings.UseAlphaTest);
	//
	//
	MasterNet.ProcessTraffic(&MasterPacket);	//Process client to master comms.
	if(DoGLDisplay){//GameSettings.DedicatedServer == false){
		DoMenus();	//Handle menu states.
	}
	//
	frames++;
	framems += msecs;
	//
	VW.Cam.a = RAD2DEG * -atan(VW.Cam.pitch);
	//
	tmr2.Start();
	if(DoGLDisplay){//GameSettings.DedicatedServer == false){
		//
		if(VW.Map.Width() > 0 && VW.Map.Height() > 0){
			VW.VoxelRend->GLTerrainRender(&VW.Map, &VW.Cam,
				(GameSettings.GraphicsSettings.WireFrame ? (GLREND_WIREFRAME | GLREND_NOSKY) : 0) |
				(GameSettings.GraphicsSettings.DetailTerrain ? 0 : GLREND_NODETAIL) |
				(GameSettings.GraphicsSettings.DisableMT ? GLREND_NOMT : 0) | (GameSettings.GraphicsSettings.UseFog ? 0 : GLREND_NOFOG) |
				(GameSettings.GraphicsSettings.DebugLockPatches ? GLREND_LOCKPATCHES : 0),
				/*ViewDistance,*/ GameSettings.GraphicsSettings.Quality, msecs);
		}else{
			GenBuf.Clear();
		}
	}

	if(VW.Net.IsServerActive() && GameSettings.AIAutoFill)
	{
		int iMaxItters = GameSettings.AIAutoFillCount;
		int ClientCount = VW.CountClients();
		if(!GameState.ActAsServer)
			ClientCount++; // add one for the player
		while ((ClientCount + GameState.NumAITanks < GameSettings.AIAutoFillCount) && iMaxItters >= 0)
		{
			// add ai
			EntityBase *ent = VW.AddEntity("racetank", RandTankType(), NULL, NULL, NULL, 0, 0, 0, 0);
			if(ent)
			{
				char	sOutput[1024];
				GameState.NumAITanks++;
				ent->SetInt(ATT_CMD_FIRSTSPAWN, true);
				ent->SetFloat(ATT_ACCEL, 0);
				ent->SetFloat(ATT_STEER, 0);
				ent->SetFloat(ATT_TURRETSTEER, 0);
				ent->SetInt(ATT_AUTODRIVE, true);
				//
				ent->SetFloat(ATT_SKILL, GameSettings.EnemySkill);
				//
				NextTankName = (NextTankName + 1) % MAX(1, NumTankNames);	//Cycle through tank names.
				if(NumTankNames > 0) ent->SetString(ATT_NAME, GameSettings.AIPrefix + TankNames[NextTankName]);

				sprintf(sOutput, Text.Get(TEXT_DEDI_JOINEDSERVER), ent->QueryString(ATT_NAME).get());
				VW.StatusMessage(sOutput);

				if(GameSettings.TeamPlay)
				{
					int tmpTeamID;
					if(AITankTeam)
					{
						TeamIndexFromHash(AITankTeam, &tmpTeamID);
						ent->SetInt(ATT_INSIGNIA1_HASH, AITankTeam);	//All AI tanks go on one team.
					}
					else
					{
						tmpTeamID = GetTeamFillSpot();
						ent->SetInt(ATT_INSIGNIA1_HASH, Teams[tmpTeamID].hash);	//AI tanks go on all teams.
					}
					sprintf(sOutput, Text.Get(TEXT_PLAYERJOINTEAM), ent->QueryString(ATT_NAME).get(), Teams[tmpTeamID].name.get());
					CTankGame::Get().GetVW()->StatusMessage(sOutput, STATUS_PRI_NETWORK);
				}
			}
			iMaxItters--;
		}
		iMaxItters = ClientCount + GameState.NumAITanks;
		while ((ClientCount + GameState.NumAITanks > GameSettings.AIAutoFillCount) && (GameState.NumAITanks > 0) && (iMaxItters >= 0))
		{
			// remove ai
			EntityBase *EntToRemove = NULL;

			if(GameSettings.TeamPlay)
			{
				// First look for a team with more members than the others that has an AI
				int *TeamCounts = new int[GetNumTeams()];
				int *TeamAICounts = new int[GetNumTeams()];
				memset(TeamCounts, 0, sizeof(int) * GetNumTeams());
				memset(TeamAICounts, 0, sizeof(int) * GetNumTeams());
				EntityBase	*ent = VW.EntHead->NextLink();

				// build up the counts for each team
				while (ent != NULL)
				{
					if(CmpLower(ent->TypePtr->cname, "racetank"))
					{
						int iTeamIndex;
						if(TeamIndexFromHash(ent->QueryInt(ATT_TEAMID), &iTeamIndex))
						{
							TeamCounts[iTeamIndex]++;
							if(ent->QueryInt(ATT_AUTODRIVE))
								TeamAICounts[iTeamIndex]++;
						}
						else
							EntToRemove = ent;
					}
					ent = ent->NextLink();
				}
				int LargestTeamCount = -1;
				for(int i = 0; i < GetNumTeams(); i++)
				{
					if(TeamCounts[i] >= LargestTeamCount && TeamAICounts[i] > 0)
					{
						int iLowestFrags = 999999999; // looking for the AI with the lowest number of frags that isn't carrying a flag
						ent = VW.EntHead->NextLink();
						while (ent != NULL)	// find AIs on that team, and check to see if it's ok to remove them
						{
							if(ent->QueryInt(ATT_AUTODRIVE) && ent->QueryInt(ATT_TEAMID) == Teams[i].hash && ent->QueryInt(ATT_FLAGID) == 0)
							{
								if(ent->QueryInt(ATT_FRAGS) < iLowestFrags && !ent->RemoveMe)
								{
									iLowestFrags = ent->QueryInt(ATT_FRAGS);
									EntToRemove = ent;
									LargestTeamCount = TeamCounts[i];	// only update this if we found a team we can remove from
								}
							}
							ent = ent->NextLink();
						}
					}
				}

				delete []TeamCounts;
				delete []TeamAICounts;
			}
			else // look for ai with lowest frag count
			{
				int iLowestFrags = 999999999;

				EntityBase	*ent = VW.EntHead->NextLink();

				while (ent != NULL)
				{
					if(ent->QueryInt(ATT_AUTODRIVE) && ent->QueryInt(ATT_FRAGS) < iLowestFrags)
					{
						EntToRemove = ent;
						iLowestFrags = ent->QueryInt(ATT_FRAGS);
					}
					ent = ent->NextLink();
				}
			}
			if(EntToRemove)
			{
				char	sOutput[1024];
				sprintf(sOutput, Text.Get(TEXT_DEDI_REMOVEFROMGAME), EntToRemove->QueryString(ATT_NAME).get());
				VW.StatusMessage(sOutput);
				EntToRemove->Remove();
				GameState.NumAITanks--;
			}
			iMaxItters--;
		}
		if(iMaxItters < 0 && GameState.NumAITanks > 0)
		{
			char	sTmp[1024];
			sprintf(sTmp, "AITanks - %d\tFillCount - %d\tClients - %d\n", GameState.NumAITanks, GameSettings.AIAutoFillCount, ClientCount);
			OutputDebugLog("Non-critical error removing AI Tank.\n");
			OutputDebugLog(sTmp);
		}
	}
	voxelms += tmr2.Check(1000);
	//
	tmr2.Start();
	VW.PolyRend->InitRender();//, NULL, NULL);
	VW.PolyRend->SetupCamera(&VW.Cam);
	//
	//This one is to grab server outputs just before think.
	VW.PulseNetwork(&TankPacket);
	//
	VW.ThinkEntities( (GameSettings.GraphicsSettings.TreadMarks ? 0 : THINK_NOTRACKS) );
	//
	if(DoGLDisplay){//GameSettings.DedicatedServer == false){
		VW.snd.SetSoundVolume(GameSettings.SoundVol);
		VW.snd.SetMusicVolume(GameSettings.MusicVol);
		//
		if(GameState.HowitzerTime > 0){
			VW.snd.SetGlobalPitch(MAX(0.1f, GameSettings.HowitzerTimeScale));
		}else{
			VW.snd.SetGlobalPitch(1.0f);
		}

		VW.snd.PulseAudio();

		if(GameSettings.PlayMusic && !VW.snd.IsMusicPlaying())
		{
			NewMusic();
			VW.snd.StartMusic(GameState.sMusicFile);
			OutputDebugLog("\nRewinding Music\n\n");
		}
	}
	//
	VW.ClearMouseFlags();	//Clears clicked/released flags for GUI entities.
	//
	if(DoGLDisplay){//GameSettings.DedicatedServer == false){
		VW.DownloadTextures(true);	//Update any new textures only.
		VW.DownloadSounds();
	}
	//
	//This one is to send server outputs just after think.
	VW.PulseNetwork(&TankPacket);
	//
	thinkms += tmr2.Check(1000);
	//
	tmr2.Start();
	if(DoGLDisplay){//GameSettings.DedicatedServer == false){
		if(GameState.TakeMapSnapshot == 0){
			VW.PolyRend->GLDoRender();	//Skip polygonal objects and sprites when map snapshot requested.
		}
		VW.VoxelRend->GLRenderWater(&VW.Map, &VW.Cam, (GameSettings.GraphicsSettings.UseFog ? 0 : GLREND_NOFOG) |
			(GameSettings.GraphicsSettings.DebugLockPatches ? GLREND_LOCKPATCHES : 0), GameSettings.GraphicsSettings.Quality);
		//Render water in front of all transparent objects.
		if(GameState.TakeMapSnapshot == 0){
			VW.PolyRend->GLDoRenderTrans();
			VW.PolyRend->GLDoRenderOrtho();
			//
			Camera cam;// = VW.Cam;
			cam.SetAll(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, GenBuf.Width());// / 2);
			cam.SetView(GenBuf.Width(), GenBuf.Height(), 1000.0f);
			VW.PolyRend->SetupCamera(&cam);
			VW.PolyRend->SetLightVector(0.0f, 1.0f, -1.0f, 0.5f);
			VW.PolyRend->GLDoRenderSecondary();	//Render secondary frustum objects e.g. spinning tank preview.
		}
	}
	//
	polyms += tmr2.Check(1000);
	//
	tmr2.Start();
	//
#ifndef HEADLESS
	if((GameState.TakeMapSnapshot || GameState.WritingAnim) && DoGLDisplay){//GameSettings.DedicatedServer == false){
		Image tbmp, tbmp2;
		if(tbmp.Init(GameSettings.GraphicsSettings.GLBWIDTH, GameSettings.GraphicsSettings.GLBHEIGHT, 32)){

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
			glDisable(GL_FOG);
			glReadPixels(0, 0, GameSettings.GraphicsSettings.GLBWIDTH, GameSettings.GraphicsSettings.GLBHEIGHT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, tbmp.Data());
			if(GameState.TakeMapSnapshot == 0){
				CStr num = String(GameState.AnimFrame++);
				for(int n = (4 - num.len()); n; n--) num = "0" + num;
				if(tbmp2.Init(tbmp.Width(), tbmp.Height(), 24)){
				    #ifdef WIN32
                        CreateDirectory(FilePathOnly(GameSettings.AnimationDir), NULL);	//Make sure path exists.
                    #else
                        mkdir(FilePathOnly(GameSettings.AnimationDir), 0777);
                    #endif
					tbmp2.Suck(&tbmp);
					tbmp2.SaveBMP(GameSettings.AnimationDir + num + ".bmp", true);
				}
			}else{	//Save a down-sized map selection snapshot in 8 bit.
				if(tbmp.Scale(MAP_SNAP_SIZE, MAP_SNAP_SIZE, true)){
					//	if(tbmp2.Init(MAP_SNAP_SIZE, MAP_SNAP_SIZE, 24)){//8)){
					if(tbmp.Quantize32to8HighQuality(&tbmp2)){
					//	if(tbmp2.Suck(&tbmp)){	//Lets try 24bit...  8bit artifacts suck.  Back to 8 bit with high-q quantizer.
						tbmp2.SaveBMP(FileNoExtension(GameSettings.MapFile) + "_shot.bmp", true);
					//	}
					}
				}
				GameState.TakeMapSnapshot = 0;
			}
		}
	}
	//
	if(DoGLDisplay){//GameSettings.DedicatedServer == false){
		GenBuf.Swap();
	}
#endif
	//
	blitms += tmr2.Check(1000);
	//
	if(frames % 10 == 0){	//Timings.
		//
		GameState.NetCPSOut = ((VW.Net.TotalBytesOut() - GameState.LastNetBytesOut) * 1000) / std::max(1, framems);// / 10;
		GameState.NetCPSIn = ((VW.Net.TotalBytesIn() - GameState.LastNetBytesIn) * 1000) / std::max(1, framems);// / 10;
		GameState.LastNetBytesOut = VW.Net.TotalBytesOut();
		GameState.LastNetBytesIn = VW.Net.TotalBytesIn();
		//

#ifdef QT_CORE_LIB
		GetDedicatedWin()->Reinit();
#endif
	}
	int uframes = 20;
	if(GameSettings.DedicatedServer || CurrentMenu != MP_None) uframes = 100;
	if(frames % uframes == 0){
		GameState.FPS = 1000.0 / ((double)framems / (double)uframes);
		sprintf(DbgBuf, "TimeStamp: %d, FPS %f, Frame %d, Think %d, Voxel %d, Poly %d, Blit %d, ThisFrame %d.\n",
			VW.Time(), (double)GameState.FPS, framems / uframes, thinkms / uframes, voxelms / uframes, polyms / uframes,
			blitms / uframes, msecs);
		OutputDebugLog(DbgBuf, 2);
		framems = thinkms = polyms = voxelms = blitms = 0;
		//
		if(VW.Net.IsClientActive()){
			unsigned int u, r, o;
			VW.Net.GetServerInByteCounts(&u, &r, &o);
			VW.Net.ResetServerInByteCounts();

			OutputDebugLog("Received Byte Counts:  U " +
				String((int)u) + "  R " + String((int)r) + "  O " + String((int)o) + "\n");
		}

#ifdef QT_CORE_LIB
		GetDedicatedWin()->Reinit();
#endif
	}
	//
	return true;
}
#ifndef HEADLESS
const char* KeyNameText(sf::Keyboard::Key key)
{
	switch(key)
	{
	case sf::Keyboard::BackSpace: return "BACKSPACE";
	case sf::Keyboard::Tab: return "TAB";
	case sf::Keyboard::Return: return "RETURN";
	case sf::Keyboard::Pause: return "PAUSE";
	case sf::Keyboard::Escape: return "ESCAPE";
	case sf::Keyboard::Space: return "SPACE";
	case sf::Keyboard::Quote: return "QUOTE";
	case sf::Keyboard::Comma: return "COMMA";
	case sf::Keyboard::Dash: return "MINUS";
	case sf::Keyboard::Period: return "PERIOD";
	case sf::Keyboard::Slash: return "SLASH";
	case sf::Keyboard::Num0: return "0";
	case sf::Keyboard::Num1: return "1";
	case sf::Keyboard::Num2: return "2";
	case sf::Keyboard::Num3: return "3";
	case sf::Keyboard::Num4: return "4";
	case sf::Keyboard::Num5: return "5";
	case sf::Keyboard::Num6: return "6";
	case sf::Keyboard::Num7: return "7";
	case sf::Keyboard::Num8: return "8";
	case sf::Keyboard::Num9: return "9";
	case sf::Keyboard::SemiColon: return "SEMICOLON";
	case sf::Keyboard::Equal: return "EQUALS";
	case sf::Keyboard::LBracket: return "LEFTBRACKET";
	case sf::Keyboard::BackSlash: return "BACKSLASH";
	case sf::Keyboard::RBracket: return "RIGHTBRACKET";
	case sf::Keyboard::Tilde: return "TILDE";
	case sf::Keyboard::A: return "A";
	case sf::Keyboard::B: return "B";
	case sf::Keyboard::C: return "C";
	case sf::Keyboard::D: return "D";
	case sf::Keyboard::E: return "E";
	case sf::Keyboard::F: return "F";
	case sf::Keyboard::G: return "G";
	case sf::Keyboard::H: return "H";
	case sf::Keyboard::I: return "I";
	case sf::Keyboard::J: return "J";
	case sf::Keyboard::K: return "K";
	case sf::Keyboard::L: return "L";
	case sf::Keyboard::M: return "M";
	case sf::Keyboard::N: return "N";
	case sf::Keyboard::O: return "O";
	case sf::Keyboard::P: return "P";
	case sf::Keyboard::Q: return "Q";
	case sf::Keyboard::R: return "R";
	case sf::Keyboard::S: return "S";
	case sf::Keyboard::T: return "T";
	case sf::Keyboard::U: return "U";
	case sf::Keyboard::V: return "V";
	case sf::Keyboard::W: return "W";
	case sf::Keyboard::X: return "X";
	case sf::Keyboard::Y: return "Y";
	case sf::Keyboard::Z: return "Z";
	case sf::Keyboard::Delete: return "DELETE";
	case sf::Keyboard::Numpad0: return "KP0";
	case sf::Keyboard::Numpad1: return "KP1";
	case sf::Keyboard::Numpad2: return "KP2";
	case sf::Keyboard::Numpad3: return "KP3";
	case sf::Keyboard::Numpad4: return "KP4";
	case sf::Keyboard::Numpad5: return "KP5";
	case sf::Keyboard::Numpad6: return "KP6";
	case sf::Keyboard::Numpad7: return "KP7";
	case sf::Keyboard::Numpad8: return "KP8";
	case sf::Keyboard::Numpad9: return "KP9";
	case sf::Keyboard::Divide: return "KP_DIVIDE";
	case sf::Keyboard::Multiply: return "KP_MULTIPLY";
	case sf::Keyboard::Subtract: return "KP_MINUS";
	case sf::Keyboard::Add: return "KP_PLUS";
	case sf::Keyboard::Up: return "UP";
	case sf::Keyboard::Down: return "DOWN";
	case sf::Keyboard::Right: return "RIGHT";
	case sf::Keyboard::Left: return "LEFT";
	case sf::Keyboard::Insert: return "INSERT";
	case sf::Keyboard::Home: return "HOME";
	case sf::Keyboard::End: return "END";
	case sf::Keyboard::PageUp: return "PAGEUP";
	case sf::Keyboard::PageDown: return "PAGEDOWN";
	case sf::Keyboard::F1: return "F1";
	case sf::Keyboard::F2: return "F2";
	case sf::Keyboard::F3: return "F3";
	case sf::Keyboard::F4: return "F4";
	case sf::Keyboard::F5: return "F5";
	case sf::Keyboard::F6: return "F6";
	case sf::Keyboard::F7: return "F7";
	case sf::Keyboard::F8: return "F8";
	case sf::Keyboard::F9: return "F9";
	case sf::Keyboard::F10: return "F10";
	case sf::Keyboard::F11: return "F11";
	case sf::Keyboard::F12: return "F12";
	case sf::Keyboard::F13: return "F13";
	case sf::Keyboard::F14: return "F14";
	case sf::Keyboard::F15: return "F15";
	case sf::Keyboard::RShift: return "RSHIFT";
	case sf::Keyboard::LShift: return "LSHIFT";
	case sf::Keyboard::RControl: return "RCTRL";
	case sf::Keyboard::LControl: return "LCTRL";
	case sf::Keyboard::RAlt: return "RALT";
	case sf::Keyboard::LAlt: return "LALT";
	case sf::Keyboard::RSystem: return "RMETA";
	case sf::Keyboard::LSystem: return "LMETA";
	case sf::Keyboard::Menu: return "MENU";
	default: return "";
	}
}

void CTankGame::DoSfmlEvents()
{
	// Reset these before we read the new input.
	GetInputState()->MouseLR = 0;
	GetInputState()->MouseUD = 0;

	sf::Event sfmlEvent;

	while(GenBuf.PollEvent(sfmlEvent))
	{
		switch(sfmlEvent.type)
		{
		case sf::Event::Resized:
			GenBuf.Resize(sfmlEvent.size.width, sfmlEvent.size.height);
			break;

		case sf::Event::KeyPressed:
			{
				if(sfmlEvent.key.alt)
				{
					switch(sfmlEvent.key.code)
					{
					case sf::Keyboard::F12 :	//Alt-F12 will take a Map Snapshot.
						GetGameState()->TakeMapSnapshot = 1;
						break;
#ifdef _DEBUG
					case sf::Keyboard::F11 :	//Alt-F11 will cycle terrain driver.
						GetVW()->SelectTerrainDriver();
						break;
					case sf::Keyboard::F9 :
						GetSettings()->GraphicsSettings.DebugLockPatches = !GetSettings()->GraphicsSettings.DebugLockPatches;
						break;
#endif
					case sf::Keyboard::K :
						GetGameState()->KillSelf = 1;	//Alt-K to self destruct.
						break;
					}
				}
				else
				{
					static bool bNumLock = false;
					static bool bScrollLock = false;
					static bool bCapsLock = false;

					CStr key;
					key = KeyNameText(sfmlEvent.key.code);

					if(CurrentMenu != MP_Chat) CTankGame::Get().GetVW()->Ififo.Set(key, 1);

					//OutputDebugString("KeyDown: \"" + key + "\"\n");

					switch(sfmlEvent.key.code)
					{
					case sf::Keyboard::Escape :
						if(GetVW()->GetChatMode() == 0) GetGameState()->ToggleMenu = true;
						break;
					case sf::Keyboard::Pause :
						GetGameState()->PauseGame = !GetGameState()->PauseGame;
						break;
					case sf::Keyboard::Up :
						GetVW()->SetChar(VWK_UP);
						break;
					case sf::Keyboard::Down :
						GetVW()->SetChar(VWK_DOWN);
						break;
					case sf::Keyboard::Left :
						GetVW()->SetChar(VWK_LEFT);
						break;
					case sf::Keyboard::Right :
						GetVW()->SetChar(VWK_RIGHT);
						break;
					case sf::Keyboard::F1:
						GetGameState()->FPSCounter = !GetGameState()->FPSCounter;
						break;
#ifdef _DEBUG
					case sf::Keyboard::F4 :
						GetSettings()->GraphicsSettings.UseFog = !GetSettings()->GraphicsSettings.UseFog;
						break;
					case sf::Keyboard::F5 : GetSettings()->GraphicsSettings.WireFrame = !GetSettings()->GraphicsSettings.WireFrame; break;
					case sf::Keyboard::F6 : GetSettings()->GraphicsSettings.TreadMarks = !GetSettings()->GraphicsSettings.TreadMarks; break;
					case sf::Keyboard::F7 : GetSettings()->GraphicsSettings.DetailTerrain = !GetSettings()->GraphicsSettings.DetailTerrain; break;
#endif
					case sf::Keyboard::F8 :
						GetGameState()->SwitchMode = true;
						GetSettings()->GraphicsSettings.UseFullScreen = !GetSettings()->GraphicsSettings.UseFullScreen;
						break;
#ifdef _DEBUG
					case sf::Keyboard::F9 :
						GetSettings()->GraphicsSettings.DisableMT = !GetSettings()->GraphicsSettings.DisableMT;
						break;
					case sf::Keyboard::F11 :
						GetSettings()->GraphicsSettings.Particles += 0.25f;
						if(fabsf(GetSettings()->GraphicsSettings.Particles - 1.0f) < 0.1f) GetSettings()->GraphicsSettings.Particles = 1.0f;
						else if(GetSettings()->GraphicsSettings.Particles > 1.0f) GetSettings()->GraphicsSettings.Particles = 0.25f;
						break;
#endif
					case sf::Keyboard::F12 :
						if(GetGameState()->WritingAnim){
							GetGameState()->WritingAnim = false;
						}else{
							if(GetSettings()->AnimationDir.len() > 0){
								GetGameState()->WritingAnim = true;
							}
						}
						break;
					case sf::Keyboard::Num1 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.1f; break;
					case sf::Keyboard::Num2 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.2f; break;
					case sf::Keyboard::Num3 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.3f; break;
					case sf::Keyboard::Num4 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.4f; break;
					case sf::Keyboard::Num5 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.5f; break;
					case sf::Keyboard::Num6 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.6f; break;
					case sf::Keyboard::Num7 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.7f; break;
					case sf::Keyboard::Num8 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.8f; break;
					case sf::Keyboard::Num9 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 0.9f; break;
					case sf::Keyboard::Num0 : if(GetVW()->GetChatMode() == 0 && CurrentMenu == MP_None) GetSettings()->GraphicsSettings.Quality = 1.0f; break;
					default:
						OutputDebugLog("Weird VK Down " + String((int)sfmlEvent.key.code) + "\n");
					}
					break;
				}
				break;
			}
		case sf::Event::KeyReleased:
			{
				CStr key;
				key = KeyNameText(sfmlEvent.key.code);

				GetVW()->Ififo.Set(key, false);

				//OutputDebugString("KeyUp: \"" + key + "\"\n");
				break;
			}
		case sf::Event::TextEntered:
			GetVW()->SetChar(char(sfmlEvent.text.unicode));
			break;
		case sf::Event::MouseButtonPressed:
			switch(sfmlEvent.mouseButton.button)
			{
			case sf::Mouse::Left:
				GetVW()->Ififo.Set("Mouse L", 1);
				break;
			case sf::Mouse::Right:
				GetVW()->Ififo.Set("Mouse R", 1);
				break;
			case sf::Mouse::Middle:
				GetVW()->Ififo.Set("Mouse M", 1);
				break;
			}
			break;
		case sf::Event::MouseButtonReleased:
			switch(sfmlEvent.mouseButton.button)
			{
			case sf::Mouse::Left:
				GetVW()->Ififo.Set("Mouse L", 0);
				break;
			case sf::Mouse::Right:
				GetVW()->Ififo.Set("Mouse R", 0);
				break;
			case sf::Mouse::Middle:
				GetVW()->Ififo.Set("Mouse M", 0);
				break;
			}
			break;
		case sf::Event::MouseMoved:
			{
				float MouseX = sfmlEvent.mouseMove.x, MouseY = sfmlEvent.mouseMove.y;

				MouseX = std::max(0.0f, std::min(MouseX, float(GenBuf.Width())));
				MouseY = std::max(0.0f, std::min(MouseY, float(GenBuf.Height())));

				MouseX /= std::max(1, GenBuf.Width());
				MouseY /= std::max(1, GenBuf.Height());

				GetVW()->InputMousePos(MouseX, MouseY);

				GetVW()->Ififo.Set("Mouse X", -1, MouseX);
				GetVW()->Ififo.Set("Mouse Y", -1, MouseY);

				int MouseXRel = sfmlEvent.mouseMove.x - (GenBuf.Width() / 2);
				int MouseYRel = sfmlEvent.mouseMove.y - (GenBuf.Height() / 2);

				GetInputState()->MouseLR = ((MouseXRel * 10.0f) / 200.0f) * GetSettings()->InputSettings.MouseSpeed;
				GetInputState()->MouseUD = ((MouseYRel * 10.0f * (GetSettings()->InputSettings.InvMouseY ? -1.0f : 1.0f)) / 200.0f) * GetSettings()->InputSettings.MouseSpeed;
				break;
			}
		case sf::Event::Closed:
			GetGameState()->Quit = true;
			GetVW()->snd.Free();
			GetVW()->UndownloadTextures();
			GenBuf.Destroy(true);
			break;
		}//Switch
	}

	if(CurrentMenu == MP_None)
		GenBuf.CaptureMouse();
}
#endif

//Sends a heartbeat (info on us) to master server, or client, or anyone really, even your pet goldfish.
int CTankGame::Heartbeat(sockaddr_in *dest){
	if(dest && dest->sin_addr.s_addr != 0){
		BitPacker<1024> pe;
		pe.PackUInt(LONG_HEAD_HEARTBEAT, 32);
		pe.PackUInt(VW.CountClients() + GameState.NumAITanks, 8);	//NOTE:  Here we have a bit of minor deception, as bots are included in the client counts sent.
		pe.PackUInt(GameSettings.MaxClients + GameState.NumAITanks, 8);
		pe.PackUInt(VW.Net.GetServerRate(), 32);
		pe.PackUInt(VW.GameMode(), 32);
		pe.PackUInt(VW.Time(), 32);
		pe.PackString(GameSettings.ServerName, 8);
		pe.PackString(GameSettings.ServerWebSite, 8);
		pe.PackString(GameSettings.ServerInfo, 8);
		pe.PackString(FileNameOnly(GameSettings.MapFile), 8);
		//New data on tail.
		pe.PackUInt(GameVersion.v[0], 8);	//Version of server.
		pe.PackUInt(GameVersion.v[1], 8);
		pe.PackUInt(GameVersion.v[2], 8);
		pe.PackUInt(GameVersion.v[3], 8);
		pe.PackUInt(GameState.NumAITanks, 8);	//Bot count.
		pe.PackUInt(GameSettings.DedicatedFPS, 8);	//Server FPS.
		pe.PackUInt(GameSettings.TimeLimit, 8);
		pe.PackUInt(GameSettings.FragLimit, 8);
		pe.PackString(GameSettings.ServerCorrectedIP, 8);
		//
		VW.Net.SendOutOfBandPacket(dest, (char*)pe.Data(), pe.BytesUsed());
		OutputDebugLog("Sent HeartBeat to " + CStr(inet_ntoa(dest->sin_addr)) + ", port " + String((int)ntohs(dest->sin_port)) + ".\n", 1);
		return 1;
	}
	return 0;
}

CStr CTankGame::RandTankType(){	//Chooses a random tank type, taking into account the type limiting variable.
	int r = 0, ok = 0, sanity = 100;
	while(ok == 0 && sanity > 0){
		r = TMrandom() % std::max(1, NumTankTypes);
		sanity--;
		if(GameSettings.LimitTankTypes == 0 || (GameSettings.LimitTankTypes == 1 && TankTypes[r].liquid == 0) ||
			(GameSettings.LimitTankTypes > 1 && TankTypes[r].liquid) ){	//Now obay limit types rules.
			ok = 1;
		}
	}
	return TankTypes[r].type;
}

bool CTankGame::StartGame(){
	EntityBase *ent;
	EntityBase *PlayerEnt;
	//
	GameState.CurrentLeader = 0;
	GameState.LastLeader = 0;

	//
	if(GameState.DemoMode){	//Do every-race demo mode setup stuff, so we can call StartGame from within demo mode and run a new demo level.
		int map;
		do
		{
			map = TMrandom() % std::max(1, NumMaps);
			GameSettings.MapFile = Maps[map].file;
			GameSettings.LimitTankTypes = TMrandom() % 3;
			GameSettings.TankType = RandTankType();//TankTypes[rand() % std::max(1, NumTankTypes)].type;
			GameSettings.Deathmatch = TMrandom() & 1;
			GameSettings.MirroredMap = TMrandom() & 1;
			if(IsMapDMOnly(map)) GameSettings.Deathmatch = 1;
		} while (Maps[map].maptype == MapTypeTraining);
	}
	//
	MasterNet.ClientDisconnect();
	VW.Net.ClientDisconnect(&VW);	//Make sure connections are empty when re-starting a game.
	VW.Net.ServerDisconnect(CLIENTID_BROADCAST, &VW);
	//
	//
	VW.snd.PauseMusic(true);
	VW.snd.StopAll();
	//
	VW.SetChatMode(0);
	VW.ClearChar();
	VW.SetFocus(NULL);
	//
	unsigned int gm = GAMEMODE_NOCANDRIVE;
	if(GameSettings.Deathmatch) gm |= GAMEMODE_DEATHMATCH;
	else gm |= GAMEMODE_RACE;
	if(GameSettings.TeamPlay){
		gm |= GAMEMODE_TEAMPLAY;
		if(GameSettings.TeamDamage) gm |= GAMEMODE_TEAMDAMAGE;
		if(GameSettings.TeamScores) gm |= GAMEMODE_TEAMSCORES;
	}

	for (int i = 0; i < NumMaps; i++)
	{
		if (Maps[i].file == GameSettings.MapFile)
		{
			if(Maps[i].maptype == MapTypeTraining)
			{
				gm |= GAMEMODE_TRAINING;
				break;
			}
		}
	}

	VW.SetGameMode(gm);
	//
	VW.BeginningOfTime();
//	TimeSinceRaceEnd = 0;
	GameState.CoolDownCounter = 0;
	GameState.SomeoneWonRace = 0;
	//
	GameState.HowitzerTime = 0;
	//
	if(GameState.ActAsServer){
		//
		LoadingStatus.Status(Text_GetLine(TEXT_STATUSLOADINGMAP));
		LoadingStatus.Draw(GenBuf);
		//
		GameState.CountdownTimer = GameSettings.StartDelay + 1;
		//
		int cl = std::min(MAX_CLIENTS, GameSettings.MaxClients);

		if((GameState.LadderMode || !GameSettings.AllowSinglePlayerJoins) && !GameSettings.DedicatedServer) cl = 0;	//No connections when playing ladder game.
		VW.InitServer(GameSettings.ServerPort, cl);
		//
		VW.Net.SetServerRate(GameSettings.ServerRate);
		//
		if(GameSettings.DedicatedServer){
			if(GameSettings.SendHeartbeats){	//Heartbeats to masters.  Listen servers won't be allowed to heartbeat for now, as there's no frame limiting on packet sending.
				//Start by looking up master addresses.
				for(int i = 0; i < MAX_MASTERS; i++){
					if(Master[i].address.len() > 1){
						if(VW.Net.LookupAddress(Master[i].address, &Master[i].ip)){
							if(Master[i].ip.sin_port == 0) Master[i].ip.sin_port = htons(GameSettings.MasterPort);
						}else{
							Master[i].ip.sin_addr.s_addr = 0;	//Lookup failed.
						}
					}
				}
			}
			//
			//Select a map from the list the dedicated server should play.
			if(NumDediMaps > 0){
				if(GameSettings.DediMapMode){	//Random maps.
					int realnum = 0;	//The usual "count alive entries, pick random from 0 to alive, find nth alive entry" pattern.
					for(int i = 0; i < NumDediMaps; i++) if(DediMaps[i].FileName.len() > 0) realnum++;
					if(realnum > 0){
						int z = TMrandom() % realnum;
						realnum = 0;
						for(int i = 0; i < NumDediMaps; i++){
							if(DediMaps[i].FileName.len() > 0) realnum++;
							if(z + 1 == realnum){	//Found the slot.
								GameState.CurDediMap = i;
								GameSettings.MapFile = DediMaps[GameState.CurDediMap].FileName;	//Got it.
								GameSettings.NumTeams = CTankGame::Get().GetMap(DediMaps[GameState.CurDediMap].Index)->MaxMapTeams;
								break;
							}
						}
					}
					GameSettings.MirroredMap = TMrandom() & 1;
				}else{	//Cycle maps.
					int safe = MaxMaps, oldCur = GameState.CurDediMap;
					do{
						GameState.CurDediMap = (GameState.CurDediMap + 1) % MAX(1, NumDediMaps);
						safe--;
					}while(safe && (GameState.CurDediMap < 0 || GameState.CurDediMap >= NumDediMaps || DediMaps[GameState.CurDediMap].FileName.len() <= 0));
					if(GameState.CurDediMap <= oldCur) GameSettings.MirroredMap = !GameSettings.MirroredMap;	//Cycle mirroring when maps wrap.
					//
					if(DediMaps[GameState.CurDediMap].FileName.len() > 0){
						GameSettings.MapFile = DediMaps[GameState.CurDediMap].FileName;	//Got next in cycle.
						GameSettings.NumTeams = CTankGame::Get().GetMap(DediMaps[GameState.CurDediMap].Index)->MaxMapTeams;
						OutputDebugLog("Cycling to Dedicated Map " + String(GameState.CurDediMap) + ", \"" + GameSettings.MapFile + "\".\n");
					}
				}
			}
		}
		//
		if(GameState.LadderMode) GameSettings.MirroredMap = Ladder.Mirrored;	//Read mirrored status from ladder data.
		//
		if(!VW.LoadVoxelWorld(GameSettings.MapFile, true, GameSettings.MirroredMap)){
			VW.ClearVoxelWorld();
			//
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSMAPFAIL));
			LoadingStatus.Draw(GenBuf);
			//
			Sleep(1000);
			LoadingStatus.Status("");
			return 0;
			//return Cleanup();
		}
		GameState.NumAITanks = 0;
		VW.ListAllResources();
		//
		//
		LoadingStatus.Status(Text_GetLine(TEXT_STATUSSHADING));
		LoadingStatus.Draw(GenBuf);
		//
		EntityBase *e = VW.AddEntity("tankgod", "sabot", NULL, NULL, NULL, 0, 0, 0, 0);
		//Resurrect our Diety to Control the World.  Muwahahahaha!
		if(e){
			e->SetInt(ATT_RACE_LAPS, GameSettings.Laps);	//Tell God how many laps the race should be.
		}
		//

		if(!GameSettings.DedicatedServer){
			//
			OutputDebugLog("Light shading map.\n");
			VW.TextureVoxelWorld32();
			//
			LoadingStatus.Status(Text_GetLine(TEXT_STATUSDOWNLOAD));
			LoadingStatus.Draw(GenBuf);
			//
			VW.DownloadTextures();	//Get terrain textures to card.
			//
			if(GameState.LadderMode)
				GameSettings.PlayerName = Ladder.PlayerName;
			//
			PlayerEnt = VW.AddEntity("racetank", GameSettings.TankType, NULL, NULL, NULL, 0, 0, 0, 0);
			if(PlayerEnt){
				//
				PlayerEnt->SetInt(ATT_AUTODRIVE, GameState.AutoDrive);
				PlayerEnt->SetInt(ATT_PLAYERTANK, true);
				PlayerEnt->SetString(ATT_NAME, GameSettings.PlayerName);
				//
				int tmpType;
				if (VW.MapCfg.FindKey("MapType"))
					VW.MapCfg.GetIntVal(&tmpType);

				PlayerEnt->SetInt(ATT_SPAWNATFIRSTWAYPOINT, tmpType == MapTypeTraining); // spawn at first waypoint if map is training map
				//
				PlayerEnt->SetInt(ATT_CMD_FIRSTSPAWN, true);	//Make sure playertank gets set first!
			}
		}
		//
		LoadingStatus.Status(Text_GetLine(TEXT_STATUSCREATING));
		LoadingStatus.Draw(GenBuf);
		//
		if(GameSettings.DedicatedServer){	//Pre-cache all tank resources on dedicated server.
			for(int i = 0; i < NumTankTypes; i++){
				VW.CacheResources("racetank", TankTypes[i].type);
			}
		}
		//
		if(GameState.LadderMode) GameSettings.AIAutoFillCount = Ladder.GetAICount();
		//
		int i;
		for(i = GameState.NumAITanks; i < GameSettings.AIAutoFillCount; i++){
			ent = VW.AddEntity("racetank", RandTankType(), NULL, NULL, NULL, 0, 0, 0, 0);
			if(ent){
				GameState.NumAITanks++;
				//
				ent->SetInt(ATT_CMD_FIRSTSPAWN, true);
				//
				ent->SetFloat(ATT_ACCEL, 0);
				ent->SetFloat(ATT_STEER, 0);
				ent->SetFloat(ATT_TURRETSTEER, 0);
				ent->SetInt(ATT_AUTODRIVE, true);
				//
				if(GameState.LadderMode){
					ent->SetString(ATT_NAME, GameSettings.AIPrefix + Ladder.GetAIName(i));	//Setup opponents from ladder window.
					ent->SetFloat(ATT_SKILL, Ladder.GetAISkill(i));
				}else{
					//
					ent->SetFloat(ATT_SKILL, GameSettings.EnemySkill);
					//
					NextTankName = (NextTankName + 1) % MAX(1, NumTankNames);	//Cycle through tank names.
					if(NumTankNames > 0) ent->SetString(ATT_NAME, GameSettings.AIPrefix + TankNames[NextTankName]);
					//
					if(GameSettings.TeamPlay){
						if(AITankTeam){
							ent->SetInt(ATT_INSIGNIA1_HASH, AITankTeam);	//All AI tanks go on one team.
						}else{
							ent->SetInt(ATT_INSIGNIA1_HASH, Teams[GetTeamFillSpot()].hash);	//AI tanks go on all teams.
						}
					}
				}
				//
			}
		}
		//
		LoadingStatus.Status("");
		//
	}else{	//We're a client.
		VW.BeginClientConnect(GameSettings.ServerAddress, GameSettings.ServerPort, GameSettings.ClientPort, VW.FindEntityType("racetank", GameSettings.TankType), GameSettings.PlayerName, GameSettings.ClientRate);
		OutputDebugLog("Sent Connection Req.\n");
		GameState.ConnectedToAddress = GameSettings.ServerAddress;	//Record address we last connected to, for level change reconnects.
	}
	return true;
}

int CTankGame::Cleanup(){
	//Kick clients/server off net connection.
	VW.Net.ClientDisconnect();
	VW.Net.ServerDisconnect(CLIENTID_BROADCAST);
	VW.Net.Free();
	MasterNet.ClientDisconnect();
	MasterNet.ServerDisconnect(CLIENTID_BROADCAST);
	MasterNet.Free();
	//Free sound a little earlier.
	VW.FreeSound();
	return 1;
}

int CTankGame::IsGameRunning(){
	if(VW.Map.Width() > 0 && VW.Map.Height() > 0) return 1;
	return 0;
}

bool CTankGame::StopGame(){
	//
	LoadingStatus.Status(Text_GetLine(TEXT_STATUSWAIT));
	LoadingStatus.Draw(GenBuf);
	//
	MasterNet.ClientDisconnect();
	VW.Net.ClientDisconnect(&VW);	//Make sure connections are empty when re-starting a game.
	VW.Net.ServerDisconnect(CLIENTID_BROADCAST, &VW);
	VW.Net.FreeServer();
	VW.Net.FreeClient();

	VW.snd.PauseMusic(true);
	VW.snd.StopAll();
	//
	VW.SetChatMode(0);
	VW.ClearChar();
	VW.SetFocus(NULL);
	//
	VW.ClearVoxelWorld();	//Frees the map and all textures.
	GameState.NumAITanks = 0;
	//
	CurrentMenu = MP_Main;
	//
	return true;
}

typedef struct CommandEntry
{
	char sEntry[255];
	CommandEntry *next;
} CommandEntry;

void CTankGame::ProcessServerCommand(char* sCommand)
{
	char sDelim[] = " \t\n";
	char *sTmp;
	CommandEntry	FirstCommand;
	CommandEntry	*TmpCommand;

	sTmp = strtok(sCommand, sDelim);
	strcpy(FirstCommand.sEntry, sTmp);
	FirstCommand.next = NULL;

	TmpCommand = &FirstCommand;
	while(sTmp = strtok(NULL, sDelim))
	{
		TmpCommand->next = new CommandEntry;
		TmpCommand = TmpCommand->next;
		TmpCommand->next = NULL;
		if(sTmp[0] == '"') // we have a quoted string, need to include whitespace
		{
			char sQuoteDelim[] = "\"\n";
			char *sTmp2 = strtok(NULL, sQuoteDelim);

			strcpy(TmpCommand->sEntry, &sTmp[1]);

			if (sTmp2 != NULL) // we had another token
			{
				TmpCommand->sEntry[strlen(sTmp)-1] = ' '; // inserting a space ... might have been a newline or a tab, but now its a space
				TmpCommand->sEntry[strlen(sTmp)] = NULL;

				strcpy(&TmpCommand->sEntry[strlen(sTmp)], sTmp2);
			}
			// we might have a trailing " to remove
			if(TmpCommand->sEntry[strlen(TmpCommand->sEntry)-1] == '"')
				TmpCommand->sEntry[strlen(TmpCommand->sEntry)-1] = NULL;
		}
		else
		{
			strcpy(TmpCommand->sEntry, sTmp);
		}
	}

	if(strcasecmp(FirstCommand.sEntry, "list") == 0)
	{
		char	sOutput[1024];
		int iCount = 0;
#ifndef HEADLESS
		sprintf(sOutput, "%s\n", Text.Get(TEXT_DEDI_LISTHEADER));
		GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if(VW.Clients[i].ClientName[0] != NULL)
			{
				EntityBase* ClientEnt;
				if(ClientEnt = VW.GetEntity(VW.Clients[i].ClientEnt))
				{
#ifndef HEADLESS
					sprintf(sOutput, "%d\t%d\t%d\t%s\n", i, VW.Clients[i].TeamID, ClientEnt->QueryInt(ATT_FRAGS), VW.Clients[i].ClientName.get());
					GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
					iCount++;
				}
			}
		}
#ifndef HEADLESS
		sprintf(sOutput, "%s %d\n", Text.Get(TEXT_DEDI_TOTAL), iCount);
		GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
	}
	else if (strcasecmp(FirstCommand.sEntry, "kick") == 0)
	{
		if (FirstCommand.next != NULL)
		{
			bool bKicked = false;
			char sOutput[1024];
			long iKickTime = 0;

			if (FirstCommand.next->next != NULL)
			{
				iKickTime = atoi(FirstCommand.next->next->sEntry);
				sprintf(sOutput, Text.Get(TEXT_DEDI_KICKINGPLAYERFORTIME), FirstCommand.next->sEntry, iKickTime);
			}
			else
			{
				sprintf(sOutput, Text.Get(TEXT_DEDI_KICKINGPLAYER), FirstCommand.next->sEntry);
			}
#ifndef HEADLESS
			sprintf(sOutput, "%s\n", sOutput);
			GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(strcasecmp(VW.Clients[i].ClientName, FirstCommand.next->sEntry) == 0)
				{
					VW.Net.BanClient(i, iKickTime);

					VW.Net.ServerDisconnect(i);
					EntityBase *e = VW.GetEntity(VW.Clients[i].ClientEnt);
					e->Remove();
#ifndef HEADLESS
					sprintf(sOutput, "%s\n", Text.Get(TEXT_DEDI_DISCSENT));
					GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif

					bKicked = true;
					sprintf(sOutput, Text.Get(TEXT_DEDI_KICKEDBYSERVEROP), FirstCommand.next->sEntry);
					VW.StatusMessage(sOutput);
				}
			}
			if (bKicked == false)
			{
#ifndef HEADLESS
				sprintf(sOutput, "%s\n", Text.Get(TEXT_DEDI_NOPLAYERSBYNAME));
				GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
			}
		}
	}
	else if (strcasecmp(FirstCommand.sEntry, "addai") == 0)
	{
		GameSettings.AIAutoFillCount++;
	}
	else if (strcasecmp(FirstCommand.sEntry, "listai") == 0)
	{
		char	sOutput[1024];
		int		iCount = 0;
		EntityBase	*ent = VW.EntHead->NextLink();

#ifndef HEADLESS
		sprintf(sOutput, "%s\n", Text.Get(TEXT_DEDI_LISTAIHEADER));
		GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
		while (ent != NULL)
		{
			if (ent->QueryInt(ATT_AUTODRIVE))
			{
#ifndef HEADLESS
				sprintf(sOutput, "%d\t%d\t%s\n", ent->QueryInt(ATT_INSIGNIA1_HASH), ent->QueryInt(ATT_FRAGS), ent->QueryString(ATT_NAME).get());
				GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
				iCount++;
			}
			ent = ent->NextLink();
		}
#ifndef HEADLESS
		sprintf(sOutput, "%s %d\n", Text.Get(TEXT_DEDI_TOTAL), iCount);
		GetDedicatedWin()->OutputFunc(sOutput, 0);
#endif
	}
	else if (strcasecmp(FirstCommand.sEntry, "kickai") == 0)
	{
		if (FirstCommand.next != NULL)
		{
			char	sOutput[1024];
			int		iCount = 0;
			EntityBase	*ent = VW.EntHead->NextLink();

			while (ent != NULL)
			{
				if (ent->QueryInt(ATT_AUTODRIVE))
				{
					if (strcasecmp(ent->QueryString(ATT_NAME).get(), FirstCommand.next->sEntry) == 0)
					{
						sprintf(sOutput, Text.Get(TEXT_DEDI_REMOVEFROMGAME), ent->QueryString(ATT_NAME).get());
						VW.StatusMessage(sOutput);
						ent->Remove();
						GameState.NumAITanks--;
						GameSettings.AIAutoFillCount--;
					}
				}
				ent = ent->NextLink();
			}
		}
	}

	TmpCommand = FirstCommand.next;
	while (TmpCommand != NULL)
	{
		CommandEntry *TmpCommand2 = TmpCommand->next;
		delete TmpCommand;
		TmpCommand = TmpCommand2;
	}
}

int CTankGame::EnumerateTeams(){
	int cookie = 0;
	EntityTypeBase *et;
	NumAvailTeams = 0;
	while((cookie = VW.EnumEntityTypes("insignia", &et, cookie)) && NumAvailTeams < MaxTeams){
		if(et->InterrogateInt("team")){	//It is a team insignia.
			Teams[NumAvailTeams].name = et->dname;
			Teams[NumAvailTeams].hash = et->thash;
			NumAvailTeams++;
		}
	}
	//Apply limiting options to team flags.
	if(GameSettings.OptAITankTeam.len() > 0){	//What team are all the AI tanks on?  Otherwise on all teams.
		AITankTeam = Teams[0].hash;	//In case not found.
		for(int i = 0; i < NumAvailTeams; i++){
			if(CmpLower(Teams[i].name, GameSettings.OptAITankTeam)){
				AITankTeam = Teams[i].hash;
				break;
			}
		}
	}
/*	if(OptTeams[0].len() > 0){
		for(int i = 0; i < NumAvailTeams; i++) Teams[i].teamok = 0;	//Set all teams to Not OK.
		for(int j = 0; j < MaxTeams; j++){
			if(OptTeams[j].len() > 0){
				for(int i = 0; i < NumAvailTeams; i++){	//Loop through real team list and flag named team as ok.
					if(CmpLower(Teams[i].name, OptTeams[j])){
						Teams[i].teamok = 1;
						break;
					}
				}
			}
		}//Now collapse team list, leaving only OK teams.
		for(int k = NumAvailTeams - 1; k > 0; k--){
			if(Teams[k].teamok == 0){	//Hole at end of list, just reduce length.
				NumAvailTeams--;
			}else{	//Team at end of list, look for first hole to copy it into.
				for(int i = 0; i < NumAvailTeams - 1; i++){
					if(Teams[i].teamok == 0){	//Found empty hole.
						Teams[i] = Teams[k];	//Copy tail team in.
						NumAvailTeams--;	//Reduce count.
						break;	//Quit loop.
					}
				}
			}
		}
	}
*/	return NumAvailTeams;
}

int CTankGame::EnumerateTanks(){
	NumTankTypes = 0;
	int cookie = 0;
	EntityTypeBase *et = 0;
	while((cookie = VW.EnumEntityTypes("racetank", &et, cookie)) && NumTankTypes < MaxTankTypes){
		if(et->dname.len() > 0){
			TankTypes[NumTankTypes].title = et->dname;
			TankTypes[NumTankTypes].type = et->tname;
			TankTypes[NumTankTypes].hash = et->thash;
			if(CmpLower(Left(et->dname, 7), "liquid ")) TankTypes[NumTankTypes].liquid = 1;
			else TankTypes[NumTankTypes].liquid = 0;
			NumTankTypes++;
		}
	}
	//
	//Sort tank names alphabetically.
	if(NumTankTypes > 0){
		TankInfo t;
		for(int i = 0; i < NumTankTypes; i++){
			for(int j = 0; j < NumTankTypes - 1; j++){
				int swap = 0;
				if(TankTypes[j].liquid == TankTypes[j + 1].liquid){	//If we are in the same group, then...
					if(strcmp(TankTypes[j].title, TankTypes[j + 1].title) > 0) swap = 1;	//Alphabetically compare.
				}else if(TankTypes[j].liquid && !TankTypes[j + 1].liquid) swap = 1;	//Otherwise, if liquid is earlier, swap it to later.
				if(swap){
					t = TankTypes[j];
					TankTypes[j] = TankTypes[j + 1];
					TankTypes[j + 1] = t;
				}
			}
		}
	}
	return NumTankTypes;
}

int CTankGame::IsMapDMOnly(int num){
	if(num >= 0 && num < NumMaps && Maps[num].maptype == MapTypeDeathMatch) return 1;
	return 0;
}

int CTankGame::ReadConfigCfg(const char *name){
	int i;
	//Read config file.
	if(cfg.Read(name)){//"config.cfg")){
		if(cfg.FindKey("animdir")) cfg.GetStringVal(GameSettings.AnimationDir);
		if(cfg.FindKey("tanktype")) cfg.GetStringVal(GameSettings.TankType);
		if(cfg.FindKey("glwidth")) cfg.GetIntVal(&GameSettings.GraphicsSettings.GLBWIDTH);
		if(cfg.FindKey("glheight")) cfg.GetIntVal(&GameSettings.GraphicsSettings.GLBHEIGHT);
		
#ifndef HEADLESS
		// Check the video mode
		{
			auto mode = sf::VideoMode::getDesktopMode();
			mode.width = GameSettings.GraphicsSettings.GLBWIDTH;
			mode.height = GameSettings.GraphicsSettings.GLBHEIGHT;
			if(!mode.isValid())
			{
				mode = sf::VideoMode::getDesktopMode();
				GameSettings.GraphicsSettings.GLBWIDTH = mode.width;
				GameSettings.GraphicsSettings.GLBHEIGHT = mode.height;
			}
		}
#endif

		if(cfg.FindKey("animfps")) cfg.GetIntVal(&GameSettings.AnimFPS);
		if(cfg.FindKey("logfilename")) { cfg.GetStringVal(GameSettings.LogFileName); GameSettings.LogFileName = GetCommonAppDataDir() + GameSettings.LogFileName; }
		if(cfg.FindKey("logfileactive")) cfg.GetBoolVal(&GameSettings.LogFileActive);
//		if(cfg.FindKey("autodrive")) cfg.GetBoolVal(&AutoDrive);
		if(cfg.FindKey("viewdistance")) cfg.GetFloatVal(&GameSettings.GraphicsSettings.ViewDistance);
		if(cfg.FindKey("NumTanks")) cfg.GetIntVal(&GameSettings.AIAutoFillCount);
		if(cfg.FindKey("AIAutoFill")) cfg.GetIntVal(&GameSettings.AIAutoFill);
		if(cfg.FindKey("quality")) cfg.GetFloatVal(&GameSettings.GraphicsSettings.Quality);
		if(cfg.FindKey("TreadMarks")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.TreadMarks);
		if(cfg.FindKey("DetailTerrain")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.DetailTerrain);
		if(cfg.FindKey("WireFrame")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.WireFrame);
		if(cfg.FindKey("FullScreen")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.UseFullScreen);
		if(cfg.FindKey("MaxTexRes")) cfg.GetIntVal(&GameSettings.GraphicsSettings.MaxTexRes);
		if(cfg.FindKey("PolyLod")) cfg.GetIntVal(&GameSettings.GraphicsSettings.PolyLod);
		if(cfg.FindKey("Particles")) cfg.GetFloatVal(&GameSettings.GraphicsSettings.Particles);
		if(cfg.FindKey("Music")) cfg.GetBoolVal(&GameSettings.PlayMusic);
		if(cfg.FindKey("MusicVolume")) cfg.GetFloatVal(&GameSettings.MusicVol);
		if(cfg.FindKey("SoundVolume")) cfg.GetFloatVal(&GameSettings.SoundVol);
		if(cfg.FindKey("Laps")) cfg.GetIntVal(&GameSettings.Laps);
		if(cfg.FindKey("MapFile")) cfg.GetStringVal(GameSettings.MapFile);
		if(cfg.FindKey("DisableMT")) cfg.GetIntVal(&GameSettings.GraphicsSettings.DisableMT);
		if(cfg.FindKey("DebugPolyNormals")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.DebugPolyNormals);
		if(cfg.FindKey("Fog")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.UseFog);
		if(cfg.FindKey("Joystick")) cfg.GetIntVal(&GameSettings.InputSettings.UseJoystick);
		if(cfg.FindKey("Deathmatch")) cfg.GetIntVal(&GameSettings.Deathmatch);
		if(cfg.FindKey("StartDelay")) cfg.GetIntVal(&GameSettings.StartDelay);
		if(cfg.FindKey("DedicatedServer")) cfg.GetBoolVal(&GameSettings.DedicatedServer);
		if(cfg.FindKey("DedicatedFPS")) cfg.GetIntVal(&GameSettings.DedicatedFPS);
		if(cfg.FindKey("MaxFPS")) cfg.GetIntVal(&GameSettings.MaxFPS);
		if(cfg.FindKey("PlayerName")) cfg.GetStringVal(GameSettings.PlayerName);
		if(_strnicmp(GameSettings.AIPrefix, GameSettings.PlayerName, strlen(GameSettings.AIPrefix)) == 0)
			GameSettings.PlayerName = "Player";
		if(cfg.FindKey("Trilinear")) cfg.GetIntVal(&GameSettings.GraphicsSettings.Trilinear);
		if(cfg.FindKey("ClientRate")) cfg.GetIntVal(&GameSettings.ClientRate);
		if(cfg.FindKey("AlphaTest")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.UseAlphaTest);
		if(cfg.FindKey("MenuFire")) cfg.GetIntVal(&GameSettings.GraphicsSettings.MenuFire);
//		if(cfg.FindKey("OverrideRegistry")) cfg.GetIntVal(&OverrideRegistry);
		if(cfg.FindKey("MouseSpeed")) cfg.GetFloatVal(&GameSettings.InputSettings.MouseSpeed);
		if(cfg.FindKey("TexCompress")) cfg.GetBoolVal(&GameSettings.GraphicsSettings.TexCompress);
		if(cfg.FindKey("AISkill")) cfg.GetFloatVal(&GameSettings.EnemySkill);
		if(cfg.FindKey("LimitTankTypes")) cfg.GetIntVal(&GameSettings.LimitTankTypes);
		if(cfg.FindKey("HowitzerTime")) cfg.GetIntVal(&GameSettings.HowitzerTimeStart);
		if(cfg.FindKey("HowitzerTimeScale")) cfg.GetFloatVal(&GameSettings.HowitzerTimeScale);
		if(cfg.FindKey("HiFiSound")) cfg.GetIntVal(&GameSettings.HiFiSound);
		if(cfg.FindKey("BypassIntro")) cfg.GetIntVal(&GameSettings.BypassIntro);
		if(cfg.FindKey("TurretSpeedScale")) cfg.GetFloatVal(&TurretSpeedScale);
		if(cfg.FindKey("ShowMPH")) cfg.GetIntVal(&GameSettings.ShowMPH);
		//
		if(cfg.FindKey("MaxClients")) cfg.GetIntVal(&GameSettings.MaxClients);
		for(i = 0; i < MAX_MASTERS; i++){
			if(cfg.FindKey("MasterServerUrl" + String(i))) cfg.GetStringVal(Master[i].address);
		}
		if(cfg.FindKey("TimeLimit")) cfg.GetIntVal(&GameSettings.TimeLimit);
		if(cfg.FindKey("FragLimit")) cfg.GetIntVal(&GameSettings.FragLimit);
		if(cfg.FindKey("CoolDownTime")) cfg.GetIntVal(&GameSettings.CoolDownTime);
		if(cfg.FindKey("MasterAddressUrl")) cfg.GetStringVal(GameSettings.MasterAddress);
		if(cfg.FindKey("SendHeartbeats")) cfg.GetIntVal(&GameSettings.SendHeartbeats);
		if(cfg.FindKey("ClientPort")) cfg.GetIntVal(&GameSettings.ClientPort);
		if(cfg.FindKey("ServerPort")) cfg.GetIntVal(&GameSettings.ServerPort);
		if(cfg.FindKey("MasterPort")) cfg.GetIntVal(&GameSettings.MasterPort);
		if(cfg.FindKey("MasterPingsPerSecond")) cfg.GetIntVal(&GameSettings.MasterPingsPerSecond);
		if(cfg.FindKey("MasterPings")) cfg.GetIntVal(&GameSettings.MasterPings);
		//
		if(cfg.FindKey("ServerRate")) cfg.GetIntVal(&GameSettings.ServerRate);
		if(cfg.FindKey("ServerName")) cfg.GetStringVal(GameSettings.ServerName);
		if(cfg.FindKey("ServerInfo")) cfg.GetStringVal(GameSettings.ServerInfo);
		if(cfg.FindKey("ServerWebSite")) cfg.GetStringVal(GameSettings.ServerWebSite);
		if(cfg.FindKey("ServerCorrectedIP")) cfg.GetStringVal(GameSettings.ServerCorrectedIP);
		//
		for(i = 0; i < MaxMaps; i++){
			if(cfg.FindKey("DediMap" + String(i))){
				cfg.GetStringVal(DediMaps[i].FileName);
				NumDediMaps = i + 1;
			}
		}
		if(cfg.FindKey("RandomMaps")) cfg.GetIntVal(&GameSettings.DediMapMode);
		//
		if(cfg.FindKey("TeamPlay")) cfg.GetIntVal(&GameSettings.TeamPlay);
		if(cfg.FindKey("TeamDamage")) cfg.GetIntVal(&GameSettings.TeamDamage);
		if(cfg.FindKey("TeamScores")) cfg.GetIntVal(&GameSettings.TeamScores);
		if(cfg.FindKey("AITankTeam")) cfg.GetStringVal(GameSettings.OptAITankTeam);	//What team are all the AI tanks on?  Otherwise on all teams.
		if(cfg.FindKey("MaxAllowedTeams")) cfg.GetIntVal(&MaxAllowedTeams);
		//
/*		if(cfg.FindKey("Team0")){
			for(int j = 0; j < MaxTeams; j++){	//Can't be any more specific teams than there are real teams.
				if(cfg.FindKey("Team" + String(j))) cfg.GetStringVal(OptTeams[j]);
			}
		}
*/		//
		return 1;
	}
	return 0;
}

void CTankGame::NewMusic()
{
	if(NumMusicFiles > 0)
	{
		GameState.sMusicFile = MusicFiles[NextMusicFile];
        
        auto sMessage = Text.Get(TEXT_NOWPLAYINGMUSIC);
        if(sMessage)
        {
            char	tmpString[1024];
            sprintf(tmpString, sMessage, GameState.sMusicFile.get());
            VW.StatusMessage(tmpString, STATUS_PRI_NETWORK, CLIENTID_NONE, TEAMID_NONE, true);
        }
		NextMusicFile++;
		if (NextMusicFile >=NumMusicFiles)
			NextMusicFile = 0;
	}
}

void CTankGame::SortMaps()
{
	if(NumMaps > 0){
		MapInfo t;
		for(int i = 0; i < NumMaps; i++){
			for(int j = 0; j < NumMaps - 1; j++)
			{
				int swap = 0;
				if (Maps[j].MapID > Maps[j + 1].MapID)
					swap = 1;
				if(swap)
				{
					t = Maps[j];
					Maps[j] = Maps[j + 1];
					Maps[j + 1] = t;
				}
			}
		}
	}
}

void CTankGame::LoadTankNames(CStr sFilename)
{
	FILE *F;

	if(F = VW.FM.Open(sFilename))
	{
		char buf[256];
		while(NumTankNames < MaxTankNames && fgets(buf, 255, F)){
			CStr b = buf;
			int n = Instr(b, "\n");	//Eat end of line chars.
			int r = Instr(b, "\r");
			if(n < 0) n = b.len();
			if(r < 0) r = b.len();
			TankNames[NumTankNames] = Left(b, MIN(n, r));
			if(TankNames[NumTankNames].len() > 1) NumTankNames++;
		}
		//Now shuffle the names.
		for(int i = 0; i < NumTankNames; i++){
			int j = TMrandom() % NumTankNames;
			CStr tmp = TankNames[i];
			TankNames[i] = TankNames[j];
			TankNames[j] = tmp;
		}
	}
}

void CTankGame::LoadMaps()
{
	FILE *F;

	VW.FM.PushFile();

	for(F = VW.FM.OpenWildcard("*.ved", NULL, 0, false, false); (F) && GetNumMaps() < MaxMaps; F = VW.FM.NextWildcard()){
		IFF iff;
		if(iff.OpenIn(F) && iff.FindChunk("SCFG")){
			int len = iff.ReadLong();
			char *buf;
			int iMapCounter = 0;
			if(len > 0 && (buf = (char*)malloc(len + 1))){
				iff.ReadBytes(buf, len);
				buf[len] = 0;
				ConfigFile cfg;
				cfg.ReadMemBuf(buf, len);
				free(buf);
				if(cfg.FindKey("mapname")){
					cfg.GetStringVal(Maps[NumMaps].title);

					if(cfg.FindKey("mapid"))
					{
						cfg.GetIntVal((int*)&(Maps[NumMaps].MapID));
						Maps[NumMaps].MapID += 1000;
					}
					else
						Maps[NumMaps].MapID = iMapCounter++;

					Maps[NumMaps].file = VW.FM.GetFileName();
					if (cfg.FindKey("maptype"))
					{
						cfg.GetIntVal((int*)&(Maps[NumMaps].maptype));
					}
					else
					{
						if(CmpLower(Left(Maps[NumMaps].title, 3), "dm ")) Maps[NumMaps].maptype = MapTypeDeathMatch;
						else Maps[NumMaps].maptype = MapTypeRace;
					}

					Maps[NumMaps].MaxMapTeams = 99;
					Maps[NumMaps].GameType = -1;
					Maps[NumMaps].Laps = -1;
					Maps[NumMaps].AITanks = -1;
					Maps[NumMaps].TimeLimit = -1;
					Maps[NumMaps].FragLimit = -1;
					Maps[NumMaps].EnemySkill = -1;
					Maps[NumMaps].TankTypes = -1;
					Maps[NumMaps].StartDelay = -1;
					Maps[NumMaps].Mirror = -1;
					Maps[NumMaps].AllowJoins = -1;
					Maps[NumMaps].NumTeams = -1;
					Maps[NumMaps].TeamScores = -1;
					Maps[NumMaps].TeamDamage = -1;

					Maps[NumMaps].DisableFrags = 0;

					if (cfg.FindKey("GameType")) cfg.GetIntVal(&Maps[NumMaps].GameType);
					if (cfg.FindKey("Laps")) cfg.GetIntVal(&Maps[NumMaps].Laps);
					if (cfg.FindKey("AITanks")) cfg.GetIntVal(&Maps[NumMaps].AITanks);
					if (cfg.FindKey("TimeLimit")) cfg.GetIntVal(&Maps[NumMaps].TimeLimit);
					if (cfg.FindKey("FragLimit")) cfg.GetIntVal(&Maps[NumMaps].FragLimit);
					if (cfg.FindKey("EnemySkill")) cfg.GetFloatVal(&Maps[NumMaps].EnemySkill);
					if (cfg.FindKey("TankTypes")) cfg.GetIntVal(&Maps[NumMaps].TankTypes);
					if (cfg.FindKey("StartDelay")) cfg.GetIntVal(&Maps[NumMaps].StartDelay);
					if (cfg.FindKey("Mirror")) cfg.GetIntVal(&Maps[NumMaps].Mirror);
					if (cfg.FindKey("AllowJoins")) cfg.GetIntVal(&Maps[NumMaps].AllowJoins);

					if (cfg.FindKey("NumTeams")) cfg.GetIntVal(&Maps[NumMaps].NumTeams);
					if (cfg.FindKey("TeamScores")) cfg.GetIntVal(&Maps[NumMaps].TeamScores);
					if (cfg.FindKey("TeamDamage")) cfg.GetIntVal(&Maps[NumMaps].TeamDamage);
					if (cfg.FindKey("MaxMapTeams")) cfg.GetIntVal(&Maps[NumMaps].MaxMapTeams);

					if (cfg.FindKey("DisableFrags")) cfg.GetIntVal(&Maps[NumMaps].DisableFrags);

#ifdef CHINA
					CStr t = FileNameOnly(Maps[NumMaps].file);
					if( (CmpLower(t, "tutorial.ved") && CmpLower(Maps[NumMaps].title, "Driver Training"))){
						NumMaps++;
					}
					else if( (CmpLower(t, "tutorial2.ved") && CmpLower(Maps[NumMaps].title, "Weapons Training"))){
						NumMaps++;
					}
					else if( (CmpLower(t, "tutorial3.ved") && CmpLower(Maps[NumMaps].title, "Missile Training"))){
						NumMaps++;
					}
#else
					NumMaps++;
#endif
				}
			}
		}
	}
	VW.FM.PopFile();
}

void CTankGame::LoadDediMaps()
{
	if(NumDediMaps <= 0){	//None specified, so use all of them.
		NumDediMaps = 0;
		for(int i = 0; i < NumMaps; i++){
			if (GetMap(i)->maptype != MapTypeTraining)
			{
				if(GameSettings.Deathmatch == 1 || GetMap(i)->maptype == MapTypeRace)
				{
					DediMaps[NumDediMaps].FileName = GetMap(i)->file;
					DediMaps[NumDediMaps].Index = i;
					OutputDebugLog("Map File \"" + DediMaps[NumDediMaps].FileName + "\" added to Dedicated playlist.\n");
					NumDediMaps++;
				}
			}
		}
	}else{
		for(int m = 0; m < NumDediMaps; m++){
			int ok = 0, n = 0;
			for(n = 0; n < GetNumMaps(); n++){
				if(CmpLower(DediMaps[m].FileName, GetMap(n)->title)){	//A map was specified by title.
					OutputDebugLog("Dedicated Map Name \"" + DediMaps[m].FileName + "\" identified as file \"" + GetMap(n)->file + "\".\n");
					DediMaps[m].FileName = GetMap(n)->file;
					DediMaps[m].Index = n;
					ok = 1;
					break;
				}
				if(CmpLower(FileNameOnly(DediMaps[m].FileName), FileNameOnly(GetMap(n)->file))){	//A map was specified by filename.
					OutputDebugLog("Dedicated Map File \"" + DediMaps[m].FileName + "\" identified as file \"" + GetMap(n)->file + "\".\n");
					DediMaps[m].FileName = GetMap(n)->file;
					DediMaps[m].Index = n;
					ok = 1;
					break;
				}
			}
			if(ok && GameSettings.Deathmatch == 0 && GetMap(n)->maptype == MapTypeDeathMatch){
				OutputDebugLog("Dedicated Map \"" + DediMaps[m].FileName + "\" is Deathmatch Only!\n");
				DediMaps[m].FileName = "";
				DediMaps[m].Index = -1;
			}else{
				if(ok == 0){
					OutputDebugLog("Dedicated Map \"" + DediMaps[m].FileName + "\" not identified!\n");
					DediMaps[m].FileName = "";
					DediMaps[m].Index = -1;
				}
			}
		}
	}
}

void CTankGame::LoadMusic()
{
	VW.FM.PushFile();
	for(FILE *F = VW.FM.OpenWildcard("music/*.ogg", NULL, 0, false, false); (F) && NumMusicFiles < MaxMusicFiles; F = VW.FM.NextWildcard())
	{
		MusicFiles[NumMusicFiles++] = VW.FM.GetFileName();
	}
	VW.FM.PopFile();
}

void CTankGame::ShuffleMusic()
{
	for(int i = 0; i < NumMusicFiles; i++)
	{
		int n = TMrandom() % NumMusicFiles;
		CStr t = MusicFiles[i];
		MusicFiles[i] = MusicFiles[n];	//Swap with a random entry.
		MusicFiles[n] = t;
	}
}

int CTankGame::GetTeamFillSpot()
{
	int iMinTeam = -1;
	int iMinTeamTotal = 99999;

	for(int i = 0; i < GetNumTeams(); i++)
	{
		int iTeamTotal = 0;
		EntityBase	*ent = VW.EntHead->NextLink();

		while (ent != NULL)
		{
			if(ent->QueryInt(ATT_INSIGNIA1_HASH) == Teams[i].hash)
				iTeamTotal++;
			ent = ent->NextLink();
		}
		if(iTeamTotal < iMinTeamTotal)
		{
			iMinTeam = i;
			iMinTeamTotal = iTeamTotal;
		}
	}
	if(iMinTeam == -1)
		return TMrandom() % MAX(1, GetNumTeams());
	else
		return iMinTeam;
}

int CTankGame::TeamIndexFromHash(const int hash, int *index)
{
	for(int i = 0; i < NumAvailTeams; i++) // maybe this should be NumAvailTeams .. not sure
	{
		if(Teams[i].hash == hash)
		{
			*index = i;
			return 1;
		}
	}
	return 0;
}
