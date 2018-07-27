//==========================================================================
//
//      e2fs.h
//
//      Second extended filesystem defines.
//
//==========================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//
// Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
// at http://sources.redhat.com/ecos/ecos-license/
// -------------------------------------------
//####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    msalter
// Contributors: msalter
// Date:         2001-06-25
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#ifndef _EXT2FS_H_
#define _EXT2FS_H_

/*----------------------------------------------------------------------
* super block
*----------------------------------------------------------------------*/
typedef struct ext2_sb_t {
	UINT32	inodes_count;
	UINT32	blocks_count;
	UINT32	r_blocks_count;
	UINT32	free_blocks_count;
	UINT32	free_inodes_count;
	UINT32	first_data_block;
	UINT32	log_block_size;
	INT32 	log_frag_size;
	UINT32	blocks_per_group;
	UINT32	frags_per_group;
	UINT32	inodes_per_group;
	UINT32	mtime;
	UINT32	wtime;
	UINT16	mnt_count;
	INT16 	max_mnt_count;
	UINT16	magic;
	UINT16	state;
	UINT16	errors;
	UINT16	minor_rev_level;
	UINT32	lastcheck;
	UINT32	checkinterval;
	UINT32	creator_os;
	UINT32	rev_level;
	UINT16	def_resuid;
	UINT16	def_resgid;
} EXT2_SB_T;

#define EXT2_PRE_02B_MAGIC			0xEF51
#define EXT2_SUPER_MAGIC			0xEF53

#define EXT2_PTRS_PER_BLOCK(e)		((e)->blocksize / sizeof(UINT32))

#define EXT2_MIN_BLOCK_SIZE			1024
#define	EXT2_MAX_BLOCK_SIZE			4096
#define EXT2_BLOCK_SIZE(s)			(EXT2_MIN_BLOCK_SIZE << SWAB_LE32((s)->log_block_size))
#define	EXT2_ADDR_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof(unsigned int))
#define EXT2_BLOCK_SIZE_BITS(s)		(SWAB_LE32((s)->log_block_size) + 10)
#define	EXT2_NR_DIR_BLOCKS			12

#define	EXT2_IND_BLOCK				EXT2_NR_DIR_BLOCKS
#define	EXT2_DIND_BLOCK				(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK				(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS				(EXT2_TIND_BLOCK + 1)

/*----------------------------------------------------------------------
* inode
*----------------------------------------------------------------------*/
typedef struct ext2_inode_t {
    UINT16   mode;
    UINT16   uid;
    UINT32   size;
    UINT32   atime;
    UINT32   ctime;
    UINT32   mtime;
    UINT32   dtime;
    UINT16   gid;
    UINT16   links_count;
    UINT32   blocks;
    UINT32   flags;
    UINT32   reserved1;
    UINT32   block[EXT2_N_BLOCKS];
    UINT32   version;
    UINT32   file_acl;
    UINT32   dir_acl;
    UINT32   faddr;
    UINT8    frag;
    UINT8    fsize;
    UINT16   pad1;
    UINT32   reserved2[2];
} EXT2_INODE_T;


#define	EXT2_INODES_PER_BLOCK(e)	((e)->blocksize / sizeof(EXT2_INODE_T))


// Special inode numbers
//
#define	EXT2_BAD_INO		 1
#define EXT2_ROOT_INO		 2

/*----------------------------------------------------------------------
* inode
*----------------------------------------------------------------------*/
typedef struct  ext2_dir_t{
    UINT32		inode;
    UINT16		reclen;
    UINT8		namelen;
    UINT8		filetype;
    char		name[2];
} EXT2_DIR_T;

#define EXT2_FTYPE_UNKNOWN  0
#define EXT2_FTYPE_REG_FILE 1
#define EXT2_FTYPE_DIR      2
#define EXT2_FTYPE_CHRDEV   3
#define EXT2_FTYPE_BLKDEV   4
#define EXT2_FTYPE_FIFO     5
#define EXT2_FTYPE_SOCK     6
#define EXT2_FTYPE_SYMLINK  7

typedef struct ext2_group_t
{
    UINT32 block_bitmap;	   // blocks bitmap block
    UINT32 inode_bitmap;	   // inodes bitmap block
    UINT32 inode_table;	   // inodes table block
    UINT16 free_blocks_count;
    UINT16 free_inodes_count;
    UINT16 used_dirs_count;
    UINT16 pad;
    UINT32 reserved[3];
} EXT2_GROUP_T;

#define EXT2_BLOCKS_PER_GROUP(s)  (SWAB_LE32((s)->blocks_per_group))
#define EXT2_INODES_PER_GROUP(s)  (SWAB_LE32((s)->inodes_per_group))

#define	EXT2_GDESC_PER_BLOCK(e)	  ((e)->blocksize / sizeof(EXT2_GROUP_T))
#define EXT2_GDESC_PER_SECTOR     (IDE_SECTOR_SIZE/sizeof(EXT2_GROUP_T))
#define EXT2_GDESC_CACHE_SIZE     (EXT2_GDESC_PER_SECTOR * 1)
#define EXT2_GDESC_PER_SECTOR     (IDE_SECTOR_SIZE/sizeof(EXT2_GROUP_T))

typedef struct ext2_desc_t {
    IDE_PART_T		*part;     			// partition holding this filesystem
    UINT32			blocksize;			// fs blocksize
    UINT32			ngroups;			// number of groups in fs
    UINT32			blocks_per_group;
    UINT32			inodes_per_group;
    UINT32			gdesc_block;		// block nr of group descriptors
    INT32			gdesc_first;		// which gdesc is first in cache
    EXT2_GROUP_T	gdesc_cache[EXT2_GDESC_CACHE_SIZE];
    UINT32   		nr_ind_blocks;
    UINT32   		nr_dind_blocks;
    UINT32   		nr_tind_blocks;
} EXT2_DESC_T;

#define EXT2_BLOCK_TO_SECTOR(e,b)  ((b) * ((e)->blocksize / IDE_SECTOR_SIZE))

typedef struct inode_info_t {
    UINT32		ino;
    UINT32		parent_ino;
    UINT8		filetype;
} INODE_INFO_T;

typedef struct {
	int				opened;
	EXT2_DESC_T		*ext2fs_desc;
    EXT2_INODE_T	inode;
    INODE_INFO_T	inode_info;
    UINT32   		file_size;
    UINT32   		file_pos;
} FILE_T;

extern FILE_T *kernel_fp;
extern FILE_T *initrd_fp;

#endif  // _EXT2FS_H_
