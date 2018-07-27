/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_cli.c
* Description	: 
*		Handle UI CLI function
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/22/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include "ui_cli.h"

extern void cli_show_sys_cfg(char, char **);
extern void cli_show_mem(char, char **);
extern void cli_exit(char, char **);
extern void cli_dump_cmd(char, char **);
extern void cli_write_mem_cmd(char, char **);
extern void cli_ping_cmd(char, char **);
extern void cli_tftp_cmd(char, char **);
extern void cli_go_cmd(char, char **);
extern void cli_load_cmd(char, char **);
extern void arp_show_cache(char, char **);
extern void timer_cli(char, char **);
extern void cli_test_cmd(char, char **);
extern void cli_stat_cmd(char, char **);
extern void astelCodecIo(char, char **);
extern void astelI2cIo(char, char **);
extern void astelMiiRead(char ,char **);
extern void astelMiiWrite(char ,char **);
extern void astelCrc32(char, char **);
extern void getEnvCrcAndFlashCrc(char, char **);
extern int getEnvSizeAstel(char, char **);
#ifdef BOARD_NAND_BOOT
extern void cli_nand_cmd(char, char **);
#endif
#ifdef BOARD_SUPPORT_IDE
	extern void ext2fs_ls_cmd(char, char **);
	extern void ide_ui_test_cmd(char, char **);
	extern void aoe_ui_cmd(char, char **);
#endif

const CLI_CMD_T cli_main_tbl[]=
{
	/* Name     Next Table      Command                 	Help */
    {"arp",		NULL,			(void *)arp_show_cache,		"Display ARP table"},
    {"config",	NULL,			(void *)cli_show_sys_cfg,	"Display System Configuration"},
    {"dm", 		NULL,			(void *)cli_dump_cmd,		"Display (hex dump) a range of memory."},
    {"exit", 	NULL,			(void *)cli_exit,			"Exit."},
    {"go", 		NULL,			(void *)cli_go_cmd,			"Execute code at a location"},
    {"load", 	NULL,			(void *)cli_load_cmd,		"Load code"},
    {"mem",		NULL,			(void *)cli_show_mem,		"Display memory information"},
    {"ping", 	NULL,			(void *)cli_ping_cmd,		"Ping host by IP address."},
    {"quit", 	NULL,			(void *)cli_exit,			"Exit."},
    {"sm", 		NULL,			(void *)cli_write_mem_cmd,	"Write data to specified location."},
    {"stat", 	NULL,			(void *)cli_stat_cmd,		"Statistics."},
    {"tftp", 	NULL,			(void *)cli_tftp_cmd,		"Get remote file by TFTP."},
    {"timer", 	NULL,			(void *)timer_cli,			"timer."},
    {"io",	 	NULL,			(void *)astelCodecIo,		"astel Codec io command"},
    {"iic",	 	NULL,			(void *)astelI2cIo,			"astel i2c command"},
    {"miiread",	 	NULL,			(void *)astelMiiRead,		"PHY Chip Read Command"},
    {"miiwrite", 	NULL,			(void *)astelMiiWrite,		"PHY Chip Write command"},
	{"crc32",	NULL,			(void *)astelCrc32,			"check crc32"},
	{"getEnvSize",	NULL,			(void *)getEnvSizeAstel,			"env(kernel, ramdisk) size"},
	{"getEnvCrc",	NULL,			(void *)getEnvCrcAndFlashCrc,			"env(kernel, ramdisk) crc"},
#ifdef BOARD_SUPPORT_IDE
    {"ls", 		NULL,			(void *)ext2fs_ls_cmd,		"List files"},
//  {"disk_test",NULL,			(void *)ide_ui_test_cmd,	"Test Disk RW"},
#ifdef BOARD_SUPPORT_AOE
    {"aoe", 		NULL,		(void *)aoe_ui_cmd,			"AOE"},
#endif   
#endif
#ifdef BOARD_NAND_BOOT
    {"nand", 		NULL,			(void *)cli_nand_cmd,		"Nand flash function"},
#endif
    {NULL, NULL, NULL, NULL} 
};

