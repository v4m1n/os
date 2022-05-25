#!/bin/sh
dev=$(udisksctl loop-setup --file disk.img | cut -d  ' ' -f 5)
mnt=$(udisksctl mount -b "${dev::-1}p1" | cut -d ' ' -f 4)
cp kernel.elf "${mnt}/boot/"
cp grub.cfg "${mnt}/boot/grub/"
udisksctl unmount -b "${dev::-1}p1" > /dev/null
udisksctl loop-delete -b "${dev::-1}" > /dev/null

