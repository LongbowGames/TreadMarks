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

#ifndef TANKGAME_H
#define TANKGAME_H

#include "VoxelWorld.h"
#include "GenericBuffer.h"
#include "SimpleStatus.h"
#include "StatsPackage.h"
#include "LadderManager.h"
#include "Heartbeat.h"
#include "CJoyInput.h"
#include "PacketProcessors.h"
#include "TextLine.hpp"

enum MapType {MapTypeRace = 0, MapTypeDeathMatch, MapTypeTraining};

struct MapInfo{
	CStr title, file;
	MapType maptype;
	int MapID;
	int GameType;
	int Laps;
	int AITanks;
	int TimeLimit;
	int FragLimit;
	float EnemySkill;
	int TankTypes;
	int StartDelay;
	int Mirror;
	int AllowJoins;
	int DisableFrags;
	int NumTeams;
	int TeamScores;
	int MaxMapTeams;
	int TeamDamage;
};

struct ServerEntryEx : public ServerEntry {
	CStr MapTitle;
	int PingTime, PingCount;
	ServerEntryEx() : PingTime(0), PingCount(0) { };
};

enum ControlEntryID{
	CEID_None = 0, CEID_Left, CEID_Right, CEID_Up, CEID_Down, CEID_TLeft, CEID_TRight, CEID_Fire,
	CEID_Chat, CEID_TeamChat, CEID_Scores, CEID_FreeLook, CEID_GunToFront, CEID_GunToBack, CEID_GunToCam,
	CEID_TurretCam, CEID_TiltCamUp, CEID_TiltCamDn, CEID_SpinCamLt, CEID_SpinCamRt, CEID_SpinCamBk, CEID_CamReset
};

struct ControlEntry{
	CStr name, ctrl1, ctrl2, ctrl3;
	ControlEntryID ctlid;
	void Set(const char *n, const char *c1, const char *c2, const char *c3, ControlEntryID id){
		name = n;  ctrl1 = c1;  ctrl2 = c2;  ctrl3 = c3; ctlid = id;
	};
};

struct TeamInfo{
	ClassHash hash;	//Insignia type hash.
	CStr name;
	int teamok;
};
#define MaxTeams 32

enum MasterSortMode{
	MSM_Name,
	MSM_Map,
	MSM_Mode,
	MSM_Time,
	MSM_Players,
	MSM_Ping
};

struct MasterServer{
	CStr address;
	sockaddr_in ip;
	MasterServer(){
		ip.sin_family = AF_INET;
		ip.sin_addr.s_addr = 0;
	};
};
#define MAX_MASTERS 10

struct CStrLink : public CStr, public LinklistBase<CStrLink> {
	CStrLink(){ };
	CStrLink(const char *c) : CStr(c) { };
};

struct TankInfo{
	CStr title, type;
	ClassHash hash;
	int liquid;
};

#define MAP_SNAP_SIZE 256 //Width and height of map snapshot files.

enum GameTypeID {RaceType = 0, DeathmatchType, TutorialType};

	#define SEND_STATUS_TIME 1000
	#define NumControls 21
	#define MaxTankNames 512
	#define MaxChatLines 100
	#define MaxMusicFiles 100
	#define MaxTankTypes 100
	#define MaxMaps 128

struct CGraphicsSettings
{
	int RendFlags;
	bool Stretch;
	bool UseFullScreen;
	int DisableMT;
	bool UseFog;
	bool UseAlphaTest;
	int MenuFire;
	int GLBWIDTH;
	int GLBHEIGHT;

	float Quality;
	bool WireFrame;
	bool StripFanMap;
	bool TreadMarks;
	bool DetailTerrain;
	int PolyLod;
	float Particles;

	int MaxTexRes;
	bool TexCompress;
	bool TexCompressAvail;
	int Trilinear;
	bool PalettedTextures;

	int DebugLockPatches;
	bool DebugPolyNormals;

	float ViewDistance;

	CGraphicsSettings();
};

struct CInputSettings
{
	float MouseSpeed;
	int InvMouseY;
	int MouseEnabled;
	int AxisMapping[K_MAXJOYAXES + 2];
	ControlEntry Controls[NumControls];
	int FirstCamControl;
	int UseJoystick;
	CStr sStickName;
	int StickID;
	float DeadZone;

	CInputSettings();
	void InitControls();
};

struct CGameSettings // This information is set at load time or changed with menus
{
	CInputSettings InputSettings;
	CGraphicsSettings GraphicsSettings;
	int HowitzerTimeStart;
	float HowitzerTimeScale;
	int ShowMPH;
	float EnemySkill;
	int ServerRate;
	bool LogFileActive;
	CStr LogFileName;
	int CamStyle;
	int BypassIntro;
	int LimitTankTypes;	//All, Steel, Liquid.
	int TeamPlay;
	int TeamDamage;
	int TeamScores;
	int NumTeams;
	int TimeLimit;
	int FragLimit;
	CStr AIPrefix;
	int HiFiSound;
	int ClientPort;
	int ServerPort;
	int MasterPort;
	int StartDelay;
	bool MirroredMap;
	int DediMapMode;	//1 is Randam.
	int CoolDownTime;
	int MaxClients;
	bool DedicatedServer;
	int DedicatedFPS;
	int MaxFPS;
	int SendHeartbeats;	//Heartbeats to Masters.
	CStr ServerName;
	CStr ServerWebSite;
	CStr ServerInfo;
	CStr ServerCorrectedIP; // If the master server misidentifies your IP address (eg, master and client are behind the same firewall) you can set this to correct it.
	CStr MasterAddress;	//The single master that the client is connecting to.
	int MasterPingsPerSecond;
	int MasterPings;
	int ClientRate;
	int AllowSinglePlayerJoins;
	CStr InsigniaType;
	CStr TankType;
	CStr MapFile;
	CStr AnimationDir;
	int AnimFPS;
	bool PlayMusic;
	float SoundVol, MusicVol;
	int Laps;
	int Deathmatch;
	CStr ServerAddress;
	CStr PlayerName;
	CStr OptAITankTeam;
	int AIAutoFillCount;
	int AIAutoFill;
	bool AutoStart;

	CGameSettings();
};

struct CGameState // This information can change from frame to frame
{
	int HowitzerTime;
	int PauseGame;
	EntityGID CurrentLeader, LastLeader;
	bool LadderMode;	//This is on if we are in ladder play mode.
	int CurDediMap;	//So first map in cycle is 0.
	int ReStartMap;
	int CoolDownCounter;
	int SomeoneWonRace;
	bool DemoMode;	//This is set when we are in automatic demo mode, so we can restart the game and stay in demo mode.
	float ViewAngle;
	int sendstatuscounter;
	int CountdownTimer;
	bool WritingAnim;
	int TakeMapSnapshot;	//When set, causes a one-off power of 2 snapshot of the game to be taken and saved with the current map's name.
	int AnimFrame;
	bool AutoDrive;
	int NumAITanks;
	bool FPSCounter;
	bool Quit;
	int ShowPlayerList;
	bool ToggleMenu;
	bool ActAsServer;
	CStr ConnectedToAddress;
	int PauseScreenOn;
	int KillSelf;
	bool SwitchSoundMode;
	ClassHash TeamHash;
	CStr MasterError;
	int MasterPingIter;
	MasterSortMode SortMode;
	bool SwitchMode;
	CStr sMusicFile;
	float FPS;
	int NetCPSOut, NetCPSIn;	//Networking stats.
	unsigned int LastNetBytesOut, LastNetBytesIn;

	CGameState();
};

struct CInputState
{
	int GunTo;
	int TurretCam;
	int PointCamUD;
	int PointCamLR;
	int PointCamBK;
	int ResetCam;
	int FreeLook;
	CJoyInput input;
	int TurnLR, MoveUD, AltUD, StrafeLR;
	int TreadL, TreadR;
	int SpaceBar;
	float MouseLR, MouseUD;

	CInputState();
};

struct CDediMap
{
	CStr FileName;
	int Index;

	CDediMap() {Index = -1;}
};

class EntityBase;
class EntityTankGod;

class CTankGame
{
public:
	TankPacketProcessor				TankPacket;
	MasterClientPacketProcessor		MasterPacket;

	CGameSettings	GameSettings;
	CGameState		GameState;
	CInputState		InputState;
	SimpleStatus	LoadingStatus;


	EntityBase		*PlayerEnt;
	EntityTankGod	*GodEnt;

	Timer			tmr, tmr2;
	int				frames;
	int				framems, polyms, voxelms, thinkms, blitms;
	int				LastSecs;

	float			ClientPacketRate;

	ConfigFile		cfg;

	int				CurChatLine;
	CStr			ChatLine[MaxChatLines];
	int				ChatLineLen;
	CStr			ChatEdit;
	CStrLink		ChannelHead, UserHead;
	CStr			CurrentChannel;

	ClassHash		AITankTeam;
	int				NumTankNames;
	int				NextTankName;	//Since we are shuffling them at load time now.
	CStr			TankNames[MaxTankNames];

	int				NumAvailTeams;
	TeamInfo		Teams[MaxTeams];
//	CStr			OptTeams[MaxTeams];
	int				MaxAllowedTeams;

	int				NumMusicFiles;
	int				NextMusicFile;
	CStr			MusicFiles[MaxMusicFiles];

	int				NumTankTypes;
	TankInfo		TankTypes[MaxTankTypes];

	int				NumMaps;
	MapInfo			Maps[MaxMaps];
	CDediMap		DediMaps[MaxMaps];	//Dedicated server map list.
	int				NumDediMaps;

	ServerEntryEx	ServerHead;
	MasterServer	Master[MAX_MASTERS];
	Network			MasterNet;	//Networking object for communication with the master server.

	VoxelWorld		VW;

	StatsPackage	Stats;
	LadderManager	Ladder;

	int				HeartbeatTime;

	unsigned int	LastMasterPingTime;
	int				FramesOver100MS;

		//Render debugging buffer.
	int				DbgBufLen;
	char			DbgBuf[1024];

private:
	CTankGame();

	void DoHUD();
	void DoInput();
#ifndef HEADLESS
	void DoSfmlEvents();
#endif
	int Announcement(const char *ann1, const char *ann2, EntityGID tank = 0);
	void DoPlayerAndCam();
	CStr RandTankType();	//Chooses a random tank type, taking into account the type limiting variable.

public:
	static CTankGame& Get();

	CStr MMSS(int secs);
	bool DoFrame();
	int Cleanup();
	int EnumerateTanks();
	int EnumerateTeams();

	bool StartGame();
	bool StopGame();
	int IsGameRunning();
	int IsMapDMOnly(int num);

	void ProcessServerCommand(char* sCommand);
	int ReadConfigCfg(const char *name);
	void NewMusic();
	void FindMapTitle(ServerEntryEx *se);
	int Heartbeat(sockaddr_in *dest);
	void SortMaps();
	void LoadTankNames(CStr sFilename);
	void LoadMaps();
	void LoadDediMaps();
	void LoadMusic();
	void ShuffleMusic();

	VoxelWorld*		GetVW() {return &VW;}
	int				GetNextTankName() {return NextTankName;}
	int				GetNumTankNames() {return NumTankNames;}
	int				GetNumTankTypes() {return NumTankTypes;}
	CStr			GetTankName(const int iName) {return TankNames[iName];}
	int				GetNumMaps() {return NumMaps;}
	MapInfo*		GetMap(const int iMap) {return &Maps[iMap];}
	CStr			GetAIPrefix() {return GameSettings.AIPrefix;}
	int				GetNumTeams() {return MIN(MaxAllowedTeams,MIN(NumAvailTeams, GameSettings.NumTeams));}
	int				GetNumAvailTeams() {return NumAvailTeams;}
	int				GetTeamFillSpot();
	TeamInfo		GetTeam(const int iTeam) {return Teams[iTeam];}
	int				TeamIndexFromHash(const int hash, int *index);
	ServerEntryEx*	GetServerHead() {return &ServerHead;}
	Network*		GetMasterNet() {return &MasterNet;}
	CStrLink*		GetUserHead() {return &UserHead;}
	CStrLink*		GetChannelHead() {return &ChannelHead;}
	CStr*			GetChatLine(const int iChat) {return &ChatLine[iChat];}
	int				GetCurChatLine() {return CurChatLine;}
	CStr*			GetCurChatLineText() {return &ChatLine[CurChatLine];}
	int				GetCurChatLineLen() {return ChatLineLen;}
	CStr*			GetChatEdit() {return &ChatEdit;}
	CStr*			GetCurChannel() {return &CurrentChannel;}
	LadderManager*	GetLadder() {return &Ladder;}
	TankInfo		GetTankType(const int iTank) {return TankTypes[iTank];}
	StatsPackage*	GetStats() {return &Stats;}
	SimpleStatus*	GetLoadingStatus() {return &LoadingStatus;}

	CGameState*		GetGameState() {return &GameState;}
	CGameSettings*	GetSettings() {return &GameSettings;}
	CInputState*	GetInputState() {return &InputState;}

	void			SetNextTankName(const int iName) {NextTankName = iName;}
	void			SetCurrentChannel(const char *sChannel) {CurrentChannel = sChannel;}
	void			SetCurChatLine(const int iChat) {CurChatLine = iChat;}
	void			SetCurChatLineText(const char* sChat) {ChatLine[CurChatLine] = sChat;}
};


#ifndef WIN32
#include <unistd.h>
inline void Sleep(unsigned int iMilliseconds)
{
    usleep(iMilliseconds*1000);
}
#endif

#endif

