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

#include "EntityGUI.h"
#include "VoxelWorld.h"
#include "Basis.h"

#ifdef WIN32
#define strcasecmp(a, b) _stricmp(a, b)
#endif

//*****************************************************************
//**  Global entity type setup function.  Must be modified to
//**  handle all available entity classes.
//*****************************************************************
extern EntityTypeBase *CreateGUIType(ConfigFile *cfg){
	if(!cfg) return 0;
	char superclass[256], subclass[256];
	if(cfg->FindKey("class") && cfg->GetStringVal(superclass, 256)){
		if(cfg->FindKey("type") && cfg->GetStringVal(subclass, 256)){
			if(strcasecmp(superclass, "guibackground") == 0) return new EntityGUIBackgroundType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "guibutton") == 0) return new EntityGUIButtonType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "guiedit") == 0) return new EntityGUIEditType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "guilistbox") == 0) return new EntityGUIListboxType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "guimesh") == 0) return new EntityGUIMeshType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "guimouse") == 0) return new EntityGUIMouseType(cfg, superclass, subclass);
			if(strcasecmp(superclass, "guiimagebrowser") == 0) return new EntityGUIImageBrowserType(cfg, superclass, subclass);
		}
	}
	return 0;
}

//This function MUST be called ONCE by your main app to register the GUI classes.
int RegisterGUIEntities(){
	return RegisterEntityTypeCreator(CreateGUIType);
}


////////////////////////////////////////////////
//     Base GUI entity, handles
//     clickability and focus.  Derives from
//     Sprite to handle texture loading.
////////////////////////////////////////////////

EntityGUIType::EntityGUIType(ConfigFile *cfg, const char *c, const char *t) : EntitySpriteType(cfg, c, t) {
	type_clickable = 1;
	type_removetime = 1.0f;
	type_addtime = 1.0f;
	if(cfg){
		if(cfg->FindKey("clickable")) cfg->GetIntVal(&type_clickable);
		if(cfg->FindKey("removetime")) cfg->GetFloatVal(&type_removetime);
		if(cfg->FindKey("addtime")) cfg->GetFloatVal(&type_addtime);
		if(cfg->FindKey("clicksound")) cfg->GetStringVal(type_clicksound);
	}
	Transitory = true;
}
bool EntityGUIType::CacheResources(){
	if(ResCached) return true;
	if(EntitySpriteType::CacheResources()){
		if(type_clicksound.len() > 0) VW->CacheResources("sound", type_clicksound);
		return ResCached = true;
	}
//	ResCached = true;	//Erm, avoid circular chaching while still caching sound if no sprite image specified.
//	if(type_clicksound.len() > 0) VW->CacheResources("sound", type_clicksound);
	return ResCached = false;
}
void EntityGUIType::ListResources(FileCRCList *fl) {
	if (ResListed) return;
	EntitySpriteType::ListResources(fl);
	ResListed = true;
	if(type_clicksound.len() > 0) VW->ListResources("sound",type_clicksound);
}
EntityBase *EntityGUIType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUI(this, Pos, Rot, Vel, id, flags);
}
int EntityGUIType::InterrogateInt(const char *thing){
	if(CmpLower(thing, "IsGUIEntity")) return 1;
	return 0;
}
//
EntityGUI::EntityGUI(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntitySprite(et, Pos, Rot, Vel, id, flags) {
	EntityGUIType *TP = (EntityGUIType*)TypePtr;
	clicked = 0;
	mouseover = 0;
	activated = 0;
	niceremove = 0;
	if(TP->type_addtime > 0.0f) niceadd = (int)(TP->type_addtime * 1000.0f);
	else niceadd = 0;
	CanCollide = false;
	x = Position[0];
	y = Position[1];
	w = Rotation[0];
	h = Rotation[1];
	z = Position[2];
}
float EntityGUI::NiceT(){
	EntityGUIType *TP = (EntityGUIType*)TypePtr;
	if(niceadd > 0) return 1.0f - ((float)niceadd / std::max(0.1f, TP->type_addtime * 1000.0f));
	if(niceremove > 0) return 1.0f - ((float)niceremove / std::max(0.1f, TP->type_removetime * 1000.0f));
	return 1.0f;
}
bool EntityGUI::Think(){
	EntityGUIType *TP = (EntityGUIType*)TypePtr;
//	x = Position[0];
//	y = Position[1];
	z = Position[2];
	if(niceadd > 0){
		niceadd -= VW->FrameTime();
		if(niceadd < 0) niceadd = 0;
	}else niceadd = 0;
	if(niceremove && niceadd == 0){
		niceremove += VW->FrameTime();
		if(niceremove > (TP->type_removetime * 1000.0f)) Remove();
	}
	if(TP->type_clickable && niceremove == 0 && niceadd == 0){
		if(VW->GetMouseX() >= x && VW->GetMouseX() < x + w && VW->GetMouseY() >= y && VW->GetMouseY() < y + h){
			//Mouse is inside button.
			if(activated){
				activated = 0;	//So will only be set for one go-around of thinking.
				clicked = 0;
			}
			mouseover = 1;
			if(VW->GetMouseClicked()){
				clicked = 1;
				VW->SetFocus(GID);
			}
			if(VW->GetMouseReleased()){
				if(clicked){
					activated = 1;
					VW->SetActivated(GID);
					if(TP->type_clicksound.len() > 0) VW->AddEntity("sound", TP->type_clicksound, Position, Rotation, Velocity, 0, 0, 0, 0);
				}
			}
		}else{
			//Mouse is outside button.
			mouseover = 0;
			if(VW->GetMouseButton() == 0 || VW->GetMouseReleased() || VW->GetMouseClicked()){
				//Any mouse activity outside of button.
				clicked = 0;
				activated = 0;
			}
		}
	}
	return true;
}
void EntityGUI::SetPos(Vec3 pos){
	EntitySprite::SetPos(pos);
	x = Position[0];
	y = Position[1];
}
int EntityGUI::QueryInt(int type){
	if(type == ATT_GUIENTITY) return 1;
	if(type == ATT_ACTIVATED) return activated;
	if(type == ATT_CLICKED) return clicked;
	return EntitySprite::QueryInt(type);
}
bool EntityGUI::SetInt(int type, int val){
	if(type == ATT_CMD_NICEREMOVE){
		niceremove = std::max(1, niceremove);
		return true;
	}
	return EntitySprite::SetInt(type, val);
}

////////////////////////////////////////////////
//     GUI Background Image Entity
////////////////////////////////////////////////

EntityGUIBackgroundType::EntityGUIBackgroundType(ConfigFile *cfg, const char *c, const char *t) : EntityGUIType(cfg, c, t) {
	type_clickable = 0;
	type_xscale = 1.0f;
	type_yscale = 1.0f;
	type_rotatetime = 1.0f;
	type_rotateshift = 1.0f;
	type_useenvtexture = 0;
	type_perturbstart = 0.0f;
	type_perturbend = 0.0f;
	type_perturbscale = 0.0f;
	type_vertsx = type_vertsy = 0;
	type_perturbxv = type_perturbyv = 0.0f;
	type_perturbbias = 0.5f;
	if(cfg){
		if(cfg->FindKey("xscale")) cfg->GetFloatVal(&type_xscale);
		if(cfg->FindKey("yscale")) cfg->GetFloatVal(&type_yscale);
		if(cfg->FindKey("rotatetime")) cfg->GetFloatVal(&type_rotatetime);
		if(cfg->FindKey("rotateshift")) cfg->GetFloatVal(&type_rotateshift);
		if(cfg->FindKey("useenvtexture")) cfg->GetIntVal(&type_useenvtexture);
		//
		if(cfg->FindKey("perturbstart")) cfg->GetFloatVal(&type_perturbstart);
		if(cfg->FindKey("perturbend")) cfg->GetFloatVal(&type_perturbend);
		if(cfg->FindKey("perturbscale")) cfg->GetFloatVal(&type_perturbscale);
		if(cfg->FindKey("perturbxv")) cfg->GetFloatVal(&type_perturbxv);
		if(cfg->FindKey("perturbyv")) cfg->GetFloatVal(&type_perturbyv);
		if(cfg->FindKey("vertsx")) cfg->GetIntVal(&type_vertsx);
		if(cfg->FindKey("vertsy")) cfg->GetIntVal(&type_vertsy);
		if(cfg->FindKey("perturbbias")) cfg->GetFloatVal(&type_perturbbias);
	}
};
EntityBase *EntityGUIBackgroundType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUIBackground(this, Pos, Rot, Vel, id, flags);
}
//
EntityGUIBackground::EntityGUIBackground(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntityGUI(et, Pos, Rot, Vel, id, flags) {
//	perturbx = perturby = 0.0f;
	tileobj.PerturbX = (TMrandom() & 255);
	tileobj.PerturbY = (TMrandom() & 255);
}
bool EntityGUIBackground::Think(){
	EntityGUIBackgroundType *TP = (EntityGUIBackgroundType*)TypePtr;
	if(TP->texture || TP->type_useenvtexture){
		float fade = Bias(TP->type_fadebias, TP->type_fade * NiceT());
		float col = (TP->type_rendflags & SPRITE_BLEND_ADD ? fade : 1.0f);
	//	if(niceremove) fade *= 1.0f - ((float)niceremove / std::max(0.1f, TP->type_removetime * 1000.0f));
		//
		double a = ((double)VW->Time() / 1000.0 / std::max(0.1, (double)TP->type_rotatetime)) * PI2;
		float xv = sin(a) * TP->type_rotateshift;
		float yv = -cos(a) * TP->type_rotateshift;
		float u1 = TP->type_xscale + xv;
		float v1 = TP->type_yscale + yv;
		tileobj.Configure(TP->texture, TP->type_useenvtexture, TP->type_rendflags,
			xv, yv, u1, yv, u1, v1, xv, v1, col, col, col, fade);
		tileobj.Z = z;
		//
		tileobj.PerturbX += TP->type_perturbxv * VW->FrameFrac();
		tileobj.PerturbY += TP->type_perturbyv * VW->FrameFrac();
		tileobj.VertsX = TP->type_vertsx;
		tileobj.VertsY = TP->type_vertsy;
		tileobj.PerturbScale = TP->type_perturbscale;
		float t = Bias(TP->type_perturbbias, 1.0f - NiceT());
		tileobj.Perturb = TP->type_perturbstart * t + TP->type_perturbend * (1.0f - t);
		//
		VW->PolyRend->AddOrthoObject(&tileobj);
	}
	return EntityGUI::Think();
}

////////////////////////////////////////////////
//     GUI Text Button Entity
////////////////////////////////////////////////

EntityGUIButtonType::EntityGUIButtonType(ConfigFile *cfg, const char *c, const char *t) : EntityGUIType(cfg, c, t) {
//	type_clickable = 1;
	SetVec3(0, 1, 0, type_overcolor);
	SetVec3(0, 0, 1, type_clickcolor);
	type_overlinger = 1.0f;
	type_pulseamount = 1.0f;
	type_pulsetime = 3.0f;
	if(cfg){
		if(cfg->FindKey("fonttype")) cfg->GetStringVal(type_fonttype);
		if(cfg->FindKey("overcolor")) cfg->GetFloatVals(&type_overcolor[0], 3);
		if(cfg->FindKey("clickcolor")) cfg->GetFloatVals(&type_clickcolor[0], 3);
		if(cfg->FindKey("overlinger")) cfg->GetFloatVal(&type_overlinger);
		if(cfg->FindKey("pulseamount")) cfg->GetFloatVal(&type_pulseamount);
		if(cfg->FindKey("pulsetime")) cfg->GetFloatVal(&type_pulsetime);
	}
};
EntityBase *EntityGUIButtonType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUIButton(this, Pos, Rot, Vel, id, flags);
}
//
EntityGUIButton::EntityGUIButton(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntityGUI(et, Pos, Rot, Vel, id, flags) {
	EntityGUIButtonType *TP = (EntityGUIButtonType*)TypePtr;
	fontent = 0;
	EntityBase *e = VW->AddEntity("text", TP->type_fonttype, Position, Rotation, Velocity, 0, 0, 0, 0);
	//Argh!  Never blindly pass FLAGS between different entity classes...
	if(e){
		e->SetFloat(ATT_TEXT_OPACITY, 0.0f);
		fontent = e->GID;
	}
	offscreenx = (float)TMrandom() / (float)RAND_MAX;
	offscreeny = -0.4f + 1.8f * ((TMrandom() < RAND_MAX / 2) ? 0.0f : 1.0f);
	overstart = 0;
	overlinger = 0.0f;
	lastmouseover = 0;
	CopyVec3(Velocity, speccolor);
	xflip = yflip = 0;
}
EntityGUIButton::~EntityGUIButton(){
	if(fontent && VW){
		EntityBase *e = VW->GetEntity(fontent);
		if(e) e->Remove();
	}
}
bool EntityGUIButton::Think(){
	EntityGUIButtonType *TP = (EntityGUIButtonType*)TypePtr;
	Vec3 color, size;
	CopyVec3(Rotation, size);
	if(mouseover && (Flags & FLAG_BUTTON_DISABLED) == 0){
		if(clicked){
			CopyVec3(TP->type_clickcolor, speccolor);	//Changes font color.
		}else{
			CopyVec3(TP->type_overcolor, speccolor);
		}
		overlinger = TP->type_overlinger;
		if(lastmouseover == 0) overstart = VW->Time();
	}
	if(overlinger > 0.0f){	//This enhancement will make the over glow linger for an amount of time.
		float t = fabsf(sin(((double)(VW->Time() - overstart) / ((double)TP->type_pulsetime * 2000.0)) * PI2));
		t *= overlinger / MAX(0.01f, TP->type_overlinger);
		size[2] = 1.0f + TP->type_pulseamount * t;
		LerpVec3(Velocity, speccolor, t, color);
		overlinger -= VW->FrameFrac();
	}else{
		if(Flags & FLAG_BUTTON_DISABLED){
			ScaleVec3(Velocity, 0.7f, color);
		}else{
			CopyVec3(Velocity, color);
		}
	}
	lastmouseover = mouseover;
	//
	EntityBase *e = VW->GetEntity(fontent);
	if(e){
	//	if(niceremove){
	//		float t = ((float)niceremove / std::max(0.1f, TP->type_removetime * 1000.0f));
		//	e->SetFloat(ATT_TEXT_OPACITY, 1.0f - t);
		e->SetFloat(ATT_TEXT_OPACITY, NiceT());
		float t = 1.0f - NiceT();
		Vec3 tpos = {offscreenx, offscreeny, Position[2]};
		Vec3 tpos2 = {x, y, z};
		LerpVec3(tpos2, tpos, t * t, tpos);
		e->SetPos(tpos);
		e->SetVel(color);
		e->SetRot(size);
	}
	if(TP->texture && ((Flags & FLAG_BUTTON_HIDDEN) == 0) ){
		float fade = TP->type_fade * NiceT();
		float col = (TP->type_rendflags & SPRITE_BLEND_ADD ? fade : 1.0f);
		//
		int x1 = 1, y1 = 1;
		if(xflip) x1 = 0;
		if(yflip) y1 = 0;
		//
		tileobj.Configure(TP->texture, 0, TP->type_rendflags,
			1-x1, 1-y1, x1, 1-y1, x1, y1, 1-x1, y1, color[0] * col, color[1] * col, color[2] * col, fade);
		//	0, 0, 1, 0, 1, 1, 0, 1, color[0] * col, color[1] * col, color[2] * col, fade);
	//	float x = Position[0], y = Position[1], w = Rotation[0] * (float)text.len(), h = Rotation[1];
		tileobj.ConfigurePoly(x, y, x + w, y, x + w, y + h, x, y + h);
		tileobj.Z = z + 0.01f;
		VW->PolyRend->AddOrthoObject(&tileobj);
	}
	return EntityGUI::Think();
}
bool EntityGUIButton::SetString(int type, const char *s){
	if(s && type == ATT_TEXT_STRING){
		text = s;
		w = (float)text.len() * Rotation[0];
		h = Rotation[1];
		if(Flags & FLAG_TEXT_CENTERX){
			x = Position[0] - w * 0.5f;
		}
		if(Flags & FLAG_TEXT_CENTERY){
			y = Position[1] - h * 0.5f;
		}
		EntityBase *e = VW->GetEntity(fontent);
		if(e) return e->SetString(ATT_TEXT_STRING, s);
	}
	return EntityGUI::SetString(type, s);
}
CStr EntityGUIButton::QueryString(int type){
	if(type == ATT_TEXT_STRING) return text;
	return EntityGUI::QueryString(type);
}
bool EntityGUIButton::SetInt(int type, int att){
	if(type == ATT_BUTTON_XFLIP){ xflip = att; return true; }
	if(type == ATT_BUTTON_YFLIP){ yflip = att; return true; }
	return EntityGUI::SetInt(type, att);
}


////////////////////////////////////////////////
//     GUI Text Edit Entity
////////////////////////////////////////////////

EntityGUIEditType::EntityGUIEditType(ConfigFile *cfg, const char *c, const char *t) : EntityGUIButtonType(cfg, c, t) {
	type_cursorflash = 0.0f;
	if(cfg){
		if(cfg->FindKey("cursorflash")) cfg->GetFloatVal(&type_cursorflash);
	}
}
EntityBase *EntityGUIEditType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUIEdit(this, Pos, Rot, Vel, id, flags);
}
EntityGUIEdit::EntityGUIEdit(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
							 int id, int flags) : EntityGUIButton(et, Pos, Rot, Vel, id, flags) {
	flashms = 0;
	enterpressed = 0;
	caretpos = 0;
	VW->ClearChar();	//Clear key buffer on creation.
	VW->SetFocus(GID);	//And set focus to last edit entity created.
	maxlength = 31;
}
#define FAKETAIL1 "_                                                                  "
#define FAKETAIL2 "                                                                   "
bool EntityGUIEdit::Think(){
	EntityGUIEditType *TP = (EntityGUIEditType*)TypePtr;
	if(VW->GetFocus() == GID){
		char c = VW->GetChar();
		if(c){
			if(((isgraph(c) || c == ' ') && Flags == 0) ||
				((isdigit(c) || (c == '-' && Flags & FLAG_EDIT_NEGATIVE)) && Flags & FLAG_EDIT_INT) ||
				((isdigit(c) || (c == '-' && Flags & FLAG_EDIT_NEGATIVE) || c == '.') && Flags & FLAG_EDIT_FLOAT)
				)
			{
				if(strlen(realtext) < maxlength)
				{
					char b[2] = {c, 0};
					realtext = Left(realtext, caretpos) + b + Mid(realtext, caretpos);
					caretpos = MIN(realtext.len(), caretpos + 1);
					VW->SetActivated(GID);
				}
			}else{
				if(c == '\b'){
					if(caretpos > 0){
						realtext = Left(realtext, caretpos - 1) + Mid(realtext, caretpos);//realtext.len() - 1);
					}
					caretpos = MAX(0, caretpos - 1);
					VW->SetActivated(GID);
				}
				if(c == '\n' || c == '\r'){
					enterpressed = 1;
					VW->SetActivated(GID);
				}
				if(c == VWK_LEFT) caretpos = MAX(0, caretpos - 1);
				if(c == VWK_RIGHT) caretpos = MIN(realtext.len(), caretpos + 1);
			}
		}
	}
	CStr faketext;
	if(TP->type_cursorflash > 0.0f && VW->GetFocus() == GID){
		flashms += VW->FrameTime();
	}else{
		flashms = 0;
	}
	int fms = (int)(TP->type_cursorflash * 1000.0f);
	if(flashms > fms / 2){
		faketext = Left(realtext, caretpos) + "_" + Mid(realtext, caretpos + 1) + FAKETAIL2;
		if(flashms > fms) flashms = 0;
	}else{
		faketext = realtext + FAKETAIL2;
	}
	int pos = (headertext.len() + realtext.len() + 1) - ID;
	pos = MAX(0, MIN(pos, headertext.len() + caretpos - 1));
	EntityGUIButton::SetString(ATT_TEXT_STRING, Mid(headertext + faketext, std::max(0, pos), ID));
	return EntityGUIButton::Think();
}
float EntityGUIEdit::QueryFloat(int type){
	if(type == ATT_EDIT_FLOAT){
		return atof(realtext);
	}
	return EntityGUIButton::QueryFloat(type);
}
int EntityGUIEdit::QueryInt(int type){
	if(type == ATT_EDIT_INT){
		return atoi(realtext);
	}
	if(type == ATT_EDIT_ENTER){
		int t = enterpressed;
		enterpressed = 0;
		return t;
	}
	return EntityGUIButton::QueryInt(type);
}
bool EntityGUIEdit::SetInt(int type, int att){
	if(type == ATT_EDIT_MAXLENGTH)
		maxlength = std::max(att, 1);
	return EntityGUIButton::SetInt(type, att);
}
bool EntityGUIEdit::SetString(int type, const char *s){
	if(type == ATT_TEXT_STRING && s){
		if(caretpos >= realtext.len()) caretpos = strlen(s);	//If caret is at end of current string, push to end of newly set string.
		realtext = s;
		caretpos = CLAMP(caretpos, 0, realtext.len());
		return true;
	}
	if(type == ATT_EDIT_HEADER && s){
		headertext = s;
		return true;
	}
	return EntityGUIButton::SetString(type, s);
}
CStr EntityGUIEdit::QueryString(int type){
	if(type == ATT_TEXT_STRING) return realtext;
	return EntityGUIButton::QueryString(type);
}

////////////////////////////////////////////////
//     GUI Listbox Entity
////////////////////////////////////////////////

EntityGUIListboxType::EntityGUIListboxType(ConfigFile *cfg, const char *c, const char *t) : EntityGUIType(cfg, c, t) {
	type_clickable = 0;
	SetVec3(1, 0, 0, type_selectcolor);
	type_knobwidth = 2;
	if(cfg){
		if(cfg->FindKey("uptype")) cfg->GetStringVal(type_uptype);
		if(cfg->FindKey("downtype")) cfg->GetStringVal(type_downtype);
		if(cfg->FindKey("listtype")) cfg->GetStringVal(type_listtype);
		if(cfg->FindKey("knobtype")) cfg->GetStringVal(type_knobtype);
		if(cfg->FindKey("selectcolor")) cfg->GetFloatVals(&type_selectcolor[0], 3);
		if(cfg->FindKey("knobwidth")) cfg->GetIntVal(&type_knobwidth);
	}
};
EntityBase *EntityGUIListboxType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUIListbox(this, Pos, Rot, Vel, id, flags);
}
bool EntityGUIListboxType::CacheResources(){
	if(ResCached) return true;
	if(EntityGUIType::CacheResources()){
		VW->CacheResources("guibutton", type_uptype);
		VW->CacheResources("guibutton", type_downtype);
		VW->CacheResources("guibutton", type_knobtype);
	}
	return ResCached;
}
void EntityGUIListboxType::ListResources(FileCRCList *fl) {
	if (ResListed) return;
	EntityGUIType::ListResources(fl);
	ResListed = true;
	VW->ListResources("guibutton", type_uptype);
	VW->ListResources("guibutton", type_downtype);
	VW->ListResources("guibutton", type_knobtype);
}
//
EntityGUIListbox::EntityGUIListbox(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntityGUI(et, Pos, Rot, Vel, id, flags) {
	EntityGUIListboxType *TP = (EntityGUIListboxType*)TypePtr;
	upent = downent = knobent = 0;
	memset(listents, 0, sizeof(listents));
	EntityBase *e;
	e = VW->AddEntity("guibutton", TP->type_uptype, Position, Rotation, Velocity, ID, Flags, 0, 0);
	if(e) upent = e->GID;
	e = VW->AddEntity("guibutton", TP->type_downtype, Position, Rotation, Velocity, ID, Flags, 0, 0);
	if(e) downent = e->GID;
	e = VW->AddEntity("guibutton", TP->type_knobtype, Position, Rotation, Velocity, ID, Flags, 0, 0);
	if(e) knobent = e->GID;
	height = width = 0;
	selection = -1;
	scroll = 0;
}
EntityGUIListbox::~EntityGUIListbox(){
	if(VW){
		EntityBase *e;
		if(e = VW->GetEntity(upent)) e->Remove();
		if(e = VW->GetEntity(downent)) e->Remove();
		if(e = VW->GetEntity(knobent)) e->Remove();
		for(int i = 0; i < MAX_LISTBOX_HEIGHT; i++){
			if(e = VW->GetEntity(listents[i])) e->Remove();
			listents[i] = 0;
		}
	}
}
bool EntityGUIListbox::Think(){
	EntityGUIListboxType *TP = (EntityGUIListboxType*)TypePtr;
	char dummy[128];	//width space characters.
	for(int d = 0; d < 128; d++){
		if(d < width) dummy[d] = ' ';
		else dummy[d] = 0;
	}
	int upscrollpresent = 0;
	int listlen = listhead.CountItems(-1);
	EntityBase *e;// = VW->GetEntity(fontent);
	if(e = VW->GetEntity(upent)){
		e->SetPos(Position);
		e->SetRot(Rotation);
		e->SetVel(Velocity);
		e->SetString(ATT_TEXT_STRING, dummy);
		if(e->QueryInt(ATT_ACTIVATED)){
		//	scroll--;
			selection--;
			scroll = selection - height / 2;
			VW->SetActivated(GID);
		}
		upscrollpresent = 1;
	}
	if(e = VW->GetEntity(downent)){
		Vec3 t = {Position[0], Position[1] + Rotation[1] * (float)(height + upscrollpresent), Position[2]};
		e->SetPos(t);
		e->SetRot(Rotation);
		e->SetVel(Velocity);
		e->SetString(ATT_TEXT_STRING, dummy);
		if(e->QueryInt(ATT_ACTIVATED)){
		//	scroll++;
			selection++;
			scroll = selection - height / 2;
			VW->SetActivated(GID);
		}
	}
	selection = CLAMP(selection, -1, listlen - 1);
	scroll = CLAMP(scroll, 0, MAX(0, listlen - height));
	if(e = VW->GetEntity(knobent)){
		float scrollt = (float)scroll / (float)MAX(1, listlen - height);	//Percentage of scroll range scrolled.
		float scrollh = (float)height / (float)MAX(1, listlen);	//Percentage that viewport is of total list, i.e. knob size.
		scrollh = MIN(scrollh, 1.0f);
		float knobh = (Rotation[1] * (float)height) * scrollh;	//Actual screen height of knob.
		float iknobh = (Rotation[1] * (float)height) * (1.0f - scrollh);	//Inverse height of knob (i.e. empty space around it).
		Vec3 tp = {Position[0] + Rotation[0] * (float)(width - TP->type_knobwidth),
			Position[1] + Rotation[1] * (float)(upscrollpresent) + scrollt * iknobh,
			Position[2]};
		e->SetPos(tp);
		Vec3 tr = {Rotation[0] * (float)TP->type_knobwidth, knobh, Rotation[2]};	//Size knob 2 chars horizontally and proportionately vertically.
		e->SetRot(tr);
		e->SetVel(Velocity);
		e->SetString(ATT_TEXT_STRING, " ");
		if(e->QueryInt(ATT_CLICKED)){	//Knob is being clicked and/or dragged.
			scrollt = (VW->GetMouseY() - (Position[1] + Rotation[1] * (float)(upscrollpresent) + knobh * 0.5f)) /
				MAX(0.01f, iknobh);//Rotation[1] * (float)height);
			scroll = (int)(scrollt * (float)((listlen - height) + 1));
		}
	}
	scroll = CLAMP(scroll, 0, std::max(0, listlen - height));
	int oriscroll = scroll;
	ListboxEntry *le = listhead.FindItem(scroll + 1);
	for(int i = 0; i < height; i++){
		if(e = VW->GetEntity(listents[i])){
			Vec3 t = {Position[0], Position[1] + Rotation[1] * (float)(i + upscrollpresent), Position[2]};
			e->SetPos(t);
			e->SetRot(Rotation);
			Vec3 col;	//Multiply special listbox entry color, if any, with select color, or default color.
			CopyVec3(((i + oriscroll) == selection ? TP->type_selectcolor : Velocity), col);
			if(le) for(int j = 0; j < 3; j++) col[j] *= le->color[j];
			e->SetVel(col);
		//	e->SetVel(((i + oriscroll) == selection ? TP->type_selectcolor : Velocity));
			e->SetString(ATT_TEXT_STRING, (le ? le->get() : ""));
			if(e->QueryInt(ATT_ACTIVATED)){
				selection = i + oriscroll;
				scroll = selection - height / 2;
				VW->SetActivated(GID);
			}
		}
		if(le) le = le->NextLink();
	}
	//
	selection = CLAMP(selection, -1, listlen - 1);
	//
	return EntityGUI::Think();
}
bool EntityGUIListbox::SetInt(int type, int val){
	EntityGUIListboxType *TP = (EntityGUIListboxType*)TypePtr;
	if(type == ATT_CMD_LISTBOX_RESET){
	//	selection = -1;
		listhead.DeleteList();
		return true;
	}
	if(type == ATT_LISTBOX_HEIGHT){
		if(val != height){
			height = CLAMP(val, 0, MAX_LISTBOX_HEIGHT);
			EntityBase *e;
			for(int i = 0; i < MAX_LISTBOX_HEIGHT; i++){
				e = VW->GetEntity(listents[i]);
				if(i < height && e == 0){
					if(e = VW->AddEntity("guibutton", TP->type_listtype, Position, Rotation, Velocity, Flags, ID, 0, 0)){
						listents[i] = e->GID;
					}
				}else{
					if(e) e->Remove();
					listents[i] = 0;
				}
			}
		}
		return true;
	}
	if(type == ATT_LISTBOX_WIDTH){
		width = val;
		return true;
	}
	if(type == ATT_LISTBOX_SELECTION){
		if(selection != val && val >= 0){
			scroll = val - height / 2;
		}
		selection = val;
		return true;
	}
	if(type == ATT_LISTBOX_SCROLL){
		scroll = val;
		return true;
	}
	if(type == ATT_CMD_NICEREMOVE){
		EntityBase *e;
		if(e = VW->GetEntity(upent)) e->SetInt(ATT_CMD_NICEREMOVE, val);
		if(e = VW->GetEntity(downent)) e->SetInt(ATT_CMD_NICEREMOVE, val);
		if(e = VW->GetEntity(knobent)) e->SetInt(ATT_CMD_NICEREMOVE, val);
		for(int i = 0; i < MAX_LISTBOX_HEIGHT; i++){
			if(e = VW->GetEntity(listents[i])) e->SetInt(ATT_CMD_NICEREMOVE, val);
		}
		//Fall through.
	}
	return EntityGUI::SetInt(type, val);
}
int EntityGUIListbox::QueryInt(int type){
	if(type == ATT_LISTBOX_SELECTION) return selection;
	if(type == ATT_LISTBOX_SCROLL) return scroll;
	return EntityGUI::QueryInt(type);
}
bool EntityGUIListbox::SetString(int type, const char *s){
	if(s && type == ATT_LISTBOX_NEWENTRY){
		listhead.AddObject(new ListboxEntry(s));
		return true;
	}
	return EntityGUI::SetString(type, s);
}
CStr EntityGUIListbox::QueryString(int type){
	if(type == ATT_LISTBOX_SELECTION)
		return listhead.FindItem(selection+1)->get();
	return EntityGUI::QueryString(type);
}
bool EntityGUIListbox::SetVec(int type, const void *v){
	if(type == ATT_LISTBOX_ENTRYCOLOR && v && listhead.NextLink()){
		ListboxEntry *le = listhead.Tail();
		if(le){
			CopyVec3((float*)v, le->color);
			return true;
		}
	}
	return EntityGUI::SetVec(type, v);
}

////////////////////////////////////////////////
//     GUI Image Browser Entity
////////////////////////////////////////////////

EntityGUIImageBrowserType::EntityGUIImageBrowserType(ConfigFile *cfg, const char *c, const char *t) : EntityGUIButtonType(cfg, c, t) {
	type_bpp = 8;

	if(cfg){
		if(cfg->FindKey("bpp")) cfg->GetIntVal(&type_bpp);
	}
}
EntityBase *EntityGUIImageBrowserType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUIImageBrowser(this, Pos, Rot, Vel, id, flags);
}
bool EntityGUIImageBrowserType::CacheResources(){
	if(ResCached) return true;
	texture = VW->GetCreateImage(cname + "_" + tname, (int)(type_w + 0.5f), (int)(type_h + 0.5f), type_bpp, true);
	if(texture){
		texture->Dynamic = true;	//Prevents texture compression.
		return ResCached = true;	//This _shouldn't_ fail...  But if it does, call parent's rescache.
	}
	return EntityGUIButtonType::CacheResources();
}
void EntityGUIImageBrowserType::ListResources(FileCRCList *fl) {
	if (ResListed) return;
	ResListed = true;
	// FIXME!
	//EntityGUIButtonType::ListResources(fl);
}
//
EntityGUIImageBrowser::EntityGUIImageBrowser(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
							 int id, int flags) : EntityGUIButton(et, Pos, Rot, Vel, id, flags) {
	SetString(ATT_TEXT_STRING, " ");	//So the "font character width and height" will specify the image size.
	bHFlip = false;						// default to not reversing the image
}
bool EntityGUIImageBrowser::Think(){
	EntityGUIImageBrowserType *TP = (EntityGUIImageBrowserType*)TypePtr;
//	EntityGUIButton::SetString(ATT_TEXT_STRING, Mid(headertext + faketext, std::max(0, pos), ID));

	TP->texture->Trans = 1;
	//TP->texture->

	return EntityGUIButton::Think();
}
bool EntityGUIImageBrowser::SetInt(int type, int att)
{
	if(type == ATT_BROWSER_HFLIP)
	{
		bHFlip = (att != 0);

		return true;
	}
	else return EntityGUIButton::SetInt(type, att);
}

bool EntityGUIImageBrowser::SetString(int type, const char *s){
	EntityGUIImageBrowserType *TP = (EntityGUIImageBrowserType*)TypePtr;
	if(type == ATT_BROWSER_SETIMAGE && s){	//Only do all this if it's a new image file name.

		 // if the image name is blank, hide the element
		if(imagename == "")
			Flags |= FLAG_BUTTON_HIDDEN;

		if(imagename == s) return true;	//Image already set (or tried to be set).
		imagename = s;
		ImageNode *img = VW->GetImage(imagename);	//Cache the image.
		if(img){
			img->flags |= BFLAG_DONTDOWNLOAD;	//Ask for it NOT to be sent to the 3D API.
			if(TP->texture){

				Flags &= ~FLAG_BUTTON_HIDDEN;

				// if the texture is not the same size as the gui ent than resize it
				if( (img->Width() != (int) TP->type_w) || (img->Height() != (int) TP->type_h) )
					img->Scale((int) TP->type_w, (int) TP->type_h);

				// if not flipped then flip it
				if( bHFlip && (img->flags & BFLAG_HFLIPPED) == 0 )
					img->FlipH();

				TP->texture->Suck(img);	//Suck cached image into display texture.
				TP->texture->SetPalette(img->pe);
				TP->texture->flags |= BFLAG_WANTDOWNLOAD;	//Ask for it to be redownloaded.
				return true;
			}
		}

		return false;	//Failure!
	}
	return EntityGUIButton::SetString(type, s);
}

////////////////////////////////////////////////
//     GUI Mesh Entity
////////////////////////////////////////////////

EntityGUIMeshType::EntityGUIMeshType(ConfigFile *cfg, const char *c, const char *t) : EntityGUIType(cfg, c, t) {
	if(cfg){
		ReadMeshRendFlags(cfg, type_rendflags);
		if(cfg->FindKey("mesh")) cfg->GetStringVal(type_meshfile);
		if(cfg->FindKey("scale")) cfg->GetFloatVal(&type_scale);
	}
}
EntityBase *EntityGUIMeshType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUIMesh(this, Pos, Rot, Vel, id, flags);
}
bool EntityGUIMeshType::CacheResources(){
	if(ResCached) return true;
	if(EntityGUIType::CacheResources()){
		if(type_meshfile.len() <= 0) return ResCached;	//If texture/sprite caches OK and no mesh specified, return successful rescache.
		type_mesh = VW->GetMesh(type_meshfile, type_scale, (type_rendflags & (MESH_SHADE_SMOOTH | MESH_ENVMAP_SMOOTH)) != 0);
		if(texture && type_mesh){
			type_mesh->SetLod(0, 1.0, NULL);
			return ResCached = true;
		}
	}
	return ResCached = false;
}
void EntityGUIMeshType::ListResources(FileCRCList *fl) {
	if (ResListed) return;
	EntityGUIType::ListResources(fl);
	ResListed = true;
	if(type_meshfile.len() > 0) fl->FileCRC(type_meshfile);
}
void EntityGUIMeshType::UnlinkResources(){
	EntityGUIType::UnlinkResources();
	type_mesh = NULL;
	ResCached = false;
	ResListed = false;
}
//
EntityGUIMesh::EntityGUIMesh(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntityGUI(et, Pos, Rot, Vel, id, flags) {
	EntityGUIMeshType *TP = (EntityGUIMeshType*)TypePtr;
	fade = TP->type_fade;
}
EntityGUIMesh::~EntityGUIMesh(){
}
bool EntityGUIMesh::Think(){
	EntityGUIMeshType *TP = (EntityGUIMeshType*)TypePtr;
	if(!TP->type_mesh) return false;
	Rot3ToMat3(Rotation, orthomeshobj.Orient);
//	ScaleVec3(orthomeshobj.Orient[1], -1.0f);
	//
	orthomeshobj.Configure(TP->texture, TP->type_mesh, orthomeshobj.Orient, Position,
		TP->type_rendflags, fade, TP->type_mesh->BndRad, 1.0f);
	orthomeshobj.Z = Position[2];
	VW->PolyRend->AddOrthoObject(&orthomeshobj);
	return EntityGUI::Think();
}

////////////////////////////////////////////////
//     GUI Mouse Pointer Entity
////////////////////////////////////////////////

EntityGUIMouseType::EntityGUIMouseType(ConfigFile *cfg, const char *c, const char *t) : EntityGUIMeshType(cfg, c, t) {
	ClearVec3(type_rotatespeed);
	ClearVec3(type_rotation);
	if(cfg){
		if(cfg->FindKey("rotatespeed")) cfg->GetFloatVals(&type_rotatespeed[0], 3);
		if(cfg->FindKey("rotation")) cfg->GetFloatVals(&type_rotation[0], 3);
		for(int i = 0; i < 3; i++){
			type_rotatespeed[i] *= DEG2RAD;
			type_rotation[i] *= DEG2RAD;
		}
	}
}
EntityBase *EntityGUIMouseType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityGUIMouse(this, Pos, Rot, Vel, id, flags);
}
//
EntityGUIMouse::EntityGUIMouse(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntityGUIMesh(et, Pos, Rot, Vel, id, flags) {
	EntityGUIMouseType *TP = (EntityGUIMouseType*)TypePtr;
	SetRot(TP->type_rotation);
}
bool EntityGUIMouse::Think(){
	EntityGUIMouseType *TP = (EntityGUIMouseType*)TypePtr;
	if(niceremove) fade = NiceT();
	ScaleAddVec3(TP->type_rotatespeed, VW->FrameFrac(), Rotation);
	NormRot3(Rotation);
	Position[0] = VW->GetMouseX();
	Position[1] = VW->GetMouseY();
	Position[2] = -10.0f;
	return EntityGUIMesh::Think();
}



