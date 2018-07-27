/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ata.c
* Description	: 
*		Handle ATA over Ethernet
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/12/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _AOE_H_
#define _AOE_H_

#define AOE_FIRMWARE_VER	0x4000
#define AOE_ETHER_TYPE		0x88a2
#define AOE_VERSION			1
#define AOE_ATA_CMD			0
#define AOE_ATA_CONFIG		1
#define AOE_RESPONSE		(1 << 3)
#define AOE_ERROR			(1 << 2)
#define AOE_BAD_CMD			1
#define AOE_BAD_ARG			2
#define AOE_DEV_UNAVAIL		3
#define AOE_CONFIG_ERROR	4
#define AOE_BAD_VERSION		5
#define AOE_WRITE			(1 << 0)
#define AOE_ASYNC			(1 << 1)
#define AOE_DEVICE			(1 << 4)
#define AOE_EXTEND			(1 << 6)
#define AOE_Qread			0
#define AOE_Qtest			1
#define AOE_Qprefix			2
#define AOE_Qset			3
#define AOE_Qfset			4
#define AOE_RETRY_NUM		3
#define AOE_BUFFER_COUNT	32
#define AOE_SUPPORT_DISKS	4

typedef struct {
	int				shelf;
	int				slot;
	UINT64			disk_size;
	IDE_DISK_T		*disk;
	IDE_PART_T		*part;
	unsigned char	server_mac[6];
} AOE_INFO_T;

extern AOE_INFO_T aoe_info[AOE_SUPPORT_DISKS];

typedef struct 
{
	UINT8	da[6];
	UINT8	sa[6];
	UINT16	type;
	UINT8	flags;
	UINT8	error;
	UINT16	major;
	UINT8	minor;
	UINT8	command;
	UINT8	tag[4];
} __GNU_PACKED AOE_HDR_T;

typedef struct
{
	AOE_HDR_T	hdr;
	UINT8		aflag;
	UINT8		err;
	UINT8		sectors;
	UINT8		cmd;
	UINT8		lba[6];
	UINT8		resvd[2];
	UINT8		data[1024];
} __GNU_PACKED AOE_ATA_H;

typedef struct
{
	AOE_HDR_T	hdr;
	UINT16		bufcnt;
	UINT16		firmware;
	UINT8		filler;
	UINT8		vercmd;
	UINT16		len;
	UINT8		data[1024];
} __GNU_PACKED AOE_CONF_T;

typedef struct
{
	UINT64	lba;
	UINT8	cmd;
	UINT8	status;
	UINT8	err;
	UINT8	feature;
	UINT8	sectors;
} AOE_ATA_REG_T;

enum {
	// err bits
	UNC =	1<<6,
	MC =	1<<5,
	IDNF =	1<<4,
	MCR =	1<<3,
	ABRT = 	1<<2,
	NM =	1<<1,

	// status bits
	BSY =	1<<7,
	DRDY =	1<<6,
	DF =	1<<5,
	DRQ =	1<<3,
	ERR =	1<<0,
};

#endif // _AOE_H_
