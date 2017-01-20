//Windows Registry wrapper class, by Seumas McNally, 1998.

#include <stdlib.h>

#include "Reg.h"
#include "CStr.h"

Registry::Registry(const char *company, const char *title, int global){
	strcpy(sKey, "Software");
	if(company){
		strcat(sKey, "\\");
		strcat(sKey, company);
	}
	if(title){
		strcat(sKey, "\\");
		strcat(sKey, title);
	}
	hKey = NULL;
	sPrefix[0] = '\0';
	//
	LocalMachine = global;
}
Registry::~Registry(){
}
int Registry::OpenKey(){
	DWORD t;
	CloseKey();
#ifdef WIN32
	if(ERROR_SUCCESS == RegCreateKeyEx((LocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER), sKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &t)){
		return TRUE;
	}
	hKey = NULL;
	return FALSE;
#else
	return TRUE;
#endif
}
int Registry::CloseKey(){
#ifdef WIN32
	if(hKey && ERROR_SUCCESS == RegCloseKey(hKey)){
		hKey = NULL;
		return TRUE;
	}
	return FALSE;
#else
	return TRUE;
#endif
}
int Registry::SetPrefix(const char *prefix){
	if(prefix && strlen(prefix) < 1000){
		strcpy(sPrefix, prefix);
		return 1;
	}else{
		sPrefix[0] = '\0';
		return 0;
	}
}
int Registry::WriteDword(const char *name, unsigned long val){
#ifdef WIN32
	if(name && OpenKey()){
		if(ERROR_SUCCESS == RegSetValueEx(hKey, CStr(sPrefix) + name, NULL, REG_DWORD, (BYTE*)&val, sizeof(val))){
			return CloseKey();
		}
	}
	CloseKey();
#endif
	return FALSE;
}
int Registry::ReadDword(const char *name, unsigned long *val){
#ifdef WIN32
	DWORD tsize = sizeof(*val), ttype = REG_DWORD, temp;
	if(name && val && OpenKey()){
		if(ERROR_SUCCESS == RegQueryValueEx(hKey, CStr(sPrefix) + name, NULL, &ttype, (BYTE*)&temp, &tsize)){
			*val = temp;
			return CloseKey();
		}
	}
#endif
	CloseKey();
	return FALSE;
}
int Registry::WriteFloat(const char *name, float val){
	return WriteDword(name, *((unsigned long*)&val));
}
int Registry::ReadFloat(const char *name, float *val){
	unsigned long t;
	if(ReadDword(name, &t)){
		*val = *((float*)&t);
		return 1;
	}
	return 0;
}

int Registry::WriteString(const char *name, const char *val){
#ifdef WIN32
	if(name && val && OpenKey()){
		if(ERROR_SUCCESS == RegSetValueEx(hKey, CStr(sPrefix) + name, NULL, REG_SZ, (BYTE*)val, strlen(val) + 1)){
			return CloseKey();
		}
	}
	CloseKey();
#endif
	return FALSE;
}
int Registry::ReadString(const char *name, char *val, int maxlen){
#ifdef WIN32
	DWORD ttype = REG_SZ;
	if(name && val && maxlen > 0 && OpenKey()){
		if(ERROR_SUCCESS == RegQueryValueEx(hKey, CStr(sPrefix) + name, NULL, &ttype, (BYTE*)val, (DWORD*)&maxlen)){
			val[maxlen - 1] = '\0';	//Add safety null.
			return CloseKey();
		}
	}
#endif
	CloseKey();
	return FALSE;
}
int Registry::ReadString(const char *name, CStr *str){
	char b[1024];
#ifdef WIN32
	if(str && ReadString(name, b, 1024)){
		*str = b;
		return TRUE;
	}
#endif
	return FALSE;
}

int Registry::SaveWindowPos(HWND hwnd, const char *name, int Size, int Max){
	char buf[256];
#ifdef WIN32
	if(hwnd && name && OpenKey()){
		RECT rc;
		GetWindowRect(hwnd, &rc);
		strcpy(buf, name); strcat(buf, "X"); WriteDword(buf, rc.left);
		strcpy(buf, name); strcat(buf, "Y"); WriteDword(buf, rc.top);
		if(Size){
			strcpy(buf, name); strcat(buf, "W"); WriteDword(buf, abs(rc.right - rc.left));
			strcpy(buf, name); strcat(buf, "H"); WriteDword(buf, abs(rc.bottom - rc.top));
		}
		if(Max){	//Save Maximised state.
			strcpy(buf, name); strcat(buf, "S"); WriteDword(buf, IsZoomed(hwnd) ? 1 : 0);
		}
		CloseKey();
		return TRUE;
	}
#endif
	return FALSE;
}
int Registry::RestoreWindowPos(HWND hwnd, const char *name, int Size, int Max){
	char buf[256];
#ifdef WIN32
	if(hwnd && name && OpenKey()){
		RECT rc, wrc;
		int PosOK = FALSE, SizeOK = FALSE, StateOK = FALSE, state;
		strcpy(buf, name); strcat(buf, "X");
		if(ReadDword(buf, (DWORD*)&rc.left)){
			strcpy(buf, name); strcat(buf, "Y");
			if(ReadDword(buf, (DWORD*)&rc.top)){
				PosOK = TRUE;
			}
		}
		if(Size){
			strcpy(buf, name); strcat(buf, "W");
			if(ReadDword(buf, (DWORD*)&rc.right)){
				strcpy(buf, name); strcat(buf, "H");
				if(ReadDword(buf, (DWORD*)&rc.bottom)){
					SizeOK = TRUE;
				}
			}
		}
		if(Max){
			strcpy(buf, name); strcat(buf, "S");
			if(ReadDword(buf, (DWORD*)&state)) StateOK = TRUE;
		}
		if(PosOK){
			if(SizeOK == FALSE){	//Don't want size read anyway, or size not present.
				GetWindowRect(hwnd, &wrc);
				rc.right = wrc.right - wrc.left;	//Compute current width and height of window and throw those into rc for MoveWindow call.
				rc.bottom = wrc.bottom - wrc.top;
			}
			MoveWindow(hwnd, __min(__max(-8, rc.left), GetSystemMetrics(SM_CXSCREEN) - rc.right / 2),
				__min(__max(-8, rc.top), GetSystemMetrics(SM_CYSCREEN) - rc.bottom / 2),
				rc.right, rc.bottom, TRUE);
		}
		if(StateOK){
			if(state == 0) ShowWindow(hwnd, SW_SHOWNORMAL);
			if(state == 1) ShowWindow(hwnd, SW_MAXIMIZE);
		}else{	//If we want state restored, but there is none, do show normal.
			if(Max) ShowWindow(hwnd, SW_SHOWNORMAL);
		}
		CloseKey();
		if(PosOK || SizeOK || StateOK){	//Only return true if data found in reg.
			return TRUE;
		}
	}
#endif
	return FALSE;
}