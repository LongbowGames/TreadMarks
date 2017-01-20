StatusX = x ortho coord (0, 0 is top left, 1, 1 is bottom right) of status display anchor.
StatusY = y coord of same.
StatusW = width of characters in status display.
StatusH = height of characters and lines in same (use negative to push lines upwards).
StatusLife = seconds for each line to live.
StatusFont = font entity for text.

StatusPriorityColor = r, g, b default status color.
StatusPriorityColorX = X from 0 to 9 sets color for that priority level of text.
StatusPrioritySound = default sound entity for status text to make.
StatusPrioritySoundX = level specific sound.

EcoCrud = the default type of Crud class entity to spew on this terrain.
EcoCrudForce = the default accelerative force required to spew crud, 10.0 is good.
EcoCrud0, EcoCrud1, EcoCrud2, etc., specific value for specific EcoSystem, starting at 0.
EcoCrudForce0, EcoCrudForce1, etc., ditto.

DMPeaShooter = initial tank pea shooter ammo in deathmatch
RacePeaShooter = initial tank pea shooter ammo in race mode

GlobalMultikillDecEvery = seconds to elapse before decrement of multikill counter, def 2.0.
GlobalMultikillThresh = when multikill counter hits this, play sound, def 4.
GlobalMultikillSound = sound entity to play, def "chaos".

