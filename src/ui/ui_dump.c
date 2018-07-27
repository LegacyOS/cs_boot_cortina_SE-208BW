/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_dump.c
* Description	: 
*		Display (hex dump) a range of memory.
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/29/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <sl2312.h>
#include "ui_cli.h"

static const char dm_syntax_msg[]=
    "Usage: dm [-b <location>] [-l <length>] [-1|2|4]\n";
static const char sm_syntax_msg[]=
    "Usage: sm [-b <location>] [-1|2|4] [data]\n";

static void dm_byte(UINT32 location, int length);
static void dm_short(UINT32 location, int length);
static void dm_long(UINT32 location, int length);

/*----------------------------------------------------------------------
* cli_dump_cmd
* 	Usage: dm [-b <location>] [-l <length>] [-1|2|4]
*----------------------------------------------------------------------*/
void cli_dump_cmd(char argc, char *argv[])
{
    int				err;
    char			*cp;
    static UINT32	location=0, length=128, size=1;
    
	argc -= 1;
	argv += 1;
	err = 0;
    while(argc > 0 && !err)
    {
        cp = argv[0];
        if (cp[0] != '-' || cp[2] != '\0')
		{
			err = 1;
    		break;
		}
		switch(tolower(cp[1]))
		{
			case 'b':
				if (argc < 2)
				{
					printf("Syntax error for \"-b\" argument!\n");
					err = 1;
					break;
				}
				location = str2value(argv[1]);
				argc -= 2;        
				argv += 2;             
				break;
			case 'l':
				if (argc < 2)
				{
					printf("Syntax error for \"-l\" argument!\n");
					err = 1;
					break;
				}
				length = str2value(argv[1]);
				if (length > 1024)
				{
					printf("Length is too long! (must be less 1024)!\n");
					err = 1;
					break;
				}
				argc -= 2;        
				argv += 2;             
				break;
			case '1':
				size = 1;
				argc--;
				argv++;
				break;
			case '2':
				size = 2;
				argc--;
				argv++;
				break;
			case '4':
				size = 4;
				argc--;
				argv++;
				break;
			default:
				printf("Syntax error for unknown argument!\n");
				err=1;
				break;
                    
		} // switch                    
    } // while        
    
    if (length == 0)
    	length = 64;
    	
	if (err)
		printf(dm_syntax_msg);
	else
    {
    	switch (size)
    	{
    		case 1:	
    			dm_byte(location, length);
    			location += length;
    			break;
    		case 2:
    			dm_short(location, length);
    			location += length * 2;
    			break;
    		case 4:		
    			dm_long(location, length);
    			location += length * 4;
    			break;
    	}
    }
}

/*----------------------------------------------------------------------
* cli_write_mem_cmd
* 	Usage: sm [-b <location>] [-1|2|4] data
*----------------------------------------------------------------------*/
void cli_write_mem_cmd(char argc, char *argv[])
{
    int				err;
    char			*cp;
	UINT32			location=0xffffffff, size=1;
	UINT32			data;
    
	argc -= 1;
	argv += 1;
	err = 0;
    while(argc > 0 && !err)
    {
        cp = argv[0];
        if (cp[0] != '-' || cp[2] != '\0')
		{
			if (location != 0xffffffff)
				data = str2value(argv[0]);
			else					
				err = 1;
    		break;
		}
		switch(tolower(cp[1]))
		{
			case 'b':
				if (argc < 2)
				{
					printf("Syntax error for \"-b\" argument!\n");
					err = 1;
					break;
				}
				location = str2value(argv[1]);
				argc -= 2;        
				argv += 2;             
				break;
			case '1':
				size = 1;
				argc--;
				argv++;
				break;
			case '2':
				size = 2;
				argc--;
				argv++;
				break;
			case '4':
				size = 4;
				argc--;
				argv++;
				break;
			default:
				printf("Syntax error for unknown argument!\n");
				err=1;
				break;
                    
		} // switch                    
    } // while        
    
	if (err || location == 0xffffffff)
		printf(sm_syntax_msg);
	else
    {
    	switch (size)
    	{
    		case 1:
    			{
    				UINT8 result;
    				REG8(location) = (UINT8)data;
    				result = REG8(location);
     				printf("Write byte 0x%X to Location 0x%X. Result=0x%x",
             				(UINT8)data, location, result);
    			}
    			break;
    		case 2:
    			{
    				UINT16 result;
    				REG16(location) = (UINT16)data;
    				result = REG16(location);
     				printf("Write word 0x%X to Location 0x%X. Result=0x%x",
             				(UINT16)data, location, result);
    			}
    			break;
    		case 4:	
    			{
    				UINT32 result;
    				REG32(location) = (UINT32)data;
    				result = REG32(location);
     				printf("Write long 0x%X to Location 0x%X. Result=0x%x",
             				(UINT32)data, location, result);
    			}
     			break;
		}
    }
}


/*----------------------------------------------------------------------
* dm_byte
*----------------------------------------------------------------------*/
static void dm_byte(UINT32 location, int length)
{
	UINT8		*start_p, *end_p, *curr_p;
	int			in_flash_range = 0;
	char		*datap, *cp, data, *bufp;
	int			i;

	start_p = (UINT8 *)location;
	end_p = start_p + length;
	
	if (length > 1024)
		length = 1024;
		
	bufp = datap = (char *)malloc(length);
	if (datap == NULL)
	{
		dbg_printf(("No free memory!\n"));
		return;
	}
	
	if (location >= (UINT32)SL2312_FLASH_SHADOW && location <= (UINT32)((UINT32)SL2312_FLASH_SHADOW + 0x10000000))
		in_flash_range = 1; 
		
	// read data
	if (in_flash_range) hal_flash_enable();
	curr_p=(UINT8 *)(location & 0xfffffff0);
	cp = datap;
	for (i=0; i<length; i++)
		*cp++ = *curr_p++;
	if (in_flash_range) hal_flash_disable();
	
	curr_p=(UINT8 *)(location & 0xfffffff0);
	do
	{
		UINT8 *p1, *p2;
        printf("0x%08x: ",(UINT32)curr_p & 0xfffffff0);
        p1 = curr_p;
        p2 = datap;
		// dump data		        
		for (i=0; i<16; i++)
        {
			if (curr_p < start_p || curr_p > end_p)
				printf("   ");
			else
			{
				data = *datap;
				printf("%02X ", data);
			}
			if (i==7)
				printf("- ");
			curr_p++;
			datap++;
        }

		// dump ascii	        
		curr_p = p1;
		datap = p2;
		for (i=0; i<16; i++)
		{
			if (curr_p < start_p || curr_p > end_p)
				printf(".");
			else
			{
				data = *datap ;
				if (data<0x20 || data>0x7f || data==0x25) 
					printf(".");
				else
					printf("%c", data);;
			}
			curr_p++;
			datap++;
		}
		printf("\n");
	} while (curr_p < end_p);
	
	free(bufp);
}

/*----------------------------------------------------------------------
* dm_short
*----------------------------------------------------------------------*/
static void dm_short(UINT32 location, int length)
{
	UINT16		*start_p, *curr_p, *end_p;
	int			i;
	UINT16		*datap, *cp, data, *bufp;
	int			in_flash_range = 0;

	start_p = (UINT16 *)location;
	end_p =  (UINT16 *)location + length;
	curr_p = (UINT16 *)((UINT32)location & 0xfffffff0);

	if (length > 1024)
		length = 1024;
		
	bufp = datap = (UINT16 *)malloc(length * 2);
	if (datap == NULL)
	{
		dbg_printf(("No free memory!\n"));
		return;
	}
	
	if (location >= (UINT32)SL2312_FLASH_SHADOW && location <= (UINT32)((UINT32)SL2312_FLASH_SHADOW + 0x10000000))
		in_flash_range = 1; 
		
	// read data
	if (in_flash_range) hal_flash_enable();
	curr_p=(UINT16 *)(location & 0xfffffff0);
	cp = datap;
	for (i=0; i<length; i++)
		*cp++ = *curr_p++;
	if (in_flash_range) hal_flash_disable();
	
	curr_p=(UINT16 *)(location & 0xfffffff0);
	do
	{
		printf("0x%08x: ",(UINT32)curr_p & 0xfffffff0);
		for (i=0; i<8; i++)
		{
			if (curr_p < start_p || curr_p > end_p)
				printf("     ");
			else
			{
				data = *datap;
				printf("%04X ", data);
			}
			if (i==3)
              printf("- ");
			curr_p++;
			datap++;
		}
		printf("\n");
	} while (curr_p < end_p);

	free(bufp);
}

/*----------------------------------------------------------------------
* dm_long
*----------------------------------------------------------------------*/
static void dm_long(UINT32 location, int length)
{
	UINT32		*start_p, *curr_p, *end_p;
	UINT32		*datap, *cp, data, *bufp;
	int			i;
	int			in_flash_range = 0;


	start_p = (UINT32 *)location;
	end_p = (UINT32 *)location + length;
	curr_p = (UINT32 *)((UINT32)location & 0xfffffff0);

	if (length > 1024)
		length = 1024;
		
	bufp = datap = (UINT32 *)malloc(length * 4);
	if (datap == NULL)
	{
		dbg_printf(("No free memory!\n"));
		return;
	}
	
	if (location >= (UINT32)SL2312_FLASH_SHADOW && location <= (UINT32)((UINT32)SL2312_FLASH_SHADOW + 0x10000000))
		in_flash_range = 1; 
		
	// read data
	if (in_flash_range) hal_flash_enable();
	curr_p=(UINT32 *)(location & 0xfffffff0);
	cp = datap;
	for (i=0; i<length; i++)
		*cp++ = *curr_p++;
	if (in_flash_range) hal_flash_disable();
	
	curr_p=(UINT32 *)(location & 0xfffffff0);
	do
	{
		printf("0x%08x: ",(UINT32)curr_p & 0xfffffff0);
		for (i=0; i<4; i++)
		{
			if (curr_p < start_p || curr_p > end_p)
               printf("     ");
			else
			{
				data = *datap;
				printf("%08X ", data);
			}
			if (i==1)
              printf("- ");
			
			curr_p++;
			datap++;
		}
        printf("\n");
	} while (curr_p <end_p);
	
	free(bufp);
}

