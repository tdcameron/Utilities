//---------------------------------------------------
// Last Modified Object
// Copyright (C) 2014 Mahood and Company.
// All rights reserved. 
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

#include "Last Modified.h"

//-------------------------------------------------------------------------------------------------------- 
void LAST_MODIFIED_LIST::Initialize (void)

{
	DoRegisterClass ("Last Modified", "Last Modified");
}

//---------------------------------------------------------------------------------------
void LAST_MODIFIED_LIST::DoCreateWindow (void)
{
  InitDisplay ();

	NoEdit = true;
	NoKeys = true;

	DefineFields ();
  
	ReadAndVerifyRecords ();

	DISPLAY_WINDOW::DoCreateWindow ();
}
 
//-----------------------------------------------------------------------------------------
void LAST_MODIFIED_LIST::DefineFields (void)
{
	EnableSQLMode (SQL_No_edit);

	//Table created in SQL Table

	DefineData (1, //Screen Version
							&LastModified,
							&LastModifiedMax,
							sizeof (LastModified),
							sizeof (LastModified[0]),
							&LastModified[0].SQLRecordControl,
							"Last Modified");

	TableNameField = AddStringField ("Table", "Name",
		&LastModified[0].TableName, sizeof (LastModified[0].TableName), Either_case);

	DateTimeField = AddDoubleTimeField ("Date", "Time",
		&LastModified[0].DateTime, 3);

	FieldsDefined ();
}

//-----------------------------------------------------------------------------------------
bool LAST_MODIFIED_LIST::StartQuery (PCHAR	Format, ...)
{
	va_list	Args; 	
	char		Where[MAX_STRING];

	//Get format and format variable arguments into Message
	va_start (Args, Format);
	vQPsprintf_s (Where, sizeof (Where), Format, Args);
	va_end (Args);

	DataTable.StartQuery ();

	QPsprintf_s (DataTable.Statement, 
		"Select [Table Name], [Date Time] From [Last Modified]"
		"%s", Where);

	if (!DataTable.ExecuteStatement ()) return false;

	DataTable.BindColumn (SQL_CHAR,									&LastModifiedItem.TableName,		sizeof (LastModifiedItem.TableName));
	DataTable.BindColumn (SQL_DATETIMEOFFSETdouble,	&LastModifiedItem.DateTime,			sizeof (LastModifiedItem.DateTime));

	ZeroMemory (&LastModifiedItem, sizeof (LastModifiedItem));

	return true;
}

//-----------------------------------------------------------------------------------------
bool LAST_MODIFIED_LIST::SQLReadRecords (void)
{
	LastModifiedMax = -1;

	ZeroMemory (&LastModified, sizeof (LastModified));

	if (!StartQuery ("")) return false;

	while (DataTable.ReadRecord ())
	{
		if (LastModifiedMax == MAX_LAST_MODIFIED)
		{
			DataTable.AbortQuery = true;

			ReportError ("Last Modified", "Too many Last Modified items!");
		}
		else
		{
			LastModifiedMax++;
			LastModified[LastModifiedMax] = LastModifiedItem;
		}

		ZeroMemory (&LastModifiedItem, sizeof (LastModifiedItem));
	}

	return true;
}

//-----------------------------------------------------------------------------------------
void LAST_MODIFIED_LIST::SQLWriteRecords (void)
{
	int		I;

	for (I = 0; I <= LastModifiedMax; I++)
	{
		if (LastModified[I].SQLRecordControl.Delete)
		{
			DataTable.DoDelete("Where [Table Name] = '%s'", LastModified[I].TableName);
		}

		if (LastModified[I].SQLRecordControl.New)
		{
			ReportError ("Last Modified", "New row not supported!");
		}

		if (LastModified[I].SQLRecordControl.Modified)
		{
			DataTable.StartUpdate ();
			DataTable.UpdateDoubleTime	("Date Time",					LastModified[I].DateTime);
			DataTable.DoUpdate ("where [Table Name] = '%s'",	LastModified[I].TableName);
		}
	}
}

//--------------------------------------------------------------------------------------
bool LAST_MODIFIED_LIST::CheckField (void)
{
	return false;
}

//-------------------------------------------------------------------------------------------------------- 
void LAST_MODIFIED_LIST::RightButtonDown (void)
{
	RefreshSelection = DefinePopupMenu	(NULL, 0, "Refresh");
}

//--------------------------------------------------------------------------------------------- 
int LAST_MODIFIED_LIST::PopupMenuCommand (int		PopupMenuIndex)
{	
	if (PopupMenuIndex == RefreshSelection)
	{
		ReadAndVerifyRecords ();
		return 0;
	}
	else return DISPLAY_WINDOW::PopupMenuCommand (PopupMenuIndex);
}

//--------------------------------------------------------------------------------------
int LAST_MODIFIED_LIST::GetIndex (PCHAR	TheTableName)
{
	int		Index;
	bool	Found;

	Index = 0;
	Found = false;
		
	while (!Found)
	{
		if (StringEqual (TheTableName, LastModified[Index].TableName))
					Found = true;
		else	Index++;
	}

	if (Found)
				return Index;
	else	return -1;
}