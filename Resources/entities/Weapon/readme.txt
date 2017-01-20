secmesh =
secmesh1 =
secmesh2 = high, med, low secondary meshes, for rotating gun barrels, sat dishes, drill bits, etc.

projectile = type of projectile to fire.

ammo = starting ammunition count.

bolton = id of bolt-on location on tank (0 = bauble, 1 = turret, 2 = hull, 3 = rear).

reloadtime = seconds between rounds.

launchangle = pitch, yaw, roll, angles for launched shell.

launchcoords = x, y, z offset from mesh origin for projectiles to start.

launchspread = pitch, yaw, roll, degrees of variation for launch angle.

launchcount = number of projectiles to fire simultaneously.

globalcoords = 1 specifies that launch coords and launch angle are global, and not relative to weapon orientation.

firesoundent = sound entity to run looped during firing sequence (one-shot sounds OK now too!).

weight = numerical probability factor for appearance at checkpoints.

SecRotMin = a, b, c degrees per second minimum rotation speed of secondary mesh.
SecRotMax = a, b, c degrees per sec max rotation speed (when fire held).
SecRotWindup = seconds for change from min to max rotation speed.

SecMeshOffset = x, y, z offset coords of center of secondary mesh.

AmmoDisplayMult = multiplier for client display of ammo, default is 1.

SecEnt = class/type name of secondary entity to create when the weapon is created, and move with the weapon.
SecEntOffset = x, y, z offset coords for sec ent from weapon origin.  This should be a client side effect only.

pickupsound = sound entity type to play when weapon is picked up (global positional).
spawnsound = sound type to play when weapon spawns.
spawnentity = entity (def explosphere class) to create when weapon spawns.
inentity = entity (def class explosphere class) to use when weapon connects to tank.
outentity = entity (def explosphere class) to use when weapon is picked up or disappears.

TransmuteTexture = 1 to cause tank to inherit weapon's texture and renderflags.

TransmutePerturb = amount to cause tank to go all jelloey as it's driving with weapon (1.0 is good).

