//DfuSe file structure referenced from STM document UM0391
//see UM0391 for descriptions of all fields

typedef struct {
	char signature[5];
	int8_t version;
	int32_t dfu_image_size;
	int8_t targets;
} dfuse_prefix;

typedef struct {
	int8_t device_low;
	int8_t device_high;
	int8_t product_low;
	int8_t product_high;
	int8_t vendor_low;
	int8_t vendor_high;
	int8_t dfu_low;
	int8_t dfu_high;
	char dfu_signature[3];
	int8_t suffix_length;
	int32_t crc;
} dfuse_suffix;

typedef struct {
	char signature[6];
	int8_t alternate_setting;
	int32_t target_named;
	char target_name[255];
	int32_t target_size;
	int32_t num_elements;
} dfuse_target_prefix;

typedef struct {
	int32_t element_address;
	int32_t element_size;
	int32_t * data;
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