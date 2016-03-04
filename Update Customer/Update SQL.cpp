//---------------------------------------------------------------------------
//	Update SQL object
//	Copyright (C) 2004 Mahood and Company.
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
//---------------------------------------------------------------------------

#include "Update SQL.h"
#include "Application.h"

//These are the old definitions with a string Lot ID used for file conversion.
//Note moved because they are unlikely to change
#include "Device.h" //For ELECTRICAL_TYPE
#include "Defect Name.h" //For SCRAP_TYPE
#include "Defects Dialog.h" //For DEFECTS_DIALOG

#define	WEEKS_TO_TRANSFER	6

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void DoUpdateWaitThread (pUPDATE_SQL UpdateSQL)
{
	pAPPLICATION	Application;

	Application = pAPPLICATION (GetApplicationPointer ());

	UpdateSQL->DoWaitUpdate ();

	Application->ThreadList.DoExitThread (0);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void DoUpdateThread (pUPDATE_SQL UpdateSQL)
{
	pAPPLICATION	Application;

	Application = pAPPLICATION (GetApplicationPointer ());

	UpdateSQL->DoUpdate ();

	Application->ThreadList.DoExitThread (0);
}

//-----------------------------------------------------------------------------------------
void CUSTOM_RACK_LIST::SQLWriteRecords (void)
{
	int	I;

	//Write everything except the bar number
	RACK_LIST::SQLWriteRecords ();

	DataTable.OpenStatementHandle ();

	for (I = 0; I <= RackMax; I++)
	{
		if (Rack[I].SQLRecordControl.Modified)
		{
      //Write bar number
			QPsprintf_s (DataTable.Statement, 
				"Update Rack "
				"Set [Bar] = '%d' "
				"Where [Tool Number] = '%d' and [Rack Number] = '%d'",
				Rack[I].WorkbarNumber,
				Rack[I].ToolNumber,
				Rack[I].RackNumber);

			if (!DataTable.ExecuteStatement ()) return;
		}
	}

	DataTable.CloseStatementHandle ();
}

//-----------------------------------------------------------------------------------------
void CUSTOM_STATION_NAME_LIST::SQLWriteRecords (void)
{
	int	I;

	//Write everything except Active Batch and Batch Enabled
	STATION_NAME_LIST::SQLWriteRecords ();

	DataTable.OpenStatementHandle ();

	for (I = 0; I <= StationMax; I++)
	{
		if (Station[I].SQLRecordControl.Modified)
		{
			//Update Station
			QPsprintf_s (DataTable.Statement, 
        "Update Station "
				"Set [Active Batch] = '%s', [Batch Enabled] = '%s' "
				"Where [Name] = '%s'",
				Station[I].ActiveBatch,
				YesNoText[Station[I].BatchEnabled],
				Station[I].Name);

			if (!DataTable.ExecuteStatement ()) return;
		}
	}

	DataTable.CloseStatementHandle ();
}

//----------------------------------------------------------------------------------------------
void UPDATE_SQL::StartWaitUpdate (void)
{
	if (!Features.SimulateIO)
	{
		ReportError ("Update SQL", "You can only update if Simulate I/O is true!");
		return;
	}

	Transfering = true;

	RemoteTable.Initialize (RemoteSQLIPAddress, CustomerName, "remote"); //String turns into Reading remote table as dialog title

	//Start report generator thread
	RemoteTable.StartReportGenerator (LPTHREAD_START_ROUTINE (DoUpdateWaitThread),
																		LPVOID (this),
																		Report_will_close_status_box);
}

//----------------------------------------------------------------------------------------------
void UPDATE_SQL::StartUpdate (bool TheUpdateSQLWithFill)
{
	if (!Features.SimulateIO)
	{
		ReportError ("Update SQL", "You can only update if Simulate I/O is true!");
		return;
	}

	UpdateSQLWithFill = TheUpdateSQLWithFill;

	Transfering = true;

	RemoteTable.Initialize (RemoteSQLIPAddress, CustomerName, "remote"); //String turns into Reading remote table as dialog title

	//Start report generator thread
	RemoteTable.StartReportGenerator (LPTHREAD_START_ROUTINE (DoUpdateThread),
																		LPVOID (this),
																		Report_will_close_status_box);
}

//-----------------------------------------------------------------------------------------
bool UPDATE_SQL::DoingTableTransfer (void)
{
	if (Transfering)
	{
		ReportError ("Update SQL", "Table transfer in process, try again later.");

		return true;
	}
	else return false;
}

//----------------------------------------------------------------------------------------------
void UPDATE_SQL::DoWaitUpdate (void)
{
	WaitList.TransferTable (&RemoteTable);

	RemoteTable.CloseStatusBox ();

	Transfering = false;
}

//----------------------------------------------------------------------------------------------
#pragma warning(suppress: 6262)
void UPDATE_SQL::DoUpdate (void)
{
	PAINT_SCHEDULE_SETUP_DIALOG	PaintScheduleSetupDialog;

	if (Features.CustomerType == Painting_customer)
	{
		UpdateChain ();
		PaintList.TransferTable (&RemoteTable);
		ColorGroupList.TransferTable (&RemoteTable);
		PaintGapList.TransferTable (&RemoteTable);
		PaintScheduleSetupDialog.TransferDialogItems ();
	}

	//Do Tool, Rack, and Part first so ChangePartNumberDialog can use local table to lookup Exceptions from Part
	ToolList.TransferTable (&RemoteTable);
	RackList.TransferTable (&RemoteTable); //Customized to write workbar number
	PartList.TransferTable (&RemoteTable);
	DefectNameList.TransferTable (&RemoteTable);

	if (Features.CustomerType != Painting_customer)
	{
		//Do Workbar and Batch together so the Active batch will be correct.
		StationNameList.TransferTable (&RemoteTable); //Customized to write active batch and batch enabled
		WorkbarList.Setup (0, NULL, NULL, NULL); //Initialize GetPartNumberDialog
		WorkbarList.TransferTable (&RemoteTable);

		ProcessNameList.TransferTable (&RemoteTable);
		ProcessTimeList.TransferTable (&RemoteTable);

		DeviceNameList.TransferTable (&RemoteTable);
		DeviceList.TransferTable (&RemoteTable);

		RectifierLoopList.TransferTable (&RemoteTable);
		RectifierSetpointList.TransferTable (&RemoteTable);
		RectifierDummyList.TransferTable (&RemoteTable);

		BathLoopList.TransferTable (&RemoteTable);
		BathSetpointList.TransferTable (&RemoteTable);
		FeederList.TransferTable (&RemoteTable);
		//Can't do this because an update on the weekend will get all the tanks in standby or some other non running state
		if (UpdateSQLWithFill)
		{
			InitializeCriticalSection (&FillControlList.CriticalSection); //Initialize critical section. Don't call Initialize because it does too much
			FillControlList.TransferTable(&RemoteTable);
		}

		MoveCommandList.TransferTable (&RemoteTable);
		MovingDeviceList.TransferTable (&RemoteTable);
		MoverList.TransferTable (&RemoteTable);
		MoveTimeList.TransferTable (&RemoteTable);

		SnapIPAddressList.TransferTable (&RemoteTable);
		SprayLoopList.TransferTable (&RemoteTable);
		FillActionTimeList.TransferTable (&RemoteTable);
		CycleList.TransferTable (&RemoteTable);
	}

	if (Features.CustomerType == Anodizing_customer)
	{
		ColorSetpointList.TransferTable(&RemoteTable);
		ColorLoopList.TransferTable(&RemoteTable);
		ThicknessLimitList.TransferTable(&RemoteTable);
		TranslateList.TransferTable(&RemoteTable);
		QualityList.Initialize(); //Initialize ShiftScheduleDialog
		QualityList.TransferTable(&RemoteTable);
	}

	UpdateProduction ();

	RemoteTable.CloseStatusBox ();

	Transfering = false;
}

//----------------------------------------------------------------------------------------------
void UPDATE_SQL::UpdateProduction (void)
{
	//These just use a string form of the enumerated types which eliminates any error for null
	//This may be a problem because the destination will be blank not NULL, probably not because reading a null gives blank

	//PRODUCTION_ITEM------------------------------------------------------------------
	typedef struct
	{
		int						LotID;
		char					StartTimeText[DATE_TIME_SIZE]; //Read as text to get the offset information
		int						StartDay;
		int						StartShift;
		char					FinishTimeText[DATE_TIME_SIZE]; //Read as text to get the offset information
		int						FinishDay;
		int						FinishShift;
		int						BarNumber;
		char					PartNumber[20];
		char					Status[24];
		int						PiecesPerLot;
		int						MissingPiecesIn;
		char					Exceptions[EXCEPTIONS_SIZE];
		char					CycleStatus[33];
	} PRODUCTION_ITEM;

	//OUR_DEFECT_ITEM------------------------------------------------------------------
	typedef struct
	{
		int						LotID;
		char					Name[DEFECT_NAME_SIZE];
		int						Pieces;
		char					ScrapType[20];
		char					CorporateCode[4];
	} OUR_DEFECT_ITEM;

	int							I,J;
	PRODUCTION_ITEM	ProductionItem;
	OUR_DEFECT_ITEM	DefectItem;
	DATE_TIME				StartTime;
	char						StartDateTimeString[DATE_TIME_SIZE];
	char						Seed[10];
	int							FirstLotID;
	int							NewLotID;

	strcpy_s (RemoteTable.TableName, "Production");
	Initialize (RemoteTable.TableName);

	QPsprintf_s (RemoteTable.StatusBoxTitle, "Reading %s table", RemoteTable.TableName);
	SetWindowText (RemoteTable.StatusBox, RemoteTable.StatusBoxTitle);
	SetDlgItemText (RemoteTable.StatusBox, IDC_LOG_READING, RemoteTable.TableName);

	//Allow inserting into the Identity column Lot ID
	//This doesn't work and the hot fix doesn't fix the problem
	//OpenStatementHandle ();
	//QPsprintf_s (Statement, "Set IDENTITY_INSERT Production ON");
	//ExecuteStatement ();
	//CloseStatementHandle ();

	//Empty the production table in case the line has been down for more then a few weeks.
	DropTable ();
	CreateTable ();

	time (&StartTime);

	if (Features.CustomerType == Painting_customer)
				StartTime = StartTime - (SECONDS_PER_WEEK * WEEKS_TO_TRANSFER);
	else	StartTime = StartTime - (SECONDS_PER_WEEK * 2);

	StartTime = PriorSunday (StartTime);

	FormatWeekOfDate (StartDateTimeString, sizeof (StartDateTimeString), StartTime);

	//Read Production records
	RemoteTable.StartQuery (PCHAR (&ProductionItem.StartTimeText));

	//We have to get records from Paint lines with null Start Times because they are the place holder for Lot ID.  If the Start time is null the finish time will be null too.
	//Otherwise get all records with start times in range regardless of finish times.  Bars in the line will have start times and finish time will be NULL
	//Production records are created when the module is scheduled.  The time in the status box will show 1969 when these records are read.
	//

	QPsprintf_s (RemoteTable.Statement,
		"Select [Lot ID], [Start Time], [Start Day], [Start Shift], [Finish Time], [Finish Day], [Finish Shift], "
		"[Bar], [Part Number], [Status], [Pieces per Lot], [Missing Pieces In], [Exceptions], [Cycle Status] From [Production] "
		"where (([Start Time] is not NULL and [Start Time] >= '%s') or ([Start Time] is NULL and [Finish Time] is NULL)) "
		"order by [Lot ID]",
		StartDateTimeString);

	if (!RemoteTable.ExecuteStatement ()) return;

	//Inspect can be null for Paint lines
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.LotID,							sizeof (ProductionItem.LotID));
	RemoteTable.BindColumn (SQL_CHAR,									&ProductionItem.StartTimeText,			sizeof (ProductionItem.StartTimeText));
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.StartDay,						sizeof (ProductionItem.StartDay));
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.StartShift,					sizeof (ProductionItem.StartShift));
	RemoteTable.BindColumn (SQL_CHAR,									&ProductionItem.FinishTimeText,			sizeof (ProductionItem.FinishTimeText));
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.FinishDay,					sizeof (ProductionItem.FinishDay));
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.FinishShift,				sizeof (ProductionItem.FinishShift));
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.BarNumber,					sizeof (ProductionItem.BarNumber));
	RemoteTable.BindColumn (SQL_CHAR,									&ProductionItem.PartNumber,					sizeof (ProductionItem.PartNumber));
	RemoteTable.BindColumn (SQL_CHAR,									&ProductionItem.Status,							sizeof (ProductionItem.Status));
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.PiecesPerLot,				sizeof (ProductionItem.PiecesPerLot));
	RemoteTable.BindColumn (SQL_INTEGER,							&ProductionItem.MissingPiecesIn,		sizeof (ProductionItem.MissingPiecesIn));
	RemoteTable.BindColumn (SQL_CHAR,									&ProductionItem.Exceptions,					sizeof (ProductionItem.Exceptions));
	RemoteTable.BindColumn (SQL_CHAR,									&ProductionItem.CycleStatus,				sizeof (ProductionItem.CycleStatus));

	ZeroMemory (&ProductionItem, sizeof (PRODUCTION_ITEM));

	FirstLotID = 0;

	//Copy record
	while (RemoteTable.ReadRecord ())
	{
		//Skip all the initial low LotID records that have null starting times
		if ((FirstLotID == 0) &&
				!StringEqual (ProductionItem.PartNumber, ""))
		{
			FirstLotID = ProductionItem.LotID;

			//OpenStatementHandle ();
			//
			//Delete all records that will have duplicate LotID's
			//QPsprintf_s (PCHAR (&Statement),
			//	"Delete from Production Where [Lot ID] >= '%d'",
			//	FirstLotID);
			//
			//ExecuteStatement ();
			//
			//CloseStatementHandle ();

			//It looks like when we delete records it doesn't reset the maximum LotID
			DropTable ();

			//This is the line that sets the seed "[Lot ID] INTEGER IDENTITY (1000000,1),"
			I = StringPosition (CreateTableStatement, "1000000");
			if (I < 0) ReportError ("Update SQL", "Identity not found");

			QPsprintf_s (Seed, "%7d", FirstLotID);

			//Move in current first lot ID 
			for (J = 0; J <= 6; J++)
			{
				CreateTableStatement[I+J] = Seed[J];
			}

			CreateTable ();

			GenerateCreateStatement (); //Restore proper statement
		}

		if (FirstLotID > 0)
		{
			//Insert Production items until we get the proper Lot ID
			do
			{
				NewLotID = InsertProductionRecord ();
				//Leave the Start time NULL because we don't know what it should be

				if (NewLotID > ProductionItem.LotID)
				{
					ReportError ("Update SQL", "New LotID too big %d %d", NewLotID, ProductionItem.LotID);
					RemoteTable.CloseStatusBox ();
					return;
				}
			} while (NewLotID != ProductionItem.LotID);

			OpenStatementHandle ();

			if (!StringEqual (ProductionItem.StartTimeText, "") &&
					!StringEqual (ProductionItem.FinishTimeText, ""))
			{
				QPsprintf_s (Statement, "Update Production "
					"Set [Start Time] = '%s', [Start Day] = '%d', [Start Shift] = '%d', [Finish Time] = '%s', [Finish Day] = '%d', [Finish Shift] = '%d', "
					"[Bar] = '%d', [Part Number] = '%s', [Status] = '%s', [Pieces per Lot] = '%d', [Missing pieces in] = '%d', [Exceptions] = '%s', [Cycle Status] = '%s' "
					"Where [Lot ID] = %d",
					ProductionItem.StartTimeText,
					ProductionItem.StartDay,
					ProductionItem.StartShift,
					ProductionItem.FinishTimeText,
					ProductionItem.FinishDay,
					ProductionItem.FinishShift,
					ProductionItem.BarNumber,
					ProductionItem.PartNumber,
					ProductionItem.Status,
					ProductionItem.PiecesPerLot,
					ProductionItem.MissingPiecesIn,
					ProductionItem.Exceptions,
					ProductionItem.CycleStatus,
					ProductionItem.LotID);
			}
			else if (!StringEqual (ProductionItem.StartTimeText, "") &&
							 !StringEqual (ProductionItem.FinishTimeText, ""))
			{
				QPsprintf_s (Statement, "Update Production "
					"Set [Start Time] = '%s', [Start Day] = '%d', [Start Shift] = '%d', [Finish Time] = '%s', [Finish Day] = '%d', [Finish Shift] = '%d', "
					"[Bar] = '%d', [Part Number] = '%s', [Status] = '%s', [Pieces per Lot] = '%d', [Missing pieces in] = '%d', [Exceptions] = '%s', [Cycle Status] = '%s' "
					"Where [Lot ID] = %d",
					ProductionItem.StartTimeText,
					ProductionItem.StartDay,
					ProductionItem.StartShift,
					ProductionItem.FinishTimeText,
					ProductionItem.FinishDay,
					ProductionItem.FinishShift,
					ProductionItem.BarNumber,
					ProductionItem.PartNumber,
					ProductionItem.Status,
					ProductionItem.PiecesPerLot,
					ProductionItem.MissingPiecesIn,
					ProductionItem.Exceptions,
					ProductionItem.CycleStatus,
					ProductionItem.LotID);
			}
			else if (!StringEqual (ProductionItem.StartTimeText, ""))
			{
				//Leave finish items NULL and Status NULL so the Production query won't get inspect records.
				QPsprintf_s (Statement, "Update Production "
					"Set [Start Time] = '%s', [Start Day] = '%d', [Start Shift] = '%d', "
					"[Bar] = '%d', [Part Number] = '%s', [Pieces per Lot] = '%d', [Missing pieces in] = '%d', [Cycle Status] = '%s' "
					"Where [Lot ID] = %d",
					ProductionItem.StartTimeText,
					ProductionItem.StartDay,
					ProductionItem.StartShift,
					ProductionItem.BarNumber,
					ProductionItem.PartNumber,
					ProductionItem.PiecesPerLot,
					ProductionItem.MissingPiecesIn,
					ProductionItem.CycleStatus,
					ProductionItem.LotID);
			}
			//Paint lines can have scheduled records have both times NULL but there should be a part number
			else if (!StringEqual (ProductionItem.PartNumber, ""))
			{
				//These records hold the Part Number for the Chain table
				QPsprintf_s (Statement, "Update Production "
					"Set [Part Number] = '%s' "
					"Where [Lot ID] = %d",
					ProductionItem.PartNumber,
					ProductionItem.LotID);
			}

			if (!StringEqual (Statement, ""))
			{
				if (!ExecuteStatement ()) RemoteTable.AbortQuery = true;
			}

			CloseStatementHandle ();
		}
		//Zero this every time so null values will come in right
		ZeroMemory (&ProductionItem, sizeof (PRODUCTION_ITEM));
	}

	//Don't allow inserting into the Identity column Lot ID
	//OpenStatementHandle ();
	//QPsprintf_s (Statement, "Set IDENTITY_INSERT Production OFF");
	//ExecuteStatement ();
	//CloseStatementHandle ();


	//Read Defect records
	strcpy_s (RemoteTable.TableName, "Defects");
	Initialize (RemoteTable.TableName);

	QPsprintf_s (RemoteTable.StatusBoxTitle, "Reading %s table", RemoteTable.TableName);
	SetWindowText (RemoteTable.StatusBox, RemoteTable.StatusBoxTitle);

	//Delete all records that will have duplicate LotID's
	//OpenStatementHandle ();
  //
	//QPsprintf_s (PCHAR (&Statement),
	//	"Delete from Defects Where [Lot ID] >= '%d'",
	//	FirstLotID);
  //
	//ExecuteStatement ();

	//We dropped the Production table so drop the defects too
	DropTable ();
	CreateTable ();

	//If a line has been down for more than a few week we may not get any production records so avoid reading all the defect records starting with Lot ID 0
	if (FirstLotID > 0)
	{
		RemoteTable.StartQuery ();

		QPsprintf_s (RemoteTable.Statement,
			"Select [Lot ID], [Name], [Pieces], [Scrap Type], [Corporate Code] From [Defects] "
			"Where [Lot ID] >= '%d' order by [Lot ID]",
			FirstLotID);

		if (!RemoteTable.ExecuteStatement ()) return;

		//Inspect can be null for Paint lines
		RemoteTable.BindColumn (SQL_INTEGER,	&DefectItem.LotID,						sizeof (DefectItem.LotID));
		RemoteTable.BindColumn (SQL_CHAR,			&DefectItem.Name,							sizeof (DefectItem.Name));
		RemoteTable.BindColumn (SQL_INTEGER,	&DefectItem.Pieces,						sizeof (DefectItem.Pieces));
		RemoteTable.BindColumn (SQL_CHAR,			&DefectItem.ScrapType,				sizeof (DefectItem.ScrapType));
		RemoteTable.BindColumn (SQL_CHAR,			&DefectItem.CorporateCode,		sizeof (DefectItem.CorporateCode));

		ZeroMemory (&DefectItem, sizeof (DEFECT_ITEM));

		FirstLotID = 0;

		OpenStatementHandle ();

		//Copy record
		while (RemoteTable.ReadRecord ())
		{
			QPsprintf_s (Statement,
				"Insert Defects\n"
				"([Lot ID], [Name], [Pieces], [Scrap Type], [Corporate Code])\n"
				"Values ('%d', '%s', '%d', '%s', '%s')",
				DefectItem.LotID,
				DefectItem.Name,
				DefectItem.Pieces,
				DefectItem.ScrapType,
				DefectItem.CorporateCode);

			if (!ExecuteStatement ()) RemoteTable.AbortQuery = true;

			//Zero this every time so null values will come in right
			ZeroMemory (&DefectItem, sizeof (DEFECT_ITEM));
		}

		CloseStatementHandle ();
	}
}

//----------------------------------------------------------------------------------------------
void UPDATE_SQL::UpdateChain (void)
{
	//These just use a string form of the enumerated types which eliminates any error for null
	//This may be a problem because the destination will be blank not NULL, probably nut because reading a null gives blank
	CHAIN_OBJECT		ChainItem;
	DATE_TIME				StartTime;
	DATE_TIME				WeekOfDate;
	char						WeekOfDateString[DATE_TIME_SIZE];

	strcpy_s (RemoteTable.TableName, "Chain");
	Initialize (RemoteTable.TableName);

	//Generate Chain create statement and copy it to our table
	ChainList.DefineFields();
	strcpy_s(CreateTableStatement, ChainList.DataTable.CreateTableStatement);

	QPsprintf_s (RemoteTable.StatusBoxTitle, "Reading %s table", RemoteTable.TableName);
	SetWindowText (RemoteTable.StatusBox, RemoteTable.StatusBoxTitle);

	time (&StartTime);

	StartTime = StartTime - (SECONDS_PER_WEEK * WEEKS_TO_TRANSFER);

	StartTime = PriorSunday (StartTime);

	FormatWeekOfDate (WeekOfDateString, sizeof (WeekOfDateString), StartTime);

	//We dropped the Production table so drop the chain too
	DropTable ();
	CreateTable ();

	//Read Chain records
	RemoteTable.StartQuery ();

	QPsprintf_s (RemoteTable.Statement,
		"Select [Week of Date], [Round], [First Hook], [Last Hook], [Gap type], [Required Gap], [Lot ID], "
		"[Color Number], [Extra Gap], [Tool Number] From Chain "
		"Where [Week of Date] >= '%s' order by [Round], [First Hook]",
		WeekOfDateString);

	if (!RemoteTable.ExecuteStatement ()) return;

	RemoteTable.BindColumn (SQL_DATETIME,	&WeekOfDate,										sizeof (WeekOfDate));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.Round,								sizeof (ChainItem.Round));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.FirstHook,						sizeof (ChainItem.FirstHook));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.LastHook,						sizeof (ChainItem.LastHook));
	RemoteTable.BindColumn (SQL_CHAR,			&ChainItem.GapType,							sizeof (ChainItem.GapType), PCHAR (&GapTypeText), sizeof (GapTypeText), MAX_GAP_TYPE);
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.RequiredGap,					sizeof (ChainItem.RequiredGap));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.LotID,								sizeof (ChainItem.LotID));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.ColorNumber,					sizeof (ChainItem.ColorNumber));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.ExtraGap,						sizeof (ChainItem.ExtraGap));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.Tool.Number,					sizeof (ChainItem.Tool.Number));

	ZeroMemory (&ChainItem, sizeof (CHAIN_OBJECT));

	//Copy record
	while (RemoteTable.ReadRecord ())
	{
		FormatDateTime (WeekOfDateString, sizeof (WeekOfDateString), WeekOfDate);

		StartInsert ();
		InsertItem ("Week of Date",	WeekOfDateString);
		InsertItem ("Round",				ChainItem.Round);
		InsertItem ("First Hook",		ChainItem.FirstHook);
		InsertItem ("Last Hook",		ChainItem.LastHook);
		InsertItem ("Gap Type",			ChainItem.GapType, PCHAR (&GapTypeText), sizeof (GapTypeText), MAX_GAP_TYPE);
		InsertItem ("Lot ID",				ChainItem.LotID);
		InsertItem ("Color Number",	ChainItem.ColorNumber);
		InsertItem ("Required Gap",	ChainItem.RequiredGap);
		InsertItem ("Extra Gap",		ChainItem.ExtraGap);
		InsertItem ("Tool Number",	ChainItem.Tool.Number);
		DoInsert ();

		//Zero this every time so null values will come in right
		ZeroMemory (&ChainItem, sizeof (CHAIN_OBJECT));
	}
}

//----------------------------------------------------------------------------------------------
void UPDATE_SQL::ConvertChainTable (void)
{
	CHAIN_OBJECT		ChainItem;

	RemoteTable.Initialize (ComputerName, CustomerName, "Chain");
	Initialize (ComputerName, CustomerName, "Chain");

	//Read Chain records
	RemoteTable.StartQuery ();

	QPsprintf_s (RemoteTable.Statement,
		"SELECT Chain.[Lot ID], Production.[Lot ID], Production.[Part Number], Part.[Color Number]\n"
		"FROM [Chain], [Production], [Part]\n"
		"Where Chain.[Lot ID] = Production.[Lot ID] and\n"
		"Production.[Part Number] = Part.Number");

	if (!RemoteTable.ExecuteStatement ()) return;

	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.LotID,								sizeof (ChainItem.LotID));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.LotID,								sizeof (ChainItem.LotID));
	RemoteTable.BindColumn (SQL_CHAR,			&ChainItem.Part.Number,					sizeof (ChainItem.Part.Number));
	RemoteTable.BindColumn (SQL_INTEGER,	&ChainItem.ColorNumber,					sizeof (ChainItem.ColorNumber));

	ZeroMemory (&ChainItem, sizeof (CHAIN_OBJECT));

	SetWaitCursor ();

	//Update record
	while (RemoteTable.ReadRecord ())
	{
		OpenStatementHandle ();

		QPsprintf_s (Statement, "Update [Chain] Set [Color Number] = '%d' Where [Lot ID] = '%d'",
			ChainItem.ColorNumber,
			ChainItem.LotID);

		if (!ExecuteStatement ()) RemoteTable.AbortQuery = true;

		CloseStatementHandle ();

		//Zero this every time so null values will come in right
		ZeroMemory (&ChainItem, sizeof (CHAIN_OBJECT));
	}

	SetArrowCursor ();
}