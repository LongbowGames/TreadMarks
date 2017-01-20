See readme.txt files in specific directories for class specific info.


For all Sprite derived classes (Tree, Smoke, Explo, Projectile, Shrubbery),
setting AlphaGrad to 1 will turn on Gradiated Alpha, and to 3 will turn
on Gradiated Alpha Only, which will set the color components to 1, basing
Alpha on the intensities of the pixels in the sprite.  An AlphaGrad of 2
is like 3 but with a little darkening where the colors go to black,
causing a hint of the dark outline that AlphaGrad 1 causes.

Now AlphaGrad = 1 mode is more flexible, with the AlphaGradBias parameter.
Default is 0.5, and setting it higher or lower will bias the alpha
(transparency) values for each pixel up or down.  0.8 for example will
make the sprite much more opaque, but still with fade at the edges.


Also for all Sprite (and now Mesh) derived classes, setting SizeBias
will specify the number of mip-map detail levels _below_ the global
maximum that the texture will be sized to.  Use this to keep low-priority
textures at the correct relative size to high-pri textures.  e.g. if most
tanks use 512 textures and weapons use 256, then SizeBias should be at
least -1 for all weapons, so that when the user selects 256 as a max
tex res, the weapons will be sized down to 128.


All meshes, set "envlight" to whether the environment map should be
oriented so up faces the global light source.  This is best for envmaps
that have a hot spot in the center, so they'll point at the light and
look correct.  Not good for envmapping with the sky though.


Use FadeBias to bias the fade-out for smoke, explosions, and crud.
A FadeBias of 0.5 means linear fade out, 0.8 or so will hold opacity much
longer then quickly fade out, 0.2 or so will fade out very quickly and
stay mostly transparent for the rest of the time.


New, for all classes derived from Sprite and Mesh (basically everything).
Set RestOnGround to 1 to make the entity rest on the terrain, and follow
it up or down when modified by weapons, etc.  Set GroundOffset to the
distance above (positive) or below (negative) the ground that the ent's
center point will be.

For some Mesh derived classes, use ScaleMatrix = x, y, z to set display-
time scaling factors for the object.


For Mesh derived classes and for Explos, entities can be chained.  When
the first entity is created, it creates all its chained entities (which
will NOT chain anything more themselves, so chaining your own type is OK).

EntityChainN = class/type, where N starts at 0 specifies a class/type to chain.
EntityChainMode = "random" to only spawn one of the list randomly, or "cycle"
                  to cycle through the list.  Default is to spawn entire list.
EntityChainMultiply = number of times to perform spawn operation, default 1.
EntityChainNochainOverride = 1 to allow spawned ents to chain ents themselves,
                  needed when chaining a Streamer, since they use chaining innately.

ServerSynch = 0  Set this to 0 to specify entity file as client side only, and
not to be synched up from the server.

