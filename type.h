#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE           1024
#define ISIZE              128

#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Default dir and regulsr file modes
#define DIR_MODE          0040777 
#define FILE_MODE         0100644
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define BUSY              1
#define READY             2

// Table sizes
#define NMINODE          64
#define NMOUNT            4
#define NPROC             2
#define NFD              16
#define NOFT             32

// Open File Table
typedef struct Oft{
  int   mode;
  int   refCount;
  struct Minode *inodeptr;
  long  offset;
} OFT;

// PROC structure
typedef struct Proc{
  int   uid;
  int   pid;
  int   gid;
  int   status;

  struct Minode *cwd;
  OFT   *fd[NFD];

  struct Proc *next;
  struct Proc *parent;
  struct Proc *child;
  struct Proc *sibling;
} PROC;
      
// In-memory inodes structure
typedef struct Minode{		
  INODE INODE;               // disk inode
  int   dev, ino;

  int   refCount;
  int   dirty;
  int   mounted;
  struct Mount *mountptr;
} MINODE;

// Mount Table structure
typedef struct Mount{
        int    dev;   
        int    ninodes;
        int    nblocks;
        int    imap, bmap, iblk; 
        struct Minode *mounted_inode;
        char   name[256]; 
        char   mount_name[64];
} MOUNT;
