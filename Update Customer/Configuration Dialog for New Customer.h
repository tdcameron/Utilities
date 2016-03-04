//---------------------------------------------------------
//	Configuration Dialog for New Customer
//	Copyright (C) 1998 Mahood and Company.
//  All rights reserved. 
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
//---------------------------------------------------------
#pragma once

#include "General.h"
#include "Application.h"
#include "Dialog.h"
#include "Configuration Dialog.h"

//CONFIGURATION_FILE_NAME	defined in General.h

class CONFIGURATION_DIALOG_FOR_NEW_CUSTOMER: public CONFIGURATION_DIALOG
{
	public:
	virtual bool	DoDialogProcedure	(UINT		wMsg,
																	WPARAM	wParam,
																	LPARAM	lParam);

	virtual bool	ShowDialog		(void);

	virtual void ReadRecord					(void);
	virtual void WriteRecord				(void);

	private: 
	virtual void CheckFields				(void);
	virtual void FillInDialog				(void);
};

typedef CONFIGURATION_DIALOG *pCONFIGURATION_DIALOG;