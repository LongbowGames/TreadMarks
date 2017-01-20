Use volume to specify the basic sound volume, and freqvariance to specify
the variation of frequency (0.0 is none, 1.0 would be zero to double, and
Not Good).

DistanceScale = scaling factor for distance from listener for attenuation.
Set to 0.5 to make sounds stay loud twice as long, 2.0 to stay loud half as long, etc.

Sound = first sound
AltSound = second sound
AltSoundX = third to tenth sounds, X can go from 2 to 10 now.

RampUp = seconds to ramp frequency from RampFreq to desired on create.
RampDown = seconds to ramp freq from desired to RampFreq on NiceRemove; only works for loopers.
RampFreq = freq fraction to ramp to/from.

Is2DSound = 1 to specify that the sound should be played without spatial effects.

Announcer = set this to 1 (along with Is2DSound!) for Announcer Vocals.

VolumeBias = load-time volume dynamics for sound file, 0.5 is no change.

