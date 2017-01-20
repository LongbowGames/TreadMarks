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
//SoundWave, RIFF/WAVE handling class.
//Written by Seumas McNally.
//

#include "Sound.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>

using namespace std;

SoundBuffer::SoundBuffer()
	: fVolumeBias(0.5f)
	, bLoop(false)
{
}

SoundVoice::SoundVoice()
	: iBufferId(-1)
	, fGain(1.0f)
	, fPitchScale(1.0f)
	, fMinDistScale(1.0f)
	, fMaxDistScale(1.0f)
	, fPriority(0.0f)
	, b2DVoice(true)
{
	ClearVec3(vPos);
	ClearVec3(vVel);
}

// TotalV is the number of simultaneous mixing voices.
void Sound::Init(int TotalV)
{
	Free();

	aVoices.resize(TotalV);

	ClearVec3(vListenerPos);
	ClearVec3(vListenerRot);
	ClearVec3(vListenerVel);

	fGlobalPitch = 1.0f;
	fMinDist = 75.0f;
	fMaxDist = 500.0f;

	iLastVoice = -1;
}

void Sound::Free()
{
	aVoices.clear();
	aBuffers.clear();
}

//Sound atten is 1/2 at 2x min dist.
bool Sound::SetDistanceModel(float mindist, float maxdist)
{
	fMinDist = mindist;
	fMaxDist = maxdist;
	return true;
}

//Returns bool, use GetBufferCount() - 1 after to find waveform ID number.
void Sound::AddBuffer(SoundBuffer *wv)
{
	aBuffers.push_back(wv);
}

//Plays an added buffer on first open voice.
//Use GetLastVoice to find voice used.
void Sound::PlayBuffer(int num, float pri)
{
	if(num < 0)
		return;

	iLastVoice = -1;

	float fLowestPriority = std::numeric_limits<float>::infinity();
	int iLowestVoice = -1;

	for(int i = 0; iLastVoice == -1 && i < aVoices.size(); i++)
	{
		if(aVoices[i].voice.getStatus() == sf::Sound::Stopped)
		{
			iLastVoice = i;
		}
		else if(aVoices[i].fPriority < fLowestPriority)
		{
			iLowestVoice = i;
			fLowestPriority = aVoices[i].fPriority;
		}
	}

	if(iLastVoice == -1 && pri > fLowestPriority)
	{
		Stop(iLowestVoice);
		iLastVoice = iLowestVoice;
	}

	if(iLastVoice != -1)
	{
		aVoices[iLastVoice].iBufferId = num;
		aVoices[iLastVoice].fPriority = pri;
		aVoices[iLastVoice].fPitchScale = aVoices[iLastVoice].fGain = 1.0f;
		ClearVec3(aVoices[iLastVoice].vPos);
		ClearVec3(aVoices[iLastVoice].vVel);
		aVoices[iLastVoice].fMaxDistScale = aVoices[iLastVoice].fMinDistScale = 1.0f;
		aVoices[iLastVoice].b2DVoice = true;

		aVoices[iLastVoice].voice.setBuffer(aBuffers[num]->buffer);
		aVoices[iLastVoice].voice.setLoop(aBuffers[num]->bLoop);
		aVoices[iLastVoice].voice.setAttenuation(0.0f);
		aVoices[iLastVoice].voice.setVolume(0.0f);
		aVoices[iLastVoice].voice.play();
	}
}

//Stops one voice.
void Sound::Stop(int v)
{
	if(v >= 0 && v < aVoices.size())
		aVoices[v].voice.stop();
}

//Stops 'em all.
void Sound::StopAll()
{
	for(auto it = aVoices.begin(); it != aVoices.end(); ++it)
		it->voice.stop();
}

//Returns nonzero if voice is playing, zero if stopped or bad voice num.
bool Sound::GetVoiceStatus(int v)
{
	if(v >= 0 && v < aVoices.size())
	{
		return aVoices[v].voice.getStatus() == sf::Sound::Playing;
	}

	return false;
}

//Returns the wave ID last played on a voice.
int Sound::GetVoiceWave(int v)
{
	if(v >= 0 && v < aVoices.size())
	{
		return aVoices[v].iBufferId;
	}

	return -1;
}

//Performs update of 3D spatial positions.
bool Sound::PulseAudio()
{
	sf::Listener::setPosition(vListenerPos[0], vListenerPos[1], vListenerPos[2]);
	sf::Listener::setDirection(vListenerRot[0], vListenerRot[1], vListenerRot[2]);

	for(auto it = aVoices.begin(); it != aVoices.end(); ++it)
	{
		if(it->voice.getStatus() == sf::Sound::Playing)
		{
			if(it->b2DVoice)
			{
				it->voice.setVolume(aBuffers[it->iBufferId]->fVolumeBias * 0.5f * 100.0f);
			}
			else
			{
				Vec3 norm;
				SubVec3(it->vPos, vListenerPos, norm);
				float dist = LengthVec3(norm);

				float tmpMaxDist = (it->fMaxDistScale * fMaxDist);
				float tmpMinDist = (it->fMinDistScale * fMinDist);
				float gainmult = max(1.0f - max(dist - tmpMinDist, 0.0f) / max(tmpMaxDist - tmpMinDist, 0.001f),0.0f);

				if(gainmult < 0.0001f) // can't hear it
					it->voice.stop();
				else
				{				
					//Doppler effect.  pitch / (1 - soundspeedfrac)
					float pitch = it->fPitchScale;
					Vec3 tv1, tv2;
					float d1, d2;
					CopyVec3(vListenerPos, tv1);
					CopyVec3(it->vPos, tv2);
					d1 = DistVec3(tv1, tv2);
					ScaleAddVec3(vListenerVel, 0.1f, tv1);
					ScaleAddVec3(it->vVel, 0.1f, tv2);
					d2 = DistVec3(tv1, tv2);
					float closingvel = (d1 - d2) * 10.0f;
					pitch /= max(0.1f, 1.0f - (closingvel / 340.0f));

					// Update Info
					it->voice.setVolume(aBuffers[it->iBufferId]->fVolumeBias * it->fGain * gainmult * 0.5f * 100.0f);
					it->voice.setPitch(pitch * fGlobalPitch);
				}
			}
		}
	}

	return true;
}

void Sound::SetListenerPos(Vec3 pos)
{
	CopyVec3(pos, vListenerPos);
}

void Sound::SetListenerRot(Rot3 rot)
{
	CopyVec3(rot, vListenerRot);
}

void Sound::SetListenerVel(Vec3 vel)
{
	CopyVec3(vel, vListenerVel);
	ScaleVec3(vListenerVel, 0.001f);
}

void Sound::SetVoicePos(int v, Vec3 pos)
{
	if(v >= 0 && v < aVoices.size())
		CopyVec3(pos, aVoices[v].vPos);
}

void Sound::SetVoiceVel(int v, Vec3 vel)
{
	if(v >= 0 && v < aVoices.size())
		CopyVec3(vel, aVoices[v].vVel);
}

void Sound::SetVoicePriority(int v, float pri)
{
	if(v >= 0 && v < aVoices.size())
		aVoices[v].fPriority = pri;
}

void Sound::SetVoiceGain(int v, float gain)
{
	if(v >= 0 && v < aVoices.size())
		aVoices[v].fGain = gain;
}

void Sound::SetVoicePitch(int v, float pitch)
{
	if(v >= 0 && v < aVoices.size())
		aVoices[v].fPitchScale = pitch;
}

void Sound::SetVoiceDistanceScale(int v, float minscale, float maxscale)
{
	if(v >= 0 && v < aVoices.size())
	{
		aVoices[v].fMaxDistScale = maxscale;
		aVoices[v].fMinDistScale = minscale;
	}
}

void Sound::SetVoice3D(int voice, Vec3 pos, Vec3 vel, float gain, float pitch, float pri)
{
	if(voice >= 0 && voice < aVoices.size())
	{
		aVoices[voice].b2DVoice = false;
		SetVoicePos(voice, pos);
		SetVoiceVel(voice, vel);
		SetVoicePitch(voice, pitch);
		SetVoicePriority(voice, pri);
		SetVoiceGain(voice, gain);
	}
}

void Sound::SetVoice2D(int voice)
{
	if(voice >= 0 && voice < aVoices.size())
	{
		aVoices[voice].voice.setPosition(0, 0, 0);
		aVoices[voice].voice.setPitch(1.0f);
		aVoices[voice].b2DVoice = true;
	}
}

void Sound::StartMusic(const CStr& sFilename)
{
	bool bSucceeded = music.openFromFile(sFilename.get());
	music.play();
}

void Sound::StopMusic()
{
	music.stop();
}

void Sound::PauseMusic(bool pause)
{
	if(pause && music.getStatus() == sf::Sound::Playing)
		music.pause();

	if(!pause && music.getStatus() == sf::Sound::Paused)
		music.play();
}

bool Sound::IsMusicPlaying()
{
	return music.getStatus() == sf::Sound::Playing;
}

//3D sound effects only.
void Sound::SetSoundVolume(float gain)
{
	sf::Listener::setGlobalVolume(gain * 100.0f);
}

//3D sound effects only.
void Sound::SetGlobalPitch(float pitch)
{
	fGlobalPitch = pitch;
}

void Sound::SetMusicVolume(float gain)
{
	music.setVolume(gain * 100.0f);
}
