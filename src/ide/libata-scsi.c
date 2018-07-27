#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scsi_cmd.h"
//#include "../usb/Pe_usb.h"
#include "ide.h"

#ifdef USB_DEV_MO

#if 0
#define DPRINTK(fmt, args...)	printf("modedb %s: " fmt, __FUNCTION__ , ## args)
#define VPRINTK(fmt, args...)	printf("modedb %s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#define VPRINTK(fmt, args...)
#endif


//debug_Aaron
//#define DEBUG_SCSI	1

void ata_scsi_set_sense(u8 *cmd, u8 sk, u8 asc, u8 ascq);

typedef unsigned int (*ata_xlat_func_t)( const u8 *scsicmd);

#define RW_RECOVERY_MPAGE 0x1
#define RW_RECOVERY_MPAGE_LEN 12
#define CACHE_MPAGE 0x8
#define CACHE_MPAGE_LEN 20
#define CONTROL_MPAGE 0xa
#define CONTROL_MPAGE_LEN 12
#define ALL_MPAGES 0x3f
#define ALL_SUB_MPAGES 0xff

//debug_Aaron
static const u8 def_iec_mpage[] = {
	0x1c, 0xa, 0x08, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 def_rw_recovery_mpage[] = {
	RW_RECOVERY_MPAGE,
	RW_RECOVERY_MPAGE_LEN - 2,
	(1 << 7) |	/* AWRE, sat-r06 say it shall be 0 */
	    (1 << 6),	/* ARRE (auto read reallocation) */
	0,		/* read retry count */
	0, 0, 0, 0,
	0,		/* write retry count */
	0, 0, 0
};

static const u8 def_cache_mpage[CACHE_MPAGE_LEN] = {
	CACHE_MPAGE,
	CACHE_MPAGE_LEN - 2,
	0,		/* contains WCE, needs to be 0 for logic */
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,		/* contains DRA, needs to be 0 for logic */
	0, 0, 0, 0, 0, 0, 0
};

static const u8 def_control_mpage[CONTROL_MPAGE_LEN] = {
	CONTROL_MPAGE,
	CONTROL_MPAGE_LEN - 2,
	2,	/* DSENSE=0, GLTSD=1 */
	0,	/* [QAM+QERR may be 1, see 05-359r1] */
	0, 0, 0, 0, 0xff, 0xff,
	0, 30	/* extended self test time, see 05-359r1 */
};

extern u8*   u8VirtOTGMemory;//INT8U*   u8VirtOTGMemory = NULL;
//extern CBW tOTGCBW;
//u8*   u8VirtOTGMemory = NULL;
CBW tOTGCBW;
CSW tOTGCSW;

extern IDE_DISK_T	*first_disk;
 u64 dev_sectors;
 u16 dev_id[512];
u8 *psscsicmd;

/**
 *	ata_msense_push - Push data onto MODE SENSE data output buffer
 *	@ptr_io: (input/output) Location to store more output data
 *	@last: End of output data buffer
 *	@buf: Pointer to BLOB being added to output buffer
 *	@buflen: Length of BLOB
 *
 *	Store MODE SENSE data on an output buffer.
 *
 *	LOCKING:
 *	None.
 */

static void ata_msense_push(u8 **ptr_io, const u8 *last,
			    const u8 *buf, unsigned int buflen)
{
	u8 *ptr = *ptr_io;

	if ((ptr + buflen - 1) > last)
		return;

	memcpy(ptr, buf, buflen);

	ptr += buflen;

	*ptr_io = ptr;
}


/**
 *	ata_msense_ctl_mode - Simulate MODE SENSE control mode page
 *	@dev: Device associated with this MODE SENSE command
 *	@ptr_io: (input/output) Location to store more output data
 *	@last: End of output data buffer
 *
 *	Generate a generic MODE SENSE control mode page.
 *
 *	LOCKING:
 *	None.
 */

static unsigned int ata_msense_ctl_mode(u8 **ptr_io, const u8 *last)
{
	ata_msense_push(ptr_io, last, def_control_mpage,
			sizeof(def_control_mpage));
	return sizeof(def_control_mpage);
}

/**
 *	ata_msense_caching - Simulate MODE SENSE caching info page
 *	@id: device IDENTIFY data
 *	@ptr_io: (input/output) Location to store more output data
 *	@last: End of output data buffer
 *
 *	Generate a caching info page, which conditionally indicates
 *	write caching to the SCSI layer, depending on device
 *	capabilities.
 *
 *	LOCKING:
 *	None.
 */

static unsigned int ata_msense_caching(u16 *id, u8 **ptr_io,
				       const u8 *last)
{
	u8 page[CACHE_MPAGE_LEN];

	memcpy(page, def_cache_mpage, sizeof(page));
	if (ata_id_wcache_enabled(id))
		page[2] |= (1 << 2);	/* write cache enable */
	if (!ata_id_rahead_enabled(id))
		page[12] |= (1 << 5);	/* disable read ahead */

	ata_msense_push(ptr_io, last, page, sizeof(page));
	return sizeof(page);
}

//debug_Aaron
static unsigned int ata_msense_iec(u8 **ptr_io, const u8 *last)
{

        ata_msense_push(ptr_io, last, def_iec_mpage,
                        sizeof(def_iec_mpage));
        return sizeof(def_iec_mpage);
}
/**
 *	ata_msense_rw_recovery - Simulate MODE SENSE r/w error recovery page
 *	@dev: Device associated with this MODE SENSE command
 *	@ptr_io: (input/output) Location to store more output data
 *	@last: End of output data buffer
 *
 *	Generate a generic MODE SENSE r/w error recovery page.
 *
 *	LOCKING:
 *	None.
 */

static unsigned int ata_msense_rw_recovery(u8 **ptr_io, const u8 *last)
{

	ata_msense_push(ptr_io, last, def_rw_recovery_mpage,
			sizeof(def_rw_recovery_mpage));
	return sizeof(def_rw_recovery_mpage);
}




/**
 *	scsi_10_lba_len - Get LBA and transfer length
 *	@scsicmd: SCSI command to translate
 *
 *	Calculate LBA and transfer length for 10-byte commands.
 *
 *	RETURNS:
 *	@plba: the LBA
 *	@plen: the transfer length
 */

static void scsi_10_lba_len(const u8 *scsicmd, u64 *plba, u32 *plen)
{
	u64 lba = 0;
	u32 len = 0;

	VPRINTK("ten-byte command\n");

	lba |= ((u64)scsicmd[2]) << 24;
	lba |= ((u64)scsicmd[3]) << 16;
	lba |= ((u64)scsicmd[4]) << 8;
	lba |= ((u64)scsicmd[5]);

	len |= ((u32)scsicmd[7]) << 8;
	len |= ((u32)scsicmd[8]);
        
	*plba = lba;
	*plen = len;
#ifdef DEBUG_SCSI
	//printf("%s: lba=%ld, len=%d\r\n", __func__, *plba, *plen);
#endif
}


/**
 *	scsi_16_lba_len - Get LBA and transfer length
 *	@scsicmd: SCSI command to translate
 *
 *	Calculate LBA and transfer length for 16-byte commands.
 *
 *	RETURNS:
 *	@plba: the LBA
 *	@plen: the transfer length
 */

static void scsi_16_lba_len(const u8 *scsicmd, u64 *plba, u32 *plen)
{
	u64 lba = 0;
	u32 len = 0;

	VPRINTK("sixteen-byte command\n");

	lba |= ((u64)scsicmd[2]) << 56;
	lba |= ((u64)scsicmd[3]) << 48;
	lba |= ((u64)scsicmd[4]) << 40;
	lba |= ((u64)scsicmd[5]) << 32;
	lba |= ((u64)scsicmd[6]) << 24;
	lba |= ((u64)scsicmd[7]) << 16;
	lba |= ((u64)scsicmd[8]) << 8;
	lba |= ((u64)scsicmd[9]);

	len |= ((u32)scsicmd[10]) << 24;
	len |= ((u32)scsicmd[11]) << 16;
	len |= ((u32)scsicmd[12]) << 8;
	len |= ((u32)scsicmd[13]);

	*plba = lba;
	*plen = len;
#ifdef DEBUG_SCSI
        printf("%s: lba=%ld, len=%d\r\n", __func__, *plba, *plen);
#endif
}


/**
 *	scsi_6_lba_len - Get LBA and transfer length
 *	@scsicmd: SCSI command to translate
 *
 *	Calculate LBA and transfer length for 6-byte commands.
 *
 *	RETURNS:
 *	@plba: the LBA
 *	@plen: the transfer length
 */

static void scsi_6_lba_len(const u8 *scsicmd, u64 *plba, u32 *plen)
{
	u64 lba = 0;
	u32 len = 0;

	VPRINTK("six-byte command\n");

	lba |= ((u64)scsicmd[2]) << 8;
	lba |= ((u64)scsicmd[3]);

	len |= ((u32)scsicmd[4]);

	*plba = lba;
	*plen = len;
#ifdef DEBUG_SCSI
        printf("%s: lba=%ld, len=%d\r\n", __func__, *plba, *plen);
#endif
}


/**
 *	ata_scsi_rw_xlat - Translate SCSI r/w command into an ATA one
 *	@qc: Storage for translated ATA taskfile
 *	@scsicmd: SCSI command to translate
 *
 *	Converts any of six SCSI read/write commands into the
 *	ATA counterpart, including starting sector (LBA),
 *	sector count, and taking into account the device's LBA48
 *	support.
 *
 *	Commands %READ_6, %READ_10, %READ_16, %WRITE_6, %WRITE_10, and
 *	%WRITE_16 are currently supported.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 *
 *	RETURNS:
 *	Zero on success, non-zero on error.
 */

static unsigned int ata_scsi_rw_xlat(const u8 *scsicmd)
{

	u64 block;
	u32 n_byte,n_block;
	u32 i,j,*pbuf,bulk_no;;

	/* Calculate the SCSI LBA and transfer length. */
	switch (scsicmd[0]) {
	case READ_10:
	case WRITE_10:
		scsi_10_lba_len(scsicmd, &block, &n_block);
		break;
	case READ_6:
	case WRITE_6:
		scsi_6_lba_len(scsicmd, &block, &n_block);

		/* for 6-byte r/w commands, transfer length 0
		 * means 256 blocks of data, not 0 block.
		 */
		if (!n_block)
			n_block = 256;
		break;
	case READ_16:
	case WRITE_16:
		scsi_16_lba_len(scsicmd, &block, &n_block);
		break;
	default:
//	        printf("ata_scsi_rw_xlat default \n");
		DPRINTK("no-byte command\n");
		goto invalid_fld;
	}

	/* Check and compose ATA command */
	if (!n_block)
		/* For 10-byte and 16-byte SCSI R/W commands, transfer
		 * length 0 means transfer 0 block of data.
		 * However, for ATA R/W commands, sector count 0 means
		 * 256 or 65536 sectors, not 0 sectors as in SCSI.
		 *
		 * WARNING: one or two older ATA drives treat 0 as 0...
		 */
		goto nothing_to_do;

	n_byte = n_block*512;

	/* The request -may- be too large for LBA48. */
	if ((block >> 48) || (n_block > 65536))
		goto out_of_range;
			
	// call ide API to rw		
	if((scsicmd[0]==READ_10)||(scsicmd[0]==READ_6)||(scsicmd[0]==READ_16))
		first_disk->read_dma(first_disk,block,n_byte,u8VirtOTGMemory);
	else if((scsicmd[0]==WRITE_10)||(scsicmd[0]==WRITE_6)||(scsicmd[0]==WRITE_16)){
#ifdef USB_SPEED_UP
		pbuf = u8VirtOTGMemory;
		if(n_byte>=SW_PIPE_SIZE){
			for(i=0;i<(n_byte/SW_PIPE_SIZE);i++){
				bulk_no = 1;
				while(pbuf[(i*SW_PIPE_SIZE+SW_PIPE_SIZE)/4-1]==CHECK_PATTERN1)
					hal_delay_us(20);
				for(j=i+1;j<(n_byte/SW_PIPE_SIZE);j++)
					if((pbuf[(j*SW_PIPE_SIZE+SW_PIPE_SIZE)/4-1]!=CHECK_PATTERN1)){
						bulk_no++;
					}
				first_disk->write_dma(first_disk,block+i*(SW_PIPE_SIZE/IDE_SECTOR_SIZE),bulk_no*SW_PIPE_SIZE,&u8VirtOTGMemory[i*SW_PIPE_SIZE]);
				if(bulk_no>1)
					i+=bulk_no-1;
			}
			if(n_byte&(SW_PIPE_SIZE-1)){		// process remainder
				if(i>(n_byte/SW_PIPE_SIZE))
					i = (n_byte/SW_PIPE_SIZE);
				// start DMA when remainder data arrived
				while(pbuf[(i*SW_PIPE_SIZE+(n_byte&(SW_PIPE_SIZE-1)))/4-1] == CHECK_PATTERN1)
					hal_delay_us(10);
				first_disk->write_dma(first_disk,block+i*(SW_PIPE_SIZE/IDE_SECTOR_SIZE),n_byte&(SW_PIPE_SIZE-1),&u8VirtOTGMemory[i*SW_PIPE_SIZE]);
			}
		}
		else
			first_disk->write_dma(first_disk,block,n_byte,u8VirtOTGMemory);
#else	
		first_disk->write_dma(first_disk,block,n_byte,u8VirtOTGMemory);
#endif
	}
	tOTGCSW.u32DataResidue = n_byte;
	return 0;

invalid_fld:
//        printf("ata_scsi_rw_xlat invalid_fld \n");
	ata_scsi_set_sense(scsicmd, ILLEGAL_REQUEST, 0x24, 0x0);
	/* "Invalid field in cbd" */
	return 1;

out_of_range:
//        printf("ata_scsi_rw_xlat out_of_range \n");
	ata_scsi_set_sense(scsicmd, ILLEGAL_REQUEST, 0x21, 0x0);
	printf("Out of range!![REQ. must small than 32MB]\n");
	return 1;

nothing_to_do:
//	qc->scsicmd->result = SAM_STAT_GOOD;
	return 1;
}


/**
 *	ata_scsi_verify_xlat - Translate SCSI VERIFY command into an ATA one
 *	@qc: Storage for translated ATA taskfile
 *	@scsicmd: SCSI command to translate
 *
 *	Converts SCSI VERIFY command to an ATA READ VERIFY command.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 *
 *	RETURNS:
 *	Zero on success, non-zero on error.
 */

static unsigned int ata_scsi_verify_xlat(const u8 *scsicmd)
{

	u64 block;
	u32 n_block;

	if (scsicmd[0] == VERIFY)
		scsi_10_lba_len(scsicmd, &block, &n_block);
	else if (scsicmd[0] == VERIFY_16)
		scsi_16_lba_len(scsicmd, &block, &n_block);
	else
		goto invalid_fld;

	if (!n_block)
		goto nothing_to_do;
	if (block >= dev_sectors)
		goto out_of_range;
	if ((block + n_block) > dev_sectors)
		goto out_of_range;


	if (n_block > (64 * 1024))
		goto invalid_fld;

	// call IDE API VERIFY

	return 0;

invalid_fld:
//        printf("ata_scsi_verify_xlat invalid_fld \n");
	ata_scsi_set_sense(scsicmd, ILLEGAL_REQUEST, 0x24, 0x0);
	/* "Invalid field in cbd" */
	return 1;

out_of_range:
//        printf("ata_scsi_verify_xlat out_of_range \n");
	ata_scsi_set_sense(scsicmd, ILLEGAL_REQUEST, 0x21, 0x0);
	/* "Logical Block Address out of range" */
	return 1;

nothing_to_do:
//	qc->scsicmd->result = SAM_STAT_GOOD;
	return 1;
}


/**
 *	ata_scsi_start_stop_xlat - Translate SCSI START STOP UNIT command
 *	@qc: Storage for translated ATA taskfile
 *	@scsicmd: SCSI command to translate
 *
 *	Sets up an ATA taskfile to issue STANDBY (to stop) or READ VERIFY
 *	(to start). Perhaps these commands should be preceded by
 *	CHECK POWER MODE to see what power mode the device is already in.
 *	[See SAT revision 5 at www.t10.org]
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 *
 *	RETURNS:
 *	Zero on success, non-zero on error.
 */

static unsigned int ata_scsi_start_stop_xlat(const u8 *scsicmd)
{

	if (scsicmd[1] & 0x1) {
		;	/* ignore IMMED bit, violates sat-r05 */
	}
	if (scsicmd[4] & 0x2)
		goto invalid_fld;       /* LOEJ bit set not supported */
	if (((scsicmd[4] >> 4) & 0xf) != 0)
		goto invalid_fld;       /* power conditions not supported */
	if (scsicmd[4] & 0x1) {
		;// Call IDE API to VERIFY
	}
		
	/*
	 * Standby and Idle condition timers could be implemented but that
	 * would require libata to implement the Power condition mode page
	 * and allow the user to change it. Changing mode pages requires
	 * MODE SELECT to be implemented.
	 */

	
	return 0;

invalid_fld:
//        printf("ata_scsi_start_stop_xlat invalid_fld \n");
	ata_scsi_set_sense(scsicmd, ILLEGAL_REQUEST, 0x24, 0x0);
	/* "Invalid field in cbd" */
	return 1;
}

/**
 *	ata_scsi_rbuf_fill - wrapper for SCSI command simulators
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@actor: Callback hook for desired SCSI command simulator
 *
 *	Takes care of the hard work of simulating a SCSI command...
 *	Mapping the response buffer, calling the command's handler,
 *	and handling the handler's return value.  This return value
 *	indicates whether the handler wishes the SCSI command to be
 *	completed successfully (0), or not (in which case cmd->result
 *	and sense buffer are assumed to be set).
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

void ata_scsi_rbuf_fill(u32 len, unsigned int (*actor) (u8 *rbuf, unsigned int buflen))
{
	u8 *rbuf=u8VirtOTGMemory;
	
	memset(rbuf, 0, len);
	actor(rbuf, len);

}


static void ata_scsi_invalid_field(u8 *cmd)
{
	ata_scsi_set_sense(cmd, ILLEGAL_REQUEST, 0x24, 0x0);
	/* "Invalid field in cbd" */
}


/**
 *	ata_scsi_set_sense - Set SCSI sense data and status
 *	@cmd: SCSI request to be handled
 *	@sk: SCSI-defined sense key
 *	@asc: SCSI-defined additional sense code
 *	@ascq: SCSI-defined additional sense code qualifier
 *
 *	Helper function that builds a valid fixed format, current
 *	response code and the given sense key (sk), additional sense
 *	code (asc) and additional sense code qualifier (ascq) with
 *	a SCSI command status of %SAM_STAT_CHECK_CONDITION and
 *	DRIVER_SENSE set in the upper bits of scsi_cmnd::result .
 *
 *	LOCKING:
 *	Not required
 */

void ata_scsi_set_sense(u8 *cmd, u8 sk, u8 asc, u8 ascq)
{
        int i;
        if(!(cmd[0]==0x1a))
        {
        tOTGCSW.u32DataResidue = 0x12;
         for (i=0;i<tOTGCSW.u32DataResidue;i++)
              {
              	u8VirtOTGMemory[i]=0x0;
                }
	u8VirtOTGMemory[0] = 0x80 |0x70;
	u8VirtOTGMemory[2] = sk;
	u8VirtOTGMemory[7] = 18-8;
	u8VirtOTGMemory[12] = asc;
	u8VirtOTGMemory[13] = ascq;
//	if (tOTGCSW.u32DataResidue>= 0x18)
        }
        else
        {
//        tOTGCSW.u32DataResidue = 0xc0;
        memset(u8VirtOTGMemory, 0x00, MAX_BUFFER_SIZE);	
        }
        
}


/**
 *	ata_dev_id_string - Convert IDENTIFY DEVICE page into string
 *	@id: IDENTIFY DEVICE results we will examine
 *	@s: string into which data is output
 *	@ofs: offset into identify device page
 *	@len: length of string to return. must be an even number.
 *
 *	The strings in the IDENTIFY DEVICE page are broken up into
 *	16-bit chunks.  Run through the string, and output each
 *	8-bit chunk linearly, regardless of platform.
 *
 *	LOCKING:
 *	caller.
 */

void ata_dev_id_string(u16 *id, unsigned char *s,
		       unsigned int ofs, unsigned int len)
{
	unsigned int c;

	while (len > 0) {
		c = id[ofs] >> 8;
		*s = c;
		s++;

		c = id[ofs] & 0xff;
		*s = c;
		s++;

		ofs++;
		len -= 2;
	}
}


static int do_read_format_capacities(u8 *rbuf, unsigned int buflen)
{
//#if 0
	u8		*buf = rbuf;
	u64 n_sectors;

	if (ata_id_has_lba(dev_id)) {
		if (ata_id_has_lba48(dev_id))
			n_sectors = ata_id_u64(dev_id, 100);
		else
			n_sectors = ata_id_u32(dev_id, 60);
	} else {
		/* CHS default translation */
		n_sectors = dev_id[1] * dev_id[3] * dev_id[6];

		if (ata_id_current_chs_valid(dev_id))
			/* CHS current translation */
			n_sectors = ata_id_u32(dev_id, 57);
	}

	n_sectors--;		/* ATA TotalUserSectors - 1 */

	buf[0] = buf[1] = buf[2] = 0;
	buf[3] = 8;		// Only the Current/Maximum Capacity Descriptor
	buf += 4;

	buf[0] = n_sectors >> 24;
	buf[1] = n_sectors >> 16;
	buf[2] = n_sectors >> 8;
	buf[3] = n_sectors & 0xff;

	buf[5] = 0;
	buf[6] = 0x02;
	buf[7] = 0x0;	
//	put_be32(&buf[0], n_sectors);		// Number of blocks
//	put_be32(&buf[4], 512);				// Block length
//	buf[4] = 0x02;					// Current capacity
    buf[4] = 0x00;					// Current capacity stone test

#ifdef DEBUG_SCSI
        printf("%s: n_sectors=%ld\r\n", __func__, n_sectors);
#endif	
	return 0;
//#endif
//MassStorageState eState;
//tOTGCSW.u8Status = CSW_STATUS_CMD_FAIL;
//eState = 
}

/**
 *	ata_scsiop_inq_std - Simulate INQUIRY command
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Returns standard device identification data associated
 *	with non-EVPD INQUIRY command output.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_inq_std( u8 *rbuf, unsigned int buflen)
{
	u8 hdr[5],versions[5];
//	static char vendor_id[] = "Linux   ";
//	static char product_id[] = "File-Stor Gadget";
	
	//hdr[0] =  0;
	hdr[1] =  0x0;
	//debug_Aaron on 12/28/2006 according to MS spec for mass storage version should >= 3 
	hdr[0] =  0;
//	hdr[0] =  0x7F;
	//hdr[2] =  2;
	hdr[2] =  5;
	hdr[3] =  2;   //always set to 2
	hdr[4] =  91;
	
	/* set scsi removeable (RMB) bit per ata bit */
	if (ata_id_removeable(dev_id))
		hdr[1] |= (1 << 7);

	VPRINTK("ENTER\n");
	memset(rbuf,0,100);
	memcpy(rbuf, hdr, sizeof(hdr));
#if 0
    memcpy(&rbuf[8],vendor_id, 8);
    memcpy(&rbuf[16],product_id, 16);
#endif
//#if 0
	if (buflen > 35) {
		memcpy(&rbuf[8], "ATA     ", 8);
		ata_dev_id_string(dev_id, &rbuf[16], ATA_ID_PROD_OFS, 16);
		ata_dev_id_string(dev_id, &rbuf[32], ATA_ID_FW_REV_OFS, 4);
		if ((rbuf[32] == 0) || (rbuf[32] == ' '))
			memcpy(&rbuf[32], "n/a ", 4);
	}

//	if (buflen > 63) {
		versions[0] = 0x60;	/* SAM-3 (no version claimed) */

		versions[1] = 0x03;
		versions[2] = 0x20;	/* SBC-2 (no version claimed) */

		//versions[3] = 0x02;
		//versions[4] = 0x60;	/* SPC-3 (no version claimed) */
		
		versions[3] = 0x02;
		versions[4] = 0x60;	/* ATA-6 (no version claimed) */

		memcpy(rbuf + 59, versions, sizeof(versions));
//	}
//        tOTGCSW.u32DataResidue = 0x40;
//#endif
//        printf("tOTGCSW.u32DataResidue in std %x \n",tOTGCSW.u32DataResidue);

#ifdef DEBUG_SCSI
        printf("%s: INQ STD\r\n", __func__);
#endif	
	return 0;
}


/**
 *	ata_scsiop_inq_00 - Simulate INQUIRY EVPD page 0, list of pages
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Returns list of inquiry EVPD pages available.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_inq_00(u8 *rbuf, unsigned int buflen)
{
	u8 pages[3];
	pages[0] = 0x00;	/* page 0x00, this page */
	pages[1] = 0x80;	/* page 0x80, unit serial no page */
	pages[2] = 0x83;	/* page 0x83, device ident page */

	rbuf[3] = sizeof(pages);	/* number of supported EVPD pages */

	if (buflen > 6)
		memcpy(rbuf + 4, pages, sizeof(pages));

#ifdef DEBUG_SCSI
        printf("%s: INQ 00\r\n", __func__);
#endif 
	return 0;
}


/**
 *	ata_scsiop_inq_80 - Simulate INQUIRY EVPD page 80, device serial number
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Returns ATA device serial number.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_inq_80( u8 *rbuf, unsigned int buflen)
{
	u8 hdr[4];
	hdr[0] = 0;
	hdr[1] = 0x80;			/* this page code */
	hdr[2] = 0;
	hdr[3] = 20;		/* page len */

	memcpy(rbuf, hdr, sizeof(hdr));

	if (buflen > (ATA_SERNO_LEN + 4 - 1))
		ata_dev_id_string(dev_id, (unsigned char *) &rbuf[4],
				  ATA_ID_SERNO_OFS, 20);

#ifdef DEBUG_SCSI
        printf("%s: INQ 80\r\n", __func__);
#endif 
	return 0;
}

static const char *inq_83_str = "Linux ATA-SCSI simulator";
/**
 *	ata_scsiop_inq_83 - Simulate INQUIRY EVPD page 83, device identity
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Returns device identification.  Currently hardcoded to
 *	return "Linux ATA-SCSI simulator".
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_inq_83( u8 *rbuf, unsigned int buflen)
{
	rbuf[1] = 0x83;			/* this page code */
	rbuf[3] = 4 + strlen(inq_83_str);	/* page len */

	/* our one and only identification descriptor (vendor-specific) */
	if (buflen > (strlen(inq_83_str) + 4 + 4 - 1)) {
		rbuf[4 + 0] = 2;	/* code set: ASCII */
		rbuf[4 + 3] = strlen(inq_83_str);
		memcpy(rbuf + 4 + 4, inq_83_str, strlen(inq_83_str));
	}

#ifdef DEBUG_SCSI
        printf("%s: INQ 83\r\n", __func__);
#endif 
	return 0;
}


/**
 *	ata_scsiop_mode_sense - Simulate MODE SENSE 6, 10 commands
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Simulate MODE SENSE commands. Assume this is invoked for direct
 *	access devices (e.g. disks) only. There should be no block
 *	descriptor for other device types.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_mode_sense( u8 *rbuf, unsigned int buflen)
{
	u8 *scsicmd = psscsicmd;
	u8 *p, *last;
	u8 sat_blk_desc[8];
	
	u8 pg, spg;
	unsigned int ebd, page_control, six_byte, output_len, alloc_len, minlen;

	rbuf[0] = 0;
	rbuf[1] = 0;
	rbuf[2] = 0;
	//rbuf[3] = 8;
	//debug_Aaron
	rbuf[3] = 0;
	rbuf[4] = 0;
	rbuf[5] = 0;
	rbuf[6] = 0;
	rbuf[7] = 0;
	rbuf[8] = 0;
	rbuf[9] = 0;
	rbuf[10] = 2;
	rbuf[11] = 0;

      {
                unsigned long n_sectors, tmp;
                if (ebd)
                        rbuf[3] = 0x08;
                else
                        rbuf[3] = 0;
                if (ata_id_has_lba(dev_id))
                {
                        if (ata_id_has_lba48(dev_id))
                                n_sectors = ata_id_u64(dev_id, 100);
                        else
                                n_sectors = ata_id_u32(dev_id, 60);
                }
                else
                {
                        /* CHS default translation */
                        n_sectors = dev_id[1] * dev_id[3] * dev_id[6];

                        if (ata_id_current_chs_valid(dev_id))
                                /* CHS current translation */
                                n_sectors = ata_id_u32(dev_id, 57);
                }

                n_sectors--;            /* ATA TotalUserSectors - 1 */

                if( n_sectors >= 0xffffffffULL )
                        tmp = 0xffffffff ;  /* Return max count on overflow */
                else
                        tmp = n_sectors ;
		 /* sector count, 32-bit */
        	rbuf[4] = tmp >> (8 * 3);
        	rbuf[5] = tmp >> (8 * 2);
        	rbuf[6] = tmp >> (8 * 1);
        	rbuf[7] = tmp;

//debug_Aaron
#ifdef DEBUG_SCSI
	printf("%s: rbuf[4]=0x%x, rbuf[[5]=0x%x, rbuf[6]=0x%x, rbuf[7]=0x%x\r\n", __func__, rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
#endif
		tmp = 512;
		rbuf[8] = tmp >> (8 * 3);
                rbuf[9] = tmp >> (8 * 2);
                rbuf[10] = tmp >> (8 * 1);
                rbuf[11] = tmp;
//debug_Aaron
#ifdef DEBUG_SCSI
	printf("%s: rbuf[8]=0x%x, rbuf[[9]=0x%x, rbuf[10]=0x%x, rbuf[11]=0x%x\r\n", __func__, rbuf[8], rbuf[9], rbuf[10], rbuf[11]);
#endif
	}
	VPRINTK("ENTER\n");

	six_byte = (scsicmd[0] == MODE_SENSE);
	ebd = !(scsicmd[1] & 0x8);      /* dbd bit inverted == edb */
	/*
	 * LLBA bit in msense(10) ignored (compliant)
	 */

	page_control = scsicmd[2] >> 6;

#ifdef DEBUG_SCSI
	printf("%s: psscsicmd[2] %x psscsicmd[3] %x page_control %x\n", __func__, psscsicmd[2],psscsicmd[3],page_control);
#endif
	switch (page_control) {
	case 0: /* current */
		break;  /* supported */
	case 3: /* saved */
		goto saving_not_supp;
	case 1: /* changeable */
	case 2: /* defaults */
	default:
		goto invalid_fld;
	}

	if (six_byte) {
		output_len = 4 + (ebd ? 8 : 0);
	//	output_len = (ebd ? 8 : 0);
		alloc_len = scsicmd[4];
	} else {
		output_len = 8 + (ebd ? 8 : 0);
		alloc_len = (scsicmd[7] << 8) + scsicmd[8];
	}
	minlen = (alloc_len < buflen) ? alloc_len : buflen;

	p = rbuf + output_len;
	last = rbuf + minlen - 1;

	pg = scsicmd[2] & 0x3f;
	spg = scsicmd[3];
	/*
	 * No mode subpages supported (yet) but asking for _all_
	 * subpages may be valid
	 */
	
	if (spg && (spg != ALL_SUB_MPAGES))
		goto invalid_fld;

#ifdef DEBUG_SCSI
        printf("%s: pg %x \n", __func__, pg);
#endif
	switch(pg) {
//debug_Aaron on 01/02/2007, donot need to return anything
#if 0
	case 0:
//		break;
	case RW_RECOVERY_MPAGE:
//		output_len += ata_msense_rw_recovery(&p, last);
//		break;

	case CACHE_MPAGE:
//		output_len += ata_msense_caching(dev_id, &p, last);
//		break;

	case CONTROL_MPAGE:
//		output_len += ata_msense_ctl_mode(&p, last);
//		break;
		
	case ALL_MPAGES:
//		output_len += ata_msense_rw_recovery(&p, last);
//		output_len += ata_msense_caching(dev_id, &p, last);
//		output_len += ata_msense_ctl_mode(&p, last);
//		output_len += ata_msense_iec(&p, last);
/*Stone debug IDE */
//        output_len = 0x0f;
		break;

	case 0x1c:
//		output_len += ata_msense_iec(&p, last);
//		break;
#endif

	default:		/* invalid page code */
	        printf("default pg %x \n",pg);
		goto invalid_fld;
	}

	rbuf[0] = output_len - 1;
	rbuf[1] = 0;
	rbuf[2] = 0;
	if (ebd)
        	rbuf[3] = 0x08;
        else
        	rbuf[3] = 0;	
#ifdef DEBUG_SCSI
	printf("%s: output_len=%d, ebd=%d\r\n", __func__, output_len, ebd);
#endif
//	tOTGCSW.u32DataResidue = output_len;
	return 0;

invalid_fld:
        printf("ata_scsiop_mode_sense invalid_fld tOTGCSW.u32DataResidue %x\n",tOTGCSW.u32DataResidue); 
//        MassStorageState eState;
//        tOTGCSW.u8Status = CSW_STATUS_CMD_FAIL;
	ata_scsi_set_sense(scsicmd, ILLEGAL_REQUEST, 0x20, 0x0);
	/* "Invalid field in cbd" */
	return 1;

saving_not_supp:
        printf("ata_scsiop_mode_sense saving_not_supp\n"); 
	ata_scsi_set_sense(scsicmd, ILLEGAL_REQUEST, 0x39, 0x0);
	 /* "Saving parameters not supported" */
	return 1;
}


/**
 *	ata_scsiop_read_cap - Simulate READ CAPACITY[ 16] commands
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Simulate READ CAPACITY commands.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_read_cap( u8 *rbuf, unsigned int buflen)
{
	u64 n_sectors;
	u32 tmp;

	VPRINTK("ENTER\n");

	if (ata_id_has_lba(dev_id)) {
//debug_Aaron
#ifdef DEBUG_SCSI
printf("%s: has LBA\r\n", __func__);
#endif
		if (ata_id_has_lba48(dev_id))
{
			n_sectors = ata_id_u64(dev_id, 100);
	  //debug_Aaron
#ifdef DEBUG_SCSI
printf("%s: has LBA48, n_sectors=%ld\r\n", __func__, n_sectors);
#endif
}
		else
			n_sectors = ata_id_u32(dev_id, 60);
	} else {
		/* CHS default translation */
		n_sectors = dev_id[1] * dev_id[3] * dev_id[6];

//debug_Aaron
#ifdef DEBUG_SCSI
printf("%s: CHS, C=dev_id[1]=%d, dev_id[3]=%d, dev_id[6]=%d\r\n", __func__,
            dev_id[1], dev_id[3], dev_id[6]);
#endif
		if (ata_id_current_chs_valid(dev_id))
{
	//debug_Aaron
#ifdef DEBUG_SCSI
printf("%s: has CHS translation\r\n", __func__);
#endif
			/* CHS current translation */
			n_sectors = ata_id_u32(dev_id, 57);
}
	}

	n_sectors--;		/* ATA TotalUserSectors - 1 */

  //debug_Aaron
#ifdef DEBUG_SCSI
printf("%s: n_sectors=%ld\r\n", __func__, n_sectors);
#endif

	if( n_sectors >= 0xffffffffULL )
		tmp = 0xffffffff ;  /* Return max count on overflow */
	else
		tmp = n_sectors ;

	/* sector count, 32-bit */
	rbuf[0] = tmp >> (8 * 3);
	rbuf[1] = tmp >> (8 * 2);
	rbuf[2] = tmp >> (8 * 1);
	rbuf[3] = tmp;

	/* sector size */
	tmp = ATA_SECT_SIZE;
	rbuf[6] = tmp >> 8;
	rbuf[7] = tmp;

#ifdef DEBUG_SCSI
        printf("%s: rbuf[0]=0x%x, rbuf[1]=0x%x, rbuf[2]=0x%x, rbuf[3]=0x%x\r\n", __func__, rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
#endif 
	return 0;
}


/**
 *	ata_scsiop_read_cap_16 - Simulate READ CAPACITY[ 16] commands
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Simulate READ CAPACITY commands.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_read_cap_16( u8 *rbuf, unsigned int buflen)
{
	u64 n_sectors;
	u32 tmp;

	VPRINTK("ENTER\n");

	if (ata_id_has_lba(dev_id)) {
		if (ata_id_has_lba48(dev_id))
			n_sectors = ata_id_u64(dev_id, 100);
		else
			n_sectors = ata_id_u32(dev_id, 60);
	} else {
		/* CHS default translation */
		n_sectors = dev_id[1] * dev_id[3] * dev_id[6];

		if (ata_id_current_chs_valid(dev_id))
			/* CHS current translation */
			n_sectors = ata_id_u32(dev_id, 57);
	}

	n_sectors--;		/* ATA TotalUserSectors - 1 */

	/* sector count, 64-bit */
	tmp = n_sectors >> (8 * 4);
	rbuf[2] = tmp >> (8 * 3);
	rbuf[3] = tmp >> (8 * 2);
	rbuf[4] = tmp >> (8 * 1);
	rbuf[5] = tmp;
	tmp = n_sectors;
	rbuf[6] = tmp >> (8 * 3);
	rbuf[7] = tmp >> (8 * 2);
	rbuf[8] = tmp >> (8 * 1);
	rbuf[9] = tmp;

	/* sector size */
	tmp = ATA_SECT_SIZE;
	rbuf[12] = tmp >> 8;
	rbuf[13] = tmp;

#ifdef DEBUG_SCSI
        printf("%s: n_sectors=%ld\r\n", __func__, n_sectors);
#endif 
	return 0;
}


/**
 *	ata_scsiop_report_luns - Simulate REPORT LUNS command
 *	@args: device IDENTIFY data / SCSI command of interest.
 *	@rbuf: Response buffer, to which simulated SCSI cmd output is sent.
 *	@buflen: Response buffer length.
 *
 *	Simulate REPORT LUNS command.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

unsigned int ata_scsiop_report_luns( u8 *rbuf, unsigned int buflen)
{
	VPRINTK("ENTER\n");
	rbuf[3] = 8;	/* just one lun, LUN 0, size 8 bytes */

	return 0;
}




static inline ata_xlat_func_t ata_get_xlat_func(u8 cmd)
{
	switch (cmd) {
	case READ_6:
	case READ_10:
	case READ_16:

	case WRITE_6:
	case WRITE_10:
	case WRITE_16:
		return ata_scsi_rw_xlat;

	case VERIFY:
	case VERIFY_16:
		return ata_scsi_verify_xlat;

	case START_STOP:
		return ata_scsi_start_stop_xlat;
	}

	return NULL;
}


/**
 *	ata_scsi_simulate - simulate SCSI command on ATA device
 *	@id: current IDENTIFY data for target device.
 *	@cmd: SCSI command being sent to device.
 *	@done: SCSI command completion function.
 *
 *	Interprets and directly executes a select list of SCSI commands
 *	that can be handled internally.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

void ata_scsi_simulate(u8 *cmd)
{
        int i;
	u8 *scsicmd = cmd;
	psscsicmd = cmd;
	u32 len=tOTGCBW.u32DataTransferLength;
	for (i=0;i<len;i++)
              {
              	u8VirtOTGMemory[i]=0x0;
                }

#ifdef DEBUG_SCSI
        printf("%s: scsicmd[0]=0x%d\r\n", __func__, scsicmd[0]);
#endif 
	switch(scsicmd[0]) {
		/* no-op's, complete with success */
		case SYNCHRONIZE_CACHE:
		case REZERO_UNIT:
		case SEEK_6:
		case SEEK_10:
		case TEST_UNIT_READY:
		case FORMAT_UNIT:		/* FIXME: correct? */
		case SEND_DIAGNOSTIC:		/* FIXME: correct? */
			//ata_scsi_rbuf_fill(len, ata_scsiop_noop);
//			printf("CMD not support, ignore\n");
			break;

		
		case INQUIRY:
			if (scsicmd[1] & 2)	           /* is CmdDt set?  */
			       {
//			       	printf("ata_scsi_simulate INQUIRY 1\n");
				ata_scsi_invalid_field(cmd);
			        } 
			else if ((scsicmd[1] & 1) == 0)    /* is EVPD clear? */
				ata_scsi_rbuf_fill(len, ata_scsiop_inq_std);
			else if (scsicmd[2] == 0x00)
				ata_scsi_rbuf_fill(len, ata_scsiop_inq_00);
			else if (scsicmd[2] == 0x80)
				ata_scsi_rbuf_fill(len, ata_scsiop_inq_80);
			else if (scsicmd[2] == 0x83)
				ata_scsi_rbuf_fill(len, ata_scsiop_inq_83);
			else   
			       {
//			       	printf("ata_scsi_simulate INQUIRY 2\n");  
				ata_scsi_invalid_field(cmd);
			         }
			break;

		case MODE_SENSE:
		case MODE_SENSE_10:
//		        printf("scsicmd[1]%x scsicmd[2]%x scsicmd[3]%x\n",scsicmd[1],scsicmd[2],scsicmd[3]);
//		        printf("psscsicmd[1]%x psscsicmd[2]%x psscsicmd[3]%x\n",psscsicmd[1],psscsicmd[2],psscsicmd[3]);
			ata_scsi_rbuf_fill(len, ata_scsiop_mode_sense);
			break;

		case MODE_SELECT:	/* unconditionally return */
		case MODE_SELECT_10:	/* bad-field-in-cdb */
//		        printf("ata_scsi_simulate MODE_SELECT_10 \n");
			ata_scsi_invalid_field(cmd);
			break;

		case READ_CAPACITY:
			ata_scsi_rbuf_fill(len, ata_scsiop_read_cap);
			break;

		case SERVICE_ACTION_IN:
			if ((scsicmd[1] & 0x1f) == SAI_READ_CAPACITY_16)
				ata_scsi_rbuf_fill(len, ata_scsiop_read_cap_16);
			else
			        {
//			        printf("ata_scsi_simulate SERVICE_ACTION_IN \n");	
				ata_scsi_invalid_field(cmd);
			         }
			break;

		case REPORT_LUNS:
			ata_scsi_rbuf_fill(len, ata_scsiop_report_luns);
			break;

//		case READ_FORMAT_CAPACITY: // read format capacity
//		        len= 12;
//		        tOTGCSW.u32DataResidue = 12;
//			ata_scsi_rbuf_fill(len, do_read_format_capacities);
//                        printf("READ_FORMAT_CAPACITY tOTGCSW.u32DataResidue %x\n",tOTGCSW.u32DataResidue);
//			break;
		/* mandatory commands we haven't implemented yet */
		case REQUEST_SENSE:
                                   ata_scsi_set_sense(cmd, ILLEGAL_REQUEST, 0x20, 0x0);
                                   break;
		/* all other commands */
		case READ_FORMAT_CAPACITY: // read format capacity
		default:
//		        printf("ata_scsi_simulate default \n");
//		        printf("scsicmd[0] %x \n",scsicmd[0]); 
//		        MassStorageState eState;
//                        tOTGCSW.u8Status = CSW_STATUS_CMD_FAIL; 
            memset(u8VirtOTGMemory, 0x00, MAX_BUFFER_SIZE);	
            
//			ata_scsi_set_sense(cmd, ILLEGAL_REQUEST, 0x20, 0x0);
			/* "Invalid command operation code" */
			break;
	}

}


/**
 *	ata_scsi_translate - Translate then issue SCSI command to ATA device
 *	@ap: ATA port to which the command is addressed
 *	@dev: ATA device to which the command is addressed
 *	@cmd: SCSI command to execute
 *	@done: SCSI command completion function
 *	@xlat_func: Actor which translates @cmd to an ATA taskfile
 *
 *	Our ->queuecommand() function has decided that the SCSI
 *	command issued can be directly translated into an ATA
 *	command, rather than handled internally.
 *
 *	This function sets up an ata_queued_cmd structure for the
 *	SCSI command, and sends that ata_queued_cmd to the hardware.
 *
 *	The xlat_func argument (actor) returns 0 if ready to execute
 *	ATA command, else 1 to finish translation. If 1 is returned
 *	then cmd->result (and possibly cmd->sense_buffer) are assumed
 *	to be set reflecting an error condition or clean (early)
 *	termination.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host_set lock)
 */

void ata_scsi_translate(u8 *cmd, ata_xlat_func_t xlat_func)
{

	u8 *scsicmd = cmd;

	VPRINTK("ENTER\n");

	if (xlat_func(cmd))
		goto early_finish;
	VPRINTK("EXIT\n");
	return;

early_finish:
 
	DPRINTK("EXIT - early finish (good or error)\n");
	return;

}

int scsi_ata_translate(u8 *cmd)
{

	ata_xlat_func_t xlat_func = ata_get_xlat_func(cmd[0]);

	if (xlat_func)
		ata_scsi_translate(cmd, xlat_func);

	else
		ata_scsi_simulate(cmd);

}

#endif //#ifdef USB_DEV_MO
