//---------------------------------------------------
//	Last Modified Object
//	Copyright (C) 2014 Mahood and Company.
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
//---------------------------------------------------
#pragma once

#include "Display.h"
 
#define MAX_LAST_MODIFIED 300

//LAST_MODIFIED------------------------------------------------------------------
typedef struct LAST_MODIFIED
{
	char				TableName[MAX_PATH + 1];
	DOUBLE_TIME	DateTime;

	SQL_RECORD_CONTROL SQLRecordControl;
} LAST_MODIFIED;

typedef LAST_MODIFIED *pLAST_MODIFIED;

//LAST_MODIFIED_LIST------------------------------------------------------------------
class LAST_MODIFIED_LIST: public DISPLAY_WINDOW
{
	public: 
	LAST_MODIFIED	LastModified[MAX_LAST_MODIFIED + 1];
	int						LastModifiedMax;

	LAST_MODIFIED	LastModifiedItem;

	int		TableNameField;
	int		DateTimeField;

	int		RefreshSelection;

	virtual void Initialize					(void);

	virtual void DoCreateWindow			(void);
	virtual	void DefineFields				(void);
					int	 GetIndex						(PCHAR				TableName);


	private: 
	virtual	bool StartQuery					(PCHAR	Format, ...);
	virtual bool SQLReadRecords			(void);
	virtual void SQLWriteRecords		(void);
	virtual bool CheckField					(void);
	virtual void RightButtonDown		(void);
	virtual int	 PopupMenuCommand		(int		PopupMenuIndex);
};

typedef LAST_MODIFIED_LIST  *pLAST_MODIFIED_LIST;