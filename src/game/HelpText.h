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


#ifndef HELPTEXT_H
#define HELPTEXT_H

#include "CStr.h"

enum HelpID{
	HELP_Main = 0,
	HELP_Story,
	HELP_Credits,
//	HELP_Credits2,
	HELP_InGameMain,
	HELP_Options,
	HELP_Options2,
	HELP_Options3,
	HELP_Controls,
	HELP_Misc,
	HELP_Tank,
	HELP_Map,
	HELP_Server,
	HELP_Server2,
	HELP_Chat,
	HELP_Roster,
	HELP_Roster2,
	HELP_Ladder,
	HELP_Ladder2,
	HELP_Stats,
	HELP_SharewareLadder,
	HELP_OrderingInfo,
	HELP_OrderingInfo2,
	HELP_WrongVersion
};

extern const int NumHelpText;

#endif


