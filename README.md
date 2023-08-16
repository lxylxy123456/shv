# SHV

TODO

```sh
sudo apt-get install -y xorriso mtools grub-pc-bin build-essential crossbuild-essential-i386

autoreconf --install && ./configure --host=i686-linux-gnu && make -j `nproc`
autoreconf --install && ./configure --host=x86_64-linux-gnu && make -j `nproc`
# --with-shv-opt=0x123
# CFLAGS=-g

qemu-system-i386 -cdrom grub.iso
qemu-system-x86_64 -kernel shv.bin -serial stdio -display none -smp 4 -enable-kvm -cpu Haswell,vmx=yes
gdb --ex 'target remote :::1234' --ex 'symbol-file shv.bin'
```
