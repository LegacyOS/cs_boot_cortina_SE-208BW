/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_vctl.c
* Description	: 
*		Handle FLASH VCTL (version Control)
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/20/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include <sys_vctl.h>
#include <sys_fis.h>

#ifdef BOARD_SUPPORT_VCTL

int sys_get_vctl_entry(int type, void *result, int size);
int sys_save_vctl_entry(int type, void *datap, int size);
void sys_get_vctl_mac(char *mac1, char *mac2);
void sys_set_vctl_mac(char *mac1, char *mac2);

static unsigned char vctl_default_mac_addr[2][6] = 
				{BOARD_DEFAULT_MAC0_ADDR, BOARD_DEFAULT_MAC1_ADDR};
static unsigned char *vctl_datap;

/*----------------------------------------------------------------------
* sys_vctl_init
*----------------------------------------------------------------------*/
int sys_vctl_init(void) 
{
	char		entry_buf[128];
	unsigned int vctl_addr=0, vctl_size=0;
		
#ifdef BOARD_SUPPORT_FIS
	FIS_T	*img;

	img = fis_find_image(BOARD_FIS_VCTL_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_VCTL_NAME, img);
		return;
	}
	vctl_addr = img->file.flash_base;
#ifdef BOARD_NAND_BOOT		
	vctl_size = BOARD_FLASH_VCTLIMG_SIZE;
#else
	vctl_size = img->file.size;
#endif	

	
#else
	vctl_addr = VERCTL_ADDR;
#ifdef BOARD_NAND_BOOT		
	vctl_size = BOARD_FLASH_VCTLIMG_SIZE;
#else
	vctl_size = BOARD_FLASH_VCTL_SIZE;
#endif
#endif


	vctl_datap = (char *)malloc(vctl_size);

	
	hal_flash_enable();
	memcpy((void *)vctl_datap, vctl_addr, vctl_size);
	
	// if VCTL VLAN entry is not found, add a default entry
	if (sys_get_vctl_entry(VCT_VLAN, (void *)entry_buf, sizeof(entry_buf)) != 0)
	{
		sys_set_vctl_mac(&vctl_default_mac_addr[0][0], &vctl_default_mac_addr[1][0]);
	}

	hal_flash_disable();
	
	return 0;
}

#define GET_BYTE(addr)		((u8)(addr[0])
#define GET_DWORD(addr)		(u32)((((u8)(addr)[3]) << 24) | (((u8)(addr)[2])  << 16) |	\
								  (((u8)(addr)[1]) << 8)  | (((u8)(addr)[0])) )

/*----------------------------------------------------------------------
* sys_get_vctl_entry
*	input:
*		type	Entry Type
*		result	points to result
*		size	max size to store the result
*	return
*		0		if found
*		-1		not found
*----------------------------------------------------------------------*/
int sys_get_vctl_entry(int type, void *result, int size)
{
	int				i;
	VCTL_HDR_T 		*mhead;
	unsigned char	*entry;
	unsigned int vctl_size=0;
	
#ifdef BOARD_SUPPORT_FIS
	FIS_T	*img;

	img = fis_find_image(BOARD_FIS_VCTL_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_VCTL_NAME, img);
		return;
	}
#ifdef BOARD_NAND_BOOT		
	vctl_size = BOARD_FLASH_VCTLIMG_SIZE;
#else
	vctl_size = img->file.size;
#endif	
	
	
#else
#ifdef BOARD_NAND_BOOT		
	vctl_size = BOARD_FLASH_VCTLIMG_SIZE;
#else
	vctl_size = BOARD_FLASH_VCTL_SIZE;
#endif
#endif
	
	hal_flash_enable();
	
	mhead = (VCTL_HDR_T *)vctl_datap;
	if (memcmp(mhead->header, VCTL_HDR_MAGIC, VCTL_MAGIC_SIZE) != 0 ||
		mhead->entry_num <= 0)
	{
		hal_flash_disable();
		// printf("VCTL entry number is not found!\n");
		return -1;
	}
		
	if (mhead->entry_num > 128)
	{
		printf("VCTL entry number is too large (%d)!\n", mhead->entry_num);
		hal_flash_disable();
		return -1;
	}
	
	entry = (unsigned char *)(mhead + 1) ;
	for (i=0; i<mhead->entry_num; i++)
	{
		int entry_type, entry_size;
		
		entry_size = GET_DWORD(entry + 4);	// entry->size
		entry_type = GET_DWORD(entry + 8);	// entry->type
		if (entry_type == type)
		{
			if (entry_size <= size)
			{
				memcpy((void *)result, (void *)entry, entry_size);
				hal_flash_disable();
				return 0;
			}
			else
			{
				hal_flash_disable();
				return -1;
			}
		}
		entry = (unsigned char *)((unsigned long)entry + entry_size);
		if ((unsigned long)entry > ((unsigned long)vctl_datap + vctl_size))
			break;
	}
	
	hal_flash_disable();
	return -1;
}

/*----------------------------------------------------------------------
* sys_save_vctl_entry
*----------------------------------------------------------------------*/
int sys_save_vctl_entry(int type, void *datap, int size)
{
	int				i, org_total, new_total, total_size;
	char 			*verctl_buf;
	VCTL_HDR_T 		*mhead, *new_hdr;
	unsigned char	*entry;
	int				stat;
	unsigned long	err_addr;
	int				added;
	unsigned int vctl_addr=0, vctl_size=0;
	
#ifdef BOARD_SUPPORT_FIS
	FIS_T	*img;

	img = fis_find_image(BOARD_FIS_VCTL_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_VCTL_NAME, img);
		return;
	}
	vctl_addr = img->file.flash_base;
#ifdef BOARD_NAND_BOOT		
	vctl_size = BOARD_FLASH_VCTLIMG_SIZE;
#else
	vctl_size = img->file.size;
#endif	
	
#else
	vctl_addr = VERCTL_ADDR;
#ifdef BOARD_NAND_BOOT		
	vctl_size = BOARD_FLASH_VCTLIMG_SIZE;
#else
	vctl_size = BOARD_FLASH_VCTL_SIZE;
#endif
#endif
	
	verctl_buf = (char *)malloc(vctl_size);
	new_hdr = (VCTL_HDR_T *)verctl_buf;
	
	if (!verctl_buf)
	{
		dbg_printf(("No free memory!\n"));
		return -1;
	}
	
	hal_flash_enable();

	mhead = (VCTL_HDR_T *)vctl_datap;
	if (memcmp(mhead->header, VCTL_HDR_MAGIC, VCTL_MAGIC_SIZE) != 0)
	{
		org_total = 0;
	}
	else
	{
		org_total = mhead->entry_num;
	}
	
	memcpy(verctl_buf, VCTL_HDR_MAGIC, VCTL_MAGIC_SIZE);
	verctl_buf += sizeof(VCTL_HDR_T);
	total_size = sizeof(VCTL_HDR_T);
	
	new_total = 0;
	added = 0;
	entry = (unsigned char *)(mhead + 1) ;
	for (i=0; i<org_total; i++)
	{
		int entry_type, entry_size;
		
		entry_size = GET_DWORD(entry + 4);	// entry->size
		entry_type = GET_DWORD(entry + 8);	// entry->type
		if (entry_type != type)
		{
			if ((total_size + entry_size) < vctl_size)
			{
				memcpy(verctl_buf, (void *)entry, entry_size);
				verctl_buf += entry_size;
				total_size += entry_size;
				new_total++;
			}
		}
		else if (!added)
		{
			if ((total_size + size) < vctl_size)
			{
				memcpy(verctl_buf, (void *)datap, size);
				verctl_buf += size;
				total_size += size;
				added = 1;
				new_total++;
			}
		}
		entry = (unsigned char *)((unsigned long)entry + entry_size);
		if ((unsigned long)entry > ((unsigned long)vctl_datap + vctl_size))
			break;
	}
	if (!added && (total_size + size) < vctl_size)
	{
		memcpy(verctl_buf, (void *)datap, size);
		verctl_buf += size;
		total_size += size;
		added = 1;
		new_total++;
	}
	
	new_hdr->entry_num = new_total;
	
	hal_flash_disable();
	
	if (!added)
		dbg_printf(("Something wrong to add a vctl entry!\n"));
		
#ifdef BOARD_NAND_BOOT		
	//printf("\nProgram flash (0x%x): Size=%u ", BOARD_FLASH_FIS_ADDR, fis_size);
	if ((stat = flash_nand_program((void *)(vctl_addr), (void *)new_hdr, total_size,  (unsigned long *)&err_addr, img->file.size)) != 0)
	{
		printf(" FAILED at 0x%x: ", err_addr);
		printf((char *)flash_errmsg(stat));
		printf("\n"); 
		return;
	}
	else
	{
		printf(" OK!\n"); 
		return;
	}
#else		
	if ((stat = flash_erase((void *)(vctl_addr), vctl_size, (unsigned long *)&err_addr)) != 0)
	{
		printf("FAILED! Erase at %p: %s\n", err_addr, flash_errmsg(stat));
		free((char *)new_hdr);
		return -1;
	}
	if ((stat = flash_program((void *)(vctl_addr),(void *)new_hdr, total_size, (unsigned long *)&err_addr)) != 0)
	{
		printf("FAILED!: Program at %p: %s\n", err_addr, flash_errmsg(stat));
		free((char *)new_hdr);
		return -1;
	}
#endif	
	hal_flash_enable();
	memcpy((void *)vctl_datap, vctl_addr, vctl_size);
	hal_flash_disable();
		
	free((char *)new_hdr);
	return 0;
}


/*----------------------------------------------------------------------
* sys_get_vctl_mac
* MAC1:0x%02X%02X%02X%02X%02X%02X:ID1:1:MAP1:0x10:MAC2:0x%02X%02X%02X%02X%02X%02X:ID2:2:MAP2:0x0f
*----------------------------------------------------------------------*/
void sys_get_vctl_mac(char *mac1, char *mac2)
{
	char	entry_buf[128], *cp;
	int		i, bug=0;
	
	if (sys_get_vctl_entry(VCT_VLAN, (void *)entry_buf, sizeof(entry_buf)) == 0)
	{
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T) + 7);
		for (i=0; i<6; i++)
		{
			mac1[i] = char2hex(*cp++) * 16 + char2hex(*cp++);
		}
		if (*cp == 0x00) bug = 1;
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T) + 43);
		for (i=0; i<6; i++)
		{
			mac2[i] = char2hex(*cp++) * 16 + char2hex(*cp++);
		}
		if (*cp == 0x00) bug = 1;
		if (bug)
			sys_set_vctl_mac(mac1, mac2);
	}
}

/*----------------------------------------------------------------------
* sys_set_vctl_mac
* MAC1:0x%02X%02X%02X%02X%02X%02X:ID1:1:MAP1:0x10:MAC2:0x%02X%02X%02X%02X%02X%02X:ID2:2:MAP2:0x0f
*----------------------------------------------------------------------*/
void sys_set_vctl_mac(char *mac1, char *mac2)
{
	unsigned char	entry_buf[128], *cp, mac_buf[20];
	VCTL_ENTRY_T	*entry;
	int len;
	
	memset(entry_buf, 0, sizeof(entry_buf));
	entry = (VCTL_ENTRY_T *)entry_buf;
	if (sys_get_vctl_entry(VCT_VLAN, (void *)entry_buf, sizeof(entry_buf)) == 0)
	{
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T) + 7);
		sprintf(mac_buf, "%02X%02X%02X%02X%02X%02X:",
						mac1[0], mac1[1], mac1[2], mac1[3], mac1[4], mac1[5]);
		memcpy(cp, mac_buf, 13);
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T) + 43);
		sprintf(mac_buf, "%02X%02X%02X%02X%02X%02X:",
					mac2[0], mac2[1], mac2[2], mac2[3], mac2[4], mac2[5]);
		memcpy(cp, mac_buf, 13);
		sys_save_vctl_entry(VCT_VLAN, entry_buf, entry->size);
	}
	else
	{
		memcpy((void *)entry->header, VCTL_ENTRY_MAGIC, VCTL_MAGIC_SIZE);
		memset((void *)entry->majorver, '0', sizeof(entry->majorver));
		memset((void *)entry->minorver, '0', sizeof(entry->minorver));
		entry->type = VCT_VLAN;
		entry->size = sizeof(VCTL_ENTRY_T);
		cp = entry_buf + sizeof(VCTL_ENTRY_T);
		len = sprintf(cp, "MAC1:0x%02X%02X%02X%02X%02X%02X:ID1:1:MAP1:0x10:", 
							mac1[0], mac1[1], mac1[2], mac1[3], mac1[4], mac1[5]);
		cp += len;
		entry->size += len;
		len = sprintf(cp, "MAC2:0x%02X%02X%02X%02X%02X%02X:ID2:2:MAP2:0x0f",
							mac2[0], mac2[1], mac2[2], mac2[3], mac2[4], mac2[5]);
		cp += len;
		entry->size += len + 1;
		sys_save_vctl_entry(VCT_VLAN, entry_buf, entry->size);
	}
}

/*----------------------------------------------------------------------
* sys_find_str
*----------------------------------------------------------------------*/
UINT8 *sys_find_str(UINT8 *strp1, UINT8 *strp2)
{
	while (*strp1)
	{
		if (toupper(*strp1) == toupper(*strp2) && 
			memicmp(strp1, strp2, strlen(strp2)) == 0)
		{
			return strp1;
		}
		strp1++;
	}
	return NULL;
}

/*----------------------------------------------------------------------
* sys_copy_str
*----------------------------------------------------------------------*/
UINT8 *sys_copy_str(UINT8 *strp1, UINT8 *strp2)
{
	while (*strp2 && (*strp2!=' '))
	{
		*strp1 = *strp2;
		strp1++;
		strp2++;
	}
	*strp1 = 0x00;
	return strp2;
}

/*----------------------------------------------------------------------
* sys_get_vctl_ip
* IP:xxx.xxx.xxx.xxx Netmask:xxx.xxx.xxx.xxx Gateway:xxx.xxx.xxx.xxx
*                   ^                        ^                      ^
*                   +--> Space               +--> Space             +--> Null  
*----------------------------------------------------------------------*/
void sys_get_vctl_ip(UINT32 *ipaddr, UINT32 *netmask, UINT32 *gateway)
{
	char	entry_buf[128], *cp;
	int		i;
	
	if (sys_get_vctl_entry(VCT_IP, (void *)entry_buf, sizeof(entry_buf)) == 0)
	{
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		cp = sys_find_str(cp, "IP:");
		if (!cp)
			return;
		cp+=3;
		*ipaddr = str2ip(cp);
			
		cp = sys_find_str(cp, "Netmask:");
		if (!cp)
			return;
		cp+=8;
		*netmask = str2ip(cp);
		
		cp = sys_find_str(cp, "Gateway:");
		if (!cp)
			return;
		cp+=8;
		*gateway = str2ip(cp);
	}
}

/*----------------------------------------------------------------------
* sys_set_vctl_ip
* IP:xxx.xxx.xxx.xxx Netmask:xxx.xxx.xxx.xxx Gateway:xxx.xxx.xxx.xxx
*                   ^                        ^                      ^
*                   +--> Space               +--> Space             +--> Null  
*----------------------------------------------------------------------*/
void sys_set_vctl_ip(UINT32 ipaddr, UINT32 netmask, UINT32 gateway)
{
	char			entry_buf[128], *cp;
	VCTL_ENTRY_T	*entry;
	int				i, len;
	
	memset(entry_buf, 0, sizeof(entry_buf));
	entry = (VCTL_ENTRY_T *)entry_buf;
	if (sys_get_vctl_entry(VCT_IP, (void *)entry_buf, sizeof(entry_buf)) == 0)
	{
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		len = sprintf(cp, "IP:%d.%d.%d.%d Netmask:%d.%d.%d.%d Gateway:%d.%d.%d.%d",
					IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr),
					IP1(netmask), IP2(netmask), IP3(netmask), IP4(netmask),
					IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway));
		entry->size = (len + sizeof(VCTL_ENTRY_T) + 1 + 3) & ~3; // for alignment
		sys_save_vctl_entry(VCT_IP, entry_buf, entry->size);
	}
	else
	{
		memcpy((void *)entry->header, VCTL_ENTRY_MAGIC, VCTL_MAGIC_SIZE);
		memset((void *)entry->majorver, '0', sizeof(entry->majorver));
		memset((void *)entry->minorver, '0', sizeof(entry->minorver));
		entry->type = VCT_IP;
		entry->size = sizeof(VCTL_ENTRY_T);
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		len = sprintf(cp, "IP:%d.%d.%d.%d Netmask:%d.%d.%d.%d Gateway:%d.%d.%d.%d",
					IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr),
					IP1(netmask), IP2(netmask), IP3(netmask), IP4(netmask),
					IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway));
		entry->size = (len + sizeof(VCTL_ENTRY_T) + 1 + 3) & ~3; // for alignment
		sys_save_vctl_entry(VCT_IP, entry_buf, entry->size);
	}
}

#ifdef BOARD_SUPPORT_RAID
/*----------------------------------------------------------------------
* sys_set_vctl_recover
* IP:xxx.xxx.xxx.xxx Netmask:xxx.xxx.xxx.xxx Server:xxx.xxx.xxx.xxx Recover:x 
*                   ^                        ^                     ^         ^
*                   +--> Space               +--> Space            +--> Space+-->NULL  
*----------------------------------------------------------------------*/
void sys_set_vctl_recover(UINT32 ipaddr, UINT32 netmask, UINT32 gateway,int recovery)
{
	char		entry_buf[128], *cp;
	VCTL_ENTRY_T	*entry;
	int		i, len;
	
	memset(entry_buf, 0, sizeof(entry_buf));
	entry = (VCTL_ENTRY_T *)entry_buf;
	if (sys_get_vctl_entry(VCT_RECOVER, (void *)entry_buf, sizeof(entry_buf)) == 0)
	{
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		len = sprintf(cp, "IP:%d.%d.%d.%d Netmask:%d.%d.%d.%d Server:%d.%d.%d.%d Recover:%d",
					IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr),
					IP1(netmask), IP2(netmask), IP3(netmask), IP4(netmask),
					IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway),recovery);
		entry->size = (len + sizeof(VCTL_ENTRY_T) + 1 + 3) & ~3; // for alignment
		sys_save_vctl_entry(VCT_RECOVER, entry_buf, entry->size);
	}
	else
	{
		memcpy((void *)entry->header, VCTL_ENTRY_MAGIC, VCTL_MAGIC_SIZE);
		memset((void *)entry->majorver, '0', sizeof(entry->majorver));
		memset((void *)entry->minorver, '0', sizeof(entry->minorver));
		entry->type = VCT_RECOVER;
		entry->size = sizeof(VCTL_ENTRY_T);
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		len = sprintf(cp, "IP:%d.%d.%d.%d Netmask:%d.%d.%d.%d Server:%d.%d.%d.%d Recover:%d",
					IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr),
					IP1(netmask), IP2(netmask), IP3(netmask), IP4(netmask),
					IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway),recovery);
		entry->size = (len + sizeof(VCTL_ENTRY_T) + 1 + 3) & ~3; // for alignment
		sys_save_vctl_entry(VCT_RECOVER, entry_buf, entry->size);
	}
}
#endif

/*----------------------------------------------------------------------
* sys_get_vctl_boot_file
* 1:0x1600000 Filename-1 2:0x800000 Filename[NULL]
*            ^          ^               ^			
*            +--> Space +--> Space       +--> Space  
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_get_vctl_boot_file(UINT8 *f1_name, UINT32 *f1_addr, UINT8 *f2_name, UINT32 *f2_addr)
{
	char	entry_buf[512], *cp, *cp2;
	int		len;
	
	if (sys_get_vctl_entry(VCT_BOOT_FILE, (void *)entry_buf, sizeof(entry_buf)) == 0)
	{
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		cp = sys_find_str(cp, "1:");
		if (!cp) return;
		cp += 2;
		*f1_addr = str2value(cp);
				
		cp = sys_find_str(cp, " ");
		if (!cp) return;
		cp += 1;
		cp = sys_copy_str(f1_name, cp);
		
		cp = sys_find_str(cp, "2:");
		if (!cp) return;
		cp += 2;
		*f2_addr = str2value(cp);
		
		cp = sys_find_str(cp, " ");
		if (!cp) return;
		cp += 1;
		cp = sys_copy_str(f2_name, cp);
	}
}
#endif

/*----------------------------------------------------------------------
* sys_set_vctl_boot_file
* 1:0x1600000 Filename-1 2:0x800000 Filename[NULL]
*            ^          ^               ^			
*            +--> Space +--> Space       +--> Space  
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_vctl_boot_file(UINT8 *f1_name, UINT32 f1_addr, UINT8 *f2_name, UINT32 f2_addr)
{
	char			entry_buf[512], *cp;
	VCTL_ENTRY_T	*entry;
	int				i, len;
	
	memset(entry_buf, 0, sizeof(entry_buf));
	entry = (VCTL_ENTRY_T *)entry_buf;
	if (sys_get_vctl_entry(VCT_BOOT_FILE, (void *)entry_buf, sizeof(entry_buf)) == 0)
	{
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		len = sprintf(cp, "1:0x%X %s 2:0x%X %s", f1_addr, f1_name, f2_addr, f2_name);
		entry->size = (len + sizeof(VCTL_ENTRY_T) + 1 + 3) & ~3; // for alignment
		sys_save_vctl_entry(VCT_BOOT_FILE, entry_buf, entry->size);
	}
	else
	{
		memcpy((void *)entry->header, VCTL_ENTRY_MAGIC, VCTL_MAGIC_SIZE);
		memset((void *)entry->majorver, '0', sizeof(entry->majorver));
		memset((void *)entry->minorver, '0', sizeof(entry->minorver));
		entry->type = VCT_BOOT_FILE;
		entry->size = sizeof(VCTL_ENTRY_T);
		cp = (char *)(entry_buf + sizeof(VCTL_ENTRY_T));
		len = sprintf(cp, "1:0x%X %s 2:0x%X %s", f1_addr, f1_name, f2_addr, f2_name);
		entry->size = (len + sizeof(VCTL_ENTRY_T) + 1 + 3) & ~3; // for alignment
		sys_save_vctl_entry(VCT_BOOT_FILE, entry_buf, entry->size);
	}
}
#endif

#endif // BOARD_SUPPORT_VCTL




