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
// Code to create and manage menu system in Tread Marks.
//

#ifndef TANKGUI_H
#define TANKGUI_H

class VoxelWorld;

extern CStr LastControl;

enum MenuPage{
	MP_Initialize,
	MP_None,
	MP_Main,
	MP_Options,
	MP_Race,
	MP_StartingGame,
	MP_TankSelect,
	MP_MapSelect,
	MP_ServerSelect,
	MP_Chat,
	MP_WrongVersion,
	MP_Help,
	MP_TankCtlCfg,
	MP_CamCtlCfg,
	MP_InGameMain,
	MP_LadderRoster,
	MP_Ladder,
	MP_LadderNew,
	MP_LadderDelete,
	MP_Stats,
	MP_TextDisplay,
	MP_Intro1,
	MP_Intro2,
	MP_Intro3,
	MP_Intro4,
	MP_Intro5,
	MP_Intro6,
	MP_Intro7,
	MP_Intro8,
	MP_Trophy,
	MP_Outro1,
	MP_Outro2,
	MP_Outro3,
	MP_Outro4,
	MP_Tutorial,
	MP_GraphicsOptions,
	MP_SoundMusicOptions,
	MP_InputOptions,
	MP_MiscOptions,
	MP_JoystickSettings,
	MP_MouseSettings,
	MP_Language,
	MP_ImageHelp,
};

enum ButtonID{
	BID_TankCtlCfg = 10000,
	BID_CamCtlCfg = 20000,
	BID_None = 0,
	BID_FirstTimeOnMenu = 1,
	BID_Help,
	BID_Credits,
	BID_ChanList,
	BID_UserList,
	BID_Channels,
	BID_Users,
	BID_RefreshLAN,
	BID_RefreshMaster,
	BID_RefreshPings,
	BID_StopPings,
	BID_SortName,
	BID_SortPing,
	BID_SortPlayers,
	BID_SortMap,
	BID_SortTime,
	BID_SortMode,
	BID_Race,
	BID_Master,
	BID_ChatEdit,
	BID_Fire,
	BID_Chat,
	BID_Options,
	BID_Back,
	BID_Web,
	BID_Quit,
	BID_Mode,
	BID_Res,
	BID_Fog,
	BID_Treadmarks,
	BID_Dust,
	BID_ViewDist,
	BID_MeshRes,
	BID_TexRes,
	BID_Joy,
	BID_MouseY,
	BID_MouseEnable,
	BID_Quality,
	BID_Detail,
	BID_Camera,
	BID_Sound,
	BID_Rate,
	BID_CamStyle,
	BID_Trilinear,
	BID_MouseLook,
	BID_TexCompress,
	BID_SoundQuality,
	BID_SoundVol,
	BID_MusicVol,
	BID_ReadCfg,
	BID_Map,
	BID_Tank,
	BID_Insignia,
	BID_Team,
	BID_Opponents,
	BID_Laps,
	BID_LapsToggle,
	BID_Delay,
	BID_DelayToggle,
	BID_Name,
	BID_NameToggle,
	BID_GameType,
	BID_Mirror,
	BID_NetJoins,
	BID_Server,
	BID_Address,
	BID_AITanks,
	BID_AITanksToggle,
	BID_Music,
	BID_Paletted,
	BID_Tourney,
	BID_Tutorial,
	BID_Demo,
	BID_Multi,
	BID_Continue,
	BID_Roster,
	BID_New,
	BID_Delete,
	BID_Yes,
	BID_No,
	BID_Skill,
	BID_Stats,
	BID_LimitTypes,
	BID_TimeLimit,
	BID_TimeLimitToggle,
	BID_FragLimit,
	BID_FragLimitToggle,
	BID_NumTeams,
	BID_TeamScores,
	BID_TeamDamage,
	BID_MouseSpeed,
	BID_Ordering,
	BID_TutSelect,
	BID_GraphicsOptions,
	BID_SoundMusicOptions,
	BID_InputOptions,
	BID_MiscOptions,
	BID_JoystickSettings,
	BID_MouseSettings,
	BID_StickSelect,
	BID_StickXAxis,
	BID_StickYAxis,
	BID_StickZAxis,
	BID_StickRAxis,
	BID_StickUAxis,
	BID_StickVAxis,
	BID_StickHatX,
	BID_StickHatY,
	BID_DeadZone,
	BID_Language,
	BID_English,
	BID_French,
	BID_German,
	BID_Spanish,
	BID_Italian,
	BID_Training,
};

extern bool bTrainingOnly;

extern MenuPage CurrentMenu, ReturnToMenu;

void DoMenus();//VoxelWorld *VW);

void CacheMenus();	//Caches menu related entities.

void JumpToMenu(MenuPage menu);

#endif

