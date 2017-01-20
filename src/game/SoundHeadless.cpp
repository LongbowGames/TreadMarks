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
//Stub for headless builds
//

#include "Sound.h"

using namespace std;

SoundBuffer::SoundBuffer()
{
}

SoundVoice::SoundVoice()
{
}

// TotalV is the number of simultaneous mixing voices.
void Sound::Init(int TotalV)
{
}

void Sound::Free()
{
}

//Sound atten is 1/2 at 2x min dist.
bool Sound::SetDistanceModel(float mindist, float maxdist)
{
	return false;
}

//Returns bool, use GetBufferCount() - 1 after to find waveform ID number.
void Sound::AddBuffer(SoundBuffer *wv)
{
}

//Plays an added buffer on first open voice.
//Use GetLastVoice to find voice used.
void Sound::PlayBuffer(int num, float pri)
{
}

//Stops one voice.
void Sound::Stop(int v)
{
}

//Stops 'em all.
void Sound::StopAll()
{
}

//Returns nonzero if voice is playing, zero if stopped or bad voice num.
bool Sound::GetVoiceStatus(int v)
{
	return false;
}

//Returns the wave ID last played on a voice.
int Sound::GetVoiceWave(int v)
{
	return 0;
}

//Performs update of 3D spatial positions.
bool Sound::PulseAudio()
{
	return false;
}

void Sound::SetListenerPos(Vec3 pos)
{
}

void Sound::SetListenerRot(Rot3 rot)
{
}

void Sound::SetListenerVel(Vec3 vel)
{
}

void Sound::SetVoicePos(int v, Vec3 pos)
{
}

void Sound::SetVoiceVel(int v, Vec3 vel)
{
}

void Sound::SetVoicePriority(int v, float pri)
{
}

void Sound::SetVoiceGain(int v, float gain)
{
}

void Sound::SetVoicePitch(int v, float pitch)
{
}

void Sound::SetVoiceDistanceScale(int v, float minscale, float maxscale)
{
}

void Sound::SetVoice3D(int voice, Vec3 pos, Vec3 vel, float gain, float pitch, float pri)
{
}

void Sound::SetVoice2D(int voice)
{
}

void Sound::StartMusic(const CStr& sFilename)
{
}

void Sound::StopMusic()
{
}

void Sound::PauseMusic(bool pause)
{
}

bool Sound::IsMusicPlaying()
{
	return false;
}

//3D sound effects only.
void Sound::SetSoundVolume(float gain)
{
}

//3D sound effects only.
void Sound::SetGlobalPitch(float pitch)
{
}

void Sound::SetMusicVolume(float gain)
{
}
