# SHV

SHV (Small HyperVisor) is a small hypervisor for learning and testing purpose.

## Introduction

SHV is a monolithic system-level program. It runs on Intel CPUs and uses VMX
to perform hardware virtualization. It can be booted using multiboot. It only
supports BIOS (i.e. no support for UEFI). It supports multiple CPUs (SMP).

SHV is open source and is released under GPLv3.

## History and acknowledgements

**SHV** is ported from LHV. The boot process of LHV based on XMHF64 and is
complicated. SHV rewrites the booting process to make it easy to debug.

[**LHV**](https://github.com/lxylxy123456/uberxmhf/tree/lhv) is used during the
development of XMHF64 to test XMHF64's nested virtualization functionality.
LHV is written by [lxylxy123456](https://github.com/lxylxy123456), based on
XMHF64. Some of the interrupt handling code is based on open source projects
that are used used by [CMU's 15-410](https://www.cs.cmu.edu/~410/). See
[15-410 Software License Statements](https://www.cs.cmu.edu/~410/licenses.html).

[**XMHF64**](https://github.com/lxylxy123456/uberxmhf) is a research project by
[lxylxy123456](https://github.com/lxylxy123456). XMHF64 is based on XMHF. XMHF
only supports 32-bit OSes, but XMHF64 also supports 64-bit OSes. XMHF does not
support nested virtualizatoin, but XMHF64 supports nested virtualization.

[**XMHF**](https://github.com/uberspark/uberxmhf/tree/master/xmhf) (eXtensible
Micro-Hypervisor Framework) is a micro-hypervisor research project by Vasudevan
et al. XMHF uses multiple pieces of open source software. Please see
<https://github.com/uberspark/uberxmhf/blob/master/xmhf/COPYING.md>.

As of the writing of this section, the git commit sha of the projects above
are:
* LHV: `ed8d59e1bac6d439946c1e18d896a4b7f6a14b27`
* XMHF64: `3c22712f5de7b60eedac3483112a69d647d9e86c`
* XMHF: `b0365e5cd7df78414d3df8553a98bcce5ce6d217`

## Building SHV

Hint: looking at [CI](#ci) section may be helpful when something does not work.

Install dependencies (`qemu-system-x86` is only needed for testing), in this
example we demonstrate installing on Debian-based Linux:
```sh
sudo apt-get update && \
sudo apt-get install -y \
	build-essential crossbuild-essential-i386 autoconf libtool xorriso \
	mtools grub-pc git qemu-system-x86
```

### Building using build.sh

[tools/build.sh](tools/build.sh) is intended to make the build process easy.
To build in directory `build/`:

```sh
mkdir build
cd build
../tools/build.sh
```

To specify the platform to build for:
* i386, 32-bit paging: `--i386` or `-i`
* i386, PAE paging: `--pae` or `-p`
* amd64, 4-level paging: `--amd64` or `-a`

To view the command `build.sh` passes to `configure`:
```sh
../tools/build.sh -n
```

To view arguments to `build.sh`:
```sh
../tools/build.sh -h
```

### Building without using build.sh

SHV uses Autoconf and Automake. In this example we build in a different
directory `build/`:
```sh
autoreconf --install
mkdir build
cd build
../configure
make -j `nproc`
```

To specify the platform to build for:
* i386, 32-bit paging: `--host=i686-linux-gnu`
* i386, PAE paging: `--host=i686-linux-gnu --enable-i386-pae`
* amd64, 4-level paging: `--host=x86_64-linux-gnu`

To change optimization level (default is O2, due to autoconf?):
* O0: `CFLAGS='-g -O0'`
* O3: `CFLAGS='-g -O3'`

To view other configuration options:
```sh
../configure -h
```

### Build artifacts

* `shv.bin`: multiboot ELF image for SHV.
* `grub.iso`: grub ISO image that boots SHV.

## Running SHV

### Running SHV on QEMU

There are two ways to load SHV to QEMU / KVM.
* To load SHV image directly, use `-kernel shv.bin`.
	* To add arguments to multiboot commandline, use `-append "arg1 arg2 ..."`.
* To boot SHV using GRUB, use `-cdrom grub.iso`.
	* To add arguments to multiboot commandline, edit `grub.cfg` in the build
	  directory and then rebuild using `make`.

Note that SHV requires hardware virtualization (VMX), so QEMU must enable KVM.
Sample command line is `-cpu Haswell,vmx=yes -enable-kvm`.

By default SHV prints output to serial port. Use `-serial stdio` to let QEMU
print the serial port message to stdout.

[tools/qemu.sh](tools/qemu.sh) is intended to make running SHV easy. For
example, the following command tests SHV with 4 Haswell CPUs and 1G RAM.
`g_shv_opt` is set to 0xdfd and `g_nmi_opt` is set to 0.
```sh
../tools/qemu.sh -kernel shv.bin -append 'shv_opt=0xdfd nmi_opt=0'
```

To view arguments to `qemu.sh`:
```sh
../tools/qemu.sh -h
```

#### Debugging SHV using QEMU and GDB

Pass `-s` to QEMU to enable GDB server. Optionally pass `-S` to let QEMU wait
for the GDB server after initializing.

To connect GDB to QEMU and load debug info in `shv.bin`:
```sh
gdb --ex 'target remote localhost:1234' --ex 'symbol-file shv.bin'
```

The approximate boot sequence is:
```
_start (boot.S, BSP only, protected mode without paging)
kernel_main (kernel.c, BSP only)
smp_init (smp.c, BSP only)
(BSP calls wakeupAPs() to wake up APs)
0x10000 (was _ap_bootstrap_start, smp-asm.S, AP only, real mode)
0x10??? (was _ap_clear_pipe, smp-asm.S, AP only, protected mode without paging)
init_core_lowlevel_setup_32 (smp-asm.S, AP only, protected mode without paging)
init_core_lowlevel_setup (smp-asm.S)
kernel_main_smp (kernel.c)
shv_main (shv.c)
shv_vmx_main (shv-vmx.c)
vmlaunch_asm (shv-asm.S)
shv_guest_entry (shv-guest-asm.S, VMX guest mode)
shv_guest_main (shv-guest.c, VMX guest mode)
```

The interrupt / exception call stack is approximately:
```
idt_stub_{host,guest}_h...h1...1 (idt-asm.S, e.g. idt_stub_host_hh11)
idt_stub_common (idt-asm.S)
	handle_idt (idt.c)
		handle_..._interrupt
```

##### Debugging interrupts and VMEXITs

`idt_stub_common()` calls `handle_idt()` with the ESP/RSP containing the
EIP/RIP of interrupted code. This allows GDB to back trace to interrupted code.

A similar trick is implemented when `vmexit_asm()` calls `vmexit_handler()`.
This allows GDB to back trace from host mode to guest mode.

Note that back trace works the best when compiled with `-O0`. When SHV is
compiled in 32-bits, QEMU must also be 32-bits (use `qemu-system-i386` or
`./tools/qemu.sh --qb32`).

For example, GDB's `bt` command may output something like:
```
(gdb) bt
#0  update_screen (..., guest=false)            // callee of #1
#1  0x00110099 in handle_timer_interrupt (...)  // callee of #2
#2  0x0010676c in handle_idt (...)              // C interrupt handler
#3  0x00104aec in idt_stub_common ()            // assembly interrupt handler
#4  0x0010a7bf in shv_guest_wait_int_vmexit_handler (...)
                                                // interrupted code
#5  0x0011da8e in vmexit_handler (...)          // C VMEXIT handler
#6  0x0011c993 in vmexit_asm ()                 // assembly VMEXIT handler
#7  0x0010a817 in shv_guest_wait_int (...)      // guest code that causes VMEXIT
#8  0x0010b377 in shv_guest_main (...)          // caller of #7
#9  0x00108fe9 in shv_guest_entry ()            // caller of #8
(gdb) x/3i 0x0010a7bf - 2
   0x10a7bd <shv_guest_wait_int_vmexit_handler+78>:	sti
   0x10a7be <shv_guest_wait_int_vmexit_handler+79>:	hlt
   0x10a7bf <shv_guest_wait_int_vmexit_handler+80>:	cli
(gdb) x/i 0x0010a817
   0x10a817 <shv_guest_wait_int+34>:	vmcall
(gdb) 
```

### Running SHV on real hardware

Running SHV on real hardware requires a machine with Intel CPU that supports
BIOS booting. Note that computers manufactures after around 2020 no longer
supports BIOS booting (i.e. only supports UEFI). See
[technical advisory](https://www.intel.com/content/dam/support/us/en/documents/intel-nuc/Legacy-BIOS-Boot-Support-Removal-for-Intel-Platforms.pdf)
by Intel.

If the real hardware has GRUB installed in BIOS mode, then it can add SHV as a
menuentry in GRUB. Otherwise, it is probably easier to boot SHV via a CD or USB
stick.

If the real hardware does not have a serial port, you can let SHV print output
to VGA (e.g. `./tools/build.sh -s 0x2000 --vga`). If the serial port does not
use the standard 0x3f8 port and 115200 baud rate etc, unfortunately you need to
manually edit [src/debug-uart.c](src/debug-uart.c).

#### Boot from existing GRUB installation

First, put `shv.bin` to `/boot` of the machine.

Second, add the following menuentry block to `/etc/grub.d/40_custom`. It is
similar to the one in [grub.cfg](./grub.cfg), but `(hd0,msdos3)` refers to the
`/` partition in GRUB. In my case it is `/dev/sda3` in Linux.
```
menuentry "shv" {
	multiboot (hd0,msdos3)/boot/shv.bin
}
```

Third, update `grub.cfg`. On Debian this is `sudo update-grub`. On Fedora this
is `sudo grub2-mkconfig -o /boot/grub2/grub.cfg`.

Fourth, restart the machine and select SHV in GRUB.

#### Boot from USB

First, write grub.iso to the USB or CD (suppose the USB is `/dev/sdX`):
```sh
dd if=grub.iso of=/dev/sdX
```

Then boot the real machine with the USB or CD.

### Running SHV on VMware
TODO

### Running SHV on VirtualBox

Create a new virtual machine using VirtualBox, select `grub.iso` as the disk
to boot the OS. Make sure to allocate enough memory. VirtualBox supports serial
port through VM settings.

Since SHV requires nested virtualization, make sure to check
"Enable Nested VT-x/AMD-V" in VM settings. If the checkbox is grayed out, try
the commands from <https://stackoverflow.com/questions/54251855/>:
```sh
VBoxManage modifyvm <VirtualMachineName> --nested-hw-virt on
```

#### VirtualBox Problems

Due to problems of VirtualBox or SHV, currently these workarounds are required:
* Only configure one CPU in VirtualBox (i.e. SMP not supported).
* Do not let SHV access VGA mmio. That is, make sure `g_shv_opt` sets bit
  `0x2000` (e.g. `./tools/build.sh -s 0x2000`) and make sure not to enable VGA
  (i.e. do not use `./tools/build.sh --vga`).
* Do not set any bits in `0x421` of `g_shv_opt`
  (e.g. `./tools/build.sh -s 0x29dc`).

The problems are:
* VirtualBox does not support MTRR.
* Guest mode of SHV cannot access LAPIC memory of VirtualBox.
* Guest mode of SHV cannot access VGA memory of VirtualBox.
* VirtualBox does not seem to support MSR bitmaps.
* For some reason `SHV_USE_UNRESTRICTED_GUEST` does not work on VirtualBox.
	* See #GP(0) when moving 0x80000000 to CR0? Check VMX CR0 shadow

### Running SHV on Hyper-V
TODO

### Running SHV on Bochs

The latest version of Bochs is probably on GitHub:
<https://github.com/bochs-emu/Bochs>

First, compile Bochs from source code. We need to make sure Bochs is compiled
with VMX support. Suppose you want to clone Bochs to `/PATH/TO/BOCHS`:
```sh
cd /PATH/TO/BOCHS
git clone https://github.com/bochs-emu/Bochs
# I got commit 253882589d65ab17e23420a416fbd2d6e7591642
cd Bochs/bochs
./configure \
            --enable-all-optimizations \
            --enable-cpu-level=6 \
            --enable-x86-64 \
            --enable-vmx=2 \
            --enable-clgd54xx \
            --enable-busmouse \
            --enable-show-ips \
            \
            --enable-smp \
            --disable-docbook \
            --with-x --with-x11 --with-term --with-sdl2 \
            \
            --prefix="$PWD"
make -j `nproc`
make install
```

Then go back to SHV build directory, create `bochsrc` that can boot SHV:
```
cpu: model=corei7_sandy_bridge_2600k, count=2, ips=50000000, reset_on_triple_fault=1, ignore_bad_msrs=1, msrs="msrs.def"
memory: guest=512, host=256
romimage: file=$BXSHARE/BIOS-bochs-latest, options=fastboot
vgaromimage: file=$BXSHARE/VGABIOS-lgpl-latest
ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
ata1: enabled=1, ioaddr1=0x170, ioaddr2=0x370, irq=15
ata0-master: type=cdrom, path="grub.iso", status=inserted
boot: cdrom, disk
log: log.txt
panic: action=ask
error: action=report
info: action=report
debug: action=ignore, pci=report # report BX_DEBUG from module 'pci'
debugger_log: -
com1: enabled=1, mode=file, dev=/dev/stdout
```

Then start Bochs
```sh
/PATH/TO/BOCHS/Bochs/bochs/bochs
```

Select "Begin simulation" to run SHV.

## Coding Style

SHV's coding style is similar to Linux's, but it uses 4 spaces as tab width.

`indent` is used to automatically format the code. The options to indent are
`-linux -ts4 -i4`. [tools/indent.sh](tools/indent.sh) is used to automatically
indent most source files.

## CI

### GitHub Actions

Dashboard: <https://github.com/lxylxy123456/shv/actions>

Configuration directory is [.github/workflows/](.github/workflows/).

* `build.yml`: Compile SHV in multiple configurations.
* `indent.yml`: Check indentation of most source files. Fail if
  `tools/indent.sh` is not happy for all of the last 10 commits.
* `vmcs.yml`: Check the format of `include/_vmx_vmcs_fields.h`.

### Circle CI

Dashboard: <https://app.circleci.com/pipelines/github/lxylxy123456/shv>

Configuration directory is [.circleci/](.circleci/).

Circle CI is used to compile SHV and test using QEMU / KVM. Circle CI is used
because it supports nested virtualization, unlike other online CI services.

### Jenkins

Jenkins is hosted locally, using a computer with Intel CPU.

Configuration directory is [tools/ci/](tools/ci/).
* `Jenkinsfile`: Compile and test SHV, similar to Circle CI
* `Jenkinsfile_xmhf`: Compile and test running SHV in XMHF64.

## Bugs Found

* <https://sourceforge.net/p/bochs/bugs/1460/>
	* Fixed by Stanislav Shwartsman in Bochs commit `6b48d6e3` (Jan 2023)
	* Before, around `EXP_BOCHS_MASK = 0x27d0b75f` is stable
	* After, around `EXP_BOCHS_MASK = 0x7ff3fffe` is stable
	* At least fixes experiments 7, 16, 28 (consistently fails before fix)
TODO

## TODO

* Complete README
* Report "#### VirtualBox Problems"
* Report Bochs bug on experiment 18, 19

