Spewer Class, spews out other entities.  Inherits from Mesh, and needs a Mesh or Sprite assigned for itself.

SpewTime = seconds between spew, or average random time between spew.

RandomTime = 1 for random spew time, 0 for regular spew time.

CycleType = 1 to cycle between spewed entity types, 0 to pick randomly.

SpewMin = x, y, z   min bounds of random spew velocity bounding box. (e.g. -10, -10, -10)
SpewMax = x, y, z   max bounds of same. (e.g. 10, 10, 10)

PropagateSpew = 1 to propagate spewed entities over network, 0 to spew entities locally on clients.
Careful, DO NOT set to 0 when spewing Dangerous and/or non-Transitory entities!  ONLY use 0 for
spewing harmless graphical entities such as smoke, etc.

LaunchCoords = x, y, z offset for launch of spewed entity.

Iterations = number of ents to spew each burst, default 1.

MaxSpewCount = number of entities to spew before removing self, default 0 (infinite).
    Use this with a low SpewTime and high Iterations as a more powerful EntityChainer.

