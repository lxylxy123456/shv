# SHV

TODO: write README.md
TODO: rename all "LHV" and "XMHF"
TODO: indent
TODO: add copyright to every file

```sh
sudo apt-get install -y xorriso mtools grub-pc-bin build-essential crossbuild-essential-i386

autoreconf --install
./configure --host=i686-linux-gnu && make -j `nproc`
./configure --host=x86_64-linux-gnu && make -j `nproc`
# --with-shv-opt=0x123
# CFLAGS=-g

qemu-system-i386 -cdrom grub.iso
qemu-system-x86_64 -kernel shv.bin -serial stdio -enable-kvm -cpu Haswell,vmx=yes -smp 4 -display none
gdb --ex 'target remote :::1234' --ex 'symbol-file shv.bin'
```

