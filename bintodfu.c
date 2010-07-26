#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "crc32.h"
#include "dfuse.h"

int main(int argc, char * argv[])
{
	int i, j;
	int binfile;
	int dfufile;
	dfuse_file * dfusefile;
	
	binfile = open("pathto.bin", O_RDONLY);
	dfufile = open("pathto.dfuse", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
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