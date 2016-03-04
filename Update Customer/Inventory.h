//---------------------------------------------------
//	Inventory Object
//	Copyright (C) 2012 Mahood and Company.
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
 
#define MAX_INVENTORY		2000
#define MAX_FIELD_SIZE	200

//INVENTORY_ITEM------------------------------------------------------------------
typedef struct INVENTORY_ITEM
{
	int					ProductID;
	char				Name[60];
	double			Price;
	double			Cost;
	double			SalePrice;
	char				Available[8];
	char				Category[60];
	char				ManufacturerName[60];
	DATE_TIME		DateAdded;

	SQL_RECORD_CONTROL SQLRecordControl;
} INVENTORY_ITEM;

typedef INVENTORY_ITEM *pINVENTORY_ITEM;

//LINE_FIELD------------------------------------------------------------------
typedef struct LINE_FIELD
{
	char				String[MAX_STRING * 4];
} LINE_FIELD;

typedef LINE_FIELD *pLINE_FIELD;

//LINE_FIELDS---------------------------------------------------------------------
typedef struct LINE_FIELDS
{
	LINE_FIELD	Field[MAX_FIELD_SIZE + 1];
	int					FieldMax;
} LINE_FIELDS;

typedef LINE_FIELDS *pLINE_FIELDS;


//INVENTORY_LIST------------------------------------------------------------------
class INVENTORY_LIST: public DISPLAY_WINDOW
{
	public: 
	INVENTORY_ITEM	Inventory[MAX_INVENTORY + 1];
	int							InventoryMax;

	INVENTORY_ITEM	InventoryItem;

	LINE_FIELDS	Heading;
	LINE_FIELDS	Data;

	int		ProductIDField;
	int		NameField;
	int		PriceField;
	int		CostField;
	int		SalePriceField;
	int		AvailableField;
	int		CategoryField;
	int		ManufacturerNameField;
	int		DateAddedField;

	int		ReadCSVFileSelection;
	int		HideSoldItemsSelection;

	virtual void Initialize					(void);

	virtual void DoCreateWindow			(void);
	virtual	void DefineFields				(void);

					void ParseLine					(pLINE_FIELDS	LineFields,
																	PCHAR					FileLine);
					int	 GetIndex						(PCHAR				HeadingString);
					int  GetProductIndex		(int					ProductID);

	private: 
	virtual	bool StartQuery					(PCHAR	Format, ...);
	virtual bool SQLReadRecords			(void);
	virtual void SQLWriteRecords		(void);
	virtual bool CheckField					(void);
	virtual void RightButtonDown		(void);
	virtual int	 PopupMenuCommand		(int		PopupMenuIndex);
};

typedef INVENTORY_LIST  *pINVENTORY_LIST;