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

//Race car handling class.
//
//Remember!!!
//Multiply all angle returning funcs by RAD2DEG!!!
//Except tcos/tsin.

//July 25th, 1998.
//Things are mostly working out OK now.  Just need a few more things, like perhaps
//more "dig in" with wheels when flipping about, suck yaw angle out of collided positions
//so yaw velocity gets frictioned away too, etc.
//It would also be "right" to use something like Quaternions for rotations.

#pragma inline_depth(255)

#include "pi.h"
//#include "Poly.h"
#include "Physics.h"
//#include <math.h>
#include "Render.h"
#include "Trees.h"
#include <cmath>
#include <limits>

using namespace std;

#define SHITDIST 2.0f

#define FORWARDFRIC 0.2f

//const int WheelPoints = 7;

//#define FRAC 0.05f
//20ths of a second...

inline float PntDistGroundPlane(float pnty, Vec3 plane){
	float planex = -plane[1];
	float planey = sqrtf(plane[0] * plane[0] + plane[2] * plane[2]);
	return pnty * planey;// / sqrtf(planex * planex + planey * planey));
	//Dot product is simplified but we need another bloody sqrt to normalize the thing.
	//Don't normalize in here, need normalized vector externally anyway.
}

Vehicle::Vehicle(){
	Init(10.0f, 10.0f, true, VEHICLE_CAR, 0.1f);
}
Vehicle::Vehicle(float grav, float fric, bool conrot, int type, float frac){
	Init(grav, fric, conrot, type, frac);
}
Vehicle::~Vehicle(){
}
void Vehicle::Init(float grav, float fric, bool conrot, int type, float frac){
	//
//	WheelPoints = 7;
	//
	WheelPoints = 8;
	//
	GroundWheelPoints = 6;
	BackLeft = 2;
	BackRight = 5;
	FrontLeft = 0;
	FrontRight = 3;
	//
	Type = type;
//	Gravity = grav;// * FRAC;
	Friction = fric;// * FRAC;
	Fraction = frac;
	ConstrainRot = conrot;
	ClearVec3(Pos);
	ClearVec3(LPos);
	IdentityMat3(RotM);
	IdentityMat3(LRotM);
	ClearVec3(Vel);
	IdentityMat3(RotVelM);
	IdentityMat3(LRotVelM);
	LastThinkTime = 0;
	LinearVelocity = 0;
	memset(InGround, 0, sizeof(InGround));
	memset(AddAccelWP, 0, sizeof(AddAccelWP));
	InGroundTotal = 0;
	ClearVec3(AddAccel);
	UseNetworkNext = 0;
	//
	FractionMS = (int)(frac * 1000.0f);
	//
	CurrentLump = 0;
	CurrentTime = 0;
//	History[CurrentLump].TimeStamp = 0;
	InGroundBits = 0;
	//
	LTurRot = TurRot = 0.0f;
}
void Vehicle::Config(float steer, float accel, float max){
	SteerSpeed = steer;// * FRAC;
	AccelSpeed = accel;// * FRAC;
	MaxSpeed = max;// * FRAC;
}
void Vehicle::SetGravity(Vec3 grav){
	if(grav) CopyVec3(grav, Gravity);
}
void Vehicle::SetFriction(float fric){
	Friction = fric;
}
//
#define THISLUMP(a) (((a) + 0) & (HISTORY_SIZE - 1))
#define NEXTLUMP(a) (((a) + 1) & (HISTORY_SIZE - 1))
#define PREVLUMP(a) (((a) - 1) & (HISTORY_SIZE - 1))
//
void Vehicle::SetCurrentTime(unsigned int time){
//	int curlump = History[CurrentLump].TimeStamp / FractionMS;
	int curlump = CurrentTime / FractionMS;
	int lump = time / FractionMS;
	if(abs(curlump - lump) > HISTORY_SIZE || lump < curlump){
		for(int n = 0; n < HISTORY_SIZE; n++) History[n].Flags = FLAG_NONE;	//Too big.
		CurrentLump = THISLUMP(lump);// % HISTORY_SIZE;
	//	History[CurrentLump].TimeStamp = time;
		CurrentTime = time;
		return;
	}
	for(int n = 0; n < lump - curlump; n++){
		CurrentLump = NEXTLUMP(CurrentLump);
		History[NEXTLUMP(CurrentLump)].Flags = FLAG_NONE;
	}
//	History[CurrentLump].TimeStamp = time;
	CurrentTime = time;
}
int Vehicle::SetControlInput(unsigned int time, float laccel, float raccel, float tursteer){
	//Sets inputs to the bucket AHEAD of normal...  Don't use for server authoritative inputs.
//	int curlump = History[CurrentLump].TimeStamp / FractionMS;
	int curlump = CurrentTime / FractionMS;
	int lump = time / FractionMS;
	int n = THISLUMP(CurrentLump + (lump - curlump) + 1);
	History[n].RAccel = raccel;
	History[n].LAccel = laccel;
	History[n].Flags |= FLAG_CONTROL;
	History[n].TurRotVel = tursteer;
//	History[n].TimeStamp = time;
	return 1;
}
int Vehicle::SetAuthoritativeState(unsigned int time, Vec3 pos, Vec3 vel, Mat3 rot, Mat3 rotvel,
								   float laccel, float raccel, float turrot, float tursteer, int ingbits){
//	int curlump = History[CurrentLump].TimeStamp / FractionMS;
	int curlump = CurrentTime / FractionMS;
	int lump = time / FractionMS;
	int n = THISLUMP(CurrentLump + (lump - curlump) + 0);
	History[n].RAccel = raccel;
	History[n].LAccel = laccel;
	History[n].Flags |= FLAG_CONTROL | FLAG_POSROTVEL | FLAG_AUTHORITATIVE;
//	History[n].TurRot = turrot;
	History[n].TurRotVel = tursteer;
	//
//	History[n].TimeStamp = time;
//	if(pos) CopyVec3(pos, History[n].Pos);
//	if(vel) CopyVec3(vel, History[n].Vel);
//	if(rot) for(int i = 0; i < 3; i++) CopyVec3(rot[i], History[n].RotM[i]);
//	if(rotvel) for(int i = 0; i < 3; i++) CopyVec3(rotvel[i], History[n].RotVelM[i]);
	History[n].Set(pos, vel, rot, rotvel, turrot);//, tursteer);
	History[n].InGroundBits = ingbits;
	return 1;
}

int Vehicle::PredictCurrentTime(Vec3 posout, Rot3 rotout, Vec3 velout, float *turrot, Terrain *map){
	if(!map) return 0;
	//
	int lump = CurrentLump, i;
	float la = 0.0f, ra = 0.0f;
	float tursteer = 0.0f;
	for(i = 0; i < HISTORY_SIZE - 1; i++){	//Go one less than SIZE steps so we don't wrap back to self completely.
		if(History[lump].Flags & FLAG_CONTROL){
			la = History[lump].LAccel;
			ra = History[lump].RAccel;
			tursteer = History[lump].TurRotVel;
		}
		if(History[lump].Flags & FLAG_AUTHORITATIVE) break;	//Found most recent authoritative lump.
		lump = PREVLUMP(lump);
	}
	//
	if((History[lump].Flags & FLAG_AUTHORITATIVE) == 0){	//Oops, no authoritative found.
		if(posout) CopyVec3(Pos, posout);
		if(rotout) Mat3ToRot3(RotM, rotout);
		if(velout) ScaleVec3(Vel, 1.0f / Fraction, velout);	//Put meters-per-second velocity in external vel.
		LinearVelocity = LengthVec3(Vel) / Fraction;	//Calc m/s linear vehicular velocity, for outside ref.
		if(turrot) *turrot = TurRot;
		return 0;
	}
	//
	//Predict through time!
	if(History[lump].Flags & FLAG_POSROTVEL){
		History[lump].Get(Pos, Vel, RotM, RotVelM, &TurRot);//, &TurRotVel);	//Retrieve stored positional info.
		InGroundBits = History[lump].InGroundBits;
	}
	do{
		int nl = NEXTLUMP(lump);
		if(History[nl].Flags & FLAG_CONTROL){
			la = History[nl].LAccel;
			ra = History[nl].RAccel;
			tursteer = History[nl].TurRotVel;
		}
		Think2(la, ra, tursteer, map);
		History[nl].Set(Pos, Vel, RotM, RotVelM, TurRot);//, TurRotVel);
		History[nl].InGroundBits = InGroundBits;
		History[nl].Flags |= FLAG_POSROTVEL;
		//
		lump = NEXTLUMP(lump);
		//
	}while(lump != NEXTLUMP(CurrentLump));
	//
	//Output lerped position.
//	float t = (float)(History[CurrentLump].TimeStamp % FractionMS) / (float)FractionMS;
	float t = (float)(CurrentTime % FractionMS) / (float)FractionMS;
	if(posout) LerpVec3(History[CurrentLump].Pos, History[NEXTLUMP(CurrentLump)].Pos, t, posout);
	Mat3 tm;
	LerpMat3(History[CurrentLump].RotM, History[NEXTLUMP(CurrentLump)].RotM, t, tm);
	if(rotout) Mat3ToRot3(tm, rotout);
	if(velout) ScaleVec3(History[NEXTLUMP(CurrentLump)].Vel, 1.0f / Fraction, velout);	//Put meters-per-second velocity in external vel.
	//
	LinearVelocity = LengthVec3(History[NEXTLUMP(CurrentLump)].Vel) / Fraction;	//Calc m/s linear vehicular velocity, for outside ref.
	//
	if(turrot){
		*turrot = LERP(History[CurrentLump].TurRot, History[NEXTLUMP(CurrentLump)].TurRot, t);
		if(fabsf(History[CurrentLump].TurRot - History[NEXTLUMP(CurrentLump)].TurRot) > PI)
			*turrot = History[NEXTLUMP(CurrentLump)].TurRot;
	}
	//
	return 1;
}
//

//void Vehicle::Reset(){
//}
void Vehicle::SetPos(Vec3 pos){
	if(pos) CopyVec3(pos, Pos);
	CopyVec3(Pos, LPos);
}
void Vehicle::SetRot(Rot3 rot){
	if(rot) Rot3ToMat3(rot, RotM);
	memcpy(LRotM, RotM, sizeof(RotM));
}
void Vehicle::SetVel(Vec3 vel){
	CopyVec3(vel, Vel);
}

#include "CStr.h"

void Vehicle::Think(float LeftAccel, float RightAccel, unsigned int clocktime, Vec3 pos, Rot3 rot, Vec3 vel,
					float *turrot, float turrotvel, Terrain *map){
	int iter = abs(int(clocktime / FractionMS - LastThinkTime / FractionMS));
	LastThinkTime = clocktime;
	//
	iter = max(min(10, iter), 0);
	int lerpnormal = 1;
	//
	for(int n = 0; n < iter; n++){
		if(UseNetworkNext){
			CopyVec3(NPos, Pos);
			CopyVec3(NVel, Vel);
			for(int m = 0; m < 3; m++){
				CopyVec3(NRotM[m], RotM[m]);
				CopyVec3(NRotVelM[m], RotVelM[m]);
			}
			UseNetworkNext = 0;
		}
		//
		Think2(LeftAccel, RightAccel, turrotvel, map);
		//
		//Collision checking.
		CopyVec3(Pos, pos);
		Mat3ToRot3(RotM, rot);
		ScaleVec3(Vel, 1.0f / Fraction, vel);	//Put meters-per-second velocity in external vel.
		LinearVelocity = LengthVec3(Vel) / Fraction;	//Calc m/s linear vehicular velocity, for outside ref.
		CheckCollision();
	}
	//Interpolate.
	if(lerpnormal){
		double t = (double)(clocktime % FractionMS) / (double)FractionMS;
		LerpVec3(LPos, Pos, t, pos);
		Mat3 tm;
		LerpMat3(LRotM, RotM, t, tm);
		Mat3ToRot3(tm, rot);
		*turrot = LERP(LTurRot, TurRot, t);
		if(fabsf(TurRot - LTurRot) > PI) *turrot = TurRot;	//Handle -PI to PI transition.
	}
}

//Here's how to do push out of terrain...  Define a Maximum Y Displacement constant,
//grab three points around the WP to form a plane, if their heights differences
//are greater than MYD from the WP then we're jumping a cliff, so pick the point
//with the closest Y to the WP and recurse through three points extended from it,
//and continue until three points that meet MYD are found.  Then cross product and
//push out onto that plane.

void Vehicle::Think2(float LeftAccel, float RightAccel, float turrotvel, Terrain *map){
	if(!map || map->Width() <= 0) return;
	//
	//Turret rotation handling...
	LTurRot = TurRot;
	TurRot = NormRot(TurRot + turrotvel * Fraction);
	//
	//
	float scale = Fraction * Fraction;	//Square of time step scales accels.
	//
	//Clamp velocity (no more orbitals...).
	{
		float len = LengthVec3(Vel) * (1.0f / Fraction);
		if(len > MaxSpeed * 1.25f){
			ScaleVec3(Vel, (MaxSpeed * 1.25f) / len);
		}
	}
	//
	//Copy current WPs into LastWPs.
	CopyVec3(Pos, LPos);
	memcpy(LRotM, RotM, sizeof(RotM));
	memcpy(LRotVelM, RotVelM, sizeof(RotVelM));
	//
	int i;

	//Decompose InGroundBits array.
	InGroundTotal = 0;
	for(i = 0; i < MAXWHEELPOINTS; i++){
		if(TestBit(InGroundBits, i)){
			InGroundTotal++;
			InGround[i] = 1;
		}else InGround[i] = 0;
	}
	//
	//Generate new WPs.
	//0   3
	//1 6 4
	//2   5
	WP[0].y = WP[1].y = WP[2].y = WP[3].y = WP[4].y = WP[5].y = BndMin.y;
	WP[0].x = WP[1].x = WP[2].x = BndMin.x * 0.8f;
	WP[3].x = WP[4].x = WP[5].x = BndMax.x * 0.8f;
	WP[0].z = WP[3].z = BndMax.z * 0.8f;
	WP[2].z = WP[5].z = BndMin.z * 0.8f;
	WP[1].z = WP[4].z = (BndMin.z + BndMax.z) * 0.5f;
	//
	WP[6].x = 0.0f;
	WP[6].y = BndMax.y;
	WP[6].z = (BndMin.z + BndMax.z) * 0.5f;
	//

	//
	//Make WP[7] average of all others.
	ClearVec3(WP[7]);
	for(i = 0; i < 7; i++) AddVec3(WP[i], WP[7]);
	ScaleVec3(WP[7], 1.0f / (float)WheelPoints);
	//

	//Get initial wp average offset.
	CVec3 Offset;
	ClearVec3(Offset);
	for(i = 0; i < WheelPoints; i++){
		//
		//Ok, now LastWPs are created from today's pos and rots too.
		//This should fix extreme friction when turning in place.
		Vec3MulMat3(WP[i], RotM, LastWP[i]);	//Rotate to vehicle heading.
		AddVec3(Pos, LastWP[i]);	//Translate to vehicle position.
		//
		AddVec3(WP[i], Offset);
	}
	ScaleVec3(Offset, -(1.0f / (float)WheelPoints));
	//
	//First add wanted velocity change!
	ScaleAddVec3(AddAccel, Fraction, Vel);
	ClearVec3(AddAccel);
	//
	//Add Velocity and RotVel to Position and Rotation.
	AddVec3(Vel, Pos);


	Mat3 trm;
	IdentityMat3(trm);
	float fmax = 0.1f;
	if(InGroundTotal <= 0) fmax = 0.03f;
	//
	//This new DAMP function may help with time step invariance.
	float tt = std::max(fmax, min(0.15f, 0.5f * (float)(InGroundTotal - 2) / (float)(GroundWheelPoints - 2)));
	LerpMat3(RotVelM, trm, 1.0f - DAMP(1.0f, 1.0f - tt, Fraction / 0.05f), trm);	//Little bit of rot vel, bled off quickly...
	Mat3MulMat3(trm, LRotM, RotM);

	//Create post-velocity/pre-acceleration wheel points, to calculate individual wheel accels.
	//Aid for accurate dirt clump chucking.
	for(i = 0; i < WheelPoints; i++){
		Vec3MulMat3(WP[i], RotM, AccelWP[i]);	//Rotate to vehicle heading.
		AddVec3(Pos, AccelWP[i]);	//Translate to vehicle position.
	}

	//Add gravity, tread accel.
	//Pre-transform, entity coordinates.
	float l = LeftAccel * scale * AccelSpeed * (3.0f / (float)std::max(InGround[0] + InGround[1] + InGround[2], 1));
	float r = RightAccel * scale * AccelSpeed * (3.0f / (float)std::max(InGround[3] + InGround[4] + InGround[5], 1));
	float veldot = DotVec3(RotM[2], Vel);
	float MS = (MaxSpeed + MaxSpeed * FORWARDFRIC) * Fraction;	//Frac * Frac for ACCELERATIONS, but just Frac for VELOCITIES!
	if(fabsf(veldot) > 1.0f * scale){
		if((veldot < 0.0f && l < 0.0f) || (veldot > 0.0f && l > 0.0f)){
			l *= 1.0f - std::min(1.0f, std::max(0.0f, fabsf(veldot) / MS));	//Up MaxSpeed by a bit to compensate for friction.
		}
		if((veldot < 0.0f && r < 0.0f) || (veldot > 0.0f && r > 0.0f)){
			r *= 1.0f - std::min(1.0f, std::max(0.0f, fabsf(veldot) / MS));
		}
	}
	WP[0].z += l * (float)InGround[0];
	WP[1].z += l * (float)InGround[1];
	WP[2].z += l * (float)InGround[2];
	WP[3].z += r * (float)InGround[3];
	WP[4].z += r * (float)InGround[4];
	WP[5].z += r * (float)InGround[5];

	//
	//Make WP[7] average of all others.
	ClearVec3(WP[7]);
	for(i = 0; i < 7; i++) AddVec3(WP[i], WP[7]);
	ScaleVec3(WP[7], 1.0f / (float)WheelPoints);
	//


	Pnt3 tp;
	for(i = 0; i < WheelPoints; i++){
		CopyVec3(WP[i], tp);
		Vec3MulMat3(tp, RotM, WP[i]);	//Rotate to vehicle heading.
		AddVec3(Pos, WP[i]);	//Translate to vehicle position.
	}

	//Add asked-for additional accelerations, from e.g. collisions.
	int tAccels = 0;
	for(i = 0; i < WheelPoints; i++){
		if(AddAccelWP[i][0] != 0.0 || AddAccelWP[i][1] != 0.0 || AddAccelWP[i][2] != 0.0){
			tAccels++;
		}
	}
	float tAccelScale = ((float)WheelPoints / (float)std::max(tAccels, 1)) * Fraction;//scale;
	for(i = 0; i < WheelPoints; i++){
		ScaleAddVec3(AddAccelWP[i], tAccelScale, WP[i]);
		ClearVec3(AddAccelWP[i]);
	}


	//Lerp LastWPs to current WPs using friction and in-ground flags.
	//Ok, vary friction based on dot product with tread's acceleration
	//vector...
	//This is how much to boost friction and acceleration and push-out based on wheels being off ground.
	float InGroundBoost = (float)(GroundWheelPoints) / (float)std::max(1, InGroundTotal);
	//
	float fric = Friction * scale * InGroundBoost;
	for(i = 0; i < WheelPoints; i++){
		if(InGround[i] || WP[i].y < WATERHEIGHT){
			CVec3 tv1, tv2;
			SubVec3(WP[i], LastWP[i], tv1);
			Vec3IMulMat3(tv1, RotM, tv2);	//Vehicle-coords velocity vector.
			for(int c = 0; c < 3; c++){
				if(InGround[i]){
					float fric2 = fric;
					if(c == 2 && ((i < 3 && fabsf(LeftAccel) > 0.2f) || (i > 2 && fabsf(RightAccel) > 0.2f)))
						fric2 *= FORWARDFRIC;
					if(c == 1) fric2 *= 1.0f;
					if(tv2[c] > 0){
						tv2[c] -= fric2;
						if(tv2[c] < 0.0f) tv2[c] = 0.0f;
					}else{
						tv2[c] += fric2;
						if(tv2[c] > 0.0f) tv2[c] = 0.0f;
					}
				}
				if(WP[i].y < WATERHEIGHT){	//Do a little viscous friction.
				//	tv2[c] *= 1.0f - (0.4f * Fraction);	//Arbitrary testing fluid friction value.
					tv2[c] = DAMP(tv2[c], 0.6f, Fraction);
				}
			}
			Vec3MulMat3(tv2, RotM, tv1);
			AddVec3(tv1, LastWP[i], WP[i]);
		}
	}

	//
	//Calculate vehicle relative accelerations for external info.
	float unscale = 1.0f / scale;
	for(i = 0; i < WheelPoints; i++){
		Vec3 tp;
		SubVec3(WP[i], AccelWP[i], tp);
		ScaleVec3(tp, unscale);	//Move into meters per second.
		Vec3IMulMat3(tp, RotM, AccelWP[i]);	//Move into vehicle relative space.
	}


	//THEN collide with ground...
	//Tree collide, push out of ground.
	//Later.
	InGroundTotal = 0;
	for(i = 0; i < WheelPoints; i++){
	//	WP[i].y -= Gravity * scale;
		ScaleAddVec3(Gravity, scale, WP[i]);
		//
		CVec3 origin(WP[i].x, map->FGetI(WP[i].x, -WP[i].z), WP[i].z);
		if(WP[i].y < (origin.y + 0.1f)){	//Give a little leeway for "InGround"
			InGround[i] = true;
			InGroundTotal++;
		}else{
			InGround[i] = false;
		}
	}
	//Recalc boost.
	InGroundBoost = (float)(GroundWheelPoints) / (float)std::max(1, InGroundTotal);
	//
	for(i = 0; i < WheelPoints; i++){
		CVec3 origin(WP[i].x, map->FGetI(WP[i].x, -WP[i].z), WP[i].z);
		if(WP[i].y < origin.y){
			//
			//
			//Test old version of out of ground code...
			float indist = origin.y - WP[i].y, h = origin.y, t;
			CVec3 tpnt, tpnt2;
			if(indist > SHITDIST && map->FGetI(Pos[0], -Pos[2]) < Pos[1]){	//We are in deep shit, so IF car center isn't in shit, pull towards it (e.g. ramming a mountain).  Fails for slamming down through ground.
				float t = 0;
				float integrate = 0.2f;
				do{
					t += integrate;
					LerpVec3(WP[i], Pos, t, tpnt);
				}while((h = map->FGetI(tpnt[0], -tpnt[2])) > tpnt[1] && t < 2.0f);	//Sanity.
			}else{	//Normal case, leave ground at closest distance on ground plane to point.
				float bestdist = indist, tempdist;
				float wheelx = WP[i][0];
				float wheelz = WP[i][2];
				float integrate;
				tpnt[0] = 0;	//Best dist point on ground.  RELATIVE TO GROUND HEIGHT STRAIGHT ABOVE CAR!
				tpnt[1] = 0;
				tpnt[2] = 0;
				if(indist > 0.01f){	//Sanity check, don't bother if in only an eany weeny bit.
					if(indist > 0.2f) integrate = indist * 0.2f;
					else integrate = indist * 0.4f;
					t = indist * 0.7f;
					int sane = 100;
					float y, x;
					for(y = -t; y <= t; y += integrate){	//Make SURE x and y never land on 0.0, or else add a check for it before normalize!
						for(x = -t; x <= t; x += integrate){
							tpnt2[1] = map->FGetI(x + wheelx, -(y + wheelz)) - h;
							if(tpnt2[1] < 0.0f){	//If it's positive, there's NO CHANCE we'll be closer along the part of the plane we want.
								tpnt2[0] = x;
								tpnt2[2] = y;
								NormVec3(tpnt2, tpnt2);
								tempdist = -PntDistGroundPlane(-indist, tpnt2);
								if(tempdist < bestdist && tempdist != std::numeric_limits<float>::infinity()){	//We have a weiner.
									bestdist = tempdist;
									CopyVec3(tpnt2, tpnt);
								}
							}
							if(sane-- <= 0) break;
						}
						if(sane-- <= 0) break;
					}
				}
				ScaleVec3(tpnt, tpnt[1] * -indist);	//Partial dot product to scale by.
				tpnt[0] += wheelx;
				tpnt[1] += h;
				tpnt[2] += wheelz;
			}
			//20hz hack.
			LerpVec3(WP[i], tpnt, 1.0f + (InGroundBoost - 1.0f) * 0.1f, WP[i]);	//This will boost further out of ground if less wheels are in ground.
			//It'll be a bit latent though due to the in ground boost being calced based
			//on LAST iterations ground collisions...  Hmm.
		}
	}

	//Average WPs to find new Position and Rotation.
	//Hrmm...
	ClearVec3(Pos);
	for(i = 0; i < WheelPoints; i++){
		AddVec3(WP[i], Pos);
	}
	ScaleVec3(Pos, 1.0f / (float)WheelPoints);
	ScaleAddVec3(RotM[1], Offset[1], Pos);	//Add difference in position from wheel base.
	//
	//The 6 / 7 is to compensate for the 1 wheel point not at BndMin.y.
	//
	//Calculate new orientation.
	Mat3 tm;
	for(i = 0; i < 3; i++){
		tm[0][i] = (WP[3][i] + WP[4][i] + WP[5][i]) * 0.333f - (WP[0][i] + WP[1][i] + WP[2][i]) * 0.333f;
		tm[1][i] = WP[6][i] - (WP[1][i] + WP[4][i]) * 0.5f;
		tm[2][i] = ((WP[0][i] + WP[3][i]) * 0.5f - (WP[1][i] + WP[4][i]) * 0.5f) +
			((WP[1][i] + WP[4][i]) * 0.5f - (WP[2][i] + WP[5][i]) * 0.5f);
	}
	//
	{	//Hack to make more well rounded rot calc.
		for(i = 0; i < 3; i++) NormVec3(tm[i]);
		CrossVec3(tm[2], tm[0], tm[1]);
		Vec3 tv;
		CrossVec3(tm[1], tm[2], tv);
		//
		//Okay, now using ratio of length to width to pull lerp towards all X vector to help really wide tanks steer better.
		//Should have no effect on l > w tanks.
		float l = BndMax.z - BndMin.z;
		float w = BndMax.x - BndMin.x;
		float t = 0.5f;
		if(w > l * 0.5f) t = ((MIN(2.0f, w / MAX(0.001f, l)) - 0.5f) / 3.0f) + 0.5f;//* 0.5f + 0.25f;
		//MIN() returns val between 0.5 and 2.0, subtract so 0.0 and 1.5, divide by 3 to get 0.0 - 0.5, add to get 0.5 - 1.0.
		//
		LerpVec3(tv, tm[0], t, tm[0]);
	}
	for(i = 0; i < 3; i++) NormVec3(tm[i]);
	CrossVec3(tm[2], tm[0], tm[1]);
	NormVec3(tm[1]);
	CrossVec3(tm[0], tm[1], tm[2]);

	//Constrain rotations.
	//20hz hack.
	{
		float rp = DAMP(1.0f, 0.3f, Fraction);
		if(ConstrainRot && InGroundTotal > 0 && tm[1][1] < 0.2f) tm[1][1] = (tm[1][1] * rp + 0.2f * (1.0f - rp));	//Average closer.
	}
	//
	CrossVec3(tm[2], tm[0], RotM[1]);
	CrossVec3(tm[0], tm[1], RotM[2]);
	CrossVec3(tm[1], tm[2], RotM[0]);
	for(i = 0; i < 3; i++){
		LerpVec3(tm[i], RotM[i], 0.5f, RotM[i]);
		NormVec3(RotM[i]);
	}
	//Sub to find new Velocity and RotVel.
	//Hrmm...
	SubVec3(Pos, LPos, Vel);
	Mat3IMulMat3(RotM, LRotM, RotVelM);
	//
	//Reduce rotational velocity by specific fraction.
	Mat3 trotvel;
	float rp = 1.0f - DAMP(1.0f, 1.0f - .75f, Fraction / 0.05f);	//20hz was where initial values were based.
	for(i = 0; i < 3; i++){
		//20hz hack.
		LerpVec3(LRotVelM[i], RotVelM[i], 0.75f, trotvel[i]);
		NormVec3(trotvel[i]);
		//New hack.
		CopyVec3(trotvel[i], RotVelM[i]);
	}
	Mat3MulMat3(trotvel, LRotM, RotM);
	//
	//Renormalize matrix.
	CrossVec3(RotM[2], RotM[0], RotM[1]);
	CrossVec3(RotM[0], RotM[1], RotM[2]);
	for(i = 0; i < 3; i++) NormVec3(RotM[i]);
	//
	//SANITY CHECK!!!
	for(i = 0; i < 3; i++){
		if(Pos[i] == std::numeric_limits<float>::infinity()) CopyVec3(LPos, Pos);
		for(int j = 0; j < 3; j++){
			if(RotM[i][j] == std::numeric_limits<float>::infinity()) CopyVec3(LRotM[i], RotM[i]);
			if(RotVelM[i][j] == std::numeric_limits<float>::infinity()) CopyVec3(LRotVelM[i], RotVelM[i]);
		}
	}
	//
	//Reduce absolute rotational change by half, try to smooth it out.  AFTER rotvel calc.
	//Compose InGroundBits array.
	InGroundBits = 0;
	for(i = 0; i < MAXWHEELPOINTS; i++){
		InGroundBits = SetBit(InGroundBits, i, InGround[i]);
	}
}

//
//**********************************************************************************
//**********************************************************************************
//           OLDE CODE BELOW!  VENTURE YE WITH CARE!
//**********************************************************************************
//**********************************************************************************
//

void Vehicle::oldThink(float Steer, float Accel, float Brake, float Scale, Terrain *terr){
	//	ToDo:
	//	General XYZd friction based on heading vs. XYZd.
	//	Friction on B rot delta based on wheels on ground, and don't allow steering
	//		unless both front wheels on ground (one is half etc.).
	//	Acceleration based on current speed, up to a max speed.
	//
	//Wheel positions in world space, front left, front right, back left, back right.
	// 0 1
	// 2 3
	//
	//	Only multiply resultant deltas by Time Scale when actually adding them to
	//		car's pos and rot.  Use deltas as-is when rotating wheel coords and such.
	//
	//	Distance moved is Average Velocity, which is (OldV + V) / 2.  Add THAT to
	//		pos and rot, times scale too of course.
	//
	//Try this:
	//1 Figure wheel coords from car pos.
	//2 Add velocities to temp wheel coords.
	//2a Add rotational velocities to temp wheel coords.
	//3 Push wheels out of ground.
	//4 Average wheel positions to find new car velocity.
	//4a Average wheel angles to find new rotational velocities.
	//5 Modify car velocity by friction.
	//6 Add car velocity to car pos.
	//6a Add car totational velocity to car rotation.
	//7 Add accel and gravity to car velocity.
	//
}
