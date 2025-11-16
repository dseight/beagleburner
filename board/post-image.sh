#!/bin/sh

BOARD_DIR=$(dirname "$0")
MKIMAGE="${HOST_DIR}/bin/mkimage"

cd "${BINARIES_DIR}" || exit 1
cp "${BOARD_DIR}/beagleburner.its" "${BINARIES_DIR}/beagleburner.its" || exit 1
# Buildroot defaults to -19, but that's just a waste of time
zstd -f -11 -T0 rootfs.cpio -o rootfs.cpio.zst || exit 1
"${MKIMAGE}" -E -B 0x200 -f beagleburner.its beagleburner.itb
