/**** link_unlink.c file ****/
#ifndef LINK_UNLINK_C
#define LINK_UNLINK_C

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

int fslink(char *oldpathname, char *newpathname)
{
    int ino = getino(oldpathname);
    if (!ino) {
        printf("%s not found\n", pathname);
        return;
    }
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    if (!(S_ISREG(ip->i_mode) || S_ISLNK(ip->i_mode))) {
        printf("%s must be a regular or link file\n", pathname);
        return;
    }

    char *dirc, *basec, *bname, *dname;
    dirc = strdup(newpathname);
    basec = strdup(newpathname);
    dname = dirname(dirc);
    bname = basename(basec);
    printf("dirname=%s basename=%s\n", dname, bname);

    int nino = getino(dname);
    if (!nino) {
        printf("parent directory %s does not exist\n", dname);
        return;
    }
    /* Ensure new pathname doesn't exist */
    MINODE *pmip = iget(mip->dev, nino);
    if (search(pmip, bname)) {
        printf("%s already exists in %s\n", bname, dname);
        return;
    }
    /* Check if on the same device */
    if (mip->dev != pmip->dev) {
        printf("dev (%d) of %s != dev (%d) of %s",
         mip->dev, oldpathname, pmip->dev, newpathname);
        return;
    }

    /* Add entry newpathname to parent directory */
    enter_name(pmip, ino, bname, 0);
    iput(pmip);

    /* Increment links count of original file */
    (&mip->INODE)->i_links_count++;
    mip->dirty = 1;
    iput(mip);
}

int fsunlink(char *pathname)
{
    int ino = getino(pathname);
    if (!ino) {
        printf("%s not found\n", pathname);
        return;
    }
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    if (!(S_ISREG(ip->i_mode) || S_ISLNK(ip->i_mode))) {
        printf("%s must be a regular or link file\n", pathname);
        return;
    }

    char *dirc, *basec, *bname, *dname;
    dirc = strdup(pathname);
    basec = strdup(pathname);
    dname = dirname(dirc);
    bname = basename(basec);
    printf("dirname=%s basename=%s\n", dname, bname);
    
    ip->i_links_count--;
    if (ip->i_links_count > 0) {
        mip->dirty = 1;
    } else {
        /* Dealloc all data blocks in INODE, dealloc INODE */
        itruncate(mip);
        idalloc(mip->dev, ino);
    }
    iput(mip);

    /* Remove entry from parent directory */
    int pino = getino(dname);
    MINODE *pmip = iget(mip->dev, pino);
    rm_child(pmip, bname);
    iput(pmip);
}

#endif
