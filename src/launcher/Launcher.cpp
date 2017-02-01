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

//
//Tread Marks Game Launcher.
//

#include <windows.h>
#include <direct.h>
#include "../game/CStr.h"
#include "resource.h"
#include "../game/Reg.h"
#include "../game/Directories.h"
#include <stdio.h>

BOOL CALLBACK DlgProc(HWND dlgwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

//int ConfigDialog(){
//	return DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc);
//}

CStr CWD;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow){
	char buf[1024];
	_getcwd(buf, 1024);
	CWD = buf;
	if(Right(CWD, 1) != "\\") CWD = CWD + "\\";
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc);
//	return ConfigDialog();
}//WinMain

CStr SecondCfg = "";

// Find the best dedicated.cfg
CStr FindDedicated()
{
	CStr sConfigFile = GetAppDataDir();
	sConfigFile.cat("dedicated.cfg");
	FILE *fp;
	if(fp = fopen(sConfigFile.get(), "r"))	// Read from this user's data directory
	{
		fclose(fp);
		return sConfigFile;
	}
	else
	{
		sConfigFile = GetCommonAppDataDir();	// Read from this machine's data directory
		sConfigFile.cat("dedicated.cfg");
		if(fp = fopen(sConfigFile.get(), "r"))
		{
			fclose(fp);
			return sConfigFile;
		}
		else
		{
			sConfigFile = CWD + "dedicated.cfg";
			if(fp = fopen(sConfigFile.get(), "r"))
			{
				fclose(fp);
				return sConfigFile;
			}
			else
			{
				sConfigFile = GetAppDataDir();
				sConfigFile.cat("dedicated.cfg");
				return sConfigFile;
			}
		}
	}
}

BOOL CALLBACK DlgProc(HWND dlgwnd, UINT iMsg, WPARAM wParam, LPARAM lParam){
	HWND ctrl;
//	RECT rc;
//	POINT pt;
	int id, code;
	switch(iMsg){
		case WM_INITDIALOG :
			//Check "Use Average" by default.
			SendDlgItemMessage(dlgwnd, IDC_RADIOAVG, BM_SETCHECK, TRUE, NULL);
			return TRUE;
		//General control handling.
		case WM_COMMAND :
			ctrl = (HWND) lParam;
			id = LOWORD(wParam);
			code = HIWORD(wParam);
			if(code == BN_CLICKED){
				switch(id){	//Clicked button responses.
				//	case IDOK :
				//		MapFile = Map[MapIdx];
				//		TankType = TankNames[TankIdx];
				//		EndDialog(dlgwnd, TRUE);
				//		break;
				//	case IDCANCEL :
				//		EndDialog(dlgwnd, FALSE);
				//		break;
				//	case IDC_RADIOCURRENT :
				//		SecondCfg = "";
				//		break;
					case IDC_RADIOHIGH :
						SecondCfg = " godlypc.cfg";//high.cfg";
						break;
					case IDC_RADIOAVG :
						SecondCfg = "";// average.cfg";
						break;
					case IDC_BUTTONHW :
						if(32 < (int)ShellExecute(NULL, "open", "tm.exe", "config.cfg" + SecondCfg, CWD, SW_SHOWNORMAL)){
							EndDialog(dlgwnd, TRUE);
						}else{
							MessageBox(dlgwnd, "Error launching program!", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONREADME :
						if(32 < (int)ShellExecute(NULL, "open", CWD + "docs\\readme.html", NULL, CWD, SW_SHOWNORMAL)){
							//Nothing.
						}else{
							MessageBox(dlgwnd, "Error launching program!", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONWEB :
						if(32 < (int)ShellExecute(NULL, "open", "http://www.treadmarks.com/index.html", NULL, NULL, NULL)){
							//EndDialog(dlgwnd, TRUE);
						}else{
							MessageBox(dlgwnd, "Error launching program!", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONDEDI :
					{
						STARTUPINFO si = {0};
						si.cb = sizeof(STARTUPINFO);
						PROCESS_INFORMATION pi;
						char sDedicated[MAX_PATH] = "\0";
						strncpy(sDedicated, "TM.exe \"" + FindDedicated() + "\"", MAX_PATH-1);

						if(CreateProcess(NULL, sDedicated, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)){
							CloseHandle(pi.hThread);
							CloseHandle(pi.hProcess);
							EndDialog(dlgwnd, TRUE);
						}else{
							MessageBox(dlgwnd, "Error launching program!", "Launcher Error", MB_OK);
						}
						break;
					}
					case IDC_BUTTONDEDICFG :
						if(32 < (int)ShellExecute(NULL, "open", "notepad.exe", FindDedicated(), CWD, SW_SHOWNORMAL)){
							//Nothing.
						}else{
							MessageBox(dlgwnd, "Error launching program!", "Launcher Error", MB_OK);
						}
						break;
				}
			}else{
//				switch(id){	//Non-clicked button responses.
//				default:
//					break;
//				}
			}
			return TRUE;
		case WM_CLOSE :
			EndDialog(dlgwnd, FALSE);
			return TRUE;
	}
	return FALSE;
}
