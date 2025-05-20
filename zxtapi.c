/*
| File    : zxtapi.c
| Purpose : Main file for the ZXTapInspector tool.
| Author  : Martin Rizzo | <martinrizzo@gmail.com>
| Repo    : https://github.com/martin-rizzo/ZXTapInspector
| License : MIT
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|                               ZXTapInspector
|           A simple CLI tool for inspecting ZX-Spectrum TAP files
|
|    Copyright (c) 2025 Martin Rizzo
|
|    Permission is hereby granted, free of charge, to any person obtaining
|    a copy of this software and associated documentation files (the
|    "Software"), to deal in the Software without restriction, including
|    without limitation the rights to use, copy, modify, merge, publish,
|    distribute, sublicense, and/or sell copies of the Software, and to
|    permit persons to whom the Software is furnished to do so, subject to
|    the following conditions:
|
|    The above copyright notice and this permission notice shall be
|    included in all copies or substantial portions of the Software.
|
|    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
|    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
|    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
|    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
|    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
|    TORT OR OTHERWISE, ARISING FROM,OUT OF OR IN CONNECTION WITH THE
|    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "common.h"
#include "zxs_bas.h"
#include "zxs_tap.h"
#include "fmt_hex.h"
const char HELP[] =
"Usage: zxtapi [OPTIONS] FILE.tap"                                                         "\n"
""                                                                                         "\n"
"Description:"                                                                             "\n"
"  ZXTapInspector (zxtapi) is a command-line tool for inspecting ZX Spectrum .tap files."  "\n"
"  It enables you to list blocks, view detailed block information, extract BASIC code,"    "\n"
"  and convert tape data into usable file formats."                                        "\n"
""                                                                                         "\n"
"Options:"                                                                                 "\n"
"  -l, --list"                                                                             "\n"
"        List all blocks contained in the specified .tap file."                            "\n"
""                                                                                         "\n"
"  -d, --detail"                                                                           "\n"
"        Display detailed information about each block (e.g., header data, sizes, types)." "\n"
""                                                                                         "\n"
"  -b, --basic"                                                                            "\n"
"        Output BASIC code stored within one or more blocks. This command detokenizes the binary""\n"
"        data to produce human-readable BASIC code."                                       "\n"
""                                                                                         "\n"
"  -x, --extract"                                                                          "\n"
"        Extract all blocks from the .tap file into separate files:"                       "\n"
"          • Basic code is saved as a .bas text file."                                     "\n"
"          • Machine code is converted to an Intel HEX (.hex) format."                     "\n"
"        The extracted files are placed in a folder named after the original tape file."   "\n"
""                                                                                         "\n"
"  -h, --help"                                                                             "\n"
"        Show this help message and exit."                                                 "\n"
""                                                                                         "\n"
"  -v, --version"                                                                          "\n"
"        Display version information."                                                     "\n"
""                                                                                         "\n"
"Examples:"                                                                                "\n"
"  zxtapi example.tap"                                                                     "\n"
"      List all blocks found within 'example.tap'."                                        "\n"
""                                                                                         "\n"
"  zxtapi -d example.tap"                                                                  "\n"
"      Show detailed block information for 'example.tap'."                                 "\n"
""                                                                                         "\n"
"  zxtapi -b example.tap"                                                                  "\n"
"      Output the BASIC code stored in 'example.tap'."                                     "\n"
""                                                                                         "\n"
"  zxtapi -x example.tap"                                                                  "\n"
"      Extract and convert all blocks from 'example.tap' into separate files."             "\n"
"\n";

/**
 * Macro to check the arguments passed by command line
 * @param arg   The argument to compare.
 * @param str1  The first string to compare against.
 * @param str2  The second string to compare against.
 * @return True if `arg` matches either `str1` or `str2`, otherwise false.
 */
#define ARG_EQ(arg, str1, str2) (strcmp(arg, str1) == 0 || strcmp(arg, str2) == 0)

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
    printf("%s", HELP);
}

/*------------------------------ SUB-COMMANDS ------------------------------*/

/**
 * Searches for a specific header in a TAP file based on optional criteria.
 * 
 * The criteria can be based on name, index, or header type. If multiple criteria are provided,
 * the function prioritizes name checks first, then index checks, and finally type checks.
 * 
 * @param header    Pointer to the ZXHeaderInfo where the matched header will be stored. 
 * @param tap_file  FILE pointer to the TAP file being processed.
 * @param name      Optional block name to match. If NULL, name-based filtering is skipped.
 * @param index     Optional block index to match. If -1, index-based filtering is skipped.
 * @param type      Header type to match. This parameter is only used if both name and index are omitted.
 * @return
 *    TRUE if a matching header is found, FALSE otherwise.
 */
BOOL find_header(ZXSHeader* header, FILE* tap_file, const char* name, int index, ZXS_DATATYPE type) {
    ZXSTapBlock *block;
    int          header_index;
    BOOL is_block_valid, found;

    /* loop through all TAP blocks until the target header is found */
    header_index   = 0;
    is_block_valid = TRUE; found = FALSE;
    while( is_block_valid && !found )
    {
        /* read next block from TAP file */
        block           = zxs_read_tap_block(tap_file);
        is_block_valid  = (block != NULL);
        if( is_block_valid && zxs_parse_header(header, block) ) {
            /* check if the current header matches the selected criteria */
            if( name             ) { found = (0==strcmp(header->filename, name)); }
            if( index>=0         ) { found = (header_index == index); }
            if( !name && index<0 ) { found = (header->datatype == type ); }
            ++header_index;
        }
        free( block ); block = NULL;
    }
    return found;
}

/**
 * Prints a formatted list of TAP blocks to the specified output file.
 * @param output    FILE pointer to the output stream where the block list will be printed.
 * @param tap_file  FILE pointer to the TAP file being processed.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_block_list(FILE* output, FILE* tap_file) {
    ZXSTapBlock *block;
    ZXSHeader    header;
    int          header_index, block_index;
    BOOL         is_block_valid;
    char buffer16[16];
    char buffer32[32];
    int  err_code = 0;

    /* loop through all TAP blocks */
    header_index   = 0;
    block_index    = 0;
    is_block_valid = TRUE;
    printf("IDX: name       : type         : Length : Param1 : Param2 \n");
    printf("---:------------:--------------:--------:--------:--------\n");
    while( is_block_valid )
    {
        /* read next block from TAP file */
        block           = zxs_read_tap_block(tap_file);
        is_block_valid  = (block != NULL);
        if( is_block_valid ) {
            if( zxs_parse_header(&header, block) )
            {
                sprintf(buffer16, "\"%s\"", header.filename);
                printf(" %02d:%-12s:%-14s %6d    %6d  %6d\n",
                       header_index, buffer16,
                       zxs_get_datatype_name(header.datatype, buffer32),
                       header.length, header.param1, header.param2
                       );
                ++header_index; block_index=0;
            }
            else {
                sprintf(buffer16, "//data%d", block_index);
                printf("    %-12s %-14s %6d\n", 
                       "", buffer16, block->datasize
                       );
            }
        }
        free( block ); block = NULL;
    }
    return err_code;
}

/**
 * Prints a detokenized ZX Spectrum BASIC program from a TAP file.
 * 
 * This function searches for a BASIC program block in a TAP file, reads its data,
 * and prints the detokenized BASIC program to the output file. It handles error cases
 * such as missing blocks or invalid block types.
 * 
 * @param output         File pointer to the output file where the BASIC program will be printed.
 * @param tap_file       File pointer to the TAP file containing the ZX Spectrum data.
 * @param selected_name  Optional block name to match. If NULL, name-based filtering is skipped.
 * @param selected_idx   Optional block index to match. If -1, index-based filtering is skipped.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_zx_basic_program(FILE* output, FILE* tap_file, const char* selected_name, int selected_idx) {
    ZXSHeader header; ZXSTapBlock *block; BOOL found;
    int err_code = 0;

    /* search for the selected BASIC program in the TAP file */
    found = find_header(&header, tap_file, selected_name, selected_idx, ZXS_DATATYPE_BASIC);

    /* handle error cases */
    if( !found  )
    { error("No BASIC program found"); return 1; }
    if( found && header.datatype!=ZXS_DATATYPE_BASIC )
    { error("Selected block is not a BASIC program"); return 1; }

    /* read the actual data block after header */
    block = zxs_read_tap_block(tap_file);
    if( block==NULL )
    { error("Error reading BASIC program, no data block found"); return 1; }

    /* print the detokenized BASIC program and return */
    err_code = zxs_fprint_basic_program(output, block->data, block->datasize);
    free(block); block=NULL;
    return err_code;
}

/**
 * Prints a binary code block from a TAP file in Intel HEX format.
 * 
 * This function searches for a binary code block in a TAP file, reads its data, and prints
 * the content in Intel HEX format. It supports optional filtering by filename or index.
 * 
 * @param output         File pointer to the output file where the binary data will be printed.
 * @param tap_file       File pointer to the TAP file containing the ZX Spectrum data.
 * @param selected_fn    Optional filename to match. If NULL, filename-based filtering is skipped.
 * @param selected_idx   Optional block index to match. If -1, index-based filtering is skipped.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_zx_binary_code(FILE* output, FILE* tap_file, const char* selected_fn, int selected_idx) {
    ZXSHeader header; ZXSTapBlock *block; BOOL found;
    int err_code = 0;

    /* search for the selected binary code in the TAP file */
    found = find_header(&header, tap_file, selected_fn, selected_idx, ZXS_DATATYPE_CODE);

    /* handle error cases */
    if( !found  )
    { error("No binary code found"); return 1; }
    if( found && header.datatype!=ZXS_DATATYPE_CODE )
    { error("Selected block is not a binary code"); return 1; }

    /* read the actual data block after header */
    block = zxs_read_tap_block(tap_file);
    if( block==NULL )
    { error("Error reading binary code, no data block found"); return 1; }

    /* print the binary code in Intel HEX format and return */
    err_code = fprint_hex_data(output, header.param1, block->data, block->datasize);
    free(block); block=NULL;
    return err_code;
}

int extract_zx_blocks(FILE* tap_file, const char* output_dirname, const char* selected_fn, int selected_idx) {
    ZXSTapBlock  *block;
    ZXSHeader header;
    int          header_index;
    BOOL is_block_valid, is_header_valid, found;
    int err_code = 0;

    /* loop through all TAP blocks extracting the selected ones */
    header_index   = 0;
    found          = FALSE;
    is_block_valid = TRUE;
    while( is_block_valid )
    {
        /* read next block from TAP file */
        block           = zxs_read_tap_block(tap_file);
        is_block_valid  = (block != NULL);
        is_header_valid = zxs_parse_header(&header, block);
        if( is_header_valid )
        {
            /* check if the current block matches the selected criteria */
            if     ( selected_fn!=NULL ) { found = strcmp(header.filename, selected_fn)==0; }
            else if( selected_idx>=0   ) { found = header_index==selected_idx;              }
            else                         { found = TRUE;                                    }

            /* placeholder for block extraction */
            if( found ) {
                printf("Extracting: %02d_%s\n", header_index, header.filename);
            }

            ++header_index;
        }
        free( block ); block = NULL;
    }
    return err_code;
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
    FILE *tap_file = NULL;
    char filename[1024] = "";
    int  non_flag_count = 0;
    char *arg;
    BinaryCode *code;
    int err_code = 0;
    enum { CMD_HELP, CMD_VERSION, CMD_LIST, CMD_DETAILS, CMD_BASIC, CMD_BINARY, CMD_EXTRACT } cmd;

    /* check if at least one parameter is provided */
    if (argc < 2) {
        error("No parameters were provided.");
        print_help(argv);
        return 1;
    }

    /* process each argument */
    cmd = CMD_LIST;
    for(i = 1; i < argc; i++) {
        arg = argv[i];
        if( arg[0] == '-' ) {
            if      (ARG_EQ(arg, "-l", "--list"   )) { cmd = CMD_LIST;    }
            else if (ARG_EQ(arg, "-d", "--detail" )) { cmd = CMD_DETAILS; }
            else if (ARG_EQ(arg, "-b", "--basic"  )) { cmd = CMD_BASIC;   }
            else if (ARG_EQ(arg, "-m", "--memory" )) { cmd = CMD_BINARY;  }
            else if (ARG_EQ(arg, "-m", "--binary" )) { cmd = CMD_BINARY;  }
            else if (ARG_EQ(arg, "-x", "--extract")) { cmd = CMD_EXTRACT; }
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
    err_code  = 0;
    tap_file = fopen(filename, "rb");
    if( !tap_file ) { fatal_error("Failed to open file '%s'", filename); }
    switch( cmd ) {
        case CMD_LIST:
            err_code = fprint_block_list(stdout, tap_file);
            break;
        case CMD_DETAILS:
            err_code = fprint_block_list(stdout, tap_file);
            break;
        case CMD_BASIC:
            err_code = fprint_zx_basic_program(stdout, tap_file, NULL, -1);
            break;
        case CMD_BINARY:
            err_code = fprint_zx_binary_code(stdout, tap_file, NULL, -1);
            break;
        case CMD_EXTRACT:
            err_code = extract_zx_blocks(tap_file, "./output", NULL, -1);
            break;
        default:
            fatal_error( "Unknown command '%d'", cmd );
    }
    fclose(tap_file);
    return err_code;
}

