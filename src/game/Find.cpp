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

//File finder wrapper for Windows 32 by Seumas McNally.

#include "Find.h"
#include "TextLine.hpp"

#include <cstring>

#include <boost/filesystem.hpp>
#include <regex>
using namespace boost::filesystem;
using namespace std;

Find::Find(){
	Clear();
}
Find::Find(const char *name){
	Clear();
	Search(name);
}
Find::~Find(){
	Clear();
}

bool Find::Clear(){
	nItems = 0;
	ListHead.DeleteList();
	return true;
}
int Find::Search(const char *name, bool recursive, const char *addpath){
	Clear();
	return AddSearch(name, recursive, addpath);
}
int Find::AddSearch(const char *name, bool recursive, const char *addpath){
	int found = 0, db;

	if(name)
	{
		//First find and search subdirectories.
		for(db = strlen(name) - 1; db > 0 && name[db] != '/' && name[db] != '\\' && name[db] != ':'; db--);
		std::string left(Left(name, db + 1)), right(Mid(name, db + 1));	//Break up path and wildcard.
		if(left == "") left = "./";

		if(is_directory(left))
		{
			regex re("[\\\\[\\]()^$|.+]"); // Regular expression to match all special regex chars except * and ?
			std::string sRightEscaped = regex_replace(right, re, std::string("\\$&"));
			re = "[*?]";
			re = regex_replace(sRightEscaped, re, std::string(".$&"));

			for(directory_iterator iter(left); iter != directory_iterator(); ++iter)
			{
				if(is_directory(iter->status()))
				{
					if(recursive)
						found += AddSearch((left + iter->path().filename().string() + "/" + right).c_str(), recursive, (std::string(addpath) + iter->path().filename().string() + "/").c_str());
				}
				else if(is_regular_file(iter->status()))
				{
					if(std::regex_match(iter->path().filename().string(), re))
					{
						if(ListHead.AddItem(new CStr(CStr(addpath) + iter->path().filename().string().c_str())))
						{
							found++;
							nItems++;
						}
					}
				}
			}
		}
	}

	return found;
}
const char *Find::operator[](int n){
	CStrList *lp = ListHead.NextLink();
	if(lp && n >= 0 && n < nItems){
		while(n > 0 && lp){
			lp = lp->NextLink();
			n--;
		}
		if(lp) return lp->Data->get();
	}
	return NULL;
}
