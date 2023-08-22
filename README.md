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
XMHF64.

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
* To boot SHV using GRUB, use `-cdrom grub.iso`.

Note that SHV requires hardware virtualization (VMX), so QEMU must enable KVM.
Sample command line is `-cpu Haswell,vmx=yes -enable-kvm`.

By default SHV prints output to serial port. Use `-serial stdio` to let QEMU
print the serial port message to stdout.

[tools/qemu.sh](tools/qemu.sh) is intended to make running SHV easy. For
example, the following command tests SHV with 4 Haswell CPUs and 1G RAM.
```sh
../tools/qemu.sh -kernel shv.bin
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

### Running SHV on real hardware
TODO

### Running SHV on VMware
TODO

### Running SHV on VirtualBox
TODO

### Running SHV on Hyper-V
TODO

### Running SHV on Bochs
TODO

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

TODO

