/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: net_main.c
* Description	: 
*		Main entry for network
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <net.h>

static int net_initialized = 0;
extern int web_on;
/*----------------------------------------------------------------------
* net_init
*----------------------------------------------------------------------*/
void net_init(void)
{
	
	if (!net_initialized)
	{
		net_initialized = 1;
		net_init_buf();
		enet_init();
		udp_init();
#ifdef BOARD_SUPPORT_WEB
	if(web_on == 1)
	{
		tcp_init();
		//s = tcp_socket(0);
		}
#endif			
	}
}


