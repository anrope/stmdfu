#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <sys/types.h>

#define STM32VENDOR 0xffff
#define STM32PRODUCT 0xffff

int main(int argc, char * argv[])
{
	libusb_device ** devlist;
	libusb_device * dfudev;
	libusb_device_handle * dfuhandle;
	struct libusb_device_descriptor devdesc;
	ssize_t nlistdevs;
	int i;
	int err;
	//malloc of size 0 is necessary for initial free() to be successful
	int * devs = (int *)malloc(0);
	int ndevs = 0;
	
	libusb_init(NULL);
	
	nlistdevs = libusb_get_device_list(NULL, &devlist);
	if (ndevs < 0)
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
			ndevs++;
			free(devs);
			devs = (int *)malloc(sizeof(int)*ndevs);
			devs[ndevs-1] = i;
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
	
	printf("hi\n");
	
	free(devs);
	
	libusb_free_device_list(devlist, 1);
	
	libusb_exit(NULL);
	
	return 0;
}
	
	