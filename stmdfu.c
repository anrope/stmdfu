#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <sys/types.h>
#include "dfurequests.h"

#define STM32VENDOR 0x424
#define STM32PRODUCT 0xa700

int main(int argc, char * argv[])
{
	libusb_device ** devlist;
	libusb_device * dfudev;
	libusb_device_handle * dfuhandle;
	struct libusb_device_descriptor devdesc;
	struct libusb_config_descriptor * cfgdesc;
	ssize_t nlistdevs;
	int i, j, k, l;
	int err;
	int ndfudevs = 0;
		
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
		
		printf("\ndevice:\n");
		printf("vendor:product <%x>:<%x>\n", devdesc.idVendor, devdesc.idProduct);
		printf("class:subclass <%x>:<%x>\n", devdesc.bDeviceClass, devdesc.bDeviceSubClass);
		printf("usbspec:configs <%x>:<%x>\n", devdesc.bcdUSB, devdesc.bNumConfigurations);
		
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
						printf("<%d>::<%d>::<%d>\n\n",
						cfgdesc->interface[k].altsetting[l].bInterfaceClass,
						cfgdesc->interface[k].altsetting[l].bInterfaceSubClass,
						cfgdesc->interface[k].altsetting[l].bInterfaceProtocol);
						
						if (cfgdesc->interface[k].altsetting[l].bInterfaceClass == DFU_ITF_CLASS &&
							cfgdesc->interface[k].altsetting[l].bInterfaceSubClass == DFU_ITF_SUBCLASS &&
							cfgdesc->interface[k].altsetting[l].bInterfaceProtocol == DFU_ITF_PROTOCOL)
						{
							ndfudevs++;
							dfudev = devlist[i];
						}
					}
				}
				libusb_free_config_descriptor(cfgdesc);
			}
			libusb_close(dfuhandle);
		}
	}
	
	if (ndfudevs > 1)
	{
		printf("More than 1 STM32 DFU device connected.\nTargetting last enumerated STM32 DFU device.\n");
	}
	
	if (dfudev)
	{
		err = libusb_open(dfudev, &dfuhandle);
		if (err)
		{
			printf("error opening dfu device handle\n");
			exit(-1);
		}
		libusb_free_device_list(devlist, 1);
		
		//now we've got a handle to the DFU device we want to deal with
		
		libusb_close(dfuhandle);
	} else {
		libusb_free_device_list(devlist, 1);
	}
	
	libusb_exit(NULL);
	
	return 0;
}
	
	