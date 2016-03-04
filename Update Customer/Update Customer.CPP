//---------------------------------------------------------------------------
//	Update Customer program                                                              
//	Copyright (C) 1995 Mahood and Company.
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
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//The icons are one large UC letter and one small UC letter if the program name is two words.  If the program is one word the second letter is LC and smaller if it looks too big.
//The font is Dutch 801 bold
//Microsoft says to have a 48x48 icon for the task bar and a 32x32 for the program title bar.
//The 48x48 first letter is 28 and the second is 22-16, whatever looks best.
//The 32x32 first letter is 22 and the second is 12.
//---------------------------------------------------------------------------


#include "Update Customer.h"
#include "resource.h" //Menu ID's
#include "Choose.h"
#include "Application.h"
#include "Batch File.h"

#include "File List.h"

#include "Service.h"
#include "Window Size.h"
#include "Column Hide.h"
#include "In Plant.h"
#include "Dialog Data.h"
#include "Customer.h"
#include "Cleanup Tabs.h"
#include "Update SQL.h"
#include "\Users\Public\Documents\Quick Plate\Trend\Movement Problem.h"
#include "\Users\Public\Documents\Quick Plate\Trend\Production Query.h"
#include "Timeway Steps.h"
#include "Inventory.h"
#include "Contact.h"
#include "Last Modified.h"


//for email
#include "/ChilKat Library/chilkat-9.5.0-x86_64-vc2015/include/CkMailMan.h"
#include "/ChilKat Library/chilkat-9.5.0-x86_64-vc2015/include/CkEmail.h"

//for mkdir
#include "direct.h"
#include "Userenv.h"

//firewall file no where
//#include "NetFW.h"

//For power schemes
extern "C"
{
	#include <powrprof.h>
}

#define	CLOSE_SERVICE_LIST				true
#define	DO_NOT_CLOSE_SERVICE_LIST	false

//---------------------------------------------------------------------------
class CUSTOM_APPLICATION: public APPLICATION
{
	public: 
	int					OrigionalX;
	int					OrigionalY;
	bool				FirstTime;
	TIME_ONLY		Seconds;
	char				Buffer[65536];
	BATCH_FILE	BatchFile;

	char	PublishCustomerName[COMPUTER_NAME_SIZE];

	virtual void	CreateRequiredDirectories (void);
					bool  InitializeDatabase				(void);
					void	Relaunch									(void);
	virtual void	InitializeDisplayWindows	(void);

	virtual	WNDPROC	WindowsProcedure			(HWND						hWndFrame,
																				UINT						wMsg,
																				WPARAM					wParam,
																				LPARAM					lParam);

 	virtual bool	CommandProc					 			(WPARAM	wParam,
																					LPARAM	lParam);

	virtual bool	CanClose									(void);
	virtual bool	SaveAll										(void);

					void	AdjustMenu								(void);

					void	SetKeyValue								(HKEY hKey,
																					PCHAR KeyName,
																					DWORD	ValueType,
																					PCHAR KeyValue);

					void	DeleteLogonSplashScreen		(void);
					bool	EnableAutoSignIn					(void);
					void	DisableAutoSignIn					(void);
					void	InstallStartMenu					(void);
					void	CopyShortcuts							(void);
					void	EraseCachedIcons					(void);
					void	SetClocks									(bool		Pause);
					void	EnableC$									(void);
					void	ChangePowerPlanToRun			(void);
					void	ChangePowerPlanToSimulate (void);
					void	TurnOffScreenSavers				(void);
					void	AdjustUserAccessControl		(void);
					void	ReservePorts							(void);
					void	TrimBigTables							(void);
					void	SetCustomerInRegistry			(void);
					void	AddPlatingToPath					(void);
					void	StressMemory							(void);
};

typedef CUSTOM_APPLICATION *pCUSTOM_APPLICATION;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void StressMemoryThread (pCUSTOM_APPLICATION	pCustomApplication)
{
	pAPPLICATION	Application;

	Application = pAPPLICATION (GetApplicationPointer ());

	pCustomApplication->StressMemory ();

	Application->ThreadList.DoExitThread (0);
}

//---------------------------------------------------------------------------
class CUSTOM_SERVICE_LIST: public SERVICE_LIST
{
	public:
	BATCH_FILE	BatchFile;
	int					UpdateSelection;
	int					RevertSelection;

	private:
	virtual bool ReadRecords			(void);
	virtual void RightButtonDown	(void);
	virtual int	 PopupMenuCommand	(int		PopupMenuIndex);
};

typedef CUSTOM_SERVICE_LIST *pCUSTOM_SERVICE_LIST;

//---------------------------------------------------------------------------
class CUSTOM_CUSTOMER_LIST: public CUSTOMER_LIST
{
	public:
	int		UpdateSQLSelection;
	int		UpdateSQLWithFillSelection;
	int		UpdateWaitSelection;
	int		PublishSelection;

	bool	UpdateSQLWithFill;

	virtual void WriteRecords				(void);
	virtual void RightButtonDown		(void);
	virtual int	 PopupMenuCommand		(int		PopupMenuIndex);
};

typedef CUSTOM_CUSTOMER_LIST *pCUSTOM_CUSTOMER_LIST;


//Application object, top level window and menu support
static CUSTOM_APPLICATION	Application;


char		FrameTitle[80];

//Display objects
FILE_LIST							HiddenFileList; //Used by Computer List
FILE_LIST							FileList;
CUSTOM_SERVICE_LIST		CustomServiceList; //This name to avoid confusion with ServiceList in Application
CUSTOM_CUSTOMER_LIST	CustomerList;

WINDOW_SIZE_LIST				WindowSizeList;
COLUMN_HIDE_LIST				ColumnHideList;
DIALOG_DATA_LIST				DialogDataList;

UPDATE_SQL									UpdateSQL;
FILED_MOVEMENT_PROBLEM_LIST	MovementProblemList;
MOVE_TIME_LIST							MoveTimeList;
PRODUCTION_QUERY						ProductionQuery;

INVENTORY_LIST			InventoryList;
CONTACT_LIST				ContactList;
LAST_MODIFIED_LIST	LastModifiedList;

SQL_TABLE						TestTable;

//Objects that can read the big tables for Trimming
SQL_TABLE						BathHistoryTable;
SQL_TABLE						EnvironmentalHistoryTable;
MOVER_LIST					MoverTable;
SQL_TABLE						WaitTable;

//  Create a new email object
CkMailMan mailman;
CkEmail		email;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void RemoteUpdateThread (pCUSTOM_APPLICATION	CustomApplication)
{
	int						I;
	bool					TransferError;

	//Wait for the transfer to finish.  This keeps us from reading our queue which stops the Copy Status from updating unless done in another thread
	while (FileList.Transfering) SleepSeconds (0.5); //Wait until files are transfered

	TransferError = false;

	for (I = 0; I <= FileList.FileMax; I++)
	{
		if (FileList.File[I].CopyStatus != Copy_OK) TransferError = true;
	}

	//We must do this for remote update to restart everything.
	//Don't auto start in simulation it is usually not desired because one must do an update SQL tables too.
	//if (!Features.SimulateIO)
	//{
	//	CustomServiceList.StartAll ();
	//}
	//Even for simulation, it is rare that they need to update SQL.  If we don't restart one has to remote control the console to start everything.
	//Don't restart everything if we are on a terminal session though, only on the console should things restart.
	if (StringEqual (TerminalName, "")) CustomServiceList.StartAll ();

	PostMessage (FrameWindow, WM_COMMAND, IDM_STOP_SERVICE, DO_NOT_ASK_TO_SAVE_DATA); //Stop without asking to save data

	strcpy_s (RemoteSQLIPAddress, "");

	CustomApplication->ThreadList.DoExitThread (0);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma warning(suppress: 6262)
void TestSQLLibraryThread (pCUSTOM_APPLICATION	CustomApplication)
{
	CustomApplication = CustomApplication;

	SQL_TABLE	LastModifiedTable;
	SQL_TABLE	GeneralProblemTable;
	SQL_TABLE	UsageTable;
	SQL_TABLE	ChangeTable;

	LastModifiedTable.Initialize ("Last Modified");
	GeneralProblemTable.Initialize ("General Problem");
	UsageTable.Initialize ("Usage");
	ChangeTable.Initialize ("Change");
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void TestSQLOpenThread (pCUSTOM_APPLICATION	pCustomApplication)
{
	DATE_TIME			PriorTime;
	double				StartTime;
	double				Slowest;
	double				SlowestSlow;
	int						Opens;
	int						Slows;
	int						Fails;
	char					Message[64];
	double				DelayTime;
	double				TimeoutTime;

	Opens = 0;
	Slows = 0;
	Fails = 0;
	Slowest			= 0.0;
	SlowestSlow = 0.0;

	time (&PriorTime);

	TimeoutTime = 30;
	DelayTime = TimeoutTime - 5;

	TestTable.Initialize ("Test table");

	do
	{
	 	GetDoubleTime (&StartTime);

		QPsprintf_s (Message, "Opens %d; Slows %d; Fails %d; Slowest %.3f - %.3f", Opens, Slows, Fails, Slowest, SlowestSlow);

		if (!SetDlgItemText (TestTable.StatusBox, IDC_LOG_READING, Message)) SimulationReportLastError ("SQL Table", "SetDlgItemText");

		//Sweep around the timeout time
		SleepSeconds (DelayTime);

		DelayTime = DelayTime + 0.016;
		if (DelayTime > TimeoutTime + 30) DelayTime = TimeoutTime - 30;

	} while (!TestTable.AbortQuery);

	TestTable.CloseStatusBox ();

	SetArrowCursor ();

	ReportError ("Update Customer", "Opens %d Slows %d Fails %d Slowest %.3f - %.3f", Opens, Slows, Fails, Slowest, SlowestSlow);

	TestTable.DropTable ();

	pCustomApplication->ThreadList.DoExitThread (0);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void TestStringClass (pCUSTOM_APPLICATION	CustomApplication,
											PCHAR								AString)
{
	int			J;
	int			I;
	char		Text[14];
	char		Txt[4];

	CustomApplication = CustomApplication;

	I = 5;
	J = 0;

	strcpy_s (Text, sizeof (Text), "Ab");

	strcpy_s (Txt, "abc");
	//strcpy_s (Txt, "abcd");
	//strcpy_s (Txt, "abcde");

	//strcpy_s (Text, sizeof (Text), "this");
	//QPsprintf_s (Text, sizeof (Text), "%s", I);
	QPsprintf_s (Text, sizeof (Text), "%s", "abcd");
	QPsprintf_s (Text, "%s", "abcd");

	strcpy_s (Txt, "abcd"); //This will fault
	QPsprintf_s (AString, sizeof (AString), "%s", "AB12345"); //This will fault because sizeof pointer is 4
	//QPsprintf_s (AString, "%s", "AB"); This won't compile which will catch mistakes
	//strcpy_s (AString, "AB"); This too won't compile

	strcpy_s (Text, "AB");

	QPsprintf_s (Text, "%s", "abcd");
	QPsprintf_s (Text, "%s", YesNoText[0]);
	//QPsprintf_s (Text, "%s", I); //This will fault

	ReportError ("Update Customer", "Test String class");
}

//---------------------------------------------------------------------------
// WINDOWS MAIN PROCEDURE
// This is the process entry-point routine.  This is the basis for all
// application events.
//---------------------------------------------------------------------------
int PASCAL WinMain (_In_ HINSTANCE hInstance,
										_In_opt_ HINSTANCE hPrevInstance,
										_In_ LPSTR lpCmdLine,
										_In_ int nShowCmd)
{
	return Application.WinMain (hInstance,
															hPrevInstance,
															lpCmdLine,
															nShowCmd,
															"Update Customer",
															"1995");
}

//---------------------------------------------------------------------------------------
#pragma warning(suppress: 6262)
bool CUSTOM_SERVICE_LIST::ReadRecords (void)
{
	int								I;
	SERVICE_ITEM			NewService;
	int								Index;
	bool							Found;
	bool							Exclude;
	bool							Server;

	//Read file with old selections
	if (!SERVICE_LIST::ReadRecords ()) return false;

	//Don't do everything else if we are doing Start Missing Services or an update
	if (!Application.UserIsAdministrator ()) return true;

	HiddenFileList.CreateList (Local_services_list);

	if (HiddenFileList.FileMax < 0) return false;

	for (I = 0; I <= HiddenFileList.FileMax; I++)
	{
		ZeroMemory (&NewService, sizeof (NewService));

		strcpy_s (NewService.Name, HiddenFileList.File[I].FileName);

		strcpy_s (NewService.FileName,			HiddenFileList.File[I].FileName);
		strcpy_s (NewService.DirectoryName, HiddenFileList.File[I].LocalDirInfo.Path);

		//Only EXE files.  It is possible for the new file to exist, do not put it into the list
		if (StringInString(NewService.FileName, ".EXE") &&
				!StringInString(NewService.FileName, ".EXE.lastcodeanalysissucceeded") && //Code Ananysis creates these 0 byte files 
				!StringInString(NewService.FileName, OLD_FILE_TAG)) //Prior version for right mouse revert
		{
			NewService.Name[StringPosition (NewService.Name, ".EXE")] = 0; //remove EXE

			//Only allow known service programs
			if (StringInString(NewService.Name, "Show Line") ||
					StringInString(NewService.Name, "Snap") ||
					StringInString(NewService.Name, "Quick Hoist Snap") ||
					StringInString(NewService.Name, "Digital Motion") ||
					StringInString(NewService.Name, "Rectifier") ||
					StringInString(NewService.Name, "Bath") ||
					StringInString(NewService.Name, "Timeway") ||
					StringInString(NewService.Name, "Color")) //For Anodize
			{
				Server	= true;
				Exclude = false;
			}
			else if (StringInString(NewService.Name, "Define") ||
							 StringInString(NewService.Name, "Update Customer") ||
							 StringInString(NewService.Name, "Trend") ||
							 StringInString(NewService.Name, "Scrap") ||
							 StringInString(NewService.Name, "Inspect") ||
							 StringInString(NewService.Name, "Portable") ||
							 StringInString(NewService.Name, "Operator") ||
							 StringInString(NewService.Name, "Paint Schedule")) //Paint lines
			{
				Server	= false;
				Exclude = false;
			}
			else
			{
				Server = false;
				Exclude = true;
			}

			if (!Features.UseDigitalMotion)
			{
				if (StringInString (NewService.Name, "Digital Motion")) Exclude = true;
			}

			//There is only a pseudo Timeway program to manage things with the interface to the chain and robot control.
			if (Features.CustomerType == Painting_customer)
			{
				if (Server &&
						!StringEqual (NewService.Name, "Timeway")) Exclude = true;
			}

			if (!Exclude)
			{
				Index = GetIndex (NewService.Name); //On any computer

				if (Index < 0)
				{
					if (ServiceMax > MAX_SERVICE)
					{
					}
					else ServiceMax++;

					ZeroMemory (&Service[ServiceMax], sizeof (Service[ServiceMax]));

					Index = ServiceMax;

					if (Server) SetSQLRecordNew (ServiceMax, true);
				}

				if (!StringEqual (Service[Index].FileName,			NewService.FileName) ||
						!StringEqual (Service[Index].DirectoryName,	NewService.DirectoryName))
				{
					if (Server) SetSQLRecordModified (Index, true);
				}

				strcpy_s (Service[Index].Name,					NewService.Name);
				strcpy_s (Service[Index].FileName,			NewService.FileName);
				strcpy_s (Service[Index].DirectoryName,	NewService.DirectoryName);
			}
			else //A Non service EXE
			{
				Index = GetIndex (NewService.Name);

				if (Index >= 0)
				{
					SetSQLRecordDelete (Index, true);
				}
			}
		}
	}

	//The list is sorted by directory then file
	//We need a sorted list by just file name so blank the directory name
	for (I = 0; I <= HiddenFileList.FileMax; I++)
	{
		strcpy_s (HiddenFileList.File[I].LocalDirInfo.Path, "");
	}

	HiddenFileList.Sort (HiddenFileList.LocalPathField);

	for (I = 0; I <= ServiceMax; I++)
	{
		//Clean up recently obsoleted programs
		if (StringEqual (Service[I].Name, "Remote Applications"))
		{
			SetSQLRecordDelete (I, true);
		}

		ZeroMemory (&NewService, sizeof (NewService));

		strcpy_s (NewService.Name, Service[I].Name);
		strcat_s (NewService.Name, ".EXE");

		Lookup (NewService.Name,
						PCHAR (&HiddenFileList.File[0].FileName),
						HiddenFileList.FileMax,
						sizeof (HiddenFileList.File[0]),
						&Index,
						&Found);

		if (!Found)
		{
			//Delete entry from service list if the file was deleted
			//Don't delete the service if we are doing a build and open Update Customer
			//SetSQLRecordDelete (I, true);
		}
	}

	AssignStartupOrder ();

	HiddenFileList.CloseConnections ();

	return true;
}

//-------------------------------------------------------------------------------------------------------- 
void CUSTOM_SERVICE_LIST::RightButtonDown (void)
{
	UpdateSelection = DefinePopupMenu	(NULL, 0, "Update %s", Service[RecordIndex].Name);
	DefinePopupMenu (NULL, 0, "MF_SEPERATOR");
	RevertSelection = DefinePopupMenu	(NULL, 0, "Revert %s", Service[RecordIndex].Name);

	SERVICE_LIST::RightButtonDown ();
}

//--------------------------------------------------------------------------------------------- 
int CUSTOM_SERVICE_LIST::PopupMenuCommand (int		PopupMenuIndex)
{
	int				Index;
	char			PublishServer[MAX_PATH];
	char			ServerPath[MAX_PATH];
	char			LocalPath[MAX_PATH];
	char			LocalName[MAX_PATH];
	char			OldLocalName[MAX_PATH];
	DISK_FILE	OldFile;

	//Generate paths and file names
	if (StringInString (Service[PopupMenuRecord].DirectoryName, "Plating"))
	{
		QPsprintf_s (LocalPath, "%s", "/Program Files/Quick Plate/Plating/");
		QPsprintf_s (ServerPath, "%s", "/Program Files/Quick Plate/Publish/Plating/");
	}
	else
	{
		QPsprintf_s (LocalPath, "%s%s/",	"/Program Files/Quick Plate/",					CustomerName);
		QPsprintf_s (ServerPath, "%s%s/", "/Program Files/Quick Plate/Publish/",	CustomerName);
	}

	QPsprintf_s (LocalName, "%s%s", LocalPath, Service[PopupMenuRecord].FileName);
	QPsprintf_s (OldLocalName, "%s%s", LocalName, " OLD");


	if (PopupMenuIndex == UpdateSelection)
	{
		if (StringEqual (TerminalName, ""))
		{
			Index = CustomerList.GetIndex (CustomerName);
			Index = CustomerList.GetIndex (CustomerList.Customer[Index].PublishServer);
			strcpy_s (PublishServer, CustomerList.Customer[Index].RemoteSQLIPAddress);	

			CustomServiceList.StopService (Service[PopupMenuRecord].Name);


			//Logon to remote computer for file SQL access.
			SetRemoteCredentials (CustomerList.Customer[Index].UserName, CustomerList.Customer[Index].LineComputerName, CustomerList.Customer[Index].Password);
			strcpy_s (FileList.RemoteCustomerName, CustomerList.Customer[Index].RemoteSQLIPAddress); //So OpenConnections will use the above credentials

			if (!FileList.OpenConnections (IPAddress, PublishServer)) //Will use remote credentials if RemoteCustomerName isn't blank
			{
				ReportError ("Update Customer", "Remote connection didn't open");
			}
			else
			{

				//Rename old version for revert
				DeleteFile(OldLocalName); //Delete any prior old version
				if (rename(LocalName, OldLocalName) != 0)
				{
					ReportError("Update Customer", "Rename current file failed %s %s", LocalPath);
				}

				if (!FileList.DownloadFile (LocalPath, ServerPath, Service[PopupMenuRecord].FileName, -1))
				{
					ReportError ("Update Customer", "File didn't update %s %s", LocalPath, ServerPath);
				}
				else FileList.CloseConnections ();

				SetRemoteCredentials ("", "", "");
				strcpy_s (FileList.RemoteCustomerName, "");
			}

			LogGeneralProblem  ("Update Customer", "%s updated.", Service[PopupMenuRecord].Name);

			//Don't restart things like Scrap that arent services
			if (Service[PopupMenuRecord].StartupOrder > 0) CustomServiceList.StartService (PopupMenuRecord, true);
		}
		else ReportError ("Update Customer", "You may only update a program from the console!");
		return 0; //processed
	}
	else if (PopupMenuIndex == RevertSelection)
	{
		if (StringEqual (TerminalName, ""))
		{
			if (!OldFile.OpenToRead(OldLocalName))
			{
				ReportError("Update Customer", "An old version doesn't exist.  Revert not possible!");
			}
			else
			{
				OldFile.Close();

				CustomServiceList.StopService(Service[PopupMenuRecord].Name);

				//Delete new version and rename old version
				DeleteFile(LocalName);
				if (rename(OldLocalName, LocalName) != 0)
				{
					ReportError("Update Customer", "Old file to current file failed %s %s", LocalPath);
				}

				LogGeneralProblem("Update Customer", "%s reverted.", Service[PopupMenuRecord].Name);

				//Don't restart things like Scrap that arent services
				if (Service[PopupMenuRecord].StartupOrder > 0) CustomServiceList.StartService(PopupMenuRecord, true);
			}
		}
		else ReportError ("Update Customer", "You may only revert a program from the console!");
		return 0; //processed
	}
	else return SERVICE_LIST::PopupMenuCommand (PopupMenuIndex);
}

//-------------------------------------------------------------------------------------------------------- 
void CUSTOM_CUSTOMER_LIST::WriteRecords(void)
{
	char	PriorCustomerName[COMPUTER_NAME_SIZE];

	//We get here if anything changed.
	strcpy_s(PriorCustomerName, CustomerName);

	DISPLAY_WINDOW::WriteRecords();

	ReadCustomerName();

	//Did the active customer change?
	if (!StringEqual(PriorCustomerName, CustomerName))
	{
		CustomServiceList.StopAll(); //Will not return until all services stopped

		Application.SetCustomerInRegistry(); //Write to registry for the shortcuts to start custom applications

		Application.CopyShortcuts();

		Application.Relaunch();
	}
}

//-------------------------------------------------------------------------------------------------------- 
void CUSTOM_CUSTOMER_LIST::RightButtonDown (void)
{
	int	CustomerIndex;

	UpdateSQLSelection						= -1;
	UpdateSQLWithFillSelection		= -1;
	PublishSelection							= -1;

	if (!StringInString (Customer[PopupMenuRecord].Name, "Help"))
	{
		UpdateSQLWithFillSelection = DefinePopupMenu	(NULL, 0, "Update SQL with Fill from %s line", CustomerName);
		UpdateSQLSelection = DefinePopupMenu					(NULL, 0, "Update SQL from %s line on the weekend", CustomerName);

		UpdateWaitSelection = DefinePopupMenu	(NULL, 0, "Update Wait table from %s line", CustomerName);

		if (StringInString (ComputerName, "Mahood"))
		{
			//Publish is only used where programs are compiled.
			CustomerIndex = GetIndex(CustomerName);
			PublishSelection = DefinePopupMenu	(NULL, 0, "Publish to %s", Customer[CustomerIndex].PublishServer);
		}
	}

	CUSTOMER_LIST::RightButtonDown ();
}

//--------------------------------------------------------------------------------------------- 
int CUSTOM_CUSTOMER_LIST::PopupMenuCommand (int		PopupMenuIndex)
{
	int	PublishIndex;
	int	CustomerIndex;

	CustomerIndex = GetIndex(CustomerName);

	//Publish goes to the publish server so we need to use those credentials
	if (PopupMenuIndex == PublishSelection)
	{
		PublishIndex = GetIndex (Customer[CustomerIndex].PublishServer);
	}
	else PublishIndex = CustomerIndex;


	if ((PopupMenuIndex == UpdateSQLSelection) ||
			(PopupMenuIndex == UpdateSQLWithFillSelection) ||
			(PopupMenuIndex == UpdateWaitSelection) ||
			(PopupMenuIndex == PublishSelection))
	{
		if (PopupMenuIndex == UpdateSQLWithFillSelection)
					UpdateSQLWithFill = true;
		else	UpdateSQLWithFill = false;

		//Make sure the remote SQL IP isn't our SQL server
		if (StringEqual (CustomerList.Customer[PublishIndex].RemoteSQLIPAddress, ""))
		{
			ReportError ("Update Customer", "Remote SQL IP Address is blank for this customer!");
			return 0;
		}

		//Logon to remote computer for file and SQL access.  Save remote credentials for use by SQL Table and fine open.
		SetRemoteCredentials (Customer[PublishIndex].UserName, Customer[PublishIndex].LineComputerName, Customer[PublishIndex].Password);

		if (StringEqual (RemoteUserName, "") ||
				StringEqual (RemoteComputerName, "") ||
				StringEqual (RemotePassword, ""))
		{
			ReportError ("Update Customer", "You must fill in the User Name, Password, and Line Computer Name!");
			return 0;
		}
	}

	if ((PopupMenuIndex == UpdateSQLSelection) ||
			(PopupMenuIndex == UpdateSQLWithFillSelection))
	{
		strcpy_s (RemoteSQLIPAddress, Customer[CustomerIndex].RemoteSQLIPAddress);

		if (FileList.DoingFileTransfer ()) return 0;
		if (UpdateSQL.DoingTableTransfer ()) return 0;

		//Update Exchange for Paint lines.  We can only update the current customer so this test will work
		if (Features.CustomerType == Painting_customer)
		{
			FileList.SetupDifferentCustomer(Customer[CustomerIndex].RemoteSQLIPAddress, Customer[CustomerIndex].Name);
			SetRemoteCredentials(Customer[CustomerIndex].UserName, Customer[CustomerIndex].LineComputerName, Customer[CustomerIndex].Password);
			FileList.DoCreateWindow(Update_data_from_remote);
		}

		//Transfer SQL tables
		UpdateSQL.StartUpdate (UpdateSQLWithFill);
		return 0;
	}

	if (PopupMenuIndex == UpdateWaitSelection)
	{
		strcpy_s (RemoteSQLIPAddress, Customer[CustomerIndex].RemoteSQLIPAddress);

		if (FileList.DoingFileTransfer ()) return 0;
		if (UpdateSQL.DoingTableTransfer ()) return 0;

		if (!UpdateSQL.WaitList.SQLQueryDialog.ShowDialog ()) return 0;

		//Transfer Wait table for time range
		UpdateSQL.StartWaitUpdate ();
		return 0;
	}
	else if (PopupMenuIndex == PublishSelection)
	{
		strcpy_s (Application.PublishCustomerName, Customer[PublishIndex].Name);

		PostMessage (FrameWindow, WM_COMMAND, ID_PROGRAMS_PUBLISH_CUSTOMER, 0);

		return 0;
	}

	else return CUSTOMER_LIST::PopupMenuCommand (PopupMenuIndex);
}

//---------------------------------------------------------------------------------------
void CUSTOM_APPLICATION::SetKeyValue (HKEY	hKey,
																			PCHAR KeyName,
																			DWORD	ValueType,
																			PCHAR KeyValue)
{
	int		Error;
	int		Value;

	if (ValueType == REG_DWORD)
	{
		if (StringEqual (KeyValue, "0"))
					Value = 0x00;
		else	Value = 0x01;

		Error = RegSetValueEx (hKey, KeyName, NULL, ValueType, LPBYTE (&Value), 4);
	}
	else
	{
		Error = RegSetValueEx (hKey, KeyName, NULL, ValueType, LPBYTE (KeyValue), DWORD (strlen (KeyValue) + 1));
	}

	if (Error != ERROR_SUCCESS)
	{
		SimulationReportLastError ("Update Customer", "RegSetValueEx %d %s %s", Error, KeyName, KeyValue);
	}
}

//---------------------------------------------------------------------------------------
void CUSTOM_APPLICATION::DeleteLogonSplashScreen (void)
{
	HKEY		hKey;
	int			Error;
	DWORD		KeyType;
	DWORD		KeySize;
	char		KeyValue[MAX_PATH];

	if (RegCreateKey(HKEY_USERS, ".DEFAULT\\Control Panel\\Desktop", &hKey) != ERROR_SUCCESS)
	{
		SimulationReportLastError("Update Customer", "RegOpenKeyEx");
		return;
	}

	KeySize = sizeof (KeyValue);

	Error = RegQueryValueEx (hKey, "Wallpaper", NULL, &KeyType, LPBYTE (KeyValue), &KeySize);

	if (Error != ERROR_SUCCESS)
	{
		SimulationReportLastError ("Update Customer", "RegSetValueEx %d", Error);
	}
	else if (!StringEqual (KeyValue, "(none)"))
	{
		ReportError ("Update Customer", "Deleting %s as the logon wallpaper!\nLogon wallpaper causes very slow logons for remote connection!", KeyValue);

		SetKeyValue (hKey, "Wallpaper",	REG_SZ, "(none)");
	}

	RegCloseKey (hKey);
}

//---------------------------------------------------------------------------------------
bool CUSTOM_APPLICATION::EnableAutoSignIn (void)
{
	HKEY	hKey;

	//Make sure the user is the auto logon user before calling this.

	//LsaOpenPolicy and LsaAddAccountRights will allow us to add a group to the Local Security Policies but we must know the name
	//of the group.  If they mess up and don't name the group right or put the users in the group it won't work.
	//We could add the Auto Start and Auto Logon users because we know their names but that isn't the best way

	if (RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon", &hKey) != ERROR_SUCCESS)
	{
		SimulationReportLastError("Update Customer", "RegOpenKeyEx");
		return false;
	}

	//This should work but it gives an access denied error.
	//Status = RegOpenKeyEx (
	//	HKEY_LOCAL_MACHINE,
	//	"Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon",
	//	NULL,
	//	KEY_WOW64_64KEY,
	//	&hKey);

	//if (Status != ERROR_SUCCESS)
	//{
	//	SetLastError (Status);
	//	SimulationReportLastError ("Update Customer", "RegOpenKeyEx");
	//}

	SetKeyValue(hKey, "AutoAdminLogon",			REG_SZ, "1");
	SetKeyValue(hKey, "DefaultDomainName",	REG_SZ, ComputerName);
	SetKeyValue(hKey, "DefaultUserName",		REG_SZ, AUTO_SIGN_IN_USER);
	SetKeyValue(hKey, "DefaultPassword",		REG_SZ, AUTO_SIGN_IN_PASSWORD);

	RegCloseKey (hKey);

	return true;
}

//---------------------------------------------------------------------------------------
void CUSTOM_APPLICATION::DisableAutoSignIn (void)
{
	HKEY	hKey;

	if (!StringEqual (AUTO_SIGN_IN_USER, UserName))
	{
		ReportError ("Update Customer", "You must be logged on as the Auto Logon User!");
		return;
	}

	if (RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon", &hKey) != ERROR_SUCCESS)
	{
		SimulationReportLastError("Update Customer", "RegOpenKeyEx");
		return;
	}

	SetKeyValue (hKey, "AutoAdminLogon", REG_SZ, "0");

	//Leave these in or it will change the default user to DefaultUserName to log onto the server
	//SetKeyValue (hKey, "DefaultDomainName",	"");
	//SetKeyValue (hKey, "DefaultUserName",		"");
	//SetKeyValue (hKey, "DefaultPassword",		"");

	RegCloseKey (hKey);

	if (StringEqual (AUTO_SIGN_IN_USER, ""))
	{
		ReportError ("Update Customer", "You have not entered an Auto Logon User in Configuration!\nAuto Logon may still be enabled for another user.");
		return;
	}

	BatchFile.Open ();
	BatchFile.Writeln ("ERASE /Q /S \"%%USERPROFILE%%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\Start Missing Programs.lnk\""); //The old one
	BatchFile.Writeln ("ERASE /Q /S \"%%USERPROFILE%%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\Start Missing Programs.bat\"");
	BatchFile.Run ();

	Relaunch ();
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::InstallStartMenu (void)
{
	SetWaitCursor ();

	if (_chdir ("\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs") < 0) return;

	MakeDirectory ("Quick Plate");

	EraseCachedIcons ();
	CopyShortcuts ();

	ReportError ("Update Customer",
		"Everyone must search the start menu for the shortcuts they want.\n"
		"Then pin them to their start menu or task bar.");

	SetArrowCursor ();
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::CopyShortcuts (void)
{
	if (!Application.UserIsAdministrator ()) return;

	BatchFile.Open ();
	BatchFile.Writeln ("XCOPY.EXE /Y \"\\Program Files\\Quick Plate\\Plating\\*.lnk\" \"\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Quick Plate\\*.*\"");
	BatchFile.Writeln ("XCOPY.EXE /Y \"\\Program Files\\Quick Plate\\%s\\*.lnk\" \"\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Quick Plate\\*.*\"", CustomerName);
	BatchFile.Run ();
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::EraseCachedIcons (void)
{
	if (!Application.UserIsAdministrator ()) return;

	BatchFile.Open ();
	BatchFile.Writeln ("cd /users");
	BatchFile.Writeln ("erase IconCache.db /ah /s /q"); //Erase all the cached icons in case some change in the shortcuts or programs
	BatchFile.Run ();
}
//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::SetClocks (bool	Pause)
{
	HKEY	hKey; 
	LONG	lRet;
	
	if (Pause) 
	{
		//Only do this when run manually.  With a single server this probably isn't needed.
		lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\services\\w32time\\TimeProviders\\NTPServer", 0, KEY_SET_VALUE, &hKey); 

		if (lRet != ERROR_SUCCESS)
		{
			ReportError ("Update Customer", "Open key failed");
			return;
		}
		SetKeyValue (hKey, "Enabled",				REG_DWORD, "1");
		//SetKeyValue (hKey, "EventLogFlags", REG_DWORD, "1");
		//SetKeyValue (hKey, "InputProvider", REG_DWORD, "1");

		RegCloseKey (hKey); 
	}
		 
	//Point to the Navy
	BatchFile.Open ();
	BatchFile.Writeln ("cd /"); //to root so the commands don't have a long path on them
	BatchFile.Writeln ("net start w32time"); //The service isn't started but starts when triggered.  Nothing seems to trigger it.
	BatchFile.Writeln ("w32tm /config /manualpeerlist:192.5.41.209,192.5.41.41 /syncfromflags:manual /update"); //United States Naval Observatory SNTP time servers

	//This may take some time
	if (Pause) 
	{
		BatchFile.Writeln ("w32tm /resync"); //Resync clock, this must be on a seperate line
		BatchFile.Writeln ("w32tm /query /status");
		BatchFile.Writeln ("w32tm /query /source");
		BatchFile.Writeln ("rem Source must be 192.5.41.209,192.5.41.41 not your computer IP!");
		BatchFile.Writeln ("Pause");
	}
	else BatchFile.Writeln ("w32tm /resync /nowait"); //Resync clock, this must be on a seperate line

	BatchFile.Run ();

	//Shut off Toredo and isatap drivers for tunneled IPv6
	BatchFile.Open();
	BatchFile.Writeln("netsh interface teredo set state disabled");
	BatchFile.Writeln("netsh interface isatap set state disabled");
	BatchFile.Writeln("netsh interface isatap set state disabled");
	BatchFile.Run();

	SetArrowCursor();
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::EnableC$(void)
{
	HKEY	hKey; 
	LONG	lRet;
	
	lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\system\\", 0, KEY_SET_VALUE, &hKey); 

	if (lRet != ERROR_SUCCESS)
	{
		ReportError ("Update Customer", "Open key failed");
		return;
	}

	//This will enable the C$ administrative share
	SetKeyValue (hKey, "LocalAccountTokenFilterPolicy", REG_DWORD, "1");

	RegCloseKey (hKey); 
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::ChangePowerPlanToRun (void)
{
	BatchFile.Open ();
	BatchFile.Writeln ("PowerCfg.EXE -Delete 8fa00120-7fa2-4027-b57c-8ec22994bbfe"); 
	BatchFile.Writeln ("PowerCfg.EXE -Import \"\\Program Files\\Quick Plate\\Plating\\Quick Plate Run.pow\" 8fa00120-7fa2-4027-b57c-8ec22994bbfe"); 
	BatchFile.Writeln ("PowerCfg.EXE -SetActive 8fa00120-7fa2-4027-b57c-8ec22994bbfe"); 
	BatchFile.Run ();
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::ChangePowerPlanToSimulate (void)
{
	BatchFile.Open ();
	BatchFile.Writeln ("PowerCfg.EXE -Delete 8bb26300-12f0-49d1-b5c7-32683d384571"); 
	BatchFile.Writeln ("PowerCfg.EXE -Import \"\\Program Files\\Quick Plate\\Plating\\Quick Plate Simulate.pow\" 8bb26300-12f0-49d1-b5c7-32683d384571"); 
	BatchFile.Writeln ("PowerCfg.EXE -SetActive 8bb26300-12f0-49d1-b5c7-32683d384571"); 
	BatchFile.Run ();
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::TurnOffScreenSavers(void)
{
	LONG	EnumRet;
	LONG	OpenRet;
	LONG	DeleteRet;
	char	Name[100];
	DWORD	NameSize;
	char	Class[100];
	DWORD	ClassSize;
	int		Index;
	HKEY	hKey; 

	Index = 0;

	do
	{
		//Enumerate all users
		NameSize	= sizeof (Name);
		ClassSize = sizeof (Class);
		EnumRet = RegEnumKeyEx (HKEY_USERS, Index, LPTSTR (&Name), &NameSize, NULL, LPTSTR (&Class), &ClassSize, NULL); 

		if (EnumRet == ERROR_SUCCESS)
		{
			//Open key where the screen saver is located
			strcat_s (Name, "\\Control Panel\\Desktop");
			OpenRet = RegOpenKeyEx (HKEY_USERS, Name, 0, KEY_SET_VALUE, &hKey); 

			if (OpenRet == ERROR_SUCCESS)
			{
				//Delete the screen saver
				DeleteRet = RegDeleteValue (hKey, "SCRNSAVE.EXE");

				if (DeleteRet == ERROR_SUCCESS)
				{
				}
				else if (DeleteRet == ERROR_FILE_NOT_FOUND)
				{
				}
				else if (DeleteRet != ERROR_SUCCESS)
				{
					ReportError ("Update Customer", "RegDeleteKeyEx failed (%d) %s!", DeleteRet, Name);
				}

				SetKeyValue (hKey, "ScreenSaveActive", REG_SZ, "0");

				RegCloseKey (hKey);
			}
		}
		Index++;
	} while (EnumRet == ERROR_SUCCESS);
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::AdjustUserAccessControl (void)
{
	HKEY	hKey; 
	int		Success;

	// for more see: http://technet.microsoft.com/en-us/library/dd835564(v=ws.10).aspx#BKMK_RegistryKeys

	if (!Application.UserIsAdministrator ()) return;

	//This will show in Action Center as UAC being shut off but much of UAC is still on.  Enough that Metro apps run and hopefully there is some protection.  If this isn't done then reboot won't auto start the programs.
	//Partually turn off User Access Control it prevents Update Customer from starting unless run as an admin and even then it can't start other programs without elevation.
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
	{
		SimulationReportLastError("Update Customer", "RegOpenKeyEx");
		return;
	}

	SetKeyValue (hKey, "ConsentPromptBehaviorAdmin", REG_DWORD, "0"); //Elevate without prompting

	RegCloseKey (hKey); 

	//This doesn't work!!! It has to be done manually.  This code sets the item properly in the Layers key but there is something else set somewhere when you change the compatibility settings for the exe.
	//This seems to keep the program from running in the statup group!!!  Without this there doesn't seem to be any way to change the priority to real time.
	//Putting a batch file in the startup group to start update customer does work.
	//Update Customer needs to run as an admin so when it starts Snap and Quick Hoist Snap they will run as administrators so they can change priorities.
	//Set run as admin for Update Programs DISABLEUSERCALLBACKEXCEPTION seems to be there too.  This means the app had an unhandled exception.  Use RegCreateKey because the Layers folder may not be there.
	//if (RegCreateKey (HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", &hKey) != ERROR_SUCCESS) SimulationReportLastError ("Update Customer", "RegCreateKey");
	//SetKeyValue (hKey, "C:\\Program Files\\Quick Plate\\Plating\\Update Customer.exexx",	REG_SZ, "~ RUNASADMIN"); //Set to run as admin for all users
	//RegCloseKey (hKey); 

	//Clearing this flag, however it got set, will allow the program to be run by non administrators
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
	{
		SimulationReportLastError("Update Customer", "RegOpenKeyEx");
		return;
	}
	Success = RegDeleteValue (hKey, "C:\\Program Files\\Quick Plate\\Plating\\Quick Hoist Snap.exe");
	Success = RegDeleteValue (hKey, "C:\\Program Files\\Quick Plate\\Plating\\Snap.exe");
	RegCloseKey (hKey); 
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::TrimBigTables(void)
{
	DOUBLE_TIME	DoubleTime;
	DOUBLE_TIME	CurrentTime;
	bool				TimeValid;
	bool				DropTable;

	//Trim the largest tables by renaming them to <table name> Old and starting a new main table.
	//The programs that read these tables must read both version ostrcpy the table to make sure they get the data.
	//The big tables are Bath History, Mover, and Wait.  Move Time can get large too but its records are small so it isn't included.

	GetDoubleTime(&CurrentTime);

	//------------------------------------------------Check the Bath History table for more than 10 weeks.
	TimeValid = false;
	BathHistoryTable.Initialize("Bath History");
	BathHistoryTable.StartQuery();
	strcpy_s(BathHistoryTable.Statement, "SELECT MIN ([Date Time]) FROM [Bath History]");
	if (!BathHistoryTable.ExecuteStatement()) ReportError("Update Customer", "Select MIN failed!");
	BathHistoryTable.BindColumn (SQL_DATETIMEOFFSETdouble, &DoubleTime, sizeof (DoubleTime));
	while (BathHistoryTable.ReadRecord())
	{
		TimeValid = true;
	}
	if (TimeValid)
	{
		DropTable = false;

		if ((DoubleTime > 0.0) && //An empty table will give us a minimum of 0
				(DoubleTime < (CurrentTime - (SECONDS_PER_WEEK * 10))))
		{
			DropTable = true;
		}

		if (DropTable)
		{
			//Drop prior old table
			BathHistoryTable.StartQuery();
			strcpy_s(BathHistoryTable.Statement, "Drop Table [Bath History Old]");
			if (!BathHistoryTable.ExecuteStatement()) ReportError("Update Customer", "Drop Bath History Old failed!");

			//Rename main file to old
			BathHistoryTable.StartQuery();
			strcpy_s(BathHistoryTable.Statement, "sp_rename [Bath History], [Bath History Old]");
			if (!BathHistoryTable.ExecuteStatement()) ReportError("Update Customer", "Select Rename failed!");

			BathHistoryTable.Initialize("Bath History"); //Will generate a new main table
		}
	}

	//------------------------------------------------Check the Environmental History table for more than 5 years. The DEQ requires them to have data on environmental loops for 5 years and everything is together.  Otherwise 10 weeks is probably enough.
	TimeValid = false;
	EnvironmentalHistoryTable.Initialize("Environmental History");
	EnvironmentalHistoryTable.StartQuery();
	strcpy_s(EnvironmentalHistoryTable.Statement, "SELECT MIN ([Date Time]) FROM [Environmental History]");
	if (!EnvironmentalHistoryTable.ExecuteStatement()) ReportError("Update Customer", "Select MIN failed!");
	EnvironmentalHistoryTable.BindColumn(SQL_DATETIMEOFFSETdouble, &DoubleTime, sizeof (DoubleTime));
	while (EnvironmentalHistoryTable.ReadRecord())
	{
		TimeValid = true;
	}
	if (TimeValid)
	{
		DropTable = false;

		if ((DoubleTime > 0.0) && //An empty table will give us a minimum of 0
				(DoubleTime < (CurrentTime - (SECONDS_PER_WEEK * 53 * 5))))  //5 years and a little more
		{
			DropTable = true;
		}

		if (DropTable)
		{
			//Drop prior old table
			EnvironmentalHistoryTable.StartQuery();
			strcpy_s(EnvironmentalHistoryTable.Statement, "Drop Table [Environmental History Old]");
			if (!EnvironmentalHistoryTable.ExecuteStatement()) ReportError("Update Customer", "Drop Environmental History Old failed!");

			//Rename main file to old
			EnvironmentalHistoryTable.StartQuery();
			strcpy_s(EnvironmentalHistoryTable.Statement, "sp_rename [Environmental History], [Environmental History Old]");
			if (!EnvironmentalHistoryTable.ExecuteStatement()) ReportError("Update Customer", "Select Rename failed!");

			EnvironmentalHistoryTable.Initialize("Environmental History"); //Will generate a new main table
		}
	}

	//------------------------------------------------Check the Wait table for more than 3 weeks
	TimeValid = false;
	WaitTable.Initialize("Wait");
	WaitTable.StartQuery();
	strcpy_s(WaitTable.Statement, "SELECT MIN ([Date Time]) FROM [Wait]");
	if (!WaitTable.ExecuteStatement()) ReportError("Update Customer", "Select MIN failed!");
	WaitTable.BindColumn (SQL_DATETIMEOFFSETdouble, &DoubleTime, sizeof (DoubleTime));
	while (WaitTable.ReadRecord())
	{
		TimeValid = true;
	}
	if (TimeValid)
	{
		if ((DoubleTime > 0.0) && //An empty table will give us a minimum of 0
				(DoubleTime < (CurrentTime - (SECONDS_PER_WEEK * 3))))
		{
			//Drop prior old table
			WaitTable.StartQuery();
			strcpy_s(WaitTable.Statement, "Drop Table [Wait Old]");
			if (!WaitTable.ExecuteStatement()) ReportError("Update Customer", "Drop Wait Old failed!");

			//Rename main file to old
			WaitTable.StartQuery();
			strcpy_s(WaitTable.Statement, "sp_rename [Wait], [Wait Old]");
			if (!WaitTable.ExecuteStatement()) ReportError("Update Customer", "Select Rename failed!");

			WaitTable.Initialize("Wait"); //Will generate a new main table
		}
	}

	//------------------------------------------------Check the Mover table for more than 25 weeks
	TimeValid = false;
	MoverTable.DefineFields ();
	MoverTable.DataTable.StartQuery();
	strcpy_s(MoverTable.DataTable.Statement, "SELECT MIN ([Date Time]) FROM [Mover]");
	if (!MoverTable.DataTable.ExecuteStatement()) ReportError("Update Customer", "Select MIN failed!");
	MoverTable.DataTable.BindColumn (SQL_DATETIMEOFFSETdouble, &DoubleTime, sizeof (DoubleTime));
	while (MoverTable.DataTable.ReadRecord())
	{
		TimeValid = true;
	}
	if (TimeValid)
	{
		if ((DoubleTime > 0.0) && //An empty table will give us a minimum of 0
				(DoubleTime < (CurrentTime - (SECONDS_PER_WEEK * 25))))
		{
			//Drop prior old table
			MoverTable.DataTable.StartQuery();
			strcpy_s(MoverTable.DataTable.Statement, "Drop Table [Mover Old]");
			if (!MoverTable.DataTable.ExecuteStatement()) ReportError("Update Customer", "Drop Mover Old failed!");

			//Rename main file to old
			MoverTable.DataTable.StartQuery();
			strcpy_s(MoverTable.DataTable.Statement, "sp_rename [Mover], [Mover Old]");
			if (!MoverTable.DataTable.ExecuteStatement()) ReportError("Update Customer", "Select Rename failed!");

			MoverTable.DefineFields (); //Will generate a new main table
		}
	}
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::ReservePorts (void)
{
	int		I;
	HKEY	hKey; 
	LONG	lRet;
	char	Value[MAX_STRING];
	char	NewValue[MAX_STRING];
	DWORD	ValueSize;

	//This doesn't seem to work on 2012.  IIS and VNC use ports in our port range.

	if (!UserIsAdministrator ()) return; //These calls can only be done by administrators, the auto logon user isn't an administrator.

	lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_QUERY_VALUE, &hKey); 
	if (lRet != ERROR_SUCCESS)
	{
		ReportError ("Update Customer", "Open key failed");
		return;
	}

	ZeroMemory(&Value, sizeof (Value)); //This key may not exist and Query won't blank the value
	ValueSize = sizeof (Value);
	lRet = RegQueryValueEx (hKey, "ReservedPorts", NULL, NULL, LPBYTE (&Value), &ValueSize);

	//This key got set to 256 characters of garbage because of a prior error.  Ignore garbage value.
	if (strlen(Value) > 15) strcpy_s(Value, "");

	StringTrim(Value);

	I = StringPosition (Value, " ");

	if (I > 0)
	{
		//Make sure there isn't a third range
		if (StringPosition (PCHAR (&Value[I + 1]), " ") >= 0)
		{
			ReportError ("Update Customer", "There are more than two ReservedPorts ranges: %s", Value);
			return;
		}

		Value[I] = 0; //Blank out everything after first range, assume this is our range from before
	}

	QPsprintf_s (NewValue, "%d-%d", MINIMUM_PORT, MAXIMUM_PORT);
	if (StringEqual(NewValue, Value))
				QPsprintf_s (NewValue, "%d-%d", MINIMUM_PORT, MAXIMUM_PORT); //We were the only port range so just put us back in again
	else	QPsprintf_s (NewValue, "%s %d-%d", Value, MINIMUM_PORT, MAXIMUM_PORT);

	RegCloseKey (hKey); 

	lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_SET_VALUE, &hKey); 
	if (lRet != ERROR_SUCCESS)
	{
		ReportError ("Update Customer", "Open key failed");
		return;
	}

	SetKeyValue (hKey, "ReservedPorts", REG_SZ, NewValue);

	RegCloseKey (hKey); 
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::SetCustomerInRegistry (void)
{
	HKEY	hKey; 
	LONG	lRet;

	if (!UserIsAdministrator ()) return; //These calls can only be done by administrators, the auto logon user isn't an administrator.

	lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_SET_VALUE, &hKey); 

	if (lRet != ERROR_SUCCESS)
	{
		ReportError ("Update Customer", "Open key failed");
		return;
	}

	SetKeyValue (hKey, "CustomerName", REG_SZ, CustomerName);

	RegCloseKey (hKey); 
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::AddPlatingToPath (void)
{
	HKEY	hKey; 
	char	Path[MAX_PATH * 10];
	DWORD	PathSize;
	DWORD	DataType;

	if (!UserIsAdministrator ()) return; //These calls can only be done by administrators, the auto logon user isn't an administrator.

	//Make sure /Quick Plate/Plating is on the path so DLL's can be found for applications that are in the customer directory.
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) 
	{
		ReportError ("Update Customer", "Open key failed");
		return;
	}

	PathSize = sizeof (Path);

	if (RegQueryValueEx (
		hKey,								// subkey handle 
		"Path",							// value name 
    0,									// must be zero 
    &DataType,					// value type 
		LPBYTE (&Path),			// pointer to value data 
		&PathSize) != ERROR_SUCCESS) // data size
	{
		SimulationReportLastError ("Update Customer", "Could not read path from registry"); 
		return;
	}

	RegCloseKey (hKey); 

	if (!StringInString (Path, "C:\\Program Files\\Quick Plate\\Plating"))
	{
		if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
		{
			ReportError ("Update Customer", "Open key failed");
			return;
		}

		strcat_s (Path, ";C:\\Program Files\\Quick Plate\\Plating");

		SetKeyValue(hKey, "Path", REG_EXPAND_SZ, Path);

		RegCloseKey (hKey); 
	}
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::CreateRequiredDirectories (void)
{
	//Don't do everything else is we are doing Start Missing Services or an update
	if (UserIsAdministrator () &&
			StringEqual (CommandTail, ""))
	{
		APPLICATION::CreateRequiredDirectories ();

		if (!Application.InitializeDatabase())
		{
			ReportError("Update Customer", "Database initialization failed!");
			exit(1);
		}
	}
	else
	{
		//If not an administrator we are starting to do Start Missing Services or doing an update.
		APPLICATION::CreateRequiredDirectories ();
	}
}

//---------------------------------------------------------------------------
#pragma warning(suppress: 6262)
bool CUSTOM_APPLICATION::InitializeDatabase (void)
{
	SQL_TABLE			SetupTable;
	SQLRETURN			Return;
	SQLCHAR				ConnStrIn[MAX_STRING] = "";
	SQLCHAR				ConnStrOut[MAX_STRING] = "";
	SQLSMALLINT		cbConnStrOut = 0;

	//Carefully check to see if the SQL servers are running

	//Assume all is well for the plain user
	if (!Application.UserIsAdministrator()) return true;

	//Test Site SQL server
	//Ok to call initialize, if create table is blank it won't try to create table
	SetupTable.Initialize (ComputerName, "master", "Setup table"); 

	SetupTable.OpenStatementHandle();

	Return = SQLAllocHandleStd (SQL_HANDLE_DBC, SQLEnvironmentHandle, &SQLPooledConnection[SetupTable.PooledConnectionIndex].ConnectHandle);
	SetupTable.SQLError ("SQLAllocHandle connect", SQLPooledConnection[SetupTable.PooledConnectionIndex].ConnectHandle, Return, SQL_HANDLE_ENV);

	QPsprintf_s (PCHAR (&ConnStrIn), sizeof (ConnStrIn),
						"DRIVER=SQL Server;NTL=TCPIP;SERVER=%s;APP=%s;WSID=%s;DATABASE=%s;LANGUAGE=English;Trusted_Connection=Yes;Enlist=false;",
						SetupTable.ServerName,
						ApplicationName,
						ComputerName,
						"master");

	//Make connection without data source. Ask that driver prompt if insufficient information.
	//Driver returns SQL_ERROR and application prompts user for missing information.
	//Window handle not needed for SQL_DRIVER_NOPROMPT.
	Return = SQLDriverConnect (SQLPooledConnection[SetupTable.PooledConnectionIndex].ConnectHandle,	// Connection handle
									FrameWindow,							// Window handle
									ConnStrIn,								// Input connect string
									SQL_NTS,									// Null-terminated string
									ConnStrOut,								// Address of output buffer
									MAX_STRING,								// Size of output buffer
									&cbConnStrOut,						// Address of output length
									SQL_DRIVER_NOPROMPT);			// When connection pooling is enabled this can only be SQL_DRIVER_NOPROMPT

	if (SetupTable.SQLError ("SQLDriverConnect", SQLPooledConnection[SetupTable.PooledConnectionIndex].ConnectHandle, Return, SQL_HANDLE_DBC))
	{
		ShowMessageBox ("Could not connect to the SQL server!\nMake sure your SQL server is running!", "Update Customer", MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONSTOP | MB_OK);
		exit(1);
	}
	SetupTable.CloseStatementHandle();


	//The servers are reachable now creat the required databases
	//Create the customer database
	SetupTable.Initialize (ComputerName, "master", "Setup table");

	SetupTable.OpenStatementHandle ();

	//[] around name because it may have a space in it
	QPsprintf_s (SetupTable.Statement, sizeof (SetupTable.Statement), "Create DATABASE [%s]", CustomerName);
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	//Bulk-logging recovery mode won't log anything.  Full recovery mode will let the log file grow until it fills the disk.  A recent 2012 bug in simple mode doesn't auto shrink the log file and it grows out of control.
	QPsprintf_s (SetupTable.Statement, "Alter Database [%s] Set Recovery Simple" , CustomerName);
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	//Will cause the log file to be shrunk after a backup
	QPsprintf_s (SetupTable.Statement, "Alter Database [%s] Set Auto_shrink on" , CustomerName);
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	//Compatibility level to SQL 2012
	QPsprintf_s (SetupTable.Statement, "Alter Database [%s] SET COMPATIBILITY_LEVEL = 110", CustomerName);
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	//Remove all replication because it will lock the table so any modify statements will fail.
	//If a customer need replication then will will have to support it in some automatic fasion
	QPsprintf_s (SetupTable.Statement, "sp_removedbreplication '%s'", CustomerName);
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	SetupTable.CloseStatementHandle ();


	//Create the Quick Plate database
	SetupTable.Initialize (ComputerName, "master", "Setup Table");

	SetupTable.OpenStatementHandle ();

	QPsprintf_s (SetupTable.Statement, "Create DATABASE [Quick Plate]");
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	//Bulk-logging recovery mode won't log anything.  Full recovery mode will let the log file grow until it fills the disk.  A recent 2012 bug in simple mode doesn't auto shrink the log file and it grows out of control.
	QPsprintf_s (SetupTable.Statement, "Alter Database [Quick Plate] Set Recovery Simple" );
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	QPsprintf_s (SetupTable.Statement, "Alter Database [Quick Plate] Set Auto_shrink on" );
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	//Compatibility level to SQL 2012
	QPsprintf_s (SetupTable.Statement, "Alter Database [Quick Plate] SET COMPATIBILITY_LEVEL = 110");
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	//Remove all replication because it will lock the table so any modify statements will fail.
	QPsprintf_s (SetupTable.Statement, "sp_removedbreplication 'Quick Plate'");
	Return = SQLExecDirect (SetupTable.StatementHandle, pSQLCHAR (&SetupTable.Statement), SQL_NTS);
	SetupTable.SQLError ("SQLExecDirect", SetupTable.StatementHandle, Return, SQL_HANDLE_STMT);

	SetupTable.CloseStatementHandle ();

	return true;
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::Relaunch (void)
{
	CreateProcess ("\"C:/Program Files/Quick Plate/Plating/Update Customer.exe\"");

	exit (0);
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::InitializeDisplayWindows (void)
{
	//Don't do everything else is we are doing Start Missing Services or an update
	if (UserIsAdministrator ())
	{
		//These windows may not be open and all the update and backup options need them
		HiddenFileList.Setup (&CustomerList);
		HiddenFileList.DefineFields (); //So sorts will work if the screen isn't open
	}

	SetClocks (false); //Make sure w32tm is running and set right, the settings don't stick
	EnableC$();

	CustomServiceList.AlwaysBuildList (NULL);


	FileList.Setup (&CustomerList);
	FileList.DefineFields (); //So sorts will work if the screen isn't open

	if (UserIsAdministrator ())
	{
		RegisterDisplayWindow (&FileList,								Local_mode, MANUAL_RESTORE);  
		//FeaturesDialog.Initialize (); Already initialized in WinMain
	}
	RegisterDisplayWindow (&CustomServiceList,			Local_mode, MANUAL_RESTORE);  
	RegisterDisplayWindow (&WindowSizeList,					Local_mode, MANUAL_RESTORE);  
	RegisterDisplayWindow(&ColumnHideList,					Local_mode, MANUAL_RESTORE);
	RegisterDisplayWindow(&InPlantList,							Local_mode, MANUAL_RESTORE);
	RegisterDisplayWindow (&DialogDataList,					Local_mode, MANUAL_RESTORE);  

	RegisterDisplayWindow (&CustomerList,						Local_mode, MANUAL_RESTORE);  

	//Do this so we can use the SQL filter dialog
	RegisterDisplayWindow (&UpdateSQL.WaitList,	Local_mode, MANUAL_RESTORE);  

	RegisterDisplayWindow (&InventoryList,			Local_mode, MANUAL_RESTORE);  
	RegisterDisplayWindow (&ContactList,				Local_mode, MANUAL_RESTORE);  
	RegisterDisplayWindow (&LastModifiedList,		Local_mode, MANUAL_RESTORE);  

	AdjustMenu ();

	QPsprintf_s (FrameTitle, "%s - Update Customer", CustomerName);

	SetWindowText (FrameWindow, FrameTitle);

	//There must be at least one window open for the remote update to work or CommandProc won't get called.
	PostMessage (FrameWindow, WM_COMMAND, ID_FILE_SERVICES, 0); 
	PostMessage (FrameWindow, WM_COMMAND, ID_FILE_CUSTOMERS, 0); //Must be open for updates to work

	CopyShortcuts ();
	
	TurnOffScreenSavers ();
	AdjustUserAccessControl ();

	SetCustomerInRegistry (); //Write to registry for the shortcuts to start custom applications
	AddPlatingToPath();

	ReservePorts (); //Reserve our TCP/IP ports

	FirstTime = true;
}

//---------------------------------------------------------------------------
WNDPROC CUSTOM_APPLICATION::WindowsProcedure	(HWND		hWndFrame,
																							UINT		wMsg,
																							WPARAM	wParam,
																							LPARAM	lParam)
{
	WINDOWPLACEMENT WindowPlacement;
	int							DeltaX;
	int							DeltaY;
	
	//Move the Configuration Dialogs around with the main window
	if (wMsg == WM_MOVE)
	{
		ZeroMemory (&WindowPlacement, sizeof (WindowPlacement));

		if (!GetWindowPlacement (FrameWindow, &WindowPlacement))
		{
			SimulationReportLastError ("General", "GetWindowPlacement WindowsProcedure");
			return APPLICATION::WindowsProcedure (hWndFrame, wMsg, wParam, lParam);
		}

		DeltaX = 0;
		DeltaY = 0;

		if (OrigionalX == -1)
		{
			OrigionalX = WindowPlacement.rcNormalPosition.left;
			OrigionalY = WindowPlacement.rcNormalPosition.top;
		}

		if ((OrigionalX != WindowPlacement.rcNormalPosition.left) ||
				(OrigionalY != WindowPlacement.rcNormalPosition.top))
		{
			DeltaX = WindowPlacement.rcNormalPosition.left - OrigionalX;
			DeltaY = WindowPlacement.rcNormalPosition.top - OrigionalY;

			OrigionalX = WindowPlacement.rcNormalPosition.left;
			OrigionalY = WindowPlacement.rcNormalPosition.top;
		}
	}

	return APPLICATION::WindowsProcedure (hWndFrame, wMsg, wParam, lParam);
}

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::AdjustMenu (void)
{

	//This only works from popups or with command tail
	DeleteMenu(OurMenu, ID_PROGRAMS_PUBLISH_CUSTOMER, MF_BYCOMMAND);
	DeleteMenu(OurMenu, ID_PROGRAMS_INSTALL_UPDATE_FINISH, MF_BYCOMMAND);
	DeleteMenu(OurMenu, ID_PROGRAMS_UPDATE_PROGRAMS, MF_BYCOMMAND);

	//Only from the console
	if (!StringEqual(TerminalName, ""))
	{
		DeleteMenu(OurMenu, ID_PROGRAMS_INSTALL_UPDATE, MF_BYCOMMAND);
		DeleteMenu(OurMenu, ID_PROGRAMS_STOP_ALL_PROGRAMS, MF_BYCOMMAND);
		DeleteMenu(OurMenu, ID_PROGRAMS_START_MISSING_PROGRAMS, MF_BYCOMMAND);
	}

	//These are internal tools not used for any customers
	if (!StringInString(ComputerName, "Mahood-"))
	{
		DeleteMenu(OurMenu, ID_BACKUP_PICTURES, MF_BYCOMMAND);
		DeleteMenu(OurMenu, ID_BACKUP_INVENTORY, MF_BYCOMMAND);
		DeleteMenu(OurMenu, ID_BACKUP_CONTACT, MF_BYCOMMAND);
		DeleteMenu(OurMenu, ID_SOURCE_BACKUP, MF_BYCOMMAND);
		DeleteMenu(OurMenu, ID_SOURCE_PUBLISH, MF_BYCOMMAND);
		DeleteMenu(OurMenu, IDM_FILE_PRINT, MF_BYCOMMAND);
	}
	else
	{
	}

	DrawMenuBar (FrameWindow);
}

//---------------------------------------------------------------------------
bool CUSTOM_APPLICATION::CanClose (void)
{
	return APPLICATION::CanClose ();
}

//---------------------------------------------------------------------------
bool CUSTOM_APPLICATION::SaveAll (void)
{
	if (APPLICATION::SaveAll ())
	{
		Relaunch (); //So Start Missing Programs will work with the new settings
		return true; //Relaunch won't return, this will keep compiler happy
	}
	return false;
}

typedef struct MAGA_BYTE
{
	int			ItemNumber;
	double	HugeArray[125000]; //about a megabyte
} MEGA_BYTE;

MEGA_BYTE	NullData;

//---------------------------------------------------------------------------
void CUSTOM_APPLICATION::StressMemory (void)
{
	int								I, J;
	bool							Forever = true;
	vector<MEGA_BYTE> MegaByte;

	ZeroMemory (&NullData, sizeof (NullData));

	for (I = 0; I <= 1024; I++)
	{
		NullData.ItemNumber = I;
		MegaByte.push_back (NullData);
	}
	while (Forever)
	{
		SleepSeconds (8.0);
		for (I = 0; I <= 1024; I++)
		{
			J = MegaByte[I].ItemNumber; //Read vector array so it will get swapped back into physical memory
		}
	}
}

//---------------------------------------------------------------------------
bool CUSTOM_APPLICATION::CommandProc (WPARAM	wParam,
																			LPARAM	lParam)
{
	int					Reply;
	int					Index;
	DOUBLE_TIME	OurStartTime;
	DOUBLE_TIME	FinishTime;
	double			Writes;
	double			Reads;
	DOUBLE_TIME	DateTime;
	char				Computer[17];
	char				Terminal[17];
	char				OurApplicationName[33];
	char				Object[49];
	char				Message[257];

	lParam = lParam;

	AdjustMenu ();

	//Check command tail here rather in InitializeDisplayWindows so all the windows will be auto restored and the the file list will get focus
	if (FirstTime &&
			!StringEqual (CommandTail, ""))
	{
		FirstTime = false;

		//These lists are used for auto start, make sure they are valid
		SocketList.ServiceList.AlwaysBuildList (NULL);
		CustomServiceList.AlwaysBuildList (NULL);
		CustomerList.AlwaysBuildList (NULL);

		//From a shortcut
		if (StringEqual (CommandTail, "Start missing services"))
		{
			//Only on console
			if (StringEqual(TerminalName, "")) PostMessage(FrameWindow, WM_COMMAND, ID_PROGRAMS_START_MISSING_PROGRAMS, 0);
		}
		else if (StringEqual (CommandTail, "Update Programs"))
		{
			//From Update Custom doing install updates
			strcpy_s (RemoteSQLIPAddress, "");

			PostMessage (FrameWindow, WM_COMMAND, ID_PROGRAMS_UPDATE_PROGRAMS, 0);
		}
		else ReportError ("Update Customer", "Unexpected command tail %s", CommandTail);
	}

  switch (wParam)
	{
		//------------------------------------------- File submenu ---------------------------------
		case ID_FILE_SERVICES:
		{
			CustomServiceList.SetAutoSave();
			Application.DoCreateWindow (&CustomServiceList, wParam);
			break;
		}

		case ID_FILE_CUSTOMERS:
			CustomerList.SetAutoSave();
			Application.DoCreateWindow(&CustomerList, wParam);
			break;

		case ID_FILE_FEATURES:
			if (FeaturesDialog.ShowDialog ())
			{
				Relaunch ();
			}
			break;

		case ID_FILE_WINDOW_SIZE:
			WindowSizeList.SetAutoSave();
			Application.DoCreateWindow (&WindowSizeList, wParam);
			break;

		case ID_FILE_COLUMN_HIDE:
			ColumnHideList.SetAutoSave();
			Application.DoCreateWindow (&ColumnHideList, wParam);
			break;

		case ID_FILE_IN_PLANT:
			InPlantList.SetAutoSave();
			Application.DoCreateWindow(&InPlantList, wParam);
			break;

		case ID_FILE_DIALOG_DATA:
			Application.DoCreateWindow (&DialogDataList, wParam);
			break;

		case ID_FILE_LAST_MODIFIED:
			Application.DoCreateWindow (&LastModifiedList, wParam);
			break;

			//--------------------------------- Programs submenu--------------------------------
		case ID_PROGRAMS_INSTALL_UPDATE:
			//This is step 1 that updates the files to our local publish folder
			if (!StringEqual(TerminalName, ""))
			{
				ReportError("Update Customer", "You may only update from the console!");
				break;
			}

			if (ReadTimewaysRunning())
			{
				ReportError("Update Customer", "Timeways are running, update not allowed!");
				break;
			}

			//Make sure there is an active customer
			Index = CustomerList.GetIndex(CustomerName);
			if (!CustomerList.Customer[Index].ActiveCustomer)
			{
				ReportError("Update Customer", "No active customer.  Fix Customer list first!");
				break;
			}

			//The publish computer may be another computer so copy publish from there to here first
			Index = CustomerList.GetIndex(CustomerName);

			if (!StringEqual(CustomerList.Customer[Index].PublishServer, ComputerName))
			{
				Index = CustomerList.GetIndex(CustomerList.Customer[Index].PublishServer);
				FileList.SetupDifferentCustomer(CustomerList.Customer[Index].RemoteSQLIPAddress, CustomerList.Customer[Index].Name);
				SetRemoteCredentials(CustomerList.Customer[Index].UserName, CustomerList.Customer[Index].LineComputerName, CustomerList.Customer[Index].Password);

				FileList.DoCreateWindow(Update_publish_from_server);
				//The update will post a message to go to ID_PROGRAMS_INSTALL_UPDATE_FINISH when its finished
				break;
			}
			break;

		case ID_PROGRAMS_INSTALL_UPDATE_FINISH:
			//Step 2 copy the new Update Customer to a temporary file and run it
			Index = CustomerList.GetIndex(CustomerName);

			//Make sure there is an active customer
			if (!CustomerList.Customer[Index].ActiveCustomer)
			{
				ReportError("Update Customer", "No active customer.  Fix Customer list first!");
				break;
			}

			//Logon to remote computer for file and SQL access.  Save remote credentials for use by SQL Table and fine open.
			SetRemoteCredentials(CustomerList.Customer[Index].UserName, CustomerList.Customer[Index].LineComputerName, CustomerList.Customer[Index].Password);

			//Copy the new Update Customer to \temp\ and run it to do the update
			//Get the latest Update Customer from our publish folder that was just updated from the publish server.
			BatchFile.Open();
			BatchFile.Writeln("XCOPY.EXE /Y  \"C:\\Program Files\\Quick Plate\\Publish\\Plating\\Update Customer.exe\" \"c:\\Temp\"");
			BatchFile.Run();

			//Update Customer must run as an administrator to do the update
			CreateProcess("\"/temp/Update Customer.exe\" Update Programs");
			exit (0);
			break;

		case ID_PROGRAMS_UPDATE_PROGRAMS:
			//Step 3 we are the temporary copy of Update Customer started with a command tail.  Do the update from our local publish folder.
			if (FileList.DoingFileTransfer()) break;

			CustomServiceList.StopAll(); //Will not return until all services stopped

			TrimBigTables(); //Can only be done when all programs are stopped

			//If this is the simulation computer at a customer site the update must come from the Update Server
			if (Features.SimulateIO)
			{
				Index = CustomerList.GetIndex(CustomerName);

				if (Index < 0)
				{
					ReportError("Update Customer", "%s not found in customer list!", CustomerName);
					exit(1);
				}

				Index = CustomerList.GetIndex(CustomerList.Customer[Index].PublishServer);

				//Here we always update from the SQL server.  In the field simulation computers update from the Publish Computer if it is different then the simulation computer.
				//Simulators that are their own remote SQL server.
				if (!StringInString(ComputerName, "Mahood") &&
					!StringEqual(IPAddress, CustomerList.Customer[Index].RemoteSQLIPAddress))
				{
					FileList.SetupDifferentCustomer(CustomerList.Customer[Index].RemoteSQLIPAddress, CustomerList.Customer[Index].Name);
					SetRemoteCredentials(CustomerList.Customer[Index].UserName, CustomerList.Customer[Index].LineComputerName, CustomerList.Customer[Index].Password);
				}
			}

			//Will do the update, the thread will wait for it to finish then restart the programs.
			FileList.DoCreateWindow(Install_update);

			//Start a thread so the progress will show and wait for the update to finish
			ThreadList.DoCreateThread(LPTHREAD_START_ROUTINE(RemoteUpdateThread), this, 0, "Remote Update");

			break;

		case ID_PROGRAMS_START_MISSING_PROGRAMS:
			if (!Features.SimulateIO &&
					!StringEqual (AUTO_SIGN_IN_USER, UserName) &&
					!StringEqual (CustomerName, "Test Snap"))
			{
				ReportError ("Update Customer", "Only %s may start missing programs!", AUTO_SIGN_IN_USER);
				exit (0);
			}
			else
			{
				SocketList.ServiceList.StartAll ();
				if (!StringEqual (CommandTail, "")) exit (0);
			}
			break;

		case ID_PROGRAMS_STOP_ALL_PROGRAMS:
			if (ReadTimewaysRunning ())
			{
				ReportError ("Update Customer", "Timeways are running, stop all programs not allowed!");
				break;
			}

			CustomServiceList.StopAll ();

			//This is useful if this computer doesn't do updates.
			TrimBigTables(); //Can only be done when all programs are stopped

			if (!Features.SimulateIO)
			{
				//Stop streaming because it looks better.  There is no harm in leaving streaming going when we do an update, this is just cosmetic.
				//The file name must be in quotes because of the spaces in the name
				CreateProcess ("\"C:\\Program Files\\Quick Plate\\Plating\\Quick Hoist Snap.exe\" AutoStarted StopStreaming");
			}
			break;

		case ID_PROGRAMS_PUBLISH_CUSTOMER:
		{
			if (FileList.DoingFileTransfer ()) break;

			if (CustomerList.DataModified ())
			{
				ReportError ("Update Customer", "Save Customers first!");
				break;
			}

			Index = CustomerList.GetIndex (PublishCustomerName);

			if (StringEqual (CustomerList.Customer[Index].RemoteSQLIPAddress, ""))
			{
				ReportError ("Update Customer", "No Publish IP Address for this customer");
			}
			else
			{
				//Replicate help to multiple lines
				BatchFile.Open ();
				BatchFile.Writeln ("XCOPY.EXE /Y \"c:\\Program Files\\Quick Plate\\Airlane South\\Customer.chm\" \"c:\\Program Files\\Quick Plate\\Airlane North\\*.*\"");
				BatchFile.Writeln ("XCOPY.EXE /Y \"c:\\Program Files\\Quick Plate\\Airlane South\\Customer.chm\" \"c:\\Program Files\\Quick Plate\\Barden\\*.*\"");
				BatchFile.Writeln("XCOPY.EXE /Y \"c:\\Program Files\\Quick Plate\\Airlane South\\Customer.chm\" \"c:\\Program Files\\Quick Plate\\Kraft\\*.*\"");
				//BatchFile.Writeln ("XCOPY.EXE /Y \"c:\\Program Files\\Quick Plate\\Airlane South\\Customer.chm\" \"c:\\Program Files\\Quick Plate\\Monroe\\*.*\"");
				BatchFile.Run ();

				Index = CustomerList.GetIndex (CustomerList.Customer[Index].PublishServer);

				FileList.SetupDifferentCustomer (CustomerList.Customer[Index].RemoteSQLIPAddress, CustomerList.Customer[Index].Name);
				FileList.DoCreateWindow (Publish_customer);

				//This isn't perfect, it will set the date even if the user cancels the backup
				time (&CustomerList.Customer[Index].LastPublished);
				CustomerList.SetSQLRecordModified (Index, true);

				CustomerList.WriteRecords ();
				CustomerList.InvalidateAllRecords ();
			}

			break;
		}

		//--------------------------------- Backup submenu ---------------------------------
		case ID_BACKUP_PICTURES:
			{
				if (FileList.DoingFileTransfer ()) break;

				FileList.DoCreateWindow (Backup_pictures);
			}
			break;

		case ID_BACKUP_INVENTORY:
			{
				if (InventoryList.OurHWND != NULL) InventoryList.Done ();

				InventoryList.DoCreateWindow ();
			}
			break;

		case ID_BACKUP_CONTACT:
			{
				if (InventoryList.OurHWND != NULL) ContactList.Done ();

				ContactList.DoCreateWindow ();
			}
			break;

		case ID_BACKUP_CONVERT_TABLES:
			{
				if (FileList.DoingFileTransfer ()) break;
				if (UpdateSQL.DoingTableTransfer ()) break;

				Reply = ShowMessageBox("Nothing to convert!\nPush No if unsure!", "Convert tables", MB_YESNOCANCEL | MB_ICONQUESTION);

				if (Reply != IDYES) break;

				ReportError ("Update Customer", "No table to convert!");
				break;
			}

		case ID_BACKUP_TEST_SQL_WRITE_READ:
			//bool			success;

			//success = mailman.UnlockComponent("JMSMHDMAILQ_PDtV8gkL5P8j");
			//if (success != true) ReportError ("Update Customer", "MailMan didn't unlock");

			////  Set the SMTP server.
			////Office doesn't seem to work
			////https://technet.microsoft.com/en-us/library/dn554323(v=exchg.150).aspx
			////mailman.put_SmtpHost("mahoodandcompany-com.mail.protection.outlook.com");
			////mailman.put_SmtpHost("smtp.office365.com");
			////mailman.put_SmtpUsername("james@mahoodandcompany.com");
			////mailman.put_SmtpPassword("****");
			//////mailman.put_SmtpPort(465);
			////mailman.put_SmtpPort(587);

			////  Set the SMTP login/password (if required)
			//mailman.put_SmtpHost("smtp.gmail.com");
			//mailman.put_SmtpUsername("lacks.environmental@gmail.com");
			//mailman.put_SmtpPassword("spinel52115");
			//mailman.put_SmtpPort(465);


			////Generate and send email
			//email.put_Subject(CustomerName);
			//email.put_Body("Environmental notify test.  This is only a test.");
			//email.put_From("lacks.environmental@gmail.com");
			//email.AddTo("James","James@MahoodAndCompany.com");
			////email.AddTo("Ben","B.Wesley@LacksTrim.com");
			////  To add more recipients, call AddTo, AddCC, or AddBcc once per recipient.

			////  Call SendEmail to connect to the SMTP server and send.
			////  The connection (i.e. session) to the SMTP server remains open so that subsequent SendEmail calls may use the same connection.
			//success = mailman.SendEmail(email);
			//if (success != true)
			//{
			//	ShowMessageBox (mailman.lastErrorText(), "MailMan", MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONSTOP | MB_OK); 
			//}

			////  Some SMTP servers do not actually send the email until the connection is closed.  In these cases, it is necessary to call CloseSmtpConnection for the mail to be  sent.
			////  Most SMTP servers send the email immediately, and it is not required to close the connection.  We'll close it here for the example:
			//success = mailman.CloseSmtpConnection();
			//if (success != true)
			//			ReportError ("Update Customer", "MailMan didn't close");
			//else	break;
			
			//Don't have a primary key so we don't have to worry about duplicate time
			strcpy_s (TestTable.CreateTableStatement, "Create Table [Test] ("
																	"[Date Time] DATETIMEOFFSET NOT NULL,"
																	"[Computer] VARCHAR(16) NOT NULL,"
																	"[Terminal] VARCHAR(16) NOT NULL,"
																	"[Application] VARCHAR(32) NOT NULL,"
																	"[Object] VARCHAR(48) NOT NULL,"
																	"[Message] VARCHAR(256) NOT NULL)");

			TestTable.Initialize ("Test");

			GetDoubleTime (&OurStartTime);
			GetDoubleTime (&FinishTime);

			SetWaitCursor ();
			
			Writes = 0;
			do
			{
				TestTable.StartInsert ();
				TestTable.InsertDoubleTime ("Date Time",	FinishTime);
				TestTable.InsertItem ("Computer",					"Computer");
				TestTable.InsertItem ("Terminal",					"Terminal");
				TestTable.InsertItem ("Application",			"Application");
				TestTable.InsertItem ("Object",						"Object");
				TestTable.InsertItem ("Message",					"Message");
				TestTable.DoInsert ();
				Writes++;
				GetDoubleTime (&FinishTime);
			} while ((FinishTime - OurStartTime) < 5.0);

			SetArrowCursor ();
			ReportError ("Update Customer", "SQL write speed %.0f per seconds", Writes / (FinishTime - OurStartTime));

			//Do read test of the table
			GetDoubleTime (&OurStartTime);
			GetDoubleTime (&FinishTime);

			TestTable.StartQuery(&DateTime);

			QPsprintf_s (TestTable.Statement, "Select [Date Time], [Computer], [Terminal], [Application], [Object],	[Message] From [Test]");

			if (!TestTable.ExecuteStatement ()) break;

			TestTable.BindColumn (SQL_DATETIMEOFFSETdouble, &DateTime, sizeof (DateTime));
			TestTable.BindColumn (SQL_CHAR,		&Computer,				sizeof (Computer));
			TestTable.BindColumn (SQL_CHAR,		&Terminal,				sizeof (Terminal));
			TestTable.BindColumn (SQL_CHAR,		&OurApplicationName,	sizeof (OurApplicationName));
			TestTable.BindColumn (SQL_CHAR,		&Object,					sizeof (Object));
			TestTable.BindColumn (SQL_CHAR,		&Message,					sizeof (Message));

			SetWaitCursor ();
			Reads = 0;
			while (TestTable.ReadRecord())
			{
				Reads++;
			}
			SetArrowCursor ();
			GetDoubleTime (&FinishTime);
			ReportError ("Update Customer", "SQL read speed %.0f,000 per seconds", (Reads / 1000) / (FinishTime - OurStartTime));

			TestTable.DropTable ();
			break;

		//case ID_BACKUP_TEST_SQL_READ:
		//	{
		//		ReportError ("Update Customer", "There is no SQL read test at this time.");

				//Each thread will fill a vector array with 1GB of data.  Server 2008 R2 crashes when the used memory reaches the physical memory.  It just freezes.
				//Server 2012 keeps running until we have consumed 40 GB, 4 times the physical memory, then it detects a low resources condition and identifies us as the most likely culprit then aborts us.  At no time was the OS unstable.
				//Live I/O continued with no interruption, slow scans, or retries.
				//
				//for (I = 0; I <= 100; I++)
				//{
				//	ThreadList.DoCreateThread	(LPTHREAD_START_ROUTINE (StressMemoryThread), 0, 0, "Stress memory");	

				//	SleepSeconds (8);
				//}

			//The combination is about 6% slower.  _int64 operations are about 11.6 faster but string lookups are about 5.8% slower.
				//DATE_TIME	Date;
				//double	OurStartTime;
				//double	StopTime;
				//GetDoubleTime (&OurStartTime);
				//UpdateSQL.DeviceList.AlwaysBuildList (NULL);
				//for (I = 0; I <=1000000; I++)
				//{
				//	J = UpdateSQL.DeviceList.GetIndex (Shuttle_host, "6", More_output, "Speed");
				//	//Date = I * time (NULL);
				//}
				//Date = I * time (NULL);
				//GetDoubleTime (&StopTime);
				//ReportError ("Update Customer", "Time %f", StopTime - OurStartTime);

				//ReportError ("Update Customer", "About to force a string error to test logging.");
				//strcpy_s (Command, "");
				//for (I = 0; I <= 30; I++) strcat_s (Command, "add some more text until it crashes");

				//TestStringClass (this, Command);

				////Don't have a primary key so we don't have to worry about duplicate time
				//strcpy_s (TestTable.CreateTableStatement, "Create Table [Test] ("
				//														"[Date Time] DATETIME NOT NULL,"
				//														"[Computer] VARCHAR(16) NOT NULL,"
				//														"[Terminal] VARCHAR(16) NOT NULL,"
				//														"[Application] VARCHAR(32) NOT NULL,"
				//														"[Object] VARCHAR(48) NOT NULL,"
				//														"[Message] VARCHAR(256) NOT NULL)");

				//TestTable.Initialize ("Test");

				//QPsprintf_s (TestTable.StatusBoxTitle, "Testing Connect for the %s table", TestTable.TableName);

				//TestTable.CreateStatusBox ();

				//ThreadList.DoCreateThread	(LPTHREAD_START_ROUTINE (TestSQLOpenThread), this, 0, "Test SQL");

				////Don't do this it aways crashes with lots of Timeout expired errors after we change CPTimeout
				//for (I = 0; I <= 30; I++)
				//{
				//	//ThreadList.DoCreateThread	(LPTHREAD_START_ROUTINE (TestSQLLibraryThread), this, 0, "Test SQL Library");
				//	//SleepSeconds (0.02);
				//}

			//	break;
			//}

		case ID_SETUP_START_MENU:
			if (FileList.DoingFileTransfer ()) break;

			InstallStartMenu ();
			break;

		case ID_SETUP_POWER_PLAN_RUN:
			ChangePowerPlanToRun ();
			break;

 		case ID_SETUP_POWER_PLAN_SIMULATE:
			ChangePowerPlanToSimulate ();
			break;

		case ID_SETUP_SET_CLOCKS:
			SetClocks (true);
			break;

		case ID_SETUP_AUTO_SIGN_IN_ENABLE:
			if (!StringEqual (AUTO_SIGN_IN_USER, UserName))
			{
				ReportError ("Update Customer", "You must be logged on as the Auto Sign in User!");
				break;
			}

			if (!EnableAutoSignIn ()) break;

			//Put the batch file that starts missing programs in the Startup Group for the Auto Log on user.  On Server 2012 the shortcut to a program that runs as an admin doesn't work.
			//This seems to work if you leave out /AppData/Roaming/Microsoft/Windows/
			BatchFile.Open ();
			BatchFile.Writeln ("DEL \"%%USERPROFILE%%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\Start Missing Programs.lnk\"");
			BatchFile.Writeln ("MD  \"%%USERPROFILE%%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\"");
			BatchFile.Writeln ("XCOPY.EXE /Y \"c:\\Program Files\\Quick Plate\\Plating\\Start Missing Programs.bat\" \"%%USERPROFILE%%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\\"");
			BatchFile.Run ();

			Relaunch ();
			break;

		case ID_SETUP_AUTO_SIGN_IN_DISABLE:
			DisableAutoSignIn ();
			break;

		case ID_CLEAR_TIMEWAYS_RUNNING:
			Reply = ShowMessageBox ("Are you sure?\nTimeways should be hung and not running if you are doing this!",
				"Clear Timeways Running", MB_YESNOCANCEL | MB_ICONQUESTION);

			if (Reply == IDYES) WriteTimewaysRunning (false);
			break;

		//--------------------------------- Source submenu ---------------------------------
		case ID_SOURCE_BACKUP:
		{
			if (FileList.DoingFileTransfer ()) break;

			FileList.DoCreateWindow (Source_backup);
			break;
		}

		case ID_SOURCE_PUBLISH:
		{
			if (FileList.DoingFileTransfer ()) break;

			//Can this do a Git sync?
			ReportError("Update Customer", "No publish customer at this time!");
			break;
		}

		//--------------------------------- Remote submenu ---------------------------------

		//Command not recognized pass to generic command processor.
		default:
     	return (APPLICATION::CommandProc (wParam, lParam));
	}
  return (true);
}
