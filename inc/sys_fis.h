/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_fis.h
* Description	: 
*		Define FLASH Image System (FIS)
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	08/01/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _SYS_FIS_H
#define _SYS_FIS_H

#define FIS_ENTRY_SIZE		256
#define FIS_MAX_ENTRY		24
#define FIS_NAME_SIZE		16
#define FIS_TOTAL_SIZE		(FIS_MAX_ENTRY * FIS_ENTRY_SIZE)

typedef struct {
    unsigned char	name[FIS_NAME_SIZE];	// Null terminated name
    unsigned long	flash_base;				// Address within FLASH of image
    unsigned long	mem_base;      			// Address in memory where it executes
    unsigned long	size;          			// Length of image
    unsigned long	entry_point;			// Execution entry point
    unsigned long	data_length;			// Length of actual data
} FIS_FILE_T;

typedef struct {
	FIS_FILE_T		file;
    unsigned char	data[FIS_ENTRY_SIZE - sizeof(FIS_FILE_T) - 8];
    unsigned long	desc_cksum;				// Checksum over image descriptor
    unsigned long	file_cksum;				// Checksum over image data
} FIS_T;

int sys_fis_init(void);
void fis_ui_list(int type);
FIS_T *fis_find_image(char *filename);
void fis_set_ip_addr(UINT32 ipaddr);
void fis_ui_delete_image(int type);
void fis_ui_create_image(int type);
void fis_ui_create_default(int type);

#endif
