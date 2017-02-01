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

#include "../Directories.h"
#include <shlobj.h>
#include <string>
#include <experimental/filesystem>

using namespace std;
using namespace std::experimental::filesystem;

CStr GetAppDataDir()
{
	TCHAR szPath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath);
	std::string rc(szPath);
	if(rc.size())
	{
		if(rc[rc.size()-1] != '/' && rc[rc.size()-1] != '\\')
			rc += '/';
		rc += "Longbow Digital Arts/Tread Marks/";
	}
	create_directories(rc);
	return CStr(rc.c_str());
}

CStr GetCommonAppDataDir()
{
	TCHAR szPath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath);
	std::string rc(szPath);
	if(rc.size())
	{
		if(rc[rc.size()-1] != '/' && rc[rc.size()-1] != '\\')
			rc += '/';
		rc += "Longbow Digital Arts/Tread Marks/";
	}
	create_directories(rc);
	return CStr(rc.c_str());
}
