/*
stmdfu.{c,h} :
This is the user interface to the USB DFU commands. A user invokes this program
on the command line with an argument (flash, dump, erase, etc.) and other necessary
information, and the program makes the necessary calls to DFU commands to complete
the operation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include "dfurequests.h"
#include "dfucommands.h"
#include "dfuse.h"
#include "stmdfu.h"

int main(int argc, char * argv[])
{	
	dfu_device * dfudev = stmdfu_init_dfu();
	
	if (!strcmp(argv[1], "flash"))
	{
		stmdfu_write_image(dfudev, argv[2]);
	}
	
	if (!strcmp(argv[1], "dump"))
	{
		int address = strtol(argv[2], NULL, 0);
		int size = strtol(argv[3], NULL, 0);
		
		if (address < 0)
			address = 0;
		
		if (size < 1)
			size = 1;
		
		stmdfu_read_flash(dfudev, address, size);
	}
	
	if (!strcmp(argv[1], "optbytes"))
	{
		stmdfu_read_optbytes(dfudev);
	}
	
	if (!strcmp(argv[1], "erase"))
	{
		int address = strtol(argv[2], NULL, 0);
		
		if (address < 0)
			address = 0;
		
		stmdfu_erase(dfudev, address);
	}
	
	if (!strcmp(argv[1], "masserase"))
	{
		stmdfu_mass_erase(dfudev);
	}
	
	cleanup(dfudev);
	
	return 0;
}

/*
stmdfu_write_image() is a wrapper function that extracts an image from
a dfuse file, and flashes it to an attached stm32 device via usb dfu.
*/
void stmdfu_write_image(dfu_device * dfudev, char * file)
{
	int i,j;
	int writesize;
	
	int dfufile = open(file, O_RDONLY);
	if (dfufile < 0)
	{
		printf("error opening <%s>\n", file);
	}
	
	dfuse_file * dfusefile = (dfuse_file *)malloc(sizeof(dfuse_file));
	dfusefile->prefix = (dfuse_prefix *)malloc(sizeof(dfuse_prefix));
	
	dfuse_readprefix(dfusefile, dfufile);
	
	dfusefile->images = (dfuse_image **)malloc(sizeof(dfuse_image *) * dfusefile->prefix->targets);
	for (i=0; i<dfusefile->prefix->targets; i++)
	{
		dfusefile->images[i] = (dfuse_image *)malloc(sizeof(dfuse_image));
		dfusefile->images[i]->tarprefix = (dfuse_target_prefix *)malloc(sizeof(dfuse_target_prefix));
		
		dfuse_readtarprefix(dfusefile, dfufile);
		
		dfusefile->images[i]->imgelement = (dfuse_image_element **)malloc(sizeof(dfuse_image_element *) * dfusefile->images[i]->tarprefix->num_elements);
		for (j=0; j<dfusefile->images[i]->tarprefix->num_elements; j++)
		{
			dfusefile->images[i]->imgelement[j] = (dfuse_image_element *)malloc(sizeof(dfuse_image_element));
			dfuse_readimgelement_meta(dfusefile, dfufile);
			dfusefile->images[i]->imgelement[j]->data = (uint8_t *)malloc(sizeof(uint8_t) * dfusefile->images[i]->imgelement[j]->element_size);
			dfuse_readimgelement_data(dfusefile, dfufile);
		}
	}
	
	dfusefile->suffix = (dfuse_suffix *)malloc(sizeof(dfuse_suffix));
	
	dfuse_readsuffix(dfusefile, dfufile);
	
	dfu_set_address_pointer(dfudev, dfusefile->images[0]->imgelement[0]->element_address);
	
	printf("address pointer set\n");
	
	dfu_make_idle(dfudev, 0);
	
	printf("made idle\n");
	
	writesize = dfusefile->images[0]->imgelement[0]->element_size / 2048;
	writesize = (writesize + 1) * 2048;
	
	dfu_write_flash(dfudev, dfusefile->images[0]->imgelement[0]->data, writesize);
	
	dfuse_struct_cleanup(dfusefile);
}

/*
stmdfu_read_flash() is a wrapper function that reads size bytes of memory
from address on an stm32 device via dfu.
*/
void stmdfu_read_flash(dfu_device * dfudev, int address, int size)
{
	int i, j;
	
	uint8_t * memdump;
	
	memdump = (uint8_t *)calloc(size, sizeof(uint8_t));
	
	dfu_set_address_pointer(dfudev, address);
	
	dfu_make_idle(dfudev, 0);
	
	dfu_read_flash(dfudev, memdump, size);
	
	for (i=0; i<ceil(size/10.); i++)
	{
		for (j=0; j<10; j++)
		{
			if ((i*10+j) < size)
			{
				printf("0x%.2X ", memdump[i*10+j]);
			} else {
			}
		}
		printf("\n");
	}
	
	free(memdump);
}

/*
stmdfu_read_optbytes() is a wrapper function that reads the option bytes
from an stm32 device via dfu.
*/
void stmdfu_read_optbytes(dfu_device * dfudev)
{
	int i;
	
	uint8_t optbytes[16];
	
	dfu_read_optbytes(dfudev, optbytes);
	
	printf("optbytes:\n");
	
	for (i=0; i<16; i+=2)
	{
		printf("0x%.2x\t 0x%.2x\n", optbytes[i], optbytes[i+1]);
	}
}

/*
stmdfu_erase() is a wrapper function that erases 1 page of flash at a
time on an stm32 device via dfu.
*/
void stmdfu_erase(dfu_device * dfudev, int address)
{
	dfu_erase(dfudev, address);
}

/*
stmdfu_mass_erase() is a wrapper function that erases all flash memory
of an stm32 device via dfu.
*/
void stmdfu_mass_erase(dfu_device * dfudev)
{
	dfu_mass_erase(dfudev);
}

/*
stmdfu_init_dfu() sets up an attached stm32 dfu device and puts it in
an idle state, so it's ready to handle dfu commands.
*/
dfu_device * stmdfu_init_dfu()
{
	dfu_device * dfudev = find_dfu_device();
	
	libusb_set_interface_alt_setting(dfudev->handle, 0, 0);
	
	//now we've got a handle to the DFU device we want to deal with
	
	if(!dfu_make_idle(dfudev, 0))
	{
		#if STMDFU_DEBUG_PRINTFS
		printf("entered dfuIDLE state\n");
		#endif
	}
	
	return dfudev;
}

/*
find_dfu_device() searches through the tree of attached usb devices,
and finds any attached stm32 dfu devices (by vendor and product id).
*/
dfu_device * find_dfu_device()
{
	libusb_device ** devlist;
	libusb_device * dfutemp;
	dfu_device * dfudev;
	libusb_device_handle * dfuhandle;
	struct libusb_device_descriptor devdesc;
	struct libusb_config_descriptor * cfgdesc;
	ssize_t nlistdevs;
	int i, j, k, l;
	int err;
	int ndfudevs = 0;
	unsigned char strdesc[100];
	
	dfudev = (dfu_device *)malloc(sizeof(dfu_device));
	
	libusb_init(NULL);
	
	nlistdevs = libusb_get_device_list(NULL, &devlist);
	if (nlistdevs < 0)
	{
		printf("error getting device list\n");
		exit(-1);
	}
	
	for (i=0; i<nlistdevs; i++)
	{
		if (libusb_get_device_descriptor(devlist[i], &devdesc))
		{
			printf("failed to get device descriptor\n");
		}
			
		if ((devdesc.idVendor == STM32VENDOR) && (devdesc.idProduct == STM32PRODUCT))
		{
			err = libusb_open(devlist[i], &dfuhandle);
			if (err)
			{
				printf("error opening device handle <%d>\n", i);
			}
			
			//according to DFU 1.1 standard, a DFU device in DFU Mode
			//will have only one each of a configuration and interface.
			//but we'll parse as if there are multiple anyways!
			
			//iterate through available configurations
			for (j=0; j<devdesc.bNumConfigurations; j++)
			{
				if (libusb_get_config_descriptor(devlist[i], j, &cfgdesc))
				{
					printf("failed to get config descriptor %d::%d\n", i, j);
				}
				//iterate through available interfaces
				for (k=0; k<cfgdesc->bNumInterfaces; k++)
				{
					//iterate through available alternate settings
					for (l=0; l<cfgdesc->interface[k].num_altsetting; l++)
					{
						libusb_get_string_descriptor_ascii(dfuhandle,
															cfgdesc->interface[k].altsetting[l].iInterface,
													  		strdesc,
													  		100);
						if (cfgdesc->interface[k].altsetting[l].bInterfaceClass == DFU_ITF_CLASS &&
							cfgdesc->interface[k].altsetting[l].bInterfaceSubClass == DFU_ITF_SUBCLASS &&
							cfgdesc->interface[k].altsetting[l].bInterfaceProtocol == DFU_ITF_PROTOCOL &&
							!strncmp(strdesc, "@Internal Flash", 15))
						{
							#if STMDFU_DEBUG_PRINTFS
							printf("\ndevice:\n");
							printf("vendor:product <%x>:<%x>\n", devdesc.idVendor, devdesc.idProduct);
							printf("class:subclass <%x>:<%x>\n", devdesc.bDeviceClass, devdesc.bDeviceSubClass);
							printf("usbspec:configs <%x>:<%x>\n", devdesc.bcdUSB, devdesc.bNumConfigurations);
							
							printf("interface:\n");
							printf("<%d>::<%d>::<%d>\n\n",
									cfgdesc->interface[k].altsetting[l].bInterfaceClass,
									cfgdesc->interface[k].altsetting[l].bInterfaceSubClass,
									cfgdesc->interface[k].altsetting[l].bInterfaceProtocol);
							#endif
							ndfudevs++;
							dfutemp = devlist[i];
							dfudev->interface = k;
						}
					}
				}
				libusb_free_config_descriptor(cfgdesc);
			}
			libusb_close(dfuhandle);
		}
	}
	
	//calling function will need to call libusb_close(dfudev->handle)
	err = libusb_open(dfutemp, &dfudev->handle);
	if (err)
	{
		printf("error opening dfudev handle \n");
	}
	libusb_free_device_list(devlist, 1);
	
	if (ndfudevs < 1)
	{
		printf("No STM32 DFU Device connected. Check boot switches and replugin board.\n");
		exit(-1);
	}
	
	if (ndfudevs > 1)
	{
		printf("More than 1 STM32 DFU device connected. Targetting last enumerated STM32 DFU device.\n");
	}
	
	err = libusb_claim_interface(dfudev->handle, dfudev->interface);
	
	if (err == LIBUSB_ERROR_BUSY)
	{
		printf("STM32 DFU device: interface already claimed\n");
	} else if (err)
	{
		printf("STM32 DFU device: interface can't be claimed\n");
	}
	
	return dfudev;
}

/*
cleanup() releases any usb handles/interfaces and deallocates memory.
*/
void cleanup(dfu_device * dfudev)
{
	libusb_release_interface(dfudev->handle, dfudev->interface);
	libusb_close(dfudev->handle);
	free(dfudev);
	libusb_exit(NULL);
}