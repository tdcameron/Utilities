//---------------------------------------------------------------------------------------
//	File List Object
//	Copyright (C) 1997 Mahood and Company.
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
//---------------------------------------------------------------------------------------

#include "Cleanup Tabs.h"

#define LEFT_BRACE		"{"
#define RIGHT_BRACE		"}"
#define SPACE					" "

//-------------------------------------------------------------------------------------------------------- 
void CLEANUP_TABS::Initialize (void)
{
strcpy_s (Path, "\users\public\Documents\Quick Plate/");
}

//-------------------------------------------------------------------------------------------------------- 
void CLEANUP_TABS::DoCleanup (HWND	hWndFrame)
{
	hWndFrame = hWndFrame;
//	int						I, J, K;
//	OPENFILENAME  OpenFileName;
//	char          InFileName[MAX_PATH + 1];
//	char          OutFileName[MAX_PATH + 1];
//	char          ShortFileName[MAX_PATH + 1];
//	char          Extension[MAX_PATH + 1];
//	char					Filter[MAX_PATH + 1];
//	DISK_FILE			InFile;
//	DISK_FILE			OutFile;
//	char					InLine[MAX_STRING + 1];
//	char					OutLine[MAX_STRING + 1];
//	bool					Success;
//	int						BytesWriten;
//	int						IndentLevel;
//	int						NextLineIndentLevel;
//	bool					Forever = true;
//
//
//	while (Forever)
//	{
//		strcpy_s (InFileName, "*.cpp;*.h");
//
//		strcpy_s (Filter,			"C++");
//		strcpy_s (&Filter[4],	"*.cpp;*.h");
//		Filter[4+10]				= 0;
//
//		strcpy_s (Extension, "cpp,h");
//
//
//		ZeroMemory (&OpenFileName, sizeof (OpenFileName));
//
//		OpenFileName.lStructSize				= sizeof (OpenFileName);
//		OpenFileName.hwndOwner					= hWndFrame;
//		OpenFileName.hInstance					= NULL;
//		OpenFileName.lpstrFilter				= Filter;
//		OpenFileName.lpstrCustomFilter	= (LPSTR) NULL;
//		OpenFileName.nMaxCustFilter			= 0;
//		OpenFileName.nFilterIndex				= 1;
//		OpenFileName.lpstrFile					= InFileName;								//Fully specified file name
//		OpenFileName.nMaxFile						= sizeof (InFileName);
//		OpenFileName.lpstrFileTitle			= ShortFileName;					//File name w/o path
//		OpenFileName.nMaxFileTitle			= sizeof (ShortFileName);
//		OpenFileName.lpstrInitialDir		= Path;
//		OpenFileName.lpstrTitle					= Path;
//		OpenFileName.Flags							= OFN_PATHMUSTEXIST | OFN_EXPLORER;
//		OpenFileName.nFileOffset				= 0;
//		OpenFileName.nFileExtension			= 0;
//		OpenFileName.lpstrDefExt				= Extension;
//		OpenFileName.lCustData					= 0;
//		OpenFileName.lpfnHook						= NULL;
//		OpenFileName.lpTemplateName			= NULL;
//
//		if (!GetOpenFileName (&OpenFileName))
//		{
//			strcpy_s (ShortFileName, "");
//			return;
//		}
//		else
//		{
//			//Copy current path over for next convert
//			strcpy_s (Path, InFileName);
//			Path[StringPosition (Path, ShortFileName)] = 0;
//
//			strcpy_s (OutFileName, InFileName);
//			strcat_s (OutFileName, "$new$");
//
//			ChangeSlashes (InFileName, "/");
//			ChangeSlashes (OutFileName, "/");
//
//			InFile.OpenToRead (InFileName);
//			OutFile.OpenToWrite (OutFileName);
//
//			IndentLevel					= 0;
//			NextLineIndentLevel	= 0;
//
//			do
//			{
//				//Read up to LF, CR and LF are not passed back
//				Success = InFile.ReadLine (InLine, sizeof (InLine), LF);
//
//				if (Success)
//				{
//					IndentLevel = NextLineIndentLevel;
//					
//					I	= 0;
//					J	= 0;
//
//					//Find the first character after the tabs and spaces
//					while ((InLine[I] != 0) &&
//								((InLine[I] == TAB[0]) ||
//									(InLine[I] == SPACE[0])))
//					{
//						I++;
//					}
//
//					//If the next character is { indent one more tab on the next line.
//					if ((InLine[I] != 0) &&
//							(InLine[I] == LEFT_BRACE[0]))
//					{
//						//Unless there is a } on the same line.
//						if (!StringInString (InLine, RIGHT_BRACE))
//						{
//							NextLineIndentLevel++;
//						}
//
//						for (K = 1; K <= IndentLevel; K++)
//						{
//							OutLine[J] = TAB[0];
//							J++;
//						}
//					}
//
//					//If the next character is } outdent one more tab on this line
//					else if ((InLine[I] != 0) &&
//									(InLine[I] == RIGHT_BRACE[0]))
//					{
//						NextLineIndentLevel--;
//						
//						if (NextLineIndentLevel < 0) NextLineIndentLevel = 0;
//						
//						IndentLevel = NextLineIndentLevel;
//						
//						for (K = 1; K <= IndentLevel; K++)
//						{
//							OutLine[J] = TAB[0];
//							J++;
//						}
//					}
//
//					else
//					{
//						I = 0; //Not a line beginning with tabs or spaces followed by a brace, just copy it as is.
//					}
//
//					//Copy the rest of the line
//					while (InLine[I] != 0)
//					{
//						OutLine[J] = InLine[I];
//						I++;
//						J++;
//					}
//
//					OutLine[J] = char (0); //End of string
//
//					//Microsoft C++ editor wants LF or CR LF in that order
//					//DoReadLine ignores LF and wants CR
//					strcat_s (OutLine, CR);
//					strcat_s (OutLine, LF);
//
//					BytesWriten = OutFile.Write (OutLine, strlen (OutLine));
//				}
//			} while (Success);
//
//				//The last line may not have a LF on it
//			if (IndentLevel > 0)
//			{
//				strcpy_s (OutLine, TAB);
//				strcat_s (OutLine, "}");
//				strcat_s (OutLine, CR);
//				strcat_s (OutLine, LF);
//
//				BytesWriten = OutFile.Write (OutLine, strlen (OutLine));
//			}
//
//			InFile.Close ();
//			OutFile.Close ();
//
//			if (strlen (InLine) < int (sizeof (InLine) -1))
//			{
//				InFile.Delete (InFileName);
//
//				rename (OutFileName, InFileName);
//
////				ReportError ("Cleanup Tabs", "Tabs cleaned up in:\n%s", InFileName);
//			}
//			else ReportError ("Cleanup Tabs", "Long line, file not converted:\n%s", InFileName);
//		}
//	}
}
