#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dfuse.h"

int main(int argc, char * argv[])
{
	int i, j;
	
	int num_images = 1;
	int num_imgelements = 1;
	
	int binfile;
	int dfufile;
	
	struct stat stat;
	
	dfuse_file * dfusefile = (dfuse_file *)malloc(sizeof(dfuse_file));
	dfusefile->prefix = (dfuse_prefix *)malloc(sizeof(dfuse_prefix));
	dfusefile->images = (dfuse_image **)malloc(sizeof(dfuse_image *) * num_images);
	for (i=0; i<num_images; i++)
	{
		dfusefile->images[i] = (dfuse_image *)malloc(sizeof(dfuse_image));
		dfusefile->images[i]->tarprefix = (dfuse_target_prefix *)malloc(sizeof(dfuse_target_prefix));
		dfusefile->images[i]->imgelement = (dfuse_image_element **)malloc(sizeof(dfuse_image_element *) * num_imgelements);
		//num_imgelements should really be dfusefile->images[i]->tarprefix->num_elements
		for (j=0; j<num_imgelements; j++)
		{
			dfusefile->images[i]->imgelement[j] = (dfuse_image_element *)malloc(sizeof(dfuse_image_element));
 			//space for actual image element data will probably be easier to allocate later (or just write to file)
			//dfusefile->images[i]->imgelement[j]->data = (int32_t *)malloc(sizeof(int32_t) * imgelement size);
		}
	}
	dfusefile->suffix = (dfuse_suffix *)malloc(sizeof(dfuse_suffix));
	
	//set predetermined prefix values
	dfusefile->prefix->signature[0] = 'D';
	dfusefile->prefix->signature[1] = 'f';
	dfusefile->prefix->signature[2] = 'u';
	dfusefile->prefix->signature[3] = 'S';
	dfusefile->prefix->signature[4] = 'e';
	dfusefile->prefix->version = 0x01;
	
	//set predetermined suffix values
	dfusefile->suffix->device_low = 0xff;
	dfusefile->suffix->device_high = 0xff;
	dfusefile->suffix->product_low = 0xff;
	dfusefile->suffix->product_high = 0xff;
	dfusefile->suffix->vendor_low = 0xff;
	dfusefile->suffix->vendor_high = 0xff;
	dfusefile->suffix->dfu_low = 0x1a;
	dfusefile->suffix->dfu_high = 0x01;
	dfusefile->suffix->dfu_signature[0] = 'U';
	dfusefile->suffix->dfu_signature[1] = 'F';
	dfusefile->suffix->dfu_signature[2] = 'D';
	dfusefile->suffix->suffix_length = 16;
	
	//set predetermined values for target prefixes for each image
	for (i=0; i<num_images; i++)
	{
		dfusefile->images[i]->tarprefix->signature[0] = 'T';
		dfusefile->images[i]->tarprefix->signature[1] = 'a';
		dfusefile->images[i]->tarprefix->signature[2] = 'r';
		dfusefile->images[i]->tarprefix->signature[3] = 'g';
		dfusefile->images[i]->tarprefix->signature[4] = 'e';
		dfusefile->images[i]->tarprefix->signature[5] = 't';
		dfusefile->images[i]->tarprefix->alternate_setting = 0;
		dfusefile->images[i]->tarprefix->target_named = 1;
		strcpy(dfusefile->images[i]->tarprefix->target_name, "stm-arp-target-name");
	}
	
	//set more values assuming just 1 image and 1 image element
	dfusefile->prefix->targets = 1;
		
	for (i=0; i<num_images; i++)
	{
		dfusefile->images[i]->tarprefix->num_elements = 1;
	}
	
	/*
	remaining fields:
	length of image excluding target prefix
	dfusefile->images[i]->tarprefix->target_size
	
	dfusefile->images[i]->imgelement[j]->element_address
	dfusefile->images[i]->imgelement[j]->element_size
	dfusefile->images[i]->imgelement[j]->data
	
	total DFU file length in bytes
	dfusefile->prefix->dfu_image_size
	
	dfusefile->suffix->crc
	*/
	
	binfile = open("pathto.bin", O_RDONLY);
	dfufile = open("pathto.dfuse", O_WRONLY | O_CREAT);
	
	fstat(binfile, &stat);
	
	printf("<%i>\n", stat.st_size);
	
	for (i=0; i<num_images; i++)
	{
		for (j=0; j<num_imgelements; j++)
		{
			//free(dfusefile->images[i]->imgelement[j]->data);
			free(dfusefile->images[i]->imgelement[j]);
		}
		free(dfusefile->images[i]->tarprefix);
		free(dfusefile->images[i]->imgelement);
		free(dfusefile->images[i]);
	}
	
	free(dfusefile->images);
	free(dfusefile->suffix);
	free(dfusefile->prefix);
	free(dfusefile);
	
	close(binfile);
	close(dfufile);
	
	return 0;
}
	
	