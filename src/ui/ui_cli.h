/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_cli.h
* Description	: 
*		Define CLI data
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/22/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _UI_CLI_H
#define _UI_CLI_H

// define command structure
typedef struct _cmd_t {
    char	      	*name; 						// command name
	struct _cmd_t	*next;						// link to next sub-commands
    void	      	(*handler)(int, char **);	// handler routine
    char	      	*help;	  					// help message, NULL if invisible
} CLI_CMD_T;

/* Define key */
#define KEY_BS			0x08
#define KEY_TAB 		0x09
#define KEY_LF			0x0a
#define KEY_CR			0x0d
#define KEY_ESC 		0x1b
#define KEY_SP			0x20
#define KEY_HELP		0x3f	/* ? */

#define ARROW_U			0xf0
#define ARROW_D			0xf1
#define ARROW_R			0xf2
#define ARROW_L			0xf3

// arrow key
#define KEY_CTRL_U		21
#define KEY_CTRL_D		4
#define KEY_CTRL_N		14

/* command interface definitions */
#define CLI_MAX_HISTORY		10		// max stored commands
#define CLI_MAX_CHARS		80		// max chars per line
#define CLI_NAME_SIZE		16   	// max command name size
#define CLI_MAX_ARGUMENTS	8

typedef struct {
	char	index;
	char	total;
	char	buf[CLI_MAX_HISTORY][CLI_MAX_CHARS];
} CLI_HISTORY_T;

/* define data used by console port or telnet port */
typedef struct cli_cmd_t {
     char			total; 						// total input chars
     CLI_CMD_T		*cmd_p; 					// points to command line table
     char			argc;						// number of arguments
     char			*argv[CLI_MAX_ARGUMENTS];	// points arguments
     char			buf[CLI_MAX_CHARS+1];		// input buffer
     CLI_HISTORY_T	history;
} CLI_INFO_T;

#endif /* _UI_CLI_H */
