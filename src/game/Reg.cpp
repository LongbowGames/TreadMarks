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

#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <boost/lexical_cast.hpp>

#include "Reg.h"
#include "CStr.h"
#include "Directories.h"

using namespace std;
using namespace boost;

Registry& Registry::Get()
{
	static Registry Reg;
	return Reg;
}

Registry::Registry(bool global)
{
	sFile = (global ? GetCommonAppDataDir() : GetAppDataDir()) + "reg.ini";

	Read();
}

Registry::~Registry()
{
	Write();
}

void Registry::Read()
{
	ifstream fin(sFile.c_str());
	if(fin)
	{
		string sLine;
		while(getline(fin, sLine))
		{
			string::size_type iEquals = sLine.find('=');
			if(iEquals != string::npos && iEquals > 0 && iEquals < sLine.size()-1)
				aValues[sLine.substr(0, iEquals)] = sLine.substr(iEquals+1);
		}
	}
}

void Registry::Write()
{
	ofstream fout(sFile.c_str());
	if(!fout)
		throw runtime_error("Could not open registry ini for writing.");

	for(ValueMap::const_iterator iter = aValues.begin(); iter != aValues.end(); ++iter)
		fout << iter->first << "=" << iter->second << "\n";
}

void Registry::WriteString(const char *name, const char *val)
{
	aValues[name] = val;
	Write();
}

void Registry::ReadString(const char *name, char *val, int maxlen) const
{
	ValueMap::const_iterator it = aValues.find(name);
	if(it != aValues.end())
		strncpy(val, it->second.c_str(), maxlen);
}

void Registry::ReadString(const char *name, CStr *str) const
{
	ValueMap::const_iterator it = aValues.find(name);
	if(it != aValues.end())
		*str = it->second.c_str();
}
