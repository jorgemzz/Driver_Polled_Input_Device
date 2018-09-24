# Description
Implementation of a Polled Input Device Driver.

This driver is part of my final project for the course EECS_X497.12: Linux Driver Primer at University of California Irvine - Division of Continuing Education.
# The Input Device:
The input device was implemented on the ATmega328PB Xplained Mini development board from Atmel(Microchip). 
The main program monitors the states of 3 push buttons(BTN_0, BTN_1, BTN_2) and updates an 8bit variable with the state of 
the buttons as following. 
* bit 0 == 0 means that BTN_0 is released.
* bit 0 == 1 means that BTN_0 is pressed.
* bit 1 == 0 means that BTN_1 is released.
* bit 1 == 1 means that BTN_1 is pressed
* bit 2 == 0 means that BTN_2 is released.
* bit 2 == 1 means that BTN_2 is pressed.

The TWI0 interface on the ATmega328PB was programmed to work as an I2C slave with the 0x10 device address and with 0x20 as the
buttons' state register address.

# The Linux Driver
The I2C messages to communicate with the Input Device are in the form: `<i2c_address>  <register>` for reading the 8bit buttons'
state register.
The address, 0x10 is sent by the i2c framework so there is only left to specify the register address(0x20) in the driver's code.

# Notes
Rebuild your kernel with static support for polled input device (**CONFIG_INPUT_POLLDEV=y**) and
for event interface (**CONFIG_INPUT_EVDEV**) support. If you do not include this configuration, you will receive the error could 
not insert module example.ko: Unknown symbol in module when you attempt to allocate or register the polling device.

This driver was tested with Kernel version 4.18.8.

# Useful links
* http://processors.wiki.ti.com/index.php/Processor_SDK_Linux_Training:_Introduction_to_Device_Driver_Development
* https://bootlin.com/doc/training/linux-kernel/linux-kernel-labs.pdf
