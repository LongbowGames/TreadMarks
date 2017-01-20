
//Uninstall string is in:

//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Tread Marks\UninstallString

//Run game path should be placed in:

//HKEY_LOCAL_MACHINE\SOFTWARE\Longbow Digital Arts\Tread Marks\LaunchPath

//
//Tread Marks AutoRun CD Launcher.
//

#include <windows.h>
#include <direct.h>
#include "CStr.h"
#include "resource.h"
#include "Reg.h"
#include <cstdio>

#define WM_REINITDIALOG (WM_USER + 100)

BOOL CALLBACK DlgProc(HWND dlgwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

CStr CWD;

Registry UninstReg("Microsoft\\Windows\\CurrentVersion\\Uninstall", "Tread Marks 1.5", TRUE);	//Use HKEY_LOCAL_MACHINE.
Registry LaunchReg("Longbow Digital Arts", "Tread Marks 1.5", TRUE);

HANDLE tmsem = 0;
const char tmsemname[] = "Tread Marks Semaphore";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow){
	//
	//Prevent multiple instances.
	tmsem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, tmsemname);
	if(tmsem){	//Oops, already running.
	//	MessageBox(NULL, "Tread Marks is already running!", "Error!", MB_OK);
		CloseHandle(tmsem);
		return 0;	//Quietly abort auto-run if TM is running.
	}
	//
	char buf[1024];
	_getcwd(buf, 1024);
	CWD = buf;
	if(Right(CWD, 1) != "\\") CWD = CWD + "\\";
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOGAUTORUN), NULL, (DLGPROC)DlgProc);
//	return ConfigDialog();
}//WinMain

//CStr SecondCfg = "";

CStr LaunchPath;
CStr UninstallString, UninstallCmdline;

BOOL CALLBACK DlgProc(HWND dlgwnd, UINT iMsg, WPARAM wParam, LPARAM lParam){
	HWND ctrl;
//	RECT rc;
//	POINT pt;
	int id, code;//, i;//, error;
	CStr str;
	FILE *f;
	switch(iMsg){
		case WM_ACTIVATE :
			if(LOWORD(wParam) == WA_INACTIVE) break;
			OutputDebugString("Window Activated.\n");
			//Fall through if we are being activated.
			//
		case WM_INITDIALOG :
		case WM_REINITDIALOG :
	//		//Check "Use Current" by default.
	//		SendDlgItemMessage(dlgwnd, IDC_RADIOCURRENT, BM_SETCHECK, TRUE, NULL);
			str = "";
			if(UninstReg.ReadString("UninstallString", &str) && str.len() > 4){
				//Button is ok.
				UninstallString = Lower(str);
				if(Left(UninstallString, 1) == "\"" && Right(UninstallString, 1) == "\""){
					UninstallString = Mid(UninstallString, 1, UninstallString.len() - 2);
					//Chop off quote chars.
				}
				int i = Instr(UninstallString, ".exe");
				if(i > 0){	//Break into command and command line.
					UninstallCmdline = Mid(UninstallString, i + 5);
					UninstallString = Left(UninstallString, i + 4);
					OutputDebugString("Uninstall Command is: \"" + UninstallString + "\".\n");
					OutputDebugString("Uninstall Command Line is: \"" + UninstallCmdline + "\".\n");
				}
				EnableWindow(GetDlgItem(dlgwnd, IDC_BUTTONUNINSTALL), TRUE);
			}else{
				//Button should be disabled.
				UninstallString = "";
				EnableWindow(GetDlgItem(dlgwnd, IDC_BUTTONUNINSTALL), FALSE);
			}
			str = "";
			if(LaunchReg.ReadString("LaunchPath", &str) && str.len() > 4
				&& (f = fopen(str, "rb"))){
				//Play button is OK.
				fclose(f);
				LaunchPath = str;
				EnableWindow(GetDlgItem(dlgwnd, IDC_BUTTONPLAY), TRUE);
			}else{
				//Play button is bad.
				LaunchPath = "";
				EnableWindow(GetDlgItem(dlgwnd, IDC_BUTTONPLAY), FALSE);
			}
			return TRUE;
		//General control handling.
		case WM_COMMAND :
			ctrl = (HWND) lParam;
			id = LOWORD(wParam);
			code = HIWORD(wParam);
			if(code == BN_CLICKED){
				switch(id){	//Clicked button responses.
					case IDC_BUTTONINSTALL :
						if(32 < (int)ShellExecute(NULL, "open", CWD + "setup.exe", NULL, CWD, SW_SHOWNORMAL)){
						//	EndDialog(dlgwnd, TRUE);
						//	PostMessage(dlgwnd, WM_REINITDIALOG, 0, 0);
							//Update buttons.
						}else{
							MessageBox(dlgwnd, "Error launching Install!  Try running setup.exe.", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONDIRECTX :
						if(32 < (int)ShellExecute(NULL, "open", CWD + "directx7\\dxsetup.exe", NULL, CWD + "directx7\\", SW_SHOWNORMAL)){
						//	EndDialog(dlgwnd, TRUE);
						}else{
							MessageBox(dlgwnd, "Error launching Install!  Try running directx7\\dxsetup.exe.", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONGLSETUP :
						if(32 < (int)ShellExecute(NULL, "open", CWD + "glsetup.110.exe", NULL, CWD, SW_SHOWNORMAL)){
						//	EndDialog(dlgwnd, TRUE);
						}else{
							MessageBox(dlgwnd, "Error launching Install!  Try running glsetup.exe.", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONPLAY :
						if(32 < (int)ShellExecute(NULL, "open", LaunchPath, NULL, FilePathOnly(LaunchPath), SW_SHOWNORMAL)){
							EndDialog(dlgwnd, TRUE);
						}else{
							MessageBox(dlgwnd, "Error launching program!  Try Re-Installing.", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONGOODIES :
						if(32 < (int)ShellExecute(NULL, "open", "goodies", NULL, CWD, SW_SHOWNORMAL)){
						//	EndDialog(dlgwnd, TRUE);
						}else{
							MessageBox(dlgwnd, "Error opening dir!  Try looking in the Goodies dir on the CD.", "Launcher Error", MB_OK);
						}
						break;
					case IDC_BUTTONUNINSTALL :
						if(32 < (int)ShellExecute(NULL, "open", UninstallString, UninstallCmdline, NULL, SW_SHOWNORMAL)){
					//	if(-1 != (int)system(UninstallString)){
						//	PostMessage(dlgwnd, WM_REINITDIALOG, 0, 0);
							//Update buttons.
						}else{
							MessageBox(dlgwnd, "Error launching Uninstall!", "Launcher Error", MB_OK);
						}
						break;
				}
			}else{
		//		switch(id){	//Non-clicked button responses.
		//		}
			}
			return TRUE;
		case WM_CLOSE :
			EndDialog(dlgwnd, FALSE);
			return TRUE;
	}
	return FALSE;
}

