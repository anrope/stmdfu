/*
stmdfu.{c,h} :
This is the user interface to the USB DFU commands. A user invokes this program
on the command line with an argument (flash, dump, erase, etc.) and other necessary
information, and the program makes the necessary calls to DFU commands to complete
the operation.
*/

#define STM32VENDOR 0x0483
#define STM32PRODUCT 0xdf11

/*
stmdfu_...() functions are simply wrapper functions that call
dfu_...() functions with the necessary parameters. They exist to make
the interface code in stmdfu.c clearer (i.e. each command line
dfu operation will call just one function, while the stmdfu_...()
wrapper functions handle setting up the arguments, looping, etc.
*/

/*
stmdfu_write_image() is a wrapper function that extracts an image from
a dfuse file, and flashes it to an attached stm32 device via usb dfu.
*/
void stmdfu_write_image(dfu_device * dfudev, char * file);

/*
stmdfu_read_flash() is a wrapper function that reads size bytes of memory
from address on an stm32 device via dfu.
*/
void stmdfu_read_flash(dfu_device * dfudev, int address, int size);

/*
stmdfu_read_optbytes() is a wrapper function that reads the option bytes
from an stm32 device via dfu.
*/
void stmdfu_read_optbytes(dfu_device * dfudev);

/*
stmdfu_erase() is a wrapper function that erases 1 page of flash at a
time on an stm32 device via dfu.
*/
void stmdfu_erase(dfu_device * dfudev, int address);

/*
stmdfu_mass_erase() is a wrapper function that erases all flash memory
of an stm32 device via dfu.
*/
void stmdfu_mass_erase(dfu_device * dfudev);

/*
stmdfu_init_dfu() sets up an attached stm32 dfu device and puts it in
an idle state, so it's ready to handle dfu commands.
*/
dfu_device * stmdfu_init_dfu();

/*
find_dfu_device() searches through the tree of attached usb devices,
and finds any attached stm32 dfu devices (by vendor and product id).
*/
dfu_device * find_dfu_device();

/*
cleanup() releases any usb handles/interfaces and deallocates memory.
*/
void cleanup(dfu_device * dfudev);