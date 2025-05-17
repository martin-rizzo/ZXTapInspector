#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "zxdetoke.h"

#if !defined(TRUE) && !defined(FALSE)
#   define TRUE  1
#   define FALSE 0
#   define BOOL int
#   define BYTE unsigned char
#endif

/**
 * Macro to check the arguments passed by command line
 * @param arg   The argument to compare.
 * @param str1  The first string to compare against.
 * @param str2  The second string to compare against.
 * @return True if `arg` matches either `str1` or `str2`, otherwise false.
 */
#define ARG_EQ(arg, str1, str2) (strcmp(arg, str1) == 0 || strcmp(arg, str2) == 0)

/**
 * Macro to extract a 16-bit unsigned integer from a byte stream.
 * @param ptr    A pointer to the byte stream data.
 * @param index  The starting index (in bytes) from which to read the 16-bit word.
 * @return       A 16-bit unsigned integer formed by combining the two bytes.
 */
#define WORD_FROM_PTR(ptr, index) ( (((unsigned char *)ptr)[index]) | (((unsigned char *)ptr)[index+1] << 8) )

/**
 * A binary code block in memory
 */
typedef struct _BinaryCode {
    unsigned int start;        /**< Start address of the code block in memory */
    unsigned int end;          /**< End address of the code block in memory */
    unsigned int entry_point;  /**< Entry point address of the code block in memory */
    unsigned int datasize;     /**< Size of the data array in bytes */
    char         data[1];      /**< Flexible array containing the actual binary code data */
} BinaryCode;


/*---------------------------- COLORED OUTPUT -----------------------------*/

/**
 * ANSI escape codes for colored terminal output
 */
char RED[]     = "\033[91m";
char GREEN[]   = "\033[92m";
char YELLOW[]  = "\033[93m";
char CYAN[]    = "\033[96m";
char DKGRAY[]  = "\033[90m";
char NOCOLOR[] = "\033[0m";

/**
 * Disables color output by resetting all color codes to empty strings
 */
void disable_colors() {
    RED[0]     = '\0';
    GREEN[0]   = '\0';
    YELLOW[0]  = '\0';
    CYAN[0]    = '\0';
    DKGRAY[0]  = '\0';
    NOCOLOR[0] = '\0';
}

/*---------------------------- ERROR MESSAGES -----------------------------*/

/**
 * Prints an error message with colored formatting.
 * @param err_type   The type of error (e.g., "Error", "Warning").
 * @param err_color  The color code for the error type.
 * @param bra_color  The color code for the brackets enclosing the error type.
 * @param tex_color  The color code for the text content.
 * @param format     The format string for the error message.
 * @param vlist      The variable arguments list.
 */
void print_colored_error(const char *err_type,
                         const char *err_color,
                         const char *bra_color,
                         const char *tex_color,
                         const char *format, va_list vlist) {
    static char text[1024];  /**< static buffer to store formatted text */

    /* format the variable arguments into the text buffer and print the colored error */
    vsnprintf(text, sizeof(text), format, vlist);
    fprintf(stderr, "\n%s[%s%s%s]%s %s\n", bra_color, err_color, err_type, bra_color, tex_color, text);
}

/**
 * Displays a warning message to stderr with color formatting
 * @param message The main warning message
 * @param ... Optional additional informational messages
 */
void warning(const char *message, ...) {
    char *info; va_list args;
    va_start(args, message);
    print_colored_error("WARNING", YELLOW, CYAN, NOCOLOR, message, args);
    va_end(args);
}

/**
 * Displays an error message to stderr with color formatting
 * @param message The main error message
 * @param ...     Optional additional informational messages
 */
void error(const char *message, ...) {
    char *info; va_list args;
    va_start(args, message);
    print_colored_error("ERROR", RED, CYAN, NOCOLOR, message, args);
    va_end(args);
}

/**
 * Displays a fatal error message to stderr with color formatting and exits
 * @param message The main fatal error message
 * @param ... Optional additional informational messages
 */
void fatal_error(const char *message, ...) {
    char *info; va_list args;
    va_start(args, message);
    print_colored_error("ERROR", RED, CYAN, NOCOLOR, message, args);
    va_end(args);
    exit(1);
}

void print_help(char *argv[]) {
    printf("Usage: %s [options] <filename>\n", argv[0]);
}

/*------------------------------ .TAP LOADER ------------------------------*/

#define ZX_HEADER_SIZE 17
#define ZX_HEADER_FLAG 0x00
#define ZX_DATA_FLAG   0xFF

/**
 * A ZX-Spectrum TAP file block
 */
typedef struct _ZXTapBlock {
    unsigned flag;         /**< Control byte indicating block type (00 for headers, FF for data blocks) */
    unsigned checksum;     /**< 8-bit checksum of the data block for error detection */
    unsigned datasize;     /**< Size of the data array in bytes */
    BYTE     data[1];      /**< Flexible array containing the actual block data */
} ZXTapBlock;

/**
 * Header types for ZX-Spectrum TAP file blocks
 */
typedef enum _ZXHeaderType {
    BASIC_PROGRAM   = 0,  /**< The block contains a BASIC program */
    NUMBER_ARRAY    = 1,  /**< The block contains a number array */
    CHARACTER_ARRAY = 2,  /**< The block contains a character array */
    BINARY_CODE     = 3   /**< The block contains binary code (machine language) */
} ZXHeaderType;


/**
 * Converts a ZXHeaderType value to its corresponding string representation.
 * @param type     The ZXHeaderType enum value to convert.
 * @param buffer64 A buffer of at least 64 characters.
 * @return The string representation of the header type.
 */
const char *get_zx_header_type_name(ZXHeaderType type, char* buffer64) {
    /* name deberia ser un buffer de 64 characters */
    assert( buffer64 != NULL );
    switch(type) {
        case BASIC_PROGRAM:   return "BASIC-PROGRAM";
        case NUMBER_ARRAY:    return "NUMBER-ARRAY";
        case CHARACTER_ARRAY: return "CHARACTER-ARRAY";
        case BINARY_CODE:     return "BINARY-CODE";
    }
    sprintf(buffer64, "UNKNOWN(%d)", type);
    return buffer64;
}

/**
 * The information in a ZX-Spetrum TAP block header
 */
typedef struct _ZXHeaderInfo {
    ZXHeaderType type;         /**< Type of the block (e.g., BASIC_PROGRAM, BINARY_CODE, ..) */
    char         filename[12]; /**< filename (null-terminated) */
    unsigned     length;       /**< Length of the program/data in bytes */
    unsigned     param1;       /**< Additional parameter 1 (specific to the block type) */
    unsigned     param2;       /**< Additional parameter 2 (specific to the block type) */
} ZXHeaderInfo;


/**
 * Reads a ZX Spectrum TAP file block from a file
 * @param tap_file The file pointer to the ZX Spectrum TAP file
 * @return A pointer to the allocated ZXTapBlock structure on success, or NULL on failure
 * @note   The caller is responsible for freeing the allocated memory using free()
 */
ZXTapBlock* read_zx_tap_block(FILE* tap_file) {
    ZXTapBlock* block = NULL;
    unsigned block_length, datasize;
    BYTE length[2], flag, checksum;
    BOOL error = FALSE;
    
    /* read the length of the block (2 bytes) */
    error        = fread(length, sizeof(length), 1, tap_file) != 1;
    block_length = WORD_FROM_PTR(length, 0);
    datasize     = block_length - 2;

    /* read the spectrum generated data (flag + data + checksum) */
    block = malloc(sizeof(ZXTapBlock) + datasize);
    error = error || block == NULL;
    error = error || fread(&flag      , sizeof(BYTE),        1, tap_file) != 1;
    error = error || fread(block->data, sizeof(BYTE), datasize, tap_file) != datasize;
    error = error || fread(&checksum  , sizeof(BYTE),        1, tap_file) != 1;

    /* if there was an error then return NULL */
    if( error ) { free(block); return NULL; }

    /* set the block properties and return it */
    block->flag     = flag;
    block->checksum = checksum;
    block->datasize = datasize;
    return block;
}

/**
 * Parses header information from a ZX-Spectrum TAP block
 * @param[out] header Pointer to the ZXHeaderInfo structure to store parsed data.
 * @param[in]  block  Pointer to the ZXTapBlock containing the header data.
 * @return TRUE if the block is a valid header block and parsing succeeds, FALSE otherwise.
 */
BOOL parse_zx_header_info(ZXHeaderInfo *header, const ZXTapBlock *block) {
    assert( header!=NULL );
    if( block==NULL ) {
        return FALSE;
    }
    if( block->flag != ZX_HEADER_FLAG || block->datasize != ZX_HEADER_SIZE ) {
        return FALSE;
    }
    header->type = block->data[0];
    memcpy( header->filename, &block->data[1], 10);
    header->filename[10] = header->filename[11] = '\0';
    header->length = WORD_FROM_PTR(block->data, 11);
    header->param1 = WORD_FROM_PTR(block->data, 13);
    header->param2 = WORD_FROM_PTR(block->data, 15);
    return TRUE;
}

/**
 * Prints the header information of a ZX-Spectrum TAP block
 * @param header Pointer to the ZXHeaderInfo structure containing the header data.
 */
void print_zx_header_info(ZXHeaderInfo *header, int block_index, BOOL one_line) {
    char buffer64[64];
    if( one_line ) {
        printf("%3d:%-10s %-16s %6d %6d %6d\n",
               block_index,
               header->filename,
               get_zx_header_type_name(header->type, buffer64),
               header->length,
               header->param1,
               header->param2
        );
    } else {
        printf("Header Info:\n");
        printf("----------------------------\n");
        printf("Type:        %s\n", get_zx_header_type_name(header->type, buffer64));
        printf("Filename:    %s\n", header->filename);
        printf("Length:      %u\n", header->length);
        printf("Param1:      %u\n", header->param1);
        printf("Param2:      %u\n", header->param2);
        printf("----------------------------\n");
    }
}

/**
 * Prints the information of a ZX-Spectrum TAP block
 * @param block Pointer to the ZXTapBlock structure containing the block data.
 */
void print_zx_block_info(ZXTapBlock *block, int block_index, BOOL one_line) {
    if( one_line ) {
        printf("%3d:%-10s %-16s %6d\n", 
            block_index,
            "",
            "_data",
            block->datasize
        );
    }
    else {
        printf("Block Info:\n");
        printf("----------------------------\n");
        printf("Flag:        %u\n", block->flag);
        printf("Checksum:    %u\n", block->checksum);
        printf("Data Size:   %u\n", block->datasize);
        printf("----------------------------\n");
    }
}

/*------------------------------ SUB-COMMANDS ------------------------------*/

/**
 * Prints ZX-Spectrum TAP file blocks to standard output
 * @param tap_file Pointer to the TAP file stream (must be opened in read mode).
 * @param one_line Flag indicating whether to print information in a single line format.
 *                 If TRUE, output is compact; if FALSE, output is detailed.
 * @return 0 on successful completion. 
 */
int print_zx_tap_blocks(FILE* tap_file, BOOL one_line) {
    ZXTapBlock  *block;
    ZXHeaderInfo header;
    BOOL is_valid_block;
    int block_index;

    block_index    = 0;
    is_valid_block = TRUE;
    while( is_valid_block ) {
        block          = read_zx_tap_block(tap_file);
        is_valid_block = (block != NULL);
        if( is_valid_block ) {
            if( parse_zx_header_info(&header, block) ) { print_zx_header_info(&header, block_index, one_line); }
            else                                       { print_zx_block_info(block, block_index, one_line);    }
        }
        free( block ); block = NULL; ++block_index;
    }
    return 0;
}

/**
 * Searches for and detokenizes a specific ZX Spectrum BASIC program block from a TAP file.
 * @param tap_file          Pointer to the TAP file stream (must be opened in read mode).
 * @param selected_filename Optional filename to match against block filenames.
 *                          If provided, the function will search for a block with this filename.
 * @param selected_index    Optional index to match against block positions.
 *                          If provided, the function will search for a block at this index.
 * @return 0 on success, or an error code otherwise.
 */
int print_zx_basic_program(FILE* tap_file, const char* selected_filename, int selected_index) {
    ZXTapBlock  *block;
    ZXHeaderInfo header;
    int          block_index;
    BOOL is_block_valid, found;

    /* loop through all TAP blocks until the target block is found */
    block_index    = 0;
    is_block_valid = TRUE;
    found          = FALSE;
    while( is_block_valid && !found )
    {
        /* read next block from TAP file */
        block          = read_zx_tap_block(tap_file);
        is_block_valid = (block != NULL);

        /* parse header information if block is valid */
        if( is_block_valid && parse_zx_header_info(&header, block) )
        {
            /* check if the current block matches the selected criteria */
            if( selected_filename!=NULL ) {
                found = strcmp(header.filename, selected_filename)==0;
            } else if( selected_index>=0 ) {
                found = block_index==selected_index || (block_index+1)==selected_index;
            } else {
                found = header.type==BASIC_PROGRAM;
            }
        }

        free( block ); block = NULL; ++block_index;
    }

    /* handle error cases */
    if( !found  )
    { error("No BASIC program found"); return 1; }
    if( found && header.type!=BASIC_PROGRAM )
    { error("Selected block is not a BASIC program"); return 1; }

    /* read the actual data block after header */
    block = read_zx_tap_block(tap_file);
    if( block==NULL )
    { error("Error reading BASIC program, no data block found"); return 1; }

    /* print the detokenized BASIC program and return */
    return detokenize_zx_basic_program(block->data, block->datasize);
}

/**
 * Converts ZX-Spectrum TAP file to HEX Intel format
 * @param output_filename  The name of the output file where the converted HEX data will be written.
 *                         If the file already exists, it will be overwritten. 
 * @param tap_file         Pointer to the TAP file stream (must be opened in read mode).
 * @return 0 on successful completion. 
 */
int convert_zx_tap_to_hex(const char* output_filename, FILE* tap_file) {
    fatal_error( "Not implemented yet" );
    return 0;
}

/*===========================================================================
/////////////////////////////////// MAIN ////////////////////////////////////
===========================================================================*/

/**
 * Main function to process command-line arguments and handle file operations.
 * 
 * This function processes command-line arguments to check for help, version, and filename.
 * It validates the input and performs the necessary actions based on the provided flags.
 * 
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, 1 on error.
 */
int main(int argc, char *argv[]) {
    int  i;
    FILE *file = NULL;
    char filename[1024] = "";
    int  non_flag_count = 0;
    char *arg;
    BinaryCode *code;
    int errcode = 0;
    enum { CMD_HELP, CMD_VERSION, CMD_LIST, CMD_TO_HEX, CMD_PRINT, CMD_BASIC } cmd;

    /* check if at least one parameter is provided */
    if (argc < 2) {
        error("No parameters were provided.");
        print_help(argv);
        return 1;
    }

    /* process each argument */
    cmd = CMD_TO_HEX;
    for(i = 1; i < argc; i++) {
        arg = argv[i];
        if( arg[0] == '-' ) {
            if      (ARG_EQ(arg, "-l", "--list"   )) { cmd = CMD_LIST;    }
            else if (ARG_EQ(arg, "-p", "--print"  )) { cmd = CMD_PRINT;   }
            else if (ARG_EQ(arg, "-b", "--basic"  )) { cmd = CMD_BASIC;   }
            else if (ARG_EQ(arg, "-h", "--help"   )) { cmd = CMD_HELP;    }
            else if (ARG_EQ(arg, "-v", "--version")) { cmd = CMD_VERSION; }
            else {
                fatal_error( "Unknown flag '%s'", arg );
            }
        }
        else {
            /* assume this parameter is the filename */
            strncpy(filename, arg, sizeof(filename)-1);
            filename[sizeof(filename)-1] = '\0';
            non_flag_count++;
        }
    }

    /* handle help & version commands */
    switch( cmd ) {
        case CMD_HELP:
            print_help(argv);
            return 0;
        case CMD_VERSION:
            printf("Version 0.1.0\n");
            return 0;
        default:
            break;
    }

    /* check that exactly one filename was provided */
    if( non_flag_count != 1 ) {
        fatal_error("Exactly one filename was expected");
    }

    /* proceed with file operations based on the selected command */
    errcode = 0;
    file    = fopen(filename, "rb");
    if( !file ) { fatal_error("Failed to open file '%s'", filename); }
    switch( cmd ) {
        case CMD_LIST:
            errcode = print_zx_tap_blocks(file, TRUE);
            break;
        case CMD_PRINT:
            errcode = print_zx_tap_blocks(file, FALSE);
            break;
        case CMD_BASIC:
            errcode = print_zx_basic_program(file, NULL, -1);
            break;
        case CMD_TO_HEX:
            errcode = convert_zx_tap_to_hex("output.hex", file);
            break;
        default:
            fatal_error( "Unknown command '%d'", cmd );
    }
    fclose(file);
    return errcode;
}

