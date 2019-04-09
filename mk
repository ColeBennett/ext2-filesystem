rm diskimage
dd if=/dev/zero of=diskimage bs=1024 count=4096
mkfs -b 1024 diskimage 4096

rm a.out
gcc main.c -g
gdb ./a.out diskimage