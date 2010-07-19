/* Time (in ms) for the device to wait for the usb reset after being told to detach
* before the giving up going into dfu mode. */
#define DFU_DETACH_TIMEOUT 1000

int32_t dfu_read_memory(dfu_device * device, int8_t * membuf, int32_t length);
int32_t dfu_read_memory_optbytes(dfu_device * device, int8_t * membuf, int32_t length);
int32_t dfu_get(dfu_device * device, uint8_t * data);

int32_t dfu_write_memory(dfu_device * device);
int32_t dfu_set_address_pointer(dfu_device * device, int32_t address);
int32_t dfu_erase(dfu_device * device, int32_t address);
int32_t dfu_erase_mass(dfu_device * device);
int32_t dfu_read_unprotect(dfu_device * device);
int32_t dfu_leave_dfu_mode(dfu_device * device);

static int32_t dfu_make_idle( dfu_device *device, const int initial_abort );
