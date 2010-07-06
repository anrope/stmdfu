#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "crc32.h"
#include "dfuse.h"

#define READLEN 100

#define DFUWRITE(var) (write(dfufile, &(var), sizeof(var)))

dfuse_file * dfuse_init(int binfile);
void dfuse_readbin(dfuse_file * dfusefile, int binfile);
int dfuse_writeprefix(dfuse_file * dfusefile, int dfufile);
int dfuse_writetarprefix(dfuse_file * dfusefile, int dfufile);
int dfuse_writeimgelement(dfuse_file * dfusefile, int dfufile);
int dfuse_writesuffix(dfuse_file * dfusefile, int dfufile);
void dfuse_cleanup(dfuse_file * dfusefile);

int main(int argc, char * argv[])
{
	int i, j;
	int binfile;
	int dfufile;
	dfuse_file * dfusefile;
	
	binfile = open("pathto.bin", O_RDONLY);
	dfufile = open("pathto.dfuse", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
	chksum_crc32gentab();
	
	dfusefile = dfuse_init(binfile);
	
	printf("<%i> :: <%i> :: <%i>\n", dfusefile->images[0]->tarprefix->target_size, \
		dfusefile->images[0]->imgelement[0]->element_size, \
		dfusefile->prefix->dfu_image_size);
	printf("Checksum should be: 0xa8775a06\n");
	
	dfuse_readbin(dfusefile, binfile);
	
	dfuse_writeprefix(dfusefile, dfufile);
	
	dfuse_writetarprefix(dfusefile, dfufile);
	
	dfuse_writeimgelement(dfusefile, dfufile);
	
	dfuse_writesuffix(dfusefile, dfufile);
	
	printf("Checksum: <%x>\n", dfusefile->suffix->crc);
	
	dfuse_cleanup(dfusefile);
	
	close(binfile);
	close(dfufile);
	
	return 0;
}

dfuse_file * dfuse_init(int binfile)
{
	int i, j;
	int num_images = 1;
	int num_imgelements = 1;	
	char * stmjunk = "abababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababab";
	struct stat stat;
	
	//allocate memory
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
// 		strcpy(dfusefile->images[i]->tarprefix->target_name, "stm-arp-target-name");
		strcpy(dfusefile->images[i]->tarprefix->target_name, stmjunk);
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
	
	fstat(binfile, &stat);
	
	dfusefile->images[0]->tarprefix->target_size = stat.st_size +
	sizeof(dfusefile->images[0]->imgelement[0]->element_address) +
	sizeof(dfusefile->images[0]->imgelement[0]->element_size);
	
	dfusefile->images[0]->imgelement[0]->element_address = 0x08000000;
	dfusefile->images[0]->imgelement[0]->element_size = stat.st_size;
	dfusefile->images[0]->imgelement[0]->data = (int8_t *)malloc(sizeof(int8_t) * dfusefile->images[0]->imgelement[0]->element_size);
	
	//From looking at dfuse file generated by STM's dfuse packager,
	//the image size does not include the standard DFU suffix
	// 	dfusefile->prefix->dfu_image_size = PREFIXLEN + SUFFIXLEN;
	dfusefile->prefix->dfu_image_size = PREFIXLEN;
	dfusefile->prefix->dfu_image_size += TARPREFIXLEN * num_images;
	for (i=0; i<num_images; i++)
	{
		dfusefile->prefix->dfu_image_size += dfusefile->images[i]->tarprefix->target_size;
	}
	
	return dfusefile;
}

void dfuse_readbin(dfuse_file * dfusefile, int binfile)
{
	int i, j;
	
	//read binary into data array
	i = read(binfile, &dfusefile->images[0]->imgelement[0]->data[0], READLEN);
	
	//when read returns < READLEN, we've hit EOF on previous read (or error...)
	for (j=i; i==READLEN; j+=READLEN)
	{
		i = read(binfile, &dfusefile->images[0]->imgelement[0]->data[j], READLEN);
	}
}

int dfuse_writeprefix(dfuse_file * dfusefile, int dfufile)
{
	int ct = 0;
	
	ct = DFUWRITE(dfusefile->prefix->signature);
	ct += DFUWRITE(dfusefile->prefix->version);
	ct += DFUWRITE(dfusefile->prefix->dfu_image_size);
	ct += DFUWRITE(dfusefile->prefix->targets);
	
	if (ct != PREFIXLEN)
		ct = -1;
	
	return ct;
}

int dfuse_writetarprefix(dfuse_file * dfusefile, int dfufile)
{
	int ct = 0;
	
	ct = DFUWRITE(dfusefile->images[0]->tarprefix->signature);
	ct += DFUWRITE(dfusefile->images[0]->tarprefix->alternate_setting);
	ct += DFUWRITE(dfusefile->images[0]->tarprefix->target_named);
	ct += DFUWRITE(dfusefile->images[0]->tarprefix->target_name);
	ct += DFUWRITE(dfusefile->images[0]->tarprefix->target_size);
	ct += DFUWRITE(dfusefile->images[0]->tarprefix->num_elements);
	
	if (ct != TARPREFIXLEN)
		ct = -1;
	
	return ct;
}

int dfuse_writeimgelement(dfuse_file * dfusefile, int dfufile)
{
	int j;
	int ct = 0;
	
	ct += DFUWRITE(dfusefile->images[0]->imgelement[0]->element_address);
	ct += DFUWRITE(dfusefile->images[0]->imgelement[0]->element_size);
	for (j=0; j<dfusefile->images[0]->imgelement[0]->element_size; j++)
	{
		ct += write(dfufile, &dfusefile->images[0]->imgelement[0]->data[j], 1);
	}
	
	if (ct != dfusefile->images[0]->imgelement[0]->element_size + \
		sizeof(dfusefile->images[0]->imgelement[0]->element_address) + \
		sizeof(dfusefile->images[0]->imgelement[0]->element_size))
	{
		ct = -1;
	}
	
	return ct;
}

int dfuse_writesuffix(dfuse_file * dfusefile, int dfufile)
{
	char * crcbuf;
	int ct = 0;
	
	ct = DFUWRITE(dfusefile->suffix->device_low);
	ct += DFUWRITE(dfusefile->suffix->device_high);
	ct += DFUWRITE(dfusefile->suffix->product_low);
	ct += DFUWRITE(dfusefile->suffix->product_high);
	ct += DFUWRITE(dfusefile->suffix->vendor_low);
	ct += DFUWRITE(dfusefile->suffix->vendor_high);
	ct += DFUWRITE(dfusefile->suffix->dfu_low);
	ct += DFUWRITE(dfusefile->suffix->dfu_high);
	ct += DFUWRITE(dfusefile->suffix->dfu_signature);
	ct += DFUWRITE(dfusefile->suffix->suffix_length);
	
// 	int offset1 = 0;
// 	int offset2 = PREFIXLEN - 1;
// 	int offset3 = PREFIXLEN + TARPREFIXLEN - 1;
// 	int offset4 = PREFIXLEN + TARPREFIXLEN + 8 - 1;
// 	
// 	lseek(dfufile, 0, SEEK_SET);
// 	crcbuf = (char *)malloc(sizeof(char)*fptr);
// 	if (fptr != read(dfufile, crcbuf, fptr))
// 		printf("UHOHHHHH\n");
// 	lseek(dfufile, 0, SEEK_END);
// 	u_int32_t crc1 = chksum_crc32(&crcbuf[offset1], (fptr-offset1));
// 	u_int32_t crc2 = chksum_crc32(&crcbuf[offset2], (fptr-offset2+1));
// 	u_int32_t crc3 = chksum_crc32(&crcbuf[offset3], (fptr-offset3+1));
// 	u_int32_t crc4 = chksum_crc32(&crcbuf[offset4], (fptr-offset4+1));
// 	dfusefile->suffix->crc = 13;
// 	free(crcbuf);
// 	
// 	printf("<%x>%d :: <%x>%d :: <%x>%d :: <%x>%d of %d\n", crc1, offset1, crc2, offset2, crc3, offset3, crc4, offset4, fptr);


	lseek(dfufile, 0, SEEK_SET);
	crcbuf = (char *)malloc(sizeof(char)*(dfusefile->prefix->dfu_image_size+12));
	if ((dfusefile->prefix->dfu_image_size+12) !=read(dfufile, crcbuf, (dfusefile->prefix->dfu_image_size+12)))
		printf("UHOHHHHHHHH\n");
	lseek(dfufile, 0, SEEK_END);
	dfusefile->suffix->crc = chksum_crc32(crcbuf, (dfusefile->prefix->dfu_image_size+12));
	free(crcbuf);
	
	ct += DFUWRITE(dfusefile->suffix->crc);
	
	return ct;
}

void dfuse_cleanup(dfuse_file * dfusefile)
{
	int i, j;
	
	for (i=0; i<dfusefile->prefix->targets; i++)
	{
		for (j=0; j<dfusefile->images[i]->tarprefix->num_elements; j++)
		{
			free(dfusefile->images[i]->imgelement[j]->data);
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
}
	
	