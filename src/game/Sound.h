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
//Pure Sound playing interface (subclass to implement specific
//sound systems), plus RIFF/WAVE file loading and streaming class.
//Written by Seumas McNally.
//

#ifndef _SOUND_H_
#define _SOUND_H_

//Disable exceptions not enabled warnings.
#pragma warning( disable : 4530 )

#include <new>
#include <iostream>
#include <vector>

#ifndef HEADLESS
#include <SFML/Audio.hpp>
#endif

#include "Vector.h"
#include "CStr.h"

struct SoundBuffer
{
public:
	SoundBuffer();

#ifndef HEADLESS
	sf::SoundBuffer buffer;
#endif
	float fVolumeBias;
	bool bLoop;
};

struct SoundVoice
{
	SoundVoice();

#ifndef HEADLESS
	sf::Sound		voice;
#endif
	int				iBufferId;
	float			fGain;
	float			fPitchScale;
	float			fMinDistScale, fMaxDistScale;
	Vec3			vPos;
	Vec3			vVel;
	float			fPriority;
	bool			b2DVoice;
};

class Sound
{
public:
	~Sound() { Free(); }
	// TotalV is the number of simultaneous mixing voices.
	void Init(int TotalV);
	void Free();

	//Sound atten is 1/2 at 2x min dist.
	bool SetDistanceModel(float mindist, float maxdist);

	//Returns bool, use GetBufferCount() - 1 after to find waveform ID number.
	void AddBuffer(SoundBuffer *wv);

	//Plays an added buffer on first open voice.
	//Use GetLastVoice to find voice used.
	void PlayBuffer(int num, float pri);
	
	//Stops one voice.
	void Stop(int v);
	
	//Stops 'em all.
	void StopAll();
		
	// Returns the number of stored buffers.
	int GetBufferCount() { return aBuffers.size(); }
	//Returns last used voice, or -1 if last Play attempt failed.
	int GetLastVoice() { return iLastVoice; }
	//Returns nonzero if voice is playing, zero if stopped or bad voice num.
	bool GetVoiceStatus(int v);
	//Returns the wave ID last played on a voice.
	int GetVoiceWave(int v);
	
	//Performs update of 3D spatial positions.
	bool PulseAudio();

	void SetListenerPos(Vec3 pos);
	void SetListenerRot(Rot3 rot);
	void SetListenerVel(Vec3 vel);

	void SetVoicePos(int v, Vec3 pos);
	void SetVoiceVel(int v, Vec3 vel);
	void SetVoicePriority(int v, float pri);
	void SetVoiceGain(int v, float gain);
	void SetVoicePitch(int v, float pitch);
	void SetVoiceDistanceScale(int v, float minscale, float maxscale);
	void SetVoice3D(int voice, Vec3 pos, Vec3 vel = 0, float gain = 1.0f, float pitch = 1.0f, float pri = 0.5f);
	void SetVoice2D(int voice);

	void StartMusic(const CStr& sFilename);
	void StopMusic();
	void PauseMusic(bool pause);
	bool IsMusicPlaying();

	void SetSoundVolume(float gain); //3D sound effects only.
	void SetGlobalPitch(float pitch); //3D sound effects only.
	void SetMusicVolume(float gain);

private:
	std::vector<SoundVoice>	aVoices;
	std::vector<SoundBuffer*> aBuffers;

	Vec3 vListenerPos, vListenerRot, vListenerVel;
	float fMinDist, fMaxDist;
	float fGlobalPitch;
	int iLastVoice;

#ifndef HEADLESS
	sf::Music music;
#endif
};

#endif

