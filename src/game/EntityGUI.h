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

//GUI Entities.

#ifndef ENTITYGUI_H
#define ENTITYGUI_H

#include "EntityBase.h"

//
//This function MUST be called ONCE by your main app to register the RaceTank classes.
int RegisterGUIEntities();
//

//Base classes for GUI entities.
////////////////////////////////////////////////
//     Base GUI entity, handles
//     clickability and focus.  Derives from
//     Sprite to handle texture loading.
////////////////////////////////////////////////

class EntityGUIType : public EntitySpriteType {
public:
	int type_clickable;
	float type_removetime;
	float type_addtime;
	CStr type_clicksound;
public:
	EntityGUIType(ConfigFile *cfg, const char *c, const char *t);// : EntityTypeBase(c, t) { };
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	virtual int InterrogateInt(const char *thing);	//Responds to "IsGUIEntity".
};

#define ATT_GUIENTITY	0x37f15001
#define ATT_ACTIVATED	0x37f10009
#define ATT_CLICKED		0x37f10008
class EntityGUI : public EntitySprite {
public:
	float x, y, w, h, z;
	int clicked, activated, mouseover;
	int niceremove;	//Zero to be alive, is set to 1 to start removal, then counts up MS.
	int niceadd;	//Zero for normal, positive and counting down when being added.
public:
	float NiceT();	//Returns 0.0 for start of niceadd or and of niceremove, to 1.0 for "normal" state.
public:
	EntityGUI(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
public:
	bool Think();
	void SetPos(Vec3 pos);
	int QueryInt(int type);
	bool SetInt(int type, int att);	//Use ATT_CMD_NICEREMOVE.
	EntityGroup Group(){ return GROUP_PROP; };
};

//ALL EntityGUI sub-classes should at least call EntityGUI::Think(), to handle clickability.
////////////////////////////////////////////////
//     GUI Background Image Entity
////////////////////////////////////////////////

class EntityGUIBackgroundType : public EntityGUIType {
public:
	float type_xscale, type_yscale, type_rotatetime, type_rotateshift;
	int type_useenvtexture;
	float type_perturbstart, type_perturbend, type_perturbscale, type_perturbbias;
	int type_vertsx, type_vertsy;
	float type_perturbxv, type_perturbyv;
public:
	EntityGUIBackgroundType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
};
class EntityGUIBackground : public EntityGUI {
public:
	TilingTextureObject tileobj;
//	float perturbx, perturby;
public:
	EntityGUIBackground(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
public:
	bool Think();
};

////////////////////////////////////////////////
//     GUI Text/Graphic Button Entity
////////////////////////////////////////////////

class EntityGUIButtonType : public EntityGUIType {
public:
//	float type_xscale, type_yscale, type_rotatespeed, type_rotateshift;
	CStr type_fonttype;
	Vec3 type_overcolor, type_clickcolor;
	float type_overlinger, type_pulseamount, type_pulsetime;
public:
	EntityGUIButtonType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
};
#define FLAG_BUTTON_DISABLED	0x100000
#define ATT_BUTTON_XFLIP		0xf1380001
#define ATT_BUTTON_YFLIP		0xf1380002
#define FLAG_BUTTON_HIDDEN		0x200000

class EntityGUIButton : public EntityGUI {
public:
	TilingTextureObject tileobj;
	CStr text;
	EntityGID fontent;
	float offscreenx, offscreeny;
	unsigned int overstart, lastmouseover;
	Vec3 speccolor;
	float overlinger;
	int xflip, yflip;	//For the image.
public:
	EntityGUIButton(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityGUIButton();
public:
	bool Think();
	bool SetString(int type, const char *s);
	CStr QueryString(int type);
	bool SetInt(int type, int att);
};

////////////////////////////////////////////////
//     GUI Text Edit Entity
////////////////////////////////////////////////

#define FLAG_EDIT_INT		0x01
#define FLAG_EDIT_FLOAT		0x02
#define FLAG_EDIT_NEGATIVE	0x04
//QueryInt/Float these.
#define ATT_EDIT_INT	0x37fa2301
#define ATT_EDIT_FLOAT	0x37fa2302
//This queries true _ONCE_ if enter key has been pressed.
#define ATT_EDIT_ENTER	0x37fa2303
//SetString this.
#define ATT_EDIT_HEADER	0x37fa2304
//Set displayed length in ID.
#define ATT_EDIT_MAXLENGTH	0x37fa2305
//Set a length limit for the text

class EntityGUIEditType : public EntityGUIButtonType {
public:
	float type_cursorflash;
public:
	EntityGUIEditType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
};
class EntityGUIEdit : public EntityGUIButton {
public:
	CStr realtext, headertext;
	int flashms;
	int enterpressed, caretpos;
	int maxlength;
public:
	EntityGUIEdit(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
public:
	bool Think();
	int QueryInt(int type);
	float QueryFloat(int type);
	bool SetInt(int type, int att);
	bool SetString(int type, const char *s);
	CStr QueryString(int type);
};

////////////////////////////////////////////////
//     GUI Listbox Entity
////////////////////////////////////////////////

class ListboxEntry : public LinklistBase<ListboxEntry>, public CStr {
public:
	Vec3 color;
	ListboxEntry(){ };
	ListboxEntry(const char *c) : CStr(c) {SetVec3(1, 1, 1, color); };
};
//SetInt to reset list.
#define ATT_CMD_LISTBOX_RESET 0x188c2001
//Set/QueryInt for selected item.
#define ATT_LISTBOX_SELECTION 0x188c2002
//Set/QueryInt for first item displayed.
#define ATT_LISTBOX_SCROLL 0x188c2004
//SetInt for number of lines of selection.
#define ATT_LISTBOX_HEIGHT 0x188c3001
//SetInt for character width of up/down buttons.
#define ATT_LISTBOX_WIDTH 0x188c3002
//SetString to add an entry to the list.
#define ATT_LISTBOX_NEWENTRY 0x188c2003
//SetVec this with a 3-float vector for the color of the most recently added entry.
#define ATT_LISTBOX_ENTRYCOLOR 0x188c2004
//
#define MAX_LISTBOX_HEIGHT 32
//
class EntityGUIListboxType : public EntityGUIType {
public:
//	float type_xscale, type_yscale, type_rotatespeed, type_rotateshift;
//	CStr type_fonttype;
//	Vec3 type_overcolor, type_clickcolor;
	CStr type_uptype, type_downtype, type_listtype, type_knobtype;
	Vec3 type_selectcolor;
	int type_knobwidth;
public:
	EntityGUIListboxType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityGUIListbox : public EntityGUI {
public:
//	TilingTextureObject tileobj;
//	CStr text;
	EntityGID upent, downent, knobent;
	EntityGID listents[MAX_LISTBOX_HEIGHT];
	int height, width, selection, scroll;
	ListboxEntry listhead;
public:
	EntityGUIListbox(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityGUIListbox();
public:
	bool Think();
	bool SetInt(int type, int val);
	int QueryInt(int type);
	bool SetString(int type, const char *s);
	CStr QueryString(int type);
	bool SetVec(int type, const void *v);
};

////////////////////////////////////////////////
//     GUI Image Browser Entity
////////////////////////////////////////////////

#define ATT_BROWSER_SETIMAGE	0x42570001
#define ATT_BROWSER_HFLIP		0x42570002
//SetString with the file name of a new image to cache in and download.
class EntityGUIImageBrowserType : public EntityGUIButtonType {
public:
	int type_bpp;
public:
	EntityGUIImageBrowserType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
};
class EntityGUIImageBrowser : public EntityGUIButton {
private:
	CStr imagename;
	bool bHFlip;
public:
	EntityGUIImageBrowser(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
public:
	bool Think();
	bool SetString(int type, const char *s);
	bool SetInt(int type, int att);
//	int QueryInt(int type);
//	float QueryFloat(int type);
//	CStr QueryString(int type);
};

////////////////////////////////////////////////
//     GUI Mesh Entity
////////////////////////////////////////////////

class EntityGUIMeshType : public EntityGUIType {
public:
	CStr type_meshfile;
	Mesh *type_mesh;
	float type_scale;
public:
	EntityGUIMeshType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
	bool CacheResources();
	void ListResources(FileCRCList *fl);
	void UnlinkResources();
};
class EntityGUIMesh : public EntityGUI {
public:
	OrthoMeshObject orthomeshobj;
public:
	EntityGUIMesh(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
	~EntityGUIMesh();
public:
	bool Think();
};

////////////////////////////////////////////////
//     GUI Mouse Pointer Entity
////////////////////////////////////////////////

class EntityGUIMouseType : public EntityGUIMeshType {
public:
	Vec3 type_rotation, type_rotatespeed;
public:
	EntityGUIMouseType(ConfigFile *cfg, const char *c, const char *t);
	EntityBase *CreateEntity(Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0, int id = 0, int flags = 0);
};
class EntityGUIMouse : public EntityGUIMesh {
public:
	EntityGUIMouse(EntityTypeBase *et, Vec3 Pos = 0, Rot3 Rot = 0, Vec3 Vel = 0,
		int id = 0, int flags = 0);
public:
	bool Think();
};

#endif

