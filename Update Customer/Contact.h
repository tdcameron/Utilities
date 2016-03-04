//---------------------------------------------------
//	Contacts Object
//	Copyright (C) 2013 Mahood and Company.
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
#include "Inventory.h"
 
#define MAX_CONTACT			300
#define MAX_FIELD_SIZE	200

//CONTACT_ITEM------------------------------------------------------------------
typedef struct CONTACT_ITEM
{
	char				FirstName[60];
	char				LastName[60];
	int					SpeedDial;
	char				BusinessPhone[60];
	char				HomePhone[60];
	char				MobilePhone[60];

	SQL_RECORD_CONTROL SQLRecordControl;
} CONTACT_ITEM;

typedef CONTACT_ITEM *pCONTACT_ITEM;

//CONTACT_LIST------------------------------------------------------------------
class CONTACT_LIST: public DISPLAY_WINDOW
{
	public: 
	CONTACT_ITEM	Contact[MAX_CONTACT + 1];
	int						ContactMax;

	CONTACT_ITEM	ContactItem;

	DISK_FILE			DiskFile;

	LINE_FIELDS	Heading;
	LINE_FIELDS	Data;

	int		FirstNameField;
	int		LastNameField;
	int		SpeedDialField;
	int		BusinessPhoneField;
	int		HomePhoneField;
	int		MobilePhoneField;

	int		ReadCSVFileSelection;
	int		WriteDirectoryFileSelection;

	virtual void Initialize					(void);

	virtual void DoCreateWindow			(void);
	virtual	void DefineFields				(void);

					void ParseLine					(pLINE_FIELDS	LineFields,
																	PCHAR					FileLine);
					int	 GetIndex						(PCHAR				HeadingString);
					int  GetContactIndex		(PCHAR				FirstName,
																	 PCHAR				LastName);

	private: 
	virtual	bool StartQuery					(PCHAR	Format, ...);
	virtual bool SQLReadRecords			(void);
	virtual void SQLWriteRecords		(void);
	virtual bool CheckField					(void);
	virtual void RightButtonDown		(void);
	virtual int	 PopupMenuCommand		(int		PopupMenuIndex);
					void WriteItem					(int		Index,
																	PCHAR		PhoneNumber,
																	PCHAR		NameModifier);
					void WriteTSVItem				(int		Index,
																	PCHAR		PhoneNumber,
																	PCHAR		NameModifier);
};

typedef CONTACT_LIST  *pCONTACT_LIST;