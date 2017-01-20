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

#ifdef QT_CORE_LIB
#include <QApplication>
#include <QPixMap>
#include <QSplashScreen>
#endif

#include "TankRacing.h"
#include "EntityTank.h"
#include "EntityGUI.h"
#include "GenericBuffer.h"
#include "Reg.h"
#include <ctime>
#include <sstream>

#include "TankGUI.h"
#include "EntityPFire.h"
#include "TankGame.h"
#include "TMMaster.h"

#include "TxtBlock.hpp"

#include "version.h"

using namespace std;

TreadMarksVersion GameVersion = {VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_REVISION};

GenericBuffer GenBuf;

#ifndef HEADLESS
Dedicated* GetDedicatedWin()
{
	static Dedicated dedicatedWin;
	return &dedicatedWin;
}
#endif

int OverrideRegistry = 0;

//Windows app stuff.
const char szAppName[] = "Tread Marks - Alt-Tab to switch apps, F8 full screen";

#ifdef WIN32
HANDLE tmsem = 0;
const char tmsemname[] = "Tread Marks Semaphore";
#endif

int ParseCommandLine(const char *cmdline, int *argc, char ***argv){
	static char cmd[1024];
	static char *iargv[256];
	if(cmdline && strlen(cmdline) < 1000 && argc && argv){
		cmd[0] = '\0';
		strcpy(cmd, cmdline);
		*argc = 1;
		*argv = &iargv[0];
		iargv[0] = &cmd[0];
		int quote = 0, n = 0;
		while(cmd[n] != 0 && (*argc) < 256){
			if(quote){
				if(cmd[n] == '\"'){
					cmd[n] = 0;
					quote = 0;
				}
			}else{
				if(cmd[n] == '\"'){
					cmd[n] = 0;
					iargv[(*argc) - 1] = &cmd[n + 1];	//Push current argv up past quote.
					quote = 1;
				}else{
					if(cmd[n] == ' ' || cmd[n] == '\t'){
						cmd[n] = 0;
						iargv[*argc] = &cmd[n + 1];
						*argc = (*argc) + 1;
					}
				}
			}
			n++;
		}
		return *argc;
	}
	if(argc) *argc = 0;
	return 0;
}

void CheckGraphics()
{
	if(CTankGame::Get().GetSettings()->DedicatedServer == false && (CTankGame::Get().GetGameState()->SwitchMode || !GenBuf.Valid())){
		//
		CTankGame::Get().GetVW()->UndownloadTextures();	//Just in case we had a window open before and had textures downloaded.
		CTankGame::Get().GetVW()->SetMaxTexRes(CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes);
		//
		if(GenBuf.Valid() && (CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH != GenBuf.Width() || CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT != GenBuf.Height() ||
			!!CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen != GenBuf.Fullscreen())){
			CTankGame::Get().GetVW()->FreeSound();
			Sleep(100);
			GenBuf.Destroy(true);
			return;	//Pass through message loop once before recreating window?
		}
		if(GenBuf.Valid()){
			CTankGame::Get().GetGameState()->SwitchSoundMode = true;
		}
		//
		if(CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen){
			OutputDebugLog("Attempting to create Full-Screen OpenGL window.\n");
		}else{
			OutputDebugLog("Attempting to create OpenGL window.\n");
		}
		OutputDebugLog("Res: " + String(CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH) + " x " + String(CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT) + "\n");
		if(!GenBuf.Valid())
			GenBuf.CreateBuffer(CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH, CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT, CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen, szAppName, "icon32.bmp");

		OutputDebugLog("Creation successful.\n");
		//
		if(TextureCompressionType != 0)
			CTankGame::Get().GetSettings()->GraphicsSettings.TexCompressAvail = true;
		else
			CTankGame::Get().GetSettings()->GraphicsSettings.TexCompressAvail = false;
		//
		CTankGame::Get().GetGameState()->SwitchMode = false;
		GenBuf.Clear();
		GenBuf.Swap();	//Give us a blank screen.
		OutputDebugLog("Downloading textures.\n");
		//
		CTankGame::Get().GetVW()->UsePalettedTextures(CTankGame::Get().GetSettings()->GraphicsSettings.PalettedTextures);
		CTankGame::Get().GetVW()->UseCompressedTextures(CTankGame::Get().GetSettings()->GraphicsSettings.TexCompress);
		CTankGame::Get().GetVW()->Map.UsePalettedTextures(CTankGame::Get().GetSettings()->GraphicsSettings.PalettedTextures);
		//
		//if(MaxOpenGLTextureSize > 0)
		//	CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes = MIN(MaxOpenGLTextureSize, CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes);
		CTankGame::Get().GetVW()->SetMaxTexRes(CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes);
		//
		CTankGame::Get().GetVW()->DownloadTextures();
	}
}

int main(int argc, char** argv)
{
#ifdef QT_CORE_LIB
	QApplication app(argc, argv);
	QPixmap pixmap(":/splash.png");
	QSplashScreen splash(pixmap);
	splash.showMessage("Tread Marks is now loading...", Qt::AlignBottom|Qt::AlignCenter, Qt::white);
	splash.show();
	app.processEvents();
#endif

	//
	CTankGame::Get().GetVW()->Net.SetChallengeKey(TREADMARKS_CHALLENGE_KEY);	//Essentially version control for network layer.
	//
	TMsrandom(time(NULL));

	RegisterRacetankEntities();
	RegisterGUIEntities();
	RegisterPFireEntities();

	FILE *f;	//Hacky search directory setting.
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	if((f = fopen("dirs.cfg", "r")) /*|| (f = fopen("x:/voxeled/dirs.cfg", "r"))*/){
		while(fgets(buf, sizeof(buf) - 1, f)) CTankGame::Get().GetVW()->FM.AddSearchDir(FilePathOnly(buf));
		fclose(f);
	}else{
	//	return FALSE;
	}
	//
	//Add UserData directories to search path up here first.

	// First in the user's application data directory
	CStr sDir = GetAppDataDir();
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/maps/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/objects/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/entities/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/sound/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/art/");

	// Then in the machine's application data directory
	sDir = GetCommonAppDataDir();
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/maps/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/objects/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/entities/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/sound/");
	CTankGame::Get().GetVW()->FM.AddSearchDir(sDir + "addons/art/");

	// Finally in the program file's directory
	CTankGame::Get().GetVW()->FM.AddSearchDir("addons/maps/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("addons/objects/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("addons/entities/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("addons/sound/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("addons/art/");

	// Post-finally, check the official directories
	CTankGame::Get().GetVW()->FM.AddSearchDir("maps/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("objects/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("entities/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("sound/");
	CTankGame::Get().GetVW()->FM.AddSearchDir("art/");

	//
	CStr sScores(GetCommonAppDataDir());
	sScores.cat("scores");
	// TODO: Make a platform-independent function for creating directories
#ifdef WIN32
	CreateDirectory(sScores.get(), NULL);	//Make dir for holding ladder saves and map stats files.
#endif
	//

//	if(strlen(szCmdLine) > 4 && ReadConfigCfg(szCmdLine)){
	int bConfigFound = 0;
	if(argc > 1)
	{
		if(CmpLower(FileExtension(argv[1]), "cfg"))
			bConfigFound = CTankGame::Get().ReadConfigCfg(argv[1]);
		else if(strlen(argv[1]) > 5 && isdigit(argv[1][0]))
		{
/*			int colon = -1;

			bConfigFound = CTankGame::Get().ReadConfigCfg("config.cfg");	//Read default.
			for(int i = 0; i < strlen(argv[1]); i++)
			{
				if(argv[1][i] == ':')
					colon = i;
			}
			if(colon != -1)
			{
				CTankGame::Get().GetSettings()->ServerAddress = Left(argv[1], colon - 1);
				CTankGame::Get().GetSettings()->ServerPort = atoi(Right(argv[1], strlen(argv[1]) - colon - 1));
			}
			else
*/				CTankGame::Get().GetSettings()->ServerAddress = argv[1];
			CTankGame::Get().GetSettings()->AutoStart = true;
		}
	}
	if(!bConfigFound)
	{
		CStr sConfigFile = GetAppDataDir();
		sConfigFile.cat("config.cfg");
		if(!CTankGame::Get().ReadConfigCfg(sConfigFile.get()))	// Read from this user's data directory
		{
			sConfigFile = GetCommonAppDataDir();	// Read from this machine's data directory
			sConfigFile.cat("config.cfg");
			if(!CTankGame::Get().ReadConfigCfg(sConfigFile.get()))
				CTankGame::Get().ReadConfigCfg("config.cfg");	// Read from the program file's directory
		}
	}

	//
	// Russ
	char tempstr[256];
	Language.Load("Local\\Language.TXT");
	Paths.Load("Local\\Paths.TXT");

	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Help.TXT");
	TextBlock.Load(tempstr);
	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Tutorial.TXT");
	TextBlock2.Load(tempstr);

	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Lines.TXT");
	Text.Load(tempstr);
	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Names.TXT");
	Names.Load(tempstr);
	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Weapons.TXT");
	Weapons.Load(tempstr);
	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Insignia.TXT");
	Insignia.Load(tempstr);
	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Sounds.TXT");
	SoundPaths.Load(tempstr);
	sprintf(tempstr, "%s\\%s", Paths.Get(0), "Controls.TXT");
	ControlText.Load(tempstr);

	CTankGame::Get().GetSettings()->InputSettings.InitControls();

	if(!OverrideRegistry && !CTankGame::Get().GetSettings()->DedicatedServer){
		RegistryLoad();
	}
	//
	if(argc > 2 && CmpLower(FileExtension(argv[2]), "cfg")){
		//If second command line parameter is a config file too, read it AFTER registry.
		CTankGame::Get().ReadConfigCfg(argv[2]);
	}
	//
	if(!CTankGame::Get().GetSettings()->DedicatedServer){	//IF NOT dedicated, check for multiple instances.
	    #ifdef WIN32
		//Prevent multiple instances.
		tmsem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, tmsemname);
		if(tmsem){	//Oops, already running.
			MessageBox(NULL, "Tread Marks is already running!", "Error!", MB_OK);
			CloseHandle(tmsem);
			return 0;
		}
		tmsem = CreateSemaphore(NULL, 0, 1, tmsemname);
		#endif
	}
	//

	CTankGame::Get().GetVW()->DisableResourceLoading(CTankGame::Get().GetSettings()->DedicatedServer);	//If we are dedicated, do not load images or sounds (meshes must be loaded, though).
	//

	if(CTankGame::Get().GetSettings()->LogFileActive){
		EnableLogFile(true, CTankGame::Get().GetSettings()->LogFileName);
	}

	for(int cm = 0; cm < argc; cm++){
		OutputDebugLog("Command line param " + String(cm) + ": \"" + argv[cm] + "\"\n");
	}

	if(CTankGame::Get().GetSettings()->InputSettings.UseJoystick && !CTankGame::Get().GetSettings()->DedicatedServer)
	{
		trDeviceID	tmp;
		tmp.sDevName = CTankGame::Get().GetSettings()->InputSettings.sStickName;
		CTankGame::Get().GetSettings()->InputSettings.StickID = CTankGame::Get().GetInputState()->input.GetDevIDFromInfo(&tmp);

		if(!CTankGame::Get().GetInputState()->input.InitController(CTankGame::Get().GetSettings()->InputSettings.StickID))
			CTankGame::Get().GetSettings()->InputSettings.UseJoystick = 0;
		else
		{
			trDeviceID tmp;
			CTankGame::Get().GetInputState()->input.GetDeviceInfo(&tmp);

			CTankGame::Get().GetSettings()->InputSettings.StickID = tmp.iDevID;
			CTankGame::Get().GetSettings()->InputSettings.sStickName = tmp.sDevName;
			RegistrySave();
		}
	}
	//Load loading status font objects.
	if(CTankGame::Get().GetSettings()->DedicatedServer == false){
		CTankGame::Get().GetLoadingStatus()->Init(&CTankGame::Get().GetVW()->FM, 0.03f, 0.06f);
		//
		//Also initialize client-to-master network object.
		if(CTankGame::Get().GetMasterNet()->Initialize()){
			OutputDebugLog("Client to Master Network Object Initialized.\n");
		}else{
			OutputDebugLog("Client to Master Network Object Initialization Failure!!\n\n");
		}
		if(CTankGame::Get().GetMasterNet()->InitClient(CTankGame::Get().GetSettings()->ClientPort + 1, 20)){
			OutputDebugLog("Client to Master Network Object Bound.\n");
		}else{
			OutputDebugLog("Client to Master Network Object Bind Failure!!\n\n");
		}
		CTankGame::Get().GetMasterNet()->SetChallengeKey(TMMASTER_CHALLENGE_KEY);
		CTankGame::Get().GetMasterNet()->Configure(600000, 2000);	//10-minute connectiontimeout, 2 second reliable packet resend.
	}
	//
	//Read AI tank name list.
	CTankGame::Get().LoadTankNames("tanknames.txt");
	//
	CTankGame::Get().GetVW()->LoadEntityClasses();
	//
	//Enumerate team flag types.
	//
	CTankGame::Get().EnumerateTeams();
	//
	//Enumerate racetank types.
	CTankGame::Get().EnumerateTanks();
	//
	//Set default client type for voxel world.
	CTankGame::Get().GetVW()->DefClientEntType = CTankGame::Get().GetTankType(0).hash;
	//
	//Enumerate maps, read names and store filenames.
	CTankGame::Get().LoadMaps();

	//
	//Sort map names numerically.

	CTankGame::Get().SortMaps();

	//Reconcile dedicated map list with enumerated maps.
	//Only maps that have been enumerated will be allowed in the DediMaps array.
	CTankGame::Get().LoadDediMaps();

	//Enumerate music files and store filenames.
	CTankGame::Get().LoadMusic();
	//Shuffle music files in array.
	CTankGame::Get().ShuffleMusic();
	//
	CTankGame::Get().GetVW()->InitializeEntities();
	//
	CTankGame::Get().GetVW()->BeginningOfTime();

	if(CTankGame::Get().GetSettings()->DedicatedServer){

#ifdef QT_CORE_LIB
		GetDedicatedWin(); // Just to make sure it exists.
		RedirectDebugLog(std::bind(&Dedicated::OutputFunc, GetDedicatedWin(), std::placeholders::_1, std::placeholders::_2));
#endif
		
		//
		CTankGame::Get().StartGame();
	}

#ifdef QT_CORE_LIB
	splash.hide();
	if(CTankGame::Get().GetSettings()->DedicatedServer) GetDedicatedWin()->show();
#endif

	while(!CTankGame::Get().GetGameState()->Quit)
	{
		CheckGraphics();
		CTankGame::Get().DoFrame();

#ifdef QT_CORE_LIB
		app.processEvents();
#endif
	}
	GenBuf.Destroy();
	CTankGame::Get().MasterNet.ClientDisconnect();
	CTankGame::Get().GetVW()->Net.ClientDisconnect(CTankGame::Get().GetVW());	//Make sure connections are empty when re-starting a game.
	CTankGame::Get().GetVW()->Net.ServerDisconnect(CLIENTID_BROADCAST, CTankGame::Get().GetVW());
	CTankGame::Get().GetVW()->Net.FreeServer();
	CTankGame::Get().GetVW()->Net.FreeClient();
	return 0;
}

bool RegistrySave(){
	if(CTankGame::Get().GetSettings()->DedicatedServer) return false;
	Registry::Get().WriteDword("Trilinear", CTankGame::Get().GetSettings()->GraphicsSettings.Trilinear);
	Registry::Get().WriteDword("GLBWIDTH", CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH);
	Registry::Get().WriteDword("GLBHEIGHT", CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT);
	Registry::Get().WriteDword("UseFullScreen", CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen);
	Registry::Get().WriteDword("UseFog", CTankGame::Get().GetSettings()->GraphicsSettings.UseFog);
	Registry::Get().WriteDword("TreadMarks", CTankGame::Get().GetSettings()->GraphicsSettings.TreadMarks);
	Registry::Get().WriteFloat("Particles", CTankGame::Get().GetSettings()->GraphicsSettings.Particles);
	Registry::Get().WriteFloat("ViewDistance", CTankGame::Get().GetSettings()->GraphicsSettings.ViewDistance);
	Registry::Get().WriteDword("UseJoystick", CTankGame::Get().GetSettings()->InputSettings.UseJoystick);
	Registry::Get().WriteString("StickName", CTankGame::Get().GetSettings()->InputSettings.sStickName);
	Registry::Get().WriteFloat("DeadZone", CTankGame::Get().GetSettings()->InputSettings.DeadZone);
	Registry::Get().WriteDword("PolyLod", CTankGame::Get().GetSettings()->GraphicsSettings.PolyLod);
	Registry::Get().WriteDword("MaxTexRes", CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes);
	Registry::Get().WriteFloat("Quality", CTankGame::Get().GetSettings()->GraphicsSettings.Quality);
	Registry::Get().WriteDword("DetailTerrain", CTankGame::Get().GetSettings()->GraphicsSettings.DetailTerrain);
	Registry::Get().WriteFloat("MusicVol", CTankGame::Get().GetSettings()->MusicVol);
	Registry::Get().WriteFloat("SoundVol", CTankGame::Get().GetSettings()->SoundVol);
	Registry::Get().WriteDword("PlayMusic", CTankGame::Get().GetSettings()->PlayMusic);
	Registry::Get().WriteDword("MenuFire", CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire);
	Registry::Get().WriteDword("HiFiSound", CTankGame::Get().GetSettings()->HiFiSound);
	Registry::Get().WriteFloat("MouseSpeed", CTankGame::Get().GetSettings()->InputSettings.MouseSpeed);
	Registry::Get().WriteDword("InvMouseY", CTankGame::Get().GetSettings()->InputSettings.InvMouseY);
	Registry::Get().WriteDword("MouseEnabled", CTankGame::Get().GetSettings()->InputSettings.MouseEnabled);
	//
	Registry::Get().WriteString("MapFile", CTankGame::Get().GetSettings()->MapFile);
	Registry::Get().WriteString("TankType", CTankGame::Get().GetSettings()->TankType);
	Registry::Get().WriteString("InsigniaType", CTankGame::Get().GetSettings()->InsigniaType);
	Registry::Get().WriteDword("NumTanks", CTankGame::Get().GetSettings()->AIAutoFillCount);
	Registry::Get().WriteDword("AIAutoFill", CTankGame::Get().GetSettings()->AIAutoFill);
	Registry::Get().WriteDword("Laps", CTankGame::Get().GetSettings()->Laps);
	Registry::Get().WriteDword("CountdownTime", CTankGame::Get().GetSettings()->StartDelay);
	Registry::Get().WriteDword("ActAsServer", CTankGame::Get().GetGameState()->ActAsServer);
	Registry::Get().WriteString("PlayerName", CTankGame::Get().GetSettings()->PlayerName);
	Registry::Get().WriteDword("Deathmatch", CTankGame::Get().GetSettings()->Deathmatch);
	Registry::Get().WriteString("ServerAddress", CTankGame::Get().GetSettings()->ServerAddress);
	Registry::Get().WriteDword("ClientRate", CTankGame::Get().GetSettings()->ClientRate);
	Registry::Get().WriteDword("CamStyle", CTankGame::Get().GetSettings()->CamStyle);
	Registry::Get().WriteString("MasterAddressUrl", CTankGame::Get().GetSettings()->MasterAddress);
	Registry::Get().WriteFloat("EnemySkill", CTankGame::Get().GetSettings()->EnemySkill);

	Registry::Get().WriteDword("NumTeams", CTankGame::Get().GetSettings()->NumTeams);
	Registry::Get().WriteDword("TeamScores", CTankGame::Get().GetSettings()->TeamScores);
	Registry::Get().WriteDword("TeamDamage", CTankGame::Get().GetSettings()->TeamDamage);

	//
	int i;
	for(i = 0; i < NumControls; i++){
		Registry::Get().WriteString("CTL_" + CTankGame::Get().GetSettings()->InputSettings.Controls[i].name + "1", CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl1);
		Registry::Get().WriteString("CTL_" + CTankGame::Get().GetSettings()->InputSettings.Controls[i].name + "2", CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl2);
		Registry::Get().WriteString("CTL_" + CTankGame::Get().GetSettings()->InputSettings.Controls[i].name + "3", CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl3);
	}
	for(i = 0; i < 8; i++)
	{
	    std::stringstream ss;
	    ss << i;

		Registry::Get().WriteDword("Axis" + CStr(ss.str().c_str()), CTankGame::Get().GetSettings()->InputSettings.AxisMapping[i]);
	}
	return true;
}
bool RegistryLoad(){
	if(CTankGame::Get().GetSettings()->DedicatedServer) return false;
	Registry::Get().ReadDword("Trilinear", &CTankGame::Get().GetSettings()->GraphicsSettings.Trilinear);
	Registry::Get().ReadDword("GLBWIDTH", &CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH);
	Registry::Get().ReadDword("GLBHEIGHT", &CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT);
	Registry::Get().ReadDword("UseFullScreen", &CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen);
	Registry::Get().ReadDword("UseFog", &CTankGame::Get().GetSettings()->GraphicsSettings.UseFog);
	Registry::Get().ReadDword("TreadMarks", &CTankGame::Get().GetSettings()->GraphicsSettings.TreadMarks);
	Registry::Get().ReadFloat("Particles", &CTankGame::Get().GetSettings()->GraphicsSettings.Particles);
	Registry::Get().ReadFloat("ViewDistance", &CTankGame::Get().GetSettings()->GraphicsSettings.ViewDistance);
	Registry::Get().ReadDword("UseJoystick", &CTankGame::Get().GetSettings()->InputSettings.UseJoystick);
	Registry::Get().ReadString("StickName", &CTankGame::Get().GetSettings()->InputSettings.sStickName);
	Registry::Get().ReadFloat("DeadZone", &CTankGame::Get().GetSettings()->InputSettings.DeadZone);
	Registry::Get().ReadDword("PolyLod", &CTankGame::Get().GetSettings()->GraphicsSettings.PolyLod);
	Registry::Get().ReadDword("MaxTexRes", &CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes);
	Registry::Get().ReadFloat("Quality", &CTankGame::Get().GetSettings()->GraphicsSettings.Quality);
	Registry::Get().ReadDword("DetailTerrain", &CTankGame::Get().GetSettings()->GraphicsSettings.DetailTerrain);
	Registry::Get().ReadFloat("MusicVol", &CTankGame::Get().GetSettings()->MusicVol);
	Registry::Get().ReadFloat("SoundVol", &CTankGame::Get().GetSettings()->SoundVol);
	Registry::Get().ReadDword("PlayMusic", &CTankGame::Get().GetSettings()->PlayMusic);
	Registry::Get().ReadDword("MenuFire", &CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire);
	Registry::Get().ReadDword("HiFiSound", &CTankGame::Get().GetSettings()->HiFiSound);
	Registry::Get().ReadFloat("MouseSpeed", &CTankGame::Get().GetSettings()->InputSettings.MouseSpeed);
	Registry::Get().ReadDword("InvMouseY", &CTankGame::Get().GetSettings()->InputSettings.InvMouseY);
	Registry::Get().ReadDword("MouseEnabled", &CTankGame::Get().GetSettings()->InputSettings.MouseEnabled);
	//
	Registry::Get().ReadString("MapFile", &CTankGame::Get().GetSettings()->MapFile);
	Registry::Get().ReadString("TankType", &CTankGame::Get().GetSettings()->TankType);
	Registry::Get().ReadString("InsigniaType", &CTankGame::Get().GetSettings()->InsigniaType);
	Registry::Get().ReadDword("NumTanks", &CTankGame::Get().GetSettings()->AIAutoFillCount);
	Registry::Get().ReadDword("AIAutoFill", &CTankGame::Get().GetSettings()->AIAutoFill);
	Registry::Get().ReadDword("Laps", &CTankGame::Get().GetSettings()->Laps);
	Registry::Get().ReadDword("CountdownTime", &CTankGame::Get().GetSettings()->StartDelay);
	Registry::Get().ReadDword("ActAsServer", &CTankGame::Get().GetGameState()->ActAsServer);
	Registry::Get().ReadString("PlayerName", &CTankGame::Get().GetSettings()->PlayerName);
	if(_strnicmp(CTankGame::Get().GetAIPrefix(), CTankGame::Get().GetSettings()->PlayerName, strlen(CTankGame::Get().GetAIPrefix())) == 0)
		CTankGame::Get().GetSettings()->PlayerName = "Player";
	Registry::Get().ReadDword("Deathmatch", &CTankGame::Get().GetSettings()->Deathmatch);
	Registry::Get().ReadString("ServerAddress", &CTankGame::Get().GetSettings()->ServerAddress);
	Registry::Get().ReadDword("ClientRate", &CTankGame::Get().GetSettings()->ClientRate);
	Registry::Get().ReadDword("CamStyle", &CTankGame::Get().GetSettings()->CamStyle);
	Registry::Get().ReadString("MasterAddressUrl", &CTankGame::Get().GetSettings()->MasterAddress);
	Registry::Get().ReadFloat("EnemySkill", &CTankGame::Get().GetSettings()->EnemySkill);
	//
	Registry::Get().ReadDword("NumTeams", &CTankGame::Get().GetSettings()->NumTeams);
	Registry::Get().ReadDword("TeamScores", &CTankGame::Get().GetSettings()->TeamScores);
	Registry::Get().ReadDword("TeamDamage", &CTankGame::Get().GetSettings()->TeamDamage);

	int i;
	for(i = 0; i < NumControls; i++){
		Registry::Get().ReadString("CTL_" + CTankGame::Get().GetSettings()->InputSettings.Controls[i].name + "1", &CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl1);
		Registry::Get().ReadString("CTL_" + CTankGame::Get().GetSettings()->InputSettings.Controls[i].name + "2", &CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl2);
		Registry::Get().ReadString("CTL_" + CTankGame::Get().GetSettings()->InputSettings.Controls[i].name + "3", &CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl3);
	}
	for(i = 0; i < 8; i++)
	{
	    std::stringstream ss;
	    ss << i;
		Registry::Get().ReadDword("Axis" + CStr(ss.str().c_str()), &CTankGame::Get().GetSettings()->InputSettings.AxisMapping[i]);
	}
	return true;
}
