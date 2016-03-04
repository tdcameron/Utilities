//---------------------------------------------------
// Inventory Object
// Copyright (C) 2012 Mahood and Company.
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

#include "Inventory.h"

//-------------------------------------------------------------------------------------------------------- 
void INVENTORY_LIST::Initialize (void)

{
	DoRegisterClass ("Inventory", "Inventory");

	UseCustomerHelp = true;
}

//---------------------------------------------------------------------------------------
void INVENTORY_LIST::DoCreateWindow (void)
{
	if (!StringEqual (CustomerName, "Antoinettes"))
	{
		ReportError ("Inventory", "Only open this from the Antoinettes customer!");
	}

  InitDisplay ();

	NoEdit = true;
	NoKeys = true;

	DefineFields ();
  
	ReadAndVerifyRecords ();

	DISPLAY_WINDOW::DoCreateWindow ();
}
 
//-----------------------------------------------------------------------------------------
void INVENTORY_LIST::DefineFields (void)
{
	EnableSQLMode (SQL_No_edit);

	strcpy_s (DataTable.CreateTableStatement, "Create Table [Inventory] ("
		"[Product ID]					INTEGER NOT NULL,\n"
		"[Name]								VARCHAR (80), \n"
		"[Price]							DOUBLE PRECISION,\n"
		"[Cost]								DOUBLE PRECISION,\n"
		"[Sale Price]					DOUBLE PRECISION,\n"
		"[Available]					VARCHAR (4), \n"
		"[Category]						VARCHAR (32), \n"
		"[Manufacturer name]	VARCHAR (80), \n"
		"[Date added]					DATETIME)");

	//strcpy_s (DataTable.PrimaryKeyStatement, "Alter Table [Inventory] add Primary Key ([Product ID])");

	DefineData (1, //Screen Version
							&Inventory,
							&InventoryMax,
							sizeof (Inventory),
							sizeof (Inventory[0]),
							&Inventory[0].SQLRecordControl,
							"Inventory");

	ProductIDField = AddIntegerField ("Product ID", "",
		&Inventory[0].ProductID);

	NameField = AddStringField ("Name", "",
		&Inventory[0].Name, sizeof (Inventory[0].Name), Either_case);

  PriceField = AddDoubleField ("Price", "",
		&Inventory[0].Price, 2);

  CostField = AddDoubleField ("Cost", "",
		&Inventory[0].Cost, 2);

  SalePriceField = AddDoubleField ("Sale", "Price",
		&Inventory[0].SalePrice, 2);

  AvailableField = AddStringField ("Available", "",
		&Inventory[0].Available, sizeof (Inventory[0].Available), Either_case);

  CategoryField = AddStringField ("Category", "",
		&Inventory[0].Category, sizeof (Inventory[0].Category), Either_case);

  ManufacturerNameField = AddStringField ("Manufacturer", "Name",
		&Inventory[0].ManufacturerName, sizeof (Inventory[0].ManufacturerName), Either_case);

	DateAddedField = AddDateTimeField ("Date", "Added",
		&Inventory[0].DateAdded);

	FieldsDefined ();
}

//-----------------------------------------------------------------------------------------
bool INVENTORY_LIST::StartQuery (PCHAR	Format, ...)
{
	va_list	Args; 	
	char		Where[MAX_STRING];

	//Get format and format variable arguments into Message
	va_start (Args, Format);
	vQPsprintf_s (Where, sizeof (Where), Format, Args);
	va_end (Args);

	DataTable.StartQuery ();

	QPsprintf_s (DataTable.Statement, 
		"Select [Product ID], [Name], [Price], [Cost], [Sale Price], [Available], [Category], [Manufacturer name], [Date added] From [Inventory]"
		"%s", Where);

	if (!DataTable.ExecuteStatement ()) return false;

	DataTable.BindColumn (SQL_INTEGER,	&InventoryItem.ProductID,					sizeof (InventoryItem.ProductID));
	DataTable.BindColumn (SQL_CHAR,			&InventoryItem.Name,							sizeof (InventoryItem.Name));
	DataTable.BindColumn (SQL_DOUBLE,		&InventoryItem.Price,							sizeof (InventoryItem.Price));
	DataTable.BindColumn (SQL_DOUBLE,		&InventoryItem.Cost,							sizeof (InventoryItem.Cost));
	DataTable.BindColumn (SQL_DOUBLE,		&InventoryItem.SalePrice,					sizeof (InventoryItem.SalePrice));
	DataTable.BindColumn (SQL_CHAR,			&InventoryItem.Available,					sizeof (InventoryItem.Available));
	DataTable.BindColumn (SQL_CHAR,			&InventoryItem.Category,					sizeof (InventoryItem.Category));
	DataTable.BindColumn (SQL_CHAR,			&InventoryItem.ManufacturerName,	sizeof (InventoryItem.ManufacturerName));
	DataTable.BindColumn (SQL_DATETIME,	&InventoryItem.DateAdded,					sizeof (InventoryItem.DateAdded));

	ZeroMemory (&InventoryItem, sizeof (InventoryItem));

	return true;
}

//-----------------------------------------------------------------------------------------
bool INVENTORY_LIST::SQLReadRecords (void)
{
	int	I;

	InventoryMax = -1;

	ZeroMemory (&Inventory, sizeof (Inventory));

	if (!StartQuery ("")) return false;

	while (DataTable.ReadRecord ())
	{
		if (InventoryMax == MAX_INVENTORY)
		{
			DataTable.AbortQuery = true;

			ReportError ("Inventory", "Too many inventory items!");
		}
		else
		{
			InventoryMax++;
			Inventory[InventoryMax] = InventoryItem;
		}

		ZeroMemory (&InventoryItem, sizeof (InventoryItem));
	}

	//Add total collection
	if (InventoryMax < MAX_INVENTORY)
	{
		InventoryMax++;
		ZeroMemory (&Inventory[InventoryMax], sizeof (Inventory[InventoryMax]));
		Inventory[InventoryMax].ProductID = 999997;
		strcpy_s (Inventory[InventoryMax].Name, "Total Collection");

		for (I = 0; I <= InventoryMax - 1; I++)
		{
	
			if (StringEqual(Inventory[I].Available, "Yes") &&
					StringEqual (Inventory[I].Category, "Collection"))
			{
				Inventory[InventoryMax].Cost	= Inventory[InventoryMax].Cost + Inventory[I].Cost;
				Inventory[InventoryMax].Price	= Inventory[InventoryMax].Price + Inventory[I].Price;
			}
		}

		//Show average gross margin
		Inventory[InventoryMax].SalePrice = 1.0 - (Inventory[InventoryMax].Cost / Inventory[InventoryMax].Price);
	}

	//Add total availagble not in collection
	if (InventoryMax < MAX_INVENTORY)
	{
		InventoryMax++;
		ZeroMemory(&Inventory[InventoryMax], sizeof(Inventory[InventoryMax]));
		Inventory[InventoryMax].ProductID = 999998;
		strcpy_s(Inventory[InventoryMax].Name, "Total Available for sale");

		for (I = 0; I <= InventoryMax - 1; I++)
		{
			if (StringEqual(Inventory[I].Available, "Yes") &&
				!StringEqual(Inventory[I].Category, "Collection"))
			{
				Inventory[InventoryMax].Cost = Inventory[InventoryMax].Cost + Inventory[I].Cost;
				Inventory[InventoryMax].Price = Inventory[InventoryMax].Price + Inventory[I].Price;
			}
		}

		//Show average gross margin
		Inventory[InventoryMax].SalePrice = 1.0 - (Inventory[InventoryMax].Cost / Inventory[InventoryMax].Price);
	}

	//Put in total available and collection
	if (InventoryMax < MAX_INVENTORY)
	{
		InventoryMax++;
		ZeroMemory(&Inventory[InventoryMax], sizeof(Inventory[InventoryMax]));
		Inventory[InventoryMax].ProductID = 999999;
		strcpy_s(Inventory[InventoryMax].Name, "Total All");

		Inventory[InventoryMax].Cost	= Inventory[InventoryMax-1].Cost + Inventory[InventoryMax-2].Cost;
		Inventory[InventoryMax].Price	= Inventory[InventoryMax-1].Price + Inventory[InventoryMax-2].Price;

		//Show average gross margin
		Inventory[InventoryMax].SalePrice = 1.0 - (Inventory[InventoryMax].Cost / Inventory[InventoryMax].Price);
	}

	return true;
}

//-----------------------------------------------------------------------------------------
void INVENTORY_LIST::SQLWriteRecords (void)
{
	int		I;

	//Drop table and recreate it to make sure we get rid of old items
	//DataTable.DropTable ();
	//DataTable.CreateTable ();

	for (I = 0; I <= InventoryMax; I++)
	{
		if (Inventory[I].SQLRecordControl.New)
		{
			DataTable.OpenStatementHandle ();
			QPsprintf_s (DataTable.Statement,  "Insert Inventory ([Product ID]) Values ('%d')", Inventory[I].ProductID);
			if (!DataTable.ExecuteStatement ()) return;
			DataTable.CloseStatementHandle ();
		}

		if (Inventory[I].SQLRecordControl.Modified)
		{
			DataTable.StartUpdate ();
			DataTable.UpdateItem ("Name",								Inventory[I].Name);
			DataTable.UpdateItem ("Price",							Inventory[I].Price);
			DataTable.UpdateItem ("Cost",								Inventory[I].Cost);
			DataTable.UpdateItem ("Sale Price",					Inventory[I].SalePrice);
			DataTable.UpdateItem ("Available",					Inventory[I].Available);
			DataTable.UpdateItem ("Category",						Inventory[I].Category);
			DataTable.UpdateItem ("Manufacturer name",	Inventory[I].ManufacturerName);
			DataTable.UpdateDateTime ("Date added",			Inventory[I].DateAdded);
			DataTable.DoUpdate ("where [Product ID] = %d", Inventory[I].ProductID);
		}
	}
}

//--------------------------------------------------------------------------------------
bool INVENTORY_LIST::CheckField (void)
{
	return false;
}

//-------------------------------------------------------------------------------------------------------- 
void INVENTORY_LIST::RightButtonDown (void)
{
	ReadCSVFileSelection		= DefinePopupMenu(NULL, 0, "Read CSV file");
	HideSoldItemsSelection	= DefinePopupMenu(NULL, 0, "Hide sold items");
}

//--------------------------------------------------------------------------------------------- 
int INVENTORY_LIST::PopupMenuCommand (int		PopupMenuIndex)
{	
	int							I,J;
	DISK_FILE				CSVFile;
	char						UserProfile[MAX_PATH];
	char						FullCSVFileName[MAX_PATH];
	char						FileLine[MAX_STRING * 8];
	char						TempString[48];
	FILETIME				FileTime;
	SYSTEMTIME			SystemTime;
	int							Index;
	int							ProductIndex;

	if (PopupMenuIndex == ReadCSVFileSelection)
	{
		//In Documents
		if (GetEnvironmentVariable ("UserProfile", UserProfile, sizeof (UserProfile)) == 0)
		{
			ReportError ("InventoryCustomer", "User profile not found");
		}

		//This is the default place the export from Pinnacle Cart puts the file.
		QPsprintf_s (FullCSVFileName,
			sizeof (FullCSVFileName),
			"%s\\Downloads\\Products.csv",
			UserProfile);

		ChangeSlashes (FullCSVFileName, "/");

		DeleteFile(FullCSVFileName); //Delete prior file so we can export a new one

		ReportError ("Inventory",
			"In Pinnical Cart go to Cart Settings | Database & Backup\n"
			"Choose Products then check all.  Don't set dates.\n"
			"Push Export Database\n"
			"Push Save.");

		SetWaitCursor ();



		if (!CSVFile.OpenToRead (FullCSVFileName))
		{
			ReportError ("Inventory", "Products.csv file not found!");
			return 0;
		}

		//Database backup uses LF and everything is in quotes.  The other save to CSV uses CR
		CSVFile.ReadLine (FileLine, sizeof (FileLine), LF);
		ParseLine (&Heading, FileLine);

		ZeroMemory (&InventoryItem, sizeof (InventoryItem));

		do
		{
			if (CSVFile.ReadLine (FileLine, sizeof (FileLine), LF))
			{
				ParseLine (&Data, FileLine);

				Index = GetIndex ("Product ID");
				if (Index < 0) ReportError ("Inventory", "Product ID not found");
				else InventoryItem.ProductID = StringToInteger32 (Data.Field[Index].String);

				Index = GetIndex ("Name");
				if (Index < 0) ReportError ("Inventory", "Name not found");
				else strcpy_s (InventoryItem.Name, Data.Field[Index].String);

				Index = GetIndex ("Price");
				if (Index < 0) ReportError ("Inventory", "Price not found");
				else InventoryItem.Price = StringToInteger32 (Data.Field[Index].String);

				Index = GetIndex ("Cost");
				if (Index < 0) ReportError ("Inventory", "Cost not found");
				else InventoryItem.Cost= StringToInteger32 (Data.Field[Index].String);

				Index = GetIndex ("Sale Price");
				if (Index < 0) ReportError ("Inventory", "Sale Price not found");
				else InventoryItem.SalePrice= StringToInteger32 (Data.Field[Index].String);

				Index = GetIndex ("Available");
				if (Index < 0) ReportError ("Inventory", "Available not found");
				else strcpy_s (InventoryItem.Available, Data.Field[Index].String);

				Index = GetIndex ("Categories");
				if (Index < 0) ReportError ("Inventory", "Categories not found");
				else strcpy_s (InventoryItem.Category, Data.Field[Index].String);

				Index = GetIndex ("Manufacturer Name");
				if (Index < 0) ReportError ("Inventory", "Manufacturer Name not found");
				else strcpy_s (InventoryItem.ManufacturerName, Data.Field[Index].String);

				Index = GetIndex ("Date Added");
				if (Index < 0) ReportError ("Inventory", "Date Added not found");
				else
				{
					strcpy_s (TempString, Data.Field[Index].String);

					//The CSV from product browse is like this M/D/Y H:M and fields can be 1-4 characters
					//The CSV from data backup is Y-M-D H:M:S and fields can be 1-4 characters
					I = 0;
					J = 0;
					ZeroMemory (&SystemTime, sizeof (SystemTime));

					while (TempString[I] != '-') I++;
					TempString[I] = 0;
					SystemTime.wYear = WORD (StringToInteger32 (PCHAR (&TempString[J])));
					I++; //next item
					J = I;

					while (TempString[I] != '-') I++;
					TempString[I] = 0;
					SystemTime.wMonth = WORD (StringToInteger32 (PCHAR (&TempString[J])));
					I++; //next item
					J = I;

					while (TempString[I] != ' ') I++;
					TempString[I] = 0;
					SystemTime.wDay = WORD (StringToInteger32 (PCHAR (&TempString[J])));
					I++; //next item
					J = I;

					while (TempString[I] != ':') I++;
					TempString[I] = 0;
					SystemTime.wHour = WORD (StringToInteger32 (PCHAR (&TempString[J])));
					I++; //next item
					J = I;

					while (TempString[I] != ':') I++;
					TempString[I] = 0;
					SystemTime.wMinute = WORD (StringToInteger32 (PCHAR (&TempString[J])));
					I++; //next item
					J = I;

					SystemTime.wSecond = WORD (StringToInteger32 (PCHAR (&TempString[J])));

					if (!SystemTimeToFileTime (&SystemTime, &FileTime))
					{
						time (&InventoryItem.DateAdded); //A valid value
						ReportError ("Inventory", "StringToDateTime 1!");
						//return 0;
					}
					else FileTimeToDateTime (FileTime, &InventoryItem.DateAdded);
				}

				//Update existing item or add new item
				ProductIndex = GetProductIndex (InventoryItem.ProductID);
				if (ProductIndex < 0)
				{
					InventoryMax++;
					ProductIndex = InventoryMax;
					Inventory[ProductIndex] = InventoryItem; //Add item
					SetSQLRecordNew (ProductIndex, true);
				}
				else
				{
					if (!StringEqual (Inventory[ProductIndex].Name,							InventoryItem.Name) ||
													 (Inventory[ProductIndex].Price !=					InventoryItem.Price) ||
													 (Inventory[ProductIndex].Cost !=						InventoryItem.Cost) ||
													 (Inventory[ProductIndex].SalePrice !=			InventoryItem.SalePrice) ||
							!StringEqual (Inventory[ProductIndex].Available,				InventoryItem.Available) ||
							!StringEqual (Inventory[ProductIndex].Category,					InventoryItem.Category) ||
							!StringEqual (Inventory[ProductIndex].ManufacturerName,	InventoryItem.ManufacturerName))
							// don't test there is a round off difference (Inventory[ProductIndex].DateAdded != InventoryItem.DateAdded))
					{
						Inventory[ProductIndex] = InventoryItem; //Update item
						SetSQLRecordModified (ProductIndex, true);
					}
				}

			ZeroMemory (&InventoryItem, sizeof (InventoryItem));
			}
		} while (!StringEqual (FileLine, ""));

		//Don't auto write for now so we can see new records and updates
		//WriteRecords ();

		InvalidateAllRecords ();

		SetArrowCursor ();

		return 0;
	}

	else if (PopupMenuIndex == HideSoldItemsSelection)
	{
		I = 0;
		while (I <= InventoryMax)
		{
			if (StringEqual (Inventory[I].Available, "No"))
						DeleteRecords(I, 1);
			else	I++;
		}
		InvalidateAllRecords();
		return 0;
	}

	else return DISPLAY_WINDOW::PopupMenuCommand (PopupMenuIndex);
}

//--------------------------------------------------------------------------------------------- 
void INVENTORY_LIST::ParseLine (pLINE_FIELDS	LineFields,
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
				ReportError ("Inventory", "Field too long %s", Field);
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
int INVENTORY_LIST::GetIndex (PCHAR	HeadingString)
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
int INVENTORY_LIST::GetProductIndex (int	ProductID)
{
	int		Index;
	bool	Found;

	Index = 0;
	Found = false;
		
	while (!Found &&
				 (Index <= InventoryMax))
	{
		if (Inventory[Index].ProductID == ProductID)
					Found = true;
		else	Index++;
	}

	if (Found)
				return Index;
	else	return -1;
}