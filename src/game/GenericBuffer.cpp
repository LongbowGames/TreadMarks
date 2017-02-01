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

#include "GenericBuffer.h"

#include "CfgParse.h"
#include "TankGame.h"
#include "TankRacing.h"

#include <SFML/Graphics.hpp>
#include <GL/glew.h>

int MaxOpenGLTextureSize = 0;
unsigned int TextureCompressionType=0;

GLenum GL_Best_Clamp = GL_CLAMP;

GenericBuffer::GenericBuffer() {
	m_fullscreen = false;
	m_w = m_h = m_bpp = 0;
	destroying = 0;
	bFocused = false;
}
GenericBuffer::~GenericBuffer(){
	Destroy();
}

class GLContextSingleton
{
public:
	static GLContextSingleton& Get()
	{
		static GLContextSingleton rc;
		return rc;
	}

	const sf::ContextSettings& GetContext() const { return context; }

private:
	sf::ContextSettings context;

	GLContextSingleton() : context(16)
	{
		glewInit();
	}
};

void GenericBuffer::CreateBuffer(int w, int h, bool fullscreen, const char *Name, const char* sIcon)
{
	try
	{
		auto mode = sf::VideoMode::getDesktopMode();
		mode.width = w;
		mode.height = h;
		window.create(mode, Name, fullscreen ? sf::Style::Fullscreen : sf::Style::Default, GLContextSingleton::Get().GetContext());
		bFocused = true;
		window.setActive();

		sf::Image image;
		image.loadFromFile(sIcon);
		window.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
		window.setMouseCursorVisible(false);

		glViewport(0, 0, w, h);

		//Output debug info.
		OutputDebugLog(CStr("\nGL_VENDOR:\n") + (char*)glGetString(GL_VENDOR) + "\n");
		OutputDebugLog(CStr("GL_RENDERER:\n") + (char*)glGetString(GL_RENDERER) + "\n");
		OutputDebugLog(CStr("GL_VERSION:\n") + (char*)glGetString(GL_VERSION) + "\n");
		OutputDebugLog(CStr("GL_EXTENSIONS:\n") + (char*)glGetString(GL_EXTENSIONS) + "\n\n");

		//Find maximum hardware texture size.
		MaxOpenGLTextureSize = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxOpenGLTextureSize);
		
		if(!GLEW_ARB_multitexture)
			OutputDebugLog("GL_ARB_multitexture NOT found.\n");
		else
			OutputDebugLog("GL_ARB_multitexture detected.\n");
		//

		if(!GLEW_EXT_compiled_vertex_array)
			OutputDebugLog("GL_EXT_compiled_vertex_array NOT found.\n");
		else
			OutputDebugLog("GL_EXT_compiled_vertex_array detected.\n");
		//
		GL_Best_Clamp = GL_CLAMP;
		if(!GLEW_EXT_texture_edge_clamp)
			OutputDebugLog("GL_EXT_texture_edge_clamp NOT found.\n");
		else
		{
			OutputDebugLog("GL_EXT_texture_edge_clamp detected.\n");
			GL_Best_Clamp = GL_CLAMP_TO_EDGE;
		}
		//
		if(GLEW_S3_s3tc)
		{
			OutputDebugLog("GL_S3_s3tc texture compression detected.\n");
			TextureCompressionType = GL_RGBA_S3TC;
		}
		else
		{
			OutputDebugLog("GL_S3_s3tc texture compression NOT found.\n");

			if(GLEW_ARB_texture_compression)
			{
				OutputDebugLog("ARB texture compression detected.\n");
				TextureCompressionType = GL_COMPRESSED_RGBA_ARB;
			}
			else
				OutputDebugLog("ARB texture compression NOT found.\n");
		}
	}
	catch(...)
	{
		window.close();
		throw;
	}

	m_w = w;
	m_h = h;
	m_fullscreen = fullscreen;
}

void GenericBuffer::Resize(int w, int h)
{
	if(!m_fullscreen)
	{
		window.setSize(sf::Vector2u(w, h));
		m_w = w;
		m_h = h;
	}
}

bool GenericBuffer::Clear(){
	if(window.isOpen()){
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return true;
	}
	return false;
}

bool GenericBuffer::Swap(){
	if(window.isOpen()){
		window.display();

		return true;
	}
	return false;
}

int GenericBuffer::Width() const { return m_w; }
int GenericBuffer::Height() const { return m_h; }
int GenericBuffer::BPP() const { return m_bpp; }
bool GenericBuffer::Fullscreen() const { return m_fullscreen; }
bool GenericBuffer::Valid() const { return window.isOpen(); }

bool GenericBuffer::PollEvent(sf::Event& sfmlEvent)
{
	if(window.pollEvent(sfmlEvent))
	{
		switch(sfmlEvent.type)
		{
		case sf::Event::LostFocus:
			bFocused = false;
			break;
		case sf::Event::GainedFocus:
			bFocused = true;
			break;
		}

		return true;
	}
	else
		return false;
}

bool GenericBuffer::Destroy(bool dwindow){
	//
	MaxOpenGLTextureSize = 0;

	destroying = 1;
	window.close();

	m_w = m_h = m_bpp = 0;

	return false;
}

void GenericBuffer::CaptureMouse()
{
	if(bFocused)
	{
		sf::Mouse::setPosition(sf::Vector2i(m_w/2, m_h/2), window);
		// TODO: Use system calls to confine the mouse the the window to avoid accidentally clicking outside.
	}
}
