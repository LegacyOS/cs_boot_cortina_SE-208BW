#ifndef COMMAND_H
#define COMMAND_H					1

#include "fLib.h"

#define MAX_USER_SYMBOL_SIZE		0x20

#define IS_NULL						0x00
#define IS_NUM						0x01
#define IS_REG						0x02
#define IS_MEM						0x04
#define IS_USER_SYMBOL				0x08
#define IS_STR						0x10
#define IS_STR_WITH_OPTION			0x20
#define IS_ARITHMETIC_OP_STR		0x40
#define IS_LOGICAL_OP_STR			0x80

#define IS_BYTE						0
#define IS_HALFWORD					1
#define IS_WORD						2

enum {EQUAL_CMD, ADD_CMD, SUB_CMD, MUL_CMD, DIV_CMD, MOD_CMD};
enum {IS_HIGHER, IS_HIGHER_SAME, IS_EQUAL, IS_LOWER_SAME, IS_LOWER, IS_NOT_EQUAL};

enum {USER_SYMBOL_COMMAND, WHILE_COMMAND=0xF0, OK_COMMAND=0xFC, NEED_HELP_COMMAND=0xFD, IF_ERROR_COMMAND=0xFE, ERROR_COMMAND=0xFF};

#define MAX_COMMAND_LEN					0x100
#define MAX_COMMAND_NUM					(MAX_COMMAND_LEN / 2)
#define COMMAND_NAME_COUNT				3
#define COMMAND_ARGUMENT_COUNT			10
#define COMMAND_OPTION_COUNT			10
#define CMD_STR_SUB_OPTION_COUNT		10

#define OPTION_OFF						0
#define OPTION_ON						1
#define OPTION_CLOSE					2

typedef union
{
	INT8 Name;
	INT8 *Para;
}ParameterStruct_T;

typedef struct
{
	UINT32 Type;
	INT8 *Str;
	UINT32 Value;
	INT8 IsSet;
	INT8 *HelpStr;

}CmdArguStruct_T;

typedef struct
{
	INT8 *Name;
	UINT32 Type;
	INT8 *SubName[CMD_STR_SUB_OPTION_COUNT];
	INT8 *Str;
	UINT32 Value;
	INT8 IsSet;
	INT8 *HelpStr;
}CmdOptStruct_T;

typedef struct CommandTableStruct
{
	INT8		*Name[COMMAND_NAME_COUNT];
	INT8 		*HelpStr;
	UINT32		ArguCount;
	UINT32		OptCount;
	UINT32		ArguMaxCount;
	UINT32		ArguMinCount;
	UINT32		OptMaxCount;
	UINT32		OptMinCount;
	UINT32		(*pf)(struct CommandTableStruct *CmdTable);
	UINT32		Key;			/* when more than one command call the same
										(*pf)(UINT32 CmdIndex, INT8 *ResultStr);
										it is used to indicate which command */
	UINT32		KeyValue;		/* it is used to save a value for key */

	CmdArguStruct_T Argu[COMMAND_ARGUMENT_COUNT];

	CmdOptStruct_T Opt[COMMAND_OPTION_COUNT];
	
}CommandTableStruct_T;


typedef struct UserSymbol_T
{
	INT8	SymName[MAX_COMMAND_LEN];
	UINT32	SymValue;
	UINT32	IsFree;
}UserSymbolStruct;

extern UINT32 fLib_InsertCommand(CommandTableStruct_T CmdTable[]);
extern UINT32 fLib_ExecuteCommand(INT8 *CommandStr);
extern void fLib_InitUserSymbol(UserSymbolStruct symbol[]);

extern BOOL IsCommandBreak(void);
extern void SaveCmdToHistory(INT8 *cmd);
extern INT8 *GetCmdFromHistory(UINT32 UpDown);
extern void RemoveCurrCmdFromHistory(void);
extern void ClearCmdHistory(void);

extern UINT32 NewUserSymbol(INT8 *str);
extern UINT32 DeleteUserSymbol(UINT32 SymbolIndex);
extern INT8 *ReadUserSymbol(UINT32 SymIndex, UINT32 *SymValue);
extern void WriteUserSymbol(UINT32 SymIndex, UINT32 NewValue);
extern void DeleteAllUserSymbol(void);

extern UINT32 ParameterIsUserSymbol(INT8 *str, UINT32 *SymIndex, UINT32 *SymValue);

extern UINT32 ParameterIsLogicalOperation(INT8 *str, UINT32 *ul);
extern UINT32 ParameterIsArithmeticOperation(INT8 *str, UINT32 *ul);
extern UINT32 ParameterIsConstant(INT8 *str, UINT32 *ul);
extern UINT32 ParameterIsMemory(INT8 *str, UINT32 *MemValue);

extern UINT32 ParameterToNumber(INT8 *para, UINT32 para_type, UINT32 *value);
extern UINT32 ParseCommandParameter(INT8 *cmd, UINT32 index, CommandTableStruct_T CmdTable[]);
extern UINT32 SaveArgument(UINT32 index, INT8 *ArgumentStr, CommandTableStruct_T CmdTable[]);
extern UINT32 FindOption(UINT32 index, INT8 *OptionName, CommandTableStruct_T CmdTable[]);
extern UINT32 FineSubOption(INT8 *Str, INT8 *SubOption[]);
extern void FreeCmdArgumentOption(UINT32 index, CommandTableStruct_T CmdTable[]);

extern UINT32 ParseCommand(INT8 *cmd, CommandTableStruct_T CommandTable[]);

extern UINT32 HelpCmd(CommandTableStruct_T *CmdTable);
extern UINT32 IfCmd(CommandTableStruct_T *CmdTable);
extern UINT32 WhileCmd(CommandTableStruct_T *CmdTable);
extern UINT32 SymbolCmd(CommandTableStruct_T *CmdTable);
extern UINT32 UserSymbolCmd(CommandTableStruct_T *CmdTable);

#endif

