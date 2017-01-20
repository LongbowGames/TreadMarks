
guidespeed = fraction of distance to target for m/s acceleration.  e.g. 0.1 would be gravitic accel at 100 meters.
guidemax = maximum accelerative speed change, 5 is default.
guidebias = amount to bias rear target distances in closest target calc, 2.0 will make rear targets half as likely.
guideramp = seconds before guidance is at max force, default is 1.0.

timetolive = seconds to let the projectile live before terminating it

FollowGround = set to non-zero for amount to sit off, bounce off of, and/or follow the ground instead of blowing up.

Bounce = floating point amount of collision force to bounce off ground with (e.g. 0.5).

LaunchInertia = amount of launch platform's (e.g. tank) momentum to transfer to projectile.

Splash Damage:
(note, when shot directly, an entity will NOT receive splash damage as well)

SplashRadius = radius in meters of splash damage sphere.
SplashDamage = damage caused at center of sphere, decreasing outwards.
SplashPush = meters per second velocity to impart, at center of sphere (calced at 1 tank mass).

Detonate = 1 to make projectile detonate at end of life span, 0 to just disappear (default).

SmokeTrailOffset = x, y, z  offset from center of projectile where smoke trail should come from.

HitScan = set to 1 for instant-hit weapons
HitScanRange = range in meters for hitscan weapon (time to live doesn't matter)

Transitory = whether server or client controls entity destruction.  Default is 1, set transitory to 0
for any projectiles that should be exactly mirrored on clients, such as very guided missiles.

SelfHitDelay = seconds before projectile can hit/hurt launching tank.

Name = what the tank death message will report as "fragged with X".
NameID = index into Weapons.TXT for localisation

iterations = how many steps to take per virtual second for hitscan weapons,
e.g. number of smoke trails per "speed" meters, default is 10.

SmokeTrailTime = fraction of a second between smoke trail entity creations, def is 0.02.

