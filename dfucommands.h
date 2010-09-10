/*
dfucommands.{c,h} :
Implements the higher level DFU commands that STM has defined in their bootloader.
These commands are made up of (usually multiple) DFU requests.

STM describes the necessary error checking (using dfu_get_status()) to make sure
the bootloader is responding correctly to commands. This is detailed in STM's
documentation.

More information on the DFU commands is available in the application note USB DFU
protocol used in the STM32 Bootloader, AN3156.
*/

#ifndef __DFU_COMMANDS__
#define __DFU_COMMANDS__

#define OPTION_BYTES_ADDRESS 0x1ffff800

/*
dfu_read_flash() fills membuf with length bytes from flash memory.
*/
int32_t dfu_read_flash(dfu_device * device, uint8_t * membuf, uint32_t length);

/*
dfu_read_optbytes() will fill membuf with the option bytes of
the microcontroller. The option bytes control things like read
and write protection for the flash memory.
*/
int32_t dfu_read_optbytes(dfu_device * device, uint8_t * membuf);

/*
dfu_get() asks the bootloader to list some (?) commands that it will
respond to, as well as their command codes.

stm32 should return 4 bytes (1 for: get (0x00), set addr pointer (0x21),
erase (0x41), read unprotect (0x92))
*/
int32_t dfu_get(dfu_device * device, uint8_t * data);

/*
dfu_write_flash() writes (in 2kB pages) the contents of membuf
to flash memory. The write begins at the location pointed to by
the address pointer (use dfu_set_address_pointer()).
*/
int32_t dfu_write_flash(dfu_device * device, uint8_t * membuf, uint32_t length);

/*
dfu_set_address_pointer() sets the STM32 device's address pointer.
This is necessary before performing some other DFU commands, such as
dfu_write_flash. The address being written to is determined relative
to where the address pointer points.
*/
int32_t dfu_set_address_pointer(dfu_device * device, int32_t address);

/*
dfu_erase() erases a single page (2kB) of flash memory. The page that
address belongs to is the page that is erased.
*/
int32_t dfu_erase(dfu_device * device, int32_t address);

/*
dfu_mass_erase() erases all pages of flash memory
*/
int32_t dfu_mass_erase(dfu_device * device);

/* unimplemented :
int32_t dfu_read_unprotect(dfu_device * device);
int32_t dfu_leave_dfu_mode(dfu_device * device);
*/

/*
*  Gets the device into the dfuIDLE state if possible.
*
*  device    - the dfu device to commmunicate with
*
*  returns 0 on success, 1 if device was reset, error otherwise
*/
int32_t dfu_make_idle( dfu_device *device, const int initial_abort );
#endif
