/**** type.h file ****/
#ifndef TYPE_H
#define TYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>

/*************** type.h file ************************/
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc GD;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER* sp;
GD* gp;
INODE* ip;
DIR* dp;

#define FREE 0
#define READY 1

#define BLKSIZE 1024
#define NMINODE 64
#define NFD 8
#define NPROC 2

typedef struct minode {
    INODE INODE;
    int dev, ino;
    int refCount;
    int dirty;
    // for level-3
    int mounted;
    struct mntable* mptr;
} MINODE;

typedef struct oft {
    int mode;
    int refCount;
    MINODE* mptr;
    int offset;
} OFT;

typedef struct proc {
    struct proc* next;
    int pid;
    int uid;
    int status;
    MINODE* cwd;
    OFT* fd[NFD];
} PROC;

#endif
