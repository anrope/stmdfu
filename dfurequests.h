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

#ifndef __DFU_H__
#define __DFU_H__

/* Wait for 10 seconds before a timeout since erasing/flashing can take some time. */
#define DFU_TIMEOUT 2500

/* Time (in ms) for the device to wait for the usb reset after being told to detach
* before the giving up going into dfu mode. */
#define DFU_DETACH_TIMEOUT 1000

//DFU interface constants
#define DFU_ITF_CLASS 0xfe
#define DFU_ITF_SUBCLASS 0x01
#define DFU_ITF_PROTOCOL 0x02

/* DFU commands */
#define DFU_DETACH      0
#define DFU_DNLOAD      1
#define DFU_UPLOAD      2
#define DFU_GETSTATUS   3
#define DFU_CLRSTATUS   4
#define DFU_GETSTATE    5
#define DFU_ABORT       6

/* DFU states */
#define STATE_APP_IDLE                  0x00
#define STATE_APP_DETACH                0x01
#define STATE_DFU_IDLE                  0x02
#define STATE_DFU_DOWNLOAD_SYNC         0x03
#define STATE_DFU_DOWNLOAD_BUSY         0x04
#define STATE_DFU_DOWNLOAD_IDLE         0x05
#define STATE_DFU_MANIFEST_SYNC         0x06
#define STATE_DFU_MANIFEST              0x07
#define STATE_DFU_MANIFEST_WAIT_RESET   0x08
#define STATE_DFU_UPLOAD_IDLE           0x09
#define STATE_DFU_ERROR                 0x0a


/* DFU status */
#define DFU_STATUS_OK                   0x00
#define DFU_STATUS_ERROR_TARGET         0x01
#define DFU_STATUS_ERROR_FILE           0x02
#define DFU_STATUS_ERROR_WRITE          0x03
#define DFU_STATUS_ERROR_ERASE          0x04
#define DFU_STATUS_ERROR_CHECK_ERASED   0x05
#define DFU_STATUS_ERROR_PROG           0x06
#define DFU_STATUS_ERROR_VERIFY         0x07
#define DFU_STATUS_ERROR_ADDRESS        0x08
#define DFU_STATUS_ERROR_NOTDONE        0x09
#define DFU_STATUS_ERROR_FIRMWARE       0x0a
#define DFU_STATUS_ERROR_VENDOR         0x0b
#define DFU_STATUS_ERROR_USBR           0x0c
#define DFU_STATUS_ERROR_POR            0x0d
#define DFU_STATUS_ERROR_UNKNOWN        0x0e
#define DFU_STATUS_ERROR_STALLEDPKT     0x0f


/* This is based off of DFU_GETSTATUS
 *
 *  1 unsigned byte bStatus
 *  3 unsigned byte bwPollTimeout
 *  1 unsigned byte bState
 *  1 unsigned byte iString
*/

typedef struct {
    uint8_t bStatus;
    uint32_t bwPollTimeout;
    uint8_t bState;
    uint8_t iString;
} dfu_status;

typedef struct {
	struct libusb_device_handle *handle;
	int32_t interface;
} dfu_device;

/*
*  DFU_DETACH Request (DFU Spec 1.1, Section 5.1)
*
*  device    - the dfu device to commmunicate with
*  timeout   - the timeout in ms the USB device should wait for a pending
*              USB reset before giving up and terminating the operation
*
*  returns 0 or < 0 on error
*/
int32_t dfu_detach( dfu_device *device, const int32_t timeout );

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
int32_t dfu_download(dfu_device *device, int32_t wvalue, uint8_t* data, int32_t length);

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
int32_t dfu_upload(dfu_device *device, int32_t wvalue, uint8_t* data, int32_t length);

/*
*  DFU_GETSTATUS Request (DFU Spec 1.1, Section 6.1.2)
*
*  device    - the dfu device to commmunicate with
*  status    - the data structure to be populated with the results
*
*  return the 0 if successful or < 0 on an error
*/
int32_t dfu_get_status( dfu_device *device, dfu_status *status );

/*
*  DFU_CLRSTATUS Request (DFU Spec 1.1, Section 6.1.3)
*
*  device    - the dfu device to commmunicate with
*
*  return 0 or < 0 on an error
*/
int32_t dfu_clear_status( dfu_device *device );

/*
*  DFU_GETSTATE Request (DFU Spec 1.1, Section 6.1.5)
*
*  device    - the dfu device to commmunicate with
*
*  returns the state or < 0 on error
*/
int32_t dfu_get_state( dfu_device *device );

/*
*  DFU_ABORT Request (DFU Spec 1.1, Section 6.1.4)
*
*  device    - the dfu device to commmunicate with
*
*  returns 0 or < 0 on an error
*/
int32_t dfu_abort( dfu_device *device );

/*
*  Used to convert the DFU state to a string.
*
*  state - the state to convert
*
*  returns the state name or "unknown state"
*/
char* dfu_status_to_string( const int32_t status );

/*
*  Used to convert the DFU status to a string.
*
*  status - the status to convert
*
*  returns the status name or "unknown status"
*/
char* dfu_state_to_string( const int32_t state );
#endif
