/*
stmdfu.c :
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
#include "dfurequests.h"
#include "dfucommands.h"
#include "dfuse.h"

#define STM32VENDOR 0x0483
#define STM32PRODUCT 0xdf11

dfu_device * find_dfu_device();
void cleanup(dfu_device * dfudev);

int main(int argc, char * argv[])
{
	dfu_device * dfudev;
	libusb_device_handle * dfuhandle;
	int i, j;
	int err;
	dfu_status status;
	int dfufile;
	
	int writesize;
	
	long dumpaddr;
	long dumpsize;
	
	dfudev = find_dfu_device();
	
	libusb_set_interface_alt_setting(dfudev->handle, 0, 0);
	
	//now we've got a handle to the DFU device we want to deal with
	
	if(!dfu_make_idle(dfudev, 0))
	{
		printf("entered dfuIDLE state\n");
	}
	
	if (!strcmp(argv[1], "flash"))
	{
		dfufile = open(argv[2], O_RDONLY);
		if (dfufile < 0)
		{
			printf("error opening <%s>\n", argv[1]);
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
		
		dfu_write_memory(dfudev, dfusefile->images[0]->imgelement[0]->data, writesize);
		
		dfuse_struct_cleanup(dfusefile);
	}
	
	if (!strcmp(argv[1], "dump"))
	{
		uint8_t * memdump;
		
		dumpaddr = strtol(argv[2], NULL, 0);
		dumpsize = strtol(argv[3], NULL, 0);
		
// 		memdump = (uint8_t *)malloc(sizeof(uint8_t) * 1000);
		memdump = (uint8_t *)calloc(dumpsize, sizeof(uint8_t));
		
		dfu_set_address_pointer(dfudev, dumpaddr);
		
		dfu_make_idle(dfudev, 0);
		
		dfu_read_flash(dfudev, memdump, dumpsize);
		
		for (i=0; i<dumpsize/10; i++)
		{
			for (j=0; j<10; j++)
			{
				printf("0x%.2X ", memdump[i*10+j]);
			}
			printf("\n");
		}
	}
	
	if (!strcmp(argv[1], "optbytes"))
	{
		uint8_t optbytes[16];
		
		dfu_read_optbytes(dfudev, optbytes);
		
		printf("optbytes:\n");
		
		for (i=0; i<16; i+=2)
		{
			printf("0x%.2x\t 0x%.2x\n", optbytes[i], optbytes[i+1]);
		}
	}
	
	if (!strcmp(argv[1], "erase"))
	{
		dfu_erase(dfudev, strtol(argv[2], NULL, 0));
	}
	
	if (!strcmp(argv[1], "masserase"))
	{
		dfu_mass_erase(dfudev);
	}
	
	cleanup(dfudev);
	
	return 0;
}

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

								if (cfgdesc->interface[k].altsetting[l].bInterfaceClass == DFU_ITF_CLASS &&
									cfgdesc->interface[k].altsetting[l].bInterfaceSubClass == DFU_ITF_SUBCLASS &&
									cfgdesc->interface[k].altsetting[l].bInterfaceProtocol == DFU_ITF_PROTOCOL)
								{
									printf("\ndevice:\n");
									printf("vendor:product <%x>:<%x>\n", devdesc.idVendor, devdesc.idProduct);
									printf("class:subclass <%x>:<%x>\n", devdesc.bDeviceClass, devdesc.bDeviceSubClass);
									printf("usbspec:configs <%x>:<%x>\n", devdesc.bcdUSB, devdesc.bNumConfigurations);
									
									printf("interface:\n");
									printf("<%d>::<%d>::<%d>\n\n",
											cfgdesc->interface[k].altsetting[l].bInterfaceClass,
											cfgdesc->interface[k].altsetting[l].bInterfaceSubClass,
											cfgdesc->interface[k].altsetting[l].bInterfaceProtocol);
									
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
	
	if (ndfudevs > 1)
	{
		printf("More than 1 STM32 DFU device connected.\nTargetting last enumerated STM32 DFU device.\n");
	}
	
	if (!libusb_claim_interface(dfudev->handle, dfudev->interface))
	{
		printf("STM32 DFU device: interface already claimed\n");
	}
	
	return dfudev;
}

void cleanup(dfu_device * dfudev)
{
	libusb_release_interface(dfudev->handle, dfudev->interface);
	libusb_close(dfudev->handle);
	free(dfudev);
	libusb_exit(NULL);
}