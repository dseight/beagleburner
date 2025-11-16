Congratulations! Somehow, you got your hands on one of BeagleBurner boards.

BeagleBurner is a simple device with 3 buttons that can burn _something_ to an
inserted SD Card. This _something_ is provided by the user of BeagleBurner by
simply connecting it to a PC. When connected to a PC, you can copy files into
one of the slots (1, 2 or 3). And then flash files from that slot in one click
of a button.

You don't need a PC when files are copied to the slot! You can just power on
your BeagleBurner from a power bank, insert an SD Card and press a button!

See https://github.com/dseight/beagleburner/blob/main/USAGE.md for more info.


## Flashing methods

### Whole image

The simplest method is to have a whole image for the SD card. It can be smaller
than the actual SD card size (and most likely should be, as the internal memory
of BeagleBurner is limited to 4GB).

Just put a file named `slotX.img` to the root of the storage, where X is a
number from 1 to 3.

This whole file will be burned to the SD card when pressing corresponding slot
button (from 1 to 3).

### Partition directory

A different approach is to create a directory `slotX_pY`, where `X` is a slot
number (from 1 to 3) and `Y` is the partition number (from 1 to 9), and then
copy all the necessary files into that directory.

E.g., `slot2_p1` will be used to copy the content of this directory to the
partition 1, when using a slot 2 button. Files will be overridden, but the
existing content on SD card partitions will not be erased.

Please note that the permissions of the copied files will be screwed up, as the
storage is using FAT filesystem which is not aware of RWX permissions for
user/group/others.

You can have multiple partition directories for one slot if you need to copy
files into multiple partitions. If one of the partitions does not exist on an
inserted SD card, nothing will be copied and "FAIL" LED will glow up.

### Mixed (directory + image)

If you put both slotX_pY directories AND slotX.img files to the storage, then
"partition directory" method will be attempted at first. If that fails,
slotX.img will be burned to an SD card instead.
