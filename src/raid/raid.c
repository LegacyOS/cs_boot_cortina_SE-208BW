/****************************************************************************
* Copyright  Stormsemi Corp 2007.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: raid.c
* Description	: 
*		Handle raid base functions
** History**	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	01/10/2007	Jason Lee	Create
*	25/10/2007	Jason Lee	Support RAID1 & JBOD
*****************************************************************************/

#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include "../ide/ide.h"
#include "md_p.h"

extern IDE_PART_T	ide_partitions[IDE_MAX_PARTITIONS];
extern int		ide_part_num;

char STORM_RAID_MODEL_NAME[]="CortinaDAS-";
char STORM_LINEAR_MODEL_NAME[]="Cortina_LINEAR";
char STORM_RAID1_MODEL_NAME[] ="Cortina_RAID1 ";
char STORM_FW_REV[] = "0.90B";

extern unsigned int	async_io;
unsigned char	sb_update=0;
int mbr_wc=0;

unsigned char	raid_dev_no=0;
md_dev_info_t  md[MAX_MD_DEV];
unsigned char	md_mbr[IDE_SECTOR_SIZE];
IDE_MBR_T *md_partition = (IDE_MBR_T *)((char *)md_mbr + MBR_PTABLE_OFFSET);
#define SECTOR_OFFSET	1
extern u16 dev_id[512];
extern u64 dev_sectors;
extern IDE_DISK_T	*first_disk;
extern IDE_DISK_T	*second_disk;

#ifdef BOARD_SUPPORT_RAID

int linear_read(md_dev_info_t *md,UINT64 start, UINT32 n_byte, UINT8 *buf);
int linear_write(md_dev_info_t *md,UINT64 start, UINT32 n_byte, UINT8 *buf);
int raid1_read(md_dev_info_t *md,UINT64 start, UINT32 n_byte, UINT8 *buf);
int raid1_write(md_dev_info_t *md,UINT64 start, UINT32 n_byte, UINT8 *buf);

int linear_read(md_dev_info_t *mddev,UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	IDE_PART_T *dev1,*dev2;
	mdp_super_t *sb,*refsb;
	u32	c1,c2;
	
	dev1 = mddev->disk[0];
	dev2 = mddev->disk[1];
	async_io = 0;
	sb = (mdp_super_t*)dev1->sb_page;
	if(((start>>1)+(n_byte>>10)) <= sb->rdev_size ){	// first 
		mddev->disk[0]->read(mddev->disk[0],start,n_byte,buf);
	}
	else if((start>>1)>=sb->rdev_size){			// sencond
		start -= sb->rdev_size<<1;
		mddev->disk[1]->read(mddev->disk[1],start,n_byte,buf);
	}
	else{							// cross-two disk
		c1 = (sb->rdev_size<<1) - start;
		c1 *= IDE_SECTOR_SIZE;
		mddev->disk[0]->read(mddev->disk[0],start,c1,buf);
		printf("boundaryR:[S:%d] [C:%d]\n",start,n_byte);
		c2 = n_byte - c1;
		mddev->disk[1]->read(mddev->disk[1],0,c2,&buf[c1]);
	}
}

int linear_write(md_dev_info_t *mddev,UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	IDE_PART_T *dev1,*dev2;
	mdp_super_t *sb,*refsb;
	u32	c1,c2;
	
	dev1 = mddev->disk[0];
	dev2 = mddev->disk[1];
	async_io = 0;
	sb = (mdp_super_t*)dev1->sb_page;
	if(((start>>1)+(n_byte>>10)) <= sb->rdev_size ){	// first 
		mddev->disk[0]->write(mddev->disk[0],start,n_byte,buf);
	}
	else if((start>>1)>=sb->rdev_size){			// sencond
		start -= sb->rdev_size<<1;
		mddev->disk[1]->write(mddev->disk[1],start,n_byte,buf);
	}
	else{							// cross-two disk
		c1 = (sb->rdev_size<<1) - start;
		c1 *= IDE_SECTOR_SIZE;
		mddev->disk[0]->write(mddev->disk[0],start,c1,buf);
		printf("boundaryW:[S:%d] [C:%d]\n",start,n_byte);
		c2 = n_byte - c1;
		mddev->disk[1]->write(mddev->disk[1],0,c2,&buf[c1]);
	}
}

int sb_90_load(IDE_PART_T *part,u8 *buf)
{
	u64	sb_offset;
	
	sb_offset = MD_NEW_SIZE_SECTORS(part->sector_num);
	part->sb_offset = sb_offset;
	(*part->read)(part,sb_offset,MD_SB_BYTES,buf);
	
}

int sb_90_store(IDE_PART_T *part,u8 *buf)
{

	(*part->write)(part,part->sb_offset,MD_SB_BYTES,buf);
	return 1;
}

int raid1_read(md_dev_info_t *mddev,UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	
	
	
/*	unsigned int count;
	if(mddev->disk_no==2){
		if(n_byte>=(SW_PIPE_SIZE*4)){	//read Parallel (gain nothing, usb's bottleneck?)
			count = n_byte/2;
			count = count&(~(SW_PIPE_SIZE-1));
			mddev->disk[0]->read(mddev->disk[0],start,count,buf);
			start += count/IDE_SECTOR_SIZE;
			mddev->disk[1]->read(mddev->disk[1],start,n_byte-count,&buf[count]);
		}
		else
			mddev->disk[0]->read(mddev->disk[0],start,n_byte,buf);
	}
	else{
		// use first device always
		mddev->disk[0]->read(mddev->disk[0],start,n_byte,buf);
	}
*/
	
	// use first device always
	mddev->disk[0]->read(mddev->disk[0],start,n_byte,buf);
	return 1;
	
}

int raid1_write(md_dev_info_t *mddev,UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	int i=0;
	IDE_PART_T *ide_part;
	mdp_super_t *sb;
	u32 add_ptr;
	unsigned int csum;
	
	async_io = 0;
	// If RAID1 just find one partition, we must update raid super block 
	// and kernel will resync data to another parner at assemble time.
	if(!sb_update){
		if(mddev->disk_no==1){
			//sb = &(((IDE_PART_T *)(mddev->disk[0]))->sb_page);
			ide_part = mddev->disk[0];
			sb = &ide_part->sb_page[0];
			sb->events_lo += 2;
			sb->cp_events_lo += 2;
			sb->sb_csum = 0;
			//check sum
			csum = csum_partial((void *)sb, MD_SB_BYTES, 0);
			sb->sb_csum = csum;
			printf("csum %x \n",csum);
			//sb_90_store(ide_part,sb);
			mddev->disk[0]->write(ide_part,ide_part->sb_offset,MD_SB_BYTES,sb);
		}
		sb_update = 1;
	}
	
	
	for(i=0;i<mddev->disk_no;i++){
		mddev->disk[i]->write(mddev->disk[i],start,n_byte,buf);
	}
	return 1;
}

int uuid_equal(mdp_super_t *sb1, mdp_super_t *sb2)
{
	if ((sb1->set_uuid0 == sb2->set_uuid0) &&
		(sb1->set_uuid1 == sb2->set_uuid1) &&
		(sb1->set_uuid2 == sb2->set_uuid2) &&
		(sb1->set_uuid3 == sb2->set_uuid3))
	return 1;

	return 0;
}

int sb_equal(mdp_super_t *sb1, mdp_super_t *sb2)
{
	int ret;
	int temp1,temp2;
	
	/*
	 * nr_disks is not constant
	 */
	temp1 = sb1->nr_disks;
	temp2 = sb2->nr_disks;
	
	sb1->nr_disks = 0;
	sb2->nr_disks = 0;

	if (memcmp(sb1, sb2, MD_SB_GENERIC_CONSTANT_WORDS * 4))
		ret = 0;
	else
		ret = 1;

	sb1->nr_disks = temp1;
	sb2->nr_disks = temp2;
	return ret;
}

static int pers_to_level (int pers)
{
	switch (pers) {
		case FAULTY:		return LEVEL_FAULTY;
		case MULTIPATH:		return LEVEL_MULTIPATH;
		case HSM:		return -3;
		case TRANSLUCENT:	return -2;
		case LINEAR:		return LEVEL_LINEAR;
		case RAID0:		return 0;
		case RAID1:		return 1;
		case RAID5:		return 5;
		case RAID6:		return 6;
		case RAID10:		return 10;
	}
	BUG();
	return MD_RESERVED;
}

static int level_to_pers (int level)
{
	switch (level) {
		case LEVEL_FAULTY: return FAULTY;
		case LEVEL_MULTIPATH: return MULTIPATH;
		case -3: return HSM;
		case -2: return TRANSLUCENT;
		case LEVEL_LINEAR: return LINEAR;
		case 0: return RAID0;
		case 1: return RAID1;
		case 4:
		case 5: return RAID5;
		case 6: return RAID6;
		case 10: return RAID10;
	}
	return MD_RESERVED;
}

static sector_t calc_dev_size(IDE_PART_T *part, unsigned chunk_size)
{
	sector_t size;

	size = part->sb_offset>>2;	// kb

	if (chunk_size)
		size &= ~((sector_t)chunk_size/1024 - 1);
	return size;
}

int md_import_device(md_dev_info_t *md,IDE_PART_T *dev1,IDE_PART_T *dev2)
{
	mdp_super_t *sb,*refsb;
	u64	ev1,ev2;
	
	md->flag = 0;
	md->status = MD_DEV_FAIL;
	sb = (mdp_super_t*)dev1->sb_page;
	
	if(dev2==NULL){
		md->disk[0] = dev1;
		dev1->rdev_size = sb->rdev_size;
		dev1->status = 1;
		md->array_size = sb->size+1;
		md->disk_no = 1;
		md->level = level_to_pers(sb->level);
	}
	else{
			
		refsb = (mdp_super_t*)dev2->sb_page;
		
		if(sb->this_disk.number < refsb->this_disk.number){	// dev1 first then dev2
			md->disk[0] = dev1;
			md->disk[1] = dev2;
		}
		else{
		md->disk[0] = dev2;
		md->disk[1] = dev1;
		}
		
		dev1->rdev_size = sb->rdev_size;
		dev2->rdev_size = refsb->rdev_size;
		dev1->status = 1;
		dev2->status = 1;
		md->array_size = sb->size;
		md->disk_no = 2;
		md->level = level_to_pers(sb->level);
		
		ev1 = md_event(sb);
		ev2 = md_event(refsb);
		// find freshest
		if(ev1>ev2){
			if(ev2++ < ev1){	// ev2 too old
				md->disk_no--;
				md->disk[1] = NULL;
				printf("kick out non-freshed dev2!\n");
			}
		}
		else{
			if(ev1++ < ev2){	// ev1 too old
				md->disk_no--;
				md->disk[0] = dev2;
				md->disk[1] = NULL;
				printf("kick out non-freshed dev1!\n");
			}
		}
	
	}
	if((md->disk_no >=1)&&(md->level==RAID1))
		md->status = MD_DEV_WORKABLE;
	if((md->disk_no >=2)&&(md->level==LINEAR))
		md->status = MD_DEV_WORKABLE;
	
	return 0;
}

int md_read(IDE_DISK_T *disk, UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	
	if(start ==0){
		memcpy(buf,&md_mbr,IDE_SECTOR_SIZE);
		return 0;
	}
	md[disk->raid_idx].read_dma(&md[disk->raid_idx],start-SECTOR_OFFSET,n_byte,buf);
	return 1;
}

int md_write(IDE_DISK_T *disk, UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	if(start ==0)	{		// do not permit to update mbr sector
		memcpy(&md_mbr,buf,IDE_SECTOR_SIZE);
		mbr_wc++;
		printf("MBR write %d %d\n",mbr_wc,n_byte);
		return 0;
	}
	
	md[disk->raid_idx].write_dma(&md[disk->raid_idx],start-SECTOR_OFFSET,n_byte,buf);
	return 1;
}

int overwrite_ident_buff(md_dev_info_t *md)
{
	unsigned char *str,i=0,j=0;
	char c_buf[20];
	int len;
	u64	block_no = md->array_size*2;
	
	len = strlen(STORM_RAID_MODEL_NAME);
	str = dev_id[ATA_ID_PROD_OFS];
	
	// Model number
	memset(&dev_id[ATA_ID_PROD_OFS],0x20,40);
	while(len>0){
		if(len!=1)
			dev_id[ATA_ID_PROD_OFS+(i/2)]= (STORM_RAID_MODEL_NAME[i]<<8) | STORM_RAID_MODEL_NAME[i+1];
		else
			dev_id[ATA_ID_PROD_OFS+(i/2)]= (STORM_RAID_MODEL_NAME[i]<<8) | '-';
		i+=2;
		len-=2; 
	}
	sprintf(c_buf,"%04d",block_no>>(1+20));	// GB
	len = strlen(c_buf);
	while(len>0){
		if(len!=1)
			dev_id[ATA_ID_PROD_OFS+(i/2)]= (c_buf[j]<<8) | c_buf[j+1];
		else
			dev_id[ATA_ID_PROD_OFS+(i/2)]= (c_buf[j]<<8) | 0x20;
		
		len -=2;
		j+=2;
		i+=2;
	}

	// Firmware revision field
	i=0;
	len = strlen(STORM_FW_REV);
	str = dev_id[ATA_ID_FW_REV_OFS];
	memset(&dev_id[ATA_ID_FW_REV_OFS],0x20,8);
	while(len>0){
		if(len!=1)
			dev_id[ATA_ID_FW_REV_OFS+(i/2)]= (STORM_FW_REV[i]<<8) | STORM_FW_REV[i+1];
		else
			dev_id[ATA_ID_FW_REV_OFS+(i/2)]= (STORM_FW_REV[i]<<8) | 0x20;
		i+=2;
		len-=2; 
	}
	
	// Serial number field
	i=0;
	len = strlen(STORM_LINEAR_MODEL_NAME);
	str = dev_id[ATA_ID_SERNO_OFS];
	memset(&dev_id[ATA_ID_SERNO_OFS],0x20,20);
	while(len>0){
		if(md->level==LINEAR){
			if(len!=1)
				dev_id[ATA_ID_SERNO_OFS+(i/2)]= (STORM_LINEAR_MODEL_NAME[i]<<8) | STORM_LINEAR_MODEL_NAME[i+1];
			else
				dev_id[ATA_ID_SERNO_OFS+(i/2)]= (STORM_LINEAR_MODEL_NAME[i]<<8) | 0x20;
		}
		else if(md->level==RAID1){
			if(len!=1)
				dev_id[ATA_ID_SERNO_OFS+(i/2)]= (STORM_RAID1_MODEL_NAME[i]<<8) | STORM_RAID1_MODEL_NAME[i+1];
			else
				dev_id[ATA_ID_SERNO_OFS+(i/2)]= (STORM_RAID1_MODEL_NAME[i]<<8) | 0x20;
		}
		i+=2;
		len-=2; 
	}
	
	dev_id[100] = block_no+SECTOR_OFFSET+2;
	dev_id[101] = (block_no+SECTOR_OFFSET+2)>>16;
	dev_id[102] = (block_no+SECTOR_OFFSET+2)>>32;
	dev_id[103] = (block_no+SECTOR_OFFSET+2)>>48;
	dev_sectors = block_no+SECTOR_OFFSET+2 ;
	
}

int raid_init(void)
{
	int i=0,j=0;
	mdp_super_t *sb,*refsb;
	u8 buf0[MD_SB_BYTES],buf1[MD_SB_BYTES];
		
	printf("Scan RAID partition and assemble\n");		
	for(i=0;i<ide_part_num;i++){
		if(ide_partitions[i].os == IDE_PART_LINUX_RAID){
			if(ide_partitions[i].status)
				continue;
			sb_90_load(&ide_partitions[i],&ide_partitions[i].sb_page);
			sb = (mdp_super_t*)&ide_partitions[i].sb_page;
			
			if(sb->md_magic != MD_SB_MAGIC){
				printf("RAID partition without raid sb[C:%d D:%d P:%d]\n",ide_partitions[i].disk->ide->ide_id,ide_partitions[i].disk->drive_id,ide_partitions[i].part_id);
				continue;
			}
			if(sb->raid_disks <= 0)
				continue;
			
			if(sb->this_disk.state & ((1<<MD_DISK_REMOVED)|(1<<MD_DISK_FAULTY)))
				continue;	
				
			for(j=i+1;j<ide_part_num;j++){
				if(ide_partitions[j].os == IDE_PART_LINUX_RAID){
					if(ide_partitions[j].status)
						continue;
					sb_90_load(&ide_partitions[j],&ide_partitions[j].sb_page);
					refsb = (mdp_super_t*)&ide_partitions[j].sb_page;
					if(refsb->md_magic != MD_SB_MAGIC){
						printf("RAID partition without raid sb[C:%d D:%d P:%d]\n",ide_partitions[j].disk->ide->ide_id,ide_partitions[j].disk->drive_id,ide_partitions[j].part_id);
						continue;
					}
					if(refsb->raid_disks <= 0)
						continue;
				
					if(refsb->this_disk.state & ((1<<MD_DISK_REMOVED)|(1<<MD_DISK_FAULTY)))
						continue;	
				
					if(!uuid_equal(sb,refsb)){
						printf("different uuid\n");
						continue;
					}
					
					if(!sb_equal(sb,refsb)){
						printf("md: has same UUID"
			       				" but different superblock to \n");
			       			continue;
			       		}
			       		
			       		/* ok, we can import devices to raid */
			       		printf("add [C:%d D:%d P:%d] and [C:%d D:%d P:%d] to md%d\n", \
			       			ide_partitions[i].disk->ide->ide_id,ide_partitions[i].disk->drive_id,ide_partitions[i].part_id, \
			       			ide_partitions[j].disk->ide->ide_id,ide_partitions[j].disk->drive_id,ide_partitions[j].part_id,raid_dev_no);
			       		md_import_device(&md[raid_dev_no],&ide_partitions[i],&ide_partitions[j]);
			       		raid_dev_no++;
				}
			}
			
			if(ide_partitions[i].status==0 && (ide_partitions[i].sector_num > 0x200000)){		// lack parner
				if(level_to_pers(sb->level)==RAID1){	// if RAID1, run alone
					printf("add [C:%d D:%d P:%d] to md%d\n", \
			       			ide_partitions[i].disk->ide->ide_id,ide_partitions[i].disk->drive_id,ide_partitions[i].part_id,raid_dev_no);
			       		md_import_device(&md[raid_dev_no],&ide_partitions[i],NULL);
			       		raid_dev_no++;
				}
			}
		}
	}
	
	memset(&md_mbr,0,IDE_SECTOR_SIZE);	// create a virtual mbr sector
	md_mbr[510] = 0x55;			// MBR signature 
	md_mbr[511] = 0xAA;
	
	/* decide system or data volume */
	for(i=0;i<raid_dev_no;i++){
		if(md[i].array_size < (0x100000))	// less than 1GB, assume system
			md[i].flag = SYSTEM_DEV;
		else{					// or data volume
			md[i].flag = DATA_DEV;
			md[i].virt_disk.read_dma = md_read;
			md[i].virt_disk.write_dma = md_write;
			//md[i].raid_idx = i;
			//first_disk = &md[i].virt_disk;
			first_disk->raid_idx = i;
			first_disk->read_dma = md_read;
			first_disk->write_dma = md_write;
			if(md[i].level==LINEAR){
				md[i].read_dma = linear_read;
				md[i].write_dma = linear_write;
			}
			else if(md[i].level==RAID1){
				md[i].read_dma = raid1_read;
				md[i].write_dma = raid1_write;
			}
			
			md_partition->boot_ind = 0x0;
			md_partition->start_head = 0x1;
			md_partition->start_sector = 0;
			md_partition->start_cyl = 0;
			md_partition->sys_ind = IDE_PART_NTFS;		// Windows can auto mount?
			md_partition->end_head = 0xFF;
			md_partition->end_sector = 0xFF;
			md_partition->end_cyl = 0xFF;
			md_partition->start_sect[0] = SECTOR_OFFSET;
			md_partition->start_sect[1] = 0x00;
			md_partition->start_sect[2] = 0x00;
			md_partition->start_sect[3] = 0x00;
			md_partition->sector_num[3] = (md[i].array_size<<1)>>24;
			md_partition->sector_num[2] = (md[i].array_size<<1)>>16;
			md_partition->sector_num[1] = (md[i].array_size<<1)>>8;
			md_partition->sector_num[0] = (md[i].array_size<<1);
			
			overwrite_ident_buff(&md[i]);
			printf("%s ",(md[i].level==LINEAR)?"JBOD":"RAID1");
			printf("block no:%ld\n",md[i].array_size<<1);
			
		}
	}
	
	return raid_dev_no;
}
#endif
