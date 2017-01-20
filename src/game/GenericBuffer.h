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

/*
	GLBuffer OpenGL buffer window setup class.

	Paradigm shift from BackBuffer:  Due to the way OpenGL works
	under Windows, the window must be destroyed and recreated for
	screen mode changes, so now the Main Idle Loop must be God,
	and closing the window can not mean app death.  Put app death
	confirmation flag in WM_CLOSE or elsewhere, and have main loop
	post Quit Message.  Return 0 from WM_DESTROY.
*/

#ifndef GENBUFFER_H
#define GENBUFFER_H

#include <memory>

#ifndef HEADLESS
#include <SFML/Window.hpp>
#endif


class GenericBuffer{
public:
    GenericBuffer();
    ~GenericBuffer();
	void CreateBuffer(int w, int h, bool fullscreen, const char *Name, const char* sIcon);
	void Resize(int w, int h);
    bool Destroy(bool dwindow = false);
    bool Clear();
    bool Swap();
	int Width() const;
	int Height() const;
	int BPP() const;
	bool Fullscreen() const;
	bool Valid() const;       //Returns true after the buffer has been created.
	bool Focused() const { return bFocused; }

#ifndef HEADLESS
	bool PollEvent(sf::Event& sfmlEvent);
#endif

	void CaptureMouse(); // Call after input handling if you need the mouse captured.

private:
	void SetIcon(const char* sIcoFile);

	bool m_fullscreen;
	bool bFocused;

	int m_w, m_h, m_bpp;
	int destroying;

#ifndef HEADLESS
	sf::Window window;
#endif
};

#ifdef WIN32
//Contains the maximum hardware texture size, or 0 if none specified or there is an error.
extern int MaxOpenGLTextureSize;
#endif
extern unsigned int TextureCompressionType;

#endif
