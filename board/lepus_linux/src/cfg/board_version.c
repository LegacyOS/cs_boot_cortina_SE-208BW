/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: board_info.c
* Description	: 
*		Dump board information
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/18/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <board_version.h>

const char sw_version[]=BOARD_SW_VERSION;
const char sw_built_date[] = __DATE__;
const char sw_built_time[] = __TIME__;

/*----------------------------------------------------------------------
* 	board_show_version
* 	show boot loader version 
*----------------------------------------------------------------------*/
void board_show_version(void)
{
	sys_printf("\n\n%s, version %s\n",
    			BOARD_BOOT_LOADER_NAME,
            	sw_version
            	);
    sys_printf("Built by %s, %s, %s\n\n",
    			BOARD_BUILD_TOOL,
            	sw_built_time,
            	sw_built_date 
            	);
}
