/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_timer.c
* Description	: 
*		Handle timer functions
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/20/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>

extern UINT64 sys_ticks;

/*----------------------------------------------------------------------
* sys_sleep
* 	Delay in ms unit
*----------------------------------------------------------------------*/
void sys_sleep(UINT32 ms)
{
	UINT64	delay_time;
	UINT32	delay_ticks;
	
	delay_ticks = (ms * BOARD_TPS) /1000;
	if (delay_ticks == 0) delay_ticks = 1;
	delay_time = sys_get_ticks() + delay_ticks;
	
	while (sys_get_ticks() < delay_time)
	{
		enet_poll();
	}

}

/*----------------------------------------------------------------------
* timer_cli
*----------------------------------------------------------------------*/
void timer_cli(char argc, char **argv)
{
	int i;
	
	printf("System Ticks = %d\n", sys_ticks);

#if 0	
	for (i=0; i<10; i++)
	{
		printf("System Ticks = %d ... ", sys_ticks);
		hal_delay_us(1 * 1000 * 1000);
		printf("%d\n", sys_ticks);
	}
#endif	
}
