//===========================================================================
//	Cleanup Tabs
//	Copyright (C) 1997 Mahood and Company.
//	All rights reserved. 
//
//This program is open source software; you can redistribute it and / or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; version 2
//of the License.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//GNU General Public License for more details.
//
//http://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
//
//ADDENDUM: Use of modified versions of this software in production
//constitutes redistribution and the modified source code must remain open source.
//Modifications must be made by forking the Github.com repository for this software
//then modifying the forked copy.
//===========================================================================
#pragma once

#include "General.h"
 

//CLEANUP_TABS_LIST-----------------------------------------------------------------------
class CLEANUP_TABS
{
	public: 
	char	Path[MAX_PATH];

	void	Initialize	(void);

	void	DoCleanup		(HWND	hWndFrame);
};

typedef CLEANUP_TABS *pCLEANUP_TABS;