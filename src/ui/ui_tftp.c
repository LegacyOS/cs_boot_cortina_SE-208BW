/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_tftp.c
* Description	: 
*		Handle TFTP function for UI
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/28/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <sl2312.h>
#include <net.h>
#include "ui_cli.h"

#define TFTP_BUF_SIZE	(2*1024*1024)

static const char tftp_syntax_msg[]=
    "Usage: tftp host [get | put] filename [location] [size]\n"
    "host      : IP address\n"
    "filename  : Filename\n"
    "location  : Destination for GET operation, or source for PUT operation.\n"
    "size      : total size for PUT operation (> 0)\n";
    
static char *tftp_get_buf = NULL;
static char *tftp_put_buf = NULL;

/*----------------------------------------------------------------------
* cli_tftp_cmd
* 	Usage: tftp host [get | put] filename [location] [size]
*
* Notes:
*	(1) The tftp buf is not freed if download successfully,
*		because the use may dump the content.
*	
*	argc 	= 4 or 6
*	argv[0]	= tftp
*	argv[1]	= host
*	argv[2]	= "get" or "put"
*	argv[3]	= filename
*	argv[4]	= location
*	argv[5]	= size
*----------------------------------------------------------------------*/
void cli_tftp_cmd(char argc, char *argv[])
{
    UINT32	host;
    int		rc;
    int		total;
    int     do_get;
    UINT32	location, size;
    char	method;
    
    if (argc <= 3)
    	goto err_end;
    
    method = argv[2][0];
    if (method == 'g' || method == 'G')
    {
    	do_get = 1;
    	if (argc != 5)
    		goto err_end;
    	location = str2value(argv[4]);
    }
    else if (method == 'p' || method == 'P')
    {
    	do_get = 0;
    	if (argc != 6)
    		goto err_end;
    		
    	size = str2value(argv[5]);
    	if (size == 0)
    	{
			printf("Size is zero!\n");
    		goto err_end;
    	}
    	location = str2value(argv[4]);
    }
    else
    	goto err_end;

	host = str2ip(argv[1]);
	if (host == 0 || host == 0xffffffff)
	{
		printf("Illegal IP address of host!\n");
    	goto err_end;
	}
    
    if (strlen(argv[3]) == 0)
	{
		printf("Illegal filename!\n");
    	goto err_end;
    	return;
	}
	
	if (do_get)
	{
#if 0
		if (tftp_get_buf)
			free (tftp_get_buf);
		if ((tftp_get_buf = (char *)malloc(TFTP_BUF_SIZE)) == NULL)
		{
			dbg_printf(("No free momory!\n"));
    		return;
		}
		if ((rc = tftpc_get(argv[3], host, tftp_get_buf, TFTP_BUF_SIZE, &total)) != 0)
		{
			printf("\nFailed for TFTP GET! (%d) %s\n", rc, tftp_err_msg(rc));
    		free(tftp_get_buf);
    		tftp_get_buf = NULL;
    		return;
		}
	
		printf("\nTFTP GET file at 0x%x, total = %d\n", (char *)tftp_get_buf, total);
#else
		if ((rc = tftpc_get(argv[3], host, (char *)location, TFTP_BUF_SIZE, &total)) != 0)
		{
			printf("\nFailed for TFTP GET! (%d) %s\n", rc, tftp_err_msg(rc));
    		return;
		}
	
		printf("\nTFTP GET file at 0x%x, total = %d\n", (char *)location, total);
#endif	
	}
	else
	{
    	int in_flash_range = 0;
		
		if (tftp_put_buf)
			free(tftp_put_buf);
		if ((tftp_put_buf = (char *)malloc(size)) == NULL)
		{
			dbg_printf(("No free momory!\n"));
    		return;
		}
		
		if (location >= (UINT32)SL2312_FLASH_SHADOW && location <= (UINT32)((UINT32)SL2312_FLASH_SHADOW + 0x10000000))
			in_flash_range = 1;
		
		if (in_flash_range) hal_flash_enable();
		
		memcpy(tftp_put_buf, location, size);
		
		if (in_flash_range) hal_flash_disable();
		
		if ((rc = tftpc_put(argv[3], host, tftp_put_buf, size, &total)) != 0)
		{
			free(tftp_put_buf);
			tftp_put_buf = NULL;
			printf("\nFailed for TFTP PUT! (%d) %s\n", rc, tftp_err_msg(rc));
    		return;
		}
		free(tftp_put_buf);
		tftp_put_buf = NULL;
		printf("\nTFTP PUT file from 0x%x, total = %d\n", (char *)location, total);
	}
	
	return;
	// Do not free buf if download OK
	// because the use may dump the content
	// free(buf);

err_end:
    printf(tftp_syntax_msg);
    return;
}


