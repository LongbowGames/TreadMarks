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

//Particle Fire effect Entity.

#include "EntityPFire.h"
#include "VoxelWorld.h"

#ifdef WIN32
#define strcasecmp(a, b) _stricmp(a, b)
#endif

//*****************************************************************
//**  Global entity type setup function.  Must be modified to
//**  handle all available entity classes.
//*****************************************************************
extern EntityTypeBase *CreatePFireType(ConfigFile *cfg){
	if(!cfg) return 0;
	char superclass[256], subclass[256];
	if(cfg->FindKey("class") && cfg->GetStringVal(superclass, 256)){
		if(cfg->FindKey("type") && cfg->GetStringVal(subclass, 256)){
			if(strcasecmp(superclass, "particlefire") == 0) return new EntityPFireType(cfg, superclass, subclass);
		}
	}
	return 0;
}

//This function MUST be called ONCE by your main app to register the GUI classes.
int RegisterPFireEntities(){
	return RegisterEntityTypeCreator(CreatePFireType);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int InitFire(int particles, int w, int h, int scheme, int flamehalf = 0, int gt = 20);
int UninitFire();
void DoFire(void *bData, int bWidth, int bHeight, int bPitch);

PaletteEntry	pe[256], cf[256], ct[256];
bool Explode = false;
int ExplodeX = 0, ExplodeY = 0;
int XMouse = 0;
int YMouse = 0;
bool FollowMouse = false;
int UseFadeZoom = 0;
int CycleColors = 1;

#define NUMSCHEMES 5
const char ColorName[NUMSCHEMES][100] = {
	"Fiery Orange",
	"Skyish Teal",
	"Velvet Blue",
	"Slimy Green",
	"Burning Pink"
};

float MovementScale = 0.7f;
float AttractScale = 1.0f;
//Amount subtracted from each averaged pixel each time through the burn.  Fade out speed.
#define BURNFADE 24

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////
//     Particle Fire effect entity.
////////////////////////////////////////////////

EntityPFireType::EntityPFireType(ConfigFile *cfg, const char *c, const char *t) : EntityTypeBase(cfg, c, t) {
	tex1 = tex2 = NULL;
	type_size = 256;
	type_updaterate = 0.2f;
	type_particles = 100;
	if(cfg){
		if(cfg->FindKey("updaterate")) cfg->GetFloatVal(&type_updaterate);
		if(cfg->FindKey("buffersize")) cfg->GetIntVal(&type_size);
		if(cfg->FindKey("particles")) cfg->GetIntVal(&type_particles);
	}
}
EntityBase *EntityPFireType::CreateEntity(Vec3 Pos, Rot3 Rot, Vec3 Vel, int id, int flags){
	return new EntityPFire(this, Pos, Rot, Vel, id, flags);
}
bool EntityPFireType::CacheResources(){
	if(ResCached) return true;
	tex1 = VW->GetCreateImage("PFireTex1", type_size, type_size, 8, false, false, ALPHAGRAD_NONE);
	tex2 = VW->GetCreateImage("PFireTex2", type_size, type_size, 8, false, false, ALPHAGRAD_NONE);
	if(tex1 && tex2){
		tex1->Clear();
		tex1->Dynamic = true;
		tex2->Clear();
		tex2->Dynamic = true;
		InitFire(type_particles, tex1->Width(), tex1->Height(), 0);
		return ResCached = true;
	}
	return ResCached = false;
}
void EntityPFireType::ListResources(FileCRCList *fl){
	if (ResListed) return;
	ResListed = true;
}
void EntityPFireType::UnlinkResources(){
	tex1 = tex2 = NULL;
	ResCached = false;
	ResListed = false;
}
//
EntityPFire::EntityPFire(EntityTypeBase *et, Vec3 Pos, Rot3 Rot, Vec3 Vel,
		int id, int flags) : EntityBase(et, Pos, Rot, Vel, id, flags) {
	EntityPFireType *TP = (EntityPFireType*)TypePtr;
	if(!RegisterEntity("ParticleFireBurner")) Remove();
	//Only allow one particle fire entity at a time.
	mssinceping = 0;
	mssinceburn = 0;
	follow = false;
	followx = 0.5f;
	followy = 0.5f;
}
bool EntityPFire::Think(){
	EntityPFireType *TP = (EntityPFireType*)TypePtr;
	//
	if(TP->tex1 && TP->tex2 && TP->tex1->Data() && TP->tex2->Data()){
		int urms = (TP->type_updaterate * 1000.0f);
		mssinceburn += VW->FrameTime();
		if(mssinceping < PFirePingTimeout && mssinceburn >= urms){
			mssinceping += VW->FrameTime();
			mssinceburn -= urms;
			FollowMouse = follow;
			XMouse = (followx * (float)TP->type_size);
			YMouse = (followy * (float)TP->type_size);
			DoFire(TP->tex1->Data(), TP->tex1->Width(), TP->tex1->Height(), TP->tex1->Pitch());
			TP->tex1->SetPalette(pe);
			TP->tex1->flags |= BFLAG_WANTDOWNLOAD;
			VW->PolyRend->SetEnvMap2(TP->tex1);
		}
		if(mssinceping < 0) mssinceping = 0;
	}
	//
	return true;
}
int EntityPFire::QueryInt(int type){
	return 0;
}
bool EntityPFire::SetInt(int type, int val){
	EntityPFireType *TP = (EntityPFireType*)TypePtr;
	if(type == ATT_PFIRE_PING && val){
		mssinceping = 0;	//Keep 'er burnin', baby!
		return true;
	}
	if(type == ATT_PFIRE_FOLLOW){
		follow = (val != 0);
		return true;
	}
	if(type == ATT_PFIRE_EXPLODE && val){
		Explode = true;
		if(follow){
			ExplodeX = (followx * (float)TP->type_size);
			ExplodeY = (followy * (float)TP->type_size);
		}
		return true;
	}
	return false;
}
bool EntityPFire::SetFloat(int type, float att){
	if(type == ATT_PFIRE_FOLLOWX){
		followx = att;
		return true;
	}
	if(type == ATT_PFIRE_FOLLOWY){
		followy = att;
		return true;
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////


enum{
	ATTRACT_NONE = 0,
	ATTRACT_GRAVITY = 1,
	ATTRACT_ANGLE = 2
};

struct Particle{
	double x, y, dx, dy, lx, ly;	//Particle position, velocity, and last position.
	double ax, ay;	//Particle's attractor location.
	int attract;	//Flag to use attractor or not.
	int color;	//Particle color.
};


///////////////////////////////////////////////////////////////////////////////
// This is a hacked up copy of the Particle Fire source for use in DX-Ball 2
///////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <time.h>
#include <stdio.h>
//#define VC_EXTRALEAN
//#define WIN32_EXTRA_LEAN
//#define WIN32_LEAN_AND_MEAN
//#include <commctrl.h>
//#include <BackBuffer.h>
//#include <CDib.h>
//#include "ParticleFire.h"
#include "Timer.h"
#include "Basis.h"
//#include <Reg.h>
//#include <scrnsave.h>
//#include "resource.h"
#include "CStr.h"
#include "Image.h"

//#include "SerialNum.h"

//Registry REG("Longbow Digital Arts", "Particle Fire");

//Remove this define to return to plain colored particles.
#define BURN

//#define BURNFADE 96
//#define BURNFADE 12

//Number of particles.
#define MAX_PART 1000
int nParticles = 300;
//Amount of lateral kick when bouncing.
#define KICK_STRENGTH 0.5
//For ordinary particles, how many levels (of 64) to fade pixels per frame.
#define FADE_SPEED 4
//Strength of gravity.
#define GRAVITY 0.1
//Number of seconds between gravity direction shifts.
int GRAV_TIME = 20;
//40
//Amount of energy retained on bounce.
#define BOUNCE 0.95
//One in X chance of an effect happening every second.
int RANDEFFECT = 4;
//Color palette.
int ColorScheme = 0;

double FlameSpeed = 0.03;	//Each pixel is this many units in Noise space.
Basis basis(0);

int WIDTH, HEIGHT;

//Number of pixels to not burn on edges of screen.
#define XOFF 5
#define YOFF 3
#define BXOFF 4
#define BX4OFF (BXOFF / 4)
#define BYOFF 2
//New for improved burn code, BXOFf must be multiple of 4, and XOFF must be greater still.

//Timer tmr2;
int Burns = 0, BurnTimes = 0, KindleTimes = 0, ParticleTimes = 0, BlitTimes = 0;

unsigned char BurnFlags[2048];

Particle		p[MAX_PART];
//HWND			hwnd, dwnd;
unsigned char	fade[256], half[256];

int resolution = 1;
bool			NoiseBurn = false, Follow = true, MultipleFollow = true, UseGravity = true;
bool			ShakeUp = false, Freeze = false, UseRandom = true;
bool			Comet = false, Emit = false;
bool InnerRing = false;
int EmitRotate;
bool			Cube3D = true, Attract = true;
int				EmitCount = 0;//, MouseX, MouseY;
double			xgrav, ygrav;
time_t			TimeStart, Time, LastTime;
bool BurnDown = false;
bool RandomColor = false;

double IdentityAngle = 3.0;

void SpreadPal(PaletteEntry *pe1, PaletteEntry *pe2, PaletteEntry *dst){
	if(pe1 && pe2 && dst){
		for(int i = 0; i < 256; i++){
			if(i < 128){
				float t = ((float)i) / 127.0f;
				dst[i].peRed = (int)(((float)pe1->peRed) * t);
				dst[i].peGreen = (int)(((float)pe1->peGreen) * t);
				dst[i].peBlue = (int)(((float)pe1->peBlue) * t);
			}else{
				float t = (float)(i - 127) / 128.0f;
				float it = 1.0f - t;
				dst[i].peRed = (int)((float)pe1->peRed * it + (float)pe2->peRed * t);
				dst[i].peGreen = (int)((float)pe1->peGreen * it + (float)pe2->peGreen * t);
				dst[i].peBlue = (int)((float)pe1->peBlue * it + (float)pe2->peBlue * t);
			}
		}
	}
}
void Palette(int ColorScheme){
	//Create palette.
	PaletteEntry p1, p2;
	switch(ColorScheme){
	case -1 :
//		p1 = CustomPE1;
//		p2 = CustomPE2;
		break;
	case 0 :
		p1.peRed = 255; p1.peGreen = 128; p1.peBlue = 0;
		p2.peRed = 255; p2.peGreen = 255; p2.peBlue = 0;
		break;
	case 1 :
		p1.peRed = 0; p1.peGreen = 128; p1.peBlue = 255;
		p2.peRed = 0; p2.peGreen = 255; p2.peBlue = 255;
		break;
	case 2 :
		p1.peRed = 64; p1.peGreen = 64; p1.peBlue = 128;
		p2.peRed = 192; p2.peGreen = 192; p2.peBlue = 255;
		break;
	case 3 :
		p1.peRed = 32; p1.peGreen = 128; p1.peBlue = 32;
		p2.peRed = 160; p2.peGreen = 255; p2.peBlue = 160;
		break;
	case 4 :
		p1.peRed = 255; p1.peGreen = 64; p1.peBlue = 64;
		p2.peRed = 255; p2.peGreen = 192; p2.peBlue = 192;
		break;
	}
	SpreadPal(&p1, &p2, pe);
}
/*
void Palette(int ColorScheme){
	//Create palette.
	int i;
	if(ColorScheme == 0){
		for(i = 0; i < 128; i++){
			pe[i].peRed = i * 2;  pe[i].peGreen = i;  pe[i].peBlue = 0;  }
		for(i = 128; i < 256; i++){
			pe[i].peRed = 255;  pe[i].peGreen = i;  pe[i].peBlue = 0;  }
	}
	if(ColorScheme == 1){
		for(i = 0; i < 128; i++){
			pe[i].peRed = 0;  pe[i].peGreen = i;  pe[i].peBlue = i * 2;  }
		for(i = 128; i < 256; i++){
			pe[i].peRed = 0;  pe[i].peGreen = i;  pe[i].peBlue = 255;  }
	}
	if(ColorScheme == 2){
		for(i = 0; i < 128; i++){
			pe[i].peRed = i / 2;  pe[i].peGreen = i / 2;  pe[i].peBlue = i;  }
		for(i = 128; i < 256; i++){
			pe[i].peRed = i - 64;  pe[i].peGreen = i - 64;  pe[i].peBlue = i;  }
	}
	if(ColorScheme == 3){
		for(i = 0; i < 128; i++){
			pe[i].peRed = i / 4;  pe[i].peGreen = i;  pe[i].peBlue = i / 4;  }
		for(i = 128; i < 256; i++){
			pe[i].peRed = i - 96;  pe[i].peGreen = i;  pe[i].peBlue = i - 96;  }
	}
	if(ColorScheme == 4){
		for(i = 0; i < 128; i++){
			pe[i].peRed = i * 2;  pe[i].peGreen = i / 2;  pe[i].peBlue = i / 2;  }
		for(i = 128; i < 256; i++){
			pe[i].peRed = 255;  pe[i].peGreen = i - 64;  pe[i].peBlue = i - 64;  }
	}
	pe[255].peRed = pe[255].peGreen = pe[255].peBlue = 255;
	for(i = 0; i < 32; i++){
		int c = 224 + i;
		int a = i + 1;
		int ia = 32 - a;
		pe[c].peRed = ((pe[c].peRed * ia) + (255 * a)) >>5;
		pe[c].peGreen = ((pe[c].peGreen * ia) + (255 * a)) >>5;
		pe[c].peBlue = ((pe[c].peBlue * ia) + (255 * a)) >>5;
	}
}
*/

bool Realized = false;
bool Preview = false;

int BlankedSecs = 0;
int TotalSecs = 0;

long int FirstUseTime = 0;

long int SecsStart = 0;

int FlameHalf = 0;

int InitFire(int particles, int w, int h, int scheme, int flamehalf, int gt){
	FlameHalf = flamehalf;
	WIDTH = w;
	HEIGHT = h;
	ColorScheme = scheme;
	Palette(scheme);
	nParticles = particles;
	GRAV_TIME = gt;
	int i;
	for(i = 0; i < 256; i++){
	//	fade[i] = std::max(i & ~63, i - FADE_SPEED);
		half[i] = i >>1;//std::max(i & ~63, i & ~63 + (int)((i & 63) / 1.5));
	}
	//Initialize particles.
	for(i = 0; i < MAX_PART; i++){
		p[i].x = p[i].lx = XOFF + rand() % (WIDTH - XOFF * 2);
		p[i].y = p[i].ly = YOFF + rand() % (HEIGHT - YOFF * 2);
		p[i].dx = p[i].dy = 0.0;
		p[i].color = rand() % 127 + 127;
	}
	TimeStart = LastTime = Time = time(NULL);
//	SetTimer(hwnd, 1, 50, NULL);
	//
	Follow = rand() & 1;
	MultipleFollow = rand() & 1;
	NoiseBurn = rand() & 1;
	UseGravity = rand() & 1;
	//
	return 1;
}

int UninitFire(){
	return 1;
}

//Returns a random double between -range and range.
double frand(double range){
	return ((double)((rand() % 10000) - 5000) * range) / 5000.0;
}

int Frames = 0;
float CycleT = 1.1f;

void DoFire(void *bData, int bWidth, int bHeight, int bPitch){
	//
	Frames++;
	if(CycleColors && Frames % 10 == 0){	//Every half second.
		CycleT += 0.005f;
		if(CycleT >= 1.0f){
			memcpy(cf, pe, sizeof(pe));	//Put current colors in from spot.
			//
			if(RandomColor) ColorScheme = rand() % (NUMSCHEMES);// + 1) - 1;
			else ColorScheme++;
			if(ColorScheme >= NUMSCHEMES) ColorScheme = 0;//-1;
			//
			Palette(ColorScheme);//rand() % (NUMSCHEMES + 1) - 1);
			memcpy(ct, pe, sizeof(pe));	//Destination palette.
			CycleT = 0.0f;
		}
		float t = CycleT, it = 1.0f - t;
		for(int i = 1; i < 255; i++){
			pe[i].peRed = (int)((float)cf[i].peRed * it + (float)ct[i].peRed * t);
			pe[i].peGreen = (int)((float)cf[i].peGreen * it + (float)ct[i].peGreen * t);
			pe[i].peBlue = (int)((float)cf[i].peBlue * it + (float)ct[i].peBlue * t);
		}
	//	dib.SetPalette(pe);
	//	dib.RealizePalette();
	}
	//
	int iseconds = time(NULL) - FirstUseTime;	//Installed seconds.
	int iminutes = iseconds / 60;
	int ihours = iminutes / 60;
	int idays = ihours / 24;
	int sseconds = (time(NULL) - SecsStart);	//Session Seconds.
	int bseconds = sseconds + TotalSecs;	//Blanked seconds.
	int bminutes = bseconds / 60;
	int bhours = bminutes / 60;
	//
	//
	int i, j;//, l;
	unsigned char *tdata, *tdata2;
	LastTime = Time;
	Time = time(NULL);
	//Gravity direction, based on time.
	switch(((Time - TimeStart) / GRAV_TIME) % 4){
		case 0 :  xgrav = 0;  ygrav = GRAVITY;  BurnDown = false;  break;
		case 1 :  xgrav = -GRAVITY;  ygrav = 0;  break;
		case 2 :  xgrav = 0;  ygrav = -GRAVITY;  BurnDown = true;  break;
		case 3 :  xgrav = GRAVITY;  ygrav = 0;  break;
	}
	//Randomly cause particle effects to happen.
	if(Time > LastTime && UseRandom){
		if(rand() % RANDEFFECT == 0){
			switch((rand() >>3) % 13){
				case 0 : ShakeUp = true;  break;
				case 1 : Freeze = true;  break;
				case 2 : Comet = true;  break;
				case 3 : Emit = true;  EmitRotate = (rand() % 3) - 1;  break;
				case 4 :
				case 5 :
				case 6 : Explode = true;  break;
				case 7 : Follow = !Follow;  break;
				case 8 : MultipleFollow = !MultipleFollow;  break;
				case 9 : NoiseBurn = !NoiseBurn;  break;
				case 10 : UseGravity = !UseGravity;  break;
				case 11 :
				case 12 : InnerRing = true;  break;
			}
			if(Follow == false && (rand() % 4) == 0){
				FollowMouse = true;
				XMouse = rand() % WIDTH;
				YMouse = rand() % HEIGHT;
			}else{
				FollowMouse = false;
			}
		}
	}
	//Shake particles up when left button pressed.
	if(InnerRing){
		double velocity = fabs(frand(10.0)) + 2.0;
		double pvel, angle;
		int exprob = (rand() % 3) + 1;
		float size = fabs(frand(0.5)) + 0.4;
		float w2 = (float)WIDTH * size * 0.5f;
		float h2 = (float)HEIGHT * size * 0.5f;
		int sunburst = rand() & 1;
		int rot = rand() % 4;
		float rx = (float)WIDTH / 2.0f + frand((float)WIDTH / 2.0f - w2);
		float ry = (float)HEIGHT / 2.0f + frand((float)HEIGHT / 2.0f - h2);
		for(i = 0; i < nParticles; i++){
			if(i % exprob == 0){
				angle = frand(3.14159);
				p[i].x = rx + sin(angle) * h2;
				p[i].y = ry - cos(angle) * h2;
				if(sunburst){
					pvel = velocity * 0.5f;
				}else{
					pvel = fabs(frand(velocity));
				}
				if(rot == 1) pvel = 0.0f;	//Frozen ring.
				if(rot == 2) angle += 3.14159f * 0.5f;	//0 produces no rotation.
				if(rot == 3) angle -= 3.14159f * 0.5f;
				p[i].dx = -sin(angle) * pvel;
				p[i].dy = cos(angle) * pvel;
//				if(AltColor) p[i].color = rand() % 84 + 170;
			}
		}
		InnerRing = false;
	}
	if(ShakeUp){
		for(i = 0; i < nParticles; i++){
			p[i].dx -= xgrav * 40;
			p[i].dy -= ygrav * 40;
		}
		ShakeUp = false;
	}
	//Freeze particles when right button pressed.
	if(Freeze){
		for(i = 0; i < nParticles; i++){
			p[i].dx = 0;
			p[i].dy = 0;
		}
		Freeze = false;
	}
	//Explode particles when space bar pressed.
	if(Explode){
		double velocity = fabs(frand(9.0)) + 3.0;
		double ex, ey;
		int sunburst = rand() & 1;
		if(ExplodeX == 0 || ExplodeY == 0){
			ex = XOFF + rand() % (WIDTH - XOFF * 2);
			ey = YOFF + rand() % (HEIGHT - YOFF * 2);
		}else{
			ex = (double)std::min(std::max(ExplodeX, XOFF), WIDTH - XOFF - 1);
			ey = (double)std::min(std::max(ExplodeY, YOFF), HEIGHT - YOFF - 1);
		}
		ExplodeX = ExplodeY = 0;
		double pvel, angle;
		int exprob = (rand() % 3) + 1;
		for(i = 0; i < nParticles; i++){
			if(i % exprob == 0){
				p[i].x = ex;
				p[i].y = ey;
				if(sunburst){
					pvel = velocity * 0.5f;
				}else{
					pvel = fabs(frand(velocity));
				}
				angle = frand(3.14159);
				p[i].dx = cos(angle) * pvel;
				p[i].dy = sin(angle) * pvel;
//				if(AltColor) p[i].color = rand() % 84 + 170;
			}
		}
		Explode = false;
	}
	//Create a comet particle when enter is pressed.
	if(Comet){
		double velocity = fabs(frand(8));
		double ex = XOFF + rand() % (WIDTH - XOFF * 2);
		double ey = YOFF + rand() % (HEIGHT - YOFF * 2);
		double angle = frand(3.14159);
		for(i = 0; i < nParticles; i++){
			p[i].x = ex + frand(1.0);
			p[i].y = ey + frand(1.0);
			p[i].dx = cos(angle) * velocity;
			p[i].dy = sin(angle) * velocity;
//			if(AltColor) p[i].color = rand() % 84 + 170;
		}
		Comet = false;
	}
	//Drop particles from center of screen when backspace pressed.
	if(Emit) EmitCount = nParticles;
	if(EmitCount > 0){
		float angscale = (3.14159f * 2.0f) / ((float)nParticles / 4.0f);
		if(EmitRotate < 0) angscale = -angscale;
		i = EmitCount;
		EmitCount = std::max(EmitCount - std::max(nParticles / 100, 1), 0);
		while(i-- > EmitCount){
			p[i].x = WIDTH / 2;
			p[i].y = HEIGHT / 2;
			if(EmitRotate != 0){
				p[i].dx = sin((float)i * angscale) * 4.0f;
				p[i].dy = -cos((float)i * angscale) * 4.0f;
			}else{
				p[i].dx = 0;
				p[i].dy = 0;
			}
//			if(AltColor) p[i].color = rand() % 84 + 170;
		}
		Emit = false;
	}
	static double cubex, cubey, cubez, cubesize = 100;
	static int cppe = 10;
	if(Cube3D){
	}
	int tt = (MultipleFollow ? 63 : 0x7fff);
	if(Attract || FollowMouse){
		for(i = 0; i < nParticles; i++){
			if(Follow && i & tt){
				if(FollowMouse && ((i & tt) == 0)){
					p[i].ax = (double)XMouse;
					p[i].ay = (double)YMouse;
				}else{
					p[i].ax = p[i & ~tt].x;
					p[i].ay = p[i & ~tt].y;
				}
				p[i].attract = ATTRACT_ANGLE | (UseGravity ? ATTRACT_GRAVITY : 0);
			}else{
				if(FollowMouse){
					p[i].ax = (double)XMouse;
					p[i].ay = (double)YMouse;
					p[i].attract = ATTRACT_ANGLE | (UseGravity ? ATTRACT_GRAVITY : 0);
				}else{
					p[i].attract = ATTRACT_NONE | (UseGravity ? ATTRACT_GRAVITY : 0);
				}
			}
		}
	}

	IdentityAngle += 0.01;
	if(IdentityAngle > 3.14159) IdentityAngle -= 3.14159 * 2.0;

	//Lock buffer.
//	if(buf.Lock(&bdesc)){
	if(bData){//dib.Data()){
		//Draw particles.
		unsigned char *data, *data2;
		int pitch;
		int d, e, dir, x, y, dx, dy;
		data = (unsigned char*)bData;//dib.Data();//bdesc.data;
		pitch = bPitch;//dib.Pitch();//bdesc.pitch;
		unsigned char *bdescdata = (unsigned char*)bData;//dib.Data();
		int bdescpitch = bPitch;//dib.Pitch();
		int bdescwidth = bWidth;//dib.Width();
		int bdescheight = bHeight;//dib.Height();
		double adx, ady, dist, dist2, ang, angd;

//		tmr2.Start();

		for(i = 0; i < nParticles; i++){
			//
//			if(AltColor){
//				p[i].color -= 1;
//				if(p[i].color <= 128) p[i].color += 32;
//			}
			//
			p[i].lx = p[i].x;
			p[i].ly = p[i].y;
			if(p[i].attract & ATTRACT_GRAVITY){
				p[i].dx += xgrav;
				p[i].dy += ygrav;
			}
			if(p[i].attract & ATTRACT_ANGLE){
				adx = p[i].ax - p[i].x;
				ady = p[i].ay - p[i].y;
				dist = sqrt(adx * adx + ady * ady);
				dist2 = sqrt(p[i].dx * p[i].dx + p[i].dy * p[i].dy);
				if(dist == 0.0) dist = 2.0;
				if(dist2 == 0.0) dist2 = 2.0;
				if(p[i].dx == 0.0 && p[i].dy == 0.0){
					ang = IdentityAngle;
				}else{
					ang = atan2(p[i].dy, p[i].dx);
				}
				if(adx == 0.0 && ady == 0.0){
					angd = IdentityAngle - ang;
				}else{
					angd = atan2(ady, adx) - ang;
				}
				if(angd > 3.14159) angd -= 3.14159 * 2;
				if(angd < -3.14159) angd += 3.14159 * 2;
				ang += angd * 0.05 * AttractScale;
				p[i].dx = cos(ang) * dist2;
				p[i].dy = sin(ang) * dist2;
			}
			p[i].x += p[i].dx * MovementScale;
			p[i].y += p[i].dy * MovementScale;
			//Edge collisions.
			if(p[i].x < XOFF){
				p[i].x = XOFF;
				p[i].dx = fabs(p[i].dx) * BOUNCE;
				p[i].dy += frand(KICK_STRENGTH);
//				if(AltColor) p[i].color = std::min(254, p[i].color + 32);

			}
			if(p[i].x >= WIDTH - XOFF){
				p[i].x = WIDTH - XOFF - 1;
				p[i].dx = -fabs(p[i].dx) * BOUNCE;
				p[i].dy += frand(KICK_STRENGTH);
//				if(AltColor) p[i].color = std::min(254, p[i].color + 32);
			}
			if(p[i].y < YOFF){
				p[i].y = YOFF;
				p[i].dy = fabs(p[i].dy) * BOUNCE;
				p[i].dx += frand(KICK_STRENGTH);
//				if(AltColor) p[i].color = std::min(254, p[i].color + 32);
			}
			if(p[i].y >= HEIGHT - YOFF){
				p[i].y = HEIGHT - YOFF - 1;
				p[i].dy = -fabs(p[i].dy) * BOUNCE;
				p[i].dx += frand(KICK_STRENGTH);
///				if(AltColor) p[i].color = std::min(254, p[i].color + 32);
			}
			x = (int)p[i].lx;
			y = (int)p[i].ly;
			dx = (int)p[i].x - x;
			dy = (int)p[i].y - y;
			if(x >= XOFF && y >= YOFF && x < WIDTH - XOFF && y < HEIGHT - YOFF){	//Make sure LAST coords are inside bounds too.
				//Bresenham style line drawer.
				if(abs(dx) > abs(dy)){	//X major.
					if(dx < 0){
						x += dx; y += dy;
						dx = -dx; dy = -dy;
					}
					d = j = -dx; e = abs(dy);
					data2 = data + x + y * pitch;
					if(dy < 0) dir = -pitch; else dir = pitch;
					unsigned char c = p[i].color, hc = c >> 1;
					while(dx >= 0){	//Draw start and end pixels.
						*(data2 - pitch) = std::max(*(data2 - pitch), hc);
						*(data2) = std::max(*(data2), c);
						*(data2 + pitch) = std::max(*(data2 + pitch), hc);
						d += e;
						if(d >= 0){
							d += j;
							data2 += dir;
						}
						data2++;
						dx--;
					}
				}else{	//Y major.
					if(dy < 0){
						x += dx; y += dy;
						dx = -dx; dy = -dy;
					}
					d = j = -dy; e = abs(dx);
					data2 = data + x + y * pitch;
					if(dx < 0) dir = -1; else dir = 1;
					unsigned char c = p[i].color, hc = c >> 1;
					while(dy >= 0){	//Draw start and end pixels.
						*(data2 - 1) = std::max(*(data2 - 1), hc);
						*(data2) = std::max(*(data2), c);
						*(data2 + 1) = std::max(*(data2 + 1), hc);
						d += e;
						if(d >= 0){
							d += j;
							data2 += dir;
						}
						data2 += pitch;
						dy--;
					}
				}
			}
		}
//		ParticleTimes += tmr2.Check(10000);
		//Kindle bottom row for main fire.
//		tmr2.Start();
		if(BurnDown){
			tdata = (unsigned char*)bdescdata + (BYOFF) * bdescpitch;
			memset((unsigned char*)bdescdata + (HEIGHT - BYOFF) * bdescpitch, 0, bdescwidth);
		}else{
			tdata = (unsigned char*)bdescdata + (HEIGHT - BYOFF) * bdescpitch;
			memset((unsigned char*)bdescdata + (BYOFF) * bdescpitch, 0, bdescwidth);
		}
		static double flamenoisey = 0.0;
		if(NoiseBurn){
			flamenoisey += FlameSpeed;
			for(x = BXOFF; x < WIDTH - BXOFF; x++){
				*(tdata + x) = (64 + (unsigned char)(basis.Noise((double)x * FlameSpeed, flamenoisey, 2) * 191.0)) >>FlameHalf;
			}
		}else{
			y = rand() & (255 >>FlameHalf);
			for(x = BXOFF; x < WIDTH - BXOFF; x++){
				if(*(tdata + x)){
					y = *(tdata + x);
				}
				y += rand() % 65 - 32;
				if(y < 0) y = 0;
				if(y > (254 >>FlameHalf)) y = (254 >>FlameHalf);
				*(tdata + x) = y;
			}
		}
//		KindleTimes += tmr2.Check(10000);

//		tmr2.Start();
		Burns++;
		//New burn code.
 #define BLONG unsigned long
 #define BLONGS sizeof(BLONG)
 #define UCP (unsigned char*)
 #define BURNIT(n) temp = (*(UCP lptr + n - pitch) + *(UCP lptr + n - 1) + *(UCP lptr + n) + *(UCP lptr + n + 1) - BURNFADE) >>2; \
	*(UCP lptr + n - pitch) = (unsigned char)(temp < 0 ? 0 : temp);
 #define BURNIT2(n) temp = (*(UCP lptr + n + pitch) + *(UCP lptr + n - 1) + *(UCP lptr + n) + *(UCP lptr + n + 1) - BURNFADE) >>2; \
	*(UCP lptr + n + pitch) = (unsigned char)(temp < 0 ? 0 : temp);
		BLONG *lptr = (BLONG*)bdescdata;
		int lpitch = bdescpitch / BLONGS;
		int lwidth = bdescwidth / BLONGS;
		int xlwidth = lwidth - BX4OFF;
		int temp;//, x2;
		unsigned char* bf;
		if(1){
		if(BurnDown){
			for(y = HEIGHT - BYOFF - 2; y > BYOFF - 1; y--){
				lptr = (BLONG*)bdescdata + (lpitch * y) + BX4OFF;
				bf = &BurnFlags[BX4OFF];
				for(x = BX4OFF; x < xlwidth; x++){
					if(*(lptr)){
						BURNIT2(0) BURNIT2(1) BURNIT2(2) BURNIT2(3);
						BurnFlags[x] = 1;
					}else{
						if(BurnFlags[x]){
							BURNIT2(0) BURNIT2(1) BURNIT2(2) BURNIT2(3);
						}
						BurnFlags[x] = 0;
					}
					lptr++;
				}
			//	lptr -= lpitch + lwidth + (BX4OFF * 2);
				if(y & 1){
					tdata = (unsigned char*)bdescdata + y * bdescpitch + BXOFF;
					if(y > BYOFF && y < HEIGHT - BYOFF - 1){	//Keeps boost splatter off edges.
						tdata2 = tdata + 1 + (rand() % (WIDTH - BXOFF * 2 - 2));
						if(*tdata2 > 32 && *tdata2 < 128){
							*(tdata2 + 1) = *(tdata2 - 1) =
							*(tdata2 + bdescpitch) = *(tdata2 - bdescpitch) =
							std::min(*tdata2 << 1, 255);
						}
					}
				}
			}
		}else{
			for(y = BYOFF + 1; y < HEIGHT - BYOFF + 1; y++){
				lptr = (BLONG*)bdescdata + (lpitch * y) + BX4OFF;
				bf = &BurnFlags[BX4OFF];
				for(x = BX4OFF; x < xlwidth; x++){
					if(*(lptr)){
						BURNIT(0) BURNIT(1) BURNIT(2) BURNIT(3);
						BurnFlags[x] = 1;
					}else{
						if(BurnFlags[x]){
							BURNIT(0) BURNIT(1) BURNIT(2) BURNIT(3);
						}
						BurnFlags[x] = 0;
					}
					lptr++;
				}
			//	lptr += lpitch - lwidth + (BX4OFF * 2);
				if(y & 1){
					tdata = (unsigned char*)bdescdata + y * bdescpitch + BXOFF;
					if(y > BYOFF && y < HEIGHT - BYOFF - 1){	//Keeps boost splatter off edges.
						tdata2 = tdata + 1 + (rand() % (WIDTH - BXOFF * 2 - 2));
						if(*tdata2 > 32 && *tdata2 < 128){
							*(tdata2 + 1) = *(tdata2 - 1) =
							*(tdata2 + bdescpitch) = *(tdata2 - bdescpitch) =
							std::min(*tdata2 << 1, 255);
						}
					}
				}
			}
		}
		}
		//
		//Try fade-zoom here.
	//	{
		static int Frames = 0;
		Frames++;
		static Bitmap TBM;
		if(UseFadeZoom){
			if(TBM.Width() != bdescwidth || TBM.Height() != bdescheight || TBM.Data() == 0){
				TBM.Init(bdescwidth, bdescheight, 8);
			}
			if(TBM.Width() == bdescwidth && TBM.Height() == bdescheight && TBM.Data()){
				TBM.Suck(bdescdata, bdescwidth, bdescheight, bdescpitch, 8);
				int Blast = 0;
			//	if((Time / 1) % 10 == 0) Blast = 1;
				if((Frames / 10) % 10 == 0) Blast = 1;
				int xshrink = WIDTH / 50, yshrink = HEIGHT / 50;
			//	int xshrink = WIDTH / 10, yshrink = HEIGHT / 10;
				if(Blast){
					xshrink = WIDTH / 40; yshrink = HEIGHT / 40;
			//		xshrink = WIDTH / 10; yshrink = HEIGHT / 10;
				}
				int xerr, yerr, xeadd, yeadd;
				int srcy, srcx;
				unsigned char *src, *dst;
				int x, y;
				int w = WIDTH - BXOFF * 2;
				int h = (HEIGHT - BYOFF * 2) - 1;
				yerr = h / 2;
				yeadd = yshrink;
				xeadd = xshrink;
				srcy = yshrink >>1;
				//
				unsigned char clamptab[512];
				for(int i = 0; i < 512; i++) clamptab[i] = std::min(i, 255);
				//
				for(y = 0; y < h; y++){
					srcx = (xshrink >>1) + BXOFF;
				//	src = bdescdata + srcx + srcy * bdescpitch;
					src = TBM.Data() + srcx + (srcy + BYOFF) * TBM.Pitch();
					dst = bdescdata + BXOFF + (y + BYOFF) * bdescpitch;
					xerr = w / 2;
					if(Blast){
						for(x = w; x; x--){
							unsigned char tc = *src >>1;
						//	int t = (*dst >>1) + (tc >>2) + (tc);//(((*src <<7)) >>8);
						//	if(t > 255) *dst = 255;
						//	else *dst = (unsigned char)t;
							*dst = clamptab[(*dst >>1) + (tc >>2) + (tc)];//(((*src <<7)) >>8);
							//
							xerr += xeadd;
							if(xerr >= w) xerr -= w;
							else src++;
							dst++;
						}
					}else{
						for(x = w; x; x--){
							*dst = (*dst >>1) + (*src >>1);
						//	int t = (*dst >>1) + (*src >>1) - 2;
						//	if(t < 0) *dst = 0;
						//	else *dst = (unsigned char)t;
							dst++;
							xerr += xeadd;
							if(xerr >= w) xerr -= w;
							else src++;
						}
					}
					//
					yerr += yeadd;
					if(yerr >= h){
					//	srcy++;
						yerr -= h;
					}else{
						srcy++;
					}
				//	srcy++;
				}
			}
		}
		//
		//
//		BurnTimes += tmr2.Check(10000);

	//	buf.Unlock();
	}
//	tmr2.Start();
}

