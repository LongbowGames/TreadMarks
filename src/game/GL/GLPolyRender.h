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


#ifndef GLPOLYRENDER_H
#define GLPOLYRENDER_H

#include "../Poly.h"

class GLPolyRender : public PolyRender {
public:
	bool GLDoRender();	//Renders using OpenGL.
	bool GLDoRenderTrans();	//Renders transparents, MUST BE CALLED AFTER ABOVE CALL!
	bool GLDoRenderOrtho();
	bool GLDoRenderSecondary();	//Renders using OpenGL.

	void GLResetStates();
	void GLBlendMode(int mode);	//Will not re-set already set modes.
	void GLBindTexture(unsigned int name);	//Will not re-bind already bound textures.

	//Russ
	void GLRenderMeshObject(MeshObject *thismesh, PolyRender *PR);
	void GLRenderSpriteObject(SpriteObject *thissprite, PolyRender *PR);
	void GLRenderParticleCloudObject(ParticleCloudObject *thispc, PolyRender *PR);
	void GLRenderStringObject(StringObject *thisstring, PolyRender *PR);
	void GLRenderLineMapObject(LineMapObject *thislinemap, PolyRender *PR);
	void GLRenderTilingTextureObject(TilingTextureObject *thistile, PolyRender *PR);
	void GLRenderChamfered2DBoxObject(Chamfered2DBoxObject *thisbox, PolyRender *PR);
};

#endif

