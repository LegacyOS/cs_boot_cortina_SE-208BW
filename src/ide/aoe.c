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
#include <define.h>
#include <board_config.h>
#include <net.h>
#include <ide/ide.h>
#include "aoe.h"

#ifdef BOARD_SUPPORT_AOE

#define aoe_printf

#define AOE_TEST_SHELF_ID	1
#define AOE_TEST_SLOT_ID	1

AOE_INFO_T		aoe_info[AOE_SUPPORT_DISKS];
static int		aoe_num;
static int		aoe_enable;

static void aoe_advertise(AOE_INFO_T *aoe);
int aoe_ata_pkt(AOE_INFO_T *aoe_p, AOE_ATA_H *aoe_ata);
static int aoe_config_pkt(AOE_INFO_T *aoe_p, AOE_CONF_T *p);
void aoe_ata_cmd(AOE_INFO_T *aoe_p, AOE_ATA_REG_T *p, UINT8 *dp);

/*----------------------------------------------------------------------
* aoe_init
*----------------------------------------------------------------------*/
void aoe_init(void)
{
	int			i, j;
	AOE_INFO_T	*aoe;
	IDE_PART_T	*part;
	
	aoe_num = 0;
	aoe_enable = 0;
	aoe = (AOE_INFO_T *)&aoe_info[0];
	part = (IDE_PART_T *)&ide_partitions[0];
	for (i=0; i<ide_part_num; i++, part++)
	{
		if (!part->present) continue;
		aoe->shelf = part->disk->ide->ide_id;
		aoe->slot = i;
		aoe->disk = part->disk;
		aoe->part = part;
		aoe->disk_size = part->sector_num * IDE_SECTOR_SIZE;
		memset((void *)aoe->server_mac, 0xff, sizeof(aoe->server_mac));
		aoe_advertise(aoe);
		aoe_num++;
		aoe++;
	}
}

/*----------------------------------------------------------------------
* aoe_input
*----------------------------------------------------------------------*/
void aoe_input(NETBUF_HDR_T *netbuf_p)
{
    AOE_HDR_T	*aoe_hdr;
    AOE_INFO_T	*aoe_p;
    UINT16		shelf;
    UINT8       slot;
    int			i, found, len;

	aoe_hdr =(AOE_HDR_T *)(netbuf_p->datap);
	
	if (!aoe_num || !aoe_enable)
		goto aoe_input_end;
	
	if (netbuf_p->len < 60)
		goto aoe_input_end;
	
	if (aoe_hdr->flags & AOE_RESPONSE)
		goto aoe_input_end;
		
	// check shelf and slot
	found = 0;
	shelf = ntohs(aoe_hdr->major);
	slot = aoe_hdr->minor;
	aoe_p = (AOE_INFO_T *)&aoe_info[0];
	
#ifdef AOE_TEST_SHELF_ID
	if (shelf == 0xffff)
		shelf = AOE_TEST_SHELF_ID;
#endif		
		
#ifdef AOE_TEST_SLOT_ID
	if (slot == 0xff)
		slot = AOE_TEST_SLOT_ID;
#endif
	
	for (i=0; i<aoe_num; i++)
	{
		// TBD: for broadcast shelf and slot
		if ((shelf == 0xffff || shelf == aoe_p->shelf) &&
			(slot == 0xff || slot == aoe_p->slot))
		{
			found = 1;
		}
		else
		{
			aoe_p++;
		}
	}	
		
	if (!found)
	{
		aoe_printf("Incorrect AoE input! Command = 0x%X Shelf=%d, Slot=%d\n", aoe_hdr->command, ntohs(aoe_hdr->major), aoe_hdr->minor);
		goto aoe_input_end;
	}
	
	aoe_printf("AoE input! Command = 0x%X Shelf=%d, Slot=%d\n", aoe_hdr->command, ntohs(aoe_hdr->major), aoe_hdr->minor);
	
	memcpy((void *)aoe_p->server_mac, aoe_hdr->sa, 6);
	
	switch (aoe_hdr->command)
	{
		case AOE_ATA_CMD:
			len = aoe_ata_pkt(aoe_p, (AOE_ATA_H *)aoe_hdr);
			break;
		case AOE_ATA_CONFIG:
			if ((len = aoe_config_pkt(aoe_p, (AOE_CONF_T *)aoe_hdr)) == 0)
				goto aoe_input_end;
			break;
		default:
			aoe_hdr->error = AOE_BAD_CMD;
			len = 1024;
			break;
	}
	
	aoe_hdr->major = htons(aoe_p->shelf);
	aoe_hdr->minor = aoe_p->slot;
	aoe_hdr->flags |= AOE_RESPONSE;
	netbuf_p->len = len - ETH_HDR_SIZE;
	enet_send(netbuf_p, aoe_p->server_mac, AOE_ETHER_TYPE);
	return;
	
aoe_input_end:
	net_free_buf((void *)netbuf_p);
	return;
}

/*----------------------------------------------------------------------
* aoe_qcget
*	TBD
*----------------------------------------------------------------------*/
static int aoe_qcget(UINT8 *p, int len)
{
	memset(p, 0xff, len);
	return len;
}

/*----------------------------------------------------------------------
* aoe_qcset
*	TBD
*----------------------------------------------------------------------*/
int aoe_qcset(UINT8 *p, int len)
{
	return len;
}

/*----------------------------------------------------------------------
* aoe_advertise
*----------------------------------------------------------------------*/
static void aoe_advertise(AOE_INFO_T *aoe)
{
	NETBUF_HDR_T	*netbuf_p;
    AOE_CONF_T		*aoe_cfg;
	UINT8			dest_msc[6];
	int len;

	if (!(netbuf_p = (NETBUF_HDR_T *)net_alloc_buf()))
	{
		printf("No free netbuf for aoe_advertise()\n");
		return;
	}
	aoe_cfg =(AOE_CONF_T *)(netbuf_p->datap);

	memset(dest_msc, 0xff, 6);
	
	aoe_cfg->hdr.flags = AOE_RESPONSE;
	aoe_cfg->hdr.major = htons(aoe->shelf);
	aoe_cfg->hdr.minor = aoe->slot;
	aoe_cfg->hdr.command = AOE_ATA_CONFIG;
	aoe_cfg->bufcnt = htons(AOE_BUFFER_COUNT);
	aoe_cfg->firmware = htons(AOE_FIRMWARE_VER);
	aoe_cfg->vercmd = 0x10 | AOE_Qread;
	
	len = aoe_qcget(aoe_cfg->data, 1024);
	aoe_cfg->len = htons(len);
	netbuf_p->len = sizeof(AOE_CONF_T) - ETH_HDR_SIZE;
	enet_send(netbuf_p, dest_msc, AOE_ETHER_TYPE);
}


/*----------------------------------------------------------------------
* aoe_getlba 
*----------------------------------------------------------------------*/
static UINT64 aoe_getlba(UINT8 *p)
{
	UINT64	v;
	int		i;

	v = 0;
	for (i = 0; i < 6; i++)
		v |= (UINT64)(*p++) << i * 8;
		
	return v;
}

/*----------------------------------------------------------------------
* aoe_ata_pkt
*----------------------------------------------------------------------*/
int aoe_ata_pkt(AOE_INFO_T *aoe_p, AOE_ATA_H *aoe_ata)
{
	AOE_ATA_REG_T	r;
	int 			len = 60;

	if (aoe_ata->sectors && (aoe_ata->aflag & AOE_WRITE) == 0)
		len = sizeof(AOE_ATA_H);
		
	memcpy((void *)&r.lba, aoe_ata->lba, 6);
	r.lba 		= aoe_getlba(aoe_ata->lba);
	r.sectors	= aoe_ata->sectors;
	r.feature	= aoe_ata->err;
	r.cmd		= aoe_ata->cmd;
	
	aoe_ata_cmd(aoe_p, &r, aoe_ata->data);
	
	aoe_ata->sectors	= r.sectors;
	aoe_ata->err		= r.err;
	aoe_ata->cmd		= r.status;
	
	return len;
}

/*----------------------------------------------------------------------
* aoe_config_pkt
*----------------------------------------------------------------------*/
static int aoe_config_pkt(AOE_INFO_T *aoe_p, AOE_CONF_T *p)	// process conf request
{
	UINT8		buf[1024];
	int			len = 0, len2;

	switch (p->vercmd & 0xf)
	{
		case AOE_Qread:
			len = aoe_qcget(p->data, 1024);
			p->len = htons(len);
			break;
		case AOE_Qtest:
			len = aoe_qcget(buf, 1024);
			if (len != ntohs(p->len))
				return 0;
			if (memcmp(buf, p->data, len) != 0)
				return 0;
			memcpy(p->data, buf, len);
			break;
		case AOE_Qprefix:
			len = aoe_qcget(buf, 1024);
			len2 = ntohs(p->len);
			if (len2 > len)
				return 0;
			if (memcmp(buf, p->data, len2) != 0)
				return 0;
			memcpy(p->data, buf, len);
			p->len = htons(len);
			break;
		case AOE_Qset:
			if (aoe_qcget(buf, 1024) > 0) {
				p->hdr.flags |= AOE_ERROR;
				p->hdr.error = AOE_CONFIG_ERROR;
				break;
			}
			len = 1024;
			// fall thru
		case AOE_Qfset:
			len = ntohs(p->len);
			aoe_qcset(p->data, len);
			break;
		default:
			p->hdr.flags |= AOE_BAD_ARG;
	}
	p->bufcnt = htons(AOE_BUFFER_COUNT);
	p->firmware = htons(AOE_FIRMWARE_VER);
	p->vercmd |= 0x10;
	return len;
}

/*----------------------------------------------------------------------
* aoe_setlba28
*----------------------------------------------------------------------*/
static void aoe_setlba28(UINT16 *ident, UINT64 lba)
{
	UINT8 *cp;

	cp = (UINT8 *) &ident[60];
	*cp++ = lba;
	*cp++ = lba >>= 8;
	*cp++ = lba >>= 8;
	*cp++ = (lba >>= 8) & 0xf;
}

/*----------------------------------------------------------------------
* ata_setlba48
*----------------------------------------------------------------------*/
static void aoe_setlba48(UINT16 *ident, UINT64 lba)
{
	UINT8 *cp;

	cp = (UINT8 *) &ident[100];
	*cp++ = lba;
	*cp++ = lba >>= 8;
	*cp++ = lba >>= 8;
	*cp++ = lba >>= 8;
	*cp++ = lba >>= 8;
	*cp++ = lba >>= 8;
}

/*----------------------------------------------------------------------
* ide_wait_drq
*----------------------------------------------------------------------*/
static inline int ide_wait_drq(IDE_INFO_T *ide, UINT32 us)
{
    UINT8	status;
    UINT32	tries;

    for (tries=0; tries<(us/10); tries++)
    {
    	hal_delay_us(10);
		status = IDE_CMD_REG8(ide->cmd_base, IDE_REG_STATUS);
        if (!(status & IDE_STAT_BSY))
        {
            if (status & IDE_STAT_DRQ)
                return 1;
            else
                return 0;
        }
    }
}

/*----------------------------------------------------------------------
* ide_wait_drq
*----------------------------------------------------------------------*/
static inline int ide_wait_busy(IDE_INFO_T *ide, UINT32 us)
{
    UINT8	status;
    UINT32	tries;
    
    for (tries=0; tries < (us/10); tries++)
    {
         hal_delay_us(10);
         status = IDE_CMD_REG8(ide->cmd_base, IDE_REG_STATUS);
         if ((status & IDE_STAT_BSY) == 0)
              return 1;
    }
    return 0;   
}

/*----------------------------------------------------------------------
* getsec
*----------------------------------------------------------------------*/
int aoe_getsec(AOE_INFO_T *aoe_p, UINT8 *place, UINT64 lba, int nsec)
{
	lba += aoe_p->part->start_sector;
	printf("Read %d Sectors, lba=0x%X\n", nsec, lba);
	if (aoe_p->disk->read(aoe_p->disk, lba, nsec, (UINT16 *)place))
		return 1;
	else
		return -1;
}

/*----------------------------------------------------------------------
* aoe_putsec
*----------------------------------------------------------------------*/
int aoe_putsec(AOE_INFO_T *aoe_p, UINT8 *place, UINT64 lba, int nsec)
{
	IDE_INFO_T		*ide;
    int				i, len;
    UINT16			data;
    UINT8			*cp;
    unsigned int	cmd_base;

	lba += aoe_p->part->start_sector;
	while (nsec--)
	{
		if (aoe_p->disk->write(aoe_p->disk, lba, (UINT8)IDE_SECTOR_SIZE, (UINT16 *)place) == 0)
			return -1;
		place += IDE_SECTOR_SIZE;
		lba += 1;
	}
	
	return 1;
}

/*----------------------------------------------------------------------
* ide_ident
*----------------------------------------------------------------------*/
static int aoe_ident(AOE_INFO_T *aoe_p, UINT64 lba, UINT16 *buf)
{
    int i;
	IDE_INFO_T		*ide;

	ide = (IDE_INFO_T *)aoe_p->disk->ide;
    IDE_CMD_REG8(ide->cmd_base, IDE_REG_DEVICE) =  ((aoe_p->disk->drive_id >> 24) & 0xff);
    IDE_CMD_REG8(ide->cmd_base, IDE_REG_COMMAND) = 0xEC;
    hal_delay_us(50000);

    if (!ide_wait_drq(ide, 1000000))
    {
		//printf("%s: NO DRQ for IDE-%d\n",
        //                __FUNCTION__, ide->ide_id);
		return -1;
	}

    for (i = 0; i < (IDE_SECTOR_SIZE / sizeof(UINT16)); i++, buf++)
		*buf = IDE_CMD_REG16(ide->cmd_base, IDE_REG_DATA);

    return 1;
}

/*----------------------------------------------------------------------
* aoe_ata_cmd
*----------------------------------------------------------------------*/
/* The ATA spec is weird in that you specify the device size as number
 * of sectors and then address the sectors with an offset.  That means
 * with LBA 28 you shouldn't see an LBA of all ones.  Still, we don't
 * check for that.
 */
void aoe_ata_cmd(AOE_INFO_T *aoe_p, AOE_ATA_REG_T *p, UINT8 *dp)		// do the ata cmd
{
	UINT64	lba;
	UINT16	*ip;
	int	n;
	
	enum { MAXLBA28SIZE = 0x0fffffff };
	
	printf("ATA command 0x%X, lba=0x%X\n", p->cmd, p->lba);

	switch (p->cmd) {
	case 0x20:		// read sectors
	case 0x30:		// write sectors
		lba = p->lba & MAXLBA28SIZE;
		break;
	case 0x24:		// read sectors ext
	case 0x34:		// write sectors ext
		lba = p->lba & 0x0000ffffffffffffLL;	// full 48
		break;
	case 0xe7:		// flush cache
		return;
	case 0xec:		// identify device
		// printf("ATA command 0x%X, lba=0x%X\n", p->cmd, p->lba);
		if (aoe_ident(aoe_p, p->lba, ( UINT16 *)dp) < 0)
		{
			p->err = 1;
			p->status = BSY;
		}
		else
		{
			p->err = 0;
			p->status = DRDY;
		}
		return;
	default:
		p->status = DRDY | ERR;
		p->err = ABRT;
		return;
	}

#if 0
	if (lba + p->sectors > size) {
		p->err = IDNF;
		p->status = DRDY | ERR;
		p->lba = lba;
		return;
	}
#endif

	if (p->cmd == 0x20 || p->cmd == 0x24) {
		n = aoe_getsec(aoe_p, dp, lba, p->sectors);
		if (n == -1) {
			dbg_printf(("read block device"));
			p->status |= ERR;
		}
	} else {
		n = aoe_putsec(aoe_p, dp, lba, p->sectors);
		if (n == -1) {
			dbg_printf(("write block device"));
			p->status |= ERR;
		}
	}
	p->lba += p->sectors;
	p->sectors = 0;
	p->status = DRDY;
	p->err = 0;
}

/*----------------------------------------------------------------------
*  aoe_ui_cmd
*	CLI command for AOE
*----------------------------------------------------------------------*/
void aoe_ui_cmd(char argc, char *argv[])
{
	if (argc != 2)
	{
		printf("AOE is %s\n", (aoe_enable) ? "enabled" : "disabled");
		printf("Syntax: aoe [enable | disable]\n");
		return;
	}
	
	if (strncasecmp(argv[1], "enable", 6)==0)
	{
		ide_init();
		aoe_enable = 1;
	}
	else if (strncasecmp(argv[1], "disable", 7)==0)
		aoe_enable = 0;
	else
		printf("Syntax: aoe [enable | disable]\n");
}
		

#endif // BOARD_SUPPORT_AOE

