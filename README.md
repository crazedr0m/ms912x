# ms912x driver for Linux (kernel 6.8 beta)

Linux kernel driver for MacroSilicon USB to VGA/HDMI adapter. VID/PID is 534d:6021. Device is USB2.0

There are two variants:
 - VID/PID is 534d:6021. Device is USB 2
 - VID/PID is 345f:9132. Device is USB 3

- Driver adapted for Linux kernel 6.8 by dcomp7
- Improved performance.
- Fixed DRM_COPY_FIELD error
- Enhanced error handling and logging

For kernel 6.15 checkout branch kernel-6.15
For kernel 6.10,6.12,6.11 checkout branch kernel-6.10

## Installation

See INSTALL.md for detailed installation instructions.

## Development

Driver is written by analyzing wireshark captures of the device.

## DKMS

Run `sudo dkms install .`

- make clean
- make all -j
- sudo rmmod ms912x # It will not work if the device is in use.
- sudo modprobe drm_shmem_helper
- sudo insmod ms912x.ko

Forked From: https://github.com/rhgndf/ms912x
