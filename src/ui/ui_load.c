/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_load.c
* Description	: 
*		Load file by/from TFTP, xModem, disk, flash
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/19/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include <net.h>
#include "ui_cli.h"
#include <ide/ide.h>
#include <ide/ext2fs.h>

static const char *load_syntax_msg[]=
{
    "Usage: load -m [tftp | xmodem | disk | flash] -b [location]\n",
    "  Load data to [location] by/from TFTP, xModem, disk, or flash\n",
    ""
};

static char disk_download_file[128];
static unsigned long flash_load_offset;
static unsigned long flash_load_size;

static int do_disk_download(char *bufp, int *size);
static int do_flash_download(char *location, int *size);

extern FILE_T *ext2fs_open(char *filepath);
extern int do_xmodem_download(char *bufp, int buf_size, int *size);
extern int do_tftp_download(char *bufp, int buf_size, int *size);

/*----------------------------------------------------------------------
* 	printf_load_syntax
*----------------------------------------------------------------------*/
static void printf_load_syntax(void)
{
	int i = 0;
	
	while(*load_syntax_msg[i]) 
	{
		printf(load_syntax_msg[i]);
		i++;
	}
}

/*----------------------------------------------------------------------
* cli_load_cmd
* 	Usage: load -m [tftp | xmodem | disk | flash] -b [location]
*
*	argc 	= 6
*	argv[0]	= load
*	argv[1]	= -m or -b
*----------------------------------------------------------------------*/
void cli_load_cmd(char argc, char *argv[])
{
    UINT32	location, offset, size;
    int 	err;
    char	*cp, *filename;
    int 	mode;
	int		got_size = 0;
    
    if (argc < 5)
    {
    	printf_load_syntax();
    	return;
    }

	err = 0;
	argv++;
	argc--;
	while (*argv[0] == '-' && !err && argc)
	{
		cp = argv[0];
		cp++;
		switch (*cp)
		{
			case 'm': 
				if (argc < 2)
				{
					err = 1;
					break;
				}
				if (strncasecmp(argv[1], "xMODEM", 6) == 0)
				{
					mode = 0;
					argv += 2;					
					argc -= 2;
				}
				else if (strncasecmp(argv[1], "tftp", 4) == 0)
				{
					mode = 1;
					argv += 2;					
					argc -= 2;
				}
				else if (strncasecmp(argv[1], "disk", 4) == 0)
				{
					mode = 2;
					argv += 2;					
					argc -= 2;
				}
				else if (strncasecmp(argv[1], "flash", 5) == 0)
				{
					if (argc >= 4)
					{
						mode = 3;
						argv += 2;					
						argc -= 2;
					}
					else
					{
						err =1;
					}
				}
				else
				{
					err = 1;
					break;
				}
				break;
			case 'b': 
				if (argc < 2)
				{
					err = 1;
					break;
				}
				location = str2value(argv[1]);
				argv += 2;					
				argc -= 2;
				break;
			default:
				err = 1;
				break;
		}
	}
	
	if (err)
	{
		printf_load_syntax();
		return;
	}
	
	switch (mode)
	{
		case 0: // xModem
			do_xmodem_download((char *)location, BOARD_BOOT2_MALLOC_BASE - location, &got_size);
			break;
		case 1: // TFTP
			do_tftp_download((char *)location, BOARD_BOOT2_MALLOC_BASE - location, &got_size);
			break;
		case 2: // Disk
			do_disk_download((char *)location, &got_size);
			break;
		case 3: // Flash
			do_flash_download((char *)location, &got_size);
			break;
	}
}

/*----------------------------------------------------------------------
* do_disk_download
*  return 
*		TRUE if OK, FALSE if failed
*----------------------------------------------------------------------*/
static int do_disk_download(char *bufp, int *size)
{
#ifndef BOARD_SUPPORT_IDE
	printf("Not supported!\n");
#else
	int				rc;
	FILE_T			*fp;
    unsigned long	filesize, nbytes;
	
	printf("Filename: ");
	rc = ui_gets(disk_download_file, sizeof(disk_download_file));
	if (rc == 0)
	{
		printf("Illegal filename!\n");
		return FALSE;
	}	
	
	ide_init();
	ext2fs_init();
	
	fp = ext2fs_open((char *)disk_download_file);
	if (fp == NULL)
	{
		printf("Failed to open file %s\n", disk_download_file);
		return FALSE;
	}

	filesize = ext2fs_file_size(fp);
	nbytes = ext2fs_read((void *)fp, (char *)bufp, filesize);
	if (nbytes != filesize)
	{
		*size = nbytes;
		printf("Failed to read %s. Expect read size: %lu but get %lu\n", 
				(char *)disk_download_file, filesize, nbytes);
		ext2fs_close(fp);
		return FALSE;
	}
	printf("Load %s size %d to 0x%08X\n", (char *)disk_download_file, filesize, bufp);
	ext2fs_close(fp);
	*size = nbytes;
	
	return TRUE;
#endif // BOARD_SUPPORT_IDE
}

/*----------------------------------------------------------------------
* do_flash_download
*  return 
*		TRUE if OK, FALSE if failed
*----------------------------------------------------------------------*/
static int do_flash_download(char *location, int *size)
{
	char	buf[20];
	int		rc;
	
	sprintf(buf, "0x%08X", flash_load_offset);
	printf("Flash Offset Address: ");
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
	{
		printf("Illegal starting offset!\n");
		return FALSE;
	}
	flash_load_offset = str2value(buf);

	if (flash_load_size)
		sprintf(buf, "0x%08X", flash_load_size);
	else
		buf[0] = 0;
	printf("Size to be loaded: ");
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
	{
		printf("Illegal loading size!\n");
		return FALSE;
	}
	flash_load_size = str2value(buf);
	
	printf("Load flash data from 0x%08X size %u to %u\n", 
			flash_load_offset + SL2312_FLASH_SHADOW, flash_load_size, location);
	hal_flash_enable();
	memcpy(location, flash_load_offset + SL2312_FLASH_SHADOW, flash_load_size);
	hal_flash_disable();
	
	return TRUE;
}

