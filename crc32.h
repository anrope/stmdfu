/*
crc32.{c,h} :
Provides routines for calculating 32 bit Cyclic Redundancy Checks (CRCs).
DfuSe uses a CRC to verify the contents of the DfuSe file.
*/

/*
 * efone - Distributed internet phone system.
 *
 * (c) 1999,2000 Krzysztof Dabrowski
 * (c) 1999,2000 ElysiuM deeZine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

/* based on implementation by Finn Yannick Jacobs. */

#ifndef __DFU_CRC32__
#define __DFU_CRC32__

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
*		so make sure, you call it before using the other
*		functions!
*/
extern u_int32_t crc_tab[256];

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
*				calculate the crcTable for crc32-checksums.
*				it is generated to the polynom [..]
*/
void chksum_crc32gentab ();

/* chksum_crc32() -- to a given block, this one calculates the
*				crc32-checksum until the length is
*				reached. the crc32-checksum will be
*				the result.
*/
u_int32_t chksum_crc32 (unsigned char *block, unsigned int length);
#endif