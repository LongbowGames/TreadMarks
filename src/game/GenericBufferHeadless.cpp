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

// Stub for headless builds

#include "GenericBuffer.h"

#include "CfgParse.h"
#include "TankGame.h"
#include "TankRacing.h"

unsigned int TextureCompressionType=0;

GenericBuffer::GenericBuffer() {
	m_fullscreen = false;
	m_w = m_h = m_bpp = 0;
	destroying = 0;
	bFocused = false;
}
GenericBuffer::~GenericBuffer(){
}

void GenericBuffer::CreateBuffer(int w, int h, bool fullscreen, const char *Name, const char* sIcon)
{
}

void GenericBuffer::Resize(int w, int h)
{
}

bool GenericBuffer::Clear(){
	return false;
}

bool GenericBuffer::Swap(){
	return false;
}

int GenericBuffer::Width() const { return m_w; }
int GenericBuffer::Height() const { return m_h; }
int GenericBuffer::BPP() const { return m_bpp; }
bool GenericBuffer::Fullscreen() const { return m_fullscreen; }
bool GenericBuffer::Valid() const { return false; }

bool GenericBuffer::Destroy(bool dwindow){
	return false;
}

void GenericBuffer::CaptureMouse()
{
}
