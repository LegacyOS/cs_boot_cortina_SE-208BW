/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: lpc.c
* Description	: 
*		Handle LPC module
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/19/2005	Gary Chen	Implement from Jason's Redboot code
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include "lpc.h"

unsigned char MBPnP_key[4]= {0x87, 0x01, 0x55, 0x55};
void LPCEnterMBPnP(void)
{
	int i;
	// unsigned char key[4] = {0x87, 0x01, 0x55, 0x55};

	for (i = 0; i<4; i++)
		HAL_WRITE_UINT8(LPC_KEY_ADDR, MBPnP_key[i]);
}

void LPCExitMBPnP(void)
{
	HAL_WRITE_UINT8(LPC_KEY_ADDR, 0x02);
	HAL_WRITE_UINT8(LPC_DATA_ADDR, 0x02);
}

void LPCSetConfig(char LdnNumber, char Index, char data)
{
	LPCEnterMBPnP();				// Enter IT8712 MB PnP mode
	HAL_WRITE_UINT8(LPC_KEY_ADDR, 0x07);
	HAL_WRITE_UINT8(LPC_DATA_ADDR, LdnNumber);
	HAL_WRITE_UINT8(LPC_KEY_ADDR, Index);	
	HAL_WRITE_UINT8(LPC_DATA_ADDR, data);
	LPCExitMBPnP();
}

char LPCGetConfig(char LdnNumber, char Index)
{
	char rtn;
	LPCEnterMBPnP();				// Enter IT8712 MB PnP mode
	HAL_WRITE_UINT8(LPC_KEY_ADDR, 0x07);
	HAL_WRITE_UINT8(LPC_DATA_ADDR, LdnNumber);
	HAL_WRITE_UINT8(LPC_KEY_ADDR, Index);	
	HAL_READ_UINT8(LPC_DATA_ADDR, rtn);
	LPCExitMBPnP();
	return rtn;
}

int SearchIT8712(void)
{
	unsigned char Id1, Id2;
	unsigned short Id;
	LPCEnterMBPnP();
	HAL_WRITE_UINT8(LPC_KEY_ADDR, 0x20); /* chip id byte 1 */
	HAL_READ_UINT8(LPC_DATA_ADDR, Id1);
	HAL_WRITE_UINT8(LPC_KEY_ADDR, 0x21); /* chip id byte 2 */
	HAL_READ_UINT8(LPC_DATA_ADDR, Id2);
	Id = (Id1 << 8) | Id2;
//	diag_printf("ID is %x \n", Id);
	LPCExitMBPnP();
	if (Id == 0x8712) 
		return 0;
	else
		return 1;
}

