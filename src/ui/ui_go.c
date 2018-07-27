/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_go.c
* Description	: 
*		Start code at a location
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/19/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <sl2312.h>
#include <net.h>
#include "ui_cli.h"

static const char go_syntax_msg[]=
    "Usage: go [location]\n"
    "location  : Starting entry.\n";

/*----------------------------------------------------------------------
* cli_go_cmd
* 	Usage: go [location]
*
*	argc 	= 2
*	argv[0]	= go
*	argv[1]	= [location]
*----------------------------------------------------------------------*/
void cli_go_cmd(char argc, char *argv[])
{
    UINT32	location;
    
    if (argc != 2)
    {
    	printf(go_syntax_msg);
    	return;
    }
    
	location = str2value(argv[1]);

	sys_run_apps((void *)location);
}


