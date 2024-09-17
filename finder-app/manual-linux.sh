#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.
# Editor: Sonal Tamrakar
# Date: 09/16/2024
# Brief: manual-linux.sh for Assignment 3 QEMU emulation build
# Credit: Mastering Embedded Linux Programming Chapter 4,5

set -e
set -u

OUTDIR=/tmp/aeld
ROOTFSDIR=/tmp/aeld/rootfs
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)


if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    # kernel build steps here

    # Deep cleaning the Kernel build tree, removes all intermediate files
    # including the .config file
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    # Configure "virt" arm development board that will be simulated in QEMU
    # Sets up default config
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig


    #Building a kernel image that will be booted with QEMU to vmlinux target
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    #Buid the kernel modules
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    #Build the devicetree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
# TODO: Copy resulting files generated in step 1.c to outdir
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem_here"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${ROOTFSDIR}
cd ${ROOTFSDIR}
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig

else
    cd busybox
fi

# TODO: Make and install busybox
# make distclean
# make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${ROOTFSDIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ${ROOTFSDIR}

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
# sysroot path for lib64 : /home/stamrakar/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64
# sysroot path for lib : /home/stamrakar/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib


# Requesting program interpreter: /lib/ld-linux-aarch64.so.1
cd ${ROOTFSDIR}
cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 lib
cp -a ${SYSROOT}/lib64/libm.so.6 lib64
cp -a ${SYSROOT}/lib64/libresolv.so.2 lib64
cp -a ${SYSROOT}/lib64/libc.so.6 lib64


# TODO: Make device nodes
# mknod <name> <type> <major> <minor>
# Null device is a known major 1 minor 3
# Console device is known major 5
echo "making device nodes here"
cd ${ROOTFSDIR}
sudo mknod -m 666 /dev/null c 1 3
sudo mknod -m 600 /dev/console c 5 1

echo "mknods of null and console completion"
# TODO: Clean and build the writer utility
# First locate the where the directory is given command
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/writer ${ROOTFSDIR}/home
cp ${FINDER_APP_DIR}/finder.sh ${ROOTFSDIR}/home
cp ${FINDER_APP_DIR}/finder-test.sh ${ROOTFSDIR}/home
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${ROOTFSDIR}/home

mkdir -p ${ROOTFSDIR}/home/conf
cp ${FINDER_APP_DIR}/conf/username.txt ${ROOTFSDIR}/home/conf
cp ${FINDER_APP_DIR}/conf/assignment.txt ${ROOTFSDIR}/home/conf


# TODO: Chown the root directory
cd ${ROOTFSDIR}
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
cd ${ROOTFSDIR}
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

cd ${OUTDIR}
gzip -f initramfs.cpio

echo "End of script"
