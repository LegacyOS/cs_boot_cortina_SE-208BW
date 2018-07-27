/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_menu.c
* Description	: 
*		Handle UI menu
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/20/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>

static void ui_show_boot_menu(void);
static int ui_set_mac_addr(void);
char ui_getc(void);

typedef struct {
	char	key;
	void	(*handler)(int);
	int		arg;
	char	*msg;
} MENU_T;

static void ui_reset(int);
static void ui_load_kernel(int);
static void ui_load_usb(int);
extern void cli_main(int arg);
static void ui_set_ip(int);
static void ui_set_mac(int);
extern void ui_upgrade(int);
extern void ui_config(int);

#ifdef BOARD_SUPPORT_IDE	
static void ui_ide_init_cmd(int);
extern int ide_init(void);
#endif

#ifdef LOAD_FROM_IDE
static void ui_update_boot_file(int);
#endif

#ifdef BOARD_SUPPORT_FIS	
extern void fis_ui_list(int);
extern void fis_ui_delete_image(int);
extern void fis_ui_create_image(int);
extern void fis_ui_create_default(int);
extern void fis_ui_over_write_image(int);
#endif

MENU_T boot_menu_table[]=
{
	{'0', (void *)ui_reset, 			0,	"Reboot"},
	{'1', (void *)ui_load_kernel,		0,	"Start the Kernel Code"},
#ifdef BOARD_SUPPORT_FIS	
	{'2', (void *)fis_ui_list,			0,	"List Image"},
	{'3', (void *)fis_ui_delete_image,	0,	"Delete Image"},
	{'4', (void *)fis_ui_create_image,	0,	"Create New Image"},
#endif	
	{'5', (void *)cli_main, 			0,	"Enter Command Line Interface"},
	{'6', (void *)ui_set_ip, 			0,	"Set IP Address"},
	{'7', (void *)ui_set_mac, 			0,	"Set MAC Address"},
	{'8', (void *)ui_config,			0,	"Show Configuration"},
//#ifdef BOARD_SUPPORT_FIS
//	{'9', (void *)fis_ui_over_write_image,	0,	"Over Write Image"},
//#endif	
#ifdef LOAD_FROM_IDE
	{'B', (void *)ui_update_boot_file, 0,	"Set Booting File"},
#endif
#ifdef BOARD_SUPPORT_FIS	
	{'F', (void *)fis_ui_create_default, 0,	"Create Default FIS"},
#endif	
#ifdef BOARD_SUPPORT_IDE	
	{'I', (void *)ui_ide_init_cmd, 		0,	"Initialize IDE"},
#endif	
	{'X', (void *)ui_upgrade, 			0,	"Upgrade Boot"},
#ifndef LOAD_FROM_IDE	
	{'Y', (void *)ui_upgrade, 			1,	"Upgrade Kernel"},
#ifndef NETBSD	
#ifndef BOARD_NAND_BOOT	
	{'Z', (void *)ui_upgrade, 			2,	"Upgrade Firmware"},
#endif	
	{'A', (void *)ui_upgrade, 			4,	"Upgrade Application"},

	//{'C', (void *)ui_upgrade, 			4,	"Upgrade FS"},
	{'R', (void *)ui_upgrade, 			3,	"Upgrade RAM Disk"},
#ifdef BOARD_SUPPORT_TWO_CPU
	{'V', (void *)ui_upgrade, 			5,	"Upgrade Kernel #1"},
	{'W', (void *)ui_upgrade, 			6,	"Upgrade RAM Disk #1"},
#endif	
#endif
#endif	

  //      {'U', (void *)ui_load_usb,                      0,       "USB device mode" },
	{0,   NULL, 						0,	NULL},
};

#define MENU_TOTAL_ITEMS	(sizeof(boot_menu_table)/sizeof(MENU_T))
/*----------------------------------------------------------------------
* ui_menu
*----------------------------------------------------------------------*/
void ui_menu(void)
{
	int res = 0;
	char line[20];
	char command;
	int show_menu;
	MENU_T *menu;

	while (1)
	{
		ui_show_boot_menu();
		
		while (uart_scanc());
		command = ui_getc();
		command = toupper(command);
		
		if (command <= 0x20 || command >= 0x7f)
			continue;
			
		printf("%c\n\n", command);
		
		menu = (MENU_T *)&boot_menu_table[0];
		while (menu->key)
		{
			if (menu->key == command)
			{
				menu->handler(menu->arg);
				break;
			}
			menu++;
		}
	}
}

/*----------------------------------------------------------------------
* ui_show_same_char(char data, int size)
*----------------------------------------------------------------------*/
static void ui_show_same_char(char c, int size)
{
	char data[81], *cp;
	
	if (size > 80) 
		size = 80;
		
	cp = data;
	while (size--)
		*cp++ = c;
	*cp = 0x00;
	printf(data);
}
		

/*----------------------------------------------------------------------
* ui_show_boot_menu
*----------------------------------------------------------------------*/
static void ui_show_boot_menu(void)
{
	MENU_T *menu;
	int count;
	
	printf("\n");
	ui_show_same_char(' ', 30);
	printf("Boot Menu\n");
	ui_show_same_char('=', 78);
	printf("\n");
	
	menu = (MENU_T *)&boot_menu_table[0];
	count = 0;
	while (menu->key)
	{
		printf("%c: ", menu->key);
		printf(menu->msg);
		if (MENU_TOTAL_ITEMS < 10)
		{
			printf("\n");	
		}
		else
		{
			if (count & 1)
				printf("\n");
			else
				ui_show_same_char(' ', 41-strlen(menu->msg));
		}
		count++;
		menu++;
	}
	if (count & 1)
		printf("\n");
	printf("\n");
	printf("=> Select: ");
}

/*----------------------------------------------------------------------
* ui_reset
*----------------------------------------------------------------------*/
static void ui_reset(int arg)
{
	printf("Reboot system...\n");
	hal_reset();
}

/*----------------------------------------------------------------------
* ui_load_kernel
*----------------------------------------------------------------------*/
static void ui_load_kernel(int arg)
{
#ifdef LOAD_FROM_IDE
	ide_init();
	ext2fs_init();
#endif
	sys_load_kernel();
}

static void ui_load_usb(int arg)
{
 faraday_usb_main(); 

}

/*----------------------------------------------------------------------
* ui_set_ip
*----------------------------------------------------------------------*/
static void ui_set_ip(int arg)
{
	int rc;
	UINT32 ipaddr, netmask, gateway;
	char ip_str[20];
	
	ipaddr = (UINT32)sys_get_ip_addr();
	printf("Old IP Address: %d.%d.%d.%d\n", IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr));
	printf("Input new IP  : ");
	sprintf(ip_str, "%d.%d.%d.%d", IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr));
	rc = ui_gets(ip_str, sizeof(ip_str));
	ipaddr = str2ip(ip_str);
	if (rc == 0 || ipaddr == 0 || ipaddr == 0xffffffff)
	{
		printf("Illegal IP address!\n");
		return;
	}
	sys_set_ip_addr(ipaddr);
	netmask = sys_get_ip_netmask();
	// sys_set_ip_netmask(0xffffff00);	// class C
	gateway = sys_get_ip_gateway();
	printf("Old Gateway Address: %d.%d.%d.%d\n", IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway));
	printf("Input new Gateway  : ");
	sprintf(ip_str, "%d.%d.%d.%d", IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway));
	rc = ui_gets(ip_str, sizeof(ip_str));
	gateway = str2ip(ip_str);
	if (rc == 0 || gateway == 0 || gateway == 0xffffffff)
	{
		printf("Illegal Gateway address!\n");
		return;
	}
	sys_set_ip_gateway(gateway);
//	sys_set_ip_gateway((ipaddr & netmask) | (gateway & ~netmask));
#ifdef BOARD_SUPPORT_VCTL	
	printf("Write New IP Address to Flash!\n");
	sys_save_ip_cfg();
#endif	
	ipaddr = sys_get_ip_addr();
	printf("New IP Address: %d.%d.%d.%d\n", IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr));
}	

/*----------------------------------------------------------------------
* ui_set_mac
*----------------------------------------------------------------------*/
static void ui_set_mac(int arg)
{
	char *mac, new_mac[6];
	char new_mac_str[18], *cp;
	int k, i, rc;
	UINT32 data;
	
	
	for (k=0; k<SYS_MAC_NUM; k++)
	{
		mac = (char *)sys_get_mac_addr(k);
		printf("MAC %d Address = %02X:%02X:%02X:%02X:%02X:%02X\n",
					k+1, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		printf("Input New MAC = ");
	
		sprintf(new_mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		rc = ui_gets(new_mac_str, 18);
		if (rc == 0 || rc != 17 || new_mac_str[2] !=':')
		{
			printf("Illegal MAC address!\nSyntax: xx:xx:xx:xx:xx:xx\n");
			return;
		}
	
		cp = (char *)&new_mac_str[0];
		for (i=0; i<6; i++)
		{
			cp[2] = 0;
			data = str2hex(cp);
			new_mac[i] = data;
			cp += 3;
		}
	
		if (new_mac[0] & 1)
		{
			printf("MAC address could not be multicast/broadcast address!\n");
			return;
		}
		printf("New MAC %d     = %02X:%02X:%02X:%02X:%02X:%02X\n",
			k+1, new_mac[0],new_mac[1],new_mac[2],new_mac[3],new_mac[4],new_mac[5]);
	
		sys_set_mac_addr(k, new_mac);
	}
		
	printf("Write new MAC address to Flash!\n");
	sys_save_mac();
}

#ifdef BOARD_SUPPORT_IDE
extern int ide_initialized;
static void ui_ide_init_cmd(int arg)
{
	ide_initialized = 0;
	ide_init();
}
#endif

#ifdef LOAD_FROM_IDE
static void ui_update_boot_file(int arg)
{
	int 			rc, change;
	char			buf[60];
	unsigned long	data;

	// kernel File
	strcpy(buf, (char *)sys_get_kernel_name());
	printf("Old Kernel File: %s\n", buf);
	printf("New Kernel File: ");
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
	{
		printf("Illegal filename!\n");
		return;
	}
	sys_set_kernel_name(buf, 0);
	change = 1;
	
	sprintf(buf, "0x%08X", sys_get_kernel_addr());
	printf("Old Kernel Location: %s\n", buf);
	printf("New Kernel Location: ");
	rc = ui_gets(buf, sizeof(buf));
	data = str2hex(buf);
	if (rc == 0 || data < 0x1000 || data >= BOARD_DRAM_BOOT2_ADDR)
	{
		printf("Illegal location!\n");
		return;
	}
	sys_set_kernel_addr(data, 0);
	change = 1;
	
	// initrd File
	strcpy(buf, (char *)sys_get_initrd_name());
	printf("Old initrd file: %s\n", buf);
	printf("New initrd file: ");
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
	{
		printf("Illegal filename!\n");
		return;
	}
	sys_set_initrd_name(buf, 0);
	change = 1;
	
	sprintf(buf, "0x%08X", sys_get_initrd_addr());
	printf("Old initrd location: %s\n", buf);
	printf("New initrd location: ");
	rc = ui_gets(buf, sizeof(buf));
	data = str2hex(buf);
	if (rc == 0 || data < 0x1000 || data >= BOARD_DRAM_BOOT2_ADDR)
	{
		printf("Illegal location!\n");
		return;
	}
	sys_set_initrd_addr(data, 0);
	change = 1;

#ifdef BOARD_SUPPORT_TWO_CPU
	// kernel #1 File
	strcpy(buf, (char *)sys_get_kernel1_name());
	printf("Old Kernel File: %s\n", buf);
	printf("New Kernel File: ");
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
	{
		printf("Illegal filename!\n");
		return;
	}
	sys_set_kernel1_name(buf, 0);
	change = 1;
	
	sprintf(buf, "0x%08X", sys_get_kernel1_addr());
	printf("Old Kernel Location: %s\n", buf);
	printf("New Kernel Location: ");
	rc = ui_gets(buf, sizeof(buf));
	data = str2hex(buf);
	if (rc == 0 || data < 0x1000 || data >= BOARD_DRAM_BOOT2_ADDR)
	{
		printf("Illegal location!\n");
		return;
	}
	sys_set_kernel1_addr(data, 0);
	change = 1;
	
	// initrd #1 File
	strcpy(buf, (char *)sys_get_initrd1_name());
	printf("Old initrd file: %s\n", buf);
	printf("New initrd file: ");
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
	{
		printf("Illegal filename!\n");
		return;
	}
	sys_set_initrd1_name(buf, 0);
	change = 1;
	
	sprintf(buf, "0x%08X", sys_get_initrd1_addr());
	printf("Old initrd location: %s\n", buf);
	printf("New initrd location: ");
	rc = ui_gets(buf, sizeof(buf));
	data = str2hex(buf);
	if (rc == 0 || data < 0x1000 || data >= BOARD_DRAM_BOOT2_ADDR)
	{
		printf("Illegal location!\n");
		return;
	}
	sys_set_initrd1_addr(data, 0);
	change = 1;
#endif
update_end:
	if (change)
	{
		printf("Write new booting files to Flash!\n");
		sys_save_boot_file();
#ifdef BOARD_SUPPORT_TWO_CPU
		sys_save_boot_file1();
#endif		
	}
}
#endif



