/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_ping.c
* Description	: 
*		Handle Ping function for UI
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/28/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <net.h>
#include "ui_cli.h"

static const char * const ping_syntax_msg[]=
{
    "Usage: ping host [-t] [-n count] [-l size] [-w timeout]\n",
    "host      : IP address\n",
    "-t        : Ping forever until pressing <CTRL-C> key.\n",
    "-n count  : Total request number.\n",
    "-l size   : Send size (32 ~ 1472 bytes).\n",
    "-w        : Timeout Interval (milliseconds).\n",
    ""
};

/*----------------------------------------------------------------------
* 	ping_show_syntax
*----------------------------------------------------------------------*/
static void ping_show_syntax(void)
{
	int i = 0;
	
	while(*ping_syntax_msg[i]) 
	{
		printf(ping_syntax_msg[i]);
		i++;
	}
}

/*----------------------------------------------------------------------
* cli_ping_cmd
* 	Usage: ping host [-t] [-n count] [-l size] [-w timeout]
*          	-t        : Ping forever until pressing <CTRL-C> key.
*			host      : IP address
*          	-n count  : Total request number (> 0).
*          	-l size   : Send size (32 ~ 1472 bytes).
*			-w        : Timeout Interval (milliseconds).
*
*			argv[0]   : ping
*			argv[1]   : host
*			argv[2...]: -t, -n ...
*----------------------------------------------------------------------*/
void cli_ping_cmd(char argc, char *argv[])
{
    int			err;
    char		*cp;
    UINT32		host;
    UINT32      count;
    UINT32      size;
    UINT32		timeout;
    
    host	= 0;
    timeout	= 1000;
    size	= ICMP_MIN_DATA_SIZE;
    count 	= 4;
     
    if (argc < 2)
    {
    	ping_show_syntax();
    	return;
    }

	host = str2ip(argv[1]);
	if (host == 0 || host == 0xffffffff)
	{
		printf("Illegal IP address of host!\n");
    	ping_show_syntax();
    	return;
	}
    
	argc -= 2;
	argv += 2;
	err = 0;
    while(argc > 0 && !err)
    {
        cp = argv[0];
        if (cp[0] != '-' || cp[2] != '\0')
		{
			err = 1;
    		break;
		}
		switch(toupper(cp[1]))
		{
			case 'T':
				count = 0;
				argc--;
				argv++;
				break;
			case 'N':
				if (argc < 2 || ((count = str2decimal(argv[1])) == 0))
				{
					printf("Syntax error for \"-n\" argument!\n");
					err = 1;
					break;
				}
				argc -= 2;        
				argv += 2;             
				break;
			case 'L':
				size = 0;
				if (argc >= 2)
					size = str2decimal(argv[1]);
				if (size < ICMP_MIN_DATA_SIZE || size > ICMP_MAX_DATA_SIZE)
				{
					printf("Syntax error for \"-l\" argument!\n");
					err=1;
					break;
				}
				argc -= 2;        
				argv += 2;             
				break;
			case 'W':
				timeout = 0;
				if (argc >= 2)
					timeout = str2decimal(argv[1]);
				if (timeout < 10 || timeout > 10000)
				{
					printf("Syntax error for \"-w\" argument!\n");
					err=1;
					break;
				}
				argc -= 2;        
				argv += 2;             
				break;
			default:
				printf("Syntax error for unknown argument!\n");
				err=1;
				break;
                    
		} // switch                    
    } // while        
    
	if (err)
		ping_show_syntax();
	else
        icmp_ping(host, count, size, timeout);
}


