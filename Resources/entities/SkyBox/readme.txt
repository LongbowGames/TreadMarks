Use the following parameters in the SkyBox entity, or in the Map Config Text.

ColorGain = red, green, blue       default is 0.5 for all
ColorBias = red, green, blue       default is 0.5 for all
ColorScale = red, green, blue      default is 1.0 for all
ColorShift = red, green, blue      default is 0.0 for all

Gain and Bias are the same as in Texturizer, Scale is multiplied with the color
channel, Shift is added to the color channel.  First Gain is applied, then Bias,
then the result is multiplied by Scale, and finally Shift is added.  Each color
channel is handled independendtly.

AutoFog = whether fog color should be computed from sky textures or not (default is 1, on)

FilterSky = should sky textures be box-filtered for smoothness (default is 1, on)

ScorchEco = EcoSystem id that should be used for tread marks/scorch marks instead of black.

DetailRotSpeed = degrees per second to rotate the detail texture.
DetailRotRadius = meter radius for circle around which to rotate.

SkyRotateSpeed = degrees per second to rotate skybox around Y axis, def 0.0.


