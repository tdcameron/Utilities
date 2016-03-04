//---------------------------------------------------
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
//---------------------------------------------------
#pragma once

#include "General.h"

//For SQL support
#include "SQL Table.h"

#include "Workbar.h"
#include "Tool.h"
#include "Rack.h"
#include "Part.h"
#include "Process Name.h"
#include "Process Time.h"
#include "Rectifier Setpoint.h"
#include "Rectifier Loop.h"
#include "Rectifier Dummy.h"
#include "Feeder.h"
#include "Bath Loop.h"
#include "Bath Setpoint.h"
#include "Mover.h"
#include "Move Time.h"
#include "Move Command.h"
#include "Moving Device.h"
#include "Snap IP Address.h"
#include "Spray Loop.h"
#include "Fill Action Time.h"
#include "Fill Control.h"
#include "Wait Query.h"

#include "\users\public\Documents\Quick Plate\52nd East Paint\Paint Schedule\Paint.h"
#include "\users\public\Documents\Quick Plate\52nd East Paint\Paint Schedule\Paint Gap.h"
#include "\users\public\Documents\Quick Plate\52nd East Paint\Paint Schedule\Color Group.h"
#include "\users\public\Documents\Quick Plate\52nd East Paint\Paint Schedule\Chain List.h"

#include "\users\public\Documents\Quick Plate\Linetec Common\Linetec Common\Color Setpoint.h"
#include "\users\public\Documents\Quick Plate\Linetec Common\Linetec Common\Color Loop.h"
#include "\users\public\Documents\Quick Plate\Linetec Common\Linetec Common\Thickness Limit.h"
#include "\users\public\Documents\Quick Plate\Linetec Common\Linetec Common\Quality.h"
#include "\users\public\Documents\Quick Plate\Linetec Common\Linetec Common\Translate.h"

#include "\users\public\Documents\Quick Plate\Trend\Cycle.h"

//CUSTOM_RACK_LIST-----------------------------------------------------------------------------------------
class CUSTOM_RACK_LIST: public RACK_LIST
{
	virtual void SQLWriteRecords		(void);
};

//CUSTOM_STATION_NAME_LIST-----------------------------------------------------------------------------------------
class CUSTOM_STATION_NAME_LIST: public STATION_NAME_LIST
{
	virtual void SQLWriteRecords		(void);
};

//UPDATE_SQL-----------------------------------------------------------------------------------------
class UPDATE_SQL: public SQL_TABLE
{
	public: 
	DATE_TIME	StartDateTime;
	DATE_TIME	EndDateTime;

	SQL_TABLE	RemoteTable;
	SQL_TABLE	TimeTable;
	bool			Transfering;
	bool			*FileListTransfering;
	char			StatusMessage[80];
	bool			UpdateSQLWithFill;

	//Generic lists
	PART_LIST									PartList;
	WORKBAR_LIST							WorkbarList;
	PROCESS_NAME_LIST					ProcessNameList;
	PROCESS_TIME_LIST					ProcessTimeList;
	TOOL_LIST									ToolList;
	DEVICE_NAME_LIST					DeviceNameList;
	DEVICE_LIST								DeviceList;
	RECTIFIER_SETPOINT_LIST		RectifierSetpointList;
	RECTIFIER_LOOP_LIST				RectifierLoopList;
	RECTIFIER_DUMMY_LIST			RectifierDummyList;
	CUSTOM_RACK_LIST					RackList; //Customized to write workbar
	CUSTOM_STATION_NAME_LIST	StationNameList; //Customized to write active batch and batch enable
	BATH_LOOP_LIST						BathLoopList;
	BATH_SETPOINT_LIST				BathSetpointList;
	FEEDER_LIST								FeederList;
	MOVER_LIST								MoverList;
	MOVE_TIME_LIST						MoveTimeList;
	MOVE_COMMAND_LIST					MoveCommandList;
	MOVING_DEVICE_LIST				MovingDeviceList;
	SNAP_IP_ADDRESS_LIST			SnapIPAddressList;
	DEFECT_NAME_LIST					DefectNameList;
	SPRAY_LOOP_LIST						SprayLoopList;
	FILL_ACTION_TIME_LIST			FillActionTimeList;
	FILL_CONTROL_LIST					FillControlList;
	WAIT_LIST									WaitList;
	CYCLE_LIST								CycleList;

	//Paint Schedule only
	PAINT_LIST								PaintList;
	PAINT_GAP_LIST						PaintGapList;
	COLOR_GROUP_LIST					ColorGroupList;
	CHAIN_LIST								ChainList;

	//For Linetec only
	COLOR_SETPOINT_LIST				ColorSetpointList;
	COLOR_LOOP_LIST						ColorLoopList;
	THICKNESS_LIMIT_LIST			ThicknessLimitList;
	QUALITY_LIST							QualityList;
	TRANSLATE_LIST						TranslateList;

	void	StartWaitUpdate			(void);
	void	StartUpdate					(bool TheUpdateSQLWithFill);
	void	DoWaitUpdate				(void);
	void	DoUpdate						(void);
	bool	DoingTableTransfer	(void);

	void	ConvertChainTable		(void);

	private:
	void	UpdateProduction	(void);
	void	UpdateChain				(void);
};

typedef UPDATE_SQL *pUPDATE_SQL;