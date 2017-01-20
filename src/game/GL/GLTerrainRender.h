// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

#ifndef GLTERRAINRENDER_H
#define GLTERRAINRENDER_H

#include "../Render.h"

class GLRenderEngine : public RenderEngine {
public:
	bool GLTerrainRender(Terrain *map, Camera *cam, int flags, float quality, int ms = 0);	//Optional number of msecs for frame parameter.
	bool GLRenderWater(Terrain *map, Camera *cam, int flags, float quality);
	const char *GLTerrainDriverName();
};

#endif

