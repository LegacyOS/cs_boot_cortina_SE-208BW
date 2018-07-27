/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_cfg.c
* Description	: 
*		Handle system configuration
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/20/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sys_vctl.h>

#ifndef SYS_MAC_NUM
#define SYS_MAC_NUM		1
#endif
#define SYS_MAC_MAX		2

static unsigned char null_mac_addr[6] = {0x00,0x000,0x00,0x00,0x00,0x00};
unsigned char my_mac_addr[SYS_MAC_MAX][6] = 
	{BOARD_DEFAULT_MAC0_ADDR, BOARD_DEFAULT_MAC1_ADDR};

static UINT32 my_ip_addr;
static UINT32 my_ip_netmask;
static UINT32 my_ip_gateway;

#ifdef LOAD_FROM_IDE	
static char my_kernel_name[128];
static char my_initrd_name[128];
static UINT32 my_kernel_addr;
static UINT32 my_initrd_addr;

#ifdef BOARD_SUPPORT_TWO_CPU
static char my_kernel1_name[128];
static char my_initrd1_name[128];
static UINT32 my_kernel1_addr;
static UINT32 my_initrd1_addr;
#endif

#endif

extern void sys_get_vctl_mac(char *mac1, char *mac2);
extern void sys_set_vctl_mac(char *mac1, char *mac2);
extern void sys_get_vctl_ip(UINT32 *ipaddr, UINT32 *netmask, UINT32 *gateway);
extern void sys_set_vctl_ip(UINT32 ipaddr, UINT32 netmask, UINT32 gateway);
void sys_get_vctl_boot_file(UINT8 *f1_name, UINT32 *f1_addr, UINT8 *f2_name, UINT32 *f2_addr);
void sys_set_vctl_boot_file(UINT8 *f1_name, UINT32 f1_addr, UINT8 *f2_name, UINT32 f2_addr);
extern void gmac_set_mac_addr(unsigned char *mac1, unsigned char *mac2);
extern UINT32 char2hex(UINT8 c);

/*----------------------------------------------------------------------
* sys_init_cfg
*----------------------------------------------------------------------*/
void sys_init_cfg(void)
{
	int ipaddr[4] = BOARD_DEFAULT_IP_ADDR;
	int netmask[4] = BOARD_DEFAULT_IP_NETMASK;
	int gateway[4] = BOARD_DEFAULT_GATEWAY;

	// default value
	my_ip_addr = IPIV(ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
	my_ip_netmask = IPIV(netmask[0], netmask[1], netmask[2], netmask[3]);
	my_ip_gateway = IPIV(gateway[0], gateway[1], gateway[2], gateway[3]);
	
	// get configuration from flash 
	sys_get_vctl_mac(&my_mac_addr[0][0], &my_mac_addr[1][0]);
	sys_get_vctl_ip(&my_ip_addr, &my_ip_netmask, &my_ip_gateway);

#ifdef LOAD_FROM_IDE	
	strcpy(my_kernel_name, KERNEL_FILENAME);
	strcpy(my_initrd_name, INITRD_FILENAME);
	my_kernel_addr = BOARD_DRAM_KERNEL_ADDR;
	my_initrd_addr = BOARD_DRAM_RAM_DISK_ADDR;
	sys_get_vctl_boot_file(my_kernel_name, &my_kernel_addr, my_initrd_name, &my_initrd_addr);
#ifdef BOARD_SUPPORT_TWO_CPU
	strcpy(my_kernel1_name, KERNEL1_FILENAME);
	strcpy(my_initrd1_name, INITRD1_FILENAME);
	my_kernel1_addr = BOARD_FLASH_CPU2_ADDR;
	my_initrd1_addr = BOARD_FLASH_CPU2_RD_ADDR;
#endif	
#endif	
}

/*----------------------------------------------------------------------
* sys_save_mac
*----------------------------------------------------------------------*/
void sys_save_mac(void)
{
	sys_set_vctl_mac(&my_mac_addr[0][0], &my_mac_addr[1][0]);
	
}

/*----------------------------------------------------------------------
* sys_save_ip_cfg
*----------------------------------------------------------------------*/
void sys_save_ip_cfg(void)
{
	sys_set_vctl_ip(my_ip_addr, my_ip_netmask, my_ip_gateway);
}

/*----------------------------------------------------------------------
* sys_save_boot_file
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_save_boot_file(void)
{
	sys_set_vctl_boot_file(my_kernel_name, my_kernel_addr, my_initrd_name, my_initrd_addr);
}
#endif

/*----------------------------------------------------------------------
* sys_get_mac_addr
*	get mac address
*----------------------------------------------------------------------*/
char *sys_get_mac_addr(int index)
{
	if (index >= 0 && index < SYS_MAC_NUM)
		return (char *)&my_mac_addr[index][0];
	else
		return (char *)null_mac_addr;
}

/*----------------------------------------------------------------------
* sys_set_mac_addr
*	set mac address
*----------------------------------------------------------------------*/
void sys_set_mac_addr(int index, char *mac_addr)
{
	if (index < 0 || index >= SYS_MAC_NUM)
		return;
	
	if ((mac_addr[0] & 1) || memcmp(mac_addr, null_mac_addr, 6) == 0)
		return;
	
	memcpy((char *)&my_mac_addr[index][0], mac_addr, 6);
#ifdef MIDWAY
	gmac_set_mac_addr(&my_mac_addr[0][0], &my_mac_addr[1][0]);
#else
	emac_set_mac_addr(&my_mac_addr[0][0], &my_mac_addr[1][0]);
#endif
}

/*----------------------------------------------------------------------
* sys_get_ip_addr
*	get IP address
*----------------------------------------------------------------------*/
UINT32 sys_get_ip_addr(void)
{
	return my_ip_addr;
}

/*----------------------------------------------------------------------
* sys_set_ip_addr
*	set IP address
*----------------------------------------------------------------------*/
void sys_set_ip_addr(UINT32 ipaddr)
{
	if (ip_verify_addr(ipaddr) == 0)
		my_ip_addr = ipaddr;
	else
		printf("Failed to change IP address due to illegal value\n");
}

/*----------------------------------------------------------------------
* sys_get_ip_netmask
*	get IP netmask
*----------------------------------------------------------------------*/
UINT32 sys_get_ip_netmask(void)
{
	return my_ip_netmask;
}

/*----------------------------------------------------------------------
* sys_set_ip_netmask
*	set IP netmask
*----------------------------------------------------------------------*/
void sys_set_ip_netmask(UINT32 netmask)
{
	if (ip_verify_netmask(netmask) == 0)
		my_ip_netmask = netmask;
	else
		printf("Failed to change IP netmask due to illegal value\n");
}

/*----------------------------------------------------------------------
* sys_get_ip_gateway
*	get IP address
*----------------------------------------------------------------------*/
UINT32 sys_get_ip_gateway(void)
{
	return my_ip_gateway;
}

/*----------------------------------------------------------------------
* sys_set_ip_gateway
*	set IP gateway
*----------------------------------------------------------------------*/
void sys_set_ip_gateway(UINT32 ipaddr)
{
	if (ip_verify_addr(ipaddr) == 0)
		my_ip_gateway = ipaddr;
	else
		printf("Failed to change IP Gateway due to illegal value\n");
}

/*----------------------------------------------------------------------
* sys_show_mac_addr
*----------------------------------------------------------------------*/
void sys_show_mac_addr(void)
{
	int i;
	char *cp;
	
	cp = (char *)& my_mac_addr[0][0];
	for (i=0; i<SYS_MAC_NUM; i++)
	{
		printf("MAC %d Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
				i+1, *cp++, *cp++, *cp++, *cp++, *cp++, *cp++);
	}
}

/*----------------------------------------------------------------------
* sys_show_ip_addr
*----------------------------------------------------------------------*/
void sys_show_ip_addr(void)
{
#if 0	
	printf("inet addr: %d.%d.%d.%d   Mask: %d.%d.%d.%d   Gateway: %d.%d.%d.%d\n",
			IP1(my_ip_addr), IP2(my_ip_addr), IP3(my_ip_addr), IP4(my_ip_addr),
			IP1(my_ip_netmask), IP2(my_ip_netmask), IP3(my_ip_netmask), IP4(my_ip_netmask),
			IP1(my_ip_gateway), IP2(my_ip_gateway), IP3(my_ip_gateway), IP4(my_ip_gateway));
#else
	printf("inet addr: %d.%d.%d.%d/%d.%d.%d.%d",
			IP1(my_ip_addr), IP2(my_ip_addr), IP3(my_ip_addr), IP4(my_ip_addr),
			IP1(my_ip_netmask), IP2(my_ip_netmask), IP3(my_ip_netmask), IP4(my_ip_netmask));
	printf(" Gateway: %d.%d.%d.%d\n",
			IP1(my_ip_gateway), IP2(my_ip_gateway), IP3(my_ip_gateway), IP4(my_ip_gateway));
#endif			
}

/*----------------------------------------------------------------------
* sys_set_kernel_name
*	set Kernel file name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_kernel_name(char *name, int write)
{
	if (strcmp(name, my_kernel_name) != 0)
	{
		strcpy(my_kernel_name, name);
		if (write) sys_save_boot_file();
	}
}
#endif
/*----------------------------------------------------------------------
* sys_get_kernel_name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
char *sys_get_kernel_name(void)
{
	return my_kernel_name;
}
#endif

/*----------------------------------------------------------------------
* sys_set_kernel_addr
*	set Kernel file addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_kernel_addr(unsigned long addr, int write)
{
	if (addr != my_kernel_addr)
	{
		my_kernel_addr = addr;
		if (write) sys_save_boot_file();
	}
}
#endif

/*----------------------------------------------------------------------
* sys_get_kernel_addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
unsigned long sys_get_kernel_addr(void)
{
	return my_kernel_addr;
}
#endif

/*----------------------------------------------------------------------
* sys_set_initrd_name
*	set initrd file name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_initrd_name(char *name, int write)
{
	if (strcmp(name, my_initrd_name) != 0)
	{
		strcpy(my_initrd_name, name);
		if (write) sys_save_boot_file();
	}
}
#endif

/*----------------------------------------------------------------------
* sys_get_initrd_name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
char *sys_get_initrd_name(void)
{
	return my_initrd_name;
}
#endif

/*----------------------------------------------------------------------
* sys_set_initrd_addr
*	set initrd file addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_initrd_addr(unsigned long addr, int write)
{
	if (addr != my_initrd_addr)
	{
		my_initrd_addr = addr;
		if (write) sys_save_boot_file();
	}
}
#endif

/*----------------------------------------------------------------------
* sys_get_initrd_addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
unsigned long sys_get_initrd_addr(void)
{
	return my_initrd_addr;
}
#endif

#ifdef BOARD_SUPPORT_TWO_CPU

/*----------------------------------------------------------------------
* sys_save_boot_file
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_save_boot_file1(void)
{
	sys_set_vctl_boot_file(my_kernel1_name, my_kernel1_addr, my_initrd1_name, my_initrd1_addr);
}
#endif

/*----------------------------------------------------------------------
* sys_set_kernel_name
*	set Kernel file name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_kernel1_name(char *name, int write)
{
	if (strcmp(name, my_kernel1_name) != 0)
	{
		strcpy(my_kernel1_name, name);
		if (write) sys_save_boot_file1();
	}
}
#endif
/*----------------------------------------------------------------------
* sys_get_kernel_name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
char *sys_get_kernel1_name(void)
{
	return my_kernel1_name;
}
#endif

/*----------------------------------------------------------------------
* sys_set_kernel_addr
*	set Kernel file addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_kernel1_addr(unsigned long addr, int write)
{
	if (addr != my_kernel1_addr)
	{
		my_kernel1_addr = addr;
		if (write) sys_save_boot_file1();
	}
}
#endif

/*----------------------------------------------------------------------
* sys_get_kernel_addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
unsigned long sys_get_kernel1_addr(void)
{
	return my_kernel1_addr;
}
#endif

/*----------------------------------------------------------------------
* sys_set_initrd_name
*	set initrd file name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_initrd1_name(char *name, int write)
{
	if (strcmp(name, my_initrd1_name) != 0)
	{
		strcpy(my_initrd1_name, name);
		if (write) sys_save_boot_file1();
	}
}
#endif

/*----------------------------------------------------------------------
* sys_get_initrd_name
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
char *sys_get_initrd1_name(void)
{
	return my_initrd1_name;
}
#endif

/*----------------------------------------------------------------------
* sys_set_initrd_addr
*	set initrd file addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
void sys_set_initrd1_addr(unsigned long addr, int write)
{
	if (addr != my_initrd1_addr)
	{
		my_initrd1_addr = addr;
		if (write) sys_save_boot_file1();
	}
}
#endif

/*----------------------------------------------------------------------
* sys_get_initrd_addr
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE	
unsigned long sys_get_initrd1_addr(void)
{
	return my_initrd1_addr;
}
#endif



#endif//BOARD_SUPPORT_TWO_CPU

/*----------------------------------------------------------------------
* sys_show_sw_cfg
*----------------------------------------------------------------------*/
void sys_show_sw_cfg(void)
{
	sys_show_mac_addr();
	sys_show_ip_addr();
#ifdef LOAD_FROM_IDE	
	printf("Kernel RAM Location: 0x%08X  Filename: %s\n", sys_get_kernel_addr(), sys_get_kernel_name());
	printf("Initrd RAM Location: 0x%08X  Filename: %s\n", sys_get_initrd_addr(), sys_get_initrd_name());
#endif
}



