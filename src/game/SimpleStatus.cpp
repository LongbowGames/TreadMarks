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

#include "SimpleStatus.h"
#include <GL/glew.h>

SimpleStatus::SimpleStatus(){
	charw = charh = 0.0f;
}
bool SimpleStatus::Init(FileManager *fm, float w, float h){
	if(!fm) return false;
	int c;
	for(c = 0; c < 36; c++){
		char l[2] = {0, 0};
		if(c < 26) l[0] = 'A' + c;
		else l[0] = '0' + (c - 26);
		if(fm->Open("Letters/" + CStr(l) + ".lwo")){
			chars[c].LoadLWO(fm->GetFile());
		}
	}
	charw = w;
	charh = h;
	return true;
}
int SimpleStatus::Status(const char *text){
	str = text;
	return 1;
}
int SimpleStatus::Draw(GenericBuffer& glb){
	if(str.len() <= 0 || charw == 0.0f || charh == 0.0f) return 0;
	//
	glb.Clear();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, 1.0f, 1.0f, 0.0f, -100.0f, 100.0f);	//Set up top-down ortho matrix.
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_FOG);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	//
	float cx = (1.0f - (charw * (float)str.len())) / 2.0f;
	float cy = 0.5f;
	for(int i = 0; i < str.len(); i++){
		Mesh *m = 0;
		char c = str[i];
		if(c >= 'a' && c <= 'z') m = &chars[c - 'a'];
		if(c >= 'A' && c <= 'Z') m = &chars[c - 'A'];
		if(c >= '0' && c <= '9') m = &chars[(c - '0') + 26];
		if(m && m->Poly && m->Vertex){

			glBegin(GL_TRIANGLES);
			float xtxl = (1.0f / (float)std::max(glb.Width(), 1));
			float ytxl = (1.0f / (float)std::max(glb.Height(), 1));
			for(int oversamp = 0; oversamp < 4; oversamp++){
				for(int p = 0; p < m->nPoly; p++){
					for(int v = 0; v < 3; v++){
						float x = m->Vertex[m->Poly[p].Points[v]][0] * charw + cx + charw * 0.5f;
						float y = m->Vertex[m->Poly[p].Points[v]][1] * -charh + cy;
						if(oversamp >> 1){
							x += xtxl * 0.5f;
							y += ytxl * (float)(oversamp & 1);
						}else{
							y += ytxl * 0.5f;
							x += xtxl * (float)(oversamp & 1);
						}
						float c = ((1.0f - fabsf(x - 0.5f)) * 1.25f - 0.25f) * 0.25f;
						glColor3f(c, c, c);
						glVertex3f(x, y, 0);
					}
				}
			}
			glEnd();
		}
		cx += charw;
	}
	//
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glb.Swap();
	return 1;
}

