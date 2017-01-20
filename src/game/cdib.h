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

#ifndef CREATEDIB_H
#define CREATEDIB_H

#include <windows.h>
#include <windowsx.h>

class CreateDib
{
	public:

		// Constructor
		CreateDib(void);

		// Destructor
		~CreateDib(void);

		// Creation of hBitmap
		bool CreateHBitmap(HWND hWnd, int width, int height, int bpp = 8);
		void DeleteHBitmap(void);

		// Palette Manipulation
		void SetPalette(PALETTEENTRY *palette);
		void RealizePalette(void);

		// Blitting interface 'Make sure to: 1) Lock surface, 2)Blit, 3)Unlock surface'
		BYTE* Lock(void);
		bool Blit(int destX, int destY, int sourceX, int sourceY, int sourceWidth, int sourceHeight);
		bool StretchBlit(int destX, int destY, int destWidth, int destHeight, int sourceX, int sourceY, int sourceWidth, int sourceHeight);
		bool StretchBlitToWnd();	//Blits to the dimensions of the attached window.
		bool ScrollDib(int dx, int dy);	//Wrap-around scrolls the contents of the bitmap.
		void Unlock(void);

		// Called from a WM_PAINT command (No locking required)
		bool PaintBlit(HDC destHdc, int destX, int destY, int sourceX, int sourceY, int sourceWidth, int sourceHeight, int dW = 0, int dH = 0);

		// Surface info
		BYTE* GetSurface(void);
		int GetWidth(void);
		int GetHeight(void);
		int GetPitch();
		BYTE *Data(){ return GetSurface(); };	//Simplified alternate interface.
		int Width(){ return GetWidth(); };
		int Height(){ return GetHeight(); };
		int Pitch(){ return GetPitch(); };
		int BPP();

		HBITMAP GetHBitmap(){ return m_hBitmap; };

	protected:

		//Added by Seumas McNally, July 1998.
		int m_pitch;
		int m_bpp;

		// Controlling window
		HWND m_hWnd;
		HDC m_sourceHdc;
		HDC m_destHdc;

		// BITMAPINFO
		typedef struct CDIB_BITMAP
		{
			BITMAPINFOHEADER bmiHead;
			RGBQUAD			 rgb[256];
		} CDIB_BITMAP;
		CDIB_BITMAP m_dibInfo;

		// Bitmap
		HBITMAP m_hBitmap;
		HBITMAP m_hOldBitmap;
		void far *m_lpCDIBBits;

		// Palette 
		typedef struct CDIBPALETTE { 
			WORD         palVersion; 
			WORD         palNumEntries; 
			PALETTEENTRY aEntries[256]; 
			} CDIBPALETTE; 	
		CDIBPALETTE m_logicalPalette;
		HPALETTE m_hPalette;
		HPALETTE m_hOldPalette;
};

bool SetClientSize(HWND hwnd, int w, int h);
	//It sucks that Windows doesn't have a function
	//to do this, so I wrote one.  -Seumas
	//Sets Client Area size to that specified.
	//_Might_ have to be called twice in a row if you've
	//got a menu that's overlapping itself and will be
	//un-overlapped by size change.

	//It also sucks that Windows doesn't have a function
	//to merely set the position of a window without having
	//to specify its size too!
bool SetWindowPosition(HWND hwnd, int x, int y);
	//Oops, I geuss it does, if you send the right flags to SetWindowPos().
	//Oh well.

#endif
