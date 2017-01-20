Base = meters to lift individual explosion puffs off ground from their centers.

count = number of explosion puffs in cloud.

See Smoke/readme.txt for more.


RandVelAddMin = x, y, z minimum bounding box values for random velocity addition on spawn.
RandVelAddMax = x, y, z maximum values for random velocity add.

FlyingTrailMode = 1 to make explosion a Flying Trail, which will have 'count' puffs in its
                  tail.  Default is 0, for normal explosions.
FlyingTrailTime = seconds between creation of each puff.

Use the above, in combination with EntityChain and EntityChainMultiply, to create flying
trail streamer explosions.

