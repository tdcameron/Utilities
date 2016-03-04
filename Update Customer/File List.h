//===========================================================================
//	FileList Object Unit                                    
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
//===========================================================================
#pragma once

#include "Display.h"
#include "Batch file.h"

//This libary has random errors on close handle calls on the remote server.  The error is ignored
#include "/ChilKat Library/chilkat-9.5.0-x86_64-vc2015/include/CkSFtp.h"
#include "/ChilKat Library/chilkat-9.5.0-x86_64-vc2015/include/CkSFtpDir.h"
#include "/ChilKat Library/chilkat-9.5.0-x86_64-vc2015/include/CkSFtpFile.h"



#define	FILE_SERVER_COMPUTER	true
#define LOCAL_COMPUTER				false

//LIST_TYPE-----------------------------------------------------------------------
enum LIST_TYPE
{
	Update_data_from_remote, Update_publish_from_server, Install_update,
	Source_backup, Source_publish,
	Publish_customer, Backup_pictures,
	Local_services_list,
};

//COMPARED_TO_SERVER-----------------------------------------------------------------------
enum COMPARED_TO_SERVER
{
	Same,
	Newer,
	Older,
	Different_size,
	Surplus,
	Missing,
	Access_denied,
	Directory
};

const char ComparedToServerText[Directory + 1][17] =
{
	"Same", 
	"Newer",
	"Older",
	"Different size",
	"Surplus",
	"Missing",
	"Access denied",
	"Directory",
};

//COPY_STATUS-----------------------------------------------------------------------
enum COPY_STATUS
{
	Copy_unknown,
	Copy_OK,
	Copy_error,
	Coping
};

#define MAX_COPY_STATUS Coping

const char CopyStatusText[MAX_COPY_STATUS + 1][17] =
{
	"",
	"OK",
	"Error",
	"Coping",
};

//LOOKUP_TYPE-----------------------------------------------------------------------
enum LOOKUP_TYPE
{
	Directories_only,
	All_entries,
	No_subdirectories,
};

//CLEANUP_TYPE-----------------------------------------------------------------------
enum CLEANUP_TYPE
{
	Local_computer,
	Server_computer,
};

//DIRECTORY_ITEM-----------------------------------------------------------------------
typedef struct DIRECTORY_ITEM
{
	char			FileName[MAX_PATH];
	FILE_TIME	LastWriteTime;
	FILE_TIME	LastAccessTime;
	FILE_TIME	CreateTime;
	_int64		FileSize;
	char			Path[MAX_PATH];
} DIRECTORY_ITEM;
typedef DIRECTORY_ITEM *pDIRECTORY_ITEM;

//FILE_ITEM-----------------------------------------------------------------------
typedef struct FILE_ITEM
{
	char								FileName[MAX_PATH];			
	COMPARED_TO_SERVER	ComparedToServer;
	COMPARED_TO_SERVER	DirectoryComparison;
	bool								SourceFile;
	COPY_STATUS					CopyStatus;
	int									PercentCopied;

	DIRECTORY_ITEM			LocalDirInfo;
	DIRECTORY_ITEM			ServerDirInfo;

	SQL_RECORD_CONTROL SQLRecordControl;
} FILE_ITEM;

typedef FILE_ITEM *pFILE_ITEM;


//FILE_LIST-----------------------------------------------------------------------
class FILE_LIST: public DISPLAY_WINDOW
{
	public: 
	vector<FILE_ITEM>	File;
	int								FileMax;

	FILE_ITEM					FileItem;

	CkSFtp				LocalSFTP;
	CkSFtp				ServerSFTP;
	char					LocalComputerName[COMPUTER_NAME_SIZE];
	char					ServerComputerName[COMPUTER_NAME_SIZE];

	bool					ShowHiddenFiles;
	bool					IgnoreErrors; //For unattended mode so programs will keep running
	DOUBLE_TIME		PriorReportTime;

	LIST_TYPE			ListType;

	HWND					StatusBox;
	DATE_TIME			PriorStatusUpdateTime;

	bool					AbortDirectoryBuild;

	bool					Transfering;
	bool					AskUpdateOk;

	int						CopyStatusMax;

	int	LocalPathField;
	int	ServerPathField;
	int	FileNameField;			
	int	ComparedToServerField;
	int	CopyStatusField;

					void Setup						(pDISPLAY_WINDOW TheCustomerList);

	virtual void Initialize				(void);

	virtual void DoCreateWindow		(LIST_TYPE		TheListType);
	
 					void CreateList					(LIST_TYPE	TheListType);

	virtual	void DefineFields				(void);

	virtual bool CanClose						(void);

					void UpdateStatusBox		(PCHAR			Directory);
					void CloseStatusBox			(void);

					bool OpenConnections	(PCHAR	TheLocalComputerName,
																PCHAR		TheServerComputerName);

					void CloseConnections	(void);

					void ShowSFTPError		(PCHAR				Title,
																CkSFtp				*ErrorSFTP);

					bool AddToList					(bool				FileServerComputer,
																	PCHAR				LocalPath,
																	PCHAR				ServerPath,
																	LOOKUP_TYPE	LookupType);

					void DoUpdate						(void);
					void DoBackup						(void);
					void DoCleanup					(CLEANUP_TYPE	CleanupType);
					void DoTransfer					(void);

					bool DoingFileTransfer	(void);

					int	 GetIndex						(PCHAR		LocalPath,
																		PCHAR		SearchFile);

					int	 GetIndex						(PCHAR		LocalPath);


					bool CancelTransfer;

					bool DownloadFile				(PCHAR		LocalPath,
																	PCHAR			ServerPath,
																	PCHAR			FileName,
																	int				FileIndex);

					bool DownloadPictureFile (PCHAR		PartNumber,
																		PCHAR		FileName,
																		int			SizeOfFileName);

	private: 
	virtual bool ReadRecords				(void);
	virtual void WriteRecords				(void);
	virtual void PaintField					(void);
	virtual bool CheckField					(void);
	virtual void Cut								(void);

					void CompareToServer		(void);

					bool UploadFile					(PCHAR		LocalPath,
																	PCHAR			ServerPath,
																	PCHAR			FileName,
																	int				FileIndex);
};

typedef FILE_LIST *pFILE_LIST;