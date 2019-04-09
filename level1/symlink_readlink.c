/**** symlink_readline.c file ****/
#ifndef SYMLINK_READLINK_C
#define SYMLINK_READLINK_C

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include "../type.h"
#include "../alloc_dealloc.c"
#include "mkdir_creat.c"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

int fssymlink(char *oldpathname, char *newpathname)
{
    int oino = getino(oldpathname);
    if (!oino) {
        printf("%s not found\n", oldpathname);
        return;
    }
    MINODE *omip = iget(dev, oino);
    INODE *oip = &omip->INODE;
    if (!(S_ISDIR(oip->i_mode) || S_ISREG(oip->i_mode))) {
        printf("%s must be a file or directory\n");
        return;
    }

    int nino = getino(newpathname);
    if (nino) {
        printf("%s already exists\n", newpathname);
        return;
    }

    nino = make_entry(newpathname, 0);
    MINODE *nmip = iget(dev, nino);
    INODE *ip = &nmip->INODE;
    ip->i_mode = 0xA1A4; /* change to LNK type */
    ip->i_size = (u32) strlen(oldpathname);
    // store oldfile in newfile's i_block[] area
    strcpy((char *) ip->i_block, oldpathname);
    nmip->dirty = 1;
    iput(nmip);
    iput(omip);
}

int fsreadlink(char *pathname, char *buf)
{
    int ino = getino(pathname);
    if (!ino) {
        printf("%s not found\n", pathname);
        return;
    }
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    if (!S_ISLNK(ip->i_mode)) {
        printf("%s is not a symlink file\n", pathname);
        return -1;
    }
    int len = ip->i_size;
    strcpy(buf, (char *) ip->i_block);
    printf("readlink: %s\n", buf);
    iput(mip);
    return len;
}

#endif
