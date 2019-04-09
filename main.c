/**
 * Cole Bennett
 * CptS 360
 * Project Level 1
 * March 29, 2019
*/

/**** main.c file ****/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
#include "globals.c"
#include "util.c"
#include "level1/cd_ls_pwd.c"
#include "level1/mkdir_creat.c"
#include "level1/rmdir.c"
#include "level1/link_unlink.c"
#include "level1/symlink_readlink.c"
#include "level1/stat.c"
#include "level1/misc1.c"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

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
int mount_root(char *disk)
{
    printf("mount_root()\n");
    int ino;
    char buf[BLKSIZE];
    if ((fd = open(disk, O_RDWR)) < 0) {
        printf("open %s failed\n", disk);
        exit(1);
    }
    printf("Opened device: %s\n", disk);
    printf("checking EXT2 FS ....");
    dev = fd;
    /********** read super block at 1024 ****************/
    get_super_block(dev, buf);
    sp = (SUPER *) buf;

    /* verify it's an ext2 file system *****************/
    if (sp->s_magic != 0xEF53) {
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        exit(1);
    }
    printf("OK\n");
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;
    printf("ninodes = %d nblocks = %d\n", ninodes, nblocks);

    get_groupdesc_block(dev, buf);
    gp = (GD *) buf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;
    printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

    for (int i = 0; i < 5; i++) {  
        ino = ialloc(fd);
        printf("allocated ino = %d\n", ino);
    }

    init();
    root = iget(dev, 2);
    printf("root refCount = %d\n", root->refCount);

    printf("creating P0 as running process\n");
    running = &proc[0];
    running->status = READY;
    running->cwd = iget(dev, 2);

    (&proc[1])->cwd = iget(dev, 2);

    printf("root refCount = %d\n", root->refCount);
}

int quit()
{
    printf("Quitting\n");
    int i;
    MINODE *mip;
    for (i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        if (mip->refCount > 0 && mip->dirty) {
            mip->refCount = 1;
            iput(mip);
        }
    }
    exit(0);
}

char *cmds[] = {
    "ls", "cd", "pwd", "mkdir", "creat", "rmdir", "link", "unlink",
    "symlink", "readlink", "stat", "chmod", "touch", "utime", "quit", 0};
int (*fptr[]) (char *) = {
    ls_dir, change_dir, pwd, make_dir, creat_file, fsrmdir, fslink, fsunlink,
    fssymlink, fsreadlink, fsstat, fschmod, fstouch, fsutime, quit};

int find_cmd(char *cmds[], char *cmd)
{
    int i = 0;
    while (cmds[i]) {
        if (strcmp(cmds[i], cmd) == 0)
            return i;
        i++;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    char *disk = "diskimage";
    if (argc > 1)
        disk = argv[1];
    mount_root(disk);

    int index;
    while (1) {
        printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|readlink|stat|chmod|touch|utime|quit]\n> ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;
        if (line[0] == 0)
            continue;
        pathname[0] = 0;
        cmd[0] = 0;
        
        sscanf(line, "%s %s", cmd, pathname);
        
        index = find_cmd(cmds, cmd);
        if (index < 0) {
            printf("Invalid command: %s\n", line);
            continue;
        }
        
        if (strcmp(cmd, "ls")==0)
            strcpy(pathname, ".");
        printf("cmd=%s pathname=%s\n", cmd, pathname);
        fptr[index](pathname);
    }
}
