/*
| File    : fmt_hex.h
| Purpose : Support for Intel HEX format.
| Author  : Martin Rizzo | <martinrizzo@gmail.com>
| Repo    : https://github.com/martin-rizzo/ZXTapInspector
| License : MIT
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|                               ZXTapInspector
|           A simple CLI tool for inspecting ZX-Spectrum TAP files
\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _*/
#ifndef FMT_HEX_H
#define FMT_HEX_H
#include <assert.h>
#include <stdio.h>
#include "common.h"
#define _HEX_MAX_BYTECOUNT (16) /**< Maximum number of bytes in a record */

/*----------------------- INTERNAL HELPER FUNCTIONS ------------------------*/

/**
 * Calculates the checksum for an Intel HEX record.
 * @param reg_type  Record type (0x00 for data record, 0x01 for EOF record, etc.)
 * @param address   16-bit memory address where the record is loaded
 * @param data      Pointer to the data bytes of the record
 * @param datasize  Number of data bytes in the record (must be <= 255)
 * @return
 *    The 8-bit checksum value (0x00-0xFF)
 */
int _hex_checksum(unsigned reg_type, unsigned address, const BYTE* data, unsigned datasize) {
    unsigned sum, i;
    assert( 0 <= reg_type && reg_type <= 255 );
    assert( 0 <= address && address <= 65535 );
    assert( data != NULL );
    assert( 0 <= datasize && datasize <= 255 );

    sum  = datasize & 0xFF;
    sum += (address >> 8) & 0xFF;
    sum += (address     ) & 0xFF;
    sum += reg_type & 0xFF;
    for ( i = 0 ; i < datasize ; ++i ) { sum += data[i]; }
    return (-sum) & 0xFF;
}

/**
 * Writes a data record to an Intel HEX file.
 * @param ofile     File pointer to the output file
 * @param address   16-bit memory address where the record is loaded
 * @param data      Pointer to the data bytes of the record
 * @param datasize  Number of data bytes in the record (must be <= 255)
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int _hex_fprint_data_record(FILE* ofile, unsigned address, const BYTE* data, unsigned datasize) {
    const unsigned reg_type = 0x00;
    unsigned i;

    /* write the record */
    fprintf(ofile, ":%02X%04X%02X", datasize, address, reg_type);
    for( i=0 ; i < datasize ; i++ ) { fprintf(ofile, "%02X", data[i]); }
    fprintf(ofile, "%02X", _hex_checksum(reg_type, address, data, datasize));
    return 0;
}

/**
 * Writes an EOF record to an Intel HEX file.
 * @param ofile  File pointer to the output file
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int _hex_fprint_eof_record(FILE* ofile) {
    const unsigned reg_type = 0x01;
    const unsigned address  = 0x0000;

    /* write the record */
    fprintf(ofile, ":%02X%04X%02X", 0x00, address, reg_type);
    fprintf(ofile, "%02X", _hex_checksum(reg_type, address, NULL, 0));
    return 0;
}

/*============================ PUBLIC FUNCTIONS ============================*/

/**
 * Prints binary data as Intel HEX records.
 * @param ofile     File pointer to the output file
 * @param address   16-bit memory address where the data is loaded
 * @param data      Pointer to the binary data to be printed
 * @param datasize  Number of bytes in the data
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_hex_data(FILE* ofile, unsigned address, const BYTE* data, unsigned datasize) {
    unsigned bytes_left, chunk_size, i;

    /* split data into chunks of _HEX_MAX_BYTECOUNT bytes and print each as a data record */
    for ( i = 0 ; i < datasize ; i += _HEX_MAX_BYTECOUNT ) {
        bytes_left = (datasize - i);
        chunk_size = bytes_left < _HEX_MAX_BYTECOUNT ? bytes_left : _HEX_MAX_BYTECOUNT;
        _hex_fprint_data_record(ofile, address + i, data + i, chunk_size);
        fprintf(ofile, "\n");
    }
    return 0;
}


#endif /* FMT_HEX_H */
