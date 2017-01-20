//Windows Registry wrapper class, by Seumas McNally, 1998.
//
//Usage:  Construct one object with Company Name and Product Name as parameters.
//Call OpenKey, write or read data, and CloseKey.  Or use Save and RestoreWindowPos.
//Note, they will open and close the key internally!
//Save/RestoreWindowPos will append the letters X, Y, W, and H onto the name given
//when writing the four size/position values.
//
//Actually, I modified it so the data reading/writing functions open and close the
//key internally themselves.  Call me paranoid, and a little inefficient.  But it
//seems to be fast enough, and Windows should cache registry accesses pretty damn
//heavily.

#ifndef REG_H
#define REG_H

#ifdef WIN32
#define VC_EXTRALEAN
#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include "Compatible Types.h"
#endif

#include "CStr.h"

class Registry{
private:
	char sKey[1024];
	char sPrefix[1024];
	HKEY hKey;
	int LocalMachine;
public:
	Registry(const char *company, const char *title, int global = FALSE);	//Set global to TRUE to use HKEY_LOCAL_MACHINE instead of HKEY_CURRENT_USER.
	~Registry();
	int OpenKey();
	int CloseKey();
	int SetPrefix(const char *prefix);	//Sets a prefix which is prepended to all keys read or written, so you can easily create fake-subdirectories of keys.
	int WriteDword(const char *name, unsigned long val);
	int ReadDword(const char *name, unsigned long *val);
	int ReadDword(const char *name, int *val){ return ReadDword(name, (unsigned long*)val); };
	int WriteFloat(const char *name, float val);
	int ReadFloat(const char *name, float *val);
	int WriteString(const char *name, const char *val);
	int ReadString(const char *name, char *val, int maxlen);
	int ReadString(const char *name, CStr *str);
	int SaveWindowPos(HWND hwnd, const char *name, int Size = FALSE, int Max = FALSE);	//Restore and save Widht and Height too?  (Not for Dialogs!!!)
	int RestoreWindowPos(HWND hwnd, const char *name, int Size = FALSE, int Max = FALSE);	//Set MAx to TRUE to save/restore window's maximised state.
};

#endif