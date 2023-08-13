set -xe

# Debian: apt install xorriso mtools grub-pc-bin

autoreconf --install && ./configure --host=i686-linux-gnu && make -j `nproc`
autoreconf --install && ./configure --host=x86_64-linux-gnu && make -j `nproc`
# --with-shv-opt=0x123
# CFLAGS=-g

set +x

echo qemu-system-i386 -cdrom grub.iso
echo qemu-system-x86_64 -kernel shv.bin -serial stdio -display none -smp 4 -enable-kvm -cpu Haswell,vmx=yes

