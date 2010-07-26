#include <stdio.h>
#include <math.h>
#include <libusb-1.0/libusb.h>
#include "dfurequests.h"
#include "dfucommands.h"

int32_t dfu_read_memory(dfu_device * device, uint8_t * membuf, uint32_t length)
{
	int32_t read_2048, read_2;
	dfu_status status;
	int i;
	int rv;
	
	if (length % 2 != 0)
	{
		printf("read length must be a multiple of 2 bytes\n");
		return -1;
	}
	
	//this can be optimized
	read_2048 = floor(length / 2048);
// 	read_2 = (length-(2048*read_2048))/2;
	
	//flash reads must be 2k, which is the flash block size on stm32
	for (i=0; i<read_2048; i++)
	{
		printf("read_2048:\n");
		if (0 > dfu_upload(device, i+2, &membuf[i*2048], 2048))
		{
			printf("read_2048 error\n");
		}
		
		if (0 > dfu_get_status(device, &status))
		{
			printf("dfu_read_memory: dfu_get_status error\n");
		}
		
		if (status.bState == STATE_DFU_ERROR)
		{
			if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
			{
				printf("dfu_read_memory failed: flash read protection enabled\n");
				return -1;
			} else
			{
				printf("dfu_read_memory failed: reason unknown\n");
			}
		}
	}
	
// 	for (i=0; i<read_2; i++)
// 	{
// 		printf("read_2:\n");
// 		
// 		rv = dfu_upload(device, i+2+(read_2048*2048), &membuf[(i*2)+(read_2048*2048)], 2);
// 		
// 		if (0 > rv)
// 		{
// 			printf("read_2 error: <%d>\n", rv);
// 		}
// 		
// 		if (0 > dfu_get_status(device, &status))
// 		{
// 			printf("dfu_read_memory: dfu_get_status error 2\n");
// 		}
// 	}

	return 1;
}

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


int32_t dfu_get(dfu_device * device, uint8_t * data)
{
	dfu_status status;
	
	//stm32 should return 4 bytes (1 for: get, set addr pointer, erase, read unprotect)
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

int32_t dfu_write_memory(dfu_device * device, uint8_t * membuf, uint32_t length)
{
	int write_2048;
	int i, j;
	dfu_status status;
	int rv;
	
	write_2048 = length / 2048;
	
	for (i=0; i<write_2048; i++)
	{
		printf("write_2048: <%d>\n", i);
		rv = dfu_download(device, i+2, &membuf[i*2048], 2048);
		
		if (0 > rv)
		{
			printf("dfu_write_memory: dfu_download error <%d>\n", rv);
		}
		
		if (0 > dfu_get_status(device, &status))
		{
			printf("dfu_write_memory: dfu_get_status error\n");
		}
		
		if (status.bState != STATE_DFU_DOWNLOAD_BUSY)
		{
			printf("dfu_write_memory: not in STATE_DFU_DOWNLOAD_BUSY after dfu_download\n");
		}
		
		if (0 > dfu_get_status(device, &status))
		{
			printf("dfu_write_memory: dfu_get_status error 2\n");
		}
		
		if (status.bState == STATE_DFU_ERROR)
		{
			if (status.bStatus == DFU_STATUS_ERROR_TARGET)
			{
				printf("dfu_write_memory failed: received address wrong/unsupported\n");
				return -1;
			} else if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
			{
				printf("dfu_write_memory failed: flash read protection enabled\n");
				return -2;
			} else
			{
				printf("dfu_write_memory failed: reason unknown\n");
				return -3;
			}
		}
	}
	
	return 0;
}

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
	
	printf("command: 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n",
			command[0],
			command[1],
			command[2],
			command[3],
			command[4]);
	
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
			printf("dfu_write_memory failed: received address wrong/unsupported\n");
			return -1;
		} else if (status.bStatus == DFU_STATUS_ERROR_VENDOR)
		{
			printf("dfu_write_memory failed: flash read protection enabled\n");
			return -2;
		} else
		{
			printf("dfu_write_memory failed: reason unknown\n");
			return -3;
		}
	} else
	{
		//success
		return 0;
	}
	
	return 1;
}

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