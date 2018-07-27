/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_config.c
*
* Description	: 
*		Handle system configuration & information for UI
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/22/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <net.h>
#include "ui_cli.h"

/*----------------------------------------------------------------------
* ui_config
*----------------------------------------------------------------------*/
void ui_config(int arg)
{
	sys_show_hw_cfg();
	sys_show_sw_cfg();
}

/*----------------------------------------------------------------------
* cli_show_sys_cfg
*----------------------------------------------------------------------*/
void cli_show_sys_cfg(char argc, char *argv[])
{
	board_show_version();
	sys_show_hw_cfg();
	sys_show_sw_cfg();
}

/*----------------------------------------------------------------------
* cli_show_mem
*----------------------------------------------------------------------*/
void cli_show_mem(char argc, char *argv[])
{
	dump_malloc_list();
	printf("\n");
	net_show_buf_info();
}

