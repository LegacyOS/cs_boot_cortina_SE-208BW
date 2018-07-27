/***************************************************************************
* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
*--------------------------------------------------------------------------*
* Name:fat.h                                                               *
* Description: FAT relative routines                                       *
* Author: Ted Hsu 			                                               *
****************************************************************************/

#ifndef __FAT_H	
#define __FAT_H								1

#include "symbol.h"

#define MAX_DIRECTORY_PATH_SIZE					0x100
#define MAX_LONG_FILE_NAME_SIZE					0x80

/* the sector offset */
#define PARTITION_AREA_SECTOR					0


/* the offset of Master Boot Record and Partition Table */
#define MASTER_BOOT_RECORD_OFFSET				0
#define PARTITION_TABLE0_OFFSET					446
#define PARTITION_TABLE1_OFFSET					462
#define PARTITION_TABLE2_OFFSET					478
#define PARTITION_TABLE3_OFFSET					494
#define PARTITION_AREA_SIGNATURE_OFFSET			510


/* partition table structure */
typedef struct
{
	UINT32 BootIndicator:8;	
	UINT32 StartingHead:8;
	UINT32 StartingSector:6;
	UINT32 StartingCylinder:10;
	
	UINT32 SystemID:8;
	UINT32 EndingHead:8;
	UINT32 EndingSector:6;
	UINT32 EndingCylinder:10;
	
	UINT32 RelativeSector;
	UINT32 TotalSector;
}PartitionTableStruct;

/* FDC Description structure */
typedef struct
{
	UINT8 JumpCommand[3];
	UINT8 SystemIdentifier[8];
	UINT16 SectorSize;
	UINT8 SectorPerCluster;
	UINT16 ReservedSectorCount;
	UINT8 FATNumber;
	UINT16 RootDirectoryEntryNumber;
	UINT16 TotalSector;
	UINT8 MediumIdentifier;
	UINT16 SectorPerFAT;
	UINT16 SectorPerTrack;
	UINT16 SlideNumber;
	UINT16 FutureReserved;
	UINT8 Reserved0[480];
	UINT16 SignatureWord;	
}FDCDescriptorStruct;

/* Extended FDC Description structure */
typedef struct
{
	UINT8 JumpCommand[3];
	UINT8 SystemIdentifier[8];
	UINT16 SectorSize;
	UINT8 SectorPerCluster;
	UINT16 ReservedSectorCount;
	UINT8 FATNumber;
	UINT16 RootDirEntryNumber;
	UINT16 TotalSector;
	UINT8 MediumIdentifier;
	UINT16 SectorPerFAT;
	UINT16 SectorPerTrack;
	UINT16 SlideNumber;
	UINT32 HiddenSectorNumber;
	UINT32 TotalSector2;
	UINT8 PhyDiskNumber;
	UINT8 Reserved0;
	UINT8 ExtendSignature;
	UINT32 VolumeID;
	UINT8 VolumeLable[11];
	UINT8 FileSystemType[8];
	UINT8 Reserved1[448];
	UINT16 SignatureWord;	
}ExtendFDCDescriptorStruct;


/* FAT constants */
#define FAT_CLUSTER_NOT_USED				0x000
#define FAT_CLUSTER_ALLOCATED				0x002
#define FAT_CLUSTER_RESERVED				0xFF6
#define FAT_CLUSTER_DEFECTIVE				0xFF7
#define FAT_CLUSTER_FINAL					0xFF8


#define ROOT_DIRECTORY_ENTRY_NUMBER			512

/* directory entry structure */
#define ATTRIB_READ_ONLY					0x01
#define ATTRIB_HIDDEN						0x02
#define ATTRIB_SYSTEM						0x04
#define ATTRIB_VOLUME_ID					0x08
#define ATTRIB_DIRECTORY					0x10
#define ATTRIB_ARCHIVE						0x20
#define ATTRIB_LONG_NAME					0x0F

typedef struct
{
	UINT8 Name[8];
	UINT8 Extension[3];
	UINT8 Attrib;
	UINT8 Reserved[10];
	UINT16 Time;
	UINT16 Date;
	UINT16 StartCluster;
	UINT32 FileLength;
}DirEntryStruct;

#if 0
typedef struct
{
	UINT16 Sec:5;
	UINT16 Min:6;
	UINT16 Hour:5;
}DirEntryTimeStruct;
#else
typedef UINT16 DirEntryTimeStruct;
#endif	

#define FAT_GET_SEC(x)		(x & 0x1F)
#define FAT_GET_MIN(x)		((x >> 5) & 0x3F)
#define FAT_GET_HOUR(x)		((x>> 11) & 0x1F)

#if 0
typedef struct
{
	UINT16 Day:5;
	UINT16 Month:4;
	UINT16 Year:7;
}DirEntryDateStruct;
#else
typedef UINT16 DirEntryDateStruct;
#endif

#define FAT_GET_DAY(x)		(x & 0x1F)
#define FAT_GET_MONTH(x)	((x >> 5) & 0x0F)
#define FAT_GET_YEAR(x)		((x>> 9) & 0x7F)

typedef struct
{
	INT8 Name[0x10];
	INT8 LongName[MAX_LONG_FILE_NAME_SIZE];
	UINT8 Attrib;
	DirEntryTimeStruct Time;
	DirEntryDateStruct Date;
	UINT16 StartCluster;
	UINT32 Length;
}DirEntryInfoStruct;

#define FAT_TYPE_FAT12						0
#define FAT_TYPE_FAT16						1
#define FAT_TYPE_FAT32						2
#define FAT_TYPE_NOT_SUPPORT				3

/*****************************************************************/
/*****************************************************************/
typedef struct
{
	UINT8 *FATTable;
	int	FATType;
	UINT32 FATSector;
	UINT32 MaxClusterNumber;
}FATStruct;

typedef struct
{
	DirEntryStruct *Entry;
	UINT32 Cluster;
	UINT32 Sector;
	UINT32 SectorCount;
	INT8 DirName[MAX_DIRECTORY_PATH_SIZE];
}DirectoryStruct;

typedef int (*INIT_CARD)(void *CardPtr);
typedef int (*READ_SECTOR)(void *CardPtr, UINT32 StartSector, UINT32 SectorCount, UINT32 SectorSize, UINT8 *Buf);
typedef int (*WRITE_SECTOR)(void *CardPtr, UINT32 StartSector, UINT32 SectorCount, UINT32 SectorSize, UINT8 *Buf);
typedef int (*ERASE_SECTOR)(void *CardPtr, UINT32 StartSector, UINT32 SectorCount, UINT32 SectorSize);

typedef struct
{
	/* FAT struct */
	FATStruct FAT;
	
	PartitionTableStruct PartitionTable;
	ExtendFDCDescriptorStruct FDCDesc;
	UINT32 ClusterSize;
	
	/* for current directory */
	UINT32 FirstClusterSector;
	
	/* for root directory */
	UINT32 RootDirCluster;
	UINT32 RootDirSector;
	UINT32 RootDirSectorCount;
	
	/* current dir struct */
	DirectoryStruct CurrDir;
	
	/* pointers */
	void *CardPtr;
	INIT_CARD InitCard;
	READ_SECTOR ReadSector;
	WRITE_SECTOR WriteSector;
	ERASE_SECTOR EraseSector;
	
	BOOL init;
}FileSysStruct;


typedef struct
{
	FileSysStruct *FileSys;
	
	UINT32 Drive;
	INT8 FileName[MAX_DIRECTORY_PATH_SIZE];
	UINT32 IsTTY;
	UINT32 StartCluster;
	UINT32 Attrib;
	UINT32 FileTotalSize;
	
	UINT32 CurrCluster;
	UINT8 *ClusterBuf;
	UINT32 ClusterBufSize;
	UINT32 ClusterBufOffset;
	
	UINT32 Pos;	
	UINT32 Dirty;	
}FileHandleStruct;

/***************************************************************************/
/***************************************************************************/

extern int OpenFATFile(FileSysStruct *FileSys, FileHandleStruct *fd, INT8 *FullFileName);
extern int OpenFATEmptyFile(FileSysStruct *FileSys, FileHandleStruct *fd, INT8 *FullFileName);
extern int CloseFATFile(FileSysStruct *FileSys, FileHandleStruct *fd);
extern int DeleteFATFile(FileSysStruct *FileSys, INT8 *FullFileName);
extern int WriteFATFile(FileSysStruct *FileSys, FileHandleStruct *fd, UINT32 Len, UINT8 *Buf);
extern int ReadFATFile(FileSysStruct *FileSys, FileHandleStruct *fd, UINT32 Len, UINT8 *Buf);

extern int ReadFATPartitionTable(FileSysStruct *FileSys);
extern int ReadFATPartitionBootSector(FileSysStruct *FileSys);
extern int ReadFATDirectoryEntry(FileSysStruct *FileSys, DirectoryStruct *Directory, UINT32 Cluster, UINT32 SectorCount, INT8 *DirName);
extern int WriteFATDirectoryEntry(FileSysStruct *FileSys, DirectoryStruct *Directory);
extern void FreeFATDirectoryEntry(DirectoryStruct *Directory);

extern int DeleteFATNextDirectory(FileSysStruct *FileSys, DirectoryStruct *Directory, INT8 *DirName);
extern int CreateFATNextEmptyFile(FileSysStruct *FileSys, DirectoryStruct *Directory, DirEntryInfoStruct *DirEntryInfo);
extern int MakeFATNextDirectory(FileSysStruct *FileSys, DirectoryStruct *Directory, DirEntryInfoStruct *DirEntryInfo);

extern int ReadFAT(FileSysStruct *FileSys);

extern int EraseFATCluster(FileSysStruct *FileSys, int ClusterNum);
extern UINT32 GetFATDirSectorNumber(FileSysStruct *FileSys, int ClusterNumber);

/***************************************************************************/
/***************************************************************************/
extern int GetFATType(UINT8 *FileSysType);
extern UINT32 GetFATMaxClusterNumber(UINT32 FATSize, int FATType);

extern DirEntryStruct *GetFirstDirEntry(DirEntryStruct *DirEntry);
extern DirEntryStruct *GetNextDirEntry(DirEntryStruct *DirEntry);
extern DirEntryStruct *GetFirstFreeDirEntry(DirEntryStruct *DirEntry, int EntryCount, UINT8 Attrib);
extern DirEntryStruct *IsFileExistInDirectory(DirEntryStruct *DirEntry, INT8 *FileName);
extern DirEntryStruct *IsDirectoryExistInDirectory(DirEntryStruct *DirEntry, INT8 *DirName);
extern void GetDirEntryInfo(DirEntryStruct *DirEntry, DirEntryInfoStruct *DirInfo);
extern void SetDirEntryInfo(DirEntryStruct *DirEntry, DirEntryInfoStruct *DirInfo);

extern void GetDirEntryFileName(DirEntryStruct *DirEntry, INT8 *FileName);
extern void SetDirEntryFileName(DirEntryStruct *DirEntry, INT8 *FileName);
extern UINT8 GetDirEntryAttrib(DirEntryStruct *DirEntry);
extern void SetDirEntryAttrib(DirEntryStruct *DirEntry, UINT8 Attrib);
extern UINT16 GetDirEntryStartCluster(DirEntryStruct *DirEntry);
extern void SetDirEntryStartCluster(DirEntryStruct *DirEntry, UINT16 StartCluster);
extern UINT32 GetDirEntryFileLength(DirEntryStruct *DirEntry);
extern void SetDirEntryFileLength(DirEntryStruct *DirEntry, UINT32 FileLength);
extern DirEntryDateStruct GetDirEntryDate(DirEntryStruct *DirEntry);
extern void SetDirEntryDate(DirEntryStruct *DirEntry, DirEntryDateStruct Date);
extern DirEntryTimeStruct GetDirEntryTime(DirEntryStruct *DirEntry);
extern void SetDirEntryTime(DirEntryStruct *DirEntry, DirEntryTimeStruct Time);
extern void GetDirEntryLongFileName(DirEntryStruct *DirEntry, INT8 *LongFileName);
extern void SetDirEntryLongFileName(DirEntryStruct *DirEntry, INT8 *LongFileName);

extern DirEntryStruct *OpenFATDirectory(FileSysStruct *FileSys, INT8 *DirStr, DirectoryStruct *Directory);
extern void CloseFATDirectory(FileSysStruct *FileSys, DirectoryStruct *Directory);

/***************************************************************************/
/***************************************************************************/
extern int DisplayFATInfo(FileSysStruct *FileSys, INT8 *InfoStr);
extern int DisplayDirEntryInfo(FileSysStruct *FileSys, DirEntryStruct *DirEntry, INT8 *InfoStr);
extern int ChangeFATDirectory(FileSysStruct *FileSys, INT8 *DirStr);
extern int MakeFATDirectory(FileSysStruct *FileSys, INT8 *FullDirName);
extern int CreateFATEmptyFile(FileSysStruct *FileSys, INT8 *FullFileName);
extern int DeleteFATDirectory(FileSysStruct *FileSys, UINT8 *FullDirName);
extern int RenameFATDirectoryFile(FileSysStruct *FileSys, INT8 *OldName, INT8 *NewName);
extern void fLib_InitFATFileSystemRoutines(FileSysStruct *FileSys, void *CardPtr, INIT_CARD InitCardPtr, READ_SECTOR ReadSectorPtr, WRITE_SECTOR WriteSectorPtr, ERASE_SECTOR EraseSectorPtr);
extern int fLib_InitFATFileSystem(FileSysStruct *FileSys);
extern int fLib_FreeFATFileSystem(FileSysStruct *FileSys);
extern void fLib_RemoveFileSystem(UINT32 Drive);
extern void GetCurrentDirectory(FileSysStruct *FileSys, INT8 *DirStr);
extern int SeekFATFile(FileSysStruct *FileSys, FileHandleStruct *fd, UINT32 Pos);



#endif