/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_test.c
* Description	: 
*		Handle testing commands
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	05/19/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include "ui_cli.h"

#ifdef MIDWAY
#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)
#else
#define emac_reset_statistics		gmac_reset_statistics
#define emac_show_statistics		gmac_show_statistics
#endif
#endif

/*----------------------------------------------------------------------
* cli_stat_cmd
* 	Usage: stat [reset]
*
* Notes:
*	(1) The tftp buf is not freed if download successfully,
*		because the use may dump the content.
*	
*	argc 	= 1 or 2
*	argv[0]	= stat
*	argv[1]	= reset
*----------------------------------------------------------------------*/
void cli_stat_cmd(char argc, char *argv[])
{
	int data;

#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)	
	printf("Command not support this version!!\n");
#else
    if (argc == 2 && (strncasecmp(argv[1], "reset", 2)==0))
    {
    	emac_reset_statistics();
    }
	
	emac_show_statistics(argc, argv);
#endif	
}

