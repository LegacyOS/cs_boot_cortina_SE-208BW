/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_vctl.h
* Description	: 
*		Define FLASH VCTL (Version Control)
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/20/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _SYS_VCTL_H
#define _SYS_VCTL_H

#define VCTL_HDR_MAGIC		"FLFM"
#define VCTL_ENTRY_MAGIC	"FLEN"
#define VCTL_MAGIC_SIZE		4

typedef struct
{
	char			header[4];
	unsigned int	entry_num;
} VCTL_HDR_T;

typedef struct
{
	char			header[4];
	unsigned int	size;
	unsigned int	type;
	char			majorver[4];
	char			minorver[4];
} VCTL_ENTRY_T;

#define VERCTL_ADDR			(BOARD_FLASH_VCTL_ADDR)
#ifdef BOARD_NAND_BOOT		
#define VERCTLSIZE			(BOARD_FLASH_VCTLIMG_SIZE)
#else
#define VERCTLSIZE			(BOARD_FLASH_VCTL_SIZE)
#endif
#define ENTRY_LEN			(sizeof(unsigned int)*4+4)

// VCTL type
#define VCT_VENDORSPEC		0
#define VCT_BOOTLOADER		1
#define VCT_KERNEL		2
#define VCT_VERCTL		3
#define VCT_CURRCONF		4
#define VCT_DEFAULTCONF		5
#define VCT_ROOTFS		6
#define VCT_APP			7
#define VCT_VLAN		8
#define VCT_RECOVER		9
#define VCT_IP			0x80
#define VCT_BOOT_FILE		0x81

#endif
