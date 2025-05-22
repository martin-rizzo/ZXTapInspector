/*
| File    : zxs_tap.h
| Purpose : Helper functions to read ZX-Spectrum TAP files
| Author  : Martin Rizzo | <martinrizzo@gmail.com>
| Repo    : https://github.com/martin-rizzo/ZXTapInspector
| License : MIT
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|                               ZXTapInspector
|           A simple CLI tool for inspecting ZX-Spectrum TAP files
\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _*/
#ifndef ZXS_TAP_H
#define ZXS_TAP_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

/** Size of header blocks in the ZX-Spectrum TAP file (in bytes) */
#define ZXS_HEADER_SIZE 17

/**
 * Block types for ZX-Spectrum TAP file blocks
 */
typedef enum ZXS_BLKTYPE {
    ZXS_BLKTYPE_HEADER = 0x00, /**< Header block containing information about the program/data  */
    ZXS_BLKTYPE_DATA   = 0xFF  /**< Data block containing actual program/data */
} ZXS_BLKTYPE;

/**
 * Data types for ZX-Spectrum TAP file blocks
 */
typedef enum ZXS_DATATYPE {
    ZXS_DATATYPE_ANY     = 0xFF, /**< Any type of data */
    ZXS_DATATYPE_BASIC   = 0,    /**< The block contains a BASIC program */
    ZXS_DATATYPE_NUMBERS = 1,    /**< The block contains a number array */
    ZXS_DATATYPE_STRINGS = 2,    /**< The block contains a character array */
    ZXS_DATATYPE_CODE    = 3     /**< The block contains binary code (machine language) */
} ZXS_DATATYPE;

/**
 * A ZX-Spectrum TAP file block
 */
typedef struct ZXSTapBlock {
    ZXS_BLKTYPE type;         /**< Block type (00 for headers, FF for data blocks) */
    unsigned    checksum;     /**< 8-bit checksum of the data block for error detection */
    unsigned    datasize;     /**< Size of the data array in bytes */
    BYTE        data[1];      /**< Flexible array containing the actual block data */
} ZXSTapBlock;

/**
 * The information in a ZX-Spetrum TAP block header
 */
typedef struct ZXSHeader {
    ZXS_DATATYPE datatype;     /**< Type of data contained in the block (e.g., BASIC, NUMBERS, STRINGS, CODE) */
    char         filename[12]; /**< filename (null-terminated) */
    unsigned     length;       /**< Length of the program/data in bytes */
    unsigned     param1;       /**< Additional parameter 1 (specific to the block type) */
    unsigned     param2;       /**< Additional parameter 2 (specific to the block type) */
} ZXSHeader;

/**
 * Converts a ZXS_DATATYPE value to its corresponding string representation.
 * @param datatype  The ZXS_DATATYPE value to convert.
 * @param buffer64  A buffer of at least 64 characters.
 * @return The string representation of the data type.
 */
const char *zxs_get_datatype_name(ZXS_DATATYPE datatype, char* buffer64) {
    const char* name;
    assert( buffer64 != NULL );
    switch( datatype ) {
        case ZXS_DATATYPE_BASIC:   name = "BASIC-PROGRAM"; break;
        case ZXS_DATATYPE_NUMBERS: name = "NUMBER-ARRAY" ; break;
        case ZXS_DATATYPE_STRINGS: name = "STRING-ARRAY" ; break;
        case ZXS_DATATYPE_CODE:    name = "CODE"         ; break;
        default:
            sprintf(buffer64, "UNKNOWN(%d)", datatype);
            name = buffer64;
    }
    return name;
}

/**
 * Reads a ZX-Spectrum TAP file block from a file
 * @param tap_file The file pointer to the ZX Spectrum TAP file
 * @return A pointer to the allocated ZXSTapBlock structure on success, or NULL on failure
 * @note   The caller is responsible for freeing the allocated memory using free()
 */
ZXSTapBlock* zxs_read_tap_block(FILE* tap_file) {
    ZXSTapBlock* block = NULL;
    unsigned block_length, datasize;
    BYTE length[2], type, checksum;
    BOOL error = FALSE;
    
    /* read the length of the block (2 bytes) */
    error        = fread(length, sizeof(length), 1, tap_file) != 1;
    block_length = GET_LE_WORD(length, 0);
    datasize     = block_length - 2;

    /* read the spectrum generated data (flag + data + checksum) */
    block = malloc(sizeof(ZXSTapBlock) + datasize);
    error = error || block == NULL;
    error = error || fread(&type      , sizeof(BYTE),        1, tap_file) != 1;
    error = error || fread(block->data, sizeof(BYTE), datasize, tap_file) != datasize;
    error = error || fread(&checksum  , sizeof(BYTE),        1, tap_file) != 1;

    /* if there was an error then return NULL */
    if( error ) { free(block); return NULL; }

    /* set the block properties and return it */
    block->type     = (ZXS_BLKTYPE)type;
    block->checksum = checksum;
    block->datasize = datasize;
    return block;
}

/**
 * Parses header information from a ZX-Spectrum TAP block
 * @param[out] header Pointer to the ZXSHeader structure to store parsed data.
 * @param[in]  block  Pointer to the ZXSTapBlock containing the header data.
 * @return TRUE if the block is a valid header block and parsing succeeds, FALSE otherwise.
 */
BOOL zxs_parse_header(ZXSHeader *header, const ZXSTapBlock *block) {
    int i;

    assert( header!=NULL );
    if( block==NULL ) {
        return FALSE;
    }
    if( block->type != ZXS_BLKTYPE_HEADER || block->datasize != ZXS_HEADER_SIZE ) {
        return FALSE;
    }
    header->datatype = block->data[0];
    memcpy( header->filename, &block->data[1], 10);
    header->filename[11] =  header->filename[10] = '\0';
    for( i=9; i>=0 && header->filename[i]==' '; i--) {
        header->filename[i] = '\0';
    }
    header->length = GET_LE_WORD(block->data, 11);
    header->param1 = GET_LE_WORD(block->data, 13);
    header->param2 = GET_LE_WORD(block->data, 15);
    return TRUE;
}

#endif /* ZXS_TAP_H */