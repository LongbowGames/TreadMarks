Generic polygon mesh entities.

mesh = LWO file
texture = texture bitmap
scale = load time scaling
collide = can things hit us

smoothshade = 1 for smooth light and envmap shading
solidlight = 1 for solid full-bright colors

mesh1, mesh2, alternate LOD meshes
lodbias = level of detail switch bias, default is 0.1, increase to keep high detail longer.

cullface = cull backfacing polies, default is 1

MeshOffset = x, y, z   amount to offset the center of the mesh when actually rendered.


ConstRotate = a, b, c, degrees to rotate per second around x, y, z axes.

ConstWave = x, y, z, half of amount to oscillate in each direction (e.g. setting 2 will go from 2 to -2).

ConstWaveTime = number of seconds for movement oscillation to take place in.

EnvBasisModel = 1 to lock the environment map basis matrix to the model's rotation.

SecondaryFrustum = set to 1 if this object should be rendered in the secondary frustum,
    (e.g. for 3D objects to appear on top of the 2D menus).

