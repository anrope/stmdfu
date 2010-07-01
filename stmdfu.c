#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <sys/types.h>

#define STM32VENDOR 0x15ba
#define STM32PRODUCT 0x3
#define LENITFDATA 300

int main(int argc, char * argv[])
{
	libusb_device ** devlist;
	libusb_device * dfudev;
	libusb_device_handle * dfuhandle;
	struct libusb_device_descriptor devdesc;
	struct libusb_config_descriptor * config;
	struct libusb_interface_descriptor * itf;
	ssize_t nlistdevs;
	int i, j, k;
	int err;
	int ct;
	int ndfudevs = 0;
	char * itfdata = (char *)malloc(sizeof(char)*LENITFDATA);
	
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
				printf("error opening dfu device handle\n");
				exit(-1);
			}

			for (j=0; j<devdesc.bNumConfigurations; j++)
			{
				if (libusb_get_config_descriptor(devlist[i], j, &config))
				{
					printf("failed to get config descriptor %d::%d\n", i, j);
				}
				for (k=0; k<config->bNumInterfaces; k++)
				{
					
					ct = libusb_get_descriptor(dfuhandle, LIBUSB_DT_INTERFACE, k, itfdata, LENITFDATA);
					printf("{\n%s\n}\n", itfdata);
					itf = itfdata;
					printf("<%d>::<%d>\n", itf->bInterfaceClass, itf->bInterfaceProtocol);
				}
				libusb_free_config_descriptor(config);
			}
			libusb_close(dfuhandle);
		}
	}	
	
	if (dfudev)
	{
		err = libusb_open(dfudev, &dfuhandle);
		if (err)
		{
			printf("error opening dfu device handle\n");
			exit(-1);
		}
		libusb_close(dfuhandle);
	}
	
	libusb_free_device_list(devlist, 1);
	
	libusb_exit(NULL);
	
	return 0;
}
	
	