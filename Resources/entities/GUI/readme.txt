All GUI Entities:

Derived from Sprite, so all texture parameters apply, for GUI ents using images.

removetime = seconds for entity to linger/fly off when removed.
addtime = seconds for entity transition when first added.


GUIBackground:

xscale = how many copies of texture horizontally.
yscale = how many copies of texture vertically.

rotatetime = seconds for full rotation.

rotateshift = radius of rotation, as relating to texture size.

VertsX = for rippling backgrounds, how many vertices horizontally.
VertsY = vertices vertically (must be at least 2).
PerturbScale = how many "waves" will cover the screen up and down.
PerturbXV = amount to move mesh through waves horizontally each second.
PerturbYV = ditto vertically (1.0 is a full screen).
PerturbStart = perturbation amount when image is faded out.
PerturbEnd = ditto when faded in (1.0 would perturb each point up to a screen's distance).


GUIButton:

overcolor = r, g, b color to modulate by when mouse touches it.
clickcolor = r, g, b color for when button is clicked.

fonttype = Type of Text Class to use for text display.

