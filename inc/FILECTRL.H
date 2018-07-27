/***************************************************************************
* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
*--------------------------------------------------------------------------*
* Name:filectrl.h                                                          *
* Description: FAT relative routines                                       *
* Author: Ted Hsu 			                                               *
****************************************************************************/

#ifndef _FILECTRL_H
#define	_FILECTRL_H					1

#include <typedef.h>
#include <FAT.h>

/* constants */
#define STANDARD_FILE_SYS_HANDLE				0x0

#define MAX_FILE_SUPPORT						0x10
#define MAX_FILE_SYS_SUPPORT					26

#define HANDLE_OFFSET							0

#define Monitor_STDIN							0
#define Monitor_STDOUT							1
#define Monitor_STDERR							2

/* file open mode */
#define OPEN_FOR_READ							0
#define OPEN_BINARY_FOR_READ					1
#define OPEN_FOR_READ_WRITE						2
#define OPEN_BINARY_FOR_READ_WRITE				3

#define OPEN_EMPTY_FOR_WRITE					4
#define OPEN_BINARY_EMPTY_FOR_WRITE				5
#define OPEN_EMPTY_FOR_READ_WRITE				6
#define OPEN_BINARY_EMPTY_FOR_READ_WRITE		7

#define OPEN_FOR_APPEND							8
#define OPEN_BINARY_FOR_APPEND					9
#define OPEN_FOR_READ_APPEND					10
#define OPEN_BINARY_FOR_READ_APPEND				11


extern UINT32 fLib_DirDirectory(INT8 *Name, INT8 *Buf);
extern UINT32 fLib_ChangeDirectory(INT8 *Name);
extern UINT32 fLib_MakeDirectory(INT8 *Name);
extern UINT32 fLib_MakeEmptyFile(INT8 *Name);
extern UINT32 fLib_DeleteDirectory(INT8 *Name);

extern UINT32 fLib_OpenFile(INT8 *FileName, UINT32 OpenMode, UINT32 FileNameLen);
extern UINT32 fLib_CloseFile(UINT32 handle);
extern UINT32 fLib_DeleteFile(INT8 *FileName, UINT32 FileNameLen);
extern UINT32 fLib_RenameFile(INT8 *OldFileName, UINT32 OldFileNameLen, INT8 *NewFileName, UINT32 NewFileNameLen);
extern UINT32 fLib_FileLen(UINT32 handle);
extern UINT32 fLib_WriteFile(UINT32 handle, UINT8 *Buf, UINT32 Len);
extern UINT32 fLib_ReadFile(UINT32 handle, UINT8 *Buf, UINT32 Len);
extern UINT32 fLib_SeekFile(UINT32 handle, UINT32 Pos);
extern UINT32 fLib_IsTTYFile(UINT32 handle);
extern int fLib_InsertFileSystem(UINT32 Drive, FileSysStruct *FileSys);
extern void fLib_InitFileSystem(void);

extern void strupr(UINT8 *Str);
extern void SplitPath(INT8 *path, INT8 *dir, INT8 *fname);
extern int IsFullPath(INT8 *path);
extern int stricmp(UINT8 *Str1, UINT8 *Str2);

#endif
