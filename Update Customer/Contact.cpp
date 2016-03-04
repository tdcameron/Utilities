//---------------------------------------------------
// Contact Object
// Copyright (C) 2013 Mahood and Company.
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

#include "Contact.h"

//-------------------------------------------------------------------------------------------------------- 
void CONTACT_LIST::Initialize (void)

{
	DoRegisterClass ("Contact", "Contact");

	UseCustomerHelp = true;
}

//---------------------------------------------------------------------------------------
void CONTACT_LIST::DoCreateWindow (void)
{
	if (!StringEqual (CustomerName, "Antoinettes"))
	{
		ReportError ("Inventory", "Only open this from the Antoinettes customer!");
	}

  InitDisplay ();

	DefineFields ();
  
	ReadAndVerifyRecords ();

	DISPLAY_WINDOW::DoCreateWindow ();
}
 
//-----------------------------------------------------------------------------------------
void CONTACT_LIST::DefineFields (void)
{
	EnableSQLMode (SQL_No_edit);

	strcpy_s (DataTable.CreateTableStatement, "Create Table [Contact] ("
		"[First Name]			VARCHAR (80), \n"
		"[Last Name]			VARCHAR (80), \n"
		"[Speed Dial]			INTEGER, \n"
		"[Business Phone]	VARCHAR (80), \n"
		"[Home Phone]			VARCHAR (80), \n"
		"[Mobile Phone]		VARCHAR (80))");

	//strcpy_s (DataTable.PrimaryKeyStatement, "Alter Table [Contact] add Primary Key ([??])");

	DefineData (1, //Screen Version
							&Contact,
							&ContactMax,
							sizeof (Contact),
							sizeof (Contact[0]),
							&Contact[0].SQLRecordControl,
							"Contact");

	FirstNameField = AddStringField ("First", "Name",
		&Contact[0].FirstName, sizeof (Contact[0].FirstName), Either_case);

	LastNameField = AddStringField ("Last", "Name",
		&Contact[0].LastName, sizeof (Contact[0].LastName), Either_case);

	//This is best left to the phone user
	//SpeedDialField = AddStringField ("Speed", "Dial",
	//	&Contact[0].SpeedDial, sizeof (Contact[0].SpeedDial), Either_case);

	BusinessPhoneField = AddStringField ("Business", "Phone",
		&Contact[0].BusinessPhone, sizeof (Contact[0].BusinessPhone), Either_case);

	HomePhoneField = AddStringField ("Home", "Phone",
		&Contact[0].HomePhone, sizeof (Contact[0].HomePhone), Either_case);

	HomePhoneField = AddStringField ("Mobile", "Phone",
		&Contact[0].MobilePhone, sizeof (Contact[0].MobilePhone), Either_case);

	FieldsDefined ();
}

//-----------------------------------------------------------------------------------------
bool CONTACT_LIST::StartQuery (PCHAR	Format, ...)
{
	va_list	Args; 	
	char		Where[MAX_STRING];

	//Get format and format variable arguments into Message
	va_start (Args, Format);
	vQPsprintf_s (Where, sizeof (Where), Format, Args);
	va_end (Args);

	DataTable.StartQuery ();

	QPsprintf_s (DataTable.Statement, 
		"Select [First Name], [Last Name], [Speed Dial], [Business Phone], [Home Phone], [Mobile Phone] From [Contact]"
		"%s", Where);

	if (!DataTable.ExecuteStatement ()) return false;

	DataTable.BindColumn (SQL_CHAR,			&ContactItem.FirstName,					sizeof (ContactItem.FirstName));
	DataTable.BindColumn (SQL_CHAR,			&ContactItem.LastName,					sizeof (ContactItem.LastName));
	DataTable.BindColumn (SQL_INTEGER,	&ContactItem.SpeedDial,					sizeof (ContactItem.SpeedDial));
	DataTable.BindColumn (SQL_CHAR,			&ContactItem.BusinessPhone,			sizeof (ContactItem.BusinessPhone));
	DataTable.BindColumn (SQL_CHAR,			&ContactItem.HomePhone,					sizeof (ContactItem.HomePhone));
	DataTable.BindColumn (SQL_CHAR,			&ContactItem.MobilePhone,				sizeof (ContactItem.MobilePhone));

	ZeroMemory (&ContactItem, sizeof (ContactItem));

	return true;
}

//-----------------------------------------------------------------------------------------
bool CONTACT_LIST::SQLReadRecords (void)
{
	ContactMax = -1;

	ZeroMemory (&Contact, sizeof (Contact));

	if (!StartQuery ("")) return false;

	while (DataTable.ReadRecord ())
	{
		if (ContactMax == MAX_CONTACT)
		{
			DataTable.AbortQuery = true;

			ReportError ("Contact", "Too many Contact items!");
		}
		else
		{
			ContactMax++;
			Contact[ContactMax] = ContactItem;
		}

		ZeroMemory (&ContactItem, sizeof (ContactItem));
	}

	return true;
}

//-----------------------------------------------------------------------------------------
void CONTACT_LIST::SQLWriteRecords (void)
{
	int		I;

	//Drop table and recreate it to make sure we get rid of old items
	//DataTable.DropTable ();
	//DataTable.CreateTable ();

	for (I = 0; I <= ContactMax; I++)
	{
		if (Contact[I].SQLRecordControl.New)
		{
			DataTable.OpenStatementHandle ();
			QPsprintf_s (DataTable.Statement,  "Insert Contact ([First Name], [Last Name]) Values ('%s', '%s')", Contact[I].FirstName, Contact[I].LastName);
			if (!DataTable.ExecuteStatement ()) return;
			DataTable.CloseStatementHandle ();
		}

		if (Contact[I].SQLRecordControl.Modified)
		{
			DataTable.CleanString (Contact[I].FirstName, sizeof (Contact[I].FirstName), Contact[I].FirstName); //Remove illegal characters	
			DataTable.CleanString (Contact[I].LastName, sizeof (Contact[I].LastName), Contact[I].LastName); //Remove illegal characters

			DataTable.StartUpdate ();
			DataTable.UpdateItem ("Speed Dial",					Contact[I].SpeedDial);
			DataTable.UpdateItem ("Business Phone",			Contact[I].BusinessPhone);
			DataTable.UpdateItem ("Home Phone",					Contact[I].HomePhone);
			DataTable.UpdateItem ("Mobile Phone",				Contact[I].MobilePhone);
			DataTable.DoUpdate ("where [First Name] = '%s' and [Last Name] = '%s'", Contact[I].FirstName, Contact[I].LastName);
		}
	}
}

//--------------------------------------------------------------------------------------
bool CONTACT_LIST::CheckField (void)
{
	return false;
}

//-------------------------------------------------------------------------------------------------------- 
void CONTACT_LIST::RightButtonDown (void)
{
	ReadCSVFileSelection				= DefinePopupMenu	(NULL, 0, "Read CSV file");
	WriteDirectoryFileSelection = DefinePopupMenu	(NULL, 0, "Write Directory files");
}

//--------------------------------------------------------------------------------------------- 
void CONTACT_LIST::WriteItem (int		Index,
															PCHAR	PhoneNumber,
															PCHAR	NameModifier)
{
	int		I,J;
	char	FileLine[MAX_STRING];
	char	CleanPhoneNumber[MAX_STRING];

	if (StringEqual (PhoneNumber, "")) return;

	strcpy_s (FileLine, "<item>\r\n");																		DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
	QPsprintf_s (FileLine, "<fn>%s</fn>\r\n", Contact[Index].FirstName);	DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));

	QPsprintf_s (FileLine, "<ln>%s (%s)</ln>\r\n", Contact[Index].LastName, NameModifier);		DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));

	ZeroMemory (&CleanPhoneNumber, sizeof (CleanPhoneNumber));
	J = 0;
	for (I = 0; I <= strlen (PhoneNumber)-1; I++)
	{
		//The phone won't match up numbers with +'s, spaces, (), and -'s in them
		if ((PhoneNumber[I] != ' ') &&
				(PhoneNumber[I] != '-') &&
				(PhoneNumber[I] != '+') &&
				(PhoneNumber[I] != '(') &&
				(PhoneNumber[I] != ')'))
		{
			CleanPhoneNumber[J] = PhoneNumber[I];
			J++;
		}
	}

	QPsprintf_s (FileLine, "<ct>%s</ct>\r\n", CleanPhoneNumber); DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));

	strcpy_s (FileLine, "<rt>0</rt>\r\n");	DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
	strcpy_s (FileLine, "<ad>0</ad>\r\n");	DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
	strcpy_s (FileLine, "<ar>0</ar>\r\n");	DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
	strcpy_s (FileLine, "</item>\r\n");			DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
}

//--------------------------------------------------------------------------------------------- 
void CONTACT_LIST::WriteTSVItem (int		Index,
																	PCHAR	PhoneNumber,
																	PCHAR	NameModifier)
{
	int		I;
	char	FileLine[MAX_STRING];

	if (StringEqual (PhoneNumber, "")) return;

	QPsprintf_s (FileLine, "%s %s (%s)%s", Contact[Index].FirstName, Contact[Index].LastName, NameModifier, TAB);

	for (I = 0; I <= strlen (FileLine)-1; I++)
	{
		DiskFile.Write (PCHAR (&FileLine[I]), 1);
		DiskFile.Write ("\x00", 1);
	}

	QPsprintf_s (FileLine, "%s\r\n", PhoneNumber);

	for (I = 0; I <= strlen (FileLine)-1; I++)
	{
		//The phone will reject numbers with +'s, spaces, () in them
		if ((FileLine[I] != '+') &&
				(FileLine[I] != '(') &&
				(FileLine[I] != ')') &&
				(FileLine[I] != '-') &&
				(FileLine[I] != ' '))
		{
			DiskFile.Write (PCHAR (&FileLine[I]), 1);
			DiskFile.Write ("\x00", 1);
		}
	}
}

//--------------------------------------------------------------------------------------------- 
int CONTACT_LIST::PopupMenuCommand (int		PopupMenuIndex)
{	
	int							I;
	DISK_FILE				CSVFile;
	char						UserProfile[MAX_PATH];
	char						FullCSVFileName[MAX_PATH];
	char						FullTSVFileName[MAX_PATH];
	char						FileLine[MAX_STRING * 8];
	int							Index;
	int							ContactIndex;

	if (PopupMenuIndex == ReadCSVFileSelection)
	{
		ReportError ("Contact",
			"In Outlook go to File | Open & Export\n"
			"Choose Import / Export\n"
			"Choose Export to a file\n"
			"Choose Comma Seperated Values\n"
			"Select folder to export from as Contacts\n"
			"Save exported file as default\n"
			"Push Finished.\n\n"
			"Import into Excel, delete columns to the right, and save back to the CSV file.");

		SetWaitCursor ();

		//In Documents
		if (GetEnvironmentVariable ("UserProfile", UserProfile, sizeof (UserProfile)) == 0)
		{
			ReportError ("ContactCustomer", "User profile not found");
		}

		//This is the default place the export from Pinnacle Cart puts the file.
		QPsprintf_s (FullCSVFileName,
			sizeof (FullCSVFileName),
			"%s\\Documents\\Outlook.csv",
			UserProfile);

		ChangeSlashes (FullCSVFileName, "/");

		if (!CSVFile.OpenToRead (FullCSVFileName))
		{
			ReportError ("Contact", "Outlook.csv file not found!");
			return 0;
		}

		//Database backup uses LF and everything is in quotes.  The other save to CSV uses CR
		CSVFile.ReadLine (FileLine, sizeof (FileLine), LF);
		ParseLine (&Heading, FileLine);

		ZeroMemory (&ContactItem, sizeof (ContactItem));

		do
		{
			if (CSVFile.ReadLine (FileLine, sizeof (FileLine), LF))
			{
				ParseLine (&Data, FileLine);

				Index = GetIndex ("First Name");
				if (Index < 0) ReportError ("Contact", "First Name not found");
				else strcpy_s (ContactItem.FirstName, Data.Field[Index].String);

				Index = GetIndex ("Last Name");
				if (Index < 0) ReportError ("Contact", "Last Name not found");
				else strcpy_s (ContactItem.LastName, Data.Field[Index].String);

				DataTable.CleanString (ContactItem.FirstName, sizeof (ContactItem.FirstName), ContactItem.FirstName); //Remove illegal characters	
				DataTable.CleanString (ContactItem.LastName, sizeof (ContactItem.LastName), ContactItem.LastName); //Remove illegal characters

				Index = GetIndex ("Business Phone");
				if (Index < 0) ReportError ("Contact", "Bussiness Phone not found");
				else strcpy_s (ContactItem.BusinessPhone, Data.Field[Index].String);

				Index = GetIndex ("Home Phone");
				if (Index < 0) ReportError ("Contact", "Home Phone not found");
				else strcpy_s (ContactItem.HomePhone, Data.Field[Index].String);

				Index = GetIndex ("Mobile Phone");
				if (Index < 0) ReportError ("Contact", "Mobile Phone not found");
				else strcpy_s (ContactItem.MobilePhone, Data.Field[Index].String);

				//Update existing item or add new item
				ContactIndex = GetContactIndex (ContactItem.FirstName, ContactItem.LastName);
				if (ContactIndex < 0)
				{
					ContactMax++;
					ContactIndex = ContactMax;
					Contact[ContactIndex] = ContactItem; //Add item
					SetSQLRecordNew (ContactIndex, true);
				}
				else
				{
					if ((Contact[ContactIndex].BusinessPhone != ContactItem.BusinessPhone) ||
							(Contact[ContactIndex].HomePhone		 != ContactItem.HomePhone) ||
							(Contact[ContactIndex].MobilePhone	 != ContactItem.MobilePhone))
					{
						Contact[ContactIndex] = ContactItem; //Update item
						SetSQLRecordModified (ContactIndex, true);
					}
				}
			ZeroMemory (&ContactItem, sizeof (ContactItem));
			}
		} while (!StringEqual (FileLine, ""));

		//Don't auto write for now so we can see new records and updates
		//WriteRecords ();

		InvalidateAllRecords ();

		SetArrowCursor ();

		return 0;
	}

	else if (PopupMenuIndex == WriteDirectoryFileSelection)
	{
		SetWaitCursor ();
		//Write the Polycom phone book
		DiskFile.OpenToWrite ("//Mahood-DNS/C$/FTPRoot/Polycom 4.1.3 Rev G/000all-directory.xml");

		strcpy_s (FileLine, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\r\n");			DiskFile.Write (FileLine, int (strlen (FileLine)));
		strcpy_s (FileLine, "<!-- edited with XMLSPY v5 rel. 4 U (http://www.xmlspy.com) -->\r\n");		DiskFile.Write (FileLine, int (strlen (FileLine)));
		strcpy_s (FileLine, "<?Saved @ --2013-08-25T17:01:05-- ?>\r\n");															DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
		strcpy_s (FileLine, "<directory>\r\n");																												DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
		strcpy_s (FileLine, "<item_list>\r\n");																												DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));

		//The phone directory only has one number per line so people with multiple phones need multiple lines
		for (I = 0; I <= ContactMax; I++)
		{
			WriteItem (I, Contact[I].BusinessPhone, "B");
			WriteItem (I, Contact[I].HomePhone,			"H");
			WriteItem (I, Contact[I].MobilePhone,		"M");
		}
		strcpy_s (FileLine, "</item_list>\r\n");	DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));
		strcpy_s (FileLine, "</directory>\r\n");	DiskFile.Write (FileLine, int (strnlen_s (FileLine, sizeof (FileLine))));

		DiskFile.Close ();

		//Write the panasonic phone book
		//In Documents
		if (GetEnvironmentVariable ("UserProfile", UserProfile, sizeof (UserProfile)) == 0)
		{
			ReportError ("ContactCustomer", "User profile not found");
		}

		QPsprintf_s (FullTSVFileName,
			sizeof (FullTSVFileName),
			"%s\\Documents\\Phonebook.tsv",
			UserProfile);

		ChangeSlashes (FullTSVFileName, "/");
		DiskFile.OpenToWrite (FullTSVFileName);

		//The BOM Beginning of message
		ZeroMemory (&FileLine, sizeof (FileLine));
		strcpy_s (FileLine, "\xFF\xFE"); DiskFile.Write (FileLine, int (strlen (FileLine)));

		for (I = 0; I <= ContactMax; I++)
		{
			//Panasonic only has room for 100 contact and with multiple numbers the list fills.  Just put one number per person
			if (!StringEqual (Contact[I].MobilePhone, ""))
			{
				WriteTSVItem (I, Contact[I].MobilePhone,		"M");
			}
			else if (!StringEqual (Contact[I].BusinessPhone, ""))
			{
				WriteTSVItem (I, Contact[I].BusinessPhone,	"B");
			}
			else if (!StringEqual (Contact[I].HomePhone, ""))
			{
				WriteTSVItem (I, Contact[I].HomePhone,			"H");
			}
		}

		DiskFile.Close ();



		//In Documents
		if (GetEnvironmentVariable ("UserProfile", UserProfile, sizeof (UserProfile)) == 0)
		{
			ReportError ("ContactCustomer", "User profile not found");
		}

		//This is the default place the export from Pinnacle Cart puts the file.
		QPsprintf_s (FullTSVFileName,
			sizeof (FullTSVFileName),
			"%s\\Downloads\\phonebook.tsv",
			UserProfile);

		ChangeSlashes (FullTSVFileName, "/");

		if (!DiskFile.OpenToRead (FullTSVFileName))
		{
			ReportError ("Contact", "Outlook.tsv file not found!");
			return 0;
		}

		//Database backup uses LF and everything is in quotes.  The other save to CSV uses CR
		DiskFile.Read (FileLine, sizeof (FileLine));
		DiskFile.Close ();

		//This is the default place the export from Pinnacle Cart puts the file.
		strcpy_s (FileLine, "");

		QPsprintf_s (FullTSVFileName,
			sizeof (FullTSVFileName),
			"%s\\Documents\\phonebook.tsv",
			UserProfile);

		ChangeSlashes (FullTSVFileName, "/");

		if (!DiskFile.OpenToRead (FullTSVFileName))
		{
			ReportError ("Contact", "Outlook.tsv file not found!");
			return 0;
		}

		//Database backup uses LF and everything is in quotes.  The other save to CSV uses CR
		DiskFile.Read (FileLine, sizeof (FileLine));
		DiskFile.Close ();


		SetArrowCursor ();
		return 0;
	}

	else return DISPLAY_WINDOW::PopupMenuCommand (PopupMenuIndex);
}

//--------------------------------------------------------------------------------------------- 
void CONTACT_LIST::ParseLine (pLINE_FIELDS	LineFields,
															PCHAR					FileLine)
{
	int		I, J;
	char	Temp[MAX_STRING * 9];
	char	Field[4];
	bool	QuoteFound;

	ZeroMemory (LineFields, sizeof (LINE_FIELDS));
	LineFields->FieldMax = -1;

	if (StringEqual (FileLine, "")) return;

	strcpy_s (Temp, FileLine);

	I = 0;
	J = 0;
	strcpy_s (Field, " "); //One character at a time
	QuoteFound = false;

	while (Temp[I] != char (0))
	{
		//CSV files that have a string with a , will put the string inside "" and some CSV files have quotes areound every field
		if (Temp[I] == '"')
		{
			//Ignore the " it is enclosing a string with a , or maybe every string
			QuoteFound = !QuoteFound;

			I++;
		}

		else if (QuoteFound ||
						 (Temp[I] != ','))
		{
			if (strlen (LineFields->Field[J].String) == (sizeof (LineFields->Field[J].String) - 1))
			{
				ReportError ("Contact", "Field too long %s", Field);
			}
			else
			{
				Field[0] = Temp[I];
				strcat_s (LineFields->Field[J].String, Field);
			}

			I++;
		}

		else //A comma found
		{
			//There may have been a space after the command prior to the number
			StringTrim (LineFields->Field[J].String);

			I++; //Next after the comma
			J++; //Next Field
		}
	}

	StringTrim (LineFields->Field[J].String);

	LineFields->FieldMax = J;
}

//--------------------------------------------------------------------------------------
int CONTACT_LIST::GetIndex (PCHAR	HeadingString)
{
	int		Index;
	bool	Found;

	Index = 0;
	Found = false;
		
	while (!Found &&
				 (Index <= Heading.FieldMax))
	{
		if (StringEqual (HeadingString, Heading.Field[Index].String))
					Found = true;
		else	Index++;
	}

	if (Found)
				return Index;
	else	return -1;
}

//--------------------------------------------------------------------------------------
int CONTACT_LIST::GetContactIndex (PCHAR	FirstName,
																	 PCHAR	LastName)
{
	int		Index;
	bool	Found;

	Index = 0;
	Found = false;
		
	while (!Found &&
				 (Index <= ContactMax))
	{
		if (StringEqual (Contact[Index].FirstName, FirstName) &&
				StringEqual (Contact[Index].LastName, LastName))
					Found = true;
		else	Index++;
	}

	if (Found)
				return Index;
	else	return -1;
}