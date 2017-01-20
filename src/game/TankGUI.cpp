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

//Disable double to float truncation warnings.
#pragma warning( disable : 4305 )
//Disable exceptions not enabled warnings.
#pragma warning( disable : 4530 )

#include "Web.h"
#include "EntityTank.h"
#include "TankGUI.h"
#include "TankRacing.h"
#include "EntityGUI.h"
#include "VoxelWorld.h"
#include "EntityPFire.h"
#include "BitPacking.h"
#include "TankGame.h"
#include "HelpText.h"

#include "TextLine.hpp"
#include "TxtBlock.hpp"

#include "version.h"

#include <GL/glew.h>

#include <set>

#define PFIRE_Z 0.9f
//Put pfire just above background image.

CStr LastControl;

MenuPage CurrentMenu = MP_Initialize;
MenuPage ReturnToMenu = MP_Main;
MenuPage LastMenu = MP_None;
MenuPage ReturnFromText = MP_Main;

bool bTrainingOnly = false;

int FirstTimeOnMenu = 0;

struct GUIElementSlot{
	EntityGID gid;
	int button;
};

#define MAX_SLOTS 128

GUIElementSlot GUISlot[MAX_SLOTS];
int NextSlot;

void GuiEntStart(){
	NextSlot = 0;
}
EntityBase *GuiEnt(const char *c, const char *t, Vec3 pos, Vec3 rot, Vec3 vel, const char *txt, int but,
				   bool firsttimetextonly = false, int id = 0, int flags = 0){
	EntityBase *e = CTankGame::Get().GetVW()->GetEntity(GUISlot[NextSlot].gid);
	if(!e){
		e = CTankGame::Get().GetVW()->AddEntity(c, t, pos, rot, vel, id, flags, 0, 0);
		if(e && txt) e->SetString(ATT_TEXT_STRING, txt);
	}
	if(e){
		GUISlot[NextSlot].gid = e->GID;
		GUISlot[NextSlot].button = but;
		if(txt && !firsttimetextonly) e->SetString(ATT_TEXT_STRING, txt);
		e->Flags = flags;
	}
	NextSlot = std::min(NextSlot + 1, MAX_SLOTS - 1);
	return e;
}
void FreeGui(){
	for(int i = 0; i < MAX_SLOTS; i++){
		EntityBase *e = CTankGame::Get().GetVW()->GetEntity(GUISlot[i].gid);
		if(e){
			if(!e->SetInt(ATT_CMD_NICEREMOVE, 1)) e->Remove();	//If doesn't support niceremove, just kill.
		}
		GUISlot[i].gid = 0;
	}
}
int Activated(EntityBase **e = NULL){
	if(FirstTimeOnMenu){
		if(e) *e = NULL;
		FirstTimeOnMenu = 0;
		return BID_FirstTimeOnMenu;
	}
	EntityGID a = CTankGame::Get().GetVW()->GetActivated();
	CTankGame::Get().GetVW()->SetActivated(NULL);
	if(a){
		for(int i = 0; i < MAX_SLOTS; i++){
			if(GUISlot[i].gid == a){
				if(e) *e = CTankGame::Get().GetVW()->GetEntity(a);
				return GUISlot[i].button;
			}
		}
	}
	return BID_None;
}
//
bool VideoSwitchNeeded = false, SoundSwitchNeeded = false;
bool RegistrySaveNeeded = false, MusicSwitchNeeded = false;
bool ClientInfoSendNeeded = false;
int StartingGameDelay = 0;
bool MasterRefreshWanted = false;
bool AreLanServers = true;
bool FirstChat = false;
CStr MapImageFailureFile;
CStr MsgTo;
//
int EditingControl = 0;
//
int LMenuFire = 0;
//
void CacheMenus(){
	CTankGame::Get().GetVW()->CacheResources("guibackground", "optionsbackground");
	CTankGame::Get().GetVW()->CacheResources("guibackground", "logobackground");
	CTankGame::Get().GetVW()->CacheResources("guibackground", "ladderbackground");
	CTankGame::Get().GetVW()->CacheResources("guibutton", "button1");
	CTankGame::Get().GetVW()->CacheResources("guibutton", "title1");
	CTankGame::Get().GetVW()->CacheResources("guimouse", "mouse1");
	CTankGame::Get().GetVW()->CacheResources("guilistbox", "listbox1");
	CTankGame::Get().GetVW()->CacheResources("guiimagebrowser", "ibinsignia");
	CTankGame::Get().GetVW()->CacheResources("guiimagebrowser", "ibmap");
	//
	if(!CTankGame::Get().GetSettings()->BypassIntro){
		CTankGame::Get().GetVW()->CacheResources("guibackground", "introback1");
		CTankGame::Get().GetVW()->CacheResources("guibackground", "introback2");
		CTankGame::Get().GetVW()->CacheResources("guibackground", "introback3");
		CTankGame::Get().GetVW()->CacheResources("guibackground", "introback4");
		CTankGame::Get().GetVW()->CacheResources("guibackground", "introback5");
		CTankGame::Get().GetVW()->CacheResources("guibackground", "introback6");
		CTankGame::Get().GetVW()->CacheResources("guibackground", "introback7");
		CTankGame::Get().GetVW()->CacheResources("mesh", "tmlogo");
	}
}
//
//Ladder roster stuff.
struct RosterEntry : public LinklistBase<RosterEntry> {
	CStr name, file;
	int rank, racesstarted, racesfinished, racesaschamp, frags, deaths;
public:
	RosterEntry() : rank(0), racesstarted(0), racesfinished(0), racesaschamp(0), frags(0), deaths(0) { };
	RosterEntry(LadderManager *l){
		if(l){
			name = l->PlayerName;
			file = l->LastFileName;
			rank = l->PlayerRank;
			racesstarted = l->RacesStarted;
			racesfinished = l->RacesRun;
			racesaschamp = l->RacesAsChamp;
			frags = l->Frags;
			deaths = l->Deaths;
		}
	};
};

RosterEntry RosterHead;
int RosterSelection = -1;
int RankSelection = 4;

bool ReadRoster(bool force = false){
	static bool RosterRead = false;
	if(RosterRead == false || force){	//Read in roster.
		RosterHead.DeleteList();
		CTankGame::Get().GetVW()->FM.PushFile();
		CStr sLadderWildCard(GetCommonAppDataDir());
		sLadderWildCard.cat("scores/*.ladder");
		for(FILE *F = CTankGame::Get().GetVW()->FM.OpenWildcard(sLadderWildCard.get(), NULL, 0, false, true); (F); F = CTankGame::Get().GetVW()->FM.NextWildcard()){
			if(CTankGame::Get().GetLadder()->Load(CTankGame::Get().GetVW()->FM.GetFileName())){	//File is valid.
				RosterHead.AddObject(new
					RosterEntry(CTankGame::Get().GetLadder()));//Ladder.PlayerName, CTankGame::Get().GetVW()->FM.GetFileName(), Ladder.PlayerRank, Ladder.RacesRun));
			}
		}
		CTankGame::Get().GetVW()->FM.PopFile();
		RosterRead = true;
		return true;
	}
	return false;
}

void JumpToMenu(MenuPage menu){
	FreeGui();
	CurrentMenu = menu;
}
bool LoadStats(){
	CStr sScores(GetCommonAppDataDir());
	sScores.cat("scores/");
	return CTankGame::Get().GetStats()->Load(sScores + FileNoExtension(FileNameOnly(CTankGame::Get().GetSettings()->MapFile)) + CStr( CTankGame::Get().GetSettings()->Deathmatch ? ".dstats" : ".rstats"));
}
//
CStr BreakLine(const char *s, int line, int wordwrap = 0){
	if(s && line >= 0){
		int n1 = 0, n2 = 0, nc = 0, lastspace = 0, lastlinepos = 0, linepos = 0;
		int endoftext = 0;
		char c = 0;
		do{
			c = s[n1];
			if (c == '\r') { // Russ - big bodge, but it works...
				nc++;
				lastlinepos = linepos;
				endoftext = n1;
				linepos = n1 + 2;
				n1++;
			} else if (c == '\n' || c == '\0') {
				nc++;
				lastlinepos = linepos;
				endoftext = n1;
				linepos = n1 + 1;
			} else if (c == ' ') {
				lastspace = n1;
			}
			if(n1 - linepos >= wordwrap && wordwrap > 0){
				lastlinepos = linepos;
				linepos = lastspace + 1;
				endoftext = lastspace;
				nc++;
			}
			if(nc == line + 1){
				return Left(&s[lastlinepos], (endoftext - lastlinepos));
			}
			n1++;
		}while(c != 0);
	}
	return "";
}
//
CStr DisplayText = "";
CStr HelpImageEntity = "";
int DisplayTextPage = 0;
//
bool StartHelp(HelpID hid)
{

// if this is the japanese translation, replace help menu text with images
#if defined(_JAPAN)

	HelpImageEntity = "";

	if(hid == HELP_Main) HelpImageEntity = "jphelpmainmenu";
	else if(hid == HELP_Chat) HelpImageEntity = "jphelpchat";
	else if(hid == HELP_Tank) HelpImageEntity = "jphelptankselect";
	else if(hid == HELP_Map) HelpImageEntity = "jphelpmapselect";
	else if(hid == HELP_Server) HelpImageEntity = "jphelpserverselect";
	else if(hid == HELP_Options) HelpImageEntity = "jphelpoptions";
	else if(hid == HELP_Misc) HelpImageEntity = "jphelpmiscoptions";
	else if(hid == HELP_Roster) HelpImageEntity = "jphelpladderroster";
	else if(hid == HELP_Ladder) HelpImageEntity = "jphelpladderprogress";
	else if(hid == HELP_Stats) HelpImageEntity = "jphelpstatistics";
	else if(hid == HELP_InGameMain) HelpImageEntity = "jphelpingamemain";

	if(HelpImageEntity != "")
	{
		FreeGui();
		ReturnFromText = CurrentMenu;
		CurrentMenu = MP_ImageHelp;

		return true;
	}
#endif


	if(hid >= 0 && hid < NumHelpText){
		FreeGui();
		ReturnFromText = CurrentMenu;
		CurrentMenu = MP_TextDisplay;

		if(hid == HELP_Story)
		{
			DisplayText =

"The Story\n\
   The Wonderful and Compelling Story So Far:\n\
   \n\
   Once upon a time, a bunch of artificially intelligent\n\
   self-aware battle tanks decided that they had seen\n\
   enough silly human conflict for one silicon sentient\n\
   lifetime, and that they would all run off into the woods\n\
   and have silly robot conflicts for a change.\n\
   \n\
   Somewhere along the line, one of the tanks suggested\n\
   they should try racing around off-road race courses with\n\
   large amounts of live ammunition strewn about.\n\
   \n\
   And there was much rejoicing.\n\
   \n\
   (Hoody Hoo!)\n\
   \n\
   And they lived happily ever after.\n\
   \n\
   The End.\n\
\r\n\
The Credits \n\
   Tread Marks was developed by Longbow Digital Arts, Inc.\n\
   Copyright 1999-2017, All Rights Reserved.\n\
   http://www.LongbowGames.com/\n\
   \n\
   Seumas McNally - Lead Design, Original Programming\n\
   \n\
   Philippe McNally - Modeling, Texturing, Sound, Design\n\
   \n\
   Jim McNally - Map & Track Design, Modeling, Sound\n\
   \n\
   Wendy McNally - Sculptural Design & Modeling\n\
   \n\
   Al Ryan - Music\n\
   \n\
   Tom Hubina - Additional Programming\n\
   Rob McConnell - Additional Programming\n\
   Rick Yorgason - Additional Programming";
		}
		else
		{
			DisplayText = TextBlock.Get(hid);
		}

		DisplayTextPage = 0;
		return true;
	}
	return false;
}
//
void GUIHeader(const char *title, const char *back, bool help){
	if(title && back){
		GuiEntStart();
		if(CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire) GuiEnt("guibackground", "pfirebackground", CVec3(0, 0, PFIRE_Z), NULL, NULL, NULL, BID_None);
		GuiEnt("guimouse", "mouse1", NULL, NULL, NULL, NULL, BID_None);
		GuiEnt("guibackground", back, CVec3(0, 0, 1), NULL, NULL, NULL, BID_None);
		GuiEnt("guibutton", "title1", CVec3(0.5, .02, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			title, BID_None, false, 0, FLAG_TEXT_CENTERX);
		// TODO: localise 'title'
		if(help){
			GuiEnt("guibutton", "button1", CVec3(.01, .01, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
				Text_GetLine(TEXT_HELP), BID_Help, false, 0, 0);
		}
	}
}
//

int FindMapIndex(CStr MapTitle)
{
	for (int i = 0; i < CTankGame::Get().GetNumMaps(); i++)
	{
		if (MapTitle == CTankGame::Get().GetMap(i)->title)
			return i;
	}
	return 0;
}

int RefreshMaster = 0;

int IntroTimer = 0;
int IntroDelayTime = 8000;
int IntroDelayTime2 = 2000;
int OutroTimer = 0;
int OutroDelayTime = 120000;
int OutroDelayTime2 = 2000;
float IntroLogoFader = 0.0f;
float IntroLogoFaderMax = 2.0f;
bool SkipIntro = false;

void DoStartingGame()
{
	FreeGui();
	StartingGameDelay -= CTankGame::Get().GetVW()->FrameTime();
	if(StartingGameDelay <= 0){
		//
		//Set up stats.
		CTankGame::Get().GetStats()->Init();
		if(CTankGame::Get().GetGameState()->ActAsServer){
		//	Stats.Load("scores/" + FileNoExtension(FileNameOnly(MapFile)) + CStr(Deathmatch ? ".dstats" : ".rstats"));
			//Load in stats file, or set LastFileName if file doesn't exist.
			LoadStats();
		}
		//
		if(CTankGame::Get().StartGame()){	//Game started successfully.
			CurrentMenu = MP_None;
		}else{	//Map load must have failed.
			CurrentMenu = MP_Main;
		}
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Trophy Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoTrophy()
{
	char tempstr[256];
	EntityBase *e;

	switch(Activated()){
	case BID_FirstTimeOnMenu :
		IntroLogoFader = 0.0f;
		IntroTimer = 0;
		CTankGame::Get().GetVW()->AddEntity("sound", "champ", NULL, NULL, NULL, 0, FLAG_SOUND_PLAYANNOUNCER, 0, 0);
		break;
	}
	GuiEntStart();
	sprintf(tempstr, Text.Get(TEXT_TROPHYTOTAL), CTankGame::Get().GetLadder()->RacesAsChamp);
	GuiEnt("guibutton", "button1", CVec3(.5, .92, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		tempstr, BID_None, false, 0, FLAG_TEXT_CENTERX);
	//
	e = GuiEnt("mesh", "trophy", CVec3(0.0f, 0.0f, 10.0f), CVec3(0, 0, 0), CVec3(0, 0, 0), 0, BID_None);
	if(e){
		e->SetRot(CVec3(0, sin(fmod(((double)CTankGame::Get().GetVW()->Time() * 0.001) * 1.0, (double)PI * 2.0)) * (PI * 0.2f), 0));
		e->SetPos(CVec3(0, (1.0f - sin((IntroLogoFader / MAX(0.01f, IntroLogoFaderMax)) * PI * 0.5f)) * 10.0f, 12.0f));
	}
	//
	if(CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer){//IntroTimer > IntroDelayTime){
		IntroTimer = 1;
		if(IntroLogoFader > 0.0f){
			IntroLogoFader -= CTankGame::Get().GetVW()->FrameFrac();
		}else{
		//	IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Ladder;
			return;
		}
	}else{
		IntroLogoFader += CTankGame::Get().GetVW()->FrameFrac();
		if(IntroLogoFader > IntroLogoFaderMax) IntroLogoFader = IntroLogoFaderMax;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Text Display Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoTextDisplay()
{
	Vec3 V, L;
	//
	EntityBase *e;
	int linelen = 60;
	//
	GUIHeader(BreakLine(DisplayText, DisplayTextPage, linelen), "logobackground", false);
	//
	SetVec3(.5, .1, 0, V);
	SetVec3(0, .04, 0, L);
	int dtpn = 0;
	{
		CStr s = " ";
		int i = 1;
		while(s.len() > 0){
			s = BreakLine(DisplayText, DisplayTextPage + i, linelen);
			if(s.len() > 0){
				GuiEnt("guibutton", "title1", V, CVec3(.015, .04, 0), CVec3(1, 1, 1),
					PadString(s, linelen), BID_None, false, 0, FLAG_TEXT_CENTERX);// | FLAG_TEXT_CENTERY);
			}
			i++;
			AddVec3(L, V);
		}
		dtpn = DisplayTextPage + i;
	}
	//
	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_OKAY), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	//
	//Odd hack here so we can check for first time on menu above text display to reset page, then check button presses down here where we know the line of the next page.
	switch(Activated(&e)){
	case BID_Back :
		if(BreakLine(DisplayText, dtpn, linelen).len() > 0){	//Just jump to next page of text.
			FreeGui();
			DisplayTextPage = dtpn;
			return;
		}else{
			CurrentMenu = ReturnFromText;	//Return to calling page.
			FreeGui();
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Image Help Menu (used for translations)
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoImageHelpDisplay()
{
	EntityBase *e;
	//
	GUIHeader("", "logobackground", false);
	//

	GuiEnt("guibutton", HelpImageEntity, CVec3(.5, 0, 0), CVec3(.1, .9, 0), CVec3(1, 1, 1),
		"         ", BID_None, false, 0, FLAG_TEXT_CENTERX); // | FLAG_TEXT_CENTERY);


	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_OKAY), BID_Back, false, 0, FLAG_TEXT_CENTERX);

	//
	switch(Activated(&e))
	{
	case BID_Back :
		CurrentMenu = ReturnFromText;	//Return to calling page.
		FreeGui();
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Ladder Roster Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoLadderRoster()
{
	char tempstr[256];
	EntityBase *e;
	//
	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Roster);
		return;
	case BID_FirstTimeOnMenu :
		ReadRoster(true);
		break;
	case BID_Back :
		FreeGui();
		CurrentMenu = MP_Main;
		return;
	case BID_Continue :
		if(RosterSelection >= 0){
			FreeGui();
			RosterEntry *rp = RosterHead.FindItem(RosterSelection + 1);
			if(rp && rp->file.len() > 0 && CTankGame::Get().GetLadder()->Load(rp->file)){
				CTankGame::Get().GetGameState()->LadderMode = true;
				CTankGame::Get().GetSettings()->TeamPlay = 0;
				CurrentMenu = MP_TankSelect;
				FreeGui();
			}
			return;
		}
		break;
	case BID_New :
		FreeGui();
		CurrentMenu = MP_LadderNew;
		return;
	case BID_Delete :
		if(RosterSelection >= 0){
			FreeGui();
			CurrentMenu = MP_LadderDelete;
			return;
		}
		break;
	case BID_Roster :
		if(e){
			RosterSelection = e->QueryInt(ATT_LISTBOX_SELECTION);
		}
		break;
	}
	//
	GUIHeader(Text.Get(TEXT_LADDERROSTER), "ladderbackground", true);
	//
	ReadRoster(false);	//Read saved games on disk once, without re-reading.

	GuiEnt("guibutton", "title1", CVec3(.05, .14, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_LADDERPLAYERS), BID_None);
	e = GuiEnt("guilistbox", "listbox1", CVec3(.05, .19, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1), 0, BID_Roster);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 15);
		e->SetInt(ATT_LISTBOX_HEIGHT, 5);
		int i = 0;
		for(RosterEntry *r = RosterHead.NextLink(); r; r = r->NextLink()){
			e->SetString(ATT_LISTBOX_NEWENTRY, r->name);
			if(i == RosterSelection){
				e->SetInt(ATT_LISTBOX_SELECTION, i);
			}
			i++;
		}
	}
	{
		RosterEntry *rp = RosterHead.FindItem(RosterSelection + 1);
		if(!rp) rp = &RosterHead;	//Point to an empty structure.
		// FIXME: buffer length
		if (rp->name.len() < 128) {
			sprintf(tempstr, Text.Get(TEXT_LADDERNAME), rp->name.get());
		} else {
			sprintf(tempstr, Text.Get(TEXT_LADDERNAME), "Error!");
		}
		GuiEnt("guibutton", "title1", CVec3(0.05, .55, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
			tempstr, BID_None, false, 0, 0);

		sprintf(tempstr, Text.Get(TEXT_LADDERRANK), Text.Get(TEXT_1ST+rp->rank));
		GuiEnt("guibutton", "title1", CVec3(0.05, .59, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
			tempstr, BID_None, false, 0, 0);

		sprintf(tempstr, Text.Get(TEXT_LADDERSTARTED), rp->racesstarted);
		GuiEnt("guibutton", "title1", CVec3(0.05, .63, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
			tempstr, BID_None, false, 0, 0);

		sprintf(tempstr, Text.Get(TEXT_LADDERFINISHED), rp->racesfinished);
		GuiEnt("guibutton", "title1", CVec3(0.05, .67, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
			tempstr, BID_None, false, 0, 0);

		sprintf(tempstr, Text.Get(TEXT_LADDERTROPHIES), rp->racesaschamp);
		GuiEnt("guibutton", "title1", CVec3(0.05, .71, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
			tempstr, BID_None, false, 0, 0);

		sprintf(tempstr, Text.Get(TEXT_LADDERFRAGS), rp->frags);
		GuiEnt("guibutton", "title1", CVec3(0.05, .75, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
			tempstr, BID_None, false, 0, 0);

		sprintf(tempstr, Text.Get(TEXT_LADDERDEATHS), rp->deaths);
		GuiEnt("guibutton", "title1", CVec3(0.05, .79, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
			tempstr, BID_None, false, 0, 0);
	}

	GuiEnt("guibutton", "button1", CVec3(0.75, .25, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_LADDERNEW), BID_New, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(0.75, .35, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_LADDERDELETE), BID_Delete, false, 0, FLAG_TEXT_CENTERX | (RosterSelection < 0 ? FLAG_BUTTON_DISABLED : 0));
	//
	GuiEnt("guibutton", "button1", CVec3(0.75, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_OKAY), BID_Continue, false, 0, FLAG_TEXT_CENTERX | (RosterSelection < 0 ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.25, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Ladder New Player Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoLadderNew()
{
	EntityBase *e;
	//
	switch(Activated(&e)){
	case BID_Name :
		if(e){
			if(_strnicmp(CTankGame::Get().GetAIPrefix(), e->QueryString(ATT_TEXT_STRING), strlen(CTankGame::Get().GetAIPrefix())) == 0)
				e->SetString(ATT_TEXT_STRING, "");
			CTankGame::Get().GetSettings()->PlayerName = e->QueryString(ATT_TEXT_STRING);
		}
		break;
	case BID_Skill :
		if(e){
			RankSelection = e->QueryInt(ATT_LISTBOX_SELECTION);
		}
		break;
	case BID_Continue :
		if(CTankGame::Get().GetSettings()->PlayerName.len() > 0){
			FreeGui();
			CurrentMenu = MP_LadderRoster;
			//
			CTankGame::Get().GetLadder()->Init(CTankGame::Get().GetSettings()->PlayerName, 39 + CLAMP(RankSelection, 0, 4) * 15);
			CStr sLadderWildCard(GetCommonAppDataDir());
			sLadderWildCard.cat("scores/");
			CTankGame::Get().GetLadder()->Save(sLadderWildCard + FileNameSafe(CTankGame::Get().GetSettings()->PlayerName) + String(rand()) + ".ladder");
			//
			ReadRoster(true);
			RosterSelection = -1;
			return;
		}
		break;
	case BID_Back :
		FreeGui();
		CurrentMenu = MP_LadderRoster;
		return;
	}
	//
	GUIHeader(Text.Get(TEXT_NEWLADDERCREATE), "ladderbackground", false);
	//
	e = GuiEnt("guiedit", "edit1", CVec3(.025, .2, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		CTankGame::Get().GetSettings()->PlayerName, BID_Name, true, 17, 0);
	if(e) e->SetString(ATT_EDIT_HEADER, "Name: ");
	//
	GuiEnt("guibutton", "title1", CVec3(.275, .4, 0), CVec3(.03, .06, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_NEWLADDERSTART), BID_None, false, 0, 0);
	e = GuiEnt("guilistbox", "listbox4", CVec3(.275, .46, 0), CVec3(.03, .06, 0), CVec3(1, 1, 1), 0, BID_Skill);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 15);
		e->SetInt(ATT_LISTBOX_HEIGHT, 5);
		e->SetString(ATT_LISTBOX_NEWENTRY, Text.Get(TEXT_NEWLADDERRANK40));
		e->SetString(ATT_LISTBOX_NEWENTRY, Text.Get(TEXT_NEWLADDERRANK55));
		e->SetString(ATT_LISTBOX_NEWENTRY, Text.Get(TEXT_NEWLADDERRANK70));
		e->SetString(ATT_LISTBOX_NEWENTRY, Text.Get(TEXT_NEWLADDERRANK85));
		e->SetString(ATT_LISTBOX_NEWENTRY, Text.Get(TEXT_NEWLADDERRANK00));
		e->SetInt(ATT_LISTBOX_SELECTION, RankSelection);
	}
	//
	GuiEnt("guibutton", "button1", CVec3(0.75, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_OKAY), BID_Continue, false, 0, FLAG_TEXT_CENTERX | (CTankGame::Get().GetSettings()->PlayerName.len() < 1 ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.25, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_CANCEL), BID_Back, false, 0, FLAG_TEXT_CENTERX);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Ladder Delete Player Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoLadderDelete()
{
	EntityBase *e;
	GUIHeader(Text.Get(TEXT_DELLADDERHDR), "ladderbackground", false);
	//
	{
		RosterEntry *rp = RosterHead.FindItem(RosterSelection + 1);
		if(!rp) rp = &RosterHead;
		e = GuiEnt("guibutton", "title1", CVec3(.5, .2, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
			rp->name, BID_None, false, 0, FLAG_TEXT_CENTERX);
	}

	GuiEnt("guibutton", "title1", CVec3(.5, .4, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_DELLADDERCONFIRM), BID_None, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);
	GuiEnt("guibutton", "button1", CVec3(.25, .6, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_YES), BID_Yes, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);
	GuiEnt("guibutton", "button1", CVec3(.75, .6, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text_GetLine(TEXT_NO), BID_No, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);
	//
	switch(Activated(&e)){
	case BID_Yes :
		FreeGui();
		CurrentMenu = MP_LadderRoster;
		{
			RosterEntry *rp = RosterHead.FindItem(RosterSelection + 1);
			if(rp && rp->file.len() > 0){
				remove(rp->file);
			}
			ReadRoster(true);
			RosterSelection = -1;
		}
		return;
	case BID_No :
		FreeGui();
		CurrentMenu = MP_LadderRoster;
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Ladder Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoLadder()
{
	char tempstr[256];
	EntityBase *e;

	switch(Activated(&e)){
	case BID_FirstTimeOnMenu :
		break;
	case BID_Help :
		StartHelp(HELP_Ladder);
		return;
	case BID_Race :
		RegistrySave();
		FreeGui();
		StartingGameDelay = 1500;
		CTankGame::Get().GetGameState()->AutoDrive = false;
		CTankGame::Get().GetGameState()->LadderMode = true;
		CTankGame::Get().GetGameState()->NumAITanks = 0;
		CTankGame::Get().GetLadder()->SetupRace(CTankGame::Get().GetSettings()->AIAutoFillCount + 1);	//Calculate which ladder members will compete.
		CTankGame::Get().GetLadder()->Save();	//Save RacesStarted stat.
		CTankGame::Get().GetGameState()->TeamHash = 0;
		CTankGame::Get().GetSettings()->StartDelay = 5;
		CTankGame::Get().GetSettings()->Deathmatch = 0;
		CTankGame::Get().GetSettings()->LimitTankTypes = 0;
		CurrentMenu = MP_StartingGame;
		FreeGui();
		return;
	case BID_Back :
		FreeGui();
		CurrentMenu = MP_TankSelect;
		return;
	case BID_AITanks :
		CTankGame::Get().GetSettings()->AIAutoFillCount += 2;
		if(CTankGame::Get().GetSettings()->AIAutoFillCount > 8)
			CTankGame::Get().GetSettings()->AIAutoFillCount = 2;
		break;
	case BID_Laps :
		CTankGame::Get().GetSettings()->Laps++;
		if(CTankGame::Get().GetSettings()->Laps > 3) CTankGame::Get().GetSettings()->Laps = 1;
		break;
	case BID_Stats :
		CurrentMenu = MP_Stats;
		ReturnToMenu = MP_Ladder;
		LoadStats();
		FreeGui();
		return;
	}
	//
	GUIHeader(Text.Get(TEXT_LADDERPROGRESS), ((CTankGame::Get().GetLadder()->PlayerRank == 0) ? "trophybackground" : "ladderbackground"), true);
	//
	CTankGame::Get().GetSettings()->MirroredMap = CTankGame::Get().GetLadder()->Mirrored;
	CTankGame::Get().GetSettings()->MapFile = CTankGame::Get().GetLadder()->GetNextMapFile();
	CTankGame::Get().GetSettings()->PlayerName = CTankGame::Get().GetLadder()->PlayerName;
	if(CTankGame::Get().GetSettings()->AIAutoFillCount != 2 && CTankGame::Get().GetSettings()->AIAutoFillCount != 4 && CTankGame::Get().GetSettings()->AIAutoFillCount != 6 && CTankGame::Get().GetSettings()->AIAutoFillCount != 8)
		CTankGame::Get().GetSettings()->AIAutoFillCount = 4;
	if(CTankGame::Get().GetSettings()->Laps < 1 || CTankGame::Get().GetSettings()->Laps > 3)
		CTankGame::Get().GetSettings()->Laps = 2;
	//
	e = GuiEnt("guiimagebrowser", "ibmap", CVec3(.05, .14, 0), CVec3(.40, .40, 0), CVec3(1, 1, 1), NULL, BID_None);
	if(e){
		if(MapImageFailureFile == CTankGame::Get().GetSettings()->MapFile || !e->SetString(ATT_BROWSER_SETIMAGE, FileNoExtension(CTankGame::Get().GetSettings()->MapFile) + "_shot.bmp")){
			e->SetString(ATT_BROWSER_SETIMAGE, "textures/ImageNotAvailable.bmp");
			MapImageFailureFile = CTankGame::Get().GetSettings()->MapFile;	//This prevents oscillations, which suck hard.
		}
		e->SetInt(ATT_BUTTON_XFLIP, CTankGame::Get().GetSettings()->MirroredMap);	//Flip image when mirrored selected.
	}
	// TODO: Russ - localise
	GuiEnt("guibutton", "title1", CVec3(0.05, .55, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		"Map: " + CTankGame::Get().GetLadder()->GetNextMapName(), BID_None, false, 0, 0);
	//
	sprintf(tempstr, Text.Get(TEXT_LADDEROPPONENTS), CTankGame::Get().GetSettings()->AIAutoFillCount);
	e = GuiEnt("guibutton", "button1", CVec3(.05, .70, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		tempstr, BID_AITanks, false, 0, 0);

	sprintf(tempstr, Text.Get(TEXT_LADDERLAPS), CTankGame::Get().GetSettings()->Laps);
	e = GuiEnt("guibutton", "button1", CVec3(.05, .75, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		tempstr, BID_Laps, false, 0, 0);
	//
	GuiEnt("guibutton", "title1", CVec3(.50, .14, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_LADDERLADDER), BID_None);

	e = GuiEnt("guilistbox", "listbox2", CVec3(.50, .19, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_None);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 19);
		e->SetInt(ATT_LISTBOX_HEIGHT, 13);
		for(int i = 0; i < LADDER_SIZE; i++){
			e->SetString(ATT_LISTBOX_NEWENTRY,
				PadString(Text.Get(TEXT_1ST+i), 6) + CTankGame::Get().GetLadder()->ladder[i].name);
			if(i == CTankGame::Get().GetLadder()->PlayerRank){
				e->SetInt(ATT_LISTBOX_SELECTION, i);
			}
		}
	}
	//
	GuiEnt("guibutton", "button1", CVec3(0.75, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_START), BID_Race, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.25, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	//
	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_VIEW), BID_Stats, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.5, .94, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_STATS), BID_Stats, false, 0, FLAG_TEXT_CENTERX);
	//
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Stats Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoStats()
{
	EntityBase *e;
	//
	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Stats);
		return;
	case BID_Back :
		FreeGui();
		CurrentMenu = ReturnToMenu;
		return;
	}
	//
	GUIHeader(Text.Get(TEXT_STATHALL), "resultsbackground", true);
	//
	GuiEnt("guibutton", "title1", CVec3(0.5, .15, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		(CTankGame::Get().GetSettings()->Deathmatch ? Text.Get(TEXT_STATBATTLE) : Text.Get(TEXT_STATRACE)), BID_None, false, 0, FLAG_TEXT_CENTERX);
	{
		CStr name = Text.Get(TEXT_STATUNKNOWN);
		for(int i = 0; i < CTankGame::Get().GetNumMaps(); i++){
			if(CmpLower(CTankGame::Get().GetSettings()->MapFile, CTankGame::Get().GetMap(i)->file)){	//Match map filename to title.  Ick...
				name = CTankGame::Get().GetMap(i)->title;
				break;
			}
		}
		GuiEnt("guibutton", "title1", CVec3(0.5, .225, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			"\"" + name + "\"", BID_None, false, 0, FLAG_TEXT_CENTERX);
	}
	//
	GuiEnt("guibutton", "title1", CVec3(0.005, .35, 0), CVec3(.015, .04, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_STATHDR), BID_None, false, 0, 0);
	e = GuiEnt("guilistbox", "listbox1", CVec3(.005, .39, 0), CVec3(.015, .04, 0), CVec3(1, 1, 1), 0, BID_None);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 66);
		e->SetInt(ATT_LISTBOX_HEIGHT, 10);
		for(int i = 0; i < STAT_ENTRIES; i++){
			StatsEntry t = CTankGame::Get().GetStats()->stats[i];
			e->SetString(ATT_LISTBOX_NEWENTRY,
				PadString(Text.Get(TEXT_1ST+i), 5) +
				PadString(t.name, 10) +
				PadString(CTankGame::Get().MMSS(t.bestlaptime / 1000), 7) +
				PadString(String(t.fragsperhour), 4) +
				PadString(String(t.deathsperhour), 4) +
				PadString(String(t.frags), 6) +
				PadString(String(t.deaths), 6) +
				PadString(String(t.laps), 5) +
				PadString(String(t.place), 6) +
				PadString(String((int)(t.skill * 100.0f + 0.5f)) + "%", 6) +
				PadString(CTankGame::Get().MMSS(t.time / 1000), 6) );
			if(CTankGame::Get().GetStats()->LastPlayerPos == i){
				e->SetInt(ATT_LISTBOX_SELECTION, i);
			}
		}
	}
	//
	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_OKAY), BID_Back, false, 0, FLAG_TEXT_CENTERX);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									In-Game Main Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoInGameMain()
{
	EntityBase *e;
//	GuiEntStart();
//	if(MenuFire) GuiEnt("guibackground", "pfirebackground", CVec3(0, 0, PFIRE_Z), NULL, NULL, NULL, BID_None);
//	GuiEnt("guimouse", "mouse1", NULL, NULL, NULL, NULL, BID_None);
//	GuiEnt("guibackground", "logobackground", CVec3(0, 0, 1), NULL, NULL, NULL, BID_None);
//	GuiEnt("guibutton", "title1", CVec3(0.5, .02, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
//		"Game In Progress Menu", BID_None, false, 0, FLAG_TEXT_CENTERX);
	//
	GUIHeader(Text.Get(TEXT_IGMHDR), "logobackground", true);
	//
	GuiEnt("guibutton", "button1", CVec3(.5, .21, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_IGMRETURN), BID_Back, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);
	GuiEnt("guibutton", "button1", CVec3(.5, .31, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_IGMTANK), BID_Tank, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY |
		(CTankGame::Get().GetGameState()->DemoMode ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.5, .41, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_IGMSERVER), BID_Multi, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY
		| (CTankGame::Get().GetGameState()->ActAsServer ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.5, .51, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_IGMGAME), BID_Options, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);
	GuiEnt("guibutton", "button1", CVec3(.5, .61, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_IGMSTOP), BID_Quit, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);
	//
	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_InGameMain);
		return;
	case BID_Back :
		CTankGame::Get().GetGameState()->ToggleMenu = true;
		return;
	case BID_Tank :
		if(CTankGame::Get().GetGameState()->DemoMode == false){
			FreeGui();
			CurrentMenu = MP_TankSelect;
		}
		return;
	case BID_Multi :
		if(CTankGame::Get().GetGameState()->ActAsServer == false){//CTankGame::Get().GetVW()->Net.IsClientActive()){
			FreeGui();
			CurrentMenu = MP_ServerSelect;
		}
		return;
	case BID_Options :
		FreeGui();
		CurrentMenu = MP_Options;
		return;
	case BID_Quit :
		CTankGame::Get().StopGame();
		FreeGui();
		CurrentMenu = MP_Main;
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Main Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoMain()
{
	Vec3 V, L;
	EntityBase *e;

	GUIHeader("Tread Marks " VERSION_STRING, "logobackground", true);

	//GuiEnt("guibutton", "button1", CVec3(.95, 0.02, 0), CVec3(.02, .035, 0), CVec3(1, 1, 1),
	//	"UK", BID_None, false, 0, 0);

	//
	SetVec3(.5, 0.1825, 0, V);
	SetVec3(0, .075, 0, L);
	//

#if defined(_JAPAN)

		GuiEnt("guibutton", "jprollingdemo", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Demo, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "jpfieldmanual", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Tutorial, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "jptraining", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Training, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "jpsingle", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Race, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "jpladder", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Tourney, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY /*| FLAG_BUTTON_DISABLED*/); AddVec3(L, V);

		GuiEnt("guibutton", "jpmultiplayer", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Multi, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "jpsettings", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Options, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
		GuiEnt("guibutton", "jpcredits", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Credits, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
		GuiEnt("guibutton", "jpwebsite", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Web, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(CVec3(0, .0525, 0), V);

		GuiEnt("guibutton", "jpquit", CVec3(.5, .91, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"            ", BID_Quit, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);

#else

	#ifndef CHINA
		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU2), BID_Demo, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
	#endif

		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU1), BID_Tutorial, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			"Training Maps", BID_Training, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU3), BID_Race, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);

		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU4), BID_Tourney, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY /*| FLAG_BUTTON_DISABLED*/); AddVec3(L, V);

	#ifndef CHINA
		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU5), BID_Multi, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
	#endif

		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU6), BID_Options, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU7), BID_Credits, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
		GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU8), BID_Web, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(CVec3(0, .0525, 0), V);
		GuiEnt("guibutton", "title1", V, CVec3(.015, .035, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU9), BID_None, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
		GuiEnt("guibutton", "button1", CVec3(.5, .91, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
			Text_GetLine(TEXT_MENU10), BID_Quit, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);

#endif

	//
	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Main);
		return;
	case BID_Credits :
		StartHelp(HELP_Story);
		return;
	case BID_Ordering :
		StartHelp(HELP_OrderingInfo);
		return;
	case BID_Web :
		SetActiveWindow(NULL);
		OpenWebLink("http://www.longbowgames.com");

		break;
	case BID_Tutorial:
		FreeGui();
		CurrentMenu = MP_Tutorial;
		break;
	case BID_Training:
		FreeGui();
		bTrainingOnly = true;
		CTankGame::Get().GetGameState()->DemoMode = false;
		CTankGame::Get().GetGameState()->LadderMode = false;
		CTankGame::Get().GetGameState()->ActAsServer = true;
		CTankGame::Get().GetVW()->Net.ClientDisconnect(CTankGame::Get().GetVW());	//Make sure connections are empty when entering single player mode.
		CurrentMenu = MP_TankSelect;
		FreeGui();
		break;
	case BID_Demo :
		FreeGui();
		CTankGame::Get().GetGameState()->LadderMode = false;
		CTankGame::Get().GetGameState()->ActAsServer = true;
		CTankGame::Get().GetGameState()->AutoDrive = true;
		CTankGame::Get().GetGameState()->NumAITanks = 0;
		CTankGame::Get().GetSettings()->AIAutoFillCount = 5;
		CTankGame::Get().GetSettings()->StartDelay = MIN(CTankGame::Get().GetSettings()->StartDelay, 5);
		CurrentMenu = MP_StartingGame;
		StartingGameDelay = 1500;
		CTankGame::Get().GetGameState()->DemoMode = true;
		CTankGame::Get().GetSettings()->TimeLimit = 20;
		CTankGame::Get().GetSettings()->FragLimit = 10;
		CTankGame::Get().GetSettings()->Laps = 2;
		CTankGame::Get().GetSettings()->EnemySkill = 1.0f;
		return;
	case BID_Race :
		FreeGui();
		bTrainingOnly = false;
		CTankGame::Get().GetGameState()->DemoMode = false;
		CTankGame::Get().GetGameState()->LadderMode = false;
		CTankGame::Get().GetGameState()->ActAsServer = true;
		CTankGame::Get().GetVW()->Net.ClientDisconnect(CTankGame::Get().GetVW());	//Make sure connections are empty when entering single player mode.
		CurrentMenu = MP_TankSelect;
		FreeGui();
		return;
	case BID_Tourney :
		//Set some mode that signifies tourney mode.
		FreeGui();
		bTrainingOnly = false;
		CTankGame::Get().GetGameState()->DemoMode = false;
		CTankGame::Get().GetGameState()->LadderMode = true;
		CTankGame::Get().GetGameState()->ActAsServer = true;
		CTankGame::Get().GetSettings()->Deathmatch = 0;
		CTankGame::Get().GetVW()->Net.ClientDisconnect(CTankGame::Get().GetVW());	//Make sure connections are empty when entering single player mode.
		CurrentMenu = MP_LadderRoster;
		FreeGui();
		return;
	case BID_Multi :
		FreeGui();
		bTrainingOnly = false;
		CTankGame::Get().GetGameState()->DemoMode = false;
		CTankGame::Get().GetGameState()->LadderMode = false;
		CTankGame::Get().GetGameState()->ActAsServer = false;
		CurrentMenu = MP_TankSelect;
		return;
	case BID_Options :
		CTankGame::Get().GetGameState()->DemoMode = false;
		CTankGame::Get().GetGameState()->LadderMode = false;
		CTankGame::Get().GetGameState()->ActAsServer = false;
		VideoSwitchNeeded = SoundSwitchNeeded = false;
		FreeGui();
		CurrentMenu = MP_Options;
		return;
	case BID_Quit :
		CTankGame::Get().GetGameState()->Quit = true;
		FreeGui();
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									TankSelect Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoTankSelect()
{
	EntityBase *e;
	//
	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Tank);
		return;
	case BID_FirstTimeOnMenu :
		ReadRoster();
		CTankGame::Get().EnumerateTeams();
		CTankGame::Get().EnumerateTanks();	//Re-enum tanks and teams to get current server side tank and team list for selection.  Insignias don't need re-enumming.
		break;
	case BID_Tank :
		if(e){
			int i = e->QueryInt(ATT_LISTBOX_SELECTION);
			CTankGame::Get().GetSettings()->TankType = CTankGame::Get().GetTankType(i).type;
			ClientInfoSendNeeded = true;
			EntityTypeBase *et = CTankGame::Get().GetVW()->FindEntityType("racetank", CTankGame::Get().GetSettings()->TankType);
			if(et){
				CTankGame::Get().GetVW()->AddEntity("sound", et->InterrogateString("namesound"), NULL, NULL, NULL, 0, FLAG_SOUND_PLAYANNOUNCER, 0, 0);
			}
		}
		break;
	case BID_Team :
		if(e){
			int i = e->QueryInt(ATT_LISTBOX_SELECTION);
			if(i == 0)
				CTankGame::Get().GetGameState()->TeamHash = 0;
			else
				CTankGame::Get().GetGameState()->TeamHash = CTankGame::Get().GetTeam(CLAMP(i - 1, 0, CTankGame::Get().GetNumAvailTeams())).hash;
		}
		break;
	case BID_Insignia :
		if(e){
			EntityTypeBase *et = 0;
			int cookie = 0, i = e->QueryInt(ATT_LISTBOX_SELECTION);
			do{
				cookie = CTankGame::Get().GetVW()->EnumEntityTypes("insignia", &et, cookie);
				if(et && et->InterrogateInt("team") == 0){
					if(--i < 0){
						CTankGame::Get().GetSettings()->InsigniaType = et->tname;
						break;
					}
				}
			}while(et && cookie);
		}
		break;
	case BID_NameToggle :
		{
			//Toggle through player names in ladder roster for easier name selection.
			RosterSelection++;
			RosterSelection = MAX(0, RosterSelection);
			RosterEntry *rp = RosterHead.FindItem(RosterSelection + 1);
			if(rp == NULL){
				RosterSelection = 0;
				rp = RosterHead.FindItem(RosterSelection + 1);
			}
			if(rp){
				CTankGame::Get().GetSettings()->PlayerName = rp->name;
			}
		}
		break;
	case BID_Name :
		if(e){
			CTankGame::Get().GetSettings()->PlayerName = e->QueryString(ATT_TEXT_STRING);
			if(_strnicmp(CTankGame::Get().GetAIPrefix(), CTankGame::Get().GetSettings()->PlayerName, strlen(CTankGame::Get().GetAIPrefix())) == 0)
				CTankGame::Get().GetSettings()->PlayerName = "";
			ClientInfoSendNeeded = true;
		}
		break;
	case BID_Back :
		if(CTankGame::Get().IsGameRunning()){
			CurrentMenu = MP_InGameMain;
		}else{
			if(CTankGame::Get().GetGameState()->LadderMode){
				CurrentMenu = MP_LadderRoster;
			}else{
				CurrentMenu = MP_Main;
			}
		}
		FreeGui();
		return;
	case BID_Continue :
		if(CTankGame::Get().GetGameState()->LadderMode){
			CurrentMenu = MP_Ladder;
			FreeGui();
		}else{
			if(CTankGame::Get().GetGameState()->ActAsServer){
				CurrentMenu = MP_MapSelect;
				FreeGui();
			}else{
				CurrentMenu = MP_ServerSelect;
			}
		}
		FreeGui();
		return;
	}
	//
	GUIHeader(Text.Get(TEXT_TANKSELECT), (CTankGame::Get().GetGameState()->LadderMode ? "ladderbackground" : "logobackground" ), true);
	//
	GuiEnt("guibutton", "title1", CVec3(.025, .14, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_TANKTREADS), BID_None);
	e = GuiEnt("guilistbox", "listbox1", CVec3(.025, .19, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_Tank);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 17);
		e->SetInt(ATT_LISTBOX_HEIGHT, 7);
		int sel = 0;
		for(int i = 0; i < CTankGame::Get().GetNumTankTypes(); i++){
			e->SetString(ATT_LISTBOX_NEWENTRY, CTankGame::Get().GetTankType(i).title);
			if(CTankGame::Get().GetTankType(i).type == CTankGame::Get().GetSettings()->TankType){
				e->SetInt(ATT_LISTBOX_SELECTION, i);
				sel = 1;
			}
		}
		if(sel == 0)
			CTankGame::Get().GetSettings()->TankType = CTankGame::Get().GetTankType(0).type;
	}
	//
	e = GuiEnt("racetankdoppelganger", "doppelganger1", CVec3(-4.0f, -3.9f, 20.0f), CVec3(0, 0, 0), CVec3(0, 0, 0), 0, BID_None);
	if(e){
		//
		e->SetString(ATT_DOPPELGANG, CTankGame::Get().GetSettings()->TankType);
		e->SetRot(CVec3(0, fmod(((double)CTankGame::Get().GetVW()->Time() * 0.001) * 1.0, (double)PI * 2.0), 0));
		e->SetFloat(ATT_TURRETHEADING, fmod(((double)CTankGame::Get().GetVW()->Time() * 0.001) * 0.12, (double)PI * 2.0));
		//
	}
	//
	if(CTankGame::Get().GetGameState()->LadderMode == false){
		GuiEnt("guibutton", "button1", CVec3(.05, .58, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_TANKNAME), BID_NameToggle, false, 0, 0);
		e = GuiEnt("guiedit", "edit1", CVec3(.05 + (.025 * 6.0), .58, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			CTankGame::Get().GetSettings()->PlayerName, BID_Name, false, 15 - 6, 0);
	}
	//
	GuiEnt("guibutton", "title1", CVec3(.55, .14, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_TANKINSIGNIA), BID_None);
	e = GuiEnt("guilistbox", "listbox1", CVec3(.55, .19, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_Insignia);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 15);
		e->SetInt(ATT_LISTBOX_HEIGHT, 7);
		EntityTypeBase *et = 0;
		int cookie = 0, i = 0, sel = 0;
		CStr ltname;
		do{
			if((cookie = CTankGame::Get().GetVW()->EnumEntityTypes("insignia", &et, cookie)) && et && et->InterrogateInt("team") == 0){
				if (et->nameid != -1) {
					e->SetString(ATT_LISTBOX_NEWENTRY, Insignia.Get(et->nameid));
				} else {
					e->SetString(ATT_LISTBOX_NEWENTRY, et->dname);
				}
				if((ltname = et->tname) == CTankGame::Get().GetSettings()->InsigniaType){
					e->SetInt(ATT_LISTBOX_SELECTION, i);
					sel = 1;
				}
				i++;
			}
		}while(et && cookie);
		if(sel == 0)
			CTankGame::Get().GetSettings()->InsigniaType = ltname;	//Don't allow null insignia for humans.
	}
	//e = GuiEnt("guiimagebrowser", "ibinsignia", CVec3(.55, .58, 0), CVec3(.20, .2666, 0), CVec3(1, 1, 1),
	e = GuiEnt("guiimagebrowser", "ibinsignia", CVec3(.55, .6166, 0), CVec3(.20, .2666, 0), CVec3(1, 1, 1),
		NULL, BID_None);
	if(e){
		EntityTypeBase *et = CTankGame::Get().GetVW()->FindEntityType("insignia", CTankGame::Get().GetSettings()->InsigniaType);

		if(et)
		{
			e->SetInt(ATT_BUTTON_XFLIP, et->InterrogateInt("hflip") );
			e->SetString(ATT_BROWSER_SETIMAGE, et->InterrogateString("texturename"));
		}
	}
	//
	if(CTankGame::Get().GetGameState()->LadderMode == false){
		GuiEnt("guibutton", "title1", CVec3(.775, .58, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_TANKTEAM), BID_None);
		e = GuiEnt("guilistbox", "listbox2", CVec3(.775, .63, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_Team);
		if(e){
			e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
			e->SetInt(ATT_LISTBOX_WIDTH, 8);
			e->SetInt(ATT_LISTBOX_HEIGHT, 5);
			EntityTypeBase *et = 0;
			int cookie = 0, i = 0, sel = 0;
			CStr ltname;
			for(i = 0; i < CTankGame::Get().GetNumAvailTeams() + 1; i++){
				if(i == 0) e->SetString(ATT_LISTBOX_NEWENTRY, "None");
				else e->SetString(ATT_LISTBOX_NEWENTRY, CTankGame::Get().GetTeam(i - 1).name);
				if((i == 0 && CTankGame::Get().GetGameState()->TeamHash == 0) || (i > 0 && CTankGame::Get().GetGameState()->TeamHash == CTankGame::Get().GetTeam(i - 1).hash)){
					e->SetInt(ATT_LISTBOX_SELECTION, i);
				}
			}
		}
	}
	//
	if(CTankGame::Get().IsGameRunning()){
		GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	}else{
		GuiEnt("guibutton", "button1", CVec3(0.75, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_OKAY), BID_Continue, false, 0, FLAG_TEXT_CENTERX);
		GuiEnt("guibutton", "button1", CVec3(.25, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	}
}

class TutInfo
{
public:
	CStr Name;
	int ID;
	TutInfo *next;
	EntityTutorialType *ptr;

	TutInfo() {next = NULL; ID = -1; ptr = NULL;}
};

class SortedTutList
{
public:
	TutInfo* head;

	SortedTutList() {head = NULL;}
	~SortedTutList();

	void Insert(TutInfo* insert);
	void BuildList();
	EntityTutorialType	*FindTut(int TutNum);
};

void SortedTutList::Insert(TutInfo* insert)
{
	TutInfo *pThis = head;
	TutInfo *pLast = NULL;

	if (head == NULL)
	{
		head = insert;
		insert->next = NULL;
		return;
	}
	while(pThis)
	{
		if (insert->ID < pThis->ID)
		{
			if (pLast == NULL)
			{
				insert->next = head;
				head = insert;
			}
			else
			{
				insert->next = pThis;
				pLast->next = insert;
			}
			return;
		}
		pLast = pThis;
		pThis = pThis->next;
	}
	pLast->next = insert;
	insert->next = NULL;
}

SortedTutList::~SortedTutList()
{
	TutInfo *tmp = head;
	while (tmp)
	{
		head = tmp->next;
		delete tmp;
		tmp = head;
	}
}

void SortedTutList::BuildList()
{
	int iCookie = 0;
	EntityTutorialType	*tmpType;
	while ((iCookie = CTankGame::Get().GetVW()->EnumEntityTypes("tutorial", (EntityTypeBase**)&tmpType, iCookie)) != 0)
	{
		TutInfo *tmp = new TutInfo;
		if (tmpType->nameid != -1) {
			tmp->Name = Names.Get(tmpType->nameid);
		} else {
			tmp->Name = tmpType->InterrogateString("dname");
		}
		tmp->ptr = tmpType;
		tmp->ID = tmpType->InterrogateInt("tutid");
		Insert(tmp);
	}
}

EntityTutorialType	*SortedTutList::FindTut(int TutNum)
{
	int iCnt = 0;
	TutInfo *pThis = head;

	if (head == NULL) return NULL;
	while(pThis)
	{
		if (iCnt == TutNum) return pThis->ptr;
		iCnt++;

		pThis = pThis->next;
	}
	return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//									Tutorial Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoTutorial()
{
	static int SelectedTut = -1;
	EntityBase *e;
	SortedTutList TutList;

	switch(Activated(&e)){
	case BID_Back :
		CurrentMenu = MP_Main;
		FreeGui();
		return;
	case BID_TutSelect:
		SelectedTut = e->QueryInt(ATT_LISTBOX_SELECTION);
		return;
	}

	#ifdef _JAPAN
		GUIHeader("", "logobackground", false);

		GuiEnt("guibutton", "jptuttitle", CVec3(.32, .02, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1), "             ", BID_None);
	#else
		GUIHeader(Text.Get(TEXT_TUTHDR), "logobackground", false);
	#endif

	GuiEnt("guibutton", "title1", CVec3(.025, .14, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_TUTCHAPTER), BID_None);

	e = GuiEnt("guilistbox", "listbox1", CVec3(.025, .19, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_TutSelect);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 17);
		e->SetInt(ATT_LISTBOX_HEIGHT, 7);

		int set = 0;
		int iCnt = 0;

		TutList.BuildList();
		TutInfo *tmp = TutList.head;

		while(tmp != NULL)
		{
			e->SetString(ATT_LISTBOX_NEWENTRY, tmp->Name);
			iCnt++;
			tmp = tmp->next;
		}
		if ((SelectedTut<0) || (SelectedTut>=iCnt)) {
			e->SetInt(ATT_LISTBOX_SELECTION, 0);
			SelectedTut = 0;
		} else {
			e->SetInt(ATT_LISTBOX_SELECTION, SelectedTut);
		}
	}

	// Text description
	EntityTutorialType	*tmpType = TutList.FindTut(SelectedTut);
	CStr				sImageDesc;

	if(tmpType)
		sImageDesc = tmpType->InterrogateString("imagedesc");

	// if there is no description image then show the text version
	GuiEnt("guibutton", "title1", CVec3(.025, .56, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		(sImageDesc == "") ? Text.Get(TEXT_TUTDESC) : "", BID_None);

	e = GuiEnt("guilistbox", "listbox2", CVec3(.025, .61, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_None);

	if(sImageDesc != "") e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
	else
	{
		if(e){
			e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
			e->SetInt(ATT_LISTBOX_WIDTH, 38);
			e->SetInt(ATT_LISTBOX_HEIGHT, 7);
			EntityTutorialType	*tmpType = TutList.FindTut(SelectedTut);
			if (tmpType != NULL)
			{
				if (tmpType->descid != -1) {
					char *src;
					int idx;
					CStr tmpstring;

					// Add string from block

					src = TextBlock2.Get(tmpType->descid);
					while (*src) {
						idx = 0;
						// Find end of string
						while (src[idx] && (src[idx]!='\n') && (src[idx]!='\r') ) idx++;

						// Set string to part of range
						if (idx > 0) {
							tmpstring = Left(src, idx);
							if (tmpstring == CStr("-")) {
								e->SetString(ATT_LISTBOX_NEWENTRY, CStr(""));
							} else {
								e->SetString(ATT_LISTBOX_NEWENTRY, tmpstring);
							}
						} else {
							e->SetString(ATT_LISTBOX_NEWENTRY, CStr(""));
						}

						// Move to end of string
						while ( (src[idx]=='\n') || (src[idx]=='\r') ) idx++;
						src = src + idx;
					}

				} else {
					// Add strings from .ENT
					for (int i = 0; i < MAX_DESC_STRINGS; i++) {
						if (tmpType->DescStrings[i] != CStr("")) {
							if (tmpType->DescStrings[i] == CStr("-")) {
								e->SetString(ATT_LISTBOX_NEWENTRY, CStr(""));
							} else {
								e->SetString(ATT_LISTBOX_NEWENTRY, tmpType->DescStrings[i]);
							}
						}
					}

				}

			}
		}
	}

	// otherwise create an image browser to show the decription image
	e = GuiEnt("guiimagebrowser", "ibtutorial", CVec3(.025, .56, 0), CVec3(.95, .3, 0), CVec3(1, 1, 1),
			NULL, BID_None);

	if(e)
	{
		e->SetString(ATT_BROWSER_SETIMAGE, sImageDesc);
	}

	// Rotating object
	e = GuiEnt("tutorial", tmpType->tname, CVec3(4.5f, 1.5f, 20.0f), CVec3(0, 0, 0), CVec3(0, 0, 0), 0, BID_None);
	if(e)
	{
		e->SetString(ATT_DOPPELGANG, tmpType->tname);
		e->SetRot(CVec3(0, fmod(((double)CTankGame::Get().GetVW()->Time() * 0.001) * 1.0, (double)PI * 2.0), 0));
	}

	GuiEnt("guibutton", "button1", CVec3(.5, .93, 0), CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_DONE), BID_Back, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									MapSelect Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoMapSelect()
{
	char tempstr[256];
	EntityBase *e;
	static int   tmpGameType = CTankGame::Get().GetSettings()->Deathmatch;
	static int   tmpLaps = CTankGame::Get().GetSettings()->Laps;
	static int   tmpAITanks = CTankGame::Get().GetSettings()->AIAutoFillCount;
	static int   tmpTimeLimit = CTankGame::Get().GetSettings()->TimeLimit;
	static int   tmpFragLimit = CTankGame::Get().GetSettings()->FragLimit;
	static float tmpEnemySkill = CTankGame::Get().GetSettings()->EnemySkill;
	static int   tmpTankTypes = CTankGame::Get().GetSettings()->LimitTankTypes;
	static int   tmpStartDelay = CTankGame::Get().GetSettings()->StartDelay;
	static bool  tmpMirror = CTankGame::Get().GetSettings()->MirroredMap;
	static int   tmpAllowJoins = CTankGame::Get().GetSettings()->AllowSinglePlayerJoins;
	static int   tmpNumTeams = CTankGame::Get().GetSettings()->NumTeams;
	static int   tmpTeamScores = CTankGame::Get().GetSettings()->TeamScores;
	static int   tmpTeamDamage = CTankGame::Get().GetSettings()->TeamDamage;
	static int iCurMap = 0;
	//

	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Map);
		return;
	case BID_FirstTimeOnMenu :
		break;
	case BID_Map :
		if(e){
			int i = FindMapIndex(e->QueryString(ATT_LISTBOX_SELECTION));

			if(CTankGame::Get().GetMap(CLAMP(i, 0, CTankGame::Get().GetNumMaps()))->maptype != CTankGame::Get().GetMap(iCurMap)->maptype)
			{
				if (CTankGame::Get().GetMap(iCurMap)->maptype != MapTypeTraining)
				{
					tmpGameType = CTankGame::Get().GetSettings()->Deathmatch;
					tmpLaps = CTankGame::Get().GetSettings()->Laps;
					tmpAITanks = CTankGame::Get().GetSettings()->AIAutoFillCount;
					tmpTimeLimit = CTankGame::Get().GetSettings()->TimeLimit;
					tmpFragLimit = CTankGame::Get().GetSettings()->FragLimit;
					tmpEnemySkill = CTankGame::Get().GetSettings()->EnemySkill;
					tmpTankTypes = CTankGame::Get().GetSettings()->LimitTankTypes;
					tmpStartDelay = CTankGame::Get().GetSettings()->StartDelay;
					tmpMirror = CTankGame::Get().GetSettings()->MirroredMap;
					tmpAllowJoins = CTankGame::Get().GetSettings()->AllowSinglePlayerJoins;

					tmpNumTeams = CTankGame::Get().GetSettings()->NumTeams;
					tmpTeamScores = CTankGame::Get().GetSettings()->TeamScores;
					tmpTeamDamage = CTankGame::Get().GetSettings()->TeamDamage;
				}
				else
				{
					CTankGame::Get().GetSettings()->Deathmatch = tmpGameType;
					CTankGame::Get().GetSettings()->Laps = tmpLaps;
					CTankGame::Get().GetSettings()->AIAutoFillCount = tmpAITanks;
					CTankGame::Get().GetSettings()->TimeLimit = tmpTimeLimit;
					CTankGame::Get().GetSettings()->FragLimit = tmpFragLimit;
					CTankGame::Get().GetSettings()->EnemySkill = tmpEnemySkill;
					CTankGame::Get().GetSettings()->LimitTankTypes = tmpTankTypes;
					CTankGame::Get().GetSettings()->StartDelay = tmpStartDelay;
					CTankGame::Get().GetSettings()->MirroredMap = tmpMirror;
					CTankGame::Get().GetSettings()->AllowSinglePlayerJoins = tmpAllowJoins;

					CTankGame::Get().GetSettings()->NumTeams = tmpNumTeams;
					CTankGame::Get().GetSettings()->TeamScores = tmpTeamScores;
					CTankGame::Get().GetSettings()->TeamDamage = tmpTeamDamage;
				}
				FreeGui();
			}
			iCurMap = CLAMP(i, 0, CTankGame::Get().GetNumMaps());
			CTankGame::Get().GetSettings()->MapFile = CTankGame::Get().GetMap(iCurMap)->file;
			if(CmpLower(Left(CTankGame::Get().GetMap(iCurMap)->title, 3), "CTF"))
			{
				CTankGame::Get().GetSettings()->TeamScores = 1;
				CTankGame::Get().GetSettings()->NumTeams = MIN(CTankGame::Get().GetNumAvailTeams(), CTankGame::Get().GetMap(iCurMap)->MaxMapTeams);
			}
			//
			CTankGame::Get().GetSettings()->NumTeams = CLAMP(CTankGame::Get().GetSettings()->NumTeams, 0, MIN(CTankGame::Get().GetNumAvailTeams(), CTankGame::Get().GetMap(iCurMap)->MaxMapTeams));
			if(CTankGame::Get().IsMapDMOnly(i)) CTankGame::Get().GetSettings()->Deathmatch = 1;
		}
		break;
	case BID_TimeLimit :
		if(e) CTankGame::Get().GetSettings()->TimeLimit = e->QueryInt(ATT_EDIT_INT);
		break;
	case BID_TimeLimitToggle :
		CTankGame::Get().GetSettings()->TimeLimit = ((CTankGame::Get().GetSettings()->TimeLimit / 10) + 1) * 10;
		if(CTankGame::Get().GetSettings()->TimeLimit > 60)
			CTankGame::Get().GetSettings()->TimeLimit = 0;
		break;
	case BID_FragLimit :
		if(e) CTankGame::Get().GetSettings()->FragLimit = e->QueryInt(ATT_EDIT_INT);
		break;
	case BID_FragLimitToggle :
		CTankGame::Get().GetSettings()->FragLimit = ((CTankGame::Get().GetSettings()->FragLimit / 10) + 1) * 10;
		if(CTankGame::Get().GetSettings()->FragLimit > 60) CTankGame::Get().GetSettings()->FragLimit = 0;
		break;
	case BID_AITanks :
		if(e) CTankGame::Get().GetSettings()->AIAutoFillCount = e->QueryInt(ATT_EDIT_INT);
		break;
	case BID_AITanksToggle :
		CTankGame::Get().GetSettings()->AIAutoFillCount = (CTankGame::Get().GetSettings()->AIAutoFillCount & (~1)) + 2;
		if(CTankGame::Get().GetSettings()->AIAutoFillCount > 8) CTankGame::Get().GetSettings()->AIAutoFillCount = 2;
		break;
	case BID_Laps :
		if(e) CTankGame::Get().GetSettings()->Laps = e->QueryInt(ATT_EDIT_INT);
		break;
	case BID_LapsToggle :
		CTankGame::Get().GetSettings()->Laps++;
		if(CTankGame::Get().GetSettings()->Laps > 3) CTankGame::Get().GetSettings()->Laps = 1;
		break;
	case BID_Delay :
		if(e) CTankGame::Get().GetSettings()->StartDelay = e->QueryInt(ATT_EDIT_INT);
		break;
	case BID_DelayToggle :
		if(CTankGame::Get().GetSettings()->StartDelay == 5) CTankGame::Get().GetSettings()->StartDelay = 10;
		else if(CTankGame::Get().GetSettings()->StartDelay == 10) CTankGame::Get().GetSettings()->StartDelay = 20;
		else if(CTankGame::Get().GetSettings()->StartDelay == 20) CTankGame::Get().GetSettings()->StartDelay = 30;
		else if(CTankGame::Get().GetSettings()->StartDelay == 30) CTankGame::Get().GetSettings()->StartDelay = 60;
		else if(CTankGame::Get().GetSettings()->StartDelay == 60) CTankGame::Get().GetSettings()->StartDelay = 120;
		else CTankGame::Get().GetSettings()->StartDelay = 5;
		break;
	case BID_GameType :
		CTankGame::Get().GetSettings()->Deathmatch = !CTankGame::Get().GetSettings()->Deathmatch;
		if(CTankGame::Get().GetSettings()->Deathmatch == 0)
		{
			for(int i = 0; i < CTankGame::Get().GetNumMaps(); i++)
			{
				if(CmpLower(CTankGame::Get().GetSettings()->MapFile, CTankGame::Get().GetMap(i)->file) && CTankGame::Get().IsMapDMOnly(i))
					CTankGame::Get().GetSettings()->Deathmatch = 1;
			}
		}
		break;
	case BID_Mirror :
		CTankGame::Get().GetSettings()->MirroredMap = !CTankGame::Get().GetSettings()->MirroredMap;
		break;
	case BID_NetJoins :
		CTankGame::Get().GetSettings()->AllowSinglePlayerJoins = !CTankGame::Get().GetSettings()->AllowSinglePlayerJoins;
		break;
	case BID_LimitTypes :
		CTankGame::Get().GetSettings()->LimitTankTypes++;
		if(CTankGame::Get().GetSettings()->LimitTankTypes > 2) CTankGame::Get().GetSettings()->LimitTankTypes = 0;	//All, Steel, Liquid.
		break;
	case BID_Skill :	//20% increments.
		CTankGame::Get().GetSettings()->EnemySkill = (float)((int)(CTankGame::Get().GetSettings()->EnemySkill * 5.0f + 0.5f) + 1) * 0.2f;
		if(CTankGame::Get().GetSettings()->EnemySkill > 1.01f) CTankGame::Get().GetSettings()->EnemySkill = 0.0f;
		break;
	case BID_NumTeams:
		if(e)
		{
			CTankGame::Get().GetSettings()->NumTeams = CLAMP(e->QueryInt(ATT_EDIT_INT), 0, MIN(CTankGame::Get().GetNumAvailTeams(), CTankGame::Get().GetMap(iCurMap)->MaxMapTeams));
			if(CTankGame::Get().GetSettings()->NumTeams == 1)
			{
				if(MIN(CTankGame::Get().GetNumAvailTeams(), CTankGame::Get().GetMap(iCurMap)->MaxMapTeams) < 2)
					CTankGame::Get().GetSettings()->NumTeams = 0;
				else
					CTankGame::Get().GetSettings()->NumTeams = 2;
			}
		}
		break;
	case BID_TeamScores:
		CTankGame::Get().GetSettings()->TeamScores = !CTankGame::Get().GetSettings()->TeamScores;
		break;
	case BID_TeamDamage:
		CTankGame::Get().GetSettings()->TeamDamage = !CTankGame::Get().GetSettings()->TeamDamage;
		break;
	case BID_Race :
		RegistrySave();
		FreeGui();
		StartingGameDelay = 1500;
		CTankGame::Get().GetGameState()->AutoDrive = false;
		CTankGame::Get().GetGameState()->NumAITanks = 0;

		CTankGame::Get().GetSettings()->TeamPlay = CTankGame::Get().GetSettings()->NumTeams > 0;

		CurrentMenu = MP_StartingGame;
		FreeGui();

		return;
	case BID_Back :
		CurrentMenu = MP_TankSelect;
		FreeGui();
		return;
	case BID_Stats :
		CurrentMenu = MP_Stats;
		ReturnToMenu = MP_MapSelect;
		LoadStats();
		FreeGui();
		return;
	}
	//
	GUIHeader(Text.Get(TEXT_MAPHDR), "logobackground", true);
	//
	GuiEnt("guibutton", "title1", CVec3(.55, .09, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_MAPMAP), BID_None);
	e = GuiEnt("guilistbox", "listbox1", CVec3(.55, .14, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_Map);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 17);
		e->SetInt(ATT_LISTBOX_HEIGHT, 7);
		int sel = 0;
		int defmap = -1;
		int c = 0;
		for(int i = 0; i < CTankGame::Get().GetNumMaps(); i++){

			if(bTrainingOnly == (CTankGame::Get().GetMap(i)->maptype == MapTypeTraining))
			{
				if(defmap == -1)
					defmap = i;

				e->SetString(ATT_LISTBOX_NEWENTRY, CTankGame::Get().GetMap(i)->title);
				if(CTankGame::Get().GetMap(i)->file == CTankGame::Get().GetSettings()->MapFile){
					e->SetInt(ATT_LISTBOX_SELECTION, c);
					sel = 1;
					iCurMap = i;
				}

				c++;
			}
		}
		if(sel == 0)
		{
			iCurMap = (defmap == -1) ? 0 : defmap;
			CTankGame::Get().GetSettings()->MapFile = CTankGame::Get().GetMap( iCurMap = (defmap == -1) ? 0 : defmap )->file;
			e->SetInt(ATT_LISTBOX_SELECTION, 0);
		}
	}
	//
	e = GuiEnt("guiimagebrowser", "ibmap", CVec3(.05, .09, 0), CVec3(.40, .40, 0), CVec3(1, 1, 1), NULL, BID_None);
	if(e){
		if(MapImageFailureFile == CTankGame::Get().GetSettings()->MapFile || !e->SetString(ATT_BROWSER_SETIMAGE, FileNoExtension(CTankGame::Get().GetSettings()->MapFile) + "_shot.bmp")){
			e->SetString(ATT_BROWSER_SETIMAGE, "textures/ImageNotAvailable.bmp");
			MapImageFailureFile = CTankGame::Get().GetSettings()->MapFile;	//This prevents oscillations, which suck hard.
		}
		e->SetInt(ATT_BUTTON_XFLIP, CTankGame::Get().GetSettings()->MirroredMap);	//Flip image when mirrored selected.
	}
	//
	if (CTankGame::Get().GetMap(iCurMap)->GameType == -1)
	{
		if (CTankGame::Get().GetSettings()->Deathmatch) {
			GuiEnt("guibutton", "button1", CVec3(.05, .50, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
				Text.Get(TEXT_MAPTYPEBATTLE), BID_GameType, false, 0, 0);//FLAG_TEXT_CENTERX);
		} else {
			GuiEnt("guibutton", "button1", CVec3(.05, .50, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
				Text.Get(TEXT_MAPTYPERACE), BID_GameType, false, 0, 0);//FLAG_TEXT_CENTERX);
		}
	}
	else
		CTankGame::Get().GetSettings()->Deathmatch = CTankGame::Get().GetMap(iCurMap)->GameType;
	//
	if (CTankGame::Get().GetMap(iCurMap)->Laps == -1)
	{
		GuiEnt("guibutton", "button1", CVec3(.05, .55, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_MAPLAPS), BID_LapsToggle, false, 0, 0);
		e = GuiEnt("guiedit", "edit1", CVec3(.05 + (.025 * 13.0), .55, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			String(CTankGame::Get().GetSettings()->Laps), BID_Laps, false, 4, FLAG_EDIT_INT);
		e->SetInt(ATT_EDIT_MAXLENGTH, 2);
	}
	else
		CTankGame::Get().GetSettings()->Laps = CTankGame::Get().GetMap(iCurMap)->Laps;
	//
	if (CTankGame::Get().GetMap(iCurMap)->AITanks == -1)
	{
		GuiEnt("guibutton", "button1", CVec3(.05, .60, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_MAPAI), BID_AITanksToggle, false, 0, 0);
		e = GuiEnt("guiedit", "edit1", CVec3(.05 + (.025 * 13.0), .60, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			String(CTankGame::Get().GetSettings()->AIAutoFillCount), BID_AITanks, false, 4, FLAG_EDIT_INT);
		e->SetInt(ATT_EDIT_MAXLENGTH, 2);
	}
	else
		CTankGame::Get().GetSettings()->AIAutoFillCount = CTankGame::Get().GetMap(iCurMap)->AITanks;
	//
	if (CTankGame::Get().GetMap(iCurMap)->TimeLimit == -1)
	{
		GuiEnt("guibutton", "button1", CVec3(.05, .65, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_MAPTIME), BID_TimeLimitToggle, false, 0, 0);
		e = GuiEnt("guiedit", "edit1", CVec3(.05 + (.025 * 13.0), .65, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			String(CTankGame::Get().GetSettings()->TimeLimit), BID_TimeLimit, false, 4, FLAG_EDIT_INT);
		e->SetInt(ATT_EDIT_MAXLENGTH, 2);
	}
	else
		CTankGame::Get().GetSettings()->TimeLimit = CTankGame::Get().GetMap(iCurMap)->TimeLimit;
	//
	if (CTankGame::Get().GetMap(iCurMap)->FragLimit == -1)
	{
		GuiEnt("guibutton", "button1", CVec3(.05, .70, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_MAPFRAG), BID_FragLimitToggle, false, 0, 0);
		e = GuiEnt("guiedit", "edit1", CVec3(.05 + (.025 * 13.0), .70, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			String(CTankGame::Get().GetSettings()->FragLimit), BID_FragLimit, false, 4, FLAG_EDIT_INT);
		e->SetInt(ATT_EDIT_MAXLENGTH, 2);
	}
	else
		CTankGame::Get().GetSettings()->FragLimit = CTankGame::Get().GetMap(iCurMap)->FragLimit;
	//
	if (CTankGame::Get().GetMap(iCurMap)->StartDelay == -1)
	{
		GuiEnt("guibutton", "button1", CVec3(.05, .75, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_MAPDELAY), BID_DelayToggle, false, 0, 0);
		e = GuiEnt("guiedit", "edit1", CVec3(.05 + (.025 * 13.0), .75, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			String(CTankGame::Get().GetSettings()->StartDelay), BID_Delay, false, 4, FLAG_EDIT_INT);
		e->SetInt(ATT_EDIT_MAXLENGTH, 2);
	}
	else
		CTankGame::Get().GetSettings()->StartDelay = CTankGame::Get().GetMap(iCurMap)->StartDelay;
	//
	if (CTankGame::Get().GetMap(iCurMap)->EnemySkill == -1.0f)
	{
		sprintf(tempstr, Text.Get(TEXT_MAPSKILL), (int)(CTankGame::Get().GetSettings()->EnemySkill * 100.5f) );
		GuiEnt("guibutton", "button1", CVec3(.05, .80, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			tempstr, BID_Skill, false, 0, 0);
	}
	else
		CTankGame::Get().GetSettings()->EnemySkill = CTankGame::Get().GetMap(iCurMap)->EnemySkill;
	//
	if (CTankGame::Get().GetMap(iCurMap)->TankTypes == -1)
	{
		if (CTankGame::Get().GetSettings()->LimitTankTypes == 0) {
			GuiEnt("guibutton", "button1", CVec3(.55, .50, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
				Text.Get(TEXT_MAPTANKALL), BID_LimitTypes, false, 0, 0);
		} else if (CTankGame::Get().GetSettings()->LimitTankTypes == 1) {
			GuiEnt("guibutton", "button1", CVec3(.55, .50, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
				Text.Get(TEXT_MAPTANKSTEEL), BID_LimitTypes, false, 0, 0);
		} else {
			GuiEnt("guibutton", "button1", CVec3(.55, .50, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
				Text.Get(TEXT_MAPTANKLIQUID), BID_LimitTypes, false, 0, 0);
		}
	}
	else
		CTankGame::Get().GetSettings()->LimitTankTypes = CTankGame::Get().GetMap(iCurMap)->TankTypes;
	//
	if (CTankGame::Get().GetMap(iCurMap)->Mirror == -1)
	{
		sprintf(tempstr, Text.Get(TEXT_MAPMIRROR), CTankGame::Get().GetSettings()->MirroredMap ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO) );
		GuiEnt("guibutton", "button1", CVec3(.55, .55, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			tempstr, BID_Mirror, false, 0, 0);
	}
	else
		CTankGame::Get().GetSettings()->MirroredMap = (CTankGame::Get().GetMap(iCurMap)->Mirror != 0);

	if (CTankGame::Get().GetMap(iCurMap)->AllowJoins == -1)
	{
		sprintf(tempstr, Text.Get(TEXT_MAPJOINS), CTankGame::Get().GetSettings()->AllowSinglePlayerJoins ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO) );
		GuiEnt("guibutton", "button1", CVec3(.55, .60, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			tempstr, BID_NetJoins, false, 0, 0);
	}
	else
		CTankGame::Get().GetSettings()->AllowSinglePlayerJoins = CTankGame::Get().GetMap(iCurMap)->AllowJoins;




	if (CTankGame::Get().GetMap(iCurMap)->NumTeams == -1)
	{
		GuiEnt("guibutton", "button1", CVec3(.55, .65, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			Text.Get(TEXT_NUMTEAMS), BID_NumTeams, false, 0, 0);
		e = GuiEnt("guiedit", "edit1", CVec3(.55 + (.025 * 13.0), .65, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			String(CTankGame::Get().GetSettings()->NumTeams), BID_NumTeams, false, 4, FLAG_EDIT_INT);
		e->SetInt(ATT_EDIT_MAXLENGTH, 2);
	}
	else
		CTankGame::Get().GetSettings()->NumTeams = CTankGame::Get().GetMap(iCurMap)->NumTeams;

	if (CTankGame::Get().GetMap(iCurMap)->TeamScores == -1)
	{
		sprintf(tempstr, Text.Get(TEXT_TEAMSCORES), CTankGame::Get().GetSettings()->TeamScores ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO) );
		GuiEnt("guibutton", "button1", CVec3(.55, .70, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			tempstr, BID_TeamScores, false, 0, 0);
	}
	else
		CTankGame::Get().GetSettings()->TeamScores = CTankGame::Get().GetMap(iCurMap)->TeamScores;

	if (CTankGame::Get().GetMap(iCurMap)->TeamDamage == -1)
	{
		sprintf(tempstr, Text.Get(TEXT_TEAMDAMAGE), CTankGame::Get().GetSettings()->TeamDamage ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO) );
		GuiEnt("guibutton", "button1", CVec3(.55, .75, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
			tempstr, BID_TeamDamage, false, 0, 0);
	}
	else
		CTankGame::Get().GetSettings()->TeamDamage = CTankGame::Get().GetMap(iCurMap)->TeamDamage;
	//
	//
	GuiEnt("guibutton", "button1", CVec3(0.75, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		(CTankGame::Get().GetSettings()->Deathmatch ? Text.Get(TEXT_BATTLE) : Text.Get(TEXT_RACE)), BID_Race, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.25, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	//
	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_VIEW), BID_Stats, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.5, .94, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_STATS), BID_Stats, false, 0, FLAG_TEXT_CENTERX);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Server Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoServerSelect()
{
	EntityBase *e;
	CStr CurName, CurWeb, CurInfo, CurDetail;
	//
	if(MasterRefreshWanted && CTankGame::Get().GetMasterNet()->ConnectionStatus(CLIENTID_SERVER) == CCS_Connected){
		char s[2] = {'/', 'z'};
		CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, s, 2);
		MasterRefreshWanted = false;
	}
	//

	//
	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Server);
		return;
	case BID_Race :
		if(CTankGame::Get().GetSettings()->ServerAddress.len() < 5) break;
		CTankGame::Get().GetGameState()->MasterPingIter = -1;	//We really should disconnect from master totally when starting a game.
		RegistrySave();
		FreeGui();
		CurrentMenu = MP_StartingGame;//MP_None;
		StartingGameDelay = 1500;
		CTankGame::Get().GetGameState()->AutoDrive = false;
		return;
	case BID_Back :
		if(CTankGame::Get().IsGameRunning()){
			CurrentMenu = MP_InGameMain;
		}else{
			CurrentMenu = MP_TankSelect;
		}
		FreeGui();
		return;
	case BID_RefreshPings :
		{
			CTankGame::Get().GetGameState()->MasterPingIter = 0;
			for(ServerEntryEx *se = (ServerEntryEx*)CTankGame::Get().GetServerHead()->NextLink(); se; se = (ServerEntryEx*)se->NextLink()){
				se->PingTime = 0;
				se->PingCount = 0;
			}
		}
		break;
	case BID_RefreshLAN :
		{
			CTankGame::Get().GetGameState()->MasterPingIter = 0;
			CTankGame::Get().GetServerHead()->DeleteList();	//Clear out old servers.
			BitPacker<16> pe;
			pe.PackUInt(LONG_HEAD_HEARTREQ, 32);	//Request heartbeats from LAN servers.
			sockaddr_in to;
			to.sin_family = AF_INET;
			to.sin_addr.s_addr = htonl(INADDR_BROADCAST);
			for(int i = -10; i <= 10; i++){	//Broadcast to 21 port radius.
				to.sin_port = htons(CTankGame::Get().GetSettings()->ServerPort + i);
				if(CTankGame::Get().GetMasterNet()->SendOutOfBandPacket(&to, (char*)pe.Data(), pe.BytesUsed())){
					OutputDebugLog("Sent HeartReq to " + CStr(inet_ntoa(to.sin_addr)) + ":" + String(ntohs(to.sin_port)) + "\n");
				}
			}
			AreLanServers = true;
		}
		break;
	case BID_Chat :
		if(CTankGame::Get().GetMasterNet()->ConnectionStatus(CLIENTID_SERVER) == CCS_Connected){
			if(FirstChat){
				CStr t;
				// TODO: how does IRC handle this internationally?
				t = "/help";
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, t.get(), t.len());
				t = "/name " + CTankGame::Get().GetSettings()->PlayerName;
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, t.get(), t.len());
				t = "/join Main";
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, t.get(), t.len());
				t = "/chan";
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, t.get(), t.len());
				t = "/users";
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, t.get(), t.len());
				FirstChat = false;
			}
			CurrentMenu = MP_Chat;
			MsgTo = "";
			FreeGui();
			return;
		}
	//	FirstChat = true;
		//else, fall through and connect to master...
	case BID_FirstTimeOnMenu :
	case BID_RefreshMaster :
		RefreshMaster = 1;
		break;
	case BID_StopPings :
		CTankGame::Get().GetGameState()->MasterPingIter = -1;
		break;
	case BID_Address :
		if(e){
			CTankGame::Get().GetSettings()->ServerAddress = e->QueryString(ATT_TEXT_STRING);
		}
		break;
	case BID_Master :
		if(e){
			OutputDebugLog("Master address changed.\n\n");
			CStr t = e->QueryString(ATT_TEXT_STRING);
			if(t != CTankGame::Get().GetSettings()->MasterAddress){
				CTankGame::Get().GetMasterNet()->ClientDisconnect();
				OutputDebugLog("Disconnecting Master.\n\n");
			}
			CTankGame::Get().GetSettings()->MasterAddress = t;
		}
		break;
	case BID_Server :
		if(e){
			int i = e->QueryInt(ATT_LISTBOX_SELECTION);
			ServerEntry *se = CTankGame::Get().GetServerHead()->FindItem(i + 1);
			if(se) CTankGame::Get().GetSettings()->ServerAddress = se->AddressText();
		}
		break;
	case BID_SortName : CTankGame::Get().GetGameState()->SortMode = MSM_Name; break;
	case BID_SortPing : CTankGame::Get().GetGameState()->SortMode = MSM_Ping; break;
	case BID_SortPlayers : CTankGame::Get().GetGameState()->SortMode = MSM_Players; break;
	case BID_SortMap : CTankGame::Get().GetGameState()->SortMode = MSM_Map; break;
	case BID_SortTime : CTankGame::Get().GetGameState()->SortMode = MSM_Time; break;
	case BID_SortMode : CTankGame::Get().GetGameState()->SortMode = MSM_Mode; break;
	}
	//
	if(RefreshMaster){
		if(CTankGame::Get().GetMasterNet()->ConnectionStatus(CLIENTID_SERVER) != CCS_Connected){
			CTankGame::Get().GetMasterNet()->ClientDisconnect();
			CTankGame::Get().GetMasterNet()->ClientConnect(CTankGame::Get().GetSettings()->MasterAddress, CTankGame::Get().GetSettings()->MasterPort);
			//Begin the connection process.
			FirstChat = true;
		}
		MasterRefreshWanted = true;
		AreLanServers = false;
		CTankGame::Get().GetServerHead()->DeleteList();	//Clear out old servers.
		//
		RefreshMaster = 0;
	}
	GUIHeader("Join a Multi-Player Game", "logobackground", true);
	//
	e = GuiEnt("guiedit", "edit1", CVec3(.05, .09, 0), CVec3(.02, .05, 0), CVec3(1, 1, 1),
		CTankGame::Get().GetSettings()->MasterAddress, BID_Master, false, 45, 0);
	if(e) e->SetString(ATT_EDIT_HEADER, "Master Address: ");
	{
		CStr MasterStatus;
		if(CTankGame::Get().GetGameState()->MasterError.len() <= 0) CTankGame::Get().GetGameState()->MasterError = "Not Connected";
		switch(CTankGame::Get().GetMasterNet()->ConnectionStatus(CLIENTID_SERVER)){
		case CCS_OutRequestConnect : MasterStatus = "Trying to Connect..."; break;
		case CCS_OutReplyChallenge : MasterStatus = "Sending Challenge Reply"; break;
		case CCS_Connected : MasterStatus = "Connected to Master Server"; break;
		case CCS_None :
		default :
			MasterStatus = CTankGame::Get().GetGameState()->MasterError; break;
		}
		e = GuiEnt("guibutton", "title1", CVec3(.05, .14, 0), CVec3(.02, .05, 0), CVec3(1, 1, 1),
			PadString("Status: " + MasterStatus, 45), BID_None);//RefreshMaster);
	}
	GuiEnt("guibutton", "title1", CVec3(.05, .19, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		PadString((AreLanServers ? "LAN Games (" : "Internet Servers (") +
		String(CTankGame::Get().GetServerHead()->CountItems(-1)) + ")", 36), BID_None);
	//
	GuiEnt("guibutton", "button1", CVec3(.05 + .015 * 0.0, .24, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		"Name", BID_SortName, false, 0, (CTankGame::Get().GetGameState()->SortMode == MSM_Name ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.05 + .015 * 20.0, .24, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		"Map", BID_SortMap, false, 0, (CTankGame::Get().GetGameState()->SortMode == MSM_Map ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.05 + .015 * 37.0, .24, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		"Tanks", BID_SortPlayers, false, 0, (CTankGame::Get().GetGameState()->SortMode == MSM_Players ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.05 + .015 * 43.0, .24, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		"Mode", BID_SortMode, false, 0, (CTankGame::Get().GetGameState()->SortMode == MSM_Mode ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.05 + .015 * 48.0, .24, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		"Time", BID_SortTime, false, 0, (CTankGame::Get().GetGameState()->SortMode == MSM_Time ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(.05 + .015 * 55.0, .24, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		"Ping", BID_SortPing, false, 0, (CTankGame::Get().GetGameState()->SortMode == MSM_Ping ? FLAG_BUTTON_DISABLED : 0));
	//
	e = GuiEnt("guilistbox", "listbox2", CVec3(.05, .27, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1), 0, BID_Server);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 60);
		e->SetInt(ATT_LISTBOX_HEIGHT, 9);
		int i = 0;
		for(ServerEntryEx *se = (ServerEntryEx*)CTankGame::Get().GetServerHead()->NextLink(); se; se = (ServerEntryEx*)se->NextLink()){
			e->SetString(ATT_LISTBOX_NEWENTRY,
				PadString(se->Name, 19) + " " +
				PadString(se->MapTitle, 17) +
				PadString(String(se->Clients) + "/" + String(se->MaxClients), 6) +
				PadString(se->GameMode & GAMEMODE_RACE ? "Race" : (se->GameMode & GAMEMODE_CTF ? "CTF" : "DM"), 5) +
				PadString(CTankGame::Get().MMSS(se->TimeInGame / 1000), 7) +
				PadString(String(se->PingTime / MAX(1, se->PingCount)), 4) );
			if((GameVersion.v[0] != se->version[0]) || (GameVersion.v[1] != se->version[1]) ){
				e->SetVec(ATT_LISTBOX_ENTRYCOLOR, CVec3(0.75f, 0.75f, 0.75f));
			}
			if(se->AddressText() == CTankGame::Get().GetSettings()->ServerAddress){
				e->SetInt(ATT_LISTBOX_SELECTION, i);
				CurName = se->Name;
				CurInfo = se->Info;
				CurWeb = se->WebSite;
				CurDetail = "Ver: " + String((int)se->version[0]) + "." + String((int)se->version[1]) +
					"." + String((int)se->version[2]) +
					(se->version[0] == 0 ? "-TEST" : (se->version[3] == 0 ? "-DEMO" : "-FULL")) +
					" Bots: " + String(se->Bots) + " TL: " + String(se->TimeLimit) +
					" FL: " + String(se->FragLimit) + " FPS: " + String(se->DediFPS) +
					" Team: " + (se->GameMode & GAMEMODE_TEAMPLAY ? "Y" : "N") +
					(se->GameMode & GAMEMODE_TEAMDAMAGE ? "+d" : "") +
					(se->GameMode & GAMEMODE_TEAMSCORES ? "+s" : "");
			}
			i++;
		}
	}
	e = GuiEnt("guiedit", "edit1", CVec3(.05, .54, 0), CVec3(.02, .05, 0), CVec3(1, 1, 1),
		CTankGame::Get().GetSettings()->ServerAddress, BID_Address, false, 45, 0);
	if(e) e->SetString(ATT_EDIT_HEADER, "Connect To (TCP/IP): ");
	e = GuiEnt("guibutton", "title1", CVec3(.05, .59, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		PadString("Name: " + CurName, 60), BID_None);
	e = GuiEnt("guibutton", "title1", CVec3(.05, .62, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		PadString(CurDetail, 60), BID_None);
	e = GuiEnt("guibutton", "title1", CVec3(.05, .65, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		PadString("Info: " + CurInfo, 60), BID_None);
	e = GuiEnt("guibutton", "title1", CVec3(.05, .68, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		PadString(Mid(CurInfo, 54), 60), BID_None);
	e = GuiEnt("guibutton", "title1", CVec3(.05, .71, 0), CVec3(.015, .03, 0), CVec3(1, 1, 1),
		PadString("Web: " + CurWeb, 60), BID_None);
	GuiEnt("guibutton", "button1", CVec3(.25, .76, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		"Refresh LAN", BID_RefreshLAN, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.75, .76, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		"Refresh Internet", BID_RefreshMaster, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.25, .82, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		"Re-Ping Servers", BID_RefreshPings, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.75, .82, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		"Stop Pinging", BID_StopPings, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.1666, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		"Chat!", BID_Chat, false, 0, FLAG_TEXT_CENTERX |
		(CTankGame::Get().GetMasterNet()->ConnectionStatus(CLIENTID_SERVER) != CCS_Connected ? FLAG_BUTTON_DISABLED : 0));
	GuiEnt("guibutton", "button1", CVec3(0.8444, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		"Start", BID_Race, false, 0, FLAG_TEXT_CENTERX |
		(CTankGame::Get().GetSettings()->ServerAddress.len() < 5 ? FLAG_BUTTON_DISABLED : 0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Chat Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoChat()
{
	EntityBase *e;
	//
	if(CTankGame::Get().GetMasterNet()->ConnectionStatus(CLIENTID_SERVER) != CCS_Connected){	//If we're not connected to master, we shouldn't be here.
		CurrentMenu = MP_ServerSelect;
		FreeGui();
		return;
	}
	//
	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Chat);
		return;
	case BID_ChatEdit :
		if(e){
			*CTankGame::Get().GetChatEdit() = e->QueryString(ATT_TEXT_STRING);
			if(e->QueryInt(ATT_EDIT_ENTER)){
				if(MsgTo.len() > 0){	//Send as a private message.
					*CTankGame::Get().GetChatEdit() = "/m " + MsgTo + " " + *CTankGame::Get().GetChatEdit();
					CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, CTankGame::Get().GetChatEdit()->get(), CTankGame::Get().GetChatEdit()->len() + 1);
					MsgTo = "";
				}else{
					CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, CTankGame::Get().GetChatEdit()->get(), CTankGame::Get().GetChatEdit()->len() + 1);
					if(CmpLower(Left(*CTankGame::Get().GetChatEdit(), 2), "/j")){
						CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, "/c", 3);	//Tack a chan list req on to all channel changes.
						CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, "/u", 3);	//Tack a user list req on to all channel changes.
					}
				}
				*CTankGame::Get().GetChatEdit() = "";
				e->SetString(ATT_TEXT_STRING, *CTankGame::Get().GetChatEdit());
			}
		}
		break;
	case BID_Channels :
		CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, "/c", 3);
		break;
	case BID_Users :
		CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, "/u", 3);
		MsgTo = "";
		break;
	case BID_ChanList :
		if(e){
			int i = e->QueryInt(ATT_LISTBOX_SELECTION);
			CStrLink *c = CTankGame::Get().GetChannelHead()->FindItem(i + 1);
			if(c && c->len() > 0){
				CStr t = "/j " + *c;
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, t.get(), t.len() + 1);
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, "/c", 3);	//Tack a chan list req on to all channel changes.
				CTankGame::Get().GetMasterNet()->QueueSendServer(TM_Ordered, "/u", 3);	//Tack a user list req on to all channel changes.
			}
		}
		break;
	case BID_UserList :
		if(e){
			int i = e->QueryInt(ATT_LISTBOX_SELECTION);
			CStrLink *c = CTankGame::Get().GetUserHead()->FindItem(i + 1);
			if(i >= 0 && c && c->len() > 0){// && ChatEdit.len() > 0){
				MsgTo = *c;
			}else{
				MsgTo = "";
			}
		}
		break;
	case BID_Back :
		CurrentMenu = MP_ServerSelect;
		FreeGui();
		return;
	}
	//
	GUIHeader("Tread Marks Chat", "logobackground", true);
	//
	GuiEnt("guibutton", "title1", CVec3(.00, .10, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		PadString("Chatting on channel: " + *CTankGame::Get().GetCurChannel(), 40), BID_None);
	e = GuiEnt("guilistbox", "chatbox", CVec3(.00, .15, 0), CVec3(.01666, .03, 0), CVec3(1, 1, 1), 0, BID_None);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, CTankGame::Get().GetCurChatLineLen() + 1);
		e->SetInt(ATT_LISTBOX_HEIGHT, 13);
		int i = CTankGame::Get().GetCurChatLine() + 1;
		for(int n = 0; n < MaxChatLines; n++){
			if(i >= MaxChatLines) i = 0;
			e->SetString(ATT_LISTBOX_NEWENTRY, *CTankGame::Get().GetChatLine(i));
			if(CTankGame::Get().GetChatLine(i)->chr(0) == '[' || Left(*CTankGame::Get().GetChatLine(i), 8) == "Msg from" || CTankGame::Get().GetChatLine(i)->chr(0) == ' '){
				//Chat text.
			}else{	//System message.
				e->SetVec(ATT_LISTBOX_ENTRYCOLOR, CVec3(0.0f, 1.0f, 1.0f));
			}
			i++;
		}
		if(e->QueryInt(ATT_LISTBOX_SELECTION) <= 0){
			e->SetInt(ATT_LISTBOX_SELECTION, MaxChatLines - 1);
		}
	}
	e = GuiEnt("guiedit", "edit1", CVec3(.00, .55, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
		*CTankGame::Get().GetChatEdit(), BID_ChatEdit, false, 50, 0);
	if(e) e->SetString(ATT_EDIT_HEADER, "> ");
	if(e) CTankGame::Get().GetVW()->SetFocus(e->GID);	//Since it's the only edit entity, force focus back to it always.
	//
	GuiEnt("guibutton", "button1", CVec3(.05, .60, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
		PadString("Channels (" + String(CTankGame::Get().GetChannelHead()->CountItems(-1)) + ")", 20), BID_Channels);
	e = GuiEnt("guilistbox", "listbox2", CVec3(.05, .65, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1), 0, BID_ChanList);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 20);
		e->SetInt(ATT_LISTBOX_HEIGHT, 5);
		for(CStrLink *c = CTankGame::Get().GetChannelHead()->NextLink(); c; c = c->NextLink()){
			e->SetString(ATT_LISTBOX_NEWENTRY, *c);
		}
	}
	//
	GuiEnt("guibutton", "button1", CVec3(.55, .60, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1),
		PadString("Users ("  + String(CTankGame::Get().GetUserHead()->CountItems(-1)) + ")", 20), BID_Users);
	e = GuiEnt("guilistbox", "listbox2", CVec3(.55, .65, 0), CVec3(.02, .04, 0), CVec3(1, 1, 1), 0, BID_UserList);
	if(e){
		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 20);
		e->SetInt(ATT_LISTBOX_HEIGHT, 5);
		int i = 0, sel = 0;
		for(CStrLink *c = CTankGame::Get().GetUserHead()->NextLink(); c; c = c->NextLink()){
			e->SetString(ATT_LISTBOX_NEWENTRY, *c);
			if(CmpLower(*c, MsgTo)){
				e->SetInt(ATT_LISTBOX_SELECTION, i);
				sel = 1;
			}
			i++;
		}
		if(sel == 0) MsgTo = "";	//Couldn't find MsgTo in list, so disengage.
		if(MsgTo.len() <= 0) e->SetInt(ATT_LISTBOX_SELECTION, -1);
	}
	//
	if(e) e->SetString(ATT_EDIT_HEADER, "Chat: ");
	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
}
// RUSS - translated to here...
void DoGraphicsOptions()
{
	char tempstr[4096];
	Vec3 V, L, S;

	GUIHeader(Text.Get(TEXT_GRAPHICSHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_APPLY), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	SetVec3(.05, .10, 0, V);
	SetVec3(0, .055, 0, L);
	SetVec3(.02, .05, 0, S);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSRES), CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH, CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT);
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Res);  AddVec3(L, V);

	if (CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen) {
		sprintf(tempstr, Text.Get(TEXT_GRAPHICSFULL));
	} else {
		sprintf(tempstr, Text.Get(TEXT_GRAPHICSWINDOW));
	}
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Mode);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSTEXRES), CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes);
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_TexRes);  AddVec3(L, V);
/*
	sprintf(tempstr, Text.Get(TEXT_GRAPHICSS3TC), TexCompress ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_TexCompress);  AddVec3(L, V);
*/
	sprintf(tempstr, Text.Get(TEXT_GRAPHICSTRILINEAR), CTankGame::Get().GetSettings()->GraphicsSettings.Trilinear ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Trilinear);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSFOG), CTankGame::Get().GetSettings()->GraphicsSettings.UseFog ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Fog);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSDUST), (int)(CTankGame::Get().GetSettings()->GraphicsSettings.Particles * 100.0f + 0.5f));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Dust);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSFIRE), CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Fire);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSVENDOR), (char*)glGetString(GL_VENDOR));
	GuiEnt("guibutton", "title1", V, CVec3(.015, .03, 0), CVec3(1, 1, 1),
		tempstr, BID_None);  AddVec3(CVec3(0, .03, 0), V);

	GuiEnt("guibutton", "title1", V, CVec3(.015, .03, 0), CVec3(1, 1, 1),
		CStr((char*)glGetString(GL_RENDERER)) + " " +
		CStr((char*)glGetString(GL_VERSION)), BID_None);  AddVec3(CVec3(0, .03, 0), V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSTEXELS), CTankGame::Get().GetVW()->Texels + CTankGame::Get().GetVW()->Map.Width() * CTankGame::Get().GetVW()->Map.Height());
	GuiEnt("guibutton", "title1", V, CVec3(.015, .03, 0), CVec3(1, 1, 1),
		tempstr, BID_None);  AddVec3(CVec3(0, .03, 0), V);

	SetVec3(.52, .10, 0, V);
	sprintf(tempstr, Text.Get(TEXT_GRAPHICSTRIS), (int)(CTankGame::Get().GetSettings()->GraphicsSettings.Quality * 10000.0f + 0.5f));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Quality);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSMESHES), CTankGame::Get().GetSettings()->GraphicsSettings.PolyLod <= 0 ? Text.Get(TEXT_HIGH) : Text.Get(TEXT_LOW));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_MeshRes);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSDETAIL), CTankGame::Get().GetSettings()->GraphicsSettings.DetailTerrain ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Detail);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSVIEWDIST), (int)(CTankGame::Get().GetSettings()->GraphicsSettings.ViewDistance + 0.5f));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_ViewDist);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_GRAPHICSTREADMARKS), CTankGame::Get().GetSettings()->GraphicsSettings.TreadMarks ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Treadmarks);  AddVec3(L, V);

	switch(Activated()){
	case BID_Help :
		StartHelp(HELP_Options);
		return;
	case BID_Back :
		CurrentMenu = MP_Options;
		FreeGui();
		if(VideoSwitchNeeded) CTankGame::Get().GetGameState()->SwitchMode = true;
		VideoSwitchNeeded = SoundSwitchNeeded = MusicSwitchNeeded = false;// = ClientInfoSendNeeded = false;
		RegistrySaveNeeded = true;
		return;
	case BID_Fire :
		CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire = !CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire;
		break;
	case BID_Res :
		VideoSwitchNeeded = true;

		{
			auto modeDesktop = sf::VideoMode::getDesktopMode();
			std::set<std::pair<unsigned int, unsigned int>> aResolutions;

			// Put the resolutions in a set so they're sorted.
			for(auto it = sf::VideoMode::getFullscreenModes().begin(); it != sf::VideoMode::getFullscreenModes().end(); ++it)
			{
				if(it->bitsPerPixel == modeDesktop.bitsPerPixel) // To keep things simple, we'll throw out modes that don't match our current BPP
				{
					aResolutions.insert(std::make_pair(it->width, it->height));
				}
			}

			bool bFound = false;
			
			for(auto it = aResolutions.begin(); it != aResolutions.end(); ++it)
			{
				if(it->first == CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH && it->second == CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT)
				{
					// Set the resolution to the next in the list, if there is one.
					++it;
					if(it != aResolutions.end())
					{
						CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH = it->first;
						CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT = it->second;
						bFound = true;
					}
					break;
				}
			}

			if(!bFound)
			{
				CTankGame::Get().GetSettings()->GraphicsSettings.GLBWIDTH = aResolutions.begin()->first;
				CTankGame::Get().GetSettings()->GraphicsSettings.GLBHEIGHT = aResolutions.begin()->second;
			}
		}

		break;
	case BID_Mode :
		VideoSwitchNeeded = true;
		CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen = !CTankGame::Get().GetSettings()->GraphicsSettings.UseFullScreen;
		break;
	case BID_TexRes :
		VideoSwitchNeeded = true;
		CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes = (CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes * 2 > 512 ? 128 : CTankGame::Get().GetSettings()->GraphicsSettings.MaxTexRes * 2);
		break;
	case BID_Trilinear :
		VideoSwitchNeeded = true;
		CTankGame::Get().GetSettings()->GraphicsSettings.Trilinear = !CTankGame::Get().GetSettings()->GraphicsSettings.Trilinear;
		break;
/*	case BID_TexCompress :
		TexCompress = !TexCompress;
		if(TexCompressAvail){
			VideoSwitchNeeded = true;
		}else{
			TexCompress = false;	//Not available.
		}
		break;
*/	case BID_Fog :
		CTankGame::Get().GetSettings()->GraphicsSettings.UseFog = !CTankGame::Get().GetSettings()->GraphicsSettings.UseFog;
		break;
	case BID_MeshRes :
		CTankGame::Get().GetSettings()->GraphicsSettings.PolyLod = !CTankGame::Get().GetSettings()->GraphicsSettings.PolyLod;
		break;
	case BID_Dust :
		CTankGame::Get().GetSettings()->GraphicsSettings.Particles += 0.25f;
		if(CTankGame::Get().GetSettings()->GraphicsSettings.Particles > 1.0001f) CTankGame::Get().GetSettings()->GraphicsSettings.Particles = 0.25f;
		break;
	case BID_Quality :
		int intq; intq = (int)(CTankGame::Get().GetSettings()->GraphicsSettings.Quality * 10.0f + 0.5f) + 1;
		if(intq > 10) intq = 1;
		CTankGame::Get().GetSettings()->GraphicsSettings.Quality = (float)intq * 0.1f;
		break;
	case BID_Detail :
		CTankGame::Get().GetSettings()->GraphicsSettings.DetailTerrain = !CTankGame::Get().GetSettings()->GraphicsSettings.DetailTerrain;
		break;
	case BID_ViewDist :
		int intv; intv = (int)(CTankGame::Get().GetSettings()->GraphicsSettings.ViewDistance + 0.5f);
		intv += 250; if(intv > 1000) intv = 250;
		CTankGame::Get().GetSettings()->GraphicsSettings.ViewDistance = (float)intv;
		break;
	case BID_Treadmarks :
		CTankGame::Get().GetSettings()->GraphicsSettings.TreadMarks = !CTankGame::Get().GetSettings()->GraphicsSettings.TreadMarks;
		break;
	}
}

void DoSoundMusicOptions()
{
	char tempstr[256];
	Vec3 V, L, S;

	GUIHeader(Text.Get(TEXT_SOUNDHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_APPLY), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	SetVec3(.05, .10, 0, V);
	SetVec3(0, .055, 0, L);
	SetVec3(.02, .05, 0, S);

	sprintf(tempstr, Text.Get(TEXT_SOUNDMUSIC), CTankGame::Get().GetSettings()->PlayMusic ? Text.Get(TEXT_ENABLED) : Text.Get(TEXT_DISABLED));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Music);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_SOUNDMUSICVOLUME), (int)(CTankGame::Get().GetSettings()->MusicVol * 100.0f));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_MusicVol);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_SOUNDQUALITY), CTankGame::Get().GetSettings()->HiFiSound ? Text.Get(TEXT_HIGH) : Text.Get(TEXT_LOW));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_SoundQuality);  AddVec3(L, V);

	sprintf(tempstr, Text.Get(TEXT_SOUNDSOUNDVOLUME), (int)(CTankGame::Get().GetSettings()->SoundVol * 100.0f));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_SoundVol);  AddVec3(L, V);

	switch(Activated()){
	case BID_Help :
		StartHelp(HELP_Options);
		return;
	case BID_Back :
		CurrentMenu = MP_Options;
		FreeGui();
		if(SoundSwitchNeeded) CTankGame::Get().GetGameState()->SwitchSoundMode = true;
		if(MusicSwitchNeeded)
		{
			if(CTankGame::Get().GetSettings()->PlayMusic)
			{
				CTankGame::Get().NewMusic();
				CTankGame::Get().GetVW()->snd.StartMusic(CTankGame::Get().GetGameState()->sMusicFile);
			}else{
				CTankGame::Get().GetVW()->snd.StopMusic();
			}
		}
		VideoSwitchNeeded = CTankGame::Get().GetGameState()->SwitchSoundMode = MusicSwitchNeeded = false;// = ClientInfoSendNeeded = false;
		RegistrySaveNeeded = true;
		return;
		break;
	case BID_SoundQuality :
		SoundSwitchNeeded = true;
		CTankGame::Get().GetSettings()->HiFiSound = !CTankGame::Get().GetSettings()->HiFiSound;
		break;
	case BID_Music :
		MusicSwitchNeeded = true;
		CTankGame::Get().GetSettings()->PlayMusic = !CTankGame::Get().GetSettings()->PlayMusic;
		break;
	case BID_SoundVol:
		CTankGame::Get().GetSettings()->SoundVol += 0.1f;
		if(CTankGame::Get().GetSettings()->SoundVol > 1.05f)
			CTankGame::Get().GetSettings()->SoundVol = 0.1f;
		CTankGame::Get().GetVW()->snd.SetSoundVolume(CTankGame::Get().GetSettings()->SoundVol);
		break;
	case BID_MusicVol:
		CTankGame::Get().GetSettings()->MusicVol += 0.1f;
		if(CTankGame::Get().GetSettings()->MusicVol > 1.05f)
			CTankGame::Get().GetSettings()->MusicVol = 0.1f;
		CTankGame::Get().GetVW()->snd.SetMusicVolume(CTankGame::Get().GetSettings()->MusicVol);
		break;
	}
}
void DoTankCtlCfg()
{
	int i;
	GUIHeader(Text.Get(TEXT_TANKHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_APPLY), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "title1", CVec3(0.5, .82, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		CStr(EditingControl ? "Press a key or button (ESC to cancel)" : "                                     "), BID_None, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "title1", CVec3(0.05, 0.1, 0), CVec3(.02, .05, 0), CVec3(1, 1, 1),
		PadString("Control:", 13) + PadString("Primary", 11) + PadString("Secondary", 11) + PadString("Tertiary", 10), BID_None);
	for(i = 0; i < CTankGame::Get().GetSettings()->InputSettings.FirstCamControl; i++){
		double y = 0.1 + (0.05 * (double)(i + 1));
		CVec3 col(1, 1, 1);
		if(EditingControl > 0 && (EditingControl - BID_TankCtlCfg) / 10 == i){	//Hilight currently active control.
			SetVec3(1, 1, 0, col);
		}
		GuiEnt("guibutton", "title1", CVec3(0.05, y, 0), CVec3(.02, .05, 0), col,
			PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[i].name, 12), BID_None);
		GuiEnt("guibutton", "button1", CVec3(0.05 + 0.24, y, 0), CVec3(.02, .05, 0), col,
			"[" + PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl1, 9) + "]", BID_TankCtlCfg + i * 10 + 0);
		GuiEnt("guibutton", "button1", CVec3(0.05 + 0.46, y, 0), CVec3(.02, .05, 0), col,
			"[" + PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl2, 9) + "]", BID_TankCtlCfg + i * 10 + 1);
		GuiEnt("guibutton", "button1", CVec3(0.05 + 0.68, y, 0), CVec3(.02, .05, 0), col,
			"[" + PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[i].ctrl3, 9) + "]", BID_TankCtlCfg + i * 10 + 2);
	}

	i = Activated();
	switch(i){
	case BID_Help :
		StartHelp(HELP_Controls);
		return;
	case BID_Back :
		EditingControl = 0;
		CurrentMenu = MP_InputOptions;
		RegistrySaveNeeded = true;
		FreeGui();
		return;
	}
	if(LastControl.len() > 0 && EditingControl){	//User has hit a key/button while waiting to select one.
		i = EditingControl;
		if(LastControl != "Esc"){
			int ctl = (i - BID_TankCtlCfg) / 10;
			int typ = (i - BID_TankCtlCfg) % 10;
			if(ctl >= 0 && ctl < NumControls){
				for(int n = 0; n < NumControls; n++){	//Clear out any duplicates.
					if(CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl1 == LastControl) CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl1 = "";
					if(CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl2 == LastControl) CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl2 = "";
					if(CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl3 == LastControl) CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl3 = "";
				}
				if(typ == 0) CTankGame::Get().GetSettings()->InputSettings.Controls[ctl].ctrl1 = LastControl;	//Set our control.
				if(typ == 1) CTankGame::Get().GetSettings()->InputSettings.Controls[ctl].ctrl2 = LastControl;
				if(typ == 2) CTankGame::Get().GetSettings()->InputSettings.Controls[ctl].ctrl3 = LastControl;
			}
		}
		i = 0;
		EditingControl = 0;
	}
	if(i >= BID_TankCtlCfg){
		LastControl = "";
		EditingControl = i;
	}
}

void DoCamCtlCfg()
{
	int i;
	GUIHeader(Text.Get(TEXT_CAMERAHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_APPLY), BID_Back, 0, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "title1", CVec3(0.5, .82, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		CStr(EditingControl ? "Press a key or button (ESC to cancel)" : "                                     "), BID_None, false, 0, FLAG_TEXT_CENTERX);
	GuiEnt("guibutton", "title1", CVec3(0.05, 0.1, 0), CVec3(.02, .05, 0), CVec3(1, 1, 1),
		PadString("Control:", 13) + PadString("Primary", 11) + PadString("Secondary", 11) + PadString("Tertiary", 10), BID_None);
	for(i = 0; i < NumControls - CTankGame::Get().GetSettings()->InputSettings.FirstCamControl; i++){
		int ii = i + CTankGame::Get().GetSettings()->InputSettings.FirstCamControl;
		double y = 0.1 + (0.05 * (double)(i + 1));
		CVec3 col(1, 1, 1);
		if(EditingControl > 0 && (EditingControl - BID_CamCtlCfg) / 10 == i){	//Hilight currently active control.
			SetVec3(1, 1, 0, col);
		}
		GuiEnt("guibutton", "title1", CVec3(0.05, y, 0), CVec3(.02, .05, 0), col,
			PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[ii].name, 12), BID_None);
		GuiEnt("guibutton", "button1", CVec3(0.05 + 0.24, y, 0), CVec3(.02, .05, 0), col,
			"[" + PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[ii].ctrl1, 9) + "]", BID_CamCtlCfg + i * 10 + 0);
		GuiEnt("guibutton", "button1", CVec3(0.05 + 0.46, y, 0), CVec3(.02, .05, 0), col,
			"[" + PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[ii].ctrl2, 9) + "]", BID_CamCtlCfg + i * 10 + 1);
		GuiEnt("guibutton", "button1", CVec3(0.05 + 0.68, y, 0), CVec3(.02, .05, 0), col,
			"[" + PadString(CTankGame::Get().GetSettings()->InputSettings.Controls[ii].ctrl3, 9) + "]", BID_CamCtlCfg + i * 10 + 2);
	}

	i = Activated();
	switch(i){
	case BID_Help :
		StartHelp(HELP_Controls);
		return;
	case BID_Back :
		EditingControl = 0;
		CurrentMenu = MP_InputOptions;
		RegistrySaveNeeded = true;
		FreeGui();
		return;
	}
	if(LastControl.len() > 0 && EditingControl){	//User has hit a key/button while waiting to select one.
		i = EditingControl;
		if(LastControl != "Esc"){
			int ctl = ((i - BID_CamCtlCfg) / 10) + CTankGame::Get().GetSettings()->InputSettings.FirstCamControl;
			int typ = (i - BID_CamCtlCfg) % 10;
			if(ctl >= 0 && ctl < NumControls){
				for(int n = 0; n < NumControls; n++){	//Clear out any duplicates.
					if(CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl1 == LastControl) CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl1 = "";
					if(CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl2 == LastControl) CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl2 = "";
					if(CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl3 == LastControl) CTankGame::Get().GetSettings()->InputSettings.Controls[n].ctrl3 = "";
				}
				if(typ == 0) CTankGame::Get().GetSettings()->InputSettings.Controls[ctl].ctrl1 = LastControl;	//Set our control.
				if(typ == 1) CTankGame::Get().GetSettings()->InputSettings.Controls[ctl].ctrl2 = LastControl;
				if(typ == 2) CTankGame::Get().GetSettings()->InputSettings.Controls[ctl].ctrl3 = LastControl;
			}
		}
		i = 0;
		EditingControl = 0;
	}
	if(i >= BID_CamCtlCfg){
		LastControl = "";
		EditingControl = i;
	}
}
// RUSS - translated to here...

inline void BuildJoyString(char *tmpstr, int iAxisName, unsigned int iAxis)
{
	if(iAxis > 5)
	{
		switch(CTankGame::Get().GetSettings()->InputSettings.AxisMapping[iAxis])
		{
		case 0:
			sprintf(tmpstr, Text.Get(iAxisName), Text.Get(TEXT_NONE));
			break;
		case 1:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.HasHat() ? Text.Get(TEXT_JOYSTICKTANKLR) : Text.Get(TEXT_NONE));
			break;
		case 2:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.HasHat() ? Text.Get(TEXT_JOYSTICKTANKFB) : Text.Get(TEXT_NONE));
			break;
		case 3:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.HasHat() ? Text.Get(TEXT_JOYSTICKTURRETLR) : Text.Get(TEXT_NONE));
			break;
		case 4:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.HasHat() ? Text.Get(TEXT_JOYSTICKCAMLR) : Text.Get(TEXT_NONE));
			break;
		case 5:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.HasHat() ? Text.Get(TEXT_JOYSTICKCAMUD) : Text.Get(TEXT_NONE));
			break;
		}
	}
	else
	{
		switch(CTankGame::Get().GetSettings()->InputSettings.AxisMapping[iAxis])
		{
		case 0:
			sprintf(tmpstr, Text.Get(iAxisName), Text.Get(TEXT_NONE));
			break;
		case 1:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.AxisSupported(iAxis) ? Text.Get(TEXT_JOYSTICKTANKLR) : Text.Get(TEXT_NONE));
			break;
		case 2:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.AxisSupported(iAxis) ? Text.Get(TEXT_JOYSTICKTANKFB) : Text.Get(TEXT_NONE));
			break;
		case 3:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.AxisSupported(iAxis) ? Text.Get(TEXT_JOYSTICKTURRETLR) : Text.Get(TEXT_NONE));
			break;
		case 4:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.AxisSupported(iAxis) ? Text.Get(TEXT_JOYSTICKCAMLR) : Text.Get(TEXT_NONE));
			break;
		case 5:
			sprintf(tmpstr, Text.Get(iAxisName), CTankGame::Get().GetInputState()->input.AxisSupported(iAxis) ? Text.Get(TEXT_JOYSTICKCAMUD) : Text.Get(TEXT_NONE));
			break;
		}
	}
}

void DoJoystickSettings()
{
	EntityBase	*e;
	int		Sticks[K_MAXCONTROLLERS] = {-1};

	switch(Activated(&e)){
	case BID_Help :
		StartHelp(HELP_Controls);
		return;
	case BID_Back :
		CurrentMenu = MP_InputOptions;
		FreeGui();
		RegistrySaveNeeded = true;
		return;
	case BID_StickSelect:
		{
			int iSel = e->QueryInt(ATT_LISTBOX_SELECTION);
			if(iSel == 0)
			{
				CTankGame::Get().GetSettings()->InputSettings.StickID = -1;
				CTankGame::Get().GetSettings()->InputSettings.UseJoystick = 0;
				CTankGame::Get().GetInputState()->input.UnInitController();
			}
			else
			{
				if(CTankGame::Get().GetInputState()->input.InitController(iSel - 1))
				{
					trDeviceID tmp;
					CTankGame::Get().GetInputState()->input.GetDeviceInfo(&tmp);

					CTankGame::Get().GetSettings()->InputSettings.StickID = tmp.iDevID;
					CTankGame::Get().GetSettings()->InputSettings.sStickName = tmp.sDevName;
					CTankGame::Get().GetSettings()->InputSettings.UseJoystick = 1;
				}
				else
				{ // should never get here, but just in case
					CTankGame::Get().GetSettings()->InputSettings.UseJoystick = 0;
					CTankGame::Get().GetSettings()->InputSettings.StickID = -1;
				}
			}
			break;
		}
	case BID_StickXAxis:
		if(CTankGame::Get().GetInputState()->input.AxisSupported(0))
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[0] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[0] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[0] = 0;
		break;
	case BID_StickYAxis:
		if(CTankGame::Get().GetInputState()->input.AxisSupported(1))
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[1] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[1] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[1] = 0;
		break;
	case BID_StickZAxis:
		if(CTankGame::Get().GetInputState()->input.AxisSupported(2))
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[2] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[2] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[2] = 0;
		break;
	case BID_StickRAxis:
		if(CTankGame::Get().GetInputState()->input.AxisSupported(3))
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[3] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[3] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[3] = 0;
		break;
	case BID_StickUAxis:
		if(CTankGame::Get().GetInputState()->input.AxisSupported(4))
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[4] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[4] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[4] = 0;
		break;
	case BID_StickVAxis:
		if(CTankGame::Get().GetInputState()->input.AxisSupported(5))
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[5] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[5] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[5] = 0;
		break;
	case BID_StickHatX:
		if(CTankGame::Get().GetInputState()->input.HasHat())
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[6] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[6] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[6] = 0;
		break;
	case BID_StickHatY:
		if(CTankGame::Get().GetInputState()->input.HasHat())
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[7] = (CTankGame::Get().GetSettings()->InputSettings.AxisMapping[7] + 1) % 6;
		else
			CTankGame::Get().GetSettings()->InputSettings.AxisMapping[7] = 0;
		break;
	case BID_DeadZone:
		CTankGame::Get().GetSettings()->InputSettings.DeadZone += 0.1f;
		if(CTankGame::Get().GetSettings()->InputSettings.DeadZone > 0.91)
			CTankGame::Get().GetSettings()->InputSettings.DeadZone = 0.0f;
	}

	GUIHeader(Text.Get(TEXT_JOYSTICKHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_APPLY), BID_Back, false, 0, FLAG_TEXT_CENTERX);

	GuiEnt("guibutton", "title1", CVec3(.025, .14, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_JOYSTICKLIST), BID_None);

	e = GuiEnt("guilistbox", "listbox1", CVec3(.025, .19, 0), CVec3(.025, .04, 0), CVec3(1, 1, 1), 0, BID_StickSelect);
	if(e)
	{
		bool	bFoundStick = false;
		CStr	tmpName;
		int		iNumSticks = 1;

		e->SetInt(ATT_CMD_LISTBOX_RESET, 1);
		e->SetInt(ATT_LISTBOX_WIDTH, 38);
		e->SetInt(ATT_LISTBOX_HEIGHT, 3);

		e->SetString(ATT_LISTBOX_NEWENTRY, Text.Get(TEXT_NONE));

		for(int i = 0; i < K_MAXCONTROLLERS; i++)
		{
			if(CTankGame::Get().GetInputState()->input.GetControllerName(i, &tmpName))
			{
				Sticks[iNumSticks] = i;
				e->SetString(ATT_LISTBOX_NEWENTRY, tmpName);
				if(CTankGame::Get().GetSettings()->InputSettings.StickID == i)
				{
					e->SetInt(ATT_LISTBOX_SELECTION, iNumSticks); // set to this stick
					bFoundStick = true;
				}
				iNumSticks++;
			}
		}
		if(!bFoundStick)
			e->SetInt(ATT_LISTBOX_SELECTION, 0); // set to none
	}
	char tempstr[256];
	if(CTankGame::Get().GetSettings()->InputSettings.StickID != -1)
	{
		sprintf(tempstr, Text.Get(TEXT_JOYSTICKBUTTONS), CTankGame::Get().GetInputState()->input.NumberOfButtons());
		GuiEnt("guibutton", "button1", CVec3(.025, .4, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
			tempstr, BID_None);
		sprintf(tempstr, Text.Get(TEXT_JOYSTICKAXES), CTankGame::Get().GetInputState()->input.NumberOfAxes());
		GuiEnt("guibutton", "button1", CVec3(.3625, .4, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
			tempstr, BID_None);
		sprintf(tempstr, Text.Get(TEXT_JOYSTICKHAT), CTankGame::Get().GetInputState()->input.HasHat() ? Text.Get(TEXT_YES) : Text.Get(TEXT_NO));
		GuiEnt("guibutton", "button1", CVec3(.675, .4, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
			tempstr, BID_None);
	}
	else
	{
		sprintf(tempstr, Text.Get(TEXT_JOYSTICKBUTTONS), 0);
		GuiEnt("guibutton", "button1", CVec3(.025, .4, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
			tempstr, BID_None);
		sprintf(tempstr, Text.Get(TEXT_JOYSTICKAXES), 0);
		GuiEnt("guibutton", "button1", CVec3(.3625, .4, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
			tempstr, BID_None);
		sprintf(tempstr, Text.Get(TEXT_JOYSTICKHAT), Text.Get(TEXT_NO));
		GuiEnt("guibutton", "button1", CVec3(.675, .4, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
			tempstr, BID_None);
	}

	sprintf(tempstr, Text.Get(TEXT_JOYSTICKDEADZONE), (int)(CTankGame::Get().GetSettings()->InputSettings.DeadZone * 100.5f) );
	GuiEnt("guibutton", "button1", CVec3(.025, .45, 0), CVec3(.025, .05, 0), CVec3(1, 1, 1),
		tempstr, BID_DeadZone, false, 0, 0);

	BuildJoyString(tempstr, TEXT_JOYSTICKXAXIS, 0);
	GuiEnt("guibutton", "button1", CVec3(.025, .50, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickXAxis);
	BuildJoyString(tempstr, TEXT_JOYSTICKYAXIS, 1);
	GuiEnt("guibutton", "button1", CVec3(.025, .55, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickYAxis);
	BuildJoyString(tempstr, TEXT_JOYSTICKZAXIS, 2);
	GuiEnt("guibutton", "button1", CVec3(.025, .60, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickZAxis);
	BuildJoyString(tempstr, TEXT_JOYSTICKRAXIS, 3);
	GuiEnt("guibutton", "button1", CVec3(.025, .65, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickRAxis);
	BuildJoyString(tempstr, TEXT_JOYSTICKUAXIS, 4);
	GuiEnt("guibutton", "button1", CVec3(.025, .70, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickUAxis);
	BuildJoyString(tempstr, TEXT_JOYSTICKVAXIS, 5);
	GuiEnt("guibutton", "button1", CVec3(.025, .75, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickVAxis);
	BuildJoyString(tempstr, TEXT_JOYSTICKHATX, 6);
	GuiEnt("guibutton", "button1", CVec3(.025, .80, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickHatX);
	BuildJoyString(tempstr, TEXT_JOYSTICKHATY, 7);
	GuiEnt("guibutton", "button1", CVec3(.025, .85, 0), CVec3(0.025, 0.05, 0), CVec3(1,1,1),
		tempstr, BID_StickHatY);
}

void DoMouseSettings()
{
	char tempstr[256];
	Vec3 V, L, S;

	SetVec3(.05, .10, 0, V);
	SetVec3(0, .055, 0, L);
	SetVec3(.02, .05, 0, S);

	GUIHeader(Text.Get(TEXT_MOUSEHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);

	sprintf(tempstr, Text.Get(TEXT_MOUSEENABLE), CTankGame::Get().GetSettings()->InputSettings.MouseEnabled ? Text.Get(TEXT_ENABLED) : Text.Get(TEXT_DISABLED));
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_MouseEnable);  AddVec3(L, V);
	if (CTankGame::Get().GetSettings()->InputSettings.MouseEnabled)
	{
		V[0] += 0.05f;
		sprintf(tempstr, Text.Get(TEXT_MOUSESPEED), (int)(CTankGame::Get().GetSettings()->InputSettings.MouseSpeed * 100.0f + 0.5f));
		GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
			tempstr, BID_MouseSpeed);  AddVec3(L, V);

		sprintf(tempstr, Text.Get(TEXT_MOUSEY), CTankGame::Get().GetSettings()->InputSettings.InvMouseY ? Text.Get(TEXT_INVERTED) : Text.Get(TEXT_NORMAL));
		GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
			tempstr, BID_MouseY);  AddVec3(L, V);
		V[0] -= 0.05f;
	}

	switch(Activated()){
	case BID_Help :
		StartHelp(HELP_Controls);
		return;
	case BID_Back :
		CurrentMenu = MP_InputOptions;
		FreeGui();
		RegistrySaveNeeded = true;
		return;
	case BID_MouseSpeed :
		CTankGame::Get().GetSettings()->InputSettings.MouseSpeed += 0.25f;
		if(CTankGame::Get().GetSettings()->InputSettings.MouseSpeed > 2.0f) CTankGame::Get().GetSettings()->InputSettings.MouseSpeed = 0.25f;
		break;
	case BID_MouseY:
		CTankGame::Get().GetSettings()->InputSettings.InvMouseY = !CTankGame::Get().GetSettings()->InputSettings.InvMouseY;
		break;
	case BID_MouseEnable:
		CTankGame::Get().GetSettings()->InputSettings.MouseEnabled = !CTankGame::Get().GetSettings()->InputSettings.MouseEnabled;
		FreeGui();
		break;
	}
}

void DoInputOptions()
{
	Vec3 V, L, S;

	GUIHeader(Text.Get(TEXT_INPUTHDR), "optionsbackground", false);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);

	SetVec3(.5, 0.1825, 0, V);
	SetVec3(0, .075, 0, L);
	SetVec3(.04, .07, 0, S);

	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		Text.Get(TEXT_INPUTTANK), BID_TankCtlCfg, false, 0, FLAG_TEXT_CENTERX);  AddVec3(L, V);

	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		Text.Get(TEXT_INPUTCAMERA), BID_CamCtlCfg, false, 0, FLAG_TEXT_CENTERX);  AddVec3(L, V);

	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		Text.Get(TEXT_INPUTJOYSTICKSETTINGS), BID_JoystickSettings, false, 0, FLAG_TEXT_CENTERX);  AddVec3(L, V);

	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		Text.Get(TEXT_INPUTMOUSESETTINGS), BID_MouseSettings, false, 0, FLAG_TEXT_CENTERX);  AddVec3(L, V);

	switch(Activated()){
	case BID_Back :
		CurrentMenu = MP_Options;
		FreeGui();
		return;
	case BID_TankCtlCfg :
		EditingControl = 0;
		CurrentMenu = MP_TankCtlCfg;
		FreeGui();
		return;
	case BID_CamCtlCfg :
		EditingControl = 0;
		CurrentMenu = MP_CamCtlCfg;
		FreeGui();
		return;
	case BID_JoystickSettings:
		EditingControl = 0;
		CurrentMenu = MP_JoystickSettings;
		FreeGui();
		return;
	case BID_MouseSettings:
		EditingControl = 0;
		CurrentMenu = MP_MouseSettings;
		FreeGui();
		return;
	}
}

void DoMiscOptions()
{
	char tempstr[256];
	Vec3 V, L, S;

	GUIHeader(Text.Get(TEXT_MISCHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_APPLY), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	SetVec3(.05, .10, 0, V);
	SetVec3(0, .055, 0, L);
	SetVec3(.02, .05, 0, S);

	sprintf(tempstr, Text.Get(TEXT_MISCRATE), CTankGame::Get().GetSettings()->ClientRate / 1000);
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_Rate);  AddVec3(L, V);

	switch(CTankGame::Get().GetSettings()->CamStyle)
	{
	case 0:
		sprintf(tempstr, Text.Get(TEXT_MISCCAMPITCH), Text.Get(TEXT_MISCFOLLOWTERRAIN));
		break;
	case 1:
		sprintf(tempstr, Text.Get(TEXT_MISCCAMPITCH), Text.Get(TEXT_MISCFOLLOWTANK));
		break;
	case 2:
		sprintf(tempstr, Text.Get(TEXT_MISCCAMPITCH), Text.Get(TEXT_NONE));
		break;
	}
	GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
		tempstr, BID_CamStyle);  AddVec3(L, V);

	switch(Activated()){
	case BID_Help :
		StartHelp(HELP_Misc);
		return;
	case BID_Back :
		CurrentMenu = MP_Options;
		FreeGui();
		RegistrySaveNeeded = true;
		return;
	case BID_Rate :
		ClientInfoSendNeeded = true;
		if(CTankGame::Get().GetSettings()->ClientRate <= 1000) CTankGame::Get().GetSettings()->ClientRate = 2000;
		else if(CTankGame::Get().GetSettings()->ClientRate <= 2000) CTankGame::Get().GetSettings()->ClientRate = 3000;
		else if(CTankGame::Get().GetSettings()->ClientRate <= 3000) CTankGame::Get().GetSettings()->ClientRate = 4000;
		else if(CTankGame::Get().GetSettings()->ClientRate <= 4000) CTankGame::Get().GetSettings()->ClientRate = 5000;
		else if(CTankGame::Get().GetSettings()->ClientRate <= 5000) CTankGame::Get().GetSettings()->ClientRate = 6000;
		else if(CTankGame::Get().GetSettings()->ClientRate <= 6000) CTankGame::Get().GetSettings()->ClientRate = 10000;
		else CTankGame::Get().GetSettings()->ClientRate = 1000;
		return;
	case BID_CamStyle:
		CTankGame::Get().GetSettings()->CamStyle = (CTankGame::Get().GetSettings()->CamStyle+1) % 3;
		return;
	}
}

void DoLanguage()
{
	char tempstr[256];
	int i,flags,active;
	Vec3 V, L, S;

	GUIHeader(Text.Get(TEXT_LANGHDR), "optionsbackground", true);

	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);
	SetVec3(.04, .07, 0, S);
	SetVec3(.5, 0.1825, 0, V);
	SetVec3(0, .075, 0, L);

	i = 0;
	while ( Language.Get(i) ) {
		if (Paths.Get(i)) {
			flags = FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY;
		} else {
			flags = FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY | FLAG_BUTTON_DISABLED;
		}

		GuiEnt("guibutton", "button1", V, S, CVec3(1, 1, 1),
			Language.Get(i), BID_English+i, false, 0, flags);
		AddVec3(L, V);
		i++;

		if (i == 5) break;
		// FIXME: allow more than 5 languages
	}

	AddVec3(L, V);
	GuiEnt("guibutton", "button1", V, CVec3(.02, .035, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_LANGDISCLAIM), BID_None, false, 0, FLAG_TEXT_CENTERX | FLAG_BUTTON_DISABLED); AddVec3(L, V);


	active = Activated();
	switch(active){
	case BID_Back :
		CurrentMenu = MP_Options;
		FreeGui();
		RegistrySaveNeeded = true;
		return;
	case BID_English:
	case BID_French:
	case BID_German:
	case BID_Italian:
	case BID_Spanish:
		i = active - BID_English;

		if (!Language.Get(i)) return;
		if (!Paths.Get(i)) return;

		sprintf(tempstr, "%s\\%s", Paths.Get(i), "Help.TXT");
		TextBlock.Load(tempstr);
		sprintf(tempstr, "%s\\%s", Paths.Get(i), "Tutorial.TXT");
		TextBlock2.Load(tempstr);

		sprintf(tempstr, "%s\\%s", Paths.Get(i), "Lines.TXT");
		Text.Load(tempstr);
		sprintf(tempstr, "%s\\%s", Paths.Get(i), "Names.TXT");
		Names.Load(tempstr);
		sprintf(tempstr, "%s\\%s", Paths.Get(i), "Weapons.TXT");
		Weapons.Load(tempstr);
		sprintf(tempstr, "%s\\%s", Paths.Get(i), "Insignia.TXT");
		Insignia.Load(tempstr);
		sprintf(tempstr, "%s\\%s", Paths.Get(i), "Sounds.TXT");
		SoundPaths.Load(tempstr);
		sprintf(tempstr, "%s\\%s", Paths.Get(i), "ControlText.TXT");
		ControlText.Load(tempstr);


		int iCookie = 0;
		EntitySoundType	*tmpType;
		while ((iCookie = CTankGame::Get().GetVW()->EnumEntityTypes("sound", (EntityTypeBase**)&tmpType, iCookie)) != 0)
		{
			// Force reload of sounds
			if (tmpType->localise) {
				tmpType->UnlinkResources();
				tmpType->CacheResources();
			}
		}


		CurrentMenu = MP_Options;
		FreeGui();
		RegistrySaveNeeded = true;
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//									Options Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoOptions()
{
	Vec3 V, L;

	GUIHeader(Text.Get(TEXT_OPTIONSHDR), "optionsbackground", false);

	SetVec3(.5, 0.1825, 0, V);
	SetVec3(0, .075, 0, L);
	GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_OPTIONS1), BID_GraphicsOptions, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
	GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_OPTIONS2), BID_SoundMusicOptions, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
	GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_OPTIONS3), BID_InputOptions, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
	GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_OPTIONS4), BID_MiscOptions, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
#if 0	// Russ
	GuiEnt("guibutton", "button1", V, CVec3(.04, .07, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_OPTIONS5), BID_Language, false, 0, FLAG_TEXT_CENTERX | FLAG_TEXT_CENTERY); AddVec3(L, V);
#endif
	GuiEnt("guibutton", "button1", CVec3(.5, .89, 0), CVec3(.05, .1, 0), CVec3(1, 1, 1),
		Text.Get(TEXT_BACK), BID_Back, false, 0, FLAG_TEXT_CENTERX);

	switch(Activated()){
	case BID_GraphicsOptions:
		CurrentMenu = MP_GraphicsOptions;
		FreeGui();
		break;
	case BID_SoundMusicOptions:
		CurrentMenu = MP_SoundMusicOptions;
		FreeGui();
		break;
	case BID_InputOptions:
		CurrentMenu = MP_InputOptions;
		FreeGui();
		break;
	case BID_MiscOptions:
		CurrentMenu = MP_MiscOptions;
		FreeGui();
		break;
	case BID_Language:
		CurrentMenu = MP_Language;
		FreeGui();
		break;
	case BID_Back:
		if(CTankGame::Get().IsGameRunning()){
			CurrentMenu = MP_InGameMain;
		}else{
			if(CTankGame::Get().GetGameState()->LadderMode){
				CurrentMenu = MP_LadderRoster;
			}else{
				CurrentMenu = MP_Main;
			}
		}
		FreeGui();
		return;
	}
}

void DoMenus(){
	EntityBase *e;
	//
	CStr CurName, CurWeb, CurInfo, CurDetail;
	//
	if(CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire){
		e = CTankGame::Get().GetVW()->FindRegisteredEntity("ParticleFireBurner");
		if(e){
			if(CurrentMenu != MP_None){
				e->SetInt(ATT_PFIRE_PING, 1);	//Keep it burning.
			}
			e->SetFloat(ATT_PFIRE_FOLLOWX, CTankGame::Get().GetVW()->GetMouseX());
			e->SetFloat(ATT_PFIRE_FOLLOWY, CTankGame::Get().GetVW()->GetMouseY());
			e->SetInt(ATT_PFIRE_FOLLOW, 1);
			if(CTankGame::Get().GetVW()->GetMouseClicked()) e->SetInt(ATT_PFIRE_EXPLODE, 1);
		}else{
			CTankGame::Get().GetVW()->AddEntity("particlefire", "burn1", NULL, NULL, NULL, 0, 0, 0, 0);	//Only one will ever live.
		}
	}
	//
	//This makes sure we get back around (i.e. no fatal crash on mode change, etc.) before saving.
	if(RegistrySaveNeeded){
		RegistrySave();
		RegistrySaveNeeded = false;
	}
	//
	if(CTankGame::Get().GetGameState()->ToggleMenu) SkipIntro = true;
	//
	if(CTankGame::Get().GetGameState()->ToggleMenu && CurrentMenu != MP_None && CurrentMenu != MP_Initialize && CurrentMenu != MP_TankCtlCfg && CurrentMenu != MP_CamCtlCfg){
		if(CTankGame::Get().GetVW()->Map.Width() > 0 && CTankGame::Get().GetVW()->Map.Height() > 0 && CTankGame::Get().GetVW()->FindRegisteredEntity("PlayerEntity")){
			FreeGui();
			CurrentMenu = MP_None;
			CTankGame::Get().GetGameState()->ToggleMenu = false;
			//
			//Send client info to server if needed.
			if(ClientInfoSendNeeded){
				CTankGame::Get().GetVW()->SendClientInfo(CTankGame::Get().GetVW()->FindEntityType("racetank", CTankGame::Get().GetSettings()->TankType), CTankGame::Get().GetSettings()->PlayerName, CTankGame::Get().GetSettings()->ClientRate);
				ClientInfoSendNeeded = false;
			}
			//
			return;
		}
	}
	if(CurrentMenu != MP_None) CTankGame::Get().GetGameState()->ToggleMenu = false;
	//
	if(LMenuFire != CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire && CurrentMenu != MP_Initialize && CurrentMenu != MP_None){
		FreeGui();	//Must free gui when menu fire state changes.
	}
	LMenuFire = CTankGame::Get().GetSettings()->GraphicsSettings.MenuFire;
	//
	if(CurrentMenu != LastMenu){
		FirstTimeOnMenu = 1;
	}
	LastMenu = CurrentMenu;
	//
	int i;
	switch(CurrentMenu){
	//
	case MP_Initialize :
		for(i = 0; i < MAX_SLOTS; i++) GUISlot[i].gid = 0;
		//
		if(!CTankGame::Get().GetSettings()->BypassIntro){
			CurrentMenu = MP_Intro1;//MP_Main;
		}else{
			CurrentMenu = MP_Main;
		}
		if(CTankGame::Get().GetSettings()->AutoStart)
		{
			CTankGame::Get().GetSettings()->AutoStart = false;
			StartingGameDelay = 1500;
			CTankGame::Get().GetGameState()->AutoDrive = false;
			CurrentMenu = MP_StartingGame;
		}
		//
		CacheMenus();
		//
		break;
	//
	case MP_None :
		FreeGui();
		if(CTankGame::Get().GetGameState()->ToggleMenu){
			CurrentMenu = MP_InGameMain;//MP_Main;
			CTankGame::Get().GetGameState()->ToggleMenu = false;
		}
		break;
	//
	case MP_StartingGame :	//Delays while menu flies off, before starting game.
		DoStartingGame();
		break;
////////////////////////////////////////////////////////////////////////////////////////////////////
//									Outro Menus
////////////////////////////////////////////////////////////////////////////////////////////////////
	case MP_Outro1 :
		switch(Activated()){
		case BID_FirstTimeOnMenu :
			CTankGame::Get().GetVW()->CacheResources("guibackground", "outroback1");	//pre-cache outro images before displaying.
			CTankGame::Get().GetVW()->CacheResources("guibackground", "outroback2");
			CTankGame::Get().GetVW()->CacheResources("guibackground", "outroback3");
			CTankGame::Get().GetVW()->ClearChar();
			break;
		}
		GuiEntStart();
		GuiEnt("guibackground", "outroback1", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		OutroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || OutroTimer > OutroDelayTime){
			OutroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Outro2;
			return;
		}
		break;
	case MP_Outro2 :
		GuiEntStart();
		GuiEnt("guibackground", "outroback2", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		OutroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || OutroTimer > OutroDelayTime){
			OutroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Outro4;
			return;
		}
		break;
	case MP_Outro3 :
		GuiEntStart();
		GuiEnt("guibackground", "outroback3", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		OutroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || OutroTimer > OutroDelayTime){
			OutroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Outro4;
			return;
		}
		break;
	case MP_Outro4 :	//This menu is just so we fade to black before terminating.
		GuiEntStart();
		OutroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || OutroTimer > OutroDelayTime2){
			CTankGame::Get().GetGameState()->Quit = true;
			FreeGui();
			return;
		}
		break;
////////////////////////////////////////////////////////////////////////////////////////////////////
//									Intro Menus
////////////////////////////////////////////////////////////////////////////////////////////////////
	case MP_Intro1 :
		GuiEntStart();
		GuiEnt("guibackground", "introback1", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer > IntroDelayTime){
			IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Intro2;
			return;
		}
		break;
	case MP_Intro2 :
		GuiEntStart();
		GuiEnt("guibackground", "introback2", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer > IntroDelayTime){
			IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Intro3;
			return;
		}
		break;
	case MP_Intro3 :
		GuiEntStart();
		GuiEnt("guibackground", "introback3", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer > IntroDelayTime){
			IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Intro4;
			return;
		}
		break;
	case MP_Intro4 :
		GuiEntStart();
		GuiEnt("guibackground", "introback4", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer > IntroDelayTime){
			IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Intro5;
			return;
		}
		break;
	case MP_Intro5 :
		GuiEntStart();
		GuiEnt("guibackground", "introback5", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer > IntroDelayTime2){
			IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Intro6;
			return;
		}
		break;
	case MP_Intro6 :
		GuiEntStart();
		GuiEnt("guibackground", "introback6", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer > IntroDelayTime2){
			IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Intro7;
			return;
		}
		break;
	case MP_Intro7 :
		GuiEntStart();
		GuiEnt("guibackground", "introback7", CVec3(0, 0, 0.5), NULL, NULL, NULL, BID_None);
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked() || IntroTimer > IntroDelayTime){
			IntroTimer = 0;
			FreeGui();
			CurrentMenu = MP_Intro8;
			IntroLogoFader = 0.0f;
			return;
		}
		break;
	case MP_Intro8 :
		GuiEntStart();
		e = GuiEnt("mesh", "tmlogo", CVec3(0.0f, 0.0f, 30.0f), CVec3(0, 0, 0), CVec3(0, 0, 0), 0, BID_None);
		if(e){
			e->SetRot(CVec3(0, fmod(((double)CTankGame::Get().GetVW()->Time() * 0.001) * 4.0, (double)PI * 2.0), 0));
			e->SetFloat(ATT_FADE, e->QueryFloat(ATT_TYPE_FADE) * (IntroLogoFader / MAX(0.01f, IntroLogoFaderMax)));
		}
		IntroTimer += CTankGame::Get().GetVW()->FrameTime();
		if(SkipIntro || CTankGame::Get().GetVW()->GetChar() || CTankGame::Get().GetVW()->GetMouseClicked()){
			IntroTimer = IntroDelayTime + 1;
		}
		if(IntroTimer > IntroDelayTime){
			if(IntroLogoFader > 0.0f){
				IntroLogoFader -= CTankGame::Get().GetVW()->FrameFrac();
			}else{
				IntroTimer = 0;
				FreeGui();
				CurrentMenu = MP_Main;
				return;
			}
		}else{
			IntroLogoFader += CTankGame::Get().GetVW()->FrameFrac();
			if(IntroLogoFader > IntroLogoFaderMax) IntroLogoFader = IntroLogoFaderMax;
		}
		break;
	case MP_Trophy :
		DoTrophy();
		break;
	case MP_TextDisplay :
		DoTextDisplay();
		break;
	case MP_ImageHelp :
		DoImageHelpDisplay();
		break;
////////////////////////////////////////////////////////////////////////////////////////////////////
//									You Have Wrong Version Menu
////////////////////////////////////////////////////////////////////////////////////////////////////
	case MP_WrongVersion :
		CurrentMenu = ReturnToMenu;
		StartHelp(HELP_WrongVersion);
		return;
		//
		break;
	case MP_LadderRoster :
		DoLadderRoster();
		break;
	case MP_LadderNew :
		DoLadderNew();
		break;
	case MP_LadderDelete :
		DoLadderDelete();
		break;
	case MP_Ladder :
		DoLadder();
		break;
	case MP_Stats :
		DoStats();
		break;
	case MP_InGameMain :
		DoInGameMain();
		break;
	case MP_Main :
		DoMain();
		break;
	case MP_TankSelect :
		DoTankSelect();
		break;
	case MP_Tutorial :
		DoTutorial();
		break;
	case MP_MapSelect :
		DoMapSelect();
		break;
	case MP_ServerSelect :
		DoServerSelect();
		break;
	case MP_Chat :
		DoChat();
		break;
	case MP_Options :
		DoOptions();
		break;
	case MP_GraphicsOptions :
		DoGraphicsOptions();
		break;
	case MP_SoundMusicOptions :
		DoSoundMusicOptions();
		break;
	case MP_InputOptions :
		DoInputOptions();
		break;
	case MP_MouseSettings :
		DoMouseSettings();
		break;
	case MP_JoystickSettings :
		DoJoystickSettings();
		break;
	case MP_MiscOptions :
		DoMiscOptions();
		break;
	case MP_Language:
		DoLanguage();
		break;
	case MP_TankCtlCfg :
		DoTankCtlCfg();
		break;
	case MP_CamCtlCfg :
		DoCamCtlCfg();
		break;
	}
}

