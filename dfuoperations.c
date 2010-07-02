#include "dfurequests.h"

/*
*  Gets the device into the dfuIDLE state if possible.
*
*  device    - the dfu device to commmunicate with
*
*  returns 0 on success, 1 if device was reset, error otherwise
*/
static int32_t dfu_make_idle( dfu_device *device, const dfu_bool initial_abort )
{
	dfu_status status;
	int32_t retries = 4;
	
	if( true == initial_abort ) {
		dfu_abort( device );
	}
	
	while( 0 < retries ) {
		if( 0 != dfu_get_status(device, &status) ) {
			dfu_clear_status( device );
			continue;
		}
		
		switch( status.bState ) {
			case STATE_DFU_IDLE:
				if( DFU_STATUS_OK == status.bStatus ) {
					return 0;
				}
				
				/* We need the device to have the DFU_STATUS_OK status. */
				dfu_clear_status( device );
				break;
				
			case STATE_DFU_DOWNLOAD_SYNC:   /* abort -> idle */
			case STATE_DFU_DOWNLOAD_IDLE:   /* abort -> idle */
			case STATE_DFU_MANIFEST_SYNC:   /* abort -> idle */
			case STATE_DFU_UPLOAD_IDLE:     /* abort -> idle */
			case STATE_DFU_DOWNLOAD_BUSY:   /* abort -> error */
			case STATE_DFU_MANIFEST:        /* abort -> error */
				dfu_abort( device );
				break;
				
			case STATE_DFU_ERROR:
				dfu_clear_status( device );
				break;
				
			case STATE_APP_IDLE:
				dfu_detach( device, DFU_DETACH_TIMEOUT );
				break;
				
			case STATE_APP_DETACH:
			case STATE_DFU_MANIFEST_WAIT_RESET:
				usb_reset( device->handle );
				return 1;
		}
		
		retries--;
	}
	
	return -2;
}