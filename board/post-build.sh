#!/bin/sh

BOARD_DIR=$(dirname "$0")

cp "${BOARD_DIR}/uEnv.txt" "${BINARIES_DIR}/uEnv.txt"

# Copy storage skeleton to the build directory, so it can be used to generate
# storage.img
rm -rf "${BINARIES_DIR}/storage"
cp -r "${BOARD_DIR}/rootfs_overlay/etc/storage-skel" "${BINARIES_DIR}/storage"

(
    cd "${BINARIES_DIR}" || exit
    # Generate storage.img and put it into rootfs
    "${CONFIG_DIR}/support/scripts/genimage.sh" -c "${BOARD_DIR}/genimage-storage.cfg"
)
cp "${BINARIES_DIR}/storage.img" "$1"

install -m 0644 -D "${BOARD_DIR}/extlinux.conf" "${BINARIES_DIR}/extlinux/extlinux.conf"
