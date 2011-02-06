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

#include <stdio.h>
#include <math.h>
#include <libusb-1.0/libusb.h>
#include "dfurequests.h"
#include "dfucommands.h"

/*
	dfu_read_flash() fills membuf with length bytes from flash memory.
*/
int32_t dfu_read_flash(dfu_device * device, uint8_t * membuf, uint32_t length)
{
	int32_t read_2048;
	dfu_status status;
	int i;
	int rv;
	uint8_t finalpage[2048];
	int finalread;
	
	read_2048 = ceil(length / 2048.);
	
	//flash reads must be 2k, which is the flash block size on stm32
	//read all but the final page
	for (i=0; i<(read_2048-1); i++)
	{
		#if STMDFU_DEBUG_PRINTFS
		printf("read_2048: <%d>\n", i);
		#endif
		if (0 > dfu_upload(device, i+2, &membuf[i*2048], 2048))
		{
			printf("read_2048 error\n");
		}
		
		if (0 > dfu_get_status(device, &status))
		{
			printf("dfu_read_flash: dfu_get_status error\n");
		}
		
		if (status.bState == STATE_DFU_ERROR)
		{
			if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
			{
				printf("dfu_read_flash failed: flash read protection enabled\n");
				return -1;
			} else
			{
				printf("dfu_read_flash failed: reason unknown\n");
			}
		}
	}
	
	//read the final page
	#if STMDFU_DEBUG_PRINTFS
	printf("final read_2048: <%d>\n", (read_2048-1));
	#endif
	if (0 > dfu_upload(device, i+2, finalpage, 2048))
	{
		printf("read_2048 error\n");
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_read_flash: dfu_get_status error\n");
	}
	
	if (status.bState == STATE_DFU_ERROR)
	{
		if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
		{
			printf("dfu_read_flash failed: flash read protection enabled\n");
			return -1;
		} else
		{
			printf("dfu_read_flash failed: reason unknown\n");
		}
	}
	
	//fill up the user's buffer with bytes from
	//the final page, ignoring bytes beyond the length
	//of the user's request
	finalread = length - ((read_2048-1)*2048);
	
	for (i=0; i<finalread; i++)
	{
		membuf[((read_2048-1)*2048)+i] = finalpage[i];
	}

	return 1;
}

/*
	dfu_read_optbytes() will fill membuf with the option bytes of
	the microcontroller. The option bytes control things like read
	and write protection for the flash memory.
*/
int32_t dfu_read_optbytes(dfu_device * device, uint8_t * membuf)
{
	dfu_status status;
	int rv;
	
	dfu_set_address_pointer(device, OPTION_BYTES_ADDRESS);
	
	dfu_make_idle(device, 0);
	
	rv = dfu_upload(device, 2, membuf, 16);
	
	if (0 > rv)
	{
		printf("dfu_read_optbytes failed: control transfer error <%d>\n", rv);
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_read_optbytes: dfu_get_status error\n");
	}
	
	return 0;
}

/*
	dfu_get() asks the bootloader to list some (?) commands that it will
	respond to, as well as their command codes.
	
	stm32 should return 4 bytes (1 for: get (0x00), set addr pointer (0x21),
	erase (0x41), read unprotect (0x92))
*/
int32_t dfu_get(dfu_device * device, uint8_t * data)
{
	dfu_status status;
	
	if (4 != dfu_upload(device, 0, data, 4))
	{
		printf("error getting commands\n");
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_get: dfu_get_status error\n");
	}
	
	return 1;
}

/*
	dfu_write_flash() writes (in 2kB pages) the contents of membuf
	to flash memory. The write begins at the location pointed to by
	the address pointer (use dfu_set_address_pointer()).
*/
int32_t dfu_write_flash(dfu_device * device, uint8_t * membuf, uint32_t length)
{
	int write_2048;
	int i, j;
	dfu_status status;
	int rv;
	uint8_t finalpage[2048];
	int finalwrite;
	
	//round up the number of writes to the next 2kB page
	write_2048 = ceil(length / 2048.);
	
	//write all but the final page
	for (i=0; i<(write_2048-1); i++)
	{
		#if STMDFU_DEBUG_PRINTFS
		printf("write_2048: <%d>\n", i);
		#endif
		rv = dfu_download(device, i+2, &membuf[i*2048], 2048);
		
		if (0 > rv)
		{
			printf("dfu_write_flash: dfu_download error <%d>\n", rv);
		}
		
		if (0 > dfu_get_status(device, &status))
		{
			printf("dfu_write_flash: dfu_get_status error\n");
		}
		
		if (status.bState != STATE_DFU_DOWNLOAD_BUSY)
		{
			printf("dfu_write_flash: not in STATE_DFU_DOWNLOAD_BUSY after dfu_download\n");
		}
		
		if (0 > dfu_get_status(device, &status))
		{
			printf("dfu_write_flash: dfu_get_status error 2\n");
		}
		
		if (status.bState == STATE_DFU_ERROR)
		{
			if (status.bStatus == DFU_STATUS_ERROR_TARGET)
			{
				printf("dfu_write_flash failed: received address wrong/unsupported\n");
				return -1;
			} else if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
			{
				printf("dfu_write_flash failed: flash read protection enabled\n");
				return -2;
			} else
			{
				printf("dfu_write_flash failed: reason unknown\n");
				return -3;
			}
		}
	}
	
	//write the final page
	//we fill the final page with whatever good data
	//is left in membuf, and pad it to 2048 with 0xff
	finalwrite = length - ((write_2048-1)*2048);
	
	for (i=0; i<finalwrite; i++)
	{
		finalpage[i] = membuf[((write_2048-1)*2048)+i];
	}
	
	for (i=finalwrite; i<2048; i++)
	{
		finalpage[i] = 0xff;
	}
	
	#if STMDFU_DEBUG_PRINTFS
	printf("final write_2048: <%d>\n", (write_2048-1));
	#endif
	rv = dfu_download(device, (write_2048-1)+2, finalpage, 2048);
	
	if (0 > rv)
	{
		printf("dfu_write_flash: dfu_download error <%d>\n", rv);
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_write_flash: dfu_get_status error\n");
	}
	
	if (status.bState != STATE_DFU_DOWNLOAD_BUSY)
	{
		printf("dfu_write_flash: not in STATE_DFU_DOWNLOAD_BUSY after dfu_download\n");
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_write_flash: dfu_get_status error 2\n");
	}
	
	if (status.bState == STATE_DFU_ERROR)
	{
		if (status.bStatus == DFU_STATUS_ERROR_TARGET)
		{
			printf("dfu_write_flash failed: received address wrong/unsupported\n");
			return -1;
		} else if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
		{
			printf("dfu_write_flash failed: flash read protection enabled\n");
			return -2;
		} else
		{
			printf("dfu_write_flash failed: reason unknown\n");
			return -3;
		}
	}
	
	return 0;
}

/*
	dfu_set_address_pointer() sets the STM32 device's address pointer.
	This is necessary before performing some other DFU commands, such as
	dfu_write_flash. The address being written to is determined relative
	to where the address pointer points.
*/
int32_t dfu_set_address_pointer(dfu_device * device, int32_t address)
{
	dfu_status status;
	int8_t command[5] = {0x21, 0, 0, 0, 0};
	int i;
	int rv;
	
	int8_t * addr = &address;
	
	for (i=0; i<4; i++)
	{
		command[i+1] = addr[i];
	}
	
	rv = dfu_download(device, 0, command, 5);
	
	if (5 != rv)
	{
		printf("dfu_set_address_pointer: dfu_download error <%d>\n", rv);
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_set_address_pointer: dfu_get_status error\n");
	}
	
	if (status.bState != STATE_DFU_DOWNLOAD_BUSY)
	{
		printf("dfu_set_address_pointer: wrong state after submitting address\n");
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_set_address_pointer: dfu_get_status error 2\n");
	}
	
	if ((status.bState != STATE_DFU_ERROR) && (status.bStatus != DFU_STATUS_ERROR_TARGET))
	{
		//success
		return 0;
	} else
	{
		printf("dfu_set_address_pointer failed\n");
		return -1;
	}
}

/*
	dfu_erase() erases a single page (2kB) of flash memory. The page that
	address belongs to is the page that is erased.
*/
int32_t dfu_erase(dfu_device * device, int32_t address)
{
	int8_t command[5] = {0x41, 0, 0, 0, 0};
	dfu_status status;
	int i;
	
	int8_t * addr = &address;
	
	for (i=0; i<4; i++)
	{
		command[i+1] = addr[i];
	}
	
	if (5 != dfu_download(device, 0, command, 5))
	{
		printf("dfu_erase: dfu_download error\n");
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_erase: dfu_get_status error\n");
	}
	
	if (status.bState == STATE_DFU_ERROR)
	{
		if (status.bStatus == DFU_STATUS_ERROR_TARGET)
		{
			printf("dfu_write_flash failed: received address wrong/unsupported\n");
			return -1;
		} else if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
		{
			printf("dfu_write_flash failed: flash read protection enabled\n");
			return -2;
		} else
		{
			printf("dfu_write_flash failed: reason unknown\n");
			return -3;
		}
	} else
	{
		//success
		return 0;
	}
	
	return 1;
}

/*
	dfu_mass_erase() erases all pages of flash memory
*/
int32_t dfu_mass_erase(dfu_device * device)
{
	int8_t command[1] = {0x41};
	dfu_status status;
	
	if (1 != dfu_download(device, 0, command, 1))
	{
		printf("dfu_erase_mass: dfu_download error\n");
	}
	
	if (0 > dfu_get_status(device, &status))
	{
		printf("dfu_erase_mass: dfu_get_status error\n");
	}
}
	
/*
*  Gets the device into the dfuIDLE state if possible.
*
*  device    - the dfu device to commmunicate with
*
*  returns 0 on success, 1 if device was reset, error otherwise
*/
int32_t dfu_make_idle( dfu_device *device, const int initial_abort )
{
	dfu_status status;
	int32_t retries = 4;
	
	if(0 != initial_abort ) {
		dfu_abort( device );
	}
	
	while( 0 < retries ) {
		if( 0 != dfu_get_status(device, &status) ) {
			dfu_clear_status( device );
			continue;
		}
		
		switch( status.bState ) {
			case STATE_DFU_IDLE:
				if( DFU_STATUS_OK == status.bStatus ) {
					return 0;
				}
				
				/* We need the device to have the DFU_STATUS_OK status. */
				dfu_clear_status( device );
				break;
				
			case STATE_DFU_DOWNLOAD_SYNC:   /* abort -> idle */
			case STATE_DFU_DOWNLOAD_IDLE:   /* abort -> idle */
			case STATE_DFU_MANIFEST_SYNC:   /* abort -> idle */
			case STATE_DFU_UPLOAD_IDLE:     /* abort -> idle */
			case STATE_DFU_DOWNLOAD_BUSY:   /* abort -> error */
			case STATE_DFU_MANIFEST:        /* abort -> error */
				dfu_abort( device );
				break;
				
			case STATE_DFU_ERROR:
				dfu_clear_status( device );
				break;
				
			case STATE_APP_IDLE:
				dfu_detach( device, DFU_DETACH_TIMEOUT );
				break;
				
			case STATE_APP_DETACH:
			case STATE_DFU_MANIFEST_WAIT_RESET:
				libusb_reset_device(device->handle);
				return 1;
		}
		
		retries--;
	}
	
	return -2;
}