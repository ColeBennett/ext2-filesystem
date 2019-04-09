/**** misc1.c file ****/
#ifndef MISC1_C
#define MISC1_C

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

/*
chmod filename mode: (mode = |rwx|rwx|rwx|, e.g. 0644 = in octal 110|100|100)
*/
int fschmod(char *pathname)
{
    int ino = getino(pathname);
    if (!ino) {
        printf("%s not found\n", pathname);
        return;
    }
    MINODE *mip = iget(dev, ino);
    u16 mode;
    printf("Enter mode (in octal): ");
    scanf("%o", &mode);
    printf("old mode: %o\n", (&mip->INODE)->i_mode); 
    (&mip->INODE)->i_mode |= mode;
    printf("new mode: %o\n", (&mip->INODE)->i_mode);
    mip->dirty = 1;
    iput(mip);
    printf("changed file mode to %o\n", mode);
}

int fstouch(char *pathname)
{
    int ino = getino(pathname);
    if (ino) {
        update_atime(ino);
    } else {
        creat_file(pathname);
    }
}

/* Change file's access time to current time. */
int fsutime(char *pathname)
{
    int ino = getino(pathname);
    if (ino) {
        update_atime(ino);
    } else {
        printf("%s not found\n", pathname);
    }
}

void update_atime(int ino)
{
    MINODE *mip = iget(dev, ino);
    (&mip->INODE)->i_atime = (u32) time(0L);
    mip->dirty = 1;
    iput(mip);
    printf("updated file access time\n");
}

#endif
