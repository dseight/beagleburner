# BeagleBurner Buildroot

## About

BeagleBurner is a simple device with 3 buttons that can burn _something_ to an
inserted SD Card. This _something_ is provided by the user of BeagleBurner by
simply connecting it to a PC. When connected to a PC, you can copy files into
one of the slots (1, 2 or 3). And then flash files from that slot in one click
of a button.

Note that you don't need a PC when files are copied to the slot! You can just
power on your BeagleBurner from a power bank, insert an SD Card and press a
button!

For more information of how to use this device, see [USAGE.md].

## Build

Clone this repo:

    git clone https://github.com/dseight/beagleburner.git

Then clone buildroot and checkout it on release `2025.11`:

    git clone https://github.com/buildroot/buildroot.git
    cd buildroot
    git checkout 2025.11

and run this, pointing to the directory with this repo:

    make BR2_EXTERNAL=/path/to/beagleburner beaglebone_defconfig

and, finally, build it:

    make

## Making changes

When some configuration changes are needed, use one of:

    make menuconfig
    make uboot-menuconfig
    make linux-menuconfig
    make busybox-menuconfig

After running one of the above commands, one might want to save it back as a
defconfig:

    make savedefconfig
    make uboot-update-defconfig
    make linux-update-defconfig
    make busybox-update-defconfig

## Flashing to BeagleBone Black

This is something that needs to be improved. The best way would be to create a
special image for sd card that will flash eMMC.

But for now, cd into `output/images` and start a tftp server on your machine
like so (checked on Fedora):

    sudo in.tftpd -4 -Lps -u $USER $PWD

Also, disable firewall (only needs to be done once):

    sudo firewall-cmd --add-port=69/udp

And then, using serial connected to BeagleBone Black, do this:

    => dhcp $loadaddr <your-pc-ip> emmc.img
    => mmc dev 1
    => mmc write $loadaddr 0 0x27000

## Booting from FIT

First, setup tftp server on your machine in the same way as written in
"Flashing to BeagleBone Black" section. Then, using serial connected to
BeagleBone Black, do this:

    => dhcp $loadaddr <your-pc-ip>:beagleburner.itb && bootm

## TODO

- [ ] User may unplug the power right after copying the files
- [ ] Try to use mmcblk1p3 partition as a storage
- [ ] Check pinmux used during bootup. DONE and S2 LEDs glow a little bit.
- [ ] Optimize boot speed
- [ ] Mount mmcblk1p1 as /boot in debug configuration
- [ ] Remove crap from uEnv.txt
- [ ] Maybe add slotX.sh scripts support as a flashing method. But be aware
      that user can mess up CR/LF (i.e. user may use Windows).
    - [ ] Some simple isolation, e.g. landlock? So that the scripts ran by
          burner from /storage will not erase anything in rootfs.

## Links

- [The Buildroot user manual](https://buildroot.org/downloads/manual/manual.html)

## License

Files under `package/burner` are licensed under [MIT License](LICENSE.MIT),
everything else is under [GPLv2](LICENSE.GPL).
