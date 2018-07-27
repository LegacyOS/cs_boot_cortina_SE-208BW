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
#include <board_config.h>
#include "ui_cli.h"

void cli_init(void);
void cli_show_all_commands(CLI_CMD_T *current_cmd);

static void cli_parse_enter(CLI_INFO_T  *cli);
static char *cli_get_name(char *in, char *name);
static char *cli_get_argument_ptr(char *in);
static int cli_get_matched_cmd(CLI_INFO_T  *cli, CLI_CMD_T *current_cmd,
                          char *name, CLI_CMD_T **matched_cmds);
static void cli_parse_argument(CLI_INFO_T  *cli, char *datap);
static void cli_show_more_cmds(CLI_CMD_T **start_cmd, int total);
static void cli_cursor_left(char col);
static int cli_bcmp(char *str1, char *str2, int len);

extern char ui_getc(void);

CLI_INFO_T cli_info;
extern const CLI_CMD_T cli_main_tbl[];
int cli_exit_flag;

#define CLI_PROMPT		printf("\n"); printf(BOARD_CLI_PROMPT)

/*----------------------------------------------------------------------
* cli_main
*----------------------------------------------------------------------*/
void cli_main(int arg)
{
    CLI_INFO_T *cli;
    CLI_HISTORY_T *history;
    char *data_bufp;
    char current_history = 0;
	char data, cnt;

    cli=(CLI_INFO_T *)&cli_info;
    history = (CLI_HISTORY_T *)&cli->history;
    data_bufp = (char *)&cli->buf[0];
    
    memset(cli, 0, sizeof(CLI_INFO_T));

    cli->cmd_p=(CLI_CMD_T *)&cli_main_tbl;
    CLI_PROMPT;

	cli_exit_flag = 0;
 
	while (!cli_exit_flag)
    {
		data = ui_getc();
        switch (data)
		{
		case KEY_BS:	// back space
		case ARROW_L:
			if (cli->total)
			{
				data_bufp[cli->total] = '\0';
				cli->total--;
				cli_cursor_left(1);
			}
			break;
		case KEY_TAB:	// '\t'
		case KEY_SP:	// space character
			data_bufp[cli->total] = ' ';
			data_bufp[cli->total+1] = '\0';
			printf(" ");
			cli->total++;
			break;
		case KEY_CR: 	// '\r'
		case KEY_LF:	// '\n'
			data_bufp[cli->total++] = 0x00;
			if (strlen(data_bufp))
			{
				strcpy((char *)&history->buf[history->index][0], data_bufp);
				if (++history->index >= CLI_MAX_HISTORY)
					history->index = 0;
				if (history->total < CLI_MAX_HISTORY)
					history->total++;
				cli_parse_enter(cli);
			}
			current_history=history->index;
			cli->total = 0;
			CLI_PROMPT;
			current_history=history->index;
			break;
		case KEY_ESC:	// ESC key
			cli->total = 0;
			CLI_PROMPT;
			current_history=history->index;
			break;
		case KEY_CTRL_U:
		case ARROW_U:
			if (history->total)
			{
				if (current_history==0)
					current_history=history->total-1;
				else
  					current_history--;
				if (cli->total)
				{
					cli_cursor_left(cli->total);
				}
				strcpy((char *)data_bufp,
					   (char *)&history->buf[current_history][0]);
				printf(data_bufp);
 				cli->total=strlen(data_bufp);
			}
			break;
		case ARROW_D:
		case KEY_CTRL_D:
		case KEY_CTRL_N:
			if (history->total)
			{
				current_history++;
				if (current_history>=history->total)
					current_history=0;
				if (cli->total)
				{
					cli_cursor_left(cli->total);
				}
				strcpy(data_bufp, (char *)&history->buf[current_history][0]);
				printf(data_bufp);
				cli->total=strlen((char *)&data_bufp[0]);
			}
			break;
		default:
			if (data<0x20 || data>0x7f) break;
			data_bufp[cli->total] = data;
			data_bufp[cli->total+1] = '\0';
			uart_putc(data);
			cli->total++;
			if(cli->total >= (CLI_MAX_CHARS -1))
			{
				cli->total=0;
				printf("\nCommand is too long!");
				CLI_PROMPT;
			}
			break;
		} // end of switch

    } // while
}

/*----------------------------------------------------------------------
* cli_show_help_line
*----------------------------------------------------------------------*/
static cli_show_help_line(CLI_CMD_T *cmd, int max_size)
{
	int i;
    char buf[31], *cp;
	
	if (cmd->help)
	{
		printf("\n  ");
		printf(cmd->name);
		cp=buf;
		*cp++=' ';
		for (i=0; i<max_size-strlen(cmd->name); i++)
			*cp++='.';
		*cp++=' ';
		*cp='\0';
		printf(buf);
		printf(cmd->help);
	}
}

/*----------------------------------------------------------------------
* cli_show_all_commands
*----------------------------------------------------------------------*/
void cli_show_all_commands(CLI_CMD_T *current_cmd)
{
    int i, max_len=0;
    char buf[31], *cp;

    if (current_cmd->name)
    {
       CLI_CMD_T *cmd_p=current_cmd;
       while(cmd_p->name)
       {
          if (strlen(cmd_p->name) > max_len)
             max_len = strlen(cmd_p->name);
            cmd_p++;
       }
       max_len+=5;

       printf("\nCommands:");
       while(current_cmd->name)
       {
			cli_show_help_line(current_cmd, max_len);
			current_cmd++;
       }
    }
}

/*----------------------------------------------------------------------
* cli_show_more_cmds
*----------------------------------------------------------------------*/
static void cli_show_more_cmds(CLI_CMD_T **start_cmd, int total)
{
    int i, j, max_len=0;
    CLI_CMD_T *cmd;

    for (i=0; i<total; i++)
    {
		cmd = start_cmd[i];
       if (strlen(cmd->name) > max_len)
           max_len = strlen(cmd->name);
    }

    printf("\nMore commands:");
    for (i=0; i<total; i++)
    {
		cmd = start_cmd[i];
		cli_show_help_line(cmd, max_len);
    }
    printf("\n");
}

/*----------------------------------------------------------------------
* cli_parse_enter
*----------------------------------------------------------------------*/
static void cli_parse_enter(CLI_INFO_T *cli)
{
    CLI_CMD_T	*current_cmd=cli->cmd_p;
    CLI_CMD_T	*old_cmd_p;
    char		*datap=&cli->buf[0], *prev_p;
    char		name[CLI_NAME_SIZE];
    CLI_CMD_T	*matched_cmds[20];
    int			matched_items=0, total_names=0,i;

    while (*datap==' ')
		datap++;
    prev_p=datap;
    while (datap=cli_get_name(datap,&name[0]))
    {
		total_names++;
		/* search matched commands */
		old_cmd_p=current_cmd;
		matched_items=cli_get_matched_cmd(cli,
										  current_cmd,
                                          &name[0],
                                          (CLI_CMD_T **)matched_cmds);
		if (matched_items==0)
		{
			printf("\nIllegal Command!");
			cli_show_all_commands(old_cmd_p);
			return;
		} 
 		else if (matched_items>1)
		{
			cli_show_more_cmds((CLI_CMD_T **)matched_cmds, matched_items);
			return;
		}
		else
		{
			old_cmd_p=current_cmd;
			current_cmd=matched_cmds[0];
			if (current_cmd->next==0)
			{
				if (current_cmd->handler)
				{
					cli_parse_argument(cli, prev_p);
                    printf("\n");
                    current_cmd->handler(cli->argc, &cli->argv[0]);
				}
				return;
			}
			else
			{
				old_cmd_p=current_cmd;
				current_cmd=current_cmd->next;
				matched_items=0;
			}
		}
		prev_p=datap;
    } /* end while (cli_get_name) */

    if (total_names)
    {
		if (old_cmd_p->next)  /* input is not completed */
		{
			if (old_cmd_p->handler)
			{
				cli_parse_argument(cli, prev_p);
				printf("\n");
				old_cmd_p->handler(cli->argc, &cli->argv[0]);
				// return;
			}
			else
			{
				printf("\n");
				cli_show_all_commands(old_cmd_p->next);
			}
		}
		else
		{
			printf("\n  Illegal argument");
		}
	}
}

/*----------------------------------------------------------------------
* cli_parse_argument
*----------------------------------------------------------------------*/
static void cli_parse_argument(CLI_INFO_T  *cli, char *datap)
{
    cli->argc=0;
    cli->argv[0]=datap;
    do
    {
       if (*datap!=0x00)
       {
           cli->argv[cli->argc]=datap;
           cli->argc++;
       }
    } while (datap=cli_get_argument_ptr(datap));
}

/*----------------------------------------------------------------------
* cli_get_name
*----------------------------------------------------------------------*/
static char *cli_get_name(char *in, char *name)
{
     UINT32 found=0;

     while (*in != '\0' && *in != ' ' && found < (CLI_NAME_SIZE - 1)) {
           *name++ = toupper(*in);
           in++;
           found++;
     }

     while (*in==' ')
           in++;
     if (found) {
        *name = '\0';
        return(in);
     }
     else
        return(0);
}

/*----------------------------------------------------------------------
* cli_get_argument_ptr
*----------------------------------------------------------------------*/
static char *cli_get_argument_ptr(char *in)
{
     UINT32 found=0;
     UINT32 double_q=FALSE;

     while (1)
     {
          char c=*in;
          if (c == '\0' || (c==' ' && (double_q==FALSE))) break;
          if (c=='"') double_q ^= 1;
          in++;
          found++;
     }

     if (*in==' ') *in++=0x00;

     while (*in==' ')
           in++;
     if (found)
     {
        return(in);
     }
     else
        return(0);
}

/*----------------------------------------------------------------------
* cli_get_matched_cmd
*----------------------------------------------------------------------*/
static int cli_get_matched_cmd(CLI_INFO_T  *cli, CLI_CMD_T *current_cmd,
                          char *name, CLI_CMD_T **matched_cmds)
{
     int matched_items=0;
     /* comapre command table */
     while(current_cmd->name)
     {
          if(cli_bcmp(name,current_cmd->name, strlen((char *)name))==TRUE)
          {
            matched_cmds[matched_items++]=current_cmd;
          }
          current_cmd++;
     }

     return(matched_items);
}

/*----------------------------------------------------------------------
* cli_cursor_left
*----------------------------------------------------------------------*/
static void cli_cursor_left(char col)
{
    while (col--)
    {
    	uart_putc('\b');
    	uart_putc(' ');
    	uart_putc('\b');
    }
}

/*----------------------------------------------------------------------
* cli_bcmp
*----------------------------------------------------------------------*/
static int cli_bcmp(char *str1, char *str2, int len)
{
	for (; len !=0; len--)
	{
		char d = toupper(*str1);
		char s = toupper(*str2);
		if (d!=s)
			return FALSE;
			
		str1++; str2++;
	}
	return TRUE;
}


/*----------------------------------------------------------------------
* cli_exit
*----------------------------------------------------------------------*/
void cli_exit(char argc, char *argv[])
{
	cli_exit_flag = 1;
}
