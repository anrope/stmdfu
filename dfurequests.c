/*
dfurequests.{c,h} :
Implements the lower level DFU requests, which are made up of USB control transfers.

More information on the DFU requests is available in the Universal Serial Bus Device
Class Specification for Device Firmware Upgrade.
*/

/*
 * Updated to use libusb-1, and to take wvalue as an 
 * argument in upload/download
 *
 * arp <anrope@gmail.com>
 *
 * Originally from:
 * dfu-programmer
 *
 * $Id: dfu.c 81 2009-01-22 09:45:15Z schmidtw $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <libusb-1.0/libusb.h>
#include <time.h>
#include "dfurequests.h"

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h>

/*
 *  DFU_DETACH Request (DFU Spec 1.1, Section 5.1)
 *
 *  device    - the dfu device to commmunicate with
 *  timeout   - the timeout in ms the USB device should wait for a pending
 *              USB reset before giving up and terminating the operation
 *
 *  returns 0 or < 0 on error
 */
int32_t dfu_detach( dfu_device *device, const int32_t timeout )
{
    int32_t result;

    if( (NULL == device) || (NULL == device->handle) || (timeout < 0) ) {
        return -1;
    }

    result = libusb_control_transfer( device->handle,
          /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
          /* bRequest      */ DFU_DETACH,
          /* wValue        */ timeout,
          /* wIndex        */ device->interface,
          /* Data          */ NULL,
          /* wLength       */ 0,
                              DFU_TIMEOUT );

    return result;
}

/*
 *  DFU_DNLOAD Request (DFU Spec 1.1, Section 6.1.1)
 *
 *  device    - the dfu device to commmunicate with
 *  length    - the total number of bytes to transfer to the USB
 *              device - must be less than wTransferSize
 *  data      - the data to transfer
 *
 *  returns the number of bytes written or < 0 on error
 */
int32_t dfu_download(dfu_device *device, int32_t wvalue, uint8_t* data, int32_t length)
{
    int32_t result;

    /* Sanity checks */
    if( (NULL == device) || (NULL == device->handle) ) {
        return -1;
    }

    if( (0 != length) && (NULL == data) ) {
        return -2;
    }

    if( (0 == length) && (NULL != data) ) {
        return -3;
    }

    result = libusb_control_transfer( device->handle,
          /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
          /* bRequest      */ DFU_DNLOAD,
          /* wValue        */ wvalue,
          /* wIndex        */ device->interface,
          /* Data          */ (char *) data,
          /* wLength       */ length,
                              DFU_TIMEOUT );

    return result;
}

/*
 *  DFU_UPLOAD Request (DFU Spec 1.1, Section 6.2)
 *
 *  device    - the dfu device to commmunicate with
 *  length    - the maximum number of bytes to receive from the USB
 *              device - must be less than wTransferSize
 *  data      - the buffer to put the received data in
 *
 *  returns the number of bytes received or < 0 on error
 */
int32_t dfu_upload(dfu_device *device, int32_t wvalue, uint8_t* data, int32_t length)
{
    int32_t result;

    /* Sanity checks */
    if( (NULL == device) || (NULL == device->handle) ) {
        return -1;
    }

    if( (0 == length) || (NULL == data) ) {
        return -2;
    }

    result = libusb_control_transfer( device->handle,
          /* bmRequestType */ LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
          /* bRequest      */ DFU_UPLOAD,
          /* wValue        */ wvalue,
          /* wIndex        */ device->interface,
          /* Data          */ (char *) data,
          /* wLength       */ length,
                              DFU_TIMEOUT );

    return result;
}

/*
 *  DFU_GETSTATUS Request (DFU Spec 1.1, Section 6.1.2)
 *
 *  device    - the dfu device to commmunicate with
 *  status    - the data structure to be populated with the results
 *
 *  return the 0 if successful or < 0 on an error
 */
int32_t dfu_get_status( dfu_device *device, dfu_status *status )
{
    char buffer[6];
    int32_t result;
	struct timespec req;
	
    if( (NULL == device) || (NULL == device->handle) ) {
        return -1;
    }

    /* Initialize the status data structure */
//     status->bStatus       = DFU_STATUS_ERROR_UNKNOWN;
//     status->bwPollTimeout = 0;
//     status->bState        = STATE_DFU_ERROR;
//     status->iString       = 0;
	
	status->bStatus       = -1;
	status->bwPollTimeout = -1;
	status->bState        = -1;
	status->iString       = -1;

    result = libusb_control_transfer( device->handle,
          /* bmRequestType */ LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
          /* bRequest      */ DFU_GETSTATUS,
          /* wValue        */ 0,
          /* wIndex        */ device->interface,
          /* Data          */ buffer,
          /* wLength       */ 6,
                              DFU_TIMEOUT );

    if( 6 == result ) {
        status->bStatus = buffer[0];
        status->bwPollTimeout = ((0xff & buffer[3]) << 16) |
                                ((0xff & buffer[2]) << 8)  |
                                (0xff & buffer[1]);

        status->bState  = buffer[4];
        status->iString = buffer[5];
		
		#if STMDFU_DEBUG_PRINTFS
		printf("Status:<%d:%s>\tWait:<%d>\tState:<%d:%s>\tidx:<%d>\n",
				status->bStatus, dfu_status_to_string(status->bStatus),
				status->bwPollTimeout,
				status->bState, dfu_state_to_string(status->bState),
				status->iString
				);
		#endif
		
		if (status->bwPollTimeout != 0)
		{
			req.tv_sec = 0;
			req.tv_nsec = status->bwPollTimeout * 1000000;
			if (0 > nanosleep(&req, NULL))
			{
				printf("dfu_get_status: nanosleep failed");
			}
		}
				
    } else {
        if( 0 < result ) {
            /* There was an error, we didn't get the entire message. */
            return -2;
        }
    }

    return 0;
}

/*
 *  DFU_CLRSTATUS Request (DFU Spec 1.1, Section 6.1.3)
 *
 *  device    - the dfu device to commmunicate with
 *
 *  return 0 or < 0 on an error
 */
int32_t dfu_clear_status( dfu_device *device )
{
    int32_t result;

    if( (NULL == device) || (NULL == device->handle) ) {
        return -1;
    }

    result = libusb_control_transfer( device->handle,
          /* bmRequestType */ LIBUSB_ENDPOINT_OUT| LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
          /* bRequest      */ DFU_CLRSTATUS,
          /* wValue        */ 0,
          /* wIndex        */ device->interface,
          /* Data          */ NULL,
          /* wLength       */ 0,
                              DFU_TIMEOUT );

    return result;
}

/*
 *  DFU_GETSTATE Request (DFU Spec 1.1, Section 6.1.5)
 *
 *  device    - the dfu device to commmunicate with
 *
 *  returns the state or < 0 on error
 */
int32_t dfu_get_state( dfu_device *device )
{
    int32_t result;
    char buffer[1];

    if( (NULL == device) || (NULL == device->handle) ) {
        return -1;
    }

    result = libusb_control_transfer( device->handle,
          /* bmRequestType */ LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
          /* bRequest      */ DFU_GETSTATE,
          /* wValue        */ 0,
          /* wIndex        */ device->interface,
          /* Data          */ buffer,
          /* wLength       */ 1,
                              DFU_TIMEOUT );

    /* Return the error if there is one. */
    if( result < 1 ) {
        return result;
    }

    /* Return the state. */
    return buffer[0];
}

/*
 *  DFU_ABORT Request (DFU Spec 1.1, Section 6.1.4)
 *
 *  device    - the dfu device to commmunicate with
 *
 *  returns 0 or < 0 on an error
 */
int32_t dfu_abort( dfu_device *device )
{
    int32_t result;

    if( (NULL == device) || (NULL == device->handle) ) {
        return -1;
    }

    result = libusb_control_transfer( device->handle,
          /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
          /* bRequest      */ DFU_ABORT,
          /* wValue        */ 0,
          /* wIndex        */ device->interface,
          /* Data          */ NULL,
          /* wLength       */ 0,
                              DFU_TIMEOUT );

    return result;
}

/*
 *  Used to convert the DFU state to a string.
 *
 *  state - the state to convert
 *
 *  returns the state name or "unknown state"
 */
char* dfu_state_to_string( const int32_t state )
{
    char *message = "unknown state";

    switch( state ) {
        case STATE_APP_IDLE:
            message = "appIDLE";
            break;
        case STATE_APP_DETACH:
            message = "appDETACH";
            break;
        case STATE_DFU_IDLE:
            message = "dfuIDLE";
            break;
        case STATE_DFU_DOWNLOAD_SYNC:
            message = "dfuDNLOAD-SYNC";
            break;
        case STATE_DFU_DOWNLOAD_BUSY:
            message = "dfuDNBUSY";
            break;
        case STATE_DFU_DOWNLOAD_IDLE:
            message = "dfuDNLOAD-IDLE";
            break;
        case STATE_DFU_MANIFEST_SYNC:
            message = "dfuMANIFEST-SYNC";
            break;
        case STATE_DFU_MANIFEST:
            message = "dfuMANIFEST";
            break;
        case STATE_DFU_MANIFEST_WAIT_RESET:
            message = "dfuMANIFEST-WAIT-RESET";
            break;
        case STATE_DFU_UPLOAD_IDLE:
            message = "dfuUPLOAD-IDLE";
            break;
        case STATE_DFU_ERROR:
            message = "dfuERROR";
            break;
    }

    return message;
}

/*
 *  Used to convert the DFU status to a string.
 *
 *  status - the status to convert
 *
 *  returns the status name or "unknown status"
 */
char* dfu_status_to_string( const int32_t status )
{
    char *message = "unknown status";

    switch( status ) {
        case DFU_STATUS_OK:
            message = "OK";
            break;
        case DFU_STATUS_ERROR_TARGET:
            message = "errTARGET";
            break;
        case DFU_STATUS_ERROR_FILE:
            message = "errFILE";
            break;
        case DFU_STATUS_ERROR_WRITE:
            message = "errWRITE";
            break;
        case DFU_STATUS_ERROR_ERASE:
            message = "errERASE";
            break;
        case DFU_STATUS_ERROR_CHECK_ERASED:
            message = "errCHECK_ERASED";
            break;
        case DFU_STATUS_ERROR_PROG:
            message = "errPROG";
            break;
        case DFU_STATUS_ERROR_VERIFY:
            message = "errVERIFY";
            break;
        case DFU_STATUS_ERROR_ADDRESS:
            message = "errADDRESS";
            break;
        case DFU_STATUS_ERROR_NOTDONE:
            message = "errNOTDONE";
            break;
        case DFU_STATUS_ERROR_FIRMWARE:
            message = "errFIRMWARE";
            break;
        case DFU_STATUS_ERROR_VENDOR:
            message = "errVENDOR";
            break;
        case DFU_STATUS_ERROR_USBR:
            message = "errUSBR";
            break;
        case DFU_STATUS_ERROR_POR:
            message = "errPOR";
            break;
        case DFU_STATUS_ERROR_UNKNOWN:
            message = "errUNKNOWN";
            break;
        case DFU_STATUS_ERROR_STALLEDPKT:
            message = "errSTALLEDPKT";
            break;

    }

    return message;
}