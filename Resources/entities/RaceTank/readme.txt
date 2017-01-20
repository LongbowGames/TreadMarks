CrudForce - multiplies the amount of accelerative force required to produce flying crud.
The base value is set in the SkyPlane entity and/or the Map config, and can be different
for each EcoSystem on the map.

MaxSpeed - maximum movement speed in meters per second.  (3600 seconds per hour.)

AccelSpeed - acceleration in meters per second.

Use Mesh1 and Mesh2 for medium and low detail object files.

Use LodBias to determine the fraction of 1 Meter == 1 Pixel distance for each
Polygon LOD level switch.  Default is 0.1.

OnFireType, smoke class type for burning damage.

BaubleType, bauble class type for antenna bauble.

HealthRegen, fraction of health to regen each second (default 0.02 or so).

Armor, basic armor value, as fraction of damage to let through.

AmmoMax = maximum pea-shooter ammo, default 8 or so.


GravityScale = amount to scale global gravity setting when applied to this tank type.

FrictionScale = amount to scale global friction setting for this tank type.  default 1.0.

RespawnExplo = class explo type to use for battle match respawn.

MultiKillSound = sound entity (announcer) to play for multi-kills.
MultiKillTime = seconds since last kill for a multi-kill.

