//---------------------------------------------------------------------------------------
//	File List Object
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
//
//---------------------------------------------------------------------------------------
// Now we can use the minimum ports over the internet.  445 only needs to be open to do network file I/O.  The only port that need to be open are:
//22					SSH for SFTP
//445					SMB for file I/O
//1433				SQL
//3389				RDP Remote desktops
//4018				Remote Debug (possibly other ports used too)
//5900				VNC Virtual desktop
//48300-49399	Quick Plate remote screens for windows programs
//48400-49440	Snap streamed I/O (never over the internet)
//It is easiest to open these ports for TCP and UDP even though UDP doesn't use anything but streamed I/O
//
//Use this to change the RDP port from 3389 to 13389
//HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\TerminalServer\WinStations\RDP - Tcp\PortNumber


#include "File List.h"
#include "Application.h"

#pragma warning (disable : 4100) //Disable unreferenced parameters
#include "/ChilKat Library/chilkat-9.5.0-x86_64-vc2015/include/CkSFtpProgress.h"
#include "/ChilKat Library/chilkat-9.5.0-x86_64-vc2015/include/CkDateTime.h"
#pragma warning (default : 4100)

//Put here to avoid a circular referance
#include "Customer.h"

#include "resource.h" //Menu ID's


//Put here to avoid a circular referance
pCUSTOMER_LIST	OurCustomerList = NULL;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void DoTransferThread (pFILE_LIST pFileList)
{
	pAPPLICATION	Application;

	Application = pAPPLICATION (GetApplicationPointer ());

	pFileList->Transfering = true;

	pFileList->DoTransfer ();

	pFileList->Transfering = false;

	Application->ThreadList.DoExitThread (0);
}

//---------------------------------------------------------------------------------------------
LRESULT CALLBACK StatusDialogProcedure (HWND		hDlg,
																				UINT		msg,
																				WPARAM	wParam,
																				LPARAM	lParam)
{
	pFILE_LIST FileList;

	lParam = lParam;

	FileList = pFILE_LIST (intptr_t (GetWindowLongPtr (hDlg, GWLP_USERDATA)));

	switch (msg)
	{
		case WM_INITDIALOG:
			FileList = pFILE_LIST (lParam);

#pragma warning (disable : 4244)
			SetWindowLongPtr (hDlg, GWLP_USERDATA, intptr_t (FileList));
#pragma warning (default : 4244)

			FileList->StatusBox = hDlg;

			return true; //Set focus
      break;

    case WM_COMMAND:

      switch (LOWORD (wParam))
		{
        case IDABORT:
					FileList->AbortDirectoryBuild = true;

          DestroyWindow (hDlg);

					FileList->StatusBox = NULL;

          return true;

        case IDCANCEL:
					//End of report or the x in the upper right hand corner of the dialog box
					FileList->AbortDirectoryBuild = true; //Pushing the x implies an abort

          DestroyWindow (hDlg);

					FileList->StatusBox = NULL;

          return true;

        case IDCLOSE:
					//Application closed dialog
          DestroyWindow (hDlg);

					FileList->StatusBox = NULL;

          return true;
		}
      break;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------------- 
void FILE_LIST::Setup (pDISPLAY_WINDOW TheCustomerList)
{
	OurCustomerList = pCUSTOMER_LIST (TheCustomerList);
	IgnoreErrors = false; //For attended mode by default
	PriorReportTime = 0;
}

//-------------------------------------------------------------------------------------------------------- 
void FILE_LIST::Initialize (void)

{
	ShowHiddenFiles			= false;
	Transfering					= false;
	StatusBox						= NULL;
	AbortDirectoryBuild	= false;

	DoRegisterClass ("Files", "");
}
//---------------------------------------------------------------------------------------
void FILE_LIST::DoCreateWindow (LIST_TYPE			TheListType)
{
	ListType = TheListType;

	InitDisplay ();
	NoEdit = true;
	NoKeys = true;
	DoRealTimeInvalidation = true;
	  
	DefineFields ();

	NewDisplayTitle ("Files"); //Reset to default title

	ReadAndVerifyRecords ();

	DISPLAY_WINDOW::DoCreateWindow ();
}
 
//---------------------------------------------------------------------------------------
void FILE_LIST::CreateList (LIST_TYPE			TheListType)
{
	ListType = TheListType;

	NoEdit = true;
	NoKeys = true;

	DefineFields (); //So we can sort

	ReadRecords ();

	CloseConnections ();
}

//-----------------------------------------------------------------------------------------
void FILE_LIST::DefineFields (void)
{
	File.push_back (FileItem);

	DefineData (1, //Screen Version
							pVECTOR_ARRAY (&File),
							&FileMax,
							sizeof (File[0]),
							&File[0].SQLRecordControl,
							"");

	LocalPathField = AddStringField ("Local", "Path",
		&File[0].LocalDirInfo.Path, sizeof (File[0].LocalDirInfo.Path), Either_case);

	//Just for debug, makes window too wide and usually this is the same as the Local Path
	//ServerPathField = AddStringField ("Server", "Path",
	//	&File[0].ServerDirInfo.Path, sizeof (File[0].ServerDirInfo.Path), Either_case);

	FileNameField = AddStringField ("", "File",
			&File[0].FileName, sizeof (File[0].FileName), Either_case);

	ComparedToServerField = AddIntegerField ("Compared",	"to Server",
																					LPINT (&File[0].ComparedToServer)); //Enum types are integers, so sort will work

	CopyStatusMax = MAX_COPY_STATUS;

	CopyStatusField = AddEnumeratorField ("Copy", "Status", 
										&File[0].CopyStatus,
										PCHAR (&CopyStatusText[0]),
										sizeof (CopyStatusText),
										CopyStatusMax);

	AddSecondarySortField (LocalPathField,			FileNameField);
	AddSecondarySortField (ComparedToServerField,	LocalPathField);

	FieldsDefined ();
}

//---------------------------------------------------------------------------------------
bool FILE_LIST::ReadRecords (void)
{
	char					LocalPath[MAX_PATH];
	pAPPLICATION	Application;

	Application = pAPPLICATION (GetApplicationPointer ());

	File.clear ();
	FileMax = -1;

	AbortDirectoryBuild = false;
	FillingVectorArray	= true;

	//---------------------------------------------------------------------------------------
	if (ListType == Local_services_list)
	{
		OpenConnections (IPAddress, "");
		FillingVectorArray	= true;

		strcpy_s (LocalPath, "/Program Files/Quick Plate/Plating/");

		if (!AddToList (LOCAL_COMPUTER, LocalPath, LocalPath, No_subdirectories)) return false;

		QPsprintf_s (LocalPath, "/Program Files/Quick Plate/%s/", CustomerName);

		if (!AddToList (LOCAL_COMPUTER, LocalPath, LocalPath, No_subdirectories)) return false;

		UpdateVectorArray ();
		FillingVectorArray	= false;
		CloseConnections ();
	}
	//---------------------------------------------------------------------------------------
	else //Other list generated in a thread
	{
		//Slow lists, start a thread which will do the update, backup, or cleanup when done

		//Display the modeless dialog box. 
		//AddToList will put text in dialog box

		StatusBox = CreateDialogParam (OurInstance,
																	MAKEINTRESOURCE (FILE_LIST_STATUS_BOX),
																	ClientWindow,
																	DLGPROC (StatusDialogProcedure),
																	LPARAM (this));

		Transfering = true; //Make sure this is set before we return

		//Start a thread to do transfer
		Application->ThreadList.DoCreateThread (LPTHREAD_START_ROUTINE (DoTransferThread), this, 0, "Do Transfer");
	}

	return true;
}

//---------------------------------------------------------------------------------------
void FILE_LIST::WriteRecords(void)
{
}
 
//---------------------------------------------------------------------------------------
void FILE_LIST::PaintField (void)
{
	char	Percent[30];

	if (FieldIndex == LocalPathField)
	{
		PaintString (File[RecordIndex].LocalDirInfo.Path);
	}

	else if (FieldIndex == ComparedToServerField)
	{
		PaintString (PCHAR (&ComparedToServerText[File[RecordIndex].ComparedToServer]));
	}
 	else if (FieldIndex == CopyStatusField)
	{
		if (File[RecordIndex].CopyStatus == Coping)
		{
			QPsprintf_s (Percent, "%d%%", File[RecordIndex].PercentCopied);
			PaintString (Percent);
		}
		else PaintString (PCHAR (&CopyStatusText[File[RecordIndex].CopyStatus]));
	}

	else DISPLAY_WINDOW::PaintField ();
}

//---------------------------------------------------------------------------------------
bool FILE_LIST::CheckField (void)
{
	return true;
}

//---------------------------------------------------------------------------------------
void FILE_LIST::UpdateStatusBox (PCHAR	Message)
{
	DATE_TIME	CurrentTime;

	time (&CurrentTime);

	if (StatusBox != NULL)
	{
		if (CurrentTime >= (PriorStatusUpdateTime + 1))
		{
			PriorStatusUpdateTime = CurrentTime;

			if (!SetDlgItemText (StatusBox, IDC_LOG_READING, Message))
			{
				SimulationReportLastError ("File List", "SetDlgItemText");
			}
		}
	}
}

//---------------------------------------------------------------------------------------
void FILE_LIST::CloseStatusBox (void)
{
	if (StatusBox != NULL)
	{
		//This will close the dialog box.  We can not call DestroyWindow.
		if (!PostMessage (StatusBox, WM_COMMAND, IDCLOSE, 0)) SimulationReportLastError ("File List", "PostMessage");

		StatusBox = NULL;
	}
}

//-----------------------------------------------------------------------------------------
void FILE_LIST::Cut(void)
{
	ShowError ("Cut not allowed!");
}

//-----------------------------------------------------------------------------------------
bool FILE_LIST::CanClose (void)
{
	int		Response;
	int		WaitCount;

	if (Transfering)
	{
		Response = ShowMessageBox ("File transfer in process! Do you want to stop it?",
															"File List",
															MB_TASKMODAL || MB_OKCANCEL); 

		if (Response == IDCANCEL) return false;
		else
		{
			SetWaitCursor ();

			CancelTransfer	= true;
			WaitCount				= 40; //20 sec
			do
			{
				Sleep (500);
				WaitCount--;
			} while ((Transfering == true) && (WaitCount > 0));

			if (WaitCount == 0)
			{
				//Transfer didn't stop by itself, phone line probably went down
				Transfering = false;

				//This call is unsafe.  Assume this won't be a problem
				//if (BackupThread != NULL) TerminateThread (BackupThread, 1);
				//if (UpdateThread != NULL) TerminateThread (UpdateThread, 1);
				//if (BuildListThread != NULL) TerminateThread (BuildListThread, 1);
			}

			CancelTransfer = false;

			SetArrowCursor ();

			CloseConnections ();

			return DISPLAY_WINDOW::CanClose ();
		}
	}

	CloseConnections ();

	return DISPLAY_WINDOW::CanClose ();
}

//-----------------------------------------------------------------------------------------
bool FILE_LIST::DoingFileTransfer (void)
{
	if (Transfering)
	{
		ReportError ("File List", "File transfer in process, try again later.");

		return true;
	}
	else return false;
}


//-------------------------------------------------------------------------------------------------------- 
bool FILE_LIST::OpenConnections (PCHAR	TheLocalComputerName,
																PCHAR		TheServerComputerName)
{
	int			Success;
	char		OurUserName[80];
	char		Password[80];

	//Pass the IP address for the local computer because the library is using the Snap NIC rather than the main nick which may cause some problems.

	//Documentation on calls
	//http://www.chilkatsoft.com/refdoc/vcCkSFtpRef.html

	//Unlock Codes received on 8/7/2013 with support renewal.
	//Chilkat Email: JMSMHDMAILQ_PDtV8gkL5P8j
	//Chilkat IMAP: JMSMHDIMAPMAILQ_BzNTXfEs1P1s
	//Chilkat Bounce: JMSMHDBOUNCE_xO6Omv5ZmM8c
	//Chilkat FTP2: JMSMHDFTP_nEhVteETlWnR
	//Chilkat HTTP: JMSMHDHttp_Nd65avAuZR2g
	//Chilkat Zip: JMSMHDZIP_Z5CDdRQS3Vxf
	//Chilkat Crypt: JMSMHDCrypt_ORc7E9eNLR7u
	//Chilkat RSA: JMSMHDRSA_pEH5gym0oUxI
	//Chilkat DSA: JMSMHDDSA_2v05RcYcmCyw
	//Chilkat SSH/SFTP: JMSMHDSSH_whCL4nNkmRn9
	//Chilkat Diffie-Hellman: JMSMHDDiffie_5aee5xraSR3y
	//Chilkat Compression: JMSMHDCompress_Tp7MSsC7OT1u
	//Chilkat MHT: JMSMHDMHT_Hkh396eJ2DqJ
	//Chilkat Mime: JMSMHDSMIME_t45ZZTsP7B7I
	//Chilkat HTML-to-XML: JMSMHDHtmlToXml_wko2hykkZD2e
	//Chilkat Socket: JMSMHDSocket_4O0hgvMLkJkR
	//Chilkat Charset: JMSMHDCharset_8954fxhmTR7h
	//Chilkat XMP: JMSMHDXMP_e7rhFF2U7Pwv
	//Chilkat TAR: JMSMHDTarArch_85FLYdRp1RJo
	//
	//Here's a Bundle unlock code in addition:
	//	MAHOOD.CB10517_ngPB5DT68ooX
	//	You can use it with the new Chilkat.Global class..


  //  Purchased forever license 10/10/2011
	Success = LocalSFTP.UnlockComponent ("JAMESASSH_2b8Tp5B89YnL");
  if (Success != TRUE)
	{
		ReportError ("File List", "Didn't unlock library");
		return false;
  }
	Success = ServerSFTP.UnlockComponent ("JAMESASSH_2b8Tp5B89YnL");
  if (Success != TRUE)
	{
		ReportError ("File List", "Didn't unlock library");
		return false;
  }

	if (!StringEqual (LocalComputerName, TheLocalComputerName))
	{
		LocalSFTP.Disconnect ();
		strcpy_s (LocalComputerName, "");

		//Connect to local SFTP
		//  Set some timeouts, in milliseconds:
		LocalSFTP.put_ConnectTimeoutMs (30000);
		LocalSFTP.put_IdleTimeoutMs (30000);
		LocalSFTP.put_PreserveDate(1); //Preserver the last modified date and time
		LocalSFTP.put_UtcMode(true); //Keep everything in UTC

		// Connect to the SSH server.
		// The standard SSH port = 22
		// The hostname may be a hostname or IP address.
		Success = LocalSFTP.Connect (TheLocalComputerName, 22);
		if (Success != TRUE)
		{
			ReportError ("File List", "BitVise Local Open failed to %s", TheLocalComputerName);
			exit (1); //Returning will just crash on OpenDir then vector subscrip out of range will abort.
		}

		//Authenticate with the SSH server.  The .local seems to be more reliable but not allways needed.
		QPsprintf_s (OurUserName, "%s\\%s", ComputerName, AUTO_SIGN_IN_USER);
		strcpy_s (Password, AUTO_SIGN_IN_PASSWORD);

		Success = LocalSFTP.AuthenticatePw (OurUserName, Password);
		if (Success != TRUE)
		{
			ReportError ("File List", "BitVise Local Authenticate failed");
			ShowSFTPError ("AuthenticatePw", &LocalSFTP);
			exit (1); //Returning will just crash on OpenDir then vector subscrip out of range will abort.
		}

		// After authenticating, the SFTP subsystem must be initialized:
		Success = LocalSFTP.InitializeSftp ();
		if (Success != TRUE)
		{
			ReportError ("File List", "BitVise Local Initialize SFTP failed");
			exit (1); //Returning will just crash on OpenDir then vector subscrip out of range will abort.
		}
		strcpy_s (LocalComputerName, TheLocalComputerName);

		//We only download from server so just set this on ServerSFTP.  Uploads work ok.
	}

	if (!StringEqual (ServerComputerName, TheServerComputerName))
	{
		//ServerSFTP.put_VerboseLogging (true);

		ServerSFTP.Disconnect ();
		strcpy_s (ServerComputerName, "");

		if (StringEqual (TheServerComputerName, "")) return true; //No server

		//Connect to server SFTP
		//  Set some timeouts, in milliseconds:
		ServerSFTP.put_ConnectTimeoutMs (5000);
		ServerSFTP.put_IdleTimeoutMs (30000);
		ServerSFTP.put_PreserveDate(true); //Preserver the last modified date and time
		ServerSFTP.put_UtcMode (true); //Keep everything in UTC

		// Connect to the SSH server.
		// The standard SSH port = 22
		// The hostname may be a hostname or IP address.
		if (StringInString(TheServerComputerName, "MahoodHome") || //DuckDNS
				StringInString(TheServerComputerName, "MahoodAndCompany.com")) //GoDaddy DNS
					Success = ServerSFTP.Connect(TheServerComputerName, 10022); //Hide the home backup computer from hackers
		else	Success = ServerSFTP.Connect(TheServerComputerName, 22);

		if (Success != TRUE)
		{
			if (IgnoreErrors) return false;

			ReportError ("File List", "BitVise Server Open failed to: %s", TheServerComputerName);
			ShowSFTPError ("AuthenticatePw", &ServerSFTP);
			exit (1); //Returning will lead to many errors and crash eventually.
		}

		//Authenticate with the SSH server.
		if (!StringEqual (RemoteCustomerName, ""))
		{
			if (StringEqual (RemoteComputerName, ""))
						QPsprintf_s (OurUserName, "%s", RemoteUserName);
			else	QPsprintf_s (OurUserName, "%s\\%s", RemoteComputerName, RemoteUserName);
			strcpy_s (Password, RemotePassword);
		}
		else
		{
			//QPsprintf_s (OurUserName, "%s.local\\%s", ComputerName, Configuration.SFTPuser);
			QPsprintf_s (OurUserName, "%s\\%s", ComputerName, AUTO_SIGN_IN_USER);
			strcpy_s (Password, AUTO_SIGN_IN_PASSWORD);
		}

		Success = ServerSFTP.AuthenticatePw (OurUserName, Password);
		if (Success != TRUE)
		{
			ReportError ("File List", "BitVise Server Authenticate failed %s %s %s", TheServerComputerName, OurUserName, Password);
			exit (1); //Returning will lead to many errors and crash eventually.
		}

		// After authenticating, the SFTP subsystem must be initialized:
		Success = ServerSFTP.InitializeSftp ();
		if (Success != TRUE)
		{
			ReportError ("File List", "BitVise Server Initialize SFTP failed");
			exit (1); //Returning will lead to many errors and crash eventually.
		}
		strcpy_s (ServerComputerName, TheServerComputerName);

		//These may work with version 9.5.0
		//With an older version neither of these work.  The times don't get preserved because of a UTC to local time bug and the UTC Mode doesn't work.  We get times in local time.
		//This messes up files with dates in the winter.  They end up one hour off.
		//ServerSFTP.put_PreserveDate (true); //Default is false for some reason, we want our time stamps to be the same as the server
		//ServerSFTP.put_UtcMode (true); //Keep everything in UTC
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------- 
void FILE_LIST::CloseConnections (void)
{
	LocalSFTP.Disconnect ();
	strcpy_s (LocalComputerName, "");

	ServerSFTP.Disconnect ();
	strcpy_s (ServerComputerName, "");
}

//-------------------------------------------------------------------------------------------------------- 
void FILE_LIST::ShowSFTPError (PCHAR	Title,
															CkSFtp	*ErrorSFTP)
{
	ShowMessageBox (ErrorSFTP->lastErrorText(), Title, MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONSTOP | MB_OK); 
}

//---------------------------------------------------------------------------------------
bool FILE_LIST::AddToList (	bool				FileServerComputer,
														PCHAR 			LocalPath,
														PCHAR 			ServerPath,
														LOOKUP_TYPE	LookupType)

{
	int										I;
	char									SearchName[MAX_PATH + 1];
	char									NewLocalPath[MAX_PATH + 1];
	char									NewServerPath[MAX_PATH + 1];
	CkString							CKStringHandle;
	const char						*SearchHandle;
	BOOL									FileFound;
	DIRECTORY_ITEM				NewFile;
	int										FileIndex;
	CkSFtpDir							*DirectoryListing = 0;
	CkSFtpFile						*FileObject = 0;
	int										Entries;
	bool									Success;
	CkDateTime						*FileDateTime;

	if (FileServerComputer)
				UpdateStatusBox (ServerPath);
	else	UpdateStatusBox (LocalPath);

	//If they push abort don't do any more reading
	if (AbortDirectoryBuild)
	{
		return false;
	}

	if (FileServerComputer == FILE_SERVER_COMPUTER)
				QPsprintf_s (SearchName, "C:%s", ServerPath);
	else	QPsprintf_s (SearchName, "C:%s", LocalPath);

	FileFound = true;

	if (FileServerComputer == FILE_SERVER_COMPUTER)
				Success = ServerSFTP.OpenDir (SearchName, CKStringHandle);
	else	Success = LocalSFTP.OpenDir (SearchName, CKStringHandle);

	if (!Success)
	{
		//Try to create the folder
		if (FileServerComputer == FILE_SERVER_COMPUTER)
					Success = ServerSFTP.CreateDir (SearchName);
		else	Success = LocalSFTP.CreateDir (SearchName);

		if (!Success &&
				!StringInString (PCHAR (ServerSFTP.lastErrorText()), "already exists")) //Actually "The file already exists" but should be folder.  Either way it isn't an error
		{
			if (FileServerComputer == FILE_SERVER_COMPUTER)
						ShowSFTPError ("CreateDir", &ServerSFTP);
			else	ShowSFTPError ("CreateDir", &LocalSFTP);
			return false;
		}

		//Try to open it again, it should be there now.
		if (FileServerComputer == FILE_SERVER_COMPUTER)
					Success = ServerSFTP.OpenDir (SearchName, CKStringHandle);
		else	Success = LocalSFTP.OpenDir (SearchName, CKStringHandle);
	}

	if (!Success)
	{
		FileFound	= false;

		if (FileServerComputer == FILE_SERVER_COMPUTER)
					ShowSFTPError ("openDir", &ServerSFTP);
		else	ShowSFTPError ("openDir", &LocalSFTP);
		CloseStatusBox ();
		return false;
	}

	SearchHandle = CKStringHandle.getString ();

	//  Download the directory listing:
	if (FileServerComputer == FILE_SERVER_COMPUTER)
	{
		DirectoryListing = ServerSFTP.ReadDir (SearchHandle);
	}
	else
	{
		DirectoryListing = LocalSFTP.ReadDir (SearchHandle);
	}

	//This is a better place to close the handle because we are done with it but this still gives Invalid Handle errors if we call AddToList recursivly.
	if (FileServerComputer == FILE_SERVER_COMPUTER)
	{
		if (!ServerSFTP.CloseHandle (SearchHandle)) ShowSFTPError ("CloseHandle", &ServerSFTP);
	}
	else
	{
		if (!LocalSFTP.CloseHandle (SearchHandle)) ShowSFTPError ("CloseHandle", &LocalSFTP);
	}

	if (DirectoryListing == 0)
	{
		if (FileServerComputer == FILE_SERVER_COMPUTER)
					ShowSFTPError ("ReadDir", &ServerSFTP);
		else	ShowSFTPError ("ReadDir", &LocalSFTP);
		CloseStatusBox ();
		return false;
	}

	Entries = DirectoryListing->get_NumFilesAndDirs ();

	if (Entries == 0) return true; //No entries

	for (I = 0; I <= Entries - 1; I++)
	{
		FileObject = DirectoryListing->GetFileObject(I);

		if (FileObject == NULL) return true; //Should never happen

		ZeroMemory (&NewFile, sizeof (NewFile));
		QPsprintf_s (NewFile.FileName, "%s", FileObject->filename());

		FileDateTime = FileObject->GetLastModifiedDt ();
		FileDateTime->GetAsFileTime (false, NewFile.LastWriteTime.FileTime); 


		FileDateTime = FileObject->GetLastAccessDt ();
		FileDateTime->GetAsFileTime (false, NewFile.LastAccessTime.FileTime); 

		FileDateTime = FileObject->GetCreateDt ();
		FileDateTime->GetAsFileTime (false, NewFile.CreateTime.FileTime); 

		NewFile.FileSize = FileObject->get_Size64 ();

		if (FileServerComputer == FILE_SERVER_COMPUTER)
					strcpy_s (NewFile.Path, ServerPath);
		else	strcpy_s (NewFile.Path, LocalPath);

		//Ignore system and hidden files.
		if ((StringPosition (NewFile.FileName, ".") != 0)	&&
			  (StringPosition (NewFile.FileName, "..") != 0) &&
				(ShowHiddenFiles || !FileObject->get_IsHidden ()))
		{	
			//Check Directory bit in attributes, other bits may be set
			if (FileObject->get_IsDirectory ())
			{
				//Skip all these.  They are recoverable and frequently very large.
				if (!StringEqual (NewFile.FileName,			"Release") &&						//C++ project objects
						!StringEqual (NewFile.FileName,			"Debug") &&							//C++ project objects
						!StringEqual (NewFile.FileName,			"obj") &&								//Visual Basic project objects
						!StringEqual (NewFile.FileName,			"x64") &&								//object folder
						!StringEqual (NewFile.FileName,			"ipch") &&							//Visual studio junk
						!StringEqual (NewFile.FileName,			"VSMacros80") &&				//Visual studio junk
						!StringInString (NewFile.FileName,	"iPod Photo Cache") &&	//iPod junk
						!StringInString (NewFile.FileName,	"Previews.lrdata") &&		//Lightroom previews
						!StringInString (NewFile.FileName,	"Documents") &&					//Customer documents and pictures
						!StringEqual (NewFile.FileName,			"!SkinSubFolder!") &&		//RoboHelp junk
						!StringEqual (NewFile.FileName,			"!Language!") &&				//RoboHelp junk
						!StringEqual(NewFile.FileName,			"!SSL!") &&							//RoboHelp junk
						!StringInString (NewFile.FileName,	"backups"))							//Lightroom backup folders
				{
					//The new path is the current path and the new directory
					QPsprintf_s (NewLocalPath,	"%s%s/", LocalPath, NewFile.FileName);
					QPsprintf_s (NewServerPath, "%s%s/", ServerPath, NewFile.FileName);

					FileIndex = GetIndex (NewLocalPath); //Find the entry for the directory
					if (FileIndex < 0)
					{
						//New directory, add it to the list
						File.push_back (FileItem);
						UpdateVectorArray ();

						FileIndex = FileMax;

						ZeroMemory (&File[FileIndex], sizeof (File[FileIndex]));
						File[FileIndex].CopyStatus = Copy_unknown;

						File[FileIndex].ComparedToServer = Directory;

						//Make sure both paths get set for folders that are not in both places
						strcpy_s (File[FileIndex].ServerDirInfo.Path, NewServerPath);
						strcpy_s (File[FileIndex].LocalDirInfo.Path, NewLocalPath);

						strcpy_s (File[FileIndex].FileName, ""); //Directories don't have a file name, it is confusing on the screen
					}

					//Save the directory name and where it came from then set the DirectoryComparison
					if (FileServerComputer == FILE_SERVER_COMPUTER)
								strcpy_s (File[FileIndex].ServerDirInfo.FileName, NewFile.FileName);
					else	strcpy_s (File[FileIndex].LocalDirInfo.FileName, NewFile.FileName);

					if (StringEqual (File[FileIndex].LocalDirInfo.FileName, File[FileIndex].ServerDirInfo.FileName)) File[FileIndex].DirectoryComparison = Same;
					if (!StringEqual (File[FileIndex].LocalDirInfo.FileName, "") && StringEqual (File[FileIndex].ServerDirInfo.FileName, "")) File[FileIndex].DirectoryComparison = Surplus;
					if (StringEqual (File[FileIndex].LocalDirInfo.FileName, "") && !StringEqual (File[FileIndex].ServerDirInfo.FileName, "")) File[FileIndex].DirectoryComparison = Missing;

					//Usually expand directory in case server has different files
					if (LookupType != No_subdirectories)
					{
						if (!AddToList (FileServerComputer, NewLocalPath, NewServerPath, LookupType))
						{
							return false;
						}
					}
				}
			}
			else if (LookupType != Directories_only)
			{
				//Look up entry first
				FileIndex = GetIndex (LocalPath, NewFile.FileName);

				if (FileIndex < 0)
				{
					//Not found add to end
					File.push_back (FileItem);
					UpdateVectorArray ();

					FileIndex = FileMax;

					ZeroMemory (&File[FileIndex], sizeof (File[FileIndex]));
					File[FileIndex].CopyStatus = Copy_unknown;
				}

				if (FileServerComputer == FILE_SERVER_COMPUTER)
							File[FileIndex].ServerDirInfo	= NewFile;
				else	File[FileIndex].LocalDirInfo	= NewFile;

				//Put the right path back in. It gets wiped out copying NewFile to get the file times.
				strcpy_s (File[FileIndex].ServerDirInfo.Path, ServerPath);
				strcpy_s (File[FileIndex].LocalDirInfo.Path, LocalPath);

				strcpy_s (File[FileIndex].FileName, NewFile.FileName);
			}
		}
		delete FileObject;
	}

	delete DirectoryListing;

	UpdateVectorArray ();

	return true;
}

//-----------------------------------------------------------------------------------------
void FILE_LIST::DoTransfer (void)
{
	int						I;
	char					LocalPath[MAX_PATH + 1];
	char					ServerPath[MAX_PATH + 1];
	pAPPLICATION	Application;
	int						Index;

	Application = pAPPLICATION (GetApplicationPointer ());

	//---------------------------------------------------------------------------------------------------------------------------
	if (ListType == Publish_customer)
	{
		OpenConnections (IPAddress, RemoteSQLServerName);
		FillingVectorArray = true;

		//Add plating to the list
		strcpy_s (LocalPath, "/Program Files/Quick Plate/Plating/");
		strcpy_s (ServerPath, "/Program Files/Quick Plate/Publish/Plating/");
		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, All_entries)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, All_entries)) return;

		Index = OurCustomerList->GetIndex (RemoteCustomerName);

		//Add all customers that have the same Publish server
		for (I = 0; I <= OurCustomerList->CustomerMax; I++)
		{
			if (StringEqual (OurCustomerList->Customer[Index].PublishServer, OurCustomerList->Customer[I].PublishServer))
			{
				QPsprintf_s (LocalPath, "/Program Files/Quick Plate/%s/", OurCustomerList->Customer[I].Name);
				QPsprintf_s (ServerPath, "/Program Files/Quick Plate/Publish/%s/", OurCustomerList->Customer[I].Name);
				if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, No_subdirectories)) return;
				if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, No_subdirectories)) return;

				if (!StringEqual (OurCustomerList->Customer[I].Name, OurCustomerList->Customer[I].PublishServer))
				{
					OurCustomerList->Customer[I].LastPublished = 0; //Will go blank on all except the actual publish computer
					OurCustomerList->SetSQLRecordModified (I, true);
					OurCustomerList->WriteRecords ();
				}
			}
		}

		UpdateVectorArray ();

		//Delete files not to backup
		I = 0;
		while (I <= FileMax)
		{
			if (StringInString (File[I].FileName, ".bsc") ||	//Visual Studio junk
					StringInString (File[I].FileName, ".suo"))		//Visual Studio junk
						DeleteRecords (I, 1);
			else	I++;
		}

		Sort (LocalPathField);
		CompareToServer ();
		RealTimeAdjustRange ();

		FillingVectorArray = false;

		DoCleanup (Server_computer);

		DoBackup ();
	}
	//---------------------------------------------------------------------------------------
	else if (ListType == Source_publish)
	{
		OpenConnections (IPAddress, RemoteSQLServerName);

		FillingVectorArray = true;

		strcpy_s (LocalPath, "/users/public/Documents/Quick Plate/");
		strcpy_s (ServerPath, "/users/public/Documents/Quick Plate/");

		ShowHiddenFiles = true;

		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, All_entries)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, All_entries)) return;

		ShowHiddenFiles = false;

		UpdateVectorArray ();

		//Delete large useless files
		I = 0;
		while (I <= FileMax)
		{
			if (!StringInString (File[I].FileName, ".sdf") && //Large SQL file for Visual Studio.  It is for browsing and will regenerate.
					!StringInString (File[I].FileName, ".opensdf") && //Same, only present when Visual Studio is open
					!StringInString (File[I].FileName, ".suo")) //Local user settings that won't write because they are hidden
						I++;
			else	DeleteRecords (I, 1);
		}

		Sort (LocalPathField);
		CompareToServer ();
		RealTimeAdjustRange ();

		FillingVectorArray = false;

		DoBackup ();
		strcpy_s(RemoteCustomerName, "");
		SetRemoteCredentials("", "", "");
	}
	//---------------------------------------------------------------------------------------
	else if (ListType == Source_backup)
	{
		BATCH_FILE	BatchFile;

		//The IOS app is discontinued so we don't need to back this up anymore
		////Copy iMac files and server backup files into the source area for backup.  We could copy directly from these areas but that would be confusing later.  It is best to have matching folders here and there.
		////\\imac\ has a manual A record in DNS.  Otherewise it keeps vanishing and \\imac NetBIOS but \\imac.local doesn't need NetBIOS.
		//BatchFile.Open ();
		////BatchFile.Writeln ("XCOPY.EXE /Y  \"\\\\%s\\c$\\Program Files\\Microsoft SQL Server\\MSSQL10.MSSQLSERVER\\MSSQL\\Backup\\*.bak\" \"c:\\users\\public\\Documents\\Quick Plate backups\\SQL\\*.* \"", ComputerName);
		//BatchFile.Writeln ("XCOPY.EXE /Y /S  \"\\\\iMac\\mahood\\documents\\QuickPlate\\*.*\" \"c:\\users\\public\\Documents\\Quick Plate backups\\iMac\\*.* \"");
		//BatchFile.Run ();

		//OpenConnections (Application->SocketList.OurIPAddress, ComputerName);
		Index = OurCustomerList->GetIndex ("Home Backup");

		if (Index < 0)
		{
			ReportError ("File List", "Home Backup customer not found!");
			return;
		}
		strcpy_s (RemoteCustomerName, OurCustomerList->Customer[Index].Name); 
		SetRemoteCredentials (OurCustomerList->Customer[Index].UserName, OurCustomerList->Customer[Index].LineComputerName, OurCustomerList->Customer[Index].Password);

		if (StringEqual (OurCustomerList->Customer[Index].RemoteSQLIPv6Address, ""))
					OpenConnections (IPAddress, OurCustomerList->Customer[Index].RemoteSQLIPAddress);
		else	OpenConnections (IPAddress, OurCustomerList->Customer[Index].RemoteSQLIPv6Address);

		FillingVectorArray	= true;
		ShowHiddenFiles			= true;

		strcpy_s (LocalPath,	"/users/public/Documents/Quick Plate/");
		strcpy_s (ServerPath, "/users/public/Documents/Quick Plate/");
		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, All_entries)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, All_entries)) return;

		//No more iMac or SQL backup being done
		//strcpy_s (LocalPath,	"/users/public/Documents/Quick Plate backups/");
		//strcpy_s (ServerPath, "/users/public/Documents/Quick Plate backups/");
		//if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, All_entries)) return;
		//if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, All_entries)) return;

		strcpy_s (LocalPath,	"/users/public/Documents/Intuit/QuickBooks/Company Files/");
		strcpy_s (ServerPath, "/users/public/Documents/Intuit/QuickBooks/Company Files/");
		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, All_entries)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, All_entries)) return;

		//Quick Plate unsuported source changes rarely so it can be backed up by hand.

		ShowHiddenFiles = false;

		UpdateVectorArray ();

		//Delete large useless files
		I = 0;
		while (I <= FileMax)
		{
			if (!StringInString (File[I].FileName, ".sdf") &&							//Large SQL file for Visual Studio.  It is for browsing and will regenerate.
					!StringInString (File[I].FileName, ".opensdf") &&					//Same, only present when Visual Studio is open
					!StringInString (File[I].FileName, "Quick Plate.lib") &&	//Very large and doesn't need to be backed up
					!StringInString (File[I].FileName, ".ADR") &&							//Very large and probably useless for recovery
					!StringInString (File[I].LocalDirInfo.Path, ".SearchIndex"))		//Useless QuickBooks files that keep changing
						I++;
			else	DeleteRecords (I, 1);
		}

		Sort (LocalPathField);
		CompareToServer ();
		RealTimeAdjustRange ();

		FillingVectorArray = false;

		DoBackup ();
		strcpy_s (RemoteCustomerName, ""); 
		SetRemoteCredentials ("", "", "");
	}
	//---------------------------------------------------------------------------------------
	else if (ListType == Backup_pictures)
	{
		//OpenConnections (Application->SocketList.OurIPAddress, ComputerName);
		Index = OurCustomerList->GetIndex ("Home Backup");

		if (Index < 0)
		{
			ReportError ("File List", "Home Backup customer not found!");
			return;
		}
		strcpy_s (RemoteCustomerName, OurCustomerList->Customer[Index].Name); 
		SetRemoteCredentials (OurCustomerList->Customer[Index].UserName, OurCustomerList->Customer[Index].LineComputerName, OurCustomerList->Customer[Index].Password);

		if (StringEqual (OurCustomerList->Customer[Index].RemoteSQLIPv6Address, ""))
					OpenConnections (IPAddress, OurCustomerList->Customer[Index].RemoteSQLIPAddress);
		else	OpenConnections (IPAddress, OurCustomerList->Customer[Index].RemoteSQLIPv6Address);

		FillingVectorArray = true;

		strcpy_s (LocalPath,	"/users/public/Pictures/");
		strcpy_s (ServerPath, "/users/public/Pictures/");

		//Preview folders aren't collected 
		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, All_entries)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, All_entries)) return;

		UpdateVectorArray ();

		//Delete files not to backup
		I = 0;
		while (I <= FileMax)
		{
			if (StringInString(File[I].FileName, "Thumbs.db"))
						DeleteRecords(I, 1);
			else	I++;
		}

		Sort (LocalPathField);
		CompareToServer ();
		RealTimeAdjustRange ();

		FillingVectorArray = false;

		DoBackup ();
		strcpy_s (RemoteCustomerName, ""); 
		SetRemoteCredentials ("", "", "");
	}
	//---------------------------------------------------------------------------------------
	else if (ListType == Update_publish_from_server)
	{
		if (StringEqual (RemoteCustomerName, ""))
					OpenConnections (IPAddress, ComputerName);
		else	OpenConnections (IPAddress, RemoteSQLServerName);

		FillingVectorArray	= true;

		strcpy_s (LocalPath,	"/Program Files/Quick Plate/Publish/Plating/");
		strcpy_s (ServerPath, "/Program Files/Quick Plate/Publish/Plating/");
		AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, No_subdirectories);
		AddToList (FILE_SERVER_COMPUTER,	LocalPath, ServerPath, No_subdirectories);

		QPsprintf_s (LocalPath,		"/Program Files/Quick Plate/Publish/%s/", CustomerName);
		QPsprintf_s (ServerPath,	"/Program Files/Quick Plate/Publish/%s/", CustomerName);
		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, No_subdirectories)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, No_subdirectories)) return;

		UpdateVectorArray ();

		Sort (LocalPathField);
		CompareToServer ();
		RealTimeAdjustRange ();

		FillingVectorArray	= false;

		DoCleanup (Local_computer);

		DoUpdate ();

		//Go back to main program to finish the update
		PostMessage (FrameWindow, WM_COMMAND, ID_PROGRAMS_INSTALL_UPDATE_FINISH, 0);
	}
	//---------------------------------------------------------------------------------------
	else if (ListType == Install_update)
	{
		if (StringEqual (RemoteCustomerName, ""))
					OpenConnections (IPAddress, ComputerName);
		else	OpenConnections (IPAddress, RemoteSQLServerName);

		FillingVectorArray = true;

		//Add plating to the list
		strcpy_s (LocalPath, "/Program Files/Quick Plate/Plating/");
		strcpy_s (ServerPath, "/Program Files/Quick Plate/Publish/Plating/");
		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, No_subdirectories)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, No_subdirectories)) return;

		QPsprintf_s (LocalPath, "/Program Files/Quick Plate/%s/", CustomerName);
		QPsprintf_s (ServerPath, "/Program Files/Quick Plate/Publish/%s/", CustomerName);
		if (!AddToList (LOCAL_COMPUTER,				LocalPath, ServerPath, No_subdirectories)) return;
		if (!AddToList (FILE_SERVER_COMPUTER, LocalPath, ServerPath, No_subdirectories)) return;

		UpdateVectorArray ();

		Sort (LocalPathField);
		CompareToServer ();
		RealTimeAdjustRange ();

		FillingVectorArray = false;

		DoCleanup (Local_computer);

		DoUpdate ();
	}
	//---------------------------------------------------------------------------------------
	else if (ListType == Update_data_from_remote)
	{
		OpenConnections (ComputerName, RemoteSQLServerName);
		FillingVectorArray = true;

		if (Features.CustomerType == Painting_customer)
		{
			//Only Painting lines uses the Corporate schedule
			//Don't return false if Exchange isn't there
			QPsprintf_s (LocalPath, "/Program Files/Quick Plate/%s/Exchange/", CustomerName);

			AddToList (LOCAL_COMPUTER,				LocalPath, LocalPath, No_subdirectories);
			AddToList (FILE_SERVER_COMPUTER,	LocalPath, LocalPath, No_subdirectories);
		}

		UpdateVectorArray ();

		Sort (LocalPathField);
		CompareToServer ();
		RealTimeAdjustRange ();

		FillingVectorArray = false;

		DoUpdate ();
	}
	else
	{
		ReportError ("File List", "Unexpected ListType %d", ListType);
		return;
	}

	SetupDifferentCustomer ("", "");
	SetRemoteCredentials ("", "", "");
}

//-----------------------------------------------------------------------------------------
void FILE_LIST::CompareToServer (void)

{
	int				I;
	_int64		FileTimeDifference;

	for (I = 0; I <= FileMax; I++)
	{
		FileTimeDifference = File[I].LocalDirInfo.LastWriteTime.Time64 - File[I].ServerDirInfo.LastWriteTime.Time64;

				 if (File[I].ComparedToServer == Directory)														File[I].ComparedToServer = Directory;
		else if (StringEqual (File[I].LocalDirInfo.FileName, ""))									File[I].ComparedToServer = Missing;
		else if (StringEqual (File[I].ServerDirInfo.FileName, ""))								File[I].ComparedToServer = Surplus;
		else if (FileTimeDifference > 20000000)																		File[I].ComparedToServer = Newer;
		else if (FileTimeDifference < -20000000)																	File[I].ComparedToServer = Older;
		else if (File[I].LocalDirInfo.FileSize != File[I].ServerDirInfo.FileSize)	File[I].ComparedToServer = Different_size;
	}
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void DoUpdateThread (pFILE_LIST pFileList)
{
	pAPPLICATION	Application;

	Application = pAPPLICATION (GetApplicationPointer ());

	pFileList->Transfering = true;

	pFileList->DoUpdate ();

	pFileList->Transfering = false;

	Application->ThreadList.DoExitThread (0);
}

//-----------------------------------------------------------------------------------------
void FILE_LIST::DoUpdate (void)
{
	int								I;
	bool							TransferError;
	pAPPLICATION			ApplicationPointer;
	char							NewPath[MAX_PATH];
	bool							Success;

	ApplicationPointer = pAPPLICATION (GetApplicationPointer ());

	NewDisplayTitle ("Update from %s", ServerComputerName);

	FillingVectorArray = true;

	UpdateVectorArray ();

	//Delete all file except older, missing, and different size
	I = 0;
	while (I <= FileMax)
	{
		UpdateStatusBox (File[I].LocalDirInfo.Path);

		if (File[I].ComparedToServer == Directory)
		{
			DeleteRecords (I, 1);
			continue; //May be another directory we need to skip
		}

		//Only new (surplus), newer, and different size files get backed up
		if ((File[I].ComparedToServer == Missing) ||
				(File[I].ComparedToServer == Older) ||
				(File[I].ComparedToServer == Different_size))
		{
			if (StringInString (File[I].FileName, OLD_FILE_TAG))
						DeleteRecords (I, 1);
			else	I++; //Keep file
		}
		else DeleteRecords (I, 1);
	}

	UpdateVectorArray ();

	FillingVectorArray = false;

	InvalidateAllRecords ();
	CloseStatusBox ();

	TransferError	= false;



	for (I = 0; I <= FileMax; I++)
	{
		//Create local directory even if we don't copy files
		QPsprintf_s (NewPath, "C:%s", File[I].LocalDirInfo.Path);
		Success = LocalSFTP.CreateDir (NewPath);
		if (!Success &&
				!StringInString (PCHAR (LocalSFTP.lastErrorText()), "already exists")) //Actually "The file already exists" but should be folder.  Either way it isn't an error
		{
			ShowSFTPError ("CreateDir", &LocalSFTP);
		}

		DownloadFile (File[I].LocalDirInfo.Path, File[I].ServerDirInfo.Path, File[I].FileName, I);

		if (File[I].CopyStatus != Copy_OK) TransferError = true;

		if (CancelTransfer) return;
	}

	if (TransferError) ReportError ("File List", "UPDATE FAILED!\nSome of the files did not update!");
}

//-----------------------------------------------------------------------------------------
void FILE_LIST::DoBackup (void)
{
	int						I;
	bool					TransferError;
	int						Response;
	bool					Success;
	char					NewPath[MAX_PATH + 1];
	pAPPLICATION	Application;

	Application = pAPPLICATION (GetApplicationPointer ());

	NewDisplayTitle ("Backup to %s", ServerComputerName);

	FillingVectorArray = true;

	UpdateVectorArray ();

	//Delete all file except Surplus, Newer, and different size
	I = 0;
	while (I <= FileMax)
	{
		UpdateStatusBox (File[I].LocalDirInfo.Path);

		//The List has the active customer, Plating, and all the source
		if ((File[I].ComparedToServer == Directory) &&
				(File[I].DirectoryComparison == Surplus) &&	//On local not on server
				!StringEqual (File[I].LocalDirInfo.FileName, "Exchange") &&
				(ListType != Source_publish)) //Source publish only publishes directories that have been manually put on remote end
		{
			//Create directory on server even if we don't copy files
			QPsprintf_s (NewPath, "C:%s", File[I].LocalDirInfo.Path);
			Success = ServerSFTP.CreateDir (NewPath);
			if (!Success &&
					!StringInString (PCHAR (ServerSFTP.lastErrorText()), "already exists")) //Actually "The file already exists" but should be folder.  Either way it isn't an error
			{
				ShowSFTPError ("CreateDir", &ServerSFTP);
			}
		}

		if ((File[I].ComparedToServer == Directory) &&
				(File[I].DirectoryComparison == Surplus) &&	//On local not on server
				(ListType == Source_publish))
		{
			//Doing source publish.  If the folder isn't there don't publish any files.
			strcpy_s (NewPath, File[I].LocalDirInfo.Path);
			DeleteRecords (I, 1); //Delete directory

			//Delete all files in directory until the next directory
			while ((I <= FileMax) &&
							(File[I].ComparedToServer != Directory) &&
							StringEqual (File[I].LocalDirInfo.Path, NewPath)) DeleteRecords (I, 1);	
			continue; //May be another directory we need to skip
		}

		//Only new (surplus), newer, older, and different size files get backed up
		//Backup (Publish) older version becasue the file got changed on the other end.  Assume this end is always correct because it was what was used to built last.
		if ((File[I].ComparedToServer == Surplus) ||
				(File[I].ComparedToServer == Newer) ||
				(File[I].ComparedToServer == Older) ||
				(File[I].ComparedToServer == Different_size))
		{
			if (StringInString (File[I].FileName, OLD_FILE_TAG))
						DeleteRecords (I, 1);
			else	I++; //Keep file
		}
		else DeleteRecords (I, 1);
	}

	UpdateVectorArray ();

	FillingVectorArray = false;

	InvalidateAllRecords ();
	CloseStatusBox ();

	if (FileMax < 0)
	{
		//We may be running under a thread so we can't call Done
		ReportError ("Backup", "No files.");
		return;
	}
	
	//Ask if its ok to do backup, allow Cancel
	Response = ShowMessageBox ("Ok to do backup?", "Backup",
														MB_TASKMODAL + MB_ICONQUESTION + MB_OKCANCEL);

	if (Response == IDCANCEL)
	{
		return;
	}

	//Now do the backup
	TransferError	= false;

	for (I = 0; I <= FileMax; I++)
	{
		UploadFile (File[I].LocalDirInfo.Path, File[I].ServerDirInfo.Path, File[I].FileName, I);

		if (File[I].CopyStatus != Copy_OK) TransferError = true;

		if (CancelTransfer) break;
	}

	if (TransferError) ReportError ("File List", "BACKUP FAILED!\nSome of the files did not backup!");

	PlaySound (LPCSTR (SND_ALIAS_SYSTEMASTERISK), NULL, SND_ASYNC | SND_ALIAS_ID);
}

//-----------------------------------------------------------------------------------------
void FILE_LIST::DoCleanup (CLEANUP_TYPE	CleanupType)
{
	int			I;
	char		OldFileName[MAX_PATH + 1];

	NewDisplayTitle ("Cleanup compared to %s", ServerComputerName);

	//Just delete files without asking, the delete is automatic now
	for (I = 0; I <= FileMax; I++)
	{
		if (CleanupType == Local_computer)
		{
			if (File[I].ComparedToServer == Surplus)
			{
				strcpy_s (OldFileName, File[I].LocalDirInfo.Path);
				strcat_s (OldFileName, File[I].FileName);

				//In case its read only
				SetFileAttributes (OldFileName, FILE_ATTRIBUTE_NORMAL);

				if (DeleteFile (OldFileName) == 0)
				{
					if (GetLastError () == ERROR_ACCESS_DENIED) ReportError ("File List", "Unexpected error removing %s", OldFileName);

					File[I].CopyStatus = Copy_error;
				}
				else File[I].CopyStatus = Copy_OK;

				InvalidateRecord (I);
			}
		}
		else if (CleanupType == Server_computer)
		{
			if (File[I].ComparedToServer == Missing)
			{
				QPsprintf_s (OldFileName, "C:%s%s", File[I].ServerDirInfo.Path, File[I].FileName);

				if (!ServerSFTP.RemoveFile (OldFileName))
				{
					ShowSFTPError ("RemoveFile", &ServerSFTP);
					File[I].CopyStatus = Copy_error;
				}
				else File[I].CopyStatus = Copy_OK;

				InvalidateRecord (I);
			}
		}
	}
}

//---------------------------------------------------------------------------------------------
class MySFtpProgress : public CkSFtpProgress
{
	public:
	MySFtpProgress() { }
	virtual ~MySFtpProgress() { }

	pFILE_LIST	FileList;
	int					FileIndex;

	// Called periodically during any SFTP method that communicates with
	// the server.  The HeartbeatMs property controls the frequency
	// of callbacks.  The default HeartbeatMs value = 0, which disables
	// AbortCheck callbacks.
	void AbortCheck(bool *abort)
	{
		if (FileList->CancelTransfer) *abort = true;
	}

	// The PercentDone callbacks is called for any method where it it is possible
	// to monitor a percentage completion, such as uploading and downloading files.
	void PercentDone(int pctDone, bool *abort)
	{
		if (FileList->CancelTransfer) *abort = true;
		FileList->File[FileIndex].PercentCopied = pctDone;
		FileList->InvalidateRecord (FileIndex);
	}
};

//-----------------------------------------------------------------------------------------
bool FILE_LIST::UploadFile (PCHAR	LocalPath,
														PCHAR	ServerPath,
														PCHAR	FileName,
														int		FileIndex)
{
	bool						Success;
	char						RemoteFilePath[MAX_STRING];
	char						LocalFilePath[MAX_STRING];
	MySFtpProgress	UploadProgress;

	QPsprintf_s (RemoteFilePath,		"C:%s%s", ServerPath, FileName);
	QPsprintf_s (LocalFilePath,			"C:%s%s", LocalPath, FileName);

	if (FileIndex >= 0)
	{
		File[FileIndex].CopyStatus = Coping;
		InvalidateRecord (FileIndex); //Show status

		// Set the event callback object:
		UploadProgress.FileList = this;
		UploadProgress.FileIndex = FileIndex;
		ServerSFTP.put_EventCallbackObject (&UploadProgress);

		// Set HeartbeatMs so that AbortCheck events are called once every 1 second.
		ServerSFTP.put_HeartbeatMs(1000);
	}


	//QuickBooks makes its files read only so they copy read only and can't be copied again.
	if (StringInString (LocalFilePath, "QuickBooks"))
	{
		if (SetFileAttributes (LocalFilePath, FILE_ATTRIBUTE_NORMAL) == 0) SimulationReportLastError ("File List", "SetFileAttributes");
	}

	//Copy the local file to the SSH server.
	//Important -- the remote filepath is the 1st argument, the local filepath is the 2nd argument;
	Success = ServerSFTP.UploadFileByName (RemoteFilePath, LocalFilePath);

	if (FileIndex >= 0) ServerSFTP.put_EventCallbackObject (NULL);

	if (!Success)
	{
		if (!CancelTransfer) ShowSFTPError ("UploadFileByName", &ServerSFTP);
	}

	if (FileIndex >= 0)
	{
		if (Success)
					File[FileIndex].CopyStatus = Copy_OK;
		else	File[FileIndex].CopyStatus = Copy_error;
		InvalidateRecord (FileIndex);
	}

	return Success;
}

//-----------------------------------------------------------------------------------------
bool FILE_LIST::DownloadFile (PCHAR	LocalPath,
															PCHAR	ServerPath,
															PCHAR	FileName,
															int		FileIndex)
{
	bool						Success;
	char						RemoteFilePath[MAX_STRING];
	char						LocalFilePath[MAX_STRING];
	MySFtpProgress	DownloadProgress;

	QPsprintf_s (RemoteFilePath,		"C:%s%s", ServerPath, FileName);

	//If this is being used by Define it will be going to ourself and the logon user may not be an admin so they can't access the C$ share
	if (StringEqual (ComputerName, LocalComputerName))
				QPsprintf_s (LocalFilePath, "C:%s%s", LocalPath, FileName);
	else	QPsprintf_s (LocalFilePath, "//%s/C$%s%s", LocalComputerName, LocalPath, FileName); //May be us or our local SQL server.  Local path has a leading /.


	if (FileIndex >= 0)
	{
		File[FileIndex].CopyStatus = Coping;
		InvalidateRecord (FileIndex); //Show status

		// Set the event callback object:
		DownloadProgress.FileList = this;
		DownloadProgress.FileIndex = FileIndex;
		ServerSFTP.put_EventCallbackObject (&DownloadProgress);

		// Set HeartbeatMs so that AbortCheck events are called once every 1 second.
		ServerSFTP.put_HeartbeatMs(1000);
	}

	//Copy the file from the SSH server to the local computer.
	//Important -- the remote filepath is the 1st argument, the local filepath is the 2nd argument.
	Success = ServerSFTP.DownloadFileByName (RemoteFilePath, LocalFilePath);

	if (FileIndex >= 0) ServerSFTP.put_EventCallbackObject (NULL);

	if (!Success)
	{
		//Only show errors for a list of copyies
		if (!CancelTransfer &&
				(FileIndex >= 0)) ShowSFTPError ("DownloadFileByName", &ServerSFTP);
	}
	else
	{
		//This may not be needed with version 9.5.0
		////These were problems in 9.3.1 and may not be problems in later libraries.  This is safe so continue to use it.
		////The preserve date time doesn't work and UTCMode doesn't work.  This is a workaround for those problems problem.
		////This counts on getting the time as CKDateTime and extracting the FILETIME which comes out UTC.
		//DiskFile.OpenToAppend (LocalFilePath);
		//FileDateTime = ServerSFTP.GetFileLastModifiedDt (RemoteFilePath, false, false);
		//FileDateTime->GetAsFileTime (false, LastWriteTime);

		//FileDateTime = ServerSFTP.GetFileCreateDt (RemoteFilePath, false, false);
		//FileDateTime->GetAsFileTime (false, CreateTime);

		//FileDateTime = ServerSFTP.GetFileLastAccessDt (RemoteFilePath, false, false);
		//FileDateTime->GetAsFileTime (false, LastAccessTime);

		//if (SetFileTime (DiskFile.hFile, &CreateTime, &LastAccessTime, &LastWriteTime) == 0)
		//{
		//	SimulationReportLastError ("File List", "SetFileTime:");
		//}
		//DiskFile.Close ();
	}

	if (FileIndex >= 0)
	{
		if (Success)
					File[FileIndex].CopyStatus = Copy_OK;
		else	File[FileIndex].CopyStatus = Copy_error;
		InvalidateRecord (FileIndex);
	}

	return Success;
}

//-----------------------------------------------------------------------------------------
bool FILE_LIST::DownloadPictureFile(PCHAR		PartNumber,
																	PCHAR			FileName,
																	int				SizeOfFileName)
{
	int					CustomerIndex;
	char				RemotePath[MAX_STRING];
	bool				Success;
	DOUBLE_TIME	CurrentTime;

	if (OurCustomerList == NULL)
	{
		ReportError("File List", "Not initialized!");
		return false;
	}

	if (StringEqual(PartNumber, "")) return false;

	if (Features.CustomerType == Anodizing_customer) return false;

	GetDoubleTime(&CurrentTime);

	CustomerIndex = OurCustomerList->GetIndex("Document Server");
	if (CustomerIndex >= 0)
	{
		QPsprintf_s(FileName, SizeOfFileName, "%s.jpg", PartNumber);
		QPsprintf_s(RemotePath, sizeof(RemotePath), "/Plating Documents/");

		strcpy_s(RemoteCustomerName, OurCustomerList->Customer[CustomerIndex].Name);
		SetRemoteCredentials(OurCustomerList->Customer[CustomerIndex].UserName, OurCustomerList->Customer[CustomerIndex].LineComputerName, OurCustomerList->Customer[CustomerIndex].Password);

		//Ignore errors 
		IgnoreErrors = true; //Unattended mode
		Success = OpenConnections(IPAddress, OurCustomerList->Customer[CustomerIndex].RemoteSQLIPAddress); //Use IP so we don't get the Snap NIC
		if (Success)
		{
			//This will fail if the file isn't there which is typical
			Success = DownloadFile("/temp/", RemotePath, FileName, -1); //Not using list of files, return on errors with no dialog

			CloseConnections();
		}
		else
		{
			if ((CurrentTime - PriorReportTime) > SECONDS_PER_HOUR)
			{
				LogGeneralProblem("File List", "Connection to Document Server failed!");
				PriorReportTime = CurrentTime;
			}
		}

		strcpy_s(RemoteCustomerName, "");
		SetRemoteCredentials("", "", "");

		//Full path for caller
		QPsprintf_s(FileName, SizeOfFileName, "/temp/%s.jpg", PartNumber);

		return Success;
	} 

	ReportError("File List", "Document Server not defined in Custom list!");

	return false;
}

//-----------------------------------------------------------------------------------------
int FILE_LIST::GetIndex (PCHAR		LocalPath,
													PCHAR		SearchFile)
{
	int		Index;
	bool	Found;

	Index = 0;
	Found	= false;

	while (!Found &&
				 (Index <= FileMax))
	{
		if (StringEqual (File[Index].LocalDirInfo.Path, LocalPath) &&
				StringEqual (File[Index].FileName, SearchFile))
					Found = true;
		else	Index++;
	}
	
	if (Found)
				return Index;
	else	return -1;
}

//-----------------------------------------------------------------------------------------
int FILE_LIST::GetIndex (PCHAR		LocalPath)
{
	int		Index;
	bool	Found;

	Index = 0;
	Found	= false;

	while (!Found &&
				 (Index <= FileMax))
	{
		if (StringEqual (File[Index].LocalDirInfo.Path, LocalPath) &&
				(File[Index].ComparedToServer == Directory))
					Found = true;
		else	Index++;
	}
	
	if (Found)
				return Index;
	else	return -1;
}