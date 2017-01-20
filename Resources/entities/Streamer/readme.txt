These are really Explo entities, but using a special mode and a few tricks to
drastically change their appearance.



RandVelAddMin = x, y, z minimum bounding box values for random velocity addition on spawn.
RandVelAddMax = x, y, z maximum values for random velocity add.

FlyingTrailMode = 1 to make explosion a Flying Trail, which will have 'count' puffs in its
                  tail.  Default is 0, for normal explosions.
FlyingTrailTime = seconds between creation of each puff.

Use the above, in combination with EntityChain and EntityChainMultiply, to create flying
trail streamer explosions.

TimeWarp = seconds to run physics at initialization time, to start streamer partly formed.

