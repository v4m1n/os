#!/bin/bash
dev=$(udisksctl loop-setup --file disk.img | cut -d  ' ' -f 5 | sed 's/\.//g')
mnt=$(udisksctl mount -b "${dev}p1" | cut -d ' ' -f 4)
#mnt=$(udisksctl mount -b "${dev::-1}p1" | cut -d ' ' -f 4)
cp kernel.elf "${mnt}/boot/"
cp grub.cfg "${mnt}/boot/grub/"
udisksctl unmount -b "${dev}p1" > /dev/null
udisksctl loop-delete -b "${dev}" > /dev/null

