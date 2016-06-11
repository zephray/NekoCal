libusb 1.0 (WinUSB) Windows USB driver for 32 and 64 bit platforms.

This directory contains the necessary files to install a USB device as a WinUSB
device for use with libusb 1.0 on Windows (through the WinUSB API).

Before you can install the driver, you need to edit the .inf files and change the
values according to your device properties. Two set of .inf files are provided:
- libusb_device.inf for non composite devices (no MI_## part in the hardware ID)
- libusb_device_multiple_interfaces_0.inf & libusb_device_multiple_interfaces_1.inf
  as an example of a two interface composite device (MI_00 + MI_01 in hardware ID)

See the .inf files for more information.