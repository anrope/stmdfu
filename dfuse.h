//DfuSe file structure referenced from STM document UM0391
//see UM0391 for descriptions of all fields

#define uint8_t u_int8_t
#define uint16_t u_int16_t
#define uint32_t u_int32_t

#define STMDFU_PREFIXLEN 11
#define STMDFU_SUFFIXLEN 16
#define STMDFU_TARPREFIXLEN 274

#define READBIN_READLEN 100

#define DFUWRITE(var) (write(dfufile, &(var), sizeof(var)))
#define DFUREAD(var) (read(dfufile, &(var), sizeof(var)))

typedef struct {
	char signature[5];
	uint8_t version;
	uint32_t dfu_image_size;
	uint8_t targets;
} dfuse_prefix;

typedef struct {
	uint8_t device_low;
	uint8_t device_high;
	uint8_t product_low;
	uint8_t product_high;
	uint8_t vendor_low;
	uint8_t vendor_high;
	uint8_t dfu_low;
	uint8_t dfu_high;
	char dfu_signature[3];
	uint8_t suffix_length;
	uint32_t crc;
} dfuse_suffix;

typedef struct {
	char signature[6];
	uint8_t alternate_setting;
	int32_t target_named;
	char target_name[255];
	uint32_t target_size;
	uint32_t num_elements;
} dfuse_target_prefix;

typedef struct {
	uint32_t element_address;
	uint32_t element_size;
	uint8_t * data;
} dfuse_image_element;

typedef struct {
	dfuse_target_prefix * tarprefix;
	dfuse_image_element ** imgelement;
} dfuse_image;

typedef struct {
	dfuse_prefix * prefix;
	dfuse_image ** images;
	dfuse_suffix * suffix;
} dfuse_file;

dfuse_file * dfuse_init(int binfile);
void dfuse_readbin(dfuse_file * dfusefile, int binfile);
int dfuse_writeprefix(dfuse_file * dfusefile, int dfufile);
int dfuse_readprefix(dfuse_file * dfusefile, int dfufile);
int dfuse_writetarprefix(dfuse_file * dfusefile, int dfufile);
int dfuse_readtarprefix(dfuse_file * dfusefile, int dfufile);
int dfuse_writeimgelement(dfuse_file * dfusefile, int dfufile);
int dfuse_readimgelement_meta(dfuse_file * dfusefile, int dfufile);
int dfuse_readimgelement_data(dfuse_file * dfusefile, int dfufile);
int dfuse_writesuffix(dfuse_file * dfusefile, int dfufile);
int dfuse_readsuffix(dfuse_file * dfusefile, int dfufile);
void calccrc(dfuse_file * dfusefile, int dfufile);
void dfuse_struct_cleanup(dfuse_file * dfusefile);