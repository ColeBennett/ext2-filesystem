/**
 * Cole Bennett
 * CptS 360
 * Lab 7
 * March 27, 2019
*/

/**** main.c file ****/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

MINODE minode[NMINODE];
MINODE* root;
PROC proc[NPROC], *running;

char gpath[256]; // global for tokenized components
char *name[64]; // assume at most 64 components in pathname
int n; // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256];

#include "util.c"
#include "cd_ls_pwd.c"

int init()
{
    int i, j;
    MINODE* mip;
    PROC* p;

    printf("init()\n");

    for (i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mptr = 0;
    }
    for (i = 0; i < NPROC; i++) {
        p = &proc[i];
        p->pid = i;
        p->uid = 0;
        p->cwd = 0;
        p->status = FREE;
        for (j = 0; j < NFD; j++)
            p->fd[j] = 0;
    }

    proc[0].uid = 0;
    proc[1].uid = 1;
    root = 0;
}

// load root INODE and set root pointer to it
int mount_root()
{
    printf("mount_root()\n");
    root = iget(dev, 2);
}

int quit()
{
    int i;
    MINODE *mip;
    for (i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        if (mip->refCount > 0 && mip->dirty)
            iput(mip);
    }
    exit(0);
}

char* disk = "mydisk";
int main(int argc, char* argv[])
{
    int ino;
    char buf[BLKSIZE];
    if (argc > 1)
        disk = argv[1];

    if ((fd = open(disk, O_RDWR)) < 0) {
        printf("open %s failed\n", disk);
        exit(1);
    }
    printf("Opened device: %s\n", disk);
    printf("checking EXT2 FS ....");
    dev = fd;
    /********** read super block at 1024 ****************/
    get_block(dev, 1, buf);
    sp = (SUPER*)buf;

    /* verify it's an ext2 file system *****************/
    if (sp->s_magic != 0xEF53) {
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        exit(1);
    }
    printf("OK\n");
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    get_block(dev, 2, buf);
    gp = (GD*)buf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;
    printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

    init();
    mount_root();

    printf("root refCount = %d\n", root->refCount);

    printf("creating P0 as running process\n");
    running = &proc[0];
    running->status = READY;
    running->cwd = iget(dev, 2);

    (&proc[1])->cwd = iget(dev, 2);

    printf("root refCount = %d\n", root->refCount);

    while (1) {
        printf("input command : [ls|cd|pwd|quit] ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;
        if (line[0] == 0)
            continue;
        pathname[0] = 0;
        cmd[0] = 0;
        
        sscanf(line, "%s %s", cmd, pathname);
        if (strcmp(cmd, "ls") == 0) {
            if (strlen(pathname) == 0)
                strcpy(pathname, ".");
            printf("cmd=%s pathname=%s\n", cmd, pathname);
            ls_dir(pathname);
        } else if (strcmp(cmd, "cd") == 0) {
            printf("cmd=%s pathname=%s\n", cmd, pathname);
            change_dir(pathname);
        } else if (strcmp(cmd, "pwd") == 0) {
            printf("cmd=%s pathname=%s\n", cmd, pathname);
            pwd(running->cwd);
        } else if (strcmp(cmd, "quit") == 0) {
            quit();
        }
    }
}