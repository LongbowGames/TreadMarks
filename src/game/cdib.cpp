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

#include "cdib.h"

CreateDib::CreateDib(void)
{
	m_hWnd = NULL;
	m_sourceHdc = NULL;
	m_destHdc = NULL;
	memset(&m_dibInfo, 0, sizeof(CDIB_BITMAP));
	m_hBitmap = NULL;
	m_hOldBitmap = NULL;
	m_lpCDIBBits = NULL;
	memset(&m_logicalPalette, 0, sizeof(CDIBPALETTE));
	m_hPalette = NULL;
	m_hOldPalette = NULL;
	m_pitch = 0;
	m_bpp = 0;
}


CreateDib::~CreateDib(void)
{
#ifdef WIN32
	DeleteHBitmap();

	memset(&m_logicalPalette, 0, sizeof(CDIBPALETTE));
	if(m_hPalette)
	{
		DeleteObject(m_hPalette);
	}
	m_hPalette = NULL;
	m_hOldPalette = NULL;

	m_hWnd = NULL;

	if(m_sourceHdc)
	{
		DeleteDC(m_sourceHdc);
		m_sourceHdc = NULL;
	}

	if(m_destHdc)
	{
		ReleaseDC(m_hWnd, m_destHdc);
		m_destHdc = NULL;
	}
#endif
}

//True Color support added by Seumas McNally.
bool CreateDib::CreateHBitmap(HWND hWnd, int width, int height, int bpp)
{
#ifdef WIN32
	DeleteHBitmap();

	bpp &= (~7);	//Wipe out lower three bits.
	if(bpp < 8 || bpp > 32 || width <= 0 || height <= 0) return false;

	m_hWnd = hWnd;
	HDC hdc = GetDC(m_hWnd);
	m_bpp = bpp;
	m_pitch = (width * (bpp >>3) + 3) & (~3);	//Keeps it on dword boundary.

	// Fill in the BITMAPINFO structure with values
	m_dibInfo.bmiHead.biSize = sizeof(BITMAPINFOHEADER);
	m_dibInfo.bmiHead.biWidth = width;
	m_dibInfo.bmiHead.biHeight = -height;
	m_dibInfo.bmiHead.biPlanes = 1;
	m_dibInfo.bmiHead.biBitCount = bpp;
	m_dibInfo.bmiHead.biCompression = BI_RGB;
	m_dibInfo.bmiHead.biSizeImage = 0;
	m_dibInfo.bmiHead.biClrUsed = 0;
	m_dibInfo.bmiHead.biClrImportant = 0;

	m_hBitmap = CreateDIBSection(	hdc, 
								(BITMAPINFO*)&m_dibInfo, 
								DIB_PAL_COLORS, 
								&m_lpCDIBBits, 
								NULL, 
								NULL); 
	ReleaseDC(m_hWnd, hdc);
	if(m_hBitmap && m_lpCDIBBits) return true;
#endif
	return false;
}


void CreateDib::DeleteHBitmap(void)
{
#ifdef WIN32
	memset(&m_dibInfo, 0, sizeof(CDIB_BITMAP));
	if ( m_hBitmap ) 
	{
		HDC hdc = GetDC(m_hWnd);
		SelectObject( hdc, m_hOldBitmap );
		DeleteObject(m_hBitmap);
		ReleaseDC(m_hWnd, hdc);
	}
	m_hBitmap = NULL;
	m_hOldBitmap = NULL;
	m_lpCDIBBits = NULL;
	m_hWnd = NULL;
	m_pitch = 0;
	m_bpp = 0;
#endif
}


void CreateDib::SetPalette(PALETTEENTRY *palette)
{
#ifdef WIN32
#ifndef TM_DC
	if(!m_hBitmap)
		return;

    m_logicalPalette.palVersion = 0x300;
    m_logicalPalette.palNumEntries = 256;

	if (m_hPalette != NULL)
	{
		DeleteObject(m_hPalette);
		m_hPalette = NULL;
	}

	UINT i;
    for( i = 0; i < 256; i++ ) 
	{
        m_dibInfo.rgb[i].rgbRed = 
			m_logicalPalette.aEntries[i].peRed = palette[i].peRed; 
        m_dibInfo.rgb[i].rgbGreen = 
            m_logicalPalette.aEntries[i].peGreen = palette[i].peGreen; 
        m_dibInfo.rgb[i].rgbBlue = 
            m_logicalPalette.aEntries[i].peBlue = palette[i].peBlue; 
        m_dibInfo.rgb[i].rgbReserved = 0;
        m_logicalPalette.aEntries[i].peFlags = 0;
    }
    m_hPalette = CreatePalette((LOGPALETTE*)&m_logicalPalette);

	HDC sHdc;
	sHdc = CreateCompatibleDC(NULL) ;
	m_hOldBitmap = (HBITMAP)SelectObject(sHdc, m_hBitmap);
	SetDIBColorTable( sHdc, 0, 256, m_dibInfo.rgb);
	SelectObject(sHdc, m_hOldBitmap);
	DeleteDC(sHdc);
#endif
#endif
}


void CreateDib::RealizePalette(void)
{
#ifdef WIN32
	if(!m_hPalette)
		return;
	
	HDC hdc = GetDC(m_hWnd);

	HPALETTE oldPalette = SelectPalette(hdc, m_hPalette, true);	//New  (fixes bad resource leak)
	::RealizePalette(hdc);
	SelectPalette(hdc, oldPalette, true);	//New
	ReleaseDC(m_hWnd,hdc);

//	InvalidateRect(m_hWnd,NULL,true);
	//You may want to call same externally when you realize the palette, but not doing it here gives more control.  SM
#endif
}


BYTE* CreateDib::Lock(void)
{
#ifdef WIN32
 #ifndef TM_DC
	if(!m_hBitmap)
		return NULL;

	GdiFlush();

	if(!m_destHdc)
		m_destHdc = GetDC(m_hWnd);
	
	if(!m_sourceHdc)
		m_sourceHdc = CreateCompatibleDC(m_destHdc) ;

	m_hOldBitmap = (HBITMAP)SelectObject(m_sourceHdc, m_hBitmap);

	return (BYTE *)m_lpCDIBBits;
 #else
	return NULL;
 #endif
#else
	return NULL;
#endif
}


bool CreateDib::Blit(int destX, int destY, int sourceX, int sourceY, int sourceWidth, int sourceHeight)
{
#ifdef WIN32
	if(!m_hBitmap)
		return false;

	bool result;
	HPALETTE oldPalette = SelectPalette(m_destHdc, m_hPalette, false);	//New
	result = !!BitBlt(	m_destHdc, destX, destY, sourceWidth, sourceHeight,
						m_sourceHdc, sourceX, sourceY, SRCCOPY);
	SelectPalette(m_destHdc, oldPalette, true);	//New

	return result;
#else
	return false;
#endif
}
//Function added by Seumas McNally.
bool CreateDib::StretchBlit(int destX, int destY, int destWidth, int destHeight, int sourceX, int sourceY, int sourceWidth, int sourceHeight)
{
#ifdef WIN32
	if(!m_hBitmap)
		return false;

	bool result;
	HPALETTE oldPalette = SelectPalette(m_destHdc, m_hPalette, false);	//New
	result = !!StretchBlt(	m_destHdc, destX, destY, destWidth, destHeight,
							m_sourceHdc, sourceX, sourceY, sourceWidth, sourceHeight, SRCCOPY);
	SelectPalette(m_destHdc, oldPalette, true);	//New

	return result;
#else
	return false;
#endif
}
//Function added by Seumas McNally.
bool CreateDib::StretchBlitToWnd(){
#ifdef WIN32
	if(!m_hBitmap)
		return false;
	bool result;
	RECT rc;
	GetClientRect(m_hWnd, &rc);
	HPALETTE oldPalette = SelectPalette(m_destHdc, m_hPalette, false);	//New
	result = !!StretchBlt(	m_destHdc, 0, 0, rc.right, rc.bottom,
							m_sourceHdc, 0, 0, GetWidth(), GetHeight(), SRCCOPY);
	SelectPalette(m_destHdc, oldPalette, true);	//New
	return result;
#else
	return false;
#endif
}
//Function added by Seumas McNally.
bool CreateDib::ScrollDib(int dx, int dy){
#ifdef WIN32
	if(!m_hBitmap) return false;
	return !!ScrollDC(m_sourceHdc, dx, dy, NULL, NULL, NULL, NULL);
#else
	return false;
#endif
}

bool CreateDib::PaintBlit(HDC destHdc, int destX, int destY, int sourceX, int sourceY, int sourceWidth, int sourceHeight, int dW, int dH)
{
#ifdef WIN32
 #ifndef TM_DC
	if(!m_hBitmap)
		return false;

	GdiFlush();

	// Lock
	HDC sourceHdc = CreateCompatibleDC(destHdc) ;
	m_hOldBitmap = (HBITMAP)SelectObject(sourceHdc, m_hBitmap);

	// Blit
	bool result;
	HPALETTE oldPalette = SelectPalette(m_destHdc, m_hPalette, false);	//New
	if(dW == 0 && dH == 0){
		result = !!BitBlt(destHdc, destX, destY, sourceWidth, sourceHeight,
						sourceHdc, sourceX, sourceY, SRCCOPY);
	}else{
		result = !!StretchBlt(destHdc, destX, destY, dW, dH,
							sourceHdc, sourceX, sourceY, sourceWidth, sourceHeight, SRCCOPY);
	}
	SelectPalette(m_destHdc, oldPalette, true);	//New

	// Unlock
	SelectObject(sourceHdc, m_hOldBitmap);
	DeleteDC(sourceHdc);

	return result;
 #else
	return false;
 #endif
#else
	return false;
#endif
}


void CreateDib::Unlock(void)
{
#ifdef WIN32
	if(!m_hBitmap)
		return;

	SelectObject(m_sourceHdc, m_hOldBitmap);

	DeleteDC(m_sourceHdc);
	m_sourceHdc = NULL;
	ReleaseDC(m_hWnd, m_destHdc);
	m_destHdc = NULL;
#endif
}

BYTE* CreateDib::GetSurface(void){
	if(!m_hBitmap)
		return NULL;
	return (BYTE *)m_lpCDIBBits;
}
int CreateDib::GetWidth(void){
	if(!m_hBitmap)
		return 0;
	return m_dibInfo.bmiHead.biWidth;
}
int CreateDib::GetHeight(void){
	if(!m_hBitmap)
		return 0;
	return abs(m_dibInfo.bmiHead.biHeight);
}
//Function added by Seumas McNally.
int CreateDib::GetPitch(void){
	if(!m_hBitmap)
		return 0;
	return m_pitch;
}
//Function added by Seumas McNally.
int CreateDib::BPP(){
	if(!m_hBitmap)
		return 0;
	return m_bpp;
}

//Helper function added by Seumas McNally.
bool SetClientSize(HWND hwnd, int w, int h){
#ifdef WIN32
	RECT rcw, rcc;
	if(hwnd){
		GetWindowRect(hwnd, &rcw);
		GetClientRect(hwnd, &rcc);
		MoveWindow(hwnd, rcw.left, rcw.top, rcw.right - rcw.left + (w - rcc.right), rcw.bottom - rcw.top + (h - rcc.bottom), true);
		return true;
	}
#endif
	return false;
}

//Helper function added by Seumas McNally.
bool SetWindowPosition(HWND hwnd, int x, int y){
#ifdef WIN32
	RECT rcw;
	if(hwnd){
		GetWindowRect(hwnd, &rcw);
		MoveWindow(hwnd, x, y, rcw.right - rcw.left, rcw.bottom - rcw.top, true);
		return true;
	}
#endif
	return false;
}
