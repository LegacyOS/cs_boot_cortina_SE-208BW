/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_cli_main.c
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
#include <ui/ui_cli.h>

/*----------------------------------------------------------------------
* ui_getc
*----------------------------------------------------------------------*/
char ui_getc(void)
{
	char	key;
	int		rc;
	char	cnt1, cnt2;
	
	do {
		rc = uart_scanc(&key);
		if (!rc)
			enet_poll();
		else if (key == KEY_ESC)
		{
			cnt1 = 200;
			while (cnt1--)
			{
				if (uart_scanc(&key) && key == '[')
				{
					cnt2 = 200;
					while (cnt2--)
					{
						if (uart_scanc(&key))
						{
							if (key == 'A')
								return ARROW_U;
							else if (key == 'B')
								return ARROW_D;
							else if (key == 'C')
								return ARROW_R;
							else if (key == 'D')
								return ARROW_L;
							else
								return KEY_ESC;
						}
						hal_delay_us(1000);
						// sys_sleep(CONFIG_TPS/100);
					}
					break;
				}
				hal_delay_us(1000);
				// sys_sleep(CONFIG_TPS/100);
			}
			return KEY_ESC;
		}			
	} while (!rc);
	
	return key;
}

/*--------------------------------------------------------------
* 	uart_get_confirm_key
*		return 	TRUE: Y or y
*				FALSE: N or n
---------------------------------------------------------------*/
int ui_get_confirm_key(void)
{
	UINT8 key;
	
	do {
		key = ui_getc();
	} while (key != 'Y' && key != 'y' && key != 'N' && key != 'n');
	
	uart_putc(key);
	
    if (key == 'Y' || key == 'y')
    	return TRUE;
    else
    	return FALSE;
}

/*--------------------------------------------------------------
* 	uart_gets
---------------------------------------------------------------*/
int ui_gets(char *buf, int size)
{
	UINT8 key;
	int total;
	char *dest;
	int first_key = 1;
	
	if (size <= 0)
		return 0;
		
	total = strlen(buf);
	if (total >= size)
	{
		total = 0;
		dest = buf;
	}
	else
	{
		dest = buf + total;
		printf(buf);
	}
		
	
	do {
		key = ui_getc();
		if (key == 0x0d || key == 0x0a)
			break;
		if (key == 0x03 || key == 0x1b)
		{
			printf("\n");
			return 0;
		}
		else if (key == '\b' || key == ARROW_L)
		{
			first_key = 0;
			if (total)
			{
				dest--;
				total--;
				uart_putc('\b');
				uart_putc(' ');
				uart_putc('\b');
			}
		}
		else if (key >= 0x20 && key <= 0x7f)
		{
			if (first_key && total)
			{
				memset(buf, '\b', total); buf[total]=0x00;
				printf(buf);
				memset(buf, ' ', total);
				printf(buf);
				memset(buf, '\b', total);
				printf(buf);
				total = 0;
				dest = buf;
			}
			first_key = 0;
				
			if (total < size)
			{
				uart_putc(key);
				*dest++ = key;
				total++;
			}
		}
	} while(1);
	
	*dest = 0x00;
	printf("\n");
	return total;
}

